#include <stdio.h>
#include "foo/hash.h"

#ifndef lint
static const char rcsid[] = "$Id: hash.c,v 1.4 2023/03/07 17:43:17 swp Exp $";
#endif

#define hash_defn_trivial(T) \
    static inline int XCONCAT(T,_hash)(const T *a) { \
        return *a * 5 + 13; \
    } \
    static inline int XCONCAT(T,_init)(T *dst, T *src) { \
        *dst = *src; \
        return 0; \
    } \
    static inline void XCONCAT(T,_fini)(T *a) { \
    } \
    static inline int XCONCAT(T,_eq)(const T *a, const T *b) { \
        return *a == *b; \
    } \
    static inline void XCONCAT(T,_swap)(T *dst, T *src) { \
        T tmp = *dst; *dst = *src; *src = tmp; \
    } \
    static inline void XCONCAT(T,_dump)(T *a, FILE *fp) { \
        fprintf(fp, "%d", *a); \
    } \
    hash_defn(T, XCONCAT(T,_hash), XCONCAT(T,_init), XCONCAT(T,_fini), \
        XCONCAT(T,_eq), XCONCAT(T,_swap), XCONCAT(T,_dump))

hash_defn_trivial(int);

// vi: ts=4:sts=4:sw=4:et
