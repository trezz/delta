#include "delta/vec.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "delta/allocator.h"

typedef struct vec_header_t {
    size_t value_size;
    size_t len;
    size_t capacity;
    bool valid;

    const allocator_t *_allocator;
} vec_header_t;

#define get_vec_header(vec) (((vec_header_t *)vec) - 1)
#define get_vec_header_const(vec) (((const vec_header_t *)vec) - 1)

static vec_header_t *vec_alloc(const allocator_t *allocator, size_t capacity,
                               size_t value_size) {
    return allocator_alloc(
        allocator,
        capacity * value_size + sizeof(vec_header_t) + 1 /* swap buffer */);
}

void *vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                          const allocator_t *allocator) {
    vec_header_t *s = vec_alloc(allocator, capacity, value_size);
    if (s == NULL) {
        return NULL;
    }

    s->value_size = value_size;
    s->len = len;
    s->capacity = capacity;
    s->valid = true;
    s->_allocator = allocator;

    char *data = (void *)(s + 1);
    if (s->len > 0) {
        memset(data, 0, s->len * s->value_size);
    }

    return data;
}

bool vec_valid(const void *const vec) {
    if (vec == NULL) {
        return true;
    }
    const vec_header_t *header = get_vec_header_const(vec);
    return header->valid;
}

void vec_del(void *vec) {
    if (vec == NULL) {
        return;
    }
    vec_header_t *header = get_vec_header(vec);
    allocator_dealloc(header->_allocator, header);
}

size_t vec_len(const void *const vec) {
    if (vec == NULL) {
        return 0;
    }
    const vec_header_t *header = get_vec_header_const(vec);
    return header->len;
}

static void vec_grow_capacity(vec_header_t **header_ptr, size_t n) {
    bool capacity_changed = false;

    vec_header_t *header = *header_ptr;
    if (header->capacity == 0) {
        header->capacity = 1;
    }
    while (header->len + n > header->capacity) {
        header->capacity *= 2;
        capacity_changed = true;
    }

    if (capacity_changed) {
        // Reallocate.
        vec_header_t *new_header =
            vec_alloc(header->_allocator, header->capacity, header->value_size);
        if (new_header == NULL) {
            header->valid = false;
            return;
        }
        memcpy(new_header, header,
               header->len * header->value_size + sizeof(vec_header_t));
        allocator_dealloc(new_header->_allocator, header);

        *header_ptr = new_header;
    }
}

void vec_resize_impl(void *vec_ptr_, size_t len, bool zero_init) {
    void **vec_ptr = vec_ptr_;
    vec_header_t *header = get_vec_header(*vec_ptr);
    const size_t prev_len = header->len;
    const size_t prev_capacity = header->capacity;

    if (len <= prev_len || (!zero_init && prev_capacity >= len)) {
        header->len = len;
        return;
    }

    vec_grow_capacity(&header, len);
    if (!header->valid) {
        return;
    }
    header->len = len;

    *vec_ptr = header + 1;
    if (zero_init) {
        char *data = *vec_ptr;
        memset(data + (prev_len * header->value_size), 0,
               (len - prev_len) * header->value_size);
    }
}

void *vec_copy(const void *const vec) {
    if (vec == NULL) {
        return NULL;
    }

    const vec_header_t *header = get_vec_header_const(vec);

    vec_t(void) new_vec = vec_make_alloc_impl(
        header->value_size, 0, header->capacity, header->_allocator);
    if (new_vec == NULL) {
        return NULL;
    }
    vec_header_t *new_header = get_vec_header(new_vec);

    const void *data = header + 1;
    void *new_data = new_header + 1;
    memcpy(new_data, data, header->len * header->value_size);
    new_header->len = header->len;

    return new_data;
}

void vec_pop(void *vec) {
    if (vec == NULL) {
        return;
    }

    vec_header_t *header = get_vec_header(vec);
    if (header->len == 0) {
        return;
    }
    --header->len;
}

void vec_clear(void *vec) {
    if (vec == NULL) {
        return;
    }
    vec_header_t *header = get_vec_header(vec);
    header->len = 0;
}

/* TODO: implement a quicksort and a stable sort. */
void vec_sort_impl(void *ctx, void *vec, less_f less) {
    if (vec == NULL) {
        return;
    }

    const size_t len = vec_len(vec);
    vec_header_t *header = get_vec_header(vec);
    char *data = vec;
    void *swapbuf = data + header->capacity * header->value_size;

    assert(1000 > header->value_size);

    for (size_t i = 0; i < len; ++i) {
        for (size_t j = i + 1; j < len; ++j) {
            if (less(ctx, vec, j, i)) {
                void *i_data = data + i * header->value_size;
                void *j_data = data + j * header->value_size;
                memcpy(swapbuf, i_data, header->value_size);
                memcpy(i_data, j_data, header->value_size);
                memcpy(j_data, swapbuf, header->value_size);
            }
        }
    }
}
