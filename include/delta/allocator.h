#ifndef DELTA_ALLOCATOR_H_
#define DELTA_ALLOCATOR_H_

#include <stddef.h>

typedef struct allocator_t allocator_t;

typedef void* (*allocate_f)(void* /* user-defined context */,
                            size_t /* size */);

typedef void (*deallocate_f)(void* /* user-defined context */,
                             void* /* pointer */);

struct allocator_t {
    void* ctx;
    allocate_f allocate;
    deallocate_f deallocate;
};

extern const allocator_t default_allocator;

void* allocator_alloc(const allocator_t* allocator, size_t size);

void allocator_dealloc(const allocator_t* allocator, void* ptr);

#endif  // DELTA_ALLOCATOR_H_
