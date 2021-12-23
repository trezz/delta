#ifndef __DELTA_SLICE_H
#define __DELTA_SLICE_H

#include <stddef.h>

/*
 * Returns a new slice.
 * It is heap allocated to the given length.
 * The content of the allocated space is zero-initialized up to len.
 *
 * A slice is meant to be used as a C array. So a slice of int should be typed int* and created
 * with:
 *      int* int_slice = slice_make(sizeof(int), 0, 10);
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
 * Adds exactly `n` literal values to the slice and returns the new slice.
 *
 * Literal values may be int, char, etc... and pointers. To store structured
 * values, use slice_storen.
 *
 * The slice is reallocated if it has not enough capacity to hold the new value.
 *
 * The input slice may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * Appending to a NULL slice is undefined behavior.
 */
void* slice_addn(void* slice, size_t n, ...);

#define slice_add(slice, v) slice_addn(slice, 1, v)

/*
 * Stores exactly `n` structured values to the slice and returns the new slice.
 *
 * Values to store must be passed by pointers. Their content are copied into the slice.
 *
 * The slice is reallocated if it has not enough capacity to hold the new value.
 *
 * The input slice may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * Storing to a NULL slice is undefined behavior.
 */
void* slice_storen(void* slice, size_t n, ...);

#define slice_store(slice, v) slice_storen(slice, 1, v)

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

/*
 * Sorts the slice using the provided less function.
 */
void slice_sort(void* slice, int (*less_func)(void* /* slice */, int /* a */, int /* b */));

/*
 * The following functions sorts a slice of the given data type in increasing order.
 */
void slice_sort_chars(char* slice);
void slice_sort_uchars(unsigned char* slice);
void slice_sort_shorts(short* slice);
void slice_sort_ushorts(unsigned short* slice);
void slice_sort_ints(int* slice);
void slice_sort_uints(unsigned int* slice);
void slice_sort_lls(long long* slice);
void slice_sort_ulls(unsigned long long* slice);
void slice_sort_floats(float* slice);
void slice_sort_doubles(double* slice);
void slice_sort_cstrings(char** slice);

#endif /* __DELTA_SLICE_H */
