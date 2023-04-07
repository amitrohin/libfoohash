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
            void * p_ = aligned_alloc(a_, n_); \
            if (unlikely(!p_)) \
                syslog(LOG_ERR|LOG_NDELAY|LOG_PERROR, \
                    "%s(): aligned_alloc(align: %zu, size: %zu): %s", \
                    __func__, a_, n_, strerror(errno)); \
            p_; \
        })
#define ALIGNED_ALLOC(a, n) ALIGNED_ALLOC_(a, n, AUTONAME, AUTONAME, AUTONAME)
#endif

_Static_assert(CHAR_BIT == 8, "Only 8 bits byte is supported.");
#define SCHAR_TNAME     int8_t
#define UCHAR_TNAME     uint8_t
#if CHAR_MAX == SCHAR_MAX
#define CHAR_TNAME      int8_t
#else
#define CHAR_TNAME      uint8_t
#endif

#if USHRT_MAX == 255u
#define SHORT_TNAME     int8_t
#define USHORT_TNAME    uint8_t
#elif USHRT_MAX == 65535u
#define SHORT_TNAME     int16_t
#define USHORT_TNAME    uint16_t
#elif USHRT_MAX == 4294967295u
#define SHORT_TNAME     int32_t
#define USHORT_TNAME    uint32_t
#else
#error "The short type must be 16, 32, or 64 bits."
#endif

#if UINT_MAX == 255u
#define INT_TNAME       int8_t
#define UINT_TNAME      uint8_t
#elif UINT_MAX == 65535u
#define INT_TNAME       int16_t
#define UINT_TNAME      uint16_t
#elif UINT_MAX == 4294967295u
#define INT_TNAME       int32_t
#define UINT_TNAME      uint32_t
#elif UINT_MAX == 18446744073709551615u
#define INT_TNAME       int64_t
#define UINT_TNAME      uint64_t
#else
#error "The short type must be 16, 32, or 64 bits."
#endif

#if ULONG_MAX == 255u
#define LONG_TNAME      int8_t
#define ULONG_TNAME     uint8_t
#elif ULONG_MAX == 65535u
#define LONG_TNAME      int16_t
#define ULONG_TNAME     uint16_t
#elif ULONG_MAX == 4294967295u
#define LONG_TNAME      int32_t
#define ULONG_TNAME     uint32_t
#elif ULONG_MAX == 18446744073709551615u
#define LONG_TNAME      int64_t
#define ULONG_TNAME     uint64_t
#else
#error "The short type must be 16, 32, or 64 bits."
#endif

#if ULLONG_MAX == 255u
#define LLONG_TNAME     int8_t
#define ULLONG_TNAME    uint8_t
#elif ULLONG_MAX == 65535u
#define LLONG_TNAME     int16_t
#define ULLONG_TNAME    uint16_t
#elif ULLONG_MAX == 4294967295u
#define LLONG_TNAME     int32_t
#define ULLONG_TNAME    uint32_t
#elif ULLONG_MAX == 18446744073709551615u
#define LLONG_TNAME     int64_t
#define ULLONG_TNAME    uint64_t
#else
#error "The short type must be 16, 32, or 64 bits."
#endif

#define HASH_DISPATCH_(T, Suff) \
    _Generic(((T *)0), \
        short *                 : XCONCAT(SHORT_TNAME, Suff), \
        unsigned short *        : XCONCAT(USHORT_TNAME, Suff), \
            default : _Generic(((T *)0), \
        int *                   : XCONCAT(INT_TNAME, Suff), \
        unsigned *              : XCONCAT(UINT_TNAME, Suff), \
            default : _Generic(((T *)0), \
        long *                  : XCONCAT(LONG_TNAME, Suff), \
        unsigned long *         : XCONCAT(ULONG_TNAME, Suff), \
            default : _Generic(((T *)0), \
        long long *             : XCONCAT(LLONG_TNAME, Suff), \
        unsigned long long *    : XCONCAT(ULONG_TNAME, Suff), \
            default : XCONCAT(T, Suff) \
    ))))
#define HASH_DISPATCH(T, Suff)          HASH_DISPATCH_(T, Suff)

#define hash_t(T)                       hash_t

#define hash_create(T, n)               HASH_DISPATCH(T, _hash_create)(n)
#define hash_destroy(T, h)              HASH_DISPATCH(T, _hash_destroy)(h)
#define hash_search(T, hp, elm, act)    HASH_DISPATCH(T, _hash_search)(hp, elm, act)
#define hash_remove(T, h, elm)          HASH_DISPATCH(T, _hash_remove)(h, elm)
#define hash_dump(T, h, fp)             HASH_DISPATCH(T, _hash_dump)(h, fp)

#define hash_create_(T, n)              XCONCAT(T, _hash_create)(n)
#define hash_destroy_(T, h)             XCONCAT(T, _hash_destroy)(h)
#define hash_search_(T, hp, elm, act)   XCONCAT(T, _hash_search)(hp, elm, act)
#define hash_remove_(T, h, elm)         XCONCAT(T, _hash_remove)(h, elm)
#define hash_dump_(T, h, fp)            XCONCAT(T, _hash_dump)(h, fp)

typedef struct hash_s {
    int cap;
    unsigned bitmap[];
} *hash_t;

inline int hash_popcount(hash_t h) {
    int c = 0;
    for (int i = 0, n = roundup2(h->cap, CHAR_BIT*sizeof(unsigned))/(CHAR_BIT*sizeof(unsigned)); i < n; i++)
        c += __builtin_popcount(h->bitmap[i]);
    return c;
}

/* int  T_hashfn(const T *);
 * int  T_init(T *);
 * void T_fini(T *);
 * int  T_eq(const T *, const T *);
 * void T_swap(T *, T *);
 * void T_dump(T *, FILE *);
 */
#define hash_decl_(T, Static) \
    Static hash_t(T)   XCONCAT(T, _hash_create)(int cap); \
    Static void        XCONCAT(T, _hash_destroy)(hash_t(T) h); \
    Static T *         XCONCAT(T, _hash_search)(hash_t(T) *h, T *elm, ACTION action); \
    Static void        XCONCAT(T, _hash_remove)(hash_t(T) h, T *elm); \
    Static void        XCONCAT(T, _hash_dump)(hash_t(T) h, FILE *fp)
#define hash_decl(T)        hash_decl_(T, )
#define hash_decl_static(T) hash_decl_(T, static)

#define HASH_CAPMIN                     63
#define HASH_SCHLIM                     16
_Static_assert(HASH_SCHLIM > 1 && HASH_SCHLIM < HASH_CAPMIN, "Search limit is wrong.");

#define hash_align(T) \
    ({ alignof(T) < alignof(unsigned) ? alignof(unsigned) : alignof(T); })

#define hash_taboffset(T, cap) \
    ({ roundup(CHAR_BIT * offsetof(struct hash_s, bitmap) + (cap), CHAR_BIT * hash_align(T)) / CHAR_BIT; })

#define hash_tab_(T, h, h_)     ({ hash_t(T) h_ = (h); (T *)(((char *)h_) + hash_taboffset(T, h_->cap)); })
#define hash_tab(T, h)          hash_tab_(T, h, AUTONAME)

#define hash_bitmapcap(T, cap) \
    ({ (hash_taboffset(T, cap) - offsetof(struct hash_s, bitmap)) / sizeof(unsigned); })

#define hash_defn_(T, Static, T_hashfn, T_init, T_fini, T_eq, T_swap, T_dump) \
    Static hash_t(T) XCONCAT(T, _hash_create)(int cap) { \
        if (cap < HASH_CAPMIN) \
            cap = HASH_CAPMIN; \
        int tab_offset = hash_taboffset(T, cap); \
        hash_t(T) h = ALIGNED_ALLOC(hash_align(T), tab_offset + cap * sizeof(T)); \
        if (likely(h)) { \
            h->cap = cap; \
            memset(h->bitmap, 0, tab_offset - offsetof(struct hash_s, bitmap)); \
        } \
        return h; \
    } \
    \
    Static void XCONCAT(T, _hash_destroy)(hash_t(T) h) { \
        if (likely(h)) { \
            int cap = h->cap; \
            T *tab = hash_tab(T, h); \
            int i = cap - 1; \
            unsigned *mp = h->bitmap + (i / (CHAR_BIT * sizeof(unsigned))), m = *mp, \
                bm = 1u << (i % (CHAR_BIT * sizeof(unsigned))); \
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
                bm = 1u << (CHAR_BIT * sizeof(unsigned) - 1); \
            } while (1); \
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
            unsigned bm = 1u, *mp = (*h)->bitmap, m = *mp; \
            do { \
                if (!m) { \
                    i += CHAR_BIT * sizeof(unsigned) - bi; \
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
    Static T *XCONCAT(T, _hash_search)(hash_t(T) *h, T *elm, ACTION action) { \
        int cap = (*h)->cap; \
        int k = T_hashfn(elm) % cap; \
        T *tab = hash_tab(T, *h); \
        unsigned *mp = (*h)->bitmap + (k / (CHAR_BIT * sizeof(unsigned))), m = *mp, \
            bm = 1u << (k % (CHAR_BIT * sizeof(unsigned))); \
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
            if (T_eq(tab + i, elm)) \
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
    Static void XCONCAT(T, _hash_remove)(hash_t(T) h, T *elm) { \
        int cap = h->cap; \
        unsigned *bitmap = h->bitmap; \
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
            if (!(bitmap[il / (CHAR_BIT * sizeof(unsigned))] & (1u << (il % (CHAR_BIT * sizeof(unsigned)))))) \
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
        bitmap[i / (CHAR_BIT * sizeof(unsigned))] &= ~(1u << (i % (CHAR_BIT * sizeof(unsigned)))); \
        T_fini(tab + i); \
    } \
    \
    Static void XCONCAT(T, _hash_dump)(hash_t(T) h, FILE *fp) { \
        int bitmap_cap = hash_bitmapcap(T, h->cap); \
        T *tab = hash_tab(T, h); \
        fprintf(fp, "hash@%p => { (%td)cap: %d, (%td)bitmap@%p[%d]: %#x", \
            h, (void *)&h->cap - (void *)h, h->cap, \
            (void *)&h->bitmap - (void *)h, h->bitmap, bitmap_cap, h->bitmap[0]); \
        for (int i = 1; i < bitmap_cap; i++) \
            fprintf(fp, ", %#x", h->bitmap[i]); \
        fprintf(fp, " {"); \
        for (int k = 0; k < h->cap; k++) { \
            int i = k / (CHAR_BIT * sizeof(unsigned)); \
            int j = k % (CHAR_BIT * sizeof(unsigned)); \
            if (h->bitmap[i] & (1u << j)) \
                fprintf(fp, " %d", k); \
        } \
        fprintf(fp, "},\n"); \
        fprintf(fp, "    (%td){T}@%p[%d]: {\n", \
            (void *)tab - (void *)h, tab, h->cap); \
        for (int k = 0; k < h->cap; k++) { \
            int i = k / (CHAR_BIT * sizeof(unsigned)); \
            int j = k % (CHAR_BIT * sizeof(unsigned)); \
            if (h->bitmap[i] & (1u << j)) { \
                fprintf(fp, "\t[%d]@%p=", k, tab + k); \
                T_dump(tab + k, fp); \
                fprintf(fp, " (k: %d)\n", T_hashfn(tab + k)%h->cap); \
            } \
        } \
        fprintf(fp, "    }\n}\n"); \
    } \
    struct __hack
#define hash_defn(T, T_hashfn, T_init, T_fini, T_eq, T_swap, T_dump) \
    hash_defn_(T, , T_hashfn, T_init, T_fini, T_eq, T_swap, T_dump)
#define hash_defn_static(T, T_hashfn, T_init, T_fini, T_eq, T_swap, T_dump) \
    hash_defn_(T, static, T_hashfn, T_init, T_fini, T_eq, T_swap, T_dump)

#define HASH_FOREACH_(T, p, h, ctx_) \
    for ( \
            struct { \
                T *tab; \
                int i, bi; \
                unsigned bm, m, *mp; \
            } ctx_ = { \
                .tab = hash_tab(T, h), \
                .i = 0, .bi = 0, \
                .bm = 1u, .m = (h)->bitmap[0], .mp = (h)->bitmap \
            } \
        ; \
            ({ \
                if (ctx_.i < (h)->cap && !ctx_.m) { \
                    ctx_.i += CHAR_BIT * sizeof(unsigned) - ctx_.bi; \
                    ctx_.bi = 0, ctx_.bm = 1u, ctx_.m = *++ctx_.mp; \
                    while (ctx_.i < (h)->cap && !ctx_.m) { \
                        ctx_.i += CHAR_BIT * sizeof(unsigned); \
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

hash_decl( int16_t);
hash_decl(uint16_t);
hash_decl( int32_t);
hash_decl(uint32_t);
hash_decl( int64_t);
hash_decl(uint64_t);

#endif
// vi: ts=4:sts=4:sw=4:et
