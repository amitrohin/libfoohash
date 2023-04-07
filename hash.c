#include <stdio.h>
#include "foo/hash.h"

#ifndef lint
static const char rcsid[] __attribute__((unused)) = "$Id: hash.c,v 1.5 2023/04/07 12:12:38 swp Exp $";
#endif

extern inline int hash_popcount(hash_t h);

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

// vi: ts=4:sts=4:sw=4:et
