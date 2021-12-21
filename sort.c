#include "./sort.h"

#include <assert.h>
#include <string.h>

#include "./slice.h"

static void sort_slice_any(void *slice, int (*less)(void *, int, int)) {
    int i = 0;
    int j = 0;
    const int len = slice_len(slice);
    slice_header *header = NULL;
    char swapbuf[1000]; /* TODO: need dynamic allocation if the size to swap is bigger. */
    void *i_data = NULL;
    void *j_data = NULL;
    char *data = slice;

    if (slice == NULL) {
        return;
    }
    header = get_slice_header(slice);

    assert(1000 > header->elem_size);

    for (i = 0; i < len; ++i) {
        for (j = i + 1; j < len; ++j) {
            if (less(slice, j, i)) {
                i_data = data + i * header->elem_size;
                j_data = data + j * header->elem_size;
                memcpy(swapbuf, i_data, header->elem_size);
                memcpy(i_data, j_data, header->elem_size);
                memcpy(j_data, swapbuf, header->elem_size);
            }
        }
    }
}

static int sort_slice_chars_less(char *s, int a, int b) { return s[a] < s[b]; }
static int sort_slice_ints_less(int *s, int a, int b) { return s[a] < s[b]; }
static int sort_slice_uints_less(unsigned int *s, int a, int b) { return s[a] < s[b]; }
static int sort_slice_cstrings_less(char **s, int a, int b) { return strcmp(s[a], s[b]) < 0; }

#define sort_less(f) ((int (*)(void *, int, int))f)

void sort_slice(void *slice, void *less_func) { sort_slice_any(slice, sort_less(less_func)); }

void sort_chars(char *slice) { sort_slice_any(slice, sort_less(sort_slice_chars_less)); }

void sort_ints(int *slice) { sort_slice_any(slice, sort_less(sort_slice_ints_less)); }

void sort_uints(unsigned int *slice) { sort_slice_any(slice, sort_less(sort_slice_uints_less)); }

void sort_cstrings(char **slice) { sort_slice_any(slice, sort_less(sort_slice_cstrings_less)); }
