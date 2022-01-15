#include <stdio.h>

#include "delta/map.h"
#include "delta/vec.h"

static bool chars_sorter(map_t(size_t) m, vec_t(char) vec, size_t a, size_t b) {
    return *map_at(m, map_key_from(vec[a])) >= *map_at(m, map_key_from(vec[b]));
}

// This program counts each distinct character given as input, and prints their
// count sorted in decreasing order.
int main(int argc, const char** argv) {
    // Make a map mapping chars to size_t, and let the capacity be computed
    // automatically.
    map_t(size_t) chars_count = map_make(size_t, 0);

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        for (char c = 0; (c = *arg++) != 0;) {
            map_get(&chars_count, map_key_from(c)) += 1;
        }
    }

    // Make a vector of char to sort the mapped characters by their counts in
    // decreasing order.
    vec_t(char) chars = vec_make(char, 0, map_len(chars_count));
    // Iterate on each mapped pairs to fill the chars vector.
    for (map_iterator_t it = map_iterator_make(chars_count);
         map_iterator_next(chars_count, &it);) {
        vec_append(&chars, map_key_as(char, it.key));
    }

    // Sort the chars vector in decreasing order.
    // Use the map as context for sorting to access the characters count.
    vec_sort_ctx(chars_count, chars, chars_sorter);
    // Print.
    for (size_t i = 0; i < vec_len(chars); ++i) {
        size_t count = map_get(&chars_count, map_key_from(chars[i]));
        printf("char '%c' counted %zu time(s)\n", chars[i], count);
    }
    // Delete the created containers.
    vec_del(chars);
    map_del(chars_count);
}
