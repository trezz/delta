#ifndef __DELTA_SORT_H
#define __DELTA_SORT_H

/*
 * Sorts the slice using the provided less function.
 *
 * The less function must conform to the following interface:
 *      int less(T* slice, int a, int b)
 * Where T is the value type stored in the slice (int for a slice of integers), and a and b are
 * indices to values in the slice.
 */
void sort_slice(void* slice, void* less_func);

/*
 * The following functions sorts a slice of the given data type in increasing order.
 */
void sort_chars(char* slice);
void sort_uchars(unsigned char* slice);
void sort_shorts(short* slice);
void sort_ushorts(unsigned short* slice);
void sort_ints(int* slice);
void sort_uints(unsigned int* slice);
void sort_lls(long long* slice);
void sort_ulls(unsigned long long* slice);
void sort_floats(float* slice);
void sort_doubles(double* slice);
void sort_cstrings(char** slice);

#endif /* __DELTA_SORT_H */
