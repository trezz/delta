#ifndef DELTA_STR_H_
#define DELTA_STR_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct str_t {
    char* data;
    size_t len;
} str_t;

#define str_from_cstr(S) \
    (str_t) { .data = S, .len = sizeof(S) - 1 }

inline str_t str_from(char* s) { return (str_t){.data = s, .len = strlen(s)}; }

inline void str_del(str_t s) { free(s.data); }

// TODO: string builder.

#endif  // DELTA_STR_H_
