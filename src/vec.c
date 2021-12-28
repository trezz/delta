#include "delta/vec.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _vec_header {
    size_t value_size;
    size_t len;
    size_t capacity;
    void *(*malloc_func)(size_t);
    void *(*realloc_func)(void *, size_t);
} vec_header;

#define get_vec_header(vec) (((vec_header *)vec) - 1)
#define get_vec_header_const(vec) (((const vec_header *)vec) - 1)

vec_config_t vec_config(size_t value_size, size_t len, size_t capacity) {
    vec_config_t c;
    c.value_size = value_size;
    c.len = len;
    c.capacity = capacity;
    c.malloc_func = &malloc;
    c.realloc_func = &realloc;
    return c;
}

void *vec_make_from_config(const vec_config_t *config) {
    vec_header *s = NULL;
    char *data = NULL;
    size_t capacity = config->capacity;

    if (capacity == 0) {
        ++capacity;
    }
    s = config->malloc_func(capacity * config->value_size + sizeof(vec_header));
    data = (void *)(s + 1);
    s->value_size = config->value_size;
    s->len = config->len;
    s->capacity = capacity;
    s->malloc_func = config->malloc_func;
    s->realloc_func = config->realloc_func;
    memset(data, 0, s->len * s->value_size);
    return data;
}

void *vec_make(size_t value_size, size_t len, size_t capacity) {
    vec_config_t config = vec_config(value_size, len, capacity);
    return vec_make_from_config(&config);
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
        header = header->realloc_func(header,
                                      header->capacity * header->value_size + sizeof(vec_header));
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
    data += header->len * header->value_size;

    for (narg = 0; narg < n; ++narg) {
        i64 = va_arg(args, int64_t);
        switch (header->value_size) {
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
        data += header->value_size;
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
    data += header->len * header->value_size;

    for (narg = 0; narg < n; ++narg) {
        arg = va_arg(args, void *);
        memcpy(data, arg, header->value_size);
        data += header->value_size;
    }

    va_end(args);

    header->len += n;
    return header + 1;
}

void vec_pop(void *vec) {
    vec_header *header = get_vec_header(vec);
    if (header->len == 0) {
        return;
    }
    --header->len;
}

void vec_clear(void *vec) {
    vec_header *header = get_vec_header(vec);
    header->len = 0;
}

void *vec_back(void *vec) {
    vec_header *header = get_vec_header(vec);
    char *data = vec;
    return data + ((header->len - 1) * header->value_size);
}

void *vec_sub(const void *vec, size_t start, int end) {
    /* Signed integers to compute the new sub-vector length. */
    int computed_len = 0;
    size_t sublen = 0;
    const vec_header *header = get_vec_header_const(vec);
    const char *begin = NULL;
    void *new = NULL;
    vec_header *new_header = NULL;

    begin = (const void *)(header + 1);
    begin += start * header->value_size;
    if (end < 0) {
        computed_len = (int)(header->len) - (int)(start) + end + 1;
    } else {
        computed_len = end - (int)(start);
    }
    if (computed_len <= 0) {
        return NULL;
    }
    sublen = (size_t)(computed_len);

    new = vec_make(header->value_size, 0, sublen);
    new_header = get_vec_header(new);
    memcpy(new, begin, sublen * new_header->value_size);
    new_header->len = sublen;

    return new;
}

/* TODO: implement a quicksort and a stable sort. */
void vec_sort(void *vec, int (*less)(void *, size_t, size_t)) {
    size_t i = 0;
    size_t j = 0;
    const size_t len = vec_len(vec);
    vec_header *header = get_vec_header(vec);
    char *swapbuf = NULL;
    void *i_data = NULL;
    void *j_data = NULL;
    char *data = vec;

    swapbuf = header->malloc_func(header->value_size);

    assert(1000 > header->elem_size);

    for (i = 0; i < len; ++i) {
        for (j = i + 1; j < len; ++j) {
            if (less(vec, j, i)) {
                i_data = data + i * header->value_size;
                j_data = data + j * header->value_size;
                memcpy(swapbuf, i_data, header->value_size);
                memcpy(i_data, j_data, header->value_size);
                memcpy(j_data, swapbuf, header->value_size);
            }
        }
    }

    free(swapbuf);
}
