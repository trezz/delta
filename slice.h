#ifndef __DELTA_SLICE_H
#define __DELTA_SLICE_H

#include <stddef.h>

/*
 * Header of a slice. It is stored before the pointer used by the API functions.
 */
typedef struct _slice_header {
    size_t len;
    size_t capacity;
    size_t elem_size;
} slice_header;

/*
 * Returns a new slice.
 * It is heap allocated to the given length.
 * The content of the allocated space is zero-initialized up to len.
 */
void* slice_make(size_t elem_size, size_t len, size_t capacity);

/*
 * Deletes the slice.
 * The underlying memory is freed.
 * If NULL is given, nothing is deleted and no error is returned.
 */
void slice_del(void* slice);

/*
 * Returns the length of the slice (the number of elements the slice holds).
 * If NULL is passed, 0 is returned.
 */
size_t slice_len(const void* slice);

/*
 * Appends exactly `n` values to the slice and returns the new slice.
 *
 * The slice is reallocated if it has not enough capacity to hold the new value.
 *
 * The input slice may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * Appending to a NULL slice is undefined behavior.
 */
void* slice_appendn(void* slice, size_t n, ...);

#define slice_append(slice, v) slice_appendn(slice, 1, v)

/*
 * Returns a sub slice from the slice with the values in the range [start; end[
 * NULL is returned if:
 *  - The input slice is NULL
 *  - start is greater than the size of the slice.
 *  - end is lower than start
 *
 * If a negative number is passed as end, the range end position is computed starting
 * from the end of the slice. -1 is equivalent to slice_len(slice), -2 is equivalent
 * to slice_len(slice)-1, etc...
 */
void* slice_sub(const void* slice, size_t start, int end);

#endif /* __DELTA_SLICE_H */
