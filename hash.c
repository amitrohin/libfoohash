#include <assert.h>
#include "foo/hash.h"

#ifndef lint
static const char rcsid[] __attribute__((unused)) = "$Id: hash.c,v 1.5 2023/04/07 12:12:38 swp Exp $";
#endif

static inline size_t hash_align(struct hash_type const *type) {
    return type->align < alignof(struct hash) ? alignof(struct hash) : type->align;
}

static inline size_t hash_table_offset(struct hash_type const *type, int cap) {
    return roundup(offsetof(struct hash, ctl) + sizeof(struct hash_item_ctl) * cap, type->align);
}

static inline void *hash_table(struct hash *hash) {
    return (char *)hash + hash_table_offset(hash->type, hash->cap);
}

struct hash *hash_create(struct hash_type const *type, int cap) {
    if (cap < HASH_CAPMIN)
        cap = HASH_CAPMIN;
    size_t size = roundup(hash_table_offset(type, cap) + 
                    cap * type->size, hash_align(type));
    struct hash *hash = ALIGNED_ALLOC(hash_align(type), size);
    if (hash) {
        hash->type = type;
        hash->cap = cap;
        memset(hash->ctl, 0, sizeof(struct hash_item_ctl) * cap);
    }
    return hash;
}

void hash_destroy(struct hash *hash) {
    if (hash) {
        void *p;
        HASH_FOREACH(p, hash)
            hash->type->fini(p);
        free(hash);
    }
}

static inline int hash_grow(struct hash **hash) {
    struct hash *old = *hash, *new = NULL;
    int cap = old->cap * 2 - 1;
    new = hash_create(old->type, cap);
    if (new) {
        void *p;
        HASH_FOREACH(p, old)
            if (!hash_search(&new, p, HASH_ENTER))
                goto E0;
    }
    hash_destroy(old);
    *hash = new;
    return 1;

E0: if (new)
        hash_destroy(new);
    return 0;
}

void *hash_search(struct hash **hash, void *data, enum hash_action action) {
    struct hash_iter iter;
    void *item, *place, *free_place = NULL;
    unsigned key, k;
    enum hash_item_state state;

    key = HASH_KEY((*hash)->type->hashfn(data));
L_restart:
    hash_iter_init(&iter, *hash, key);
    while (1) {
        item = hash_iter_get(&iter, HASH_ITER_KEY, HASH_ITER_FORWARD,
                    &place, &k, &state);
        if (!item)
            break;
        if (state == HASH_ITEM_NOOP) {
            if (!free_place)
                free_place = place;
            break;
        }
        if (state == HASH_ITEM_FREE) {
            if (!free_place)
                free_place = place;
            continue;
        }
        assert(state == HASH_ITEM_USED);
        if (k == key && iter.hash->type->eq(item, data))
            goto L0;
    }
    if (action == HASH_FIND)
        goto L0;
    if (action == HASH_REMOVE) {
        if (!item)
            goto L0;
        int index = ((char *)item - (char *)iter.table) / iter.hash->type->size;
        assert(index >= 0);
        assert(index < iter.hash->cap);
        if (iter.hash->type->fini)
            iter.hash->type->fini(item);
        iter.hash->ctl[index].key = 0;
        iter.hash->ctl[index].state = HASH_ITEM_FREE;
        goto L0;
    }
    assert(action == HASH_ENTER);
    if (free_place) {
        if (iter.hash->type->copy) {
            if (iter.hash->type->copy(free_place, data) != 0)
                goto L0;
        } else
            memcpy(free_place, data, iter.hash->type->size);
        int index = ((char *)free_place - (char *)iter.table) / iter.hash->type->size;
        assert(index >= 0);
        assert(index < iter.hash->cap);
        iter.hash->ctl[index].key = key;
        iter.hash->ctl[index].state = HASH_ITEM_USED;
        item = free_place;
        goto L0;
    }
    if (hash_grow(hash))
        goto L_restart;

L0: return item;
}

void hash_iter_init(struct hash_iter *iter, struct hash *hash, unsigned key) {
    iter->advance = 0;
    iter->index = iter->index_key = HASH_KEY(key) % hash->cap;
    iter->table = hash_table(hash);
    iter->hash = hash;
}

void *hash_iter_get(
    struct hash_iter *          iter,
    enum hash_iter_mode         mode,
    enum hash_iter_direction    direction,
    void **                     place,
    unsigned *                  key,
    enum hash_item_state *      state)
{
    void *p = NULL;

    if (mode == HASH_ITER_KEY) {
        int index = iter->index;
        int shift = index - iter->index_key;
        if (shift < 0) 
            shift += iter->hash->cap;
        if (iter->advance)
    L_advance_iter_key:
            if (direction == HASH_ITER_FORWARD) {
                if (iter->hash->ctl[iter->index].state == HASH_ITEM_NOOP)
                    goto L0;
                index++;
                shift++;
            } else {
                index--;
                shift--;
            }
        else
            iter->advance = 1;
        if (shift == HASH_SCHLIM || shift == -1)
            goto L0; 
        iter->index = index % iter->hash->cap;
        if (iter->hash->ctl[iter->index].state == HASH_ITEM_USED &&
                iter->hash->ctl[iter->index].key % iter->hash->cap != iter->index_key)
            goto L_advance_iter_key;
        p = (char *)iter->table + iter->index * iter->hash->type->size;

    } else if (mode == HASH_ITER_USED) {
        if (iter->advance)
    L_advance:
            if (direction == HASH_ITER_FORWARD) {
                if (iter->index == iter->hash->cap)
                    goto L0;
                iter->index++;
            } else {
                if (!iter->index)
                    goto L0;
                iter->index--;
            }
        else
            iter->advance = 1;
        if (iter->hash->ctl[iter->index].state != HASH_ITEM_USED)
            goto L_advance;
        p = (char *)iter->table + iter->index * iter->hash->type->size;

    } else {
        assert(mode == HASH_ITER_ALL);
        if (iter->advance)
            if (direction == HASH_ITER_FORWARD) {
                if (iter->index == iter->hash->cap)
                    goto L0;
                iter->index++;
            } else {
                if (!iter->index)
                    goto L0;
                iter->index--;
            }
        else
            iter->advance = 1;
        p = (char *)iter->table + iter->index * iter->hash->type->size;
    }

L0: if (p) {
        if (place)
            *place = p;
        if (key)
            *key = iter->hash->ctl[iter->index].key;
        if (state)
            *state = iter->hash->ctl[iter->index].state;
    } else
        if (iter->index > 0 && iter->index < iter->hash->cap) {
            if (place)
                *place = (char *)iter->table + iter->index * iter->hash->type->size;
            if (key)
                *key = iter->hash->ctl[iter->index].key;
            if (state)
                *state = iter->hash->ctl[iter->index].state;
        } else {
            if (place)
                *place = NULL;
            if (key)
                *key = 0;
            if (state)
                *state = HASH_ITEM_UNKNOWN;
        }
    return p;
}

static char const *hash_item_state_string_map[] = {
    [HASH_ITEM_NOOP] = "HASH_ITEM_NOOP",
    [HASH_ITEM_USED] = "HASH_ITEM_USED",
    [HASH_ITEM_FREE] = "HASH_ITEM_FREE",
};
static inline 
char const *get_hash_item_state_string(enum hash_item_state state) {
    assert(state >= HASH_ITEM_NOOP && state <= HASH_ITEM_FREE);
    return hash_item_state_string_map[state];
}
void hash_dump(struct hash *hash, FILE *fp) {
    fprintf(fp,
            "struct hash *hash = %p -> {\n"
            "   struct hash_type *type = %p -> {\n"
            "       size_t size  = %3zu,\n"
            "       size_t align = %3zu,\n"
            "       ...\n"
            "   },\n"
            "   int cap = %d,\n"
            "   struct hash_item_ctl ctl[] = %p -> {\n"
            , hash
            , hash->type
            , hash->type->size
            , hash->type->align
            , hash->cap
            , hash->ctl);
    for (int i = 0; i < hash->cap; i++) {
        if (hash->ctl[i].state == HASH_ITEM_NOOP)
            continue;
        int count = 0, pathlen_exists = 0, pathlen_notexists = 0;
        for (int j = 0; j < HASH_SCHLIM; j++) {
            unsigned k = (i + j) % hash->cap;
            if (hash->ctl[k].state == HASH_ITEM_NOOP)
                break;
            pathlen_notexists++;
            if (hash->ctl[k].state == HASH_ITEM_FREE)
                continue;
            if (hash->ctl[k].key % hash->cap == i) {
                pathlen_exists = pathlen_notexists;
                count++;
            }
        }
        if (!count)
            continue;
        fprintf(fp,
            "       [%4d] = {", i);
        for (int j = 0, first = 1; j < HASH_SCHLIM; j++) {
            unsigned k = (i + j) % hash->cap;
            if (hash->ctl[k].state == HASH_ITEM_NOOP)
                break;
            if (hash->ctl[k].state == HASH_ITEM_FREE)
                continue;
            if (hash->ctl[k].key % hash->cap == i) {
                fprintf(fp, "%s[%4d]={%9u, %s}",
                    first ? "" : ", ",
                    k, hash->ctl[k].key, 
                    get_hash_item_state_string(hash->ctl[k].state)
                );
                first = 0;
            }
        }
        fprintf(fp, "}, # count: %2d, pathlen: good=%2d/bad=%2d\n",
            count, pathlen_exists, pathlen_notexists);
    }
    fprintf(fp,
            "   },\n"
            "};\n");
}


#if 0
#define hash_defn_trivial(T) \
    static inline int XCONCAT(T,_hash)(const T *a) { \
        return (*a * 5ull + 13) % INT_MAX; \
    } \
    static inline int XCONCAT(T,_init)(T *dst, T *src) { \
        *dst = *src; \
        return 0; \
    } \
    static inline void XCONCAT(T,_fini)(T *a __attribute__((unused))) { \
    } \
    static inline int XCONCAT(T,_eq)(const T *a, const T *b) { \
        return *a == *b; \
    } \
    static inline void XCONCAT(T,_swap)(T *dst, T *src) { \
        T tmp = *dst; *dst = *src; *src = tmp; \
    } \
    static inline void XCONCAT(T,_dump)(T *a, FILE *fp) { \
        fprintf(fp, _Generic((*a), \
                         short          : "%hd", \
                unsigned short          : "%hu", \
                         int            : "%d", \
                unsigned int            : "%u", \
                         long int       : "%ld", \
                unsigned long int       : "%lu", \
                         long long int  : "%lld", \
                unsigned long long int  : "%llu"), \
            *a); \
    } \
    hash_defn(T, XCONCAT(T,_hash), XCONCAT(T,_init), XCONCAT(T,_fini), \
        XCONCAT(T,_eq), XCONCAT(T,_swap), XCONCAT(T,_dump))

hash_defn_trivial( int16_t);
hash_defn_trivial(uint16_t);
hash_defn_trivial( int32_t);
hash_defn_trivial(uint32_t);
hash_defn_trivial( int64_t);
hash_defn_trivial(uint64_t);
#endif

// vi: ts=4:sts=4:sw=4:et
