#include "./sort.h"

#include <assert.h>
#include <string.h>

#include "./slice.h"

/* TODO: implement a quicksort and a stable sort. */
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

#define sort_less(f) ((int (*)(void *, int, int))f)

void sort_slice(void *slice, void *less_func) { sort_slice_any(slice, sort_less(less_func)); }

static int chars_less(char *s, int a, int b) { return s[a] < s[b]; }
static int uchars_less(unsigned char *s, int a, int b) { return s[a] < s[b]; }
static int shorts_less(short *s, int a, int b) { return s[a] < s[b]; }
static int ushorts_less(unsigned short *s, int a, int b) { return s[a] < s[b]; }
static int ints_less(int *s, int a, int b) { return s[a] < s[b]; }
static int uints_less(unsigned int *s, int a, int b) { return s[a] < s[b]; }
static int lls_less(long long *s, int a, int b) { return s[a] < s[b]; }
static int ulls_less(unsigned long long *s, int a, int b) { return s[a] < s[b]; }
static int floats_less(float *s, int a, int b) { return s[a] < s[b]; }
static int doubles_less(double *s, int a, int b) { return s[a] < s[b]; }
static int cstrings_less(char **s, int a, int b) { return strcmp(s[a], s[b]) < 0; }

void sort_chars(char *slice) { sort_slice_any(slice, sort_less(chars_less)); }
void sort_uchars(unsigned char *slice) { sort_slice_any(slice, sort_less(uchars_less)); }
void sort_shorts(short *slice) { sort_slice_any(slice, sort_less(shorts_less)); }
void sort_ushorts(unsigned short *slice) { sort_slice_any(slice, sort_less(ushorts_less)); }
void sort_ints(int *slice) { sort_slice_any(slice, sort_less(ints_less)); }
void sort_uints(unsigned int *slice) { sort_slice_any(slice, sort_less(uints_less)); }
void sort_lls(long long *slice) { sort_slice_any(slice, sort_less(lls_less)); }
void sort_ulls(unsigned long long *slice) { sort_slice_any(slice, sort_less(ulls_less)); }
void sort_floats(float *slice) { sort_slice_any(slice, sort_less(floats_less)); }
void sort_doubles(double *slice) { sort_slice_any(slice, sort_less(doubles_less)); }
void sort_cstrings(char **slice) { sort_slice_any(slice, sort_less(cstrings_less)); }
