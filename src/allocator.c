#include "delta/allocator.h"

#include <stdlib.h>

static void* default_allocate(void* ctx, size_t n) {
    (void)ctx;
    return malloc(n);
}

static void default_deallocate(void* ctx, void* ptr) {
    (void)ctx;
    free(ptr);
}

const allocator_t default_allocator = {
    .ctx = NULL,
    .allocate = default_allocate,
    .deallocate = default_deallocate,
};

void* allocator_alloc(const allocator_t* allocator, size_t size) {
    return allocator->allocate(allocator->ctx, size);
}

void allocator_dealloc(const allocator_t* allocator, void* ptr) {
    allocator->deallocate(allocator->ctx, ptr);
}
