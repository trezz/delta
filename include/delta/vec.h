#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <stdbool.h>
#include <stddef.h>

#include "delta/allocator.h"
#include "delta/pputil.h"

// Returns a new vector. It is heap allocated to the given length.
// NULL is returned in case of error.
//
// A vec is meant to be used as a C array. So a vec of int should be typed int*
// and created with: int* int_vec = vec_make(sizeof(int), 0, 10);
#define vec_make(T, len, capacity) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), &default_allocator))

// Returns a new vector as vec_make, but use the provided allocator to manage
// the internal storage memory allocation and deallocation.
// NULL is returned in case of error.
#define vec_make_alloc(T, len, capacity, allocator) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), (allocator)))

void* vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                          const allocator_t* allocator);

// Returns true if the provided vector is valid, false otherwise.
bool vec_valid(const void* vec);

// Deletes the vector. The underlying memory is freed.
// If NULL is given, nothing is deleted and no error is returned.
void vec_del(void* vec);

// Returns the length of the vector (the number of elements the vec holds).
size_t vec_len(const void* vec);

// Resizes the vector pointed to be vec_ptr to len.
// If an error occurs the vector is set as invalid.
void vec_resize(void* vec_ptr, size_t len);

// Clears the vector. The internal storage isn't freed.
#define vec_clear(vec) vec_resize(vec, 0)

// Appends the value to the vector pointed to by vec_ptr.
// If an error occurs, nothing is appended and the vector is set as invalid.
#define vec_append(vec_ptr, value)                            \
    do {                                                      \
        const size_t DELTA_UNIQUE(len) = vec_len(*(vec_ptr)); \
        vec_resize((vec_ptr), DELTA_UNIQUE(len) + 1);         \
        if (vec_valid(*(vec_ptr))) {                          \
            (*(vec_ptr))[DELTA_UNIQUE(len)] = value;          \
        }                                                     \
    } while (0)

// Swaps the vector value stored at index i with the one stored at index j.
// i and j must be in the range [0; len[
void vec_swap(void* vec, size_t i, size_t j);

// Less function pointer taking the vector and two indices. The function must
// return whether vec[i] <= vec[j].
typedef bool (*vec_less_f)(void* /* vec */, size_t /* i */, size_t /* j */);

// Sorts the vector using the provided less function which can use the provided
// context.
void vec_sort(void* vec, vec_less_f less);

// Less function pointer taking the vector, two indices and a user-defined
// context. The function must return whether vec[i] <= vec[j].
typedef bool (*vec_less_ctx_f)(void* /* vec */, size_t /* i */, size_t /* j */,
                               void* /* ctx */);

// Sorts the vector using the provided less function which can use the provided
// context.
void vec_sort_ctx(void* vec, vec_less_ctx_f less, void* ctx);

#endif  // DELTA_VEC_H_
