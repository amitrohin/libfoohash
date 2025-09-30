#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <foo/hash.h>

#define dprintf(fmt, ...) \
    fprintf(stdout, "%s(),%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

static uint32_t int_hashfn(const union hash_data data) {
    return data.d * 7 + 1;
}
static int int_eq(const union hash_data a, const union hash_data b) {
    return a.d == b.d;
}
static void int_dump(const union hash_data data, FILE *fp) {
    fprintf(fp, "%d", data.d);
}
struct hash_type hash_type_int = {
    .malloc     = malloc,
    .free       = free,
    .hashfn     = int_hashfn,
    .item_eq    = int_eq,
    .item_dump  = int_dump,
    .name       = "hash_type_int",
};

int main(int argc, char *argv[]) {
    long seed = strtol(argv[1], 0, 0);

    struct hash *hash = hash_create(&hash_type_int, 0, 4);
    assert(hash);
    srand(seed);

    for (int i = 0; i < 1000; i++) {
        union hash_data data;
        data.d = rand() % 10000;
        if (i < 5 || rand() % 2) {
            dprintf("%3d. +%d\n", i, data.d);
            if (hash_search(&hash, data, HASH_ENTER, NULL) < 0) {
                dprintf("FAILURE\n");
                break;
            }
        } else {
            dprintf("%3d. -%d\n", i, data.d);
            hash_search(&hash, data, HASH_REMOVE, NULL);
        }
        hash_dump(&hash, stdout);
        printf("\n");
    }
    hash_destroy(&hash, NULL);
    return 0;
}

// vi: ts=4:sts=4:sw=4:et
