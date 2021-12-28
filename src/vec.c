#include "delta/vec.h"

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
#define get_vec_header_const(vec) (((const vec_header *)vec) - 1)

void *vec_make(size_t elem_size, size_t len, size_t capacity) {
    vec_header *s = NULL;
    char *data = NULL;

    if (capacity == 0) {
        ++capacity;
    }
    s = malloc(capacity * elem_size + sizeof(vec_header));
    data = (void *)(s + 1);
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
    const vec_header *header = NULL;
    if (vec == NULL) {
        return 0;
    }
    header = get_vec_header_const(vec);
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
    int8_t i8 = 0;
    int16_t i16 = 0;
    int32_t i32 = 0;
    int64_t i64 = 0;
    size_t narg = 0;
    va_list args;

    va_start(args, n);

    header = vec_grow_to_fit(vec, n);

    data = (void *)(header + 1);
    data += header->len * header->elem_size;

    for (narg = 0; narg < n; ++narg) {
        i64 = va_arg(args, int64_t);
        switch (header->elem_size) {
            case sizeof(int8_t):
                i8 = (int8_t)i64;
                memcpy(data, &i8, sizeof(int8_t));
                break;
            case sizeof(int16_t):
                i16 = (int16_t)i64;
                memcpy(data, &i16, sizeof(int16_t));
                break;
            case sizeof(int32_t):
                i32 = (int32_t)i64;
                memcpy(data, &i32, sizeof(int32_t));
                break;
            case sizeof(int64_t):
                memcpy(data, &i64, sizeof(int64_t));
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
    size_t narg = 0;
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
    /* Signed integers to compute the new sub-vector length. */
    int computed_len = 0;
    size_t sublen = 0;

    const vec_header *header = NULL;
    const char *begin = NULL;
    void *new = NULL;
    vec_header *new_header = NULL;

    if (vec == NULL) {
        return NULL;
    }
    header = get_vec_header_const(vec);
    if (start >= header->len) {
        return NULL;
    }
    if ((end >= 0) && (end <= (int)(start))) {
        return NULL;
    }

    begin = (const void *)(header + 1);
    begin += start * header->elem_size;
    if (end < 0) {
        computed_len = (int)(header->len) - (int)(start) + end + 1;
    } else {
        computed_len = end - (int)(start);
    }
    if (computed_len <= 0) {
        return NULL;
    }
    sublen = (size_t)(computed_len);

    new = vec_make(header->elem_size, 0, sublen);
    new_header = get_vec_header(new);
    memcpy(new, begin, sublen * new_header->elem_size);
    new_header->len = sublen;

    return new;
}

/* TODO: implement a quicksort and a stable sort. */
static void vec_sort_any(void *vec, int (*less)(void *, size_t, size_t)) {
    size_t i = 0;
    size_t j = 0;
    const size_t len = vec_len(vec);
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

void vec_sort(void *vec, int (*less_func)(void *, size_t, size_t)) { vec_sort_any(vec, less_func); }
