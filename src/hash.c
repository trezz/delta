#include "delta/hash.h"

/* Murmur hash implementation taken from the GNU ISO C++ Standard Library. */

static size_t unaligned_load(const char* p) {
    size_t result;
    __builtin_memcpy(&result, p, sizeof(result));
    return result;
}

#if __SIZEOF_SIZE_T__ == 8
/* Loads n bytes, where 1 <= n < 8. */
static size_t load_bytes(const char* p, int n) {
    size_t result = 0;
    --n;
    do {
        result = (result << 8) + (unsigned char)(p[n]);
    } while (--n >= 0);
    return result;
}

static size_t shift_mix(size_t v) { return v ^ (v >> 47); }
#endif

#if __SIZEOF_SIZE_T__ == 4

/* Implementation of Murmur hash for 32-bit size_t. */
size_t hash_bytes(const void* ptr, size_t len, size_t seed) {
    const size_t m = 0x5bd1e995;
    size_t hash = seed ^ len;
    const char* buf = (const char*)(ptr);

    /* Mix 4 bytes at a time into the hash. */
    while (len >= 4) {
        size_t k = unaligned_load(buf);
        k *= m;
        k ^= k >> 24;
        k *= m;
        hash *= m;
        hash ^= k;
        buf += 4;
        len -= 4;
    }

    /* Handle the last few bytes of the input array. */
    switch (len) {
        case 3:
            hash ^= (unsigned char)(buf[2]) << 16;
            [[gnu::fallthrough]];
        case 2:
            hash ^= (unsigned char)(buf[1]) << 8;
            [[gnu::fallthrough]];
        case 1:
            hash ^= (unsigned char)(buf[0]);
            hash *= m;
    };

    /* Do a few final mixes of the hash. */
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;
    return hash;
}

#elif __SIZEOF_SIZE_T__ == 8

/* Implementation of Murmur hash for 64-bit size_t. */
size_t hash_bytes(const void* ptr, size_t len, size_t seed) {
    static const size_t mul =
        (((size_t)0xc6a4a793UL) << 32UL) + (size_t)0x5bd1e995UL;
    const char* const buf = (const char*)(ptr);
    const char* p = NULL;

    /*
     * Remove the bytes not divisible by the sizeof(size_t).  This
     * allows the main loop to process the data as 64-bit integers.
     */
    const size_t len_aligned = len & ~(size_t)0x7;
    const char* const end = buf + len_aligned;
    size_t hash = seed ^ (len * mul);
    for (p = buf; p != end; p += 8) {
        const size_t data = shift_mix(unaligned_load(p) * mul) * mul;
        hash ^= data;
        hash *= mul;
    }
    if ((len & 0x7) != 0) {
        const size_t data = load_bytes(end, len & 0x7);
        hash ^= data;
        hash *= mul;
    }
    hash = shift_mix(hash) * mul;
    hash = shift_mix(hash);
    return hash;
}

#else

/* Dummy hash implementation for unusual sizeof(size_t). */
size_t hash_bytes(const void* ptr, size_t len, size_t seed) {
    size_t hash = seed;
    const char* cptr = (const char*)(ptr);
    for (; len; --len) hash = (hash * 131) + *cptr++;
    return hash;
}

#endif /* __SIZEOF_SIZE_T__ */
