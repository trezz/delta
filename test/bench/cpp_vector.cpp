#include <stdio.h>
#include <time.h>

#include <vector>

static void append_cpp_vector(size_t n) {
    std::vector<size_t> v;
    for (size_t i = 0; i < n; ++i) {
        v.push_back(i);
    }
}

int main() {
    size_t n_start = 100'000;

    for (size_t i = 1; i <= 10; ++i) {
        const size_t n = n_start * i;
        clock_t start, end;

        printf("# %zu\n", n);

        start = clock();
        append_cpp_vector(n);
        end = clock();
        printf("  cpp vector: %f\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    }

    return 0;
}
