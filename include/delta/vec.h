#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// vec_valid returns true if the vector is valid.
bool vec_valid(void* v);

// vec_make returns a new vector.
//
// It is heap allocated to the given length.
// The content of the allocated space is zero-initialized up to len.
// NULL is returned in case of error.
//
// A vec is meant to be used as a C array. So a vector of int should be typed
// int* and created with:
//      int* v = vec_make(sizeof(int), 0, 10);
void* vec_make(size_t value_size, size_t len, size_t capacity);

// vec_del deletes the vector.
// The underlying memory is freed.
// If NULL is given, nothing is deleted and no error occurs.
void vec_del(void* v);

// vec_len returns the number of elements the vector holds.
size_t vec_len(const void* v);

// vec_resize resizes the vector to n and returns the resized vector.
// The vector is set as invalid in case of error.
// The vector is reallocated if it hasn't enough capacity.
// The input vector may be invalidated. Do not attempt to use it after calling
// this function.
void* vec_resize(void* v, size_t n);

// vec_appendn appends exactly `n` literal values to the vector and returns the
// new vector. The vector is set as invalid in case of error.
// Literal values may be int, char, etc... and pointers. To store structured
// values, use vec_appendnp.
// The vector is reallocated if it has not enough capacity to hold the new
// value. The input vector may be invalidated. Do not attempt to use it after
// calling this function.
void* vec_appendn(void* v, size_t n, ...);

#define vec_append(vec, v) vec_appendn(vec, 1, v)

// vec_storebackn stores exactly `n` values at the end of the vector and returns
// the new vector. The vector is set as invalid in case of error.
// Values to store must be passed by pointers. Their content are copied into the
// vector.
// The vector is reallocated if it has not enough capacity to hold the new
// value. The input vector may be invalidated. Do not attempt to use it after
// calling this function.
void* vec_storebackn(void* v, size_t n, ...);

#define vec_storeback(vec, ptr) vec_storebackn(vec, 1, ptr)

// vec_pop pops the last value from the vector and decreases its size by one.
// Popping from an empty vector does nothing.
void vec_pop(void* v);

// vec_clear clears the vector. The internal storage isn't freed.
void vec_clear(void* v);

// vec_back returns the address of the last value or NULL if the vec is empty.
void* vec_back(void* v);

// Less function pointer taking a vector and two indices.
// The function must return whether vec[a] <= vec[b].
typedef int (*less_f)(void* /* vec */, size_t /* a */, size_t /* b */);

// Less function pointer taking a vector, two indices and a user-defined
// context. The function must return whether vec[a] <= vec[b].
typedef int (*less_with_ctx_f)(void* /* vec */, size_t /* a */, size_t /* b */,
                               void* /* ctx */);

// vec_sort sorts the vector using the provided less function.
void vec_sort(void* v, less_f less);

// vec_sort_ctx sorts the vector using the provided less function.
// The less function takes a pointer on a context data.
void vec_sort_ctx(void* v, less_with_ctx_f less, void* ctx);

#endif  // DELTA_VEC_H_
