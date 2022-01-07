#include "delta/vec.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
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
static void vec_header_grow_to_fit(vec_header_t **h_ptr, size_t n) {
    vec_header_t *h = *h_ptr;
    if (h->len + n <= h->capacity) {
        return;  // No need to grow the vector.
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
        return;
    }
    *h_ptr = newh;
}

void vec_resize(void *v_ptr, size_t n) {
    void **vec_ptr = v_ptr;
    vec_header_t *h = vec_header(*vec_ptr);
    if (n > h->len) {
        vec_header_grow_to_fit(&h, n);
        if (!h->valid) {
            return;
        }
    }
    h->len = n;
    *vec_ptr = h + 1;
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

/* TODO: implement a quicksort and a stable sort. */
void vec_sort(void *v, less_f less, void *ctx) {
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
