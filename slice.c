#include "./slice.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define get_slice_header(slice) (((slice_header*)slice) - 1)

void* slice_make(size_t elem_size, size_t len, size_t capacity) {
    slice_header* s = malloc(capacity * elem_size + sizeof(slice_header));
    char* data = (void*)(s + 1);
    s->len = len;
    s->capacity = capacity;
    s->elem_size = elem_size;
    memset(data, 0, len * elem_size);
    return data;
}

void slice_del(void* slice) {
    slice_header* header = NULL;
    if (slice == NULL) {
        return;
    }
    header = get_slice_header(slice);
    free(header);
}

size_t slice_len(const void* slice) {
    slice_header* header = NULL;
    if (slice == NULL) {
        return 0;
    }
    header = get_slice_header(slice);
    return header->len;
}

void* slice_appendn(void* slice, size_t n, ...) {
    slice_header* header = NULL;
    char* data = NULL;
    int capacity_changed = 0;
    char c = 0;
    short sh = 0;
    int i = 0;
    long long ll = 0;
    int narg = 0;
    va_list args;

    assert(sizeof(c) == 1 && "unsupported runtime environment");
    assert(sizeof(sh) == 2 && "unsupported runtime environment");
    assert(sizeof(i) == 4 && "unsupported runtime environment");
    assert(sizeof(ll) == 8 && "unsupported runtime environment");

    va_start(args, n);

    header = get_slice_header(slice);
    while (header->len + n > header->capacity) {
        header->capacity *= 2;
        capacity_changed = 1;
    }
    if (capacity_changed) {
        header = realloc(header, header->capacity * header->elem_size + sizeof(slice_header));
    }

    data = (void*)(header + 1);
    data += header->len * header->elem_size;

    for (narg = 0; narg < n; ++narg) {
        ll = va_arg(args, long long);
        switch (header->elem_size) {
            case 1:
                c = (char)ll;
                memcpy(data, &c, 1);
                break;
            case 2:
                sh = (short)ll;
                memcpy(data, &sh, 2);
                break;
            case 4:
                i = (int)ll;
                memcpy(data, &i, 4);
                break;
            case 8:
                memcpy(data, &ll, 8);
                break;
            default:
                assert(0 && "unsupported value data size");
        }
        data += header->elem_size;
    }

    header->len += n;
    return header + 1;
}

void* slice_sub(const void* slice, size_t start, int end) {
    slice_header* header = NULL;
    const char* begin = NULL;
    void* new = NULL;
    slice_header* new_header = NULL;
    int len = 0;

    if (slice == NULL) {
        return NULL;
    }
    header = get_slice_header(slice);
    if (start >= header->len) {
        return NULL;
    }
    if ((end >= 0) && (end <= start)) {
        return NULL;
    }

    begin = (void*)(header + 1);
    begin += start * header->elem_size;
    if (end < 0) {
        len = header->len - start + end + 1;
    } else {
        len = end - start;
    }

    if (len <= 0) {
        return NULL;
    }

    new = slice_make(header->elem_size, 0, len);
    new_header = get_slice_header(new);
    memcpy(new, begin, len * new_header->elem_size);
    new_header->len = len;

    return new;
}
