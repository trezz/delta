#ifndef DELTA_HASH_H_
#define DELTA_HASH_H_

#include <stddef.h>

// hash_bytes returns the hash of the value of size len pointed to by ptr.
size_t hash_bytes(const void* ptr, size_t len, size_t seed);

#endif  // DELTA_HASH_H_
