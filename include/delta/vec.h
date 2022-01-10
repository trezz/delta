#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <stdbool.h>
#include <stddef.h>

#include "delta/allocator.h"
#include "delta/utilpp.h"

// vec_t(T) is a dynamically allocated vector of values of type T.
// The type vec_t(void) is a vector of any type. The type vec_t(const void) is a
// vector of any constant type.
#define vec_t(T) T*

// vec_make returns a new vec_t(T) of the given length and the given internal
// storage capacity.
// The elements in the range [0, len[ are zero-initialized.
//
// NULL is returned in case of error (do not use vec_valid to check the return
// of vec_make).
#define vec_make(T, len, capacity) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), &default_allocator))

// vec_make_alloc behaves as vec_make, but set the new vector to manage memory
// using the provided allocator instead of the default one.
// The elements in the range [0, len[ are zero-initialized.
//
// NULL is returned in case of error (do not use vec_valid to check the return
// of vec_make_alloc).
#define vec_make_alloc(T, len, capacity, allocator) \
    ((T*)vec_make_alloc_impl(sizeof(T), (len), (capacity), (allocator)))

// vec_valid returns whether the given vector is valid or not.
// A NULL vector is considered valid.
bool vec_valid(vec_t(const void) const vec);

// vec_del deletes the given vector. The underlying memory is deallocated.
// Deleting a NULL vector is a noop.
void vec_del(vec_t(void) vec);

// vec_len returns the number of elements the given vector holds.
// The length of a NULL vector is 0.
size_t vec_len(vec_t(const void) const vec);

// vec_resize resizes the vector pointed to by vec_ptr so that it can store at
// least len elements.
// New values are zero initialized.
//
// Resizing a NULL vector is undefined behavior. Use vec_make instead.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check the
// validity status of the returned vector.
#define vec_resize(v_ptr, len) vec_resize_impl((v_ptr), (len), true)

// vec_copy returns a copy of the given vector.
//
// Copying a NULL vector returns a NULL vector.
//
// If an error occurs, the returned vector is set as invalid. Use vec_valid to
// check the validity status of the returned vector.
vec_t(void) vec_copy(vec_t(const void) const vec);

// vec_append appends the given value to the vector pointed to by vec_ptr,
// increasing the vector's length by one.
//
// Appending to a NULL vector creates a vector of size 1 and appends the given
// value to it.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check its
// validity status.
//
// NOTE: This macro function uses __typeof__. If you compiler doesn't support
// it, appending to a vector can be done by resizing the vector first (using
// vec_resize), then accessing the values using the [] operator.
#define vec_append(vec_ptr, value)                                      \
    do {                                                                \
        __typeof__(vec_ptr) DELTA_UNIQUE_SYMBOL(p) = (vec_ptr);         \
        const size_t DELTA_UNIQUE_SYMBOL(vlen) =                        \
            vec_len(*DELTA_UNIQUE_SYMBOL(p));                           \
        if (*DELTA_UNIQUE_SYMBOL(p) == NULL) {                          \
            *DELTA_UNIQUE_SYMBOL(p) =                                   \
                vec_make(__typeof__(**DELTA_UNIQUE_SYMBOL(p)), 1, 1);   \
        } else {                                                        \
            vec_resize_impl(DELTA_UNIQUE_SYMBOL(p),                     \
                            DELTA_UNIQUE_SYMBOL(vlen) + 1, false);      \
        }                                                               \
        if (!vec_valid(*DELTA_UNIQUE_SYMBOL(p))) {                      \
            break;                                                      \
        }                                                               \
        (*DELTA_UNIQUE_SYMBOL(p))[DELTA_UNIQUE_SYMBOL(vlen)] = (value); \
    } while (0)

// vec_pop removes the last element of the given vector, decreasing its length
// by one.
//
// Popping from a NULL or an empty vector is a noop.
void vec_pop(vec_t(void) vec);

// vec_clear removes all elements of the vector, setting its length to 0.
// The internal storage isn't deallocated.
//
// Clearing a NULL vector is a noop.
void vec_clear(vec_t(void) vec);

// less_f is a pointer on a function returning whether vec[a] <= vec[b] and
// taking as input arguments (in that order):
//      - ctx:      A pointer on user-defined context (may be NULL)
//      - vec:      A vector
//      - (a, b):   Two indices in the range [0; vec_len(vec)[
typedef int (*less_f)(void*, vec_t(void), size_t, size_t);

// vec_sort sorts the given vector using the given less function.
//
// An optional pointer on a user-defined sort context can be passed. It may be
// NULL.
//
// Sorting a NULL vector is a noop.
//
// NOTE: this macro function uses __typeof__. If you compiler doesn't support
// it, use vec_sort_impl instead.
#define vec_sort(ctx, vec, less)                                        \
    do {                                                                \
        int (*DELTA_UNIQUE_SYMBOL(vec_sorter))(                         \
            __typeof__(ctx), __typeof__(vec), size_t, size_t) = (less); \
        vec_sort_impl((void*)(ctx), (vec_t(void))(vec),                 \
                      (less_f)DELTA_UNIQUE_SYMBOL(vec_sorter));         \
    } while (0)

// Private implementations.

vec_t(void) vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                                const allocator_t* allocator);

void vec_resize_impl(void* vec_ptr, size_t len, bool zero_init);

void vec_sort_impl(void* ctx, vec_t(void) vec, less_f less);

// Inlined implementations.

#endif  // DELTA_VEC_H_
