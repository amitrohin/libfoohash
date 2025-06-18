#include <stdlib.h>
#include <assert.h>
#include <foo/hash.h>

#define dprintf(fmt, ...) \
    fprintf(stderr, "%s(),%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

unsigned int_nohashfn(void const *p) {
    return HASH_KEY(*(unsigned const *)p);
}
int int_init(void *p) {
    *(int *)p = 0;
    dprintf("%p -> %d\n", p, *(int *)p);
    return 0;
}
void int_fini(void *p) {
    dprintf("%p -> %d\n", p, *(int *)p);
}
int int_eq(void const *a, void const *b) {
    return *(int const *)a == *(int const *)b;
}

struct hash_type hash_type_int = {
    .size   = sizeof(int),
    .align  = alignof(int),
    .hashfn = int_nohashfn,
    .init   = int_init,
    .fini   = int_fini,
    .eq     = int_eq,
    .swap   = NULL,
    .dump   = NULL,
};

int main() {
    struct hash *hash = hash_create(&hash_type_int, 8);
    assert(hash);
    srand(1);

    for (int i = 0; i < 1000; i++) {
        int k = rand() % ~(-1U<<28);
        dprintf("%3d. key: %9d, index: %d, cap: %d\n", i, k, k % hash->cap, hash->cap);
        void *p = hash_search(&hash, &k, HASH_ENTER);
        if (!p) {
            dprintf("FAILURE\n");
            break;
        }
        hash_dump(hash, stderr);
    }

    hash_destroy(hash);
}

// vi: ts=4:sts=4:sw=4:et
