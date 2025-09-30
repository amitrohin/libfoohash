#ifndef __FOOHASH_H_INCLUDED__
#define __FOOHASH_H_INCLUDED__

#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifndef CONCAT
#define CONCAT(x, y)            x##y
#endif
#ifndef XCONCAT
#define XCONCAT(x, y)           CONCAT(x, y)
#endif
#ifndef CONCAT3
#define CONCAT3(x, y, z)        x##y##z
#endif
#ifndef XCONCAT
#define XCONCAT3(x, y, z)       CONCAT3(x, y, z)
#endif
#ifndef AUTONAME
#define AUTONAME                XCONCAT(_autoname_, __COUNTER__)
#endif

#if !defined(likely)
#define likely(x)               __builtin_expect(!!(x),1)
#endif
#if !defined(unlikely)
#define unlikely(x)             __builtin_expect(!!(x),0)
#endif

#define HASH_CAPMIN 8     /* минимальное кол-во элементов в хэш-таблице */

union hash_data {
    void *      ptr;
    char *      s;
    uintptr_t   uiptr;
    intptr_t    iptr;
    long        ld;
    unsigned long   lu;
    int         d;
    unsigned    u;
    int64_t     i64;
    uint64_t    u64;
    int32_t     i32[2];
    uint32_t    u32[2];
    int16_t     i16[4];
    uint16_t    u16[4];
    int8_t      i8[8];
    uint8_t     u8[8];
    char        c[8];
};
_Static_assert(sizeof(union hash_data) == 8, "");
struct hash_item {
    uint32_t        hkey;       /* хэш значение поля data */
    uint8_t         xoff;       /* смещение индекса до следующего элемента в цепочке */
    uint8_t         hoff;       /* смещение индекса до первого элемента в цепочке */
    unsigned        in_use  :1; /* занята ячейка или свободна */
    unsigned        __pad   :15;
    union hash_data data;
};
_Static_assert(sizeof(struct hash_item) == 16, "");
struct hash_type {
    void *(*malloc)(size_t);
    void (*free)(void *);
    unsigned (*hashfn)(const union hash_data);
    int (*item_eq)(const union hash_data, const union hash_data);
    void (*item_dump)(union hash_data const, FILE *);
    char const *name;
};
typedef struct hash {
    struct hash_type const *type;
    int chain_limit;        /* ограничение на длину цепочек */
    int cap;
    struct hash_item htab[];
} *hash_t;

struct hash *   hash_create(struct hash_type const *type, int cap, int chain_limit);
void            hash_destroy(hash_t *hash, void (*free_data)(union hash_data));
void            hash_dump(hash_t *hash, FILE *fp);

enum hash_action {
    HASH_FIND, HASH_ENTER, HASH_REMOVE
};
/* Return values:
 *  - HASH_FIND
 *       0  - не найден элемент в хэше.
 *       1  - найден элемент. *hdata - копия данных этого элемента.
 *  - HASH_ENTER
 *      -1  - не найден элемент и произошла ошибка при его добавлении.
 *       0  - не найден элемент в хэше, но успешно вставлен.
 *       1  - найден элемент. *hdata - копия данных этого элемента.
 *  - HASH_REMOVE
 *       0  - не найден элемент в хэше, удалять нечего.
 *       1  - найден элемент и успешно удален из хэша. *hdata - копия данных
 *            удаленного элемента.
 */
int             hash_search(hash_t *hash, union hash_data data,
                    enum hash_action action, union hash_data *hdata);

/* Цепочки.
 *     Хэш hkey попадает в цепочку хэш-таблицы с индексом hindex, если
 * hindex == hkey % размер_хэш_таблицы. Таким образм все ключи распределяются
 * по своим цепочкам. Каждый элемент хэш-таблицы имеет поле hoff, которое
 * определяет смещение до первого элемента в hindex цепочке (по сути указатель
 * на начало списка), поле xoff содержит смещение до следующего элемента в
 * цепочке (получаем такой односвязный список). Поле in_use - это состояние
 * элемента:
 * - in_use == 0
 *      Ячейка не хранит пару hkey/data. В этом состоянии она все равно
 *      определяет с какого элемента начинается цепочка, соответствующая его 
 *      индексу, т.е. поле hoff валидно всегда.
 * - in_use == 1
 *      Ячейка хранит данные, и в дополнение к hoff теперь валидны поля hkey,
 *      data и xoff.
 */

/* Находит первый используемый элемент цепочки hindex.
 * Возвращается его индекс или -1 (недопустимый индекс).
 */
inline int hash_chain_first(struct hash *hash, int hindex) {
    int index = (hindex + hash->htab[hindex].hoff) % hash->cap;
    if (!hash->htab[index].in_use || hash->htab[index].hkey % hash->cap != hindex)
        index = -1;
    return index;
}
/* Находит следующий после index используемый элемент цепочки hindex.
 * Возвращается индекс найденного элемента или -1.
 */
inline int hash_chain_next(struct hash *hash, int index) {
    int next = -1;
    if (hash->htab[index].xoff)
        next = (index + hash->htab[index].xoff) % hash->cap;
    return next;
}

#define HASH_CHAIN_FOREACH_(v, h, hkey, d_) \
    for ( \
            struct { \
                struct hash *hash; \
                int cap; \
                int hindex; \
                int index; \
            } d_ = { .hash = NULL } \
        ; \
            ({ \
                if (!d_.hash) { \
                    d_.hash = *(h); \
                    d_.cap = d_.hash->cap; \
                    d_.hindex = (hkey) % d_.cap; \
                    d_.index = hash_chain_first(d_.hash, d_.hindex); \
                } else \
                    d_.index = hash_chain_next(d_.hash, d_.index); \
                if (d_.index != -1) \
                    v = cd_.hash->htab[d_.index].data; \
                d_.index != -1; \
            }) \
        ; \
    )
#define HASH_CHAIN_FOREACH(v, h, hkey)  HASH_CHAIN_FOREACH_(v, h, hkey, AUTONAME)

#define HASH_FOREACH_(v, h, d_) \
    for ( \
            struct { \
                struct hash *hash; \
                int cap; \
                int index; \
            } d_ = { .hash = NULL } \
        ; \
            ({ \
                if (!d_.hash) { \
                    d_.hash = *(h); \
                    d_.cap = d_.hash->cap; \
                    d_.index = 0; \
                } else \
                    d_.index++; \
                if (d_.index != d_.cap) { \
                    d_.item = cd_.hash->htab + d_.index; \
                    v = d_.item->data; \
                } \
                d_.index != d_.cap; \
            }) \
        ; \
    ) \
        if (!d_.item->in_use) \
            continue; \
        else
#define HASH_FOREACH(p, h)  HASH_FOREACH_(p, h, AUTONAME)

#endif
// vi: ts=4:sts=4:sw=4:et:tw=78:syn=off:nu
