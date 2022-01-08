#include <delta/strmap.h>
#include <stdio.h>

#include "delta/vec.h"

static char key_data[2] = {0, 0};
static str_t key = {.data = key_data, .len = 1};

static int chars_desc_count_sorter(void* vec, size_t a, size_t b, void* ctx);

/*
 * This program counts each distinct character given as input, and prints
 * their count sorted in decreasing order.
 */
int main(int argc, char** argv) {
    // Make a map mapping strings to size_t, and let the capacity be computed
    // automatically.
    strmap_t(size_t) char_count_map = strmap_make(size_t, 0);

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        while ((key.data[0] = *arg++) != 0) {
            // Update the current count of the key from the map.
            strmap_get(&char_count_map, key) += 1;
        }
    }

    // Make a vector of char to sort the mapped characters by their counts in
    // decreasing order.
    vec_t(str_t) chars_vec = vec_make(str_t, 0, strmap_len(char_count_map));

    // Iterate on each mapped pairs to fill the chars vector.
    for (strmap_iterator_t it = strmap_iterator(char_count_map);
         strmap_next(&it);) {
        vec_append(&chars_vec, it.key);
    }

    // Sort the chars vector in decreasing order.
    // Use the map as context for sorting to access the characters count.
    vec_sort(chars_vec, chars_desc_count_sorter, char_count_map);

    // Print.
    for (size_t i = 0; i < vec_len(chars_vec); ++i) {
        const str_t c = chars_vec[i];
        size_t count = strmap_get(&char_count_map, c);
        printf("char '%s' counted %zu time(s)\n", c.data, count);
    }

    // Delete the created containers.
    vec_del(chars_vec);
    strmap_del(char_count_map);
}

static int chars_desc_count_sorter(vec_t(void) vec, size_t a, size_t b,
                                   void* ctx) {
    // Convert the input arguments to their respective types.
    vec_t(str_t) chars_vec = vec;
    strmap_t(size_t) char_count_map = ctx;

    // Get the count of char a and b from the map.
    size_t a_count = 0;
    size_t b_count = 0;
    a_count = strmap_get(&char_count_map, chars_vec[a]);
    b_count = strmap_get(&char_count_map, chars_vec[b]);

    // Sort in decreasing order.
    return a_count >= b_count;
}
