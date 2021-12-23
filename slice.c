#include "./slice.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _slice_header {
    size_t len;
    size_t capacity;
    size_t elem_size;
} slice_header;

#define get_slice_header(slice) (((slice_header *)slice) - 1)

void *slice_make(size_t elem_size, size_t len, size_t capacity) {
    slice_header *s = malloc(capacity * elem_size + sizeof(slice_header));
    char *data = (void *)(s + 1);
    s->len = len;
    s->capacity = capacity;
    s->elem_size = elem_size;
    memset(data, 0, len * elem_size);
    return data;
}

void slice_del(void *slice) {
    slice_header *header = NULL;
    if (slice == NULL) {
        return;
    }
    header = get_slice_header(slice);
    free(header);
}

size_t slice_len(const void *slice) {
    slice_header *header = NULL;
    if (slice == NULL) {
        return 0;
    }
    header = get_slice_header(slice);
    return header->len;
}

static slice_header *slice_grow_to_fit(void *slice, size_t n) {
    slice_header *header = NULL;
    int capacity_changed = 0;

    header = get_slice_header(slice);
    if (header->capacity == 0) {
        header->capacity = 1;
    }
    while (header->len + n > header->capacity) {
        header->capacity *= 2;
        capacity_changed = 1;
    }
    if (capacity_changed) {
        header = realloc(header, header->capacity * header->elem_size + sizeof(slice_header));
    }

    return header;
}

void *slice_addn(void *slice, size_t n, ...) {
    slice_header *header = NULL;
    char *data = NULL;
    char c = 0;
    short sh = 0;
    int i = 0;
    long long ll = 0;
    int narg = 0;
    va_list args;

    assert(sizeof(c) == 1 && "unsupported runtime environment");
    assert(sizeof(sh) == 2 && "unsupported runtime environment");
    assert(sizeof(i) == 4 && "unsupported runtime environment");
    assert(sizeof(ll) == 8 && "unsupported runtime environment");

    va_start(args, n);

    header = slice_grow_to_fit(slice, n);

    data = (void *)(header + 1);
    data += header->len * header->elem_size;

    for (narg = 0; narg < n; ++narg) {
        ll = va_arg(args, long long);
        switch (header->elem_size) {
            case 1:
                c = (char)ll;
                memcpy(data, &c, 1);
                break;
            case 2:
                sh = (short)ll;
                memcpy(data, &sh, 2);
                break;
            case 4:
                i = (int)ll;
                memcpy(data, &i, 4);
                break;
            case 8:
                memcpy(data, &ll, 8);
                break;
            default:
                assert(0 && "unsupported value data size");
        }
        data += header->elem_size;
    }

    va_end(args);

    header->len += n;
    return header + 1;
}

void *slice_storen(void *slice, size_t n, ...) {
    slice_header *header = NULL;
    char *data = NULL;
    int narg = 0;
    void *arg = NULL;
    va_list args;

    va_start(args, n);

    header = slice_grow_to_fit(slice, n);

    data = (void *)(header + 1);
    data += header->len * header->elem_size;

    for (narg = 0; narg < n; ++narg) {
        arg = va_arg(args, void *);
        memcpy(data, arg, header->elem_size);
        data += header->elem_size;
    }

    va_end(args);

    header->len += n;
    return header + 1;
}

void *slice_sub(const void *slice, size_t start, int end) {
    slice_header *header = NULL;
    const char *begin = NULL;
    void *new = NULL;
    slice_header *new_header = NULL;
    int len = 0;

    if (slice == NULL) {
        return NULL;
    }
    header = get_slice_header(slice);
    if (start >= header->len) {
        return NULL;
    }
    if ((end >= 0) && (end <= start)) {
        return NULL;
    }

    begin = (void *)(header + 1);
    begin += start * header->elem_size;
    if (end < 0) {
        len = header->len - start + end + 1;
    } else {
        len = end - start;
    }

    if (len <= 0) {
        return NULL;
    }

    new = slice_make(header->elem_size, 0, len);
    new_header = get_slice_header(new);
    memcpy(new, begin, len * new_header->elem_size);
    new_header->len = len;

    return new;
}

/* TODO: implement a quicksort and a stable sort. */
static void slice_sort_any(void *slice, int (*less)(void *, int, int)) {
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

#define slice_sort_less(f) ((int (*)(void *, int, int))f)

void slice_sort(void *slice, int (*less_func)(void *, int, int)) {
    slice_sort_any(slice, slice_sort_less(less_func));
}

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

void slice_sort_chars(char *slice) { slice_sort_any(slice, slice_sort_less(chars_less)); }
void slice_sort_uchars(unsigned char *slice) {
    slice_sort_any(slice, slice_sort_less(uchars_less));
}
void slice_sort_shorts(short *slice) { slice_sort_any(slice, slice_sort_less(shorts_less)); }
void slice_sort_ushorts(unsigned short *slice) {
    slice_sort_any(slice, slice_sort_less(ushorts_less));
}
void slice_sort_ints(int *slice) { slice_sort_any(slice, slice_sort_less(ints_less)); }
void slice_sort_uints(unsigned int *slice) { slice_sort_any(slice, slice_sort_less(uints_less)); }
void slice_sort_lls(long long *slice) { slice_sort_any(slice, slice_sort_less(lls_less)); }
void slice_sort_ulls(unsigned long long *slice) {
    slice_sort_any(slice, slice_sort_less(ulls_less));
}
void slice_sort_floats(float *slice) { slice_sort_any(slice, slice_sort_less(floats_less)); }
void slice_sort_doubles(double *slice) { slice_sort_any(slice, slice_sort_less(doubles_less)); }
void slice_sort_cstrings(char **slice) { slice_sort_any(slice, slice_sort_less(cstrings_less)); }
