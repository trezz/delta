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

typedef struct strmap_key_t {
    size_t pos;
    size_t len;
} strmap_key_t;

typedef struct _strmap_bucket {
    size_t hash[MAPB_CAPA];
    strmap_key_t keys[MAPB_CAPA];
    char* values;
    size_t len;
    struct _strmap_bucket* next;
} _strmap_bucket;

typedef struct _map {
    size_t value_size;
    size_t capacity;

    size_t len;
    size_t nb_buckets;
    _strmap_bucket* buckets;

    size_t hash_seed;
    char* keys;
    size_t keys_len;
    size_t keys_capacity;
} _map;

static void* init_new_bucket(const _map* m, _strmap_bucket* b) {
    if ((b->values = malloc(m->value_size * MAPB_CAPA)) == NULL) {
        return NULL;
    }
    b->len = 0;
    b->next = NULL;
    return b;
}

strmap_t strmap_make(size_t value_size, size_t capacity) {
    _map* m = NULL;
    size_t i = 0;

    if ((m = malloc(sizeof(_map))) == NULL) {
        return NULL;
    }

    m->value_size = value_size;
    m->capacity = capacity;

    if (m->capacity == 0) {
        ++m->capacity;
    }
    while (m->capacity % 8 != 0) {
        ++m->capacity;
    }

    m->len = 0;

    m->hash_seed = 13;
    m->nb_buckets = m->capacity / MAPB_CAPA;
    if ((m->buckets = malloc(sizeof(_strmap_bucket) * m->nb_buckets)) == NULL) {
        return NULL;
    }
    m->keys_len = 0;
    m->keys_capacity = 1024;
    if ((m->keys = malloc(m->keys_capacity)) == NULL) {
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
 * Finds the map bucket holding the given key and returns 1 if the key was
 * found. If the key is not in the map, 0 is returned and the bucket that is
 * expected to store the key is set in found_bucket.
 */
static int find_bucket_pos(const _map* m, str_t key,
                           unsigned long* out_key_hash,
                           _strmap_bucket** found_bucket, size_t* found_pos) {
    const unsigned long h = hash_bytes(key.data, key.len, m->hash_seed);
    const size_t bpos = bucket_pos(m, h);
    _strmap_bucket* b = &m->buckets[bpos];
    size_t i = 0;

    *out_key_hash = h;

    while (1) {
        for (i = 0; i < b->len; ++i) {
            if (h == b->hash[i]) {
                const char* bkey = m->keys + b->keys[i].pos;
                if (memcmp(key.data, bkey, key.len) != 0) {
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

int strmap_get(const strmap_t map, str_t key, void* v) {
    const _map* m = map;
    const void* data = strmap_at(map, key);
    if (data == NULL) {
        return 0;
    }
    if (v != NULL) {
        memcpy(v, data, m->value_size);
    }
    return 1;
}

void* strmap_at(const strmap_t map, str_t key) {
    const _map* m = map;
    _strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;

    if (!find_bucket_pos(m, key, &h, &b, &pos)) {
        return NULL;
    }
    return bucket_val(m, b, pos);
}

int strmap_erase(strmap_t map, str_t key) {
    _map* m = map;
    _strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;

    if (!find_bucket_pos(m, key, &h, &b, &pos)) {
        return 0;
    }
    if (b->len > 1) {
        b->hash[pos] = b->hash[b->len - 1];
        b->keys[pos] = b->keys[b->len - 1];
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
static size_t append_new_key(_map* m, str_t key) {
    size_t key_pos = 0;
    size_t key_len = key.len;

    ++key_len;
    while (m->keys_len + key_len > m->keys_capacity) {
        m->keys_capacity *= 2;
        if ((m->keys = realloc(m->keys, m->keys_capacity)) == NULL) {
            return SIZE_MAX;
        }
    }

    key_pos = m->keys_len;
    memcpy(m->keys + key_pos, key.data, key_len);
    m->keys_len += key_len;

    return key_pos;
}

/*
 * Insert a new key/value pair in the map and return a pointer to the map.
 * NULL is returned in case of error.
 */
static void* strmap_insert(_map* m, str_t key, const void* val_ptr) {
    _strmap_bucket* b = NULL;
    size_t pos = 0;
    unsigned long h = 0;

    if (find_bucket_pos(m, key, &h, &b, &pos)) {
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

    const size_t new_key_pos = append_new_key(m, key);
    if (new_key_pos == SIZE_MAX) {
        return NULL;
    }

    b->hash[pos] = h;
    b->keys[pos] = (strmap_key_t){.pos = new_key_pos, .len = key.len};

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

strmap_t strmap_addp(strmap_t map, str_t key, const void* val_ptr) {
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

strmap_t strmap_addv(strmap_t map, str_t key, ...) {
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
                it->key = (str_t){
                    .data = m->keys + b->keys[it->_kpos].pos,
                    .len = b->keys[it->_kpos].len,
                };
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
