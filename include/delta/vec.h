#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "delta/allocator.h"
#include "delta/utilpp.h"

// vec_t(T) is a dynamically allocated vector of values of type T.
//
// The type vec_t(void) is a vector of any type. The type vec_t(const void) is a
// vector of any constant type.
#define vec_t(T) T*

// vec_make_alloc behaves as vec_make, but set the new vector to manage memory
// using the provided allocator instead of the default one.
// The elements in the range [0, len[ are zero-initialized.
//
// NULL is returned in case of error.
#define vec_make_alloc(T, len, capacity, allocator) \
    ((vec_t(T))vec_make_alloc_impl(sizeof(T), (len), (capacity), (allocator)))

// vec_make returns a new vec_t(T) of the given length and the given internal
// storage capacity.
// The elements in the range [0, len[ are zero-initialized.
//
// NULL is returned in case of error.
#define vec_make(T, len, capacity) \
    vec_make_alloc(T, len, capacity, &default_allocator)

// vec_valid returns whether the given vector is valid or not.
// Returns false if NULL is given.
bool vec_valid(const void* vec);

// vec_del deletes the given vector. The underlying memory is deallocated.
// Deleting a NULL vector is a noop.
void vec_del(vec_t(void) vec);

// vec_len returns the number of elements the given vector holds.
// Returns 0 if NULL is given.
size_t vec_len(vec_t(const void) vec);

// vec_resize resizes the vector pointed to by vec_ptr so that it can store at
// least len elements.
// New values are zero initialized.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check the
// validity status of the returned vector.
#define vec_resize(v_ptr, len) vec_resize_impl((v_ptr), (len), true)

// vec_copy returns a copy of the given vector.
//
// NULL is returned in case of error.
#define vec_copy(vec) ((__typeof__(vec))vec_copy_impl(vec))

// vec_append appends the given value to the vector pointed to by vec_ptr,
// increasing the vector's length by one.
//
// If an error occurs, the vector is set as invalid. Use vec_valid to check the
// validity status of the returned vector.
//
// NOTE: This macro function uses __typeof__. If you compiler doesn't support
// it, appending to a vector can be done by resizing the vector first (using
// vec_resize), then accessing the values using the [] operator.
#define vec_append(vec_ptr, value)                                           \
    do {                                                                     \
        __typeof__(*vec_ptr)* const DELTA_UNIQUE_SYM(p) = (vec_ptr);         \
        const size_t DELTA_UNIQUE_SYM(vlen) = vec_len(*DELTA_UNIQUE_SYM(p)); \
        vec_resize_impl(DELTA_UNIQUE_SYM(p), DELTA_UNIQUE_SYM(vlen) + 1,     \
                        false);                                              \
        (*DELTA_UNIQUE_SYM(p))[DELTA_UNIQUE_SYM(vlen)] = (value);            \
    } while (0)

// vec_less_f is a pointer on a function returning whether vec[a] <= vec[b] and
// taking as input arguments (in that order):
//      - vec:      A vector
//      - (a, b):   Two indices in the range [0; vec_len(vec)[
typedef bool (*vec_less_f)(vec_t(void), size_t, size_t);

// vec_sort sorts the given vector using the given less function.
//
// NOTE: this macro function uses __typeof__. If you compiler doesn't support
// it, use vec_sort_impl instead.
#define vec_sort(vec, less)                                           \
    do {                                                              \
        bool (*DELTA_UNIQUE_SYM(vec_sorter))(__typeof__(vec), size_t, \
                                             size_t) = (less);        \
        vec_sort_impl((vec_t(void))(vec),                             \
                      (vec_less_f)DELTA_UNIQUE_SYM(vec_sorter));      \
    } while (0)

// vec_less_ctx_f is a pointer on a function returning whether vec[a] <= vec[b]
// and taking as input arguments (in that order):
//      - ctx:      A pointer on user-defined context (may be NULL)
//      - vec:      A vector
//      - (a, b):   Two indices in the range [0; vec_len(vec)[
typedef bool (*vec_less_ctx_f)(void*, vec_t(void), size_t, size_t);

// vec_sort_ctx sorts the given vector using the given less function and a
// pointer on a user-defined context that is passed as first argument to the
// less function.
//
// NOTE: this macro function uses __typeof__. If you compiler doesn't support
// it, use vec_sort_ctx_impl instead.
#define vec_sort_ctx(ctx, vec, less)                                           \
    do {                                                                       \
        bool (*DELTA_UNIQUE_SYM(vec_sorter))(__typeof__(ctx), __typeof__(vec), \
                                             size_t, size_t) = (less);         \
        vec_sort_ctx_impl((void*)(ctx), (vec_t(void))(vec),                    \
                          (vec_less_ctx_f)DELTA_UNIQUE_SYM(vec_sorter));       \
    } while (0)

// Private implementations.

vec_t(void) vec_make_alloc_impl(size_t value_size, size_t len, size_t capacity,
                                const allocator_t* allocator);

void vec_resize_impl(void* vec_ptr, size_t len, bool zero_init);

vec_t(void) vec_copy_impl(vec_t(const void) vec);

void vec_sort_impl(vec_t(void) vec, vec_less_f less);
void vec_sort_ctx_impl(void* ctx, vec_t(void) vec, vec_less_ctx_f less);

#endif  // DELTA_VEC_H_
