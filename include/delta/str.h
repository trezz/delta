#ifndef DELTA_STR_H_
#define DELTA_STR_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct str_t {
    char* data;
    size_t len;
} str_t;

#define str_from_cstr(cstr) \
    (str_t) { .data = cstr, .len = sizeof(cstr) - 1 }

#define str_from(str) \
    (str_t) { .data = str, .len = strlen(str) }

#endif  // DELTA_STR_H_
