#ifndef __DELTA_vec_H
#define __DELTA_vec_H

#include <stddef.h>

/*
 * Returns a new vec.
 * It is heap allocated to the given length.
 * The content of the allocated space is zero-initialized up to len.
 *
 * A vec is meant to be used as a C array. So a vec of int should be typed int* and created
 * with:
 *      int* int_vec = vec_make(sizeof(int), 0, 10);
 */
void* vec_make(size_t elem_size, size_t len, size_t capacity);

/*
 * Deletes the vec.
 * The underlying memory is freed.
 * If NULL is given, nothing is deleted and no error is returned.
 */
void vec_del(void* vec);

/*
 * Returns the length of the vec (the number of elements the vec holds).
 * If NULL is passed, 0 is returned.
 */
size_t vec_len(const void* vec);

/*
 * Appends exactly `n` literal values to the vec and returns the new vec.
 *
 * Literal values may be int, char, etc... and pointers. To store structured
 * values, use vec_appendn.
 *
 * The vec is reallocated if it has not enough capacity to hold the new value.
 *
 * The input vec may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * Appending to a NULL vec is undefined behavior.
 */
void* vec_appendnv(void* vec, size_t n, ...);

#define vec_appendv(vec, v) vec_appendnv(vec, 1, v)

/*
 * Stores exactly `n` structured values at the end of the vec and returns the new vec.
 *
 * Values to store must be passed by pointers. Their content are copied into the vec.
 *
 * The vec is reallocated if it has not enough capacity to hold the new value.
 *
 * The input vec may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * Storing to a NULL vec is undefined behavior.
 */
void* vec_appendn(void* vec, size_t n, ...);

#define vec_append(vec, ptr) vec_appendn(vec, 1, ptr)

/*
 * Pops the last vec value and decreases the vec size by one.
 */
void vec_pop(void* vec);

/*
 * Clears the vec. The internal storage isn't freed.
 */
void vec_clear(void* vec);

/*
 * Returns the address of the last value or NULL if the vec is empty.
 */
void* vec_back(void* vec);

/*
 * Returns a sub vec from the vec with the values in the range [start; end[
 * NULL is returned if:
 *  - The input vec is NULL
 *  - start is greater than the size of the vec.
 *  - end is lower than start
 *
 * If a negative number is passed as end, the range end position is computed starting
 * from the end of the vec. -1 is equivalent to vec_len(vec), -2 is equivalent
 * to vec_len(vec)-1, etc...
 */
void* vec_sub(const void* vec, size_t start, int end);

/*
 * Sorts the vec using the provided less function.
 */
void vec_sort(void* vec, int (*less_func)(void* /* vec */, int /* a */, int /* b */));

/*
 * The following functions sorts a vec of the given data type in increasing order.
 */
void vec_sort_chars(char* vec);
void vec_sort_uchars(unsigned char* vec);
void vec_sort_shorts(short* vec);
void vec_sort_ushorts(unsigned short* vec);
void vec_sort_ints(int* vec);
void vec_sort_uints(unsigned int* vec);
void vec_sort_lls(long long* vec);
void vec_sort_ulls(unsigned long long* vec);
void vec_sort_floats(float* vec);
void vec_sort_doubles(double* vec);
void vec_sort_cstrings(char** vec);

#endif /* __DELTA_vec_H */
