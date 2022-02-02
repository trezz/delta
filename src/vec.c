#include "delta/vec.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "delta/allocator.h"

typedef struct vec_header {
    size_t value_size;
    size_t len;
    size_t capacity;
    bool valid;
    char _[7];

    const allocator_t *_allocator;
} vec_header;

#define get_vec_header(vec) (((vec_header *)vec) - 1)
#define get_vec_header_const(vec) (((const vec_header *)vec) - 1)

static vec_header *vec_alloc(const allocator_t *allocator, size_t capacity,
                             size_t value_size) {
    return allocator_alloc(
        allocator,
        (capacity + 1 /* swap buffer */) * value_size + sizeof(vec_header));
}

void *vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                          const allocator_t *allocator) {
    vec_header *s = vec_alloc(allocator, capacity, value_size);
    if (s == NULL) {
        return NULL;
    }

    s->value_size = value_size;
    s->len = len;
    s->capacity = capacity;
    s->valid = true;
    s->_allocator = allocator;

    return s + 1;
}

bool vec_valid(const void *vec) {
    const vec_header *header = get_vec_header_const(vec);
    return header->valid;
}

void vec_del(void *vec) {
    if (vec == NULL) {
        return;
    }
    vec_header *header = get_vec_header(vec);
    allocator_dealloc(header->_allocator, header);
}

size_t vec_len(const void *vec) {
    const vec_header *header = get_vec_header_const(vec);
    return header->len;
}

// Grows the internal capacity of the vector to at least n elements.
// The original vector is left untouched and set as invalid if an error occurs
static void vec_grow_capacity(vec_header **header_ptr, size_t n) {
    if (n == 0) {
        return;
    }

    bool capacity_changed = false;
    vec_header *header = *header_ptr;
    if (header->capacity == 0) {
        header->capacity = 1;
    }
    while (header->len + n > header->capacity) {
        header->capacity *= 2;
        capacity_changed = true;
    }

    if (capacity_changed) {
        // Reallocate.
        vec_header *new_header =
            vec_alloc(header->_allocator, header->capacity, header->value_size);
        if (new_header == NULL) {
            header->valid = false;
            return;
        }
        memcpy(new_header, header,
               header->len * header->value_size + sizeof(vec_header));
        allocator_dealloc(new_header->_allocator, header);

        *header_ptr = new_header;
    }
}

void vec_resize(void *vec_ptr, size_t len) {
    void **vec_addr = vec_ptr;
    vec_header *header = get_vec_header(*vec_addr);
    if (header->capacity < len) {
        vec_grow_capacity(&header, len - header->capacity);
        if (!header->valid) {
            return;
        }
    }
    header->len = len;
    *vec_addr = header + 1;
}

void vec_swap(void *vec, size_t i, size_t j) {
    vec_header *header = get_vec_header(vec);
    char *data = vec;
    void *swapbuf = data + header->capacity * header->value_size;
    void *i_data = data + i * header->value_size;
    void *j_data = data + j * header->value_size;
    memcpy(swapbuf, i_data, header->value_size);
    memcpy(i_data, j_data, header->value_size);
    memcpy(j_data, swapbuf, header->value_size);
}

// Less function forwarding the call to the less function without context passed
// as context. It is used to reuse the implementation of the sort with context
// without requiring the user to provide a NULL context.
static bool vec_less_from_ctx(void *vec, size_t i, size_t j, void *ctx) {
    const vec_less_f lessFunc = *(vec_less_f *)ctx;
    return lessFunc(vec, i, j);
}

void vec_sort(void *vec, vec_less_f less) {
    vec_sort_ctx(vec, vec_less_from_ctx, &less);
}

/* TODO: implement a quicksort and a stable sort. */
void vec_sort_ctx(void *vec, vec_less_ctx_f less, void *ctx) {
    vec_header *header = get_vec_header(vec);
    for (size_t i = 0; i < header->len - 1; ++i) {
        for (size_t j = i + 1; j < header->len; ++j) {
            if (less(vec, j, i, ctx)) {
                vec_swap(vec, j, i);
            }
        }
    }
}
