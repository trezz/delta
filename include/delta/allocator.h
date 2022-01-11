#ifndef DELTA_ALLOCATOR_H_
#define DELTA_ALLOCATOR_H_

#include <stddef.h>

// allocate_f is a pointer on a function that takes a pointer on an optional
// user-defined context and a size in bytes, and returns a pointer on an
// allocated storage of at least the given size.
typedef void* (*allocate_f)(void* /* user-defined context */,
                            size_t /* size */);

// deallocate_f is a pointer on a function that takes a pointer on an optional
// user-defined context and a pointer, and deallocates the given pointer.
typedef void (*deallocate_f)(void* /* user-defined context */,
                             void* /* pointer */);

// allocator_t describes an allocator.
// It contains a pointer on an optional user-defined context that is passed to
// the allocator functions and pointers on an allocate and a deallocate
// function.
typedef struct allocator_t {
    void* ctx;
    allocate_f allocate;
    deallocate_f deallocate;
} allocator_t;

// default_allocator is the default allocator to be used.
// It uses the C allocation functions malloc and free.
extern const allocator_t default_allocator;

// allocator_alloc uses the given allocator to allocate a storage of at least
// the given size and returns a pointer to it.
void* allocator_alloc(const allocator_t* allocator, size_t size);

// allocator_dealloc uses the given allocator to deallocate the given pointer.
void allocator_dealloc(const allocator_t* allocator, void* ptr);

#endif  // DELTA_ALLOCATOR_H_
