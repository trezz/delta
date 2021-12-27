#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strmap.h"

#define MAPB_CAPA 8
#define MAP_MAX_LOAD_FACTOR 6.5

typedef struct _strmap_bucket {
    unsigned long hash[MAPB_CAPA];
    size_t key_positions[MAPB_CAPA];
    char* values;
    size_t len;
    struct _strmap_bucket* next;
} _strmap_bucket;

typedef struct _map {
    unsigned long hash_seed;
    size_t value_size;
    size_t len;
    size_t capacity;

    size_t nb_buckets;
    _strmap_bucket* buckets;

    char* keys;
    size_t keys_len;
    size_t keys_capacity;
} _map;

static void* init_new_bucket(const _map* m, _strmap_bucket* b) {
    b->values = malloc(m->value_size * MAPB_CAPA);
    if (b->values == NULL) {
        return NULL;
    }

    b->len = 0;
    b->next = NULL;
    return b;
}

strmap_t strmap_make(size_t value_size, size_t capacity) {
    _map* m = NULL;
    int i = 0;

    m = malloc(sizeof(_map));
    if (m == NULL) {
        return NULL;
    }

    if (capacity == 0) {
        capacity = 1;
    }
    while (capacity % 8 != 0) {
        ++capacity;
    }

    m->hash_seed = 5381; /* https://stackoverflow.com/questions/7666509/hash-function-for-string */
    m->value_size = value_size;
    m->len = 0;
    m->capacity = capacity;
    m->nb_buckets = capacity / MAPB_CAPA;
    m->buckets = malloc(sizeof(_strmap_bucket) * m->nb_buckets);
    if (m->buckets == NULL) {
        return NULL;
    }

    m->keys_capacity = 1024;
    m->keys_len = 0;
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
    int i = 0;

    if (m == NULL) {
        return;
    }

    for (i = 0; i < m->nb_buckets; ++i) {
        _strmap_bucket* b = &m->buckets[i];
        free(b->values);
        b = b->next;
    }

    free(m->buckets);
    free(m->keys);
    free(m);
}

size_t strmap_len(const strmap_t map) {
    const _map* m = map;
    if (m == NULL) {
        return 0;
    }
    return m->len;
}

/*
 * https://stackoverflow.com/questions/7666509/hash-function-for-string
 *
 * TODO: better and faster hash function.
 */
static unsigned long hash(const _map* m, const char* s, size_t len) {
    unsigned long h = m->hash_seed;
    size_t* input = (void*)s;
    size_t input_len = len / sizeof(size_t);
    int i = 0;
    int c;

    for (i = 0; i < input_len; ++i) {
        h = ((h << 5) + h) + input[i];
    }
    s += input_len * sizeof(size_t);
    while ((c = *s++) != 0) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

#define bucket_pos(m, h) ((h) & ((m)->nb_buckets - 1))
#define bucket_val(m, b, i) ((b)->values + ((i) * (m)->value_size))

/*
 * Finds the map bucket holding the given key and returns true if the key was found.
 * If the key is not in the map, the bucket that is expected to store the key is returned.
 */
static int find_bucket_pos(const _map* m, const char* key, size_t key_len,
                           unsigned long* out_key_hash, _strmap_bucket** found_bucket,
                           int* found_pos) {
    const unsigned long h = hash(m, key, key_len);
    const size_t bpos = bucket_pos(m, h);
    _strmap_bucket* b = &m->buckets[bpos];
    int i = 0;

    *out_key_hash = h;

    while (1) {
        for (i = 0; i < b->len; ++i) {
            if (h == b->hash[i]) {
                const char* bkey = m->keys + b->key_positions[i];
                if (strcmp(bkey, key) != 0) {
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

int strmap_get(const strmap_t map, const char* key, void* v) {
    const _map* m = map;
    _strmap_bucket* b = NULL;
    int pos = 0;
    unsigned long h = 0;
    const size_t key_len = strlen(key);

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return 0;
    }
    if (v != NULL) {
        memcpy(v, bucket_val(m, b, pos), m->value_size);
    }

    return 1;
}

int strmap_erase(strmap_t map, const char* key) {
    _map* m = map;
    _strmap_bucket* b = NULL;
    int pos = 0;
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

int insert_new_key(_map* m, _strmap_bucket* b, size_t pos, const char* key) {
    const size_t key_len = strlen(key) + 1;
    int key_pos = 0;

    while (m->keys_len + key_len > m->keys_capacity) {
        m->keys_capacity *= 2;
        if ((m->keys = realloc(m->keys, m->keys_capacity)) == NULL) {
            return -1;
        }
    }

    key_pos = m->keys_len;
    memcpy(m->keys + key_pos, key, key_len);
    m->keys_len += key_len;

    return key_pos;
}

static void* strmap_insert(_map* m, const char* key, const void* val_ptr) {
    _strmap_bucket* b = NULL;
    int pos = 0;
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
    b->key_positions[pos] = insert_new_key(m, b, pos, key);
    if (b->key_positions[pos] == -1) {
        return NULL;
    }

    ++b->len;
    ++m->len;
    return m;
}

static _map* strmap_rehash(_map* m) {
    _map* n = strmap_make(m->value_size, m->capacity * 2);
    strmap_iterator_t it = strmap_iterator(m);
    if (n == NULL) {
        return NULL;
    }

    while (strmap_next(&it)) {
        if (strmap_insert(n, it.key, it.value) == NULL) {
            return NULL;
        }
    }

    strmap_del(m);

    return n;
}

strmap_t strmap_addp(strmap_t map, const char* key, const void* val_ptr) {
    _map* m = map;
    float load_factor = m->len;

    load_factor /= (float)m->nb_buckets;
    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        if ((m = strmap_rehash(m)) == NULL) {
            return NULL;
        }
    }

    return strmap_insert(m, key, val_ptr);
}

strmap_t strmap_addv(strmap_t map, const char* key, ...) {
    _map* m = map;
    char c = 0;
    short sh = 0;
    int i = 0;
    long long ll = 0;
    va_list args;

    assert(sizeof(c) == 1 && "unsupported runtime environment");
    assert(sizeof(sh) == 2 && "unsupported runtime environment");
    assert(sizeof(i) == 4 && "unsupported runtime environment");
    assert(sizeof(ll) == 8 && "unsupported runtime environment");

    va_start(args, key);
    ll = va_arg(args, long long);
    va_end(args);

    switch (m->value_size) {
        case 1:
            c = (char)ll;
            return strmap_addp(m, key, &c);
        case 2:
            sh = (short)ll;
            return strmap_addp(m, key, &sh);
        case 4:
            i = (int)ll;
            return strmap_addp(m, key, &i);
        case 8:
            return strmap_addp(m, key, &ll);
        default:
            assert(0 && "unsupported value data size");
    }
    return NULL;
}

strmap_iterator_t strmap_iterator(const strmap_t map) {
    const _map* m = map;
    strmap_iterator_t it;

    it.key = NULL;
    it.value = NULL;
    it._map = map;
    it._bpos = 0;
    it._kpos = 0;

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
                it->value = bucket_val(m, b, it->_kpos);
                ++it->_kpos;
                return 1;
            };
        }
        it->_kpos = 0;
        if (++it->_bpos == m->nb_buckets) {
            return 0;
        }
        it->_b = &m->buckets[it->_bpos];
    }
    return 0;
}
