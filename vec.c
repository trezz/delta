#include "./vec.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _vec_header {
    size_t len;
    size_t capacity;
    size_t elem_size;
} vec_header;

#define get_vec_header(vec) (((vec_header *)vec) - 1)

void *vec_make(size_t elem_size, size_t len, size_t capacity) {
    vec_header *s = malloc(capacity * elem_size + sizeof(vec_header));
    char *data = (void *)(s + 1);
    s->len = len;
    s->capacity = capacity;
    s->elem_size = elem_size;
    memset(data, 0, len * elem_size);
    return data;
}

void vec_del(void *vec) {
    vec_header *header = NULL;
    if (vec == NULL) {
        return;
    }
    header = get_vec_header(vec);
    free(header);
}

size_t vec_len(const void *vec) {
    vec_header *header = NULL;
    if (vec == NULL) {
        return 0;
    }
    header = get_vec_header(vec);
    return header->len;
}

static vec_header *vec_grow_to_fit(void *vec, size_t n) {
    vec_header *header = NULL;
    int capacity_changed = 0;

    header = get_vec_header(vec);
    if (header->capacity == 0) {
        header->capacity = 1;
    }
    while (header->len + n > header->capacity) {
        header->capacity *= 2;
        capacity_changed = 1;
    }
    if (capacity_changed) {
        header = realloc(header, header->capacity * header->elem_size + sizeof(vec_header));
    }

    return header;
}

void *vec_appendnv(void *vec, size_t n, ...) {
    vec_header *header = NULL;
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

    header = vec_grow_to_fit(vec, n);

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

void *vec_appendnp(void *vec, size_t n, ...) {
    vec_header *header = NULL;
    char *data = NULL;
    int narg = 0;
    void *arg = NULL;
    va_list args;

    va_start(args, n);

    header = vec_grow_to_fit(vec, n);

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

void vec_pop(void *vec) {
    vec_header *header = NULL;
    if (vec == NULL) {
        return;
    }
    header = get_vec_header(vec);
    if (header->len == 0) {
        return;
    }
    --header->len;
}

void vec_clear(void *vec) {
    vec_header *header = NULL;
    if (vec == NULL) {
        return;
    }
    header = get_vec_header(vec);
    header->len = 0;
}

void *vec_back(void *vec) {
    vec_header *header = NULL;
    char *data = vec;
    if (vec == NULL) {
        return NULL;
    }
    header = get_vec_header(vec);
    return data + ((header->len - 1) * header->elem_size);
}

void *vec_sub(const void *vec, size_t start, int end) {
    vec_header *header = NULL;
    const char *begin = NULL;
    void *new = NULL;
    vec_header *new_header = NULL;
    int len = 0;

    if (vec == NULL) {
        return NULL;
    }
    header = get_vec_header(vec);
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

    new = vec_make(header->elem_size, 0, len);
    new_header = get_vec_header(new);
    memcpy(new, begin, len * new_header->elem_size);
    new_header->len = len;

    return new;
}

/* TODO: implement a quicksort and a stable sort. */
static void vec_sort_any(void *vec, int (*less)(void *, int, int)) {
    int i = 0;
    int j = 0;
    const int len = vec_len(vec);
    vec_header *header = NULL;
    char swapbuf[1000]; /* TODO: need dynamic allocation if the size to swap is bigger. */
    void *i_data = NULL;
    void *j_data = NULL;
    char *data = vec;

    if (vec == NULL) {
        return;
    }
    header = get_vec_header(vec);

    assert(1000 > header->elem_size);

    for (i = 0; i < len; ++i) {
        for (j = i + 1; j < len; ++j) {
            if (less(vec, j, i)) {
                i_data = data + i * header->elem_size;
                j_data = data + j * header->elem_size;
                memcpy(swapbuf, i_data, header->elem_size);
                memcpy(i_data, j_data, header->elem_size);
                memcpy(j_data, swapbuf, header->elem_size);
            }
        }
    }
}

#define vec_sort_less(f) ((int (*)(void *, int, int))f)

void vec_sort(void *vec, int (*less_func)(void *, int, int)) {
    vec_sort_any(vec, vec_sort_less(less_func));
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

void vec_sort_chars(char *vec) { vec_sort_any(vec, vec_sort_less(chars_less)); }
void vec_sort_uchars(unsigned char *vec) { vec_sort_any(vec, vec_sort_less(uchars_less)); }
void vec_sort_shorts(short *vec) { vec_sort_any(vec, vec_sort_less(shorts_less)); }
void vec_sort_ushorts(unsigned short *vec) { vec_sort_any(vec, vec_sort_less(ushorts_less)); }
void vec_sort_ints(int *vec) { vec_sort_any(vec, vec_sort_less(ints_less)); }
void vec_sort_uints(unsigned int *vec) { vec_sort_any(vec, vec_sort_less(uints_less)); }
void vec_sort_lls(long long *vec) { vec_sort_any(vec, vec_sort_less(lls_less)); }
void vec_sort_ulls(unsigned long long *vec) { vec_sort_any(vec, vec_sort_less(ulls_less)); }
void vec_sort_floats(float *vec) { vec_sort_any(vec, vec_sort_less(floats_less)); }
void vec_sort_doubles(double *vec) { vec_sort_any(vec, vec_sort_less(doubles_less)); }
void vec_sort_cstrings(char **vec) { vec_sort_any(vec, vec_sort_less(cstrings_less)); }
