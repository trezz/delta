#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <stdbool.h>
#include <stddef.h>

#include "delta/allocator.h"

// vec_t(T) is a dynamically allocated vector of values of type T.
// The type vec_t(void) is a vector of any type. The type vec_t(const void) is a
// vector of any constant type.
#define vec_t(T) T*

// vec_make returns a new vec_t(T) of the given length and the given internal
// storage capacity.
// The elements in the range [0, len[ are zero-initialized.
//
// If an error occurs, the returned vector is set as invalid. Use vec_valid to
// check its validity status.
#define vec_make(T, len, capacity) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), &default_allocator))

// vec_make_alloc behaves as vec_make, but set the new vector to manage memory
// using the provided allocator instead of the default one.
// The elements in the range [0, len[ are zero-initialized.
//
// If an error occurs, the returned vector is set as invalid. Use vec_valid to
// check the validity status of the returned vector.
#define vec_make_alloc(T, len, capacity, allocator) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), (allocator)))

// vec_valid returns whether the given vector is valid or not.
bool vec_valid(vec_t(const void) vec);

// vec_del deletes the given vector. The underlying memory is deallocated.
// Deleting NULL is a noop.
void vec_del(vec_t(void) vec);

// vec_len returns the number of elements the given vector holds.
size_t vec_len(vec_t(const void) vec);

// vec_resize resizes the vector pointed to by vec_ptr so that it can store at
// least len elements.
// New values are zero initialized.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check the
// validity status of the returned vector.
void vec_resize(void* vec_ptr, size_t len);

// vec_clone returns a copy of the given vector.
//
// If an error occurs, the returned vector is set as invalid. Use vec_valid to
// check the validity status of the returned vector.
vec_t(void) vec_clone(vec_t(const void) vec);

// vec_append appends the given literal value to the vector pointed to by
// vec_ptr, increasing the vector's length by one.
//
// Literal values may be int, char, etc... and pointers. To store structured
// values, use vec_store.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check its
// validity status.
#define vec_append(vec_ptr, value) vec_appendn((vec_ptr), 1, (value))

// vec_appendn appends exactly n literal values to the vector pointed to by
// vec_ptr, increasing the vector's length by n.
//
// Literal values may be int, char, etc... and pointers. To store structured
// values, use vec_store.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check its
// validity status.
void vec_appendn(void* vec_ptr, size_t n, ...);

// vec_store stores the value pointed to by value_ptr at the end of the vector
// pointed to by vec_ptr, increasing the vector's length by one.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check its
// validity status.
#define vec_store(vec_ptr, value_ptr) vec_storen((vec_ptr), 1, (value_ptr))

// vec_storen stores exactly n values at the end of the vector pointed to by
// vec_ptr, increasing the vector's length by n.
//
// Values to store must be passed by poitners. Their content are copied into the
// vector.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check its
// validity status.
void vec_storen(void* vec_ptr, size_t n, ...);

// vec_pop removes the last element of the given vector, decreasing its length
// by one.
void vec_pop(vec_t(void) vec);

// vec_clear removes all elements of the vector, setting its length to 0.
// The internal storage isn't deallocated.
void vec_clear(vec_t(void) vec);

// less_f is a pointer on a function returning whether vec[a] <= vec[b] and
// taking as input arguments (in that order):
//      - ctx:      A pointer on user-defined context (may be NULL)
//      - vec:      A vector
//      - (a, b):   Two indices in the range [0; vec_len(vec)[
typedef int (*less_f)(void*, vec_t(void), size_t, size_t);

// vec_sort sorts the given vector using the given less function.
// An optional pointer on a user-defined sort context can be passed. It may be
// NULL.
void vec_sort(void* ctx, vec_t(void) vec, less_f less);

//
// Private implementations.
//

vec_t(void) vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                                const allocator_t* allocator);

#endif  // DELTA_VEC_H_
