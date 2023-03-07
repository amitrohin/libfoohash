# libfoohash

Implementation of hash tables on C language macros for an arbitrary type (in the style 
of C++ language templates).

Example of use:

[mytype.h]
```
#ifndef __MYTYPE_H_INCLUDED__
#define __MYTYPE_H_INCLUDED__
#include <foo/hash.h>
typedef struct { long x; } mytype_t;
hash_decl(mytype_t);
#endif
```

[mytype.c]
```
#include "mytype.h"

/* int  T_hashfn(const T *);
 * int  T_init(T *);
 * void T_fini(T *);
 * int  T_eq(const T *, const T *);
 * void T_swap(T *, T *);
 * void T_dump(T *, FILE *);
 */
static inline int mytype_t_hashfn(const mytype_t *g) {
    return g->x * 5 + 13;
}
static inline int mytype_t_init(mytype_t_t *dst, mytype_t *src) {
    *dst = *src;
    return 0;
}
static inline void mytype_t_fini(mytype_t *g) {
}
static inline int mytype_t_eq(const mytype_t *a, const mytype_t *b) {
    return a->x == b->x;
}
static inline void mytype_t_swap(mytype_t *a, mytype_t *b) {
    mytype_t_t x = *a; *a = *b; *b = x;
}
static inline void mytype_t_dump(const mytype_t *g, FILE *fp) {
    fprintf(fp, "%ld", g->x);
}
hash_defn(mytype_t_t, mytype_t_hashfn, mytype_t_init, mytype_t_fini, \
    mytype_t_eq, mytype_t_swap, mytype_t_dump);
```


[main.c]
```
...
#include <mytype.h>
...
int main() {
    mytype_t d, *p;
    hash_t(mytype_t) h = hash_create(mytype_t, 0);
    if (!h)
	err(1, "no memory");

    d.x = 10;
    p = hash_search(mytype_t, &h, &d, ENTER);
    if (!p)
	err(1, "no memory to add (or resize and add) new value");

    p = hash_search(mytype_t, &h, &d, FIND);
    assert(p);

    HASH_FOREACH(mytype_t, h, p)
	printf("%ld\n", p->x);
    fputc('\n', stdout);

    printf("count of items: %d\n", hash_popcount(mytype_t, h));

    hash_remove(mytype_t, h, &d);

    hash_destroy(mytype_t, h);
    h = NULL;

    return 0;
}
```

`$Id: README.md,v 1.3 2023/03/07 18:37:06 swp Exp $`
