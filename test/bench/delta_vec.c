#include <stdio.h>
#include <time.h>

#include "delta/vec.h"

static void append_delta_vec(size_t n) {
    vec_t(size_t) v = vec_make(size_t, 0, 0);
    for (size_t i = 0; i < n; ++i) {
        vec_append(&v, i);
    }
    vec_del(v);
}

int main() {
    size_t n_start = 100000;

    for (size_t i = 1; i <= 10; ++i) {
        const size_t n = n_start * i;
        clock_t start, end;

        printf("# %zu\n", n);

        start = clock();
        append_delta_vec(n);
        end = clock();
        printf("  delta vec : %f\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    }

    return 0;
}
