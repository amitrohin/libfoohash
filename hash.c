#include <assert.h>
#include "foo/hash.h"

#ifndef lint
static const char rcsid[] __attribute__((unused)) = "$Id: hash.c,v 1.5 2023/04/07 12:12:38 swp Exp $";
#endif

extern int hash_chain_first(struct hash *hash, int hindex);
extern int hash_chain_next(struct hash *hash, int index);

/* Удаление элемента с индексом index из цепочки hindex. pindex - это
 * предидущий перед index элемент в цепочке или -1, если удаляемый элемент
 * первый.
 * Возвращается указатель data.
 */
static union hash_data hash_chain_remove(struct hash *hash, int hindex, int pindex, int index) {
    struct hash_item *item = hash->htab + index;
    assert(item->in_use);
    if (pindex != -1) {
        struct hash_item *pitem = hash->htab + pindex;
        assert(pitem->in_use);
        if (item->xoff)
            pitem->xoff = (pitem->xoff + item->xoff) % hash->cap;
        else
            pitem->xoff = 0;
    } else {
        struct hash_item *hitem = hash->htab + hindex;
        if (item->xoff)
            hitem->hoff = (hitem->hoff + item->xoff) % hash->cap;
        else
            hitem->hoff = 0;
    }
    item->in_use = 0;
    return item->data;
}
static int hash_getfreeitem(struct hash *hash, int hindex, int *lindex) {
    int index = -1;
    int li = -1;
    for (int len = 0; len < hash->chain_limit; len++) {
        int i = (hindex + len) % hash->cap;
        if (hash->htab[i].in_use) {
            if (hash->htab[i].hkey % hash->cap == hindex)
                li = i;
        } else {
            index = i;
            *lindex = li;
            break;
        }
    }
    return index;
}
/* Вставка в цепочку hindex элемента хэш-таблицы с индексом index. pindex -
 * это индекс элемента после которого делается вставка. hkey и data - хэш
 * данных и сами данные.
 * [!] Вставляемый элемент должен быть в состоянии in_use == 0.
 */
static void hash_chain_insert(
    struct hash *hash,
    int hindex,
    int pindex,
    int index,
    uint32_t hkey,
    union hash_data data
) {
    struct hash_item *item = hash->htab + index;
    assert(!item->in_use);
    if (pindex != -1) {
        struct hash_item *pitem = hash->htab + pindex;
        if (pitem->xoff)
            item->xoff = pindex + pitem->xoff - index;
        else
            item->xoff = 0;
        pitem->xoff = index - pindex;
    } else {
        struct hash_item *hitem = hash->htab + hindex;
        if (hitem->hoff)
            item->xoff = hindex + hitem->xoff - index;
        else
            item->xoff = 0;
        hitem->hoff = index - hindex;
    }
    item->hkey = hkey;
    item->data = data;
    item->in_use = 1;
}

struct hash *hash_create(struct hash_type const *type, int cap, int chain_limit) {
    struct hash *hash;

    if (cap < HASH_CAPMIN)
        cap = HASH_CAPMIN;
    hash = (type->malloc ? type->malloc : malloc)(
                offsetof(struct hash, htab) + cap * sizeof(struct hash_item));
    if (hash) {
        hash->type = type;
        hash->chain_limit = chain_limit;
        hash->cap = cap;
        memset(hash->htab, 0, cap * sizeof(struct hash_item));
    }
    return hash;
}

void hash_destroy(struct hash **hash, void (*free_data)(union hash_data)) {
    if (*hash) {
        struct hash_type const *type = (*hash)->type;
        if (free_data)
            for (int i = 0; i < (*hash)->cap; i++) {
                struct hash_item *item = (*hash)->htab + i;
                if (!item->in_use)
                    continue;
                free_data(item->data);
            }
        (type->free ? type->free : free)(*hash);
        *hash = NULL;
    }
}

static int hash_grow(struct hash **hash) {
    struct hash *new = hash_create((*hash)->type, (*hash)->cap * 2 - 1,
                            (*hash)->chain_limit);
    if (!new)
        goto E0;
    for (int i = 0; i < (*hash)->cap; i++) {
        struct hash_item *item = (*hash)->htab + i;
        if (!item->in_use)
            continue;
        if (hash_search(&new, item->data, HASH_ENTER, NULL) < 0)
            goto E1;
    }
    hash_destroy(hash, NULL);
    *hash = new;
    return 1;

E1: hash_destroy(&new, NULL);
E0: return 0;
}

int hash_search(struct hash **hash, union hash_data data,
        enum hash_action action, union hash_data *hdata)
{
    struct hash_type const *type = (*hash)->type;
    uint32_t hkey = type->hashfn(data);
    int hindex, pindex, index;
    struct hash_item *item;
    int retcode;

    retcode = 0;
L_restart:
    item = NULL;
    hindex = hkey % (*hash)->cap;
    pindex = -1;
    index = hash_chain_first(*hash, hindex);
    while (index != -1) {
        item = (*hash)->htab + index;
        if (item->hkey == hkey && type->item_eq(item->data, data)) {
            if (hdata)
                *hdata = item->data;
            retcode = 1;
            break;
        }
        pindex = index;
        index = hash_chain_next(*hash, index);
    }
    if (action == HASH_FIND)
        ;
    else if (action == HASH_REMOVE) {
        if (index != -1)
            hash_chain_remove(*hash, hindex, pindex, index);
    } else {
        assert(action == HASH_ENTER);
        if (index == -1) {
            int lindex, free_index = hash_getfreeitem(*hash, hindex, &lindex);
            if (free_index == -1) {
                if (!hash_grow(hash)) {
                    retcode = -1;
                    goto L0;
                }
                assert(!retcode);
                goto L_restart;
            }
            hash_chain_insert(*hash, hindex, lindex, free_index, hkey, data);
        }
    }
L0: return retcode;
}

void hash_dump(struct hash **hash_p, FILE *fp) {
    struct hash *hash = *hash_p;
    fprintf(fp,
            "struct hash ** = %p -> *%p {\n"
            "   .type = (struct hash_type *) %p -> {\n"
            "       .name = (char const *) %s,\n"
            "   },\n"
            "   .chain_limit = (int) %d,\n"
            "   .cap = (int) %d,\n"
            "   .htab = (struct hash_item []) {\n"
            , hash_p, hash
            , hash->type
            , hash->type->name
            , hash->chain_limit
            , hash->cap);
    for (int hindex = 0; hindex < hash->cap; hindex++) {
        struct hash_item *hitem = hash->htab + hindex;
        if (!hitem->hoff && (!hitem->in_use || 
                hitem->hkey % hash->cap != hindex))
            continue;
        int count = 0;
        int len = 0;
        for (int index = hash_chain_first(hash, hindex);
                index != -1; index = hash_chain_next(hash, index)) {
            count++;
            len = index - hindex;
            if (len < 0)
                len += hash->cap;
        }
        fprintf(fp,
            "       [%d] = (lst[%d], len=%d) {",
            hindex, count, len);
        int index = hash_chain_first(hash, hindex);
        if (index != -1) {
            struct hash_item *item = hash->htab + index;
            fprintf(fp, "[%d]={hkey=%" PRIu32 "[%d], "
                , index
                , item->hkey
                , item->hkey % hash->cap);
            if (hash->type->item_dump)
                hash->type->item_dump(item->data, fp);
            fprintf(fp, "}");
            for (;;) {
                index = hash_chain_next(hash, index);
                if (index == -1)
                    break;
                item = hash->htab + index;
                fprintf(fp, ", [%d]={hkey=%" PRIu32 "[%d], "
                    , index
                    , item->hkey
                    , item->hkey % hash->cap);
                if (hash->type->item_dump)
                    hash->type->item_dump(item->data, fp);
                fprintf(fp, "}");
            }
        }
        fprintf(fp, "},\n");
    }
    fprintf(fp,
            "   },\n"
            "};\n");
}

// vi: ts=4:sts=4:sw=4:et:tw=78
