#include "delta/vec.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct vec_header_t {
    size_t value_size;
    size_t len;
    size_t capacity;
    bool valid;
} vec_header_t;

#define vec_header(v) (((vec_header_t *)v) - 1)
#define vec_header_const(v) (((const vec_header_t *)v) - 1)

bool vec_valid(void *v) { return v != NULL && vec_header(v)->valid; }

void *vec_make(size_t value_size, size_t len, size_t capacity) {
    vec_header_t *h = malloc(capacity * value_size + sizeof(vec_header_t));
    if (h == NULL) {
        return NULL;
    }

    h->value_size = value_size;
    h->len = len;
    h->capacity = capacity;
    h->valid = true;

    void *data = h + 1;
    if (len > 0) {
        memset(data, 0, h->len * h->value_size);
    }

    return data;
}

void vec_del(void *v) {
    if (v) {
        free(vec_header(v));
    }
}

size_t vec_len(const void *v) { return vec_header_const(v)->len; }

// vec_header_grow_to_fit grows the capacity of the vector to at least n.
// If an error occurs, the vector is set as invalid.
static vec_header_t *vec_header_grow_to_fit(vec_header_t *h, size_t n) {
    if (h->len + n <= h->capacity) {
        return h;  // No need to grow the vector.
    }

    if (h->capacity == 0) {
        h->capacity++;
    }
    while (h->len + n > h->capacity) {
        h->capacity *= 2;
    }

    vec_header_t *newh =
        realloc(h, h->capacity * h->value_size + sizeof(vec_header_t));
    if (newh == NULL) {
        h->valid = false;
        return h;
    }
    return newh;
}

void *vec_resize(void *v, size_t n) {
    vec_header_t *h = vec_header(v);
    if (n > h->len) {
        h = vec_header_grow_to_fit(h, n);
        if (!h->valid) {
            return v;
        }
    }
    h->len = n;
    return h + 1;
}

void *vec_appendn(void *v, size_t n, ...) {
    int8_t i8 = 0;
    int16_t i16 = 0;
    int32_t i32 = 0;
    int64_t i64 = 0;
    size_t narg = 0;
    va_list args;
    va_start(args, n);

    vec_header_t *h = vec_header(v);
    h = vec_header_grow_to_fit(h, n);
    if (!h->valid) {
        return v;
    }

    char *data = (void *)(h + 1);
    data += h->len * h->value_size;

    for (narg = 0; narg < n; ++narg) {
        i64 = va_arg(args, int64_t);
        switch (h->value_size) {
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
        data += h->value_size;
    }

    va_end(args);

    h->len += n;
    return h + 1;
}

void *vec_storebackn(void *v, size_t n, ...) {
    size_t narg = 0;
    void *arg = NULL;
    va_list args;
    va_start(args, n);

    vec_header_t *h = vec_header(v);
    h = vec_header_grow_to_fit(h, n);
    if (!h->valid) {
        return v;
    }

    char *data = (void *)(h + 1);
    data += h->len * h->value_size;

    for (narg = 0; narg < n; ++narg) {
        arg = va_arg(args, void *);
        memcpy(data, arg, h->value_size);
        data += h->value_size;
    }

    va_end(args);

    h->len += n;
    return h + 1;
}

void vec_pop(void *v) {
    vec_header_t *h = vec_header(v);
    if (h->len > 0) {
        --h->len;
    }
}

void vec_clear(void *v) {
    vec_header_t *h = vec_header(v);
    h->len = 0;
}

void *vec_back(void *v) {
    vec_header_t *h = vec_header(v);
    if (h->len == 0) {
        return NULL;
    }
    char *data = v;
    return data + ((h->len - 1) * h->value_size);
}

static int less_no_context(void *vec, size_t a, size_t b, void *ctx) {
    less_f less = (less_f)ctx;
    return less(vec, a, b);
}

void vec_sort(void *vec, less_f less) {
    vec_sort_ctx(vec, less_no_context, (void *)less);
}

/* TODO: implement a quicksort and a stable sort. */
void vec_sort_ctx(void *v, less_with_ctx_f less, void *ctx) {
    const vec_header_t *h = vec_header_const(v);

    // Temporary swap buffer that may be dynamically allocated in case the
    // vector data size doesn't fit in the static buffer.
    char tmp[100];
    char *swapbuf = tmp;
    if (h->value_size > sizeof(tmp)) {
        swapbuf = malloc(h->value_size);
        assert(swapbuf != NULL && "dynamic allocation of swap buffer failed");
    }

    char *data = v;
    for (size_t i = 0; i < h->len; ++i) {
        for (size_t j = i + 1; j < h->len; ++j) {
            if (less(v, j, i, ctx)) {
                void *i_data = data + i * h->value_size;
                void *j_data = data + j * h->value_size;
                memcpy(swapbuf, i_data, h->value_size);
                memcpy(i_data, j_data, h->value_size);
                memcpy(j_data, swapbuf, h->value_size);
            }
        }
    }

    if (tmp != swapbuf) {
        free(swapbuf);
    }
}
