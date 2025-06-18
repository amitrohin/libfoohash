#ifndef __FOOHASH_H_INCLUDED__
#define __FOOHASH_H_INCLUDED__

#include <stddef.h>
#include <stdalign.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <search.h>
#include <syslog.h>
#include <stdio.h>

#if defined(__has_include)
#   if __has_include(<foo/ectl.h>)
#       include <foo/ectl.h>
#   endif
#endif

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

#ifndef roundup
#define roundup_(x, y, y_)      ({ __auto_type y_ = (y); (((x) + (y_ - 1)) / y_) * y_; })
#define roundup(x, y)           roundup_(x, y, AUTONAME)
#endif
#ifndef roundup2
#define roundup2_(x, y, z_)     ({ __auto_type z_ = (y) - 1; ((x) + z_) & ~z_; })
#define roundup2(x, y)          roundup2_(x, y, AUTONAME)
#endif

#ifndef ALIGNED_ALLOC
#define ALIGNED_ALLOC_(a, n, a_, n_, p_) ({ \
            size_t a_ = (a), n_ = (n); \
            void *p_ = aligned_alloc(a_, n_); \
            if (unlikely(!p_)) \
                syslog(LOG_ERR|LOG_NDELAY|LOG_PERROR, \
                    "%s(): aligned_alloc(align: %zu, size: %zu): %s", \
                    __func__, a_, n_, strerror(errno)); \
            p_; \
        })
#define ALIGNED_ALLOC(a, n) ALIGNED_ALLOC_(a, n, AUTONAME, AUTONAME, AUTONAME)
#endif


#define HASH_CAPMIN 253     /* минимальное кол-во элементов в хэш-таблице */
#define HASH_SCHLIM 32      /* предел кол-ва просматриваемых последовательно
                                   ячеек при поиске: [k, k + HASH_SCHLIM) */
enum hash_item_state {
    HASH_ITEM_NOOP = 0,     /* ячейка ещё не использовалась */
    HASH_ITEM_USED = 1,     /* ячейка используется */
    HASH_ITEM_FREE = 2,     /* ячейка освобождена */
    HASH_ITEM_UNKNOWN = 3   /* используется, если нужно вернуть состояние, но
                             * нет ячейки, чтобы это состояние прочитать */
};
#define HASH_ITEM_STATE_NBITS   2
#define HASH_KEY_NBITS          30
struct hash_item_ctl {
    unsigned key                :HASH_KEY_NBITS;
    enum hash_item_state state  :HASH_ITEM_STATE_NBITS;
};
#define HASH_KEY_MASK           ((1U << HASH_KEY_NBITS) - 1U)
#define HASH_KEY(k)             ((k) & HASH_KEY_MASK)

struct hash_type {
    size_t size, align;
    unsigned (*hashfn)(void const *);
    int (*init)(void *);
    void (*fini)(void *);
    int (*copy)(void *, void *);
    int (*eq)(void const *, void const *);
    void (*swap)(void *, void *);
    void (*dump)(void const *, FILE *);
};
struct hash {
    struct hash_type const *type;
    int cap;
    struct hash_item_ctl ctl[];
    /* hash table */
};

struct hash *   hash_create(struct hash_type const *type, int cap);
void            hash_destroy(struct hash *hash);

enum hash_iter_mode {
    HASH_ITER_KEY, HASH_ITER_USED, HASH_ITER_ALL
};
enum hash_iter_direction {
    HASH_ITER_FORWARD, HASH_ITER_BACKWARD
};
struct hash_iter {
    int advance;
    int index_key;
    int index;
    void *table;
    struct hash *hash;
};
void    hash_iter_init(struct hash_iter *, struct hash *, unsigned key);
void *  hash_iter_get(struct hash_iter *, enum hash_iter_mode,
            enum hash_iter_direction, void **place, unsigned *key,
            enum hash_item_state *state);

#define HASH_FOREACH_(p, h, iter_, state_) \
    for (struct hash_iter iter_ = {}; ({ \
        if (!iter_.hash) \
            hash_iter_init(&iter_, (h), 0); \
        p = hash_iter_get(&iter_, HASH_ITER_USED, HASH_ITER_FORWARD, \
                NULL, NULL, NULL); \
    }) ;)
#define HASH_FOREACH(p, hash)   HASH_FOREACH_(p, hash, AUTONAME, AUTONAME)

enum hash_action {
    HASH_FIND, HASH_ENTER, HASH_REMOVE
};
void *hash_search(struct hash **hash, void *data, enum hash_action action);

void hash_dump(struct hash *, FILE *);


#if 0
#define hash_create(T, n)               XCONCAT(T, _hash_create)(n)
#define hash_destroy(T, h)              XCONCAT(T, _hash_destroy)(h)
#define hash_search(T, hp, elm, act)    XCONCAT(T, _hash_search)(hp, elm, act)
#define hash_remove(T, h, elm)          XCONCAT(T, _hash_remove)(h, elm)
#define hash_dump(T, h, fp)             XCONCAT(T, _hash_dump)(h, fp)


#define hash_decl_(T, Static) \
    Static hash_t(T)   XCONCAT(T, _hash_create)(int cap); \
    Static void        XCONCAT(T, _hash_destroy)(hash_t(T) h); \
    Static T *         XCONCAT(T, _hash_search)(hash_t(T) *h, T *elm, ACTION action); \
    Static void        XCONCAT(T, _hash_remove)(hash_t(T) h, T *elm); \
    Static void        XCONCAT(T, _hash_dump)(hash_t(T) h, FILE *fp)
#define hash_decl(T)        hash_decl_(T, )
#define hash_decl_static(T) hash_decl_(T, static)
#endif

#endif
// vi: ts=4:sts=4:sw=4:et
