#ifndef DELTA_VEC_H_
#define DELTA_VEC_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define vec_t(T) T*

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
#define vec_make(T, len, capacity) \
    (vec_t(T))(vec_make_impl(sizeof(T), (len), (capacity)))

vec_t(void) vec_make_impl(size_t value_size, size_t len, size_t capacity);

// vec_del deletes the vector.
// The underlying memory is freed.
// If NULL is given, nothing is deleted and no error occurs.
void vec_del(vec_t(void) v);

// vec_len returns the number of elements the vector holds.
// If NULL is given, returns 0.
size_t vec_len(const vec_t(void) v);

// vec_resize resizes the vector to n and returns the resized vector.
//
// WARNING:
//      Resizing a pointer on a NULL vector is undefined behavior. Use vec_make
//      or vec_append instead.
//
// The vector is set as invalid in case of error. The vector
// is reallocated if it hasn't enough capacity. The input vector may be
// invalidated. Do not attempt to use it after calling this function.
void vec_resize(void* v_ptr, size_t n);

// vec_append appends the given value to the vector pointed to by v_ptr.
// If a pointer on a NULL vector is given, a new vector is created and the value
// is appended to it.
// The vector is set as invalid in case of error. The vector
// is reallocated if it has not enough capacity to hold the new value.
#define vec_append(v_ptr, val)                          \
    do {                                                \
        size_t len##__line__ = 0;                       \
        if (*(v_ptr) == NULL) {                         \
            *(v_ptr) = vec_make(__typeof__(val), 1, 1); \
        } else {                                        \
            len##__line__ = vec_len(*(v_ptr));          \
            vec_resize(v_ptr, len##__line__ + 1);       \
        }                                               \
        if (vec_valid(*(v_ptr))) {                      \
            (*(v_ptr))[len##__line__] = (val);          \
        }                                               \
    } while (0)

// vec_pop pops the last value from the vector and decreases its size by one.
// Popping from an empty or a NULL vector does nothing.
void vec_pop(vec_t(void) v);

// vec_clear clears the vector. The internal storage isn't freed.
// Clearing a NULL vector does nothing.
void vec_clear(vec_t(void) v);

// Less function pointer taking a vector, two indices and a user-defined
// context. The function must return whether vec[a] <= vec[b].
typedef int (*less_f)(vec_t(void), size_t, size_t, void*);

// vec_sort sorts the vector using the provided less function.
// The less function takes a pointer on a user-defined context data.
// Sorting a NULL vector does nothing.
void vec_sort(vec_t(void) v, less_f less, void* ctx);

#endif  // DELTA_VEC_H_
