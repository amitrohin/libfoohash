#ifndef __FOOHASH_H_INCLUDED__
#define __FOOHASH_H_INCLUDED__

#include <stddef.h>
#include <stdalign.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <search.h>
#include <syslog.h>

#ifndef CONCAT
#define CONCAT(x, y)            x##y
#endif
#ifndef XCONCAT
#define XCONCAT(x, y)           CONCAT(x, y)
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

#ifndef ALIGNED_ALLOC
#define ALIGNED_ALLOC_(a, n, a_, n_, p_) ({ \
            size_t a_ = (a), n_ = (n); \
            void * p_ = aligned_alloc(a_, n_); \
            if (unlikely(!p_)) \
                syslog(LOG_ERR|LOG_NDELAY|LOG_PERROR, \
                    "%s(): aligned_alloc(align: %zu, size: %zu): %s", \
                    __func__, a_, n_, strerror(errno)); \
            p_; \
        })
#define ALIGNED_ALLOC(a, n) ALIGNED_ALLOC_(a, n, AUTONAME, AUTONAME, AUTONAME)
#endif

#define HASH_CAPMIN                     (((8 * sizeof(unsigned int)) << 1) - 1)
#define HASH_SCHLIM                     (8 * sizeof(unsigned int)) /* [!] не может быть меньше 2 */

#define hash_t(T)                       XCONCAT(T, _hash_t)
#define hash_s(T)                       XCONCAT(T, _hash_s)
#define hash_create(T, n)               XCONCAT(T, _hash_create)(n)
#define hash_destroy(T, h)              XCONCAT(T, _hash_destroy)(h)
#define hash_search(T, hp, elm, act)    XCONCAT(T, _hash_search)(hp, elm, act)
#define hash_remove(T, h, elm)          XCONCAT(T, _hash_remove)(h, elm)
#define hash_dump(T, h, fp)             XCONCAT(T, _hash_dump)(h, fp)

/* int  T_hashfn(const T *);
 * int  T_init(T *);
 * void T_fini(T *);
 * int  T_cmp(const T *, const T *);
 * void T_swap(T *, T *);
 * void T_dump(T *, FILE *);
 */
#define hash_decl(T) \
    typedef struct hash_s(T) { \
        int cap; \
        unsigned int bitmap[]; \
    } *hash_t(T); \
    \
    hash_t(T)   XCONCAT(T, _hash_create)(int cap); \
    void        XCONCAT(T, _hash_destroy)(hash_t(T) h); \
    T *         XCONCAT(T, _hash_search)(hash_t(T) *h, T *elm, ACTION action); \
    void        XCONCAT(T, _hash_remove)(hash_t(T) h, T *elm); \
    void        XCONCAT(T, _hash_dump)(hash_t(T) h, FILE *fp)

#define hash_align(T) ({ \
            alignof(T) < alignof(unsigned int) ? alignof(unsigned int) : alignof(T); \
        })

#define hash_taboffset(T, cap) ({ \
            roundup(8 * offsetof(struct hash_s(T), bitmap) + (cap), 8 * hash_align(T)) / 8; \
        })

#define hash_tab_(T, h, h_) ({ \
            hash_t(T) h_ = (h); (T *)(((char *)h_) + hash_taboffset(T, h_->cap)); \
        })
#define hash_tab(T, h)          hash_tab_(T, h, AUTONAME)

#define hash_bitmapcap(T, cap) ({ \
            (hash_taboffset(T, cap) - offsetof(struct hash_s(T), bitmap)) / sizeof(unsigned int); \
        })

#define hash_defn(T, T_hashfn, T_init, T_fini, T_cmp, T_swap, T_dump) \
    static inline int XCONCAT(T, _hash_elm_cmp)(const hash_t(T) h, const T *a, const T *b) { \
        int cap = h->cap; \
        int ka = T_hashfn(a) % cap; \
        int kb = T_hashfn(b) % cap; \
        if (ka != kb) \
            return ka - kb; \
        return T_cmp(a, b); \
    } \
    \
    hash_t(T) XCONCAT(T, _hash_create)(int cap) { \
        if (cap < HASH_CAPMIN) \
            cap = HASH_CAPMIN; \
        int tab_offset = hash_taboffset(T, cap); \
        hash_t(T) h = ALIGNED_ALLOC(hash_align(T), tab_offset + cap * sizeof(T)); \
        if (likely(h)) { \
            h->cap = cap; \
            memset(h->bitmap, 0, tab_offset - offsetof(struct hash_s(T), bitmap)); \
        } \
        return h; \
    } \
    \
    void XCONCAT(T, _hash_destroy)(hash_t(T) h) { \
        if (likely(h)) { \
            int cap = h->cap; \
            T *tab = hash_tab(T, h); \
            int i = cap - 1; \
            unsigned int *mp = h->bitmap + (i / (8 * sizeof(unsigned int))); \
            unsigned int m = *mp; \
            unsigned int bm = 1u << (i % (8 * sizeof(unsigned int))); \
            do { \
                while (bm) { \
                    if (m & bm) \
                        T_fini(tab + i); \
                    bm >>= 1; \
                    i--; \
                } \
                if (unlikely(i < 0)) \
                    break; \
                m = *--mp; \
                bm = 1u << (8 * sizeof(unsigned int) - 1); \
            } while (1); \
        L_endloop: \
            free(h); \
        } \
    } \
    \
    static T *XCONCAT(T, _hash_growup)(hash_t(T) *h, T *elm) { \
        T *rp = NULL; \
        int cap = (*h)->cap; \
        hash_t(T) h2 = hash_create(T, cap + (cap >> 1) + 7); \
        if (likely(h2)) { \
            rp = hash_search(T, &h2, elm, ENTER); \
            if (unlikely(!rp)) \
                goto L_err; \
            T *tab = hash_tab(T, *h); \
            int i = 0, bi = 0; \
            unsigned int bm = 1u, *mp = (*h)->bitmap, m = *mp; \
            do { \
                if (!m) { \
                    i += 8 * sizeof(unsigned int) - bi; \
                    if (unlikely(i >= cap)) \
                        break; \
                } else { \
                    if (m & bm) { \
                        if (unlikely(!hash_search(T, &h2, tab + i, ENTER))) \
                            goto L_err; \
                        m &= ~bm; \
                    } \
                    i++; \
                    if (unlikely(i >= cap)) \
                        break; \
                    bi++, bm <<= 1; \
                    if (likely(bm)) \
                        continue; \
                } \
                bi = 0, bm = 1u, m = *++mp; \
            } while (1); \
            hash_destroy(T, *h); \
            *h = h2; \
        } \
        return rp; \
        \
    L_err: \
        hash_destroy(T, h2); \
        return NULL; \
    } \
    \
    T *XCONCAT(T, _hash_search)(hash_t(T) *h, T *elm, ACTION action) { \
        int cap = (*h)->cap; \
        int k = T_hashfn(elm) % cap; \
        T *tab = hash_tab(T, *h); \
        unsigned int *mp = (*h)->bitmap + (k / (8 * sizeof(unsigned int))), m = *mp; \
        unsigned int bm = 1u << (k % (8 * sizeof(unsigned int))); \
        int i = k, lim = HASH_SCHLIM < cap ? HASH_SCHLIM : cap; \
        do { \
            if (!(m & bm)) { \
                if (action == ENTER) { \
                    if (likely(!T_init(tab + i, elm))) { \
                        *mp = m | bm; \
                        return tab + i; \
                    } \
                } \
                break; \
            } \
            if (!XCONCAT(T, _hash_elm_cmp)(*h, tab + i, elm)) \
                return tab + i; \
            if (unlikely(!--lim)) { \
                if (action == FIND) \
                    break; \
                return XCONCAT(T, _hash_growup)(h, elm); \
            } \
            i++; \
            if (unlikely(i == cap)) { \
                i = 0; \
                bm = 1u; \
                mp = (*h)->bitmap; \
                m = *mp; \
                continue; \
            } \
            bm <<= 1; \
            if (unlikely(!bm)) { \
                m = *++mp; \
                bm = 1u; \
            } \
        } while (1); \
        return NULL; \
    } \
    \
    void XCONCAT(T, _hash_remove)(hash_t(T) h, T *elm) { \
        int cap = h->cap; \
        unsigned int *bitmap = h->bitmap; \
        T *tab = hash_tab(T, h); \
        if (elm < tab || elm >= tab + cap) { \
            /* [!] hash_search() не реаллоцирует память h для FIND */ \
            elm = hash_search(T, &h, elm, FIND); \
            if (!elm) \
                return; \
        } \
        int i = elm - tab; \
        int lim = HASH_SCHLIM < cap ? HASH_SCHLIM : cap; \
        for (int l = 1, il, kl, lb; l < lim; l++) { \
    L_restart: \
            il = (i + l) % cap; \
            if (!(bitmap[il / (8 * sizeof(unsigned int))] & (1u << (il % (8 * sizeof(unsigned int)))))) \
                break; \
            kl = T_hashfn(tab + il) % cap; \
            lb = i - lim + 2; \
            if ( (lb <  0 && (kl <= i || kl >= i - lim + 2 + cap)) || \
                 (lb >= 0 && (kl <= i && kl >= lb               )) ) \
            { \
                T_swap(tab + i, tab + il); \
                i = il; \
                l = 1; \
                goto L_restart; \
            } \
        } \
        bitmap[i / (8 * sizeof(unsigned int))] &= ~(1u << (i % (8 * sizeof(unsigned int)))); \
        T_fini(tab + i); \
    } \
    \
    void XCONCAT(T, _hash_dump)(hash_t(T) h, FILE *fp) { \
        int bitmap_cap = hash_bitmapcap(T, h->cap); \
        T *tab = hash_tab(T, h); \
        fprintf(stdout, "hash@%p => { (%td)cap: %d, (%td)bitmap@%p[%d]: %#x", \
            h, (void *)&h->cap - (void *)h, h->cap, \
            (void *)&h->bitmap - (void *)h, h->bitmap, bitmap_cap, h->bitmap[0]); \
        for (int i = 1; i < bitmap_cap; i++) \
            fprintf(stdout, ", %#x", h->bitmap[i]); \
        fprintf(stdout, " {"); \
        for (int k = 0; k < h->cap; k++) { \
            int i = k / (8*sizeof(unsigned int)); \
            int j = k % (8*sizeof(unsigned int)); \
            /* fprintf(stdout, "\n\t<k:%d,i:%d,j:%d,mask:%u,bitmap[i]:%u>", k, i, j, 1u<<j, h->bitmap[i]); */ \
            if (h->bitmap[i] & (1u << j)) \
                fprintf(stdout, " %d", k); \
        } \
        fprintf(stdout, "},\n"); \
        fprintf(stdout, "    (%td){T}@%p[%d]: {\n", \
            (void *)tab - (void *)h, tab, h->cap); \
        for (int k = 0; k < h->cap; k++) { \
            int i = k / (8*sizeof(unsigned int)); \
            int j = k % (8*sizeof(unsigned int)); \
            if (h->bitmap[i] & (1u << j)) { \
                fprintf(stdout, "\t[%d]@%p=", k, tab + k); \
                T_dump(tab + k, stdout); \
                fprintf(stdout, " (k: %d)\n", T_hashfn(tab + k)%h->cap); \
            } \
        } \
        fprintf(stdout, "    }\n}\n"); \
    } \
    struct __hack

#define HASH_FOREACH_(T, p, h, ctx_) \
    for ( \
            struct { \
                T *tab; \
                int i, bi; \
                unsigned int bm, m, *mp; \
            } ctx_ = { \
                .tab = hash_tab(T, h), \
                .i = 0, .bi = 0, \
                .bm = 1u, .m = (h)->bitmap[0], .mp = (h)->bitmap \
            } \
        ; \
            ({ \
                if (ctx_.i < (h)->cap && !ctx_.m) { \
                    ctx_.i += 8 * sizeof(unsigned int) - ctx_.bi; \
                    ctx_.bi = 0, ctx_.bm = 1u, ctx_.m = *++ctx_.mp; \
                    while (ctx_.i < (h)->cap && !ctx_.m) { \
                        ctx_.i += 8 * sizeof(unsigned int); \
                        ctx_.m = *++ctx_.mp; \
                    } \
                } \
                p = ctx_.tab + ctx_.i; \
                ctx_.i < (h)->cap; \
            }) \
        ; \
            ({ \
                ctx_.m &= ~ctx_.bm; \
                ctx_.i++; \
                if (likely(ctx_.i < (h)->cap)) { \
                    ctx_.bi++, ctx_.bm <<= 1; \
                    if (unlikely(!ctx_.bm)) \
                        ctx_.bi = 0, ctx_.bm = 1u, ctx_.m = *++ctx_.mp; \
                } \
            }) \
    ) \
        if (ctx_.m & ctx_.bm)
#define HASH_FOREACH(T, p, h)   HASH_FOREACH_(T, p, h, AUTONAME)

hash_decl(int);

#endif
// vi: ts=4:sts=4:sw=4:et
