#include "delta/strmap.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAPB_CAPA 8
#define MAP_MAX_LOAD_FACTOR 6.5

/****************************************************************************/
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
static size_t hash_bytes(const void* ptr, size_t len, size_t seed) {
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
static size_t hash_bytes(const void* ptr, size_t len, size_t seed) {
    static const size_t mul = (((size_t)0xc6a4a793UL) << 32UL) + (size_t)0x5bd1e995UL;
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
static size_t hash_bytes(const void* ptr, size_t len, size_t seed) {
    size_t hash = seed;
    const char* cptr = (const char*)(ptr);
    for (; len; --len) hash = (hash * 131) + *cptr++;
    return hash;
}

#endif /* __SIZEOF_SIZE_T__ */
/****************************************************************************/

typedef struct _strmap_bucket {
    size_t hash[MAPB_CAPA];
    size_t key_positions[MAPB_CAPA];
    char* values;
    size_t len;
    struct _strmap_bucket* next;
} _strmap_bucket;

typedef struct _map {
    size_t value_size;
    size_t capacity;
    void* (*malloc_func)(size_t);
    void* (*realloc_func)(void*, size_t);
    int (*strncmp_func)(const char*, const char*, size_t);

    size_t len;
    size_t nb_buckets;
    _strmap_bucket* buckets;

    size_t hash_seed;
    char* keys;
    size_t keys_len;
    size_t keys_capacity;
} _map;

static void* init_new_bucket(const _map* m, _strmap_bucket* b) {
    if ((b->values = m->malloc_func(m->value_size * MAPB_CAPA)) == NULL) {
        return NULL;
    }
    b->len = 0;
    b->next = NULL;
    return b;
}

strmap_config_t strmap_config(size_t value_size, size_t capacity) {
    strmap_config_t c;
    c.value_size = value_size;
    c.capacity = capacity;
    c.malloc_func = &malloc;
    c.realloc_func = &realloc;
    c.strncmp_func = &strncmp;
    return c;
}

strmap_t strmap_make_from_config(const strmap_config_t* config) {
    _map* m = NULL;
    size_t i = 0;

    if ((m = config->malloc_func(sizeof(_map))) == NULL) {
        return NULL;
    }

    m->value_size = config->value_size;
    m->capacity = config->capacity;
    m->malloc_func = config->malloc_func;
    m->realloc_func = config->realloc_func;
    m->strncmp_func = config->strncmp_func;

    if (m->capacity == 0) {
        ++m->capacity;
    }
    while (m->capacity % 8 != 0) {
        ++m->capacity;
    }

    m->len = 0;

    m->hash_seed = 13;
    m->nb_buckets = m->capacity / MAPB_CAPA;
    if ((m->buckets = m->malloc_func(sizeof(_strmap_bucket) * m->nb_buckets)) == NULL) {
        return NULL;
    }
    m->keys_len = 0;
    m->keys_capacity = 1024;
    if ((m->keys = m->malloc_func(m->keys_capacity)) == NULL) {
        return NULL;
    }

    for (i = 0; i < m->nb_buckets; ++i) {
        _strmap_bucket* b = &m->buckets[i];
        if (init_new_bucket(m, b) == NULL) {
            return NULL;
        }
    }

    return m;
}

strmap_t strmap_make(size_t value_size, size_t capacity) {
    strmap_config_t config = strmap_config(value_size, capacity);
    return strmap_make_from_config(&config);
}

void strmap_del(strmap_t map) {
    _map* m = map;
    size_t i = 0;

    for (i = 0; i < m->nb_buckets; ++i) {
        _strmap_bucket* b = &m->buckets[i];
        free(b->values);
        b = b->next;
        while (b != NULL) {
            _strmap_bucket* cur = b;
            free(cur->values);
            b = cur->next;
            free(cur);
        }
    }

    free(m->buckets);
    free(m->keys);
    free(m);
}

size_t strmap_len(const strmap_t map) {
    const _map* m = map;
    return m->len;
}

#define bucket_pos(m, h) ((h) & ((m)->nb_buckets - 1))
#define bucket_val(m, b, i) ((b)->values + ((i) * (m)->value_size))

/*
 * Finds the map bucket holding the given key and returns 1 if the key was found.
 * If the key is not in the map, 0 is returned and the bucket that is expected to store the key is
 * set in found_bucket.
 */
static int find_bucket_pos(const _map* m, const char* key, size_t key_len,
                           unsigned long* out_key_hash, _strmap_bucket** found_bucket,
                           size_t* found_pos) {
    const unsigned long h = hash_bytes(key, key_len, m->hash_seed);
    const size_t bpos = bucket_pos(m, h);
    _strmap_bucket* b = &m->buckets[bpos];
    size_t i = 0;

    *out_key_hash = h;

    while (1) {
        for (i = 0; i < b->len; ++i) {
            if (h == b->hash[i]) {
                const char* bkey = m->keys + b->key_positions[i];
                if (m->strncmp_func(key, bkey, key_len) != 0) {
                    continue;
                }
                break;
            }
        }
        if (i < b->len) {
            break;
        }
        if (b->next == NULL) {
            break;
        }
        b = b->next;
    }

    *found_bucket = b;
    *found_pos = i;

    return i < b->len;
}

void* strmap_at_withlen(const strmap_t map, const char* key, size_t key_len) {
    const _map* m = map;
    _strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return NULL;
    }
    return bucket_val(m, b, pos);
}

int strmap_get_withlen(const strmap_t map, const char* key, size_t key_len, void* v) {
    const _map* m = map;
    const void* data = strmap_at_withlen(map, key, key_len);
    if (data == NULL) {
        return 0;
    }
    if (v != NULL) {
        memcpy(v, data, m->value_size);
    }
    return 1;
}

int strmap_erase(strmap_t map, const char* key) {
    _map* m = map;
    _strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;
    const size_t key_len = strlen(key);

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return 0;
    }
    if (b->len > 1) {
        b->hash[pos] = b->hash[b->len - 1];
        b->key_positions[pos] = b->key_positions[b->len - 1];
        memcpy(bucket_val(m, b, pos), bucket_val(m, b, b->len - 1), m->value_size);
    }
    --b->len;
    --m->len;

    return 1;
}

/*
 * Appends the given key in the keys buffer of the map.
 * The inserted key position is returned on success.
 * SIZE_MAX is returned in case of error.
 */
static size_t append_new_key(_map* m, const char* key, size_t key_len) {
    size_t key_pos = 0;

    ++key_len;
    while (m->keys_len + key_len > m->keys_capacity) {
        m->keys_capacity *= 2;
        if ((m->keys = m->realloc_func(m->keys, m->keys_capacity)) == NULL) {
            return SIZE_MAX;
        }
    }

    key_pos = m->keys_len;
    memcpy(m->keys + key_pos, key, key_len);
    m->keys_len += key_len;

    return key_pos;
}

/*
 * Insert a new key/value pair in the map and return a pointer to the map.
 * NULL is returned in case of error.
 */
static void* strmap_insert(_map* m, const char* key, const void* val_ptr) {
    _strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;
    const size_t key_len = strlen(key);

    if (find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
        return m;
    }
    pos = b->len;

    if (pos == MAPB_CAPA) {
        if ((b->next = malloc(sizeof(_strmap_bucket))) == NULL) {
            return NULL;
        }
        if ((b = init_new_bucket(m, b->next)) == NULL) {
            return NULL;
        }
        pos = 0;
    }
    memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
    b->hash[pos] = h;
    if ((b->key_positions[pos] = append_new_key(m, key, key_len)) == SIZE_MAX) {
        return NULL;
    }

    ++b->len;
    ++m->len;
    return m;
}

/*
 * Increase the map capacity by 2 and rehashs the existing key/value pairs.
 * A pointer to the rehased map is returned.
 * NULL is returned in case of error.
 */
static _map* strmap_rehash(_map* m) {
    _map* n = strmap_make(m->value_size, m->capacity * 2);
    strmap_iterator_t it = strmap_iterator(m);
    if (n == NULL) {
        return NULL;
    }

    while (strmap_next(&it)) {
        if (strmap_insert(n, it.key, it.val_ptr) == NULL) {
            return NULL;
        }
    }

    strmap_del(m);

    return n;
}

strmap_t strmap_addp(strmap_t map, const char* key, const void* val_ptr) {
    _map* m = map;
    double load_factor = (double)(m->len);

    load_factor /= (double)m->nb_buckets;
    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        if ((m = strmap_rehash(m)) == NULL) {
            return NULL;
        }
    }

    return strmap_insert(m, key, val_ptr);
}

strmap_t strmap_addv(strmap_t map, const char* key, ...) {
    _map* m = map;
    int8_t i8 = 0;
    int16_t i16 = 0;
    int32_t i32 = 0;
    int64_t i64 = 0;
    va_list args;

    va_start(args, key);
    i64 = va_arg(args, int64_t);
    va_end(args);

    switch (m->value_size) {
        case sizeof(int8_t):
            i8 = (int8_t)i64;
            return strmap_addp(m, key, &i8);
        case sizeof(int16_t):
            i16 = (int16_t)i64;
            return strmap_addp(m, key, &i16);
        case sizeof(int32_t):
            i32 = (int32_t)i64;
            return strmap_addp(m, key, &i32);
        case sizeof(int64_t):
            return strmap_addp(m, key, &i64);
        default:
            assert(0 && "unsupported value data size");
    }
    return NULL;
}

strmap_iterator_t strmap_iterator(const strmap_t map) {
    const _map* m = map;
    strmap_iterator_t it;

    it.key = NULL;
    it.val_ptr = NULL;
    it._map = map;
    it._bpos = 0;
    it._kpos = 0;
    it._b = NULL;

    if (m->len == 0) {
        return it;
    }
    it._b = &m->buckets[0];
    return it;
}

int strmap_next(strmap_iterator_t* it) {
    const _map* m = it->_map;

    while (it->_bpos < m->nb_buckets) {
        _strmap_bucket* b = it->_b;
        for (; b != NULL; it->_b = b = b->next, it->_kpos = 0) {
            if (it->_kpos < b->len) {
                it->key = m->keys + b->key_positions[it->_kpos];
                it->val_ptr = bucket_val(m, b, it->_kpos);
                ++it->_kpos;
                return 1;
            }
        }
        it->_kpos = 0;
        if (++it->_bpos == m->nb_buckets) {
            return 0;
        }
        it->_b = &m->buckets[it->_bpos];
    }
    return 0;
}
