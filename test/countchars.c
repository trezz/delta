#include <delta/strmap.h>
#include <stdio.h>

#include "delta/vec.h"

static char key[2] = {0, 0};

static int chars_desc_count_sorter(void* ctx, void* vec, size_t a, size_t b) {
    // Convert the input arguments to their respective types.
    char** chars_vec = vec;
    strmap_t char_count_map = ctx;

    // Get the count of char a and b from the map.
    size_t a_count = 0;
    size_t b_count = 0;
    strmap_get(char_count_map, chars_vec[a], &a_count);
    strmap_get(char_count_map, chars_vec[b], &b_count);

    // Sort in decreasing order.
    return a_count >= b_count;
}

/*
 * This program counts each distinct character given as input, and prints
 * their count sorted in decreasing order.
 */
int main(int argc, char** argv) {
    // Make a map mapping strings to size_t, and let the capacity be computed
    // automatically.
    strmap_t char_count_map = strmap_make(sizeof(size_t), 0);

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        while ((*key = *arg++) != 0) {
            // Get the current count of the key from the map.
            size_t n = 0;
            strmap_get(char_count_map, key, &n);

            // Update the count if the key exists, or insert the pair (key, 1).
            char_count_map = strmap_addv(char_count_map, key, n + 1);
        }
    }

    // Make a vector of char to sort the mapped characters by their counts in
    // decreasing order.
    char** chars_vec = vec_make(char*, 0, strmap_len(char_count_map));

    // Iterate on each mapped pairs to fill the chars vector.
    for (strmap_iterator_t it = strmap_iterator(char_count_map);
         strmap_next(&it);) {
        chars_vec = vec_append(chars_vec, it.key);
    }

    // Sort the chars vector in decreasing order.
    // Use the map as context for sorting to access the characters count.
    vec_sort(char_count_map, chars_vec, chars_desc_count_sorter);

    // Print.
    for (size_t i = 0; i < vec_len(chars_vec); ++i) {
        const char* c = chars_vec[i];
        size_t count = 0;
        strmap_get(char_count_map, c, &count);
        printf("char '%s' counted %zu time(s)\n", c, count);
    }

    // Delete the created containers.
    vec_del(chars_vec);
    strmap_del(char_count_map);
}
