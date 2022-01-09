#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <stdbool.h>
#include <stddef.h>

#include "delta/allocator.h"

/*
 * Returns a new vec.
 * It is heap allocated to the given length.
 * The content of the allocated space is zero-initialized up to len.
 * NULL is returned in case of error.
 *
 * A vec is meant to be used as a C array. So a vec of int should be typed int*
 * and created with: int* int_vec = vec_make(sizeof(int), 0, 10);
 */
#define vec_make(T, len, capacity) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), &default_allocator))

#define vec_make_alloc(T, len, capacity, allocator) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), (allocator)))

void* vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                          const allocator_t* allocator);

bool vec_valid(const void* vec);

/*
 * Deletes the vec.
 * The underlying memory is freed.
 * If NULL is given, nothing is deleted and no error is returned.
 */
void vec_del(void* vec);

/*
 * Returns the length of the vec (the number of elements the vec holds).
 */
size_t vec_len(const void* vec);

/*
 * Appends exactly `n` literal values to the vec and returns the new vec.
 * NULL is returned in case of error.
 *
 * Literal values may be int, char, etc... and pointers. To store structured
 * values, use vec_appendnp.
 *
 * The vec is reallocated if it has not enough capacity to hold the new value.
 *
 * The input vec may be invalidated. Do not attempt to use it after calling this
 * function.
 */
void* vec_appendnv(void* vec, size_t n, ...);

#define vec_appendv(vec, v) vec_appendnv(vec, 1, v)

/*
 * Stores exactly `n` structured values at the end of the vec and returns the
 * new vec. NULL is returned in case of error.
 *
 * Values to store must be passed by pointers. Their content are copied into the
 * vec.
 *
 * The vec is reallocated if it has not enough capacity to hold the new value.
 *
 * The input vec may be invalidated. Do not attempt to use it after calling this
 * function.
 */
void* vec_appendnp(void* vec, size_t n, ...);

#define vec_appendp(vec, ptr) vec_appendnp(vec, 1, ptr)

/*
 * Pops the last vec value and decreases the vec size by one.
 */
void vec_pop(void* vec);

/*
 * Clears the vec. The internal storage isn't freed.
 */
void vec_clear(void* vec);

/*
 * Less function pointer taking a user-defined context, the vector and two
 * indices.
 * The function must return whether vec[a] <= vec[b].
 */
typedef int (*less_f)(void* /* ctx */, void* /* vec */, size_t /* a */,
                      size_t /* b */
);

/*
 * Sorts the vec using the provided less function.
 * ctx is an optional user-defined context passed to the less function. It may
 * be NULL.
 */
void vec_sort(void* ctx, void* vec, less_f less);

#endif  // DELTA_VEC_H_
