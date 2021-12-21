#ifndef __DELTA_SORT_H
#define __DELTA_SORT_H

void sort_slice(void* slice, void* less_func);

void sort_chars(char* slice);

void sort_ints(int* slice);

void sort_uints(unsigned int* slice);

void sort_cstrings(char** slice);

#endif /* __DELTA_SORT_H */
