#include "delta/strmap.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "delta/hash.h"

#define MAPB_CAPA 8
#define MAP_MAX_LOAD_FACTOR 6.5

typedef struct strmap_bucket {
    size_t hash[MAPB_CAPA];
    size_t key_positions[MAPB_CAPA];
    char* values;
    size_t len;
    struct strmap_bucket* next;
} strmap_bucket;

typedef struct strmap {
    size_t value_size;
    size_t capacity;
    void* (*malloc_func)(size_t);
    void* (*realloc_func)(void*, size_t);
    int (*strncmp_func)(const char*, const char*, size_t);

    size_t len;
    size_t nb_buckets;
    strmap_bucket* buckets;

    size_t hash_seed;
    char* keys;
    size_t keys_len;
    size_t keys_capacity;
} strmap;

static void* init_new_bucket(const strmap* m, strmap_bucket* b) {
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
    strmap* m = NULL;
    size_t i = 0;

    if ((m = config->malloc_func(sizeof(strmap))) == NULL) {
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
    if ((m->buckets = m->malloc_func(sizeof(strmap_bucket) * m->nb_buckets)) ==
        NULL) {
        return NULL;
    }
    m->keys_len = 0;
    m->keys_capacity = 1024;
    if ((m->keys = m->malloc_func(m->keys_capacity)) == NULL) {
        return NULL;
    }

    for (i = 0; i < m->nb_buckets; ++i) {
        strmap_bucket* b = &m->buckets[i];
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
    strmap* m = map;
    size_t i = 0;

    for (i = 0; i < m->nb_buckets; ++i) {
        strmap_bucket* b = &m->buckets[i];
        free(b->values);
        b = b->next;
        while (b != NULL) {
            strmap_bucket* cur = b;
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
    const strmap* m = map;
    return m->len;
}

#define bucket_pos(m, h) ((h) & ((m)->nb_buckets - 1))
#define bucket_val(m, b, i) ((b)->values + ((i) * (m)->value_size))

/*
 * Finds the map bucket holding the given key and returns 1 if the key was
 * found. If the key is not in the map, 0 is returned and the bucket that is
 * expected to store the key is set in found_bucket.
 */
static int find_bucket_pos(const strmap* m, const char* key, size_t key_len,
                           unsigned long* out_key_hash,
                           strmap_bucket** found_bucket, size_t* found_pos) {
    const unsigned long h = hash_bytes(key, key_len, m->hash_seed);
    const size_t bpos = bucket_pos(m, h);
    strmap_bucket* b = &m->buckets[bpos];
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
    const strmap* m = map;
    strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return NULL;
    }
    return bucket_val(m, b, pos);
}

int strmap_get_withlen(const strmap_t map, const char* key, size_t key_len,
                       void* v) {
    const strmap* m = map;
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
    strmap* m = map;
    strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;
    const size_t key_len = strlen(key);

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return 0;
    }
    if (b->len > 1) {
        b->hash[pos] = b->hash[b->len - 1];
        b->key_positions[pos] = b->key_positions[b->len - 1];
        memcpy(bucket_val(m, b, pos), bucket_val(m, b, b->len - 1),
               m->value_size);
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
static size_t append_new_key(strmap* m, const char* key, size_t key_len) {
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
static void* strmap_insert(strmap* m, const char* key, const void* val_ptr) {
    strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;
    const size_t key_len = strlen(key);

    if (find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
        return m;
    }
    pos = b->len;

    if (pos == MAPB_CAPA) {
        if ((b->next = malloc(sizeof(strmap_bucket))) == NULL) {
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
static strmap* strmap_rehash(strmap* m) {
    strmap* n = strmap_make(m->value_size, m->capacity * 2);
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
    strmap* m = map;
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
    strmap* m = map;
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
    const strmap* m = map;
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
    const strmap* m = it->_map;

    while (it->_bpos < m->nb_buckets) {
        strmap_bucket* b = it->_b;
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
