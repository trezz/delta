#include "map.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAPB_CAPA 8
#define MAP_MAX_LOAD_FACTOR 6.5

typedef struct _map_bucket {
    unsigned long hash[MAPB_CAPA];
    char* keys[MAPB_CAPA];
    char* values;
    size_t len;
    struct _map_bucket* next;
} _map_bucket;

typedef struct _map {
    unsigned long hash_seed;
    size_t value_size;
    size_t len;
    size_t capacity;

    size_t nb_buckets;
    _map_bucket* buckets;
} _map;

static void* init_new_bucket(const _map* m, _map_bucket* b) {
    b->values = malloc(m->value_size * MAPB_CAPA);
    if (b->values == NULL) {
        return NULL;
    }

    b->len = 0;
    b->next = NULL;
    return b;
}

map_t map_make(size_t value_size, size_t capacity) {
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
    m->buckets = malloc(sizeof(_map_bucket) * m->nb_buckets);
    if (m->buckets == NULL) {
        return NULL;
    }

    for (i = 0; i < m->nb_buckets; ++i) {
        _map_bucket* b = &m->buckets[i];
        if (init_new_bucket(m, b) == NULL) {
            return NULL;
        }
    }

    return m;
}

static void map_del_data(_map* m, int del_keys) {
    int i = 0;
    for (i = 0; i < m->nb_buckets; ++i) {
        _map_bucket* b = &m->buckets[i];
        while (b != NULL) {
            if (del_keys) {
                int j = 0;
                for (j = 0; j < b->len; ++j) {
                    free(b->keys[j]); /* TODO: store keys in a big buffer to alloc and free once */
                }
            }
            free(b->values);
            b = b->next;
        }
    }
    free(m->buckets);
}

void map_del(map_t m) {
    if (m == NULL) {
        return;
    }
    map_del_data(m, 1);
    free(m);
}

size_t map_len(const map_t map) {
    const _map* m = map;
    if (m == NULL) {
        return 0;
    }
    return m->len;
}

/*
 * https://stackoverflow.com/questions/7666509/hash-function-for-string
 *
 * TODO: better hash function.
 */
static unsigned long hash(const _map* m, const char* s) {
    unsigned long h = m->hash_seed;
    int c;

    while ((c = *s++) != 0) {
        h = ((h << 5) + h) + c; /* hash * 33 + c */
    }

    return h;
}

#define bucket_pos(m, h) ((h) & ((m)->nb_buckets - 1))
#define bucket_val(m, b, i) ((b)->values + ((i) * (m)->value_size))

/*
 * Finds the map bucket holding the given key and returns true if the key was found.
 * If the key is not in the map, the bucket that is expected to store the key is returned.
 */
static int find_bucket_pos(const _map* m, const char* key, unsigned long* out_key_hash,
                           _map_bucket** found_bucket, int* found_pos) {
    const unsigned long h = hash(m, key);
    const size_t bpos = bucket_pos(m, h);
    _map_bucket* b = &m->buckets[bpos];
    int i = 0;

    *out_key_hash = h;

    while (1) {
        for (i = 0; i < b->len; ++i) {
            if (h == b->hash[i]) {
                if (strcmp(b->keys[i], key) != 0) {
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

int map_get(const map_t map, const char* key, void* v) {
    const _map* m = map;
    _map_bucket* b = NULL;
    int pos = 0;
    unsigned long h = 0;

    if (!find_bucket_pos(m, key, &h, &b, &pos)) {
        return 0;
    }
    if (v != NULL) {
        memcpy(v, bucket_val(m, b, pos), m->value_size);
    }

    return 1;
}

int map_erase(map_t map, const char* key) {
    _map* m = map;
    _map_bucket* b = NULL;
    int pos = 0;
    unsigned long h = 0;

    if (!find_bucket_pos(m, key, &h, &b, &pos)) {
        return 0;
    }
    if (b->len > 1) {
        b->hash[pos] = b->hash[b->len - 1];
        b->keys[pos] = b->keys[b->len - 1];
        memcpy(bucket_val(m, b, pos), bucket_val(m, b, b->len - 1), m->value_size);
    }
    --b->len;
    --m->len;

    return 1;
}

static void* map_insert(_map* m, const char* key, const void* val_ptr, int copy_key) {
    _map_bucket* b = NULL;
    int pos = 0;
    unsigned long h = 0;

    if (find_bucket_pos(m, key, &h, &b, &pos)) {
        memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
        return m;
    }
    pos = b->len;

    if (pos == MAPB_CAPA) {
        if ((b->next = malloc(sizeof(_map_bucket))) == NULL) {
            return NULL;
        }
        if ((b = init_new_bucket(m, b->next)) == NULL) {
            return NULL;
        }
        pos = 0;
    }
    memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
    b->hash[pos] = h;
    b->keys[pos] = (copy_key ? strdup(key) : (char*)key);
    ++b->len;
    ++m->len;
    return m;
}

static _map* map_rehash(_map* m) {
    _map* n = map_make(m->value_size, m->capacity * 2);
    map_iterator_t it = map_iterator(m);
    if (n == NULL) {
        return NULL;
    }

    while (map_next(&it)) {
        if (map_insert(n, it.key, it.value, 0 /* Move keys. */) == NULL) {
            return NULL;
        }
    }

    map_del_data(m, 0 /* Do not delete keys that were moved to the new map. */);
    free(m);
    return n;
}

map_t map_addp(map_t map, const char* key, const void* val_ptr) {
    _map* m = map;
    float load_factor = m->len;

    load_factor /= (float)m->nb_buckets;
    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        if ((m = map_rehash(m)) == NULL) {
            return NULL;
        }
    }

    return map_insert(m, key, val_ptr, 1 /* Copy key. */);
}

map_t map_addv(map_t map, const char* key, ...) {
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
            return map_addp(m, key, &c);
        case 2:
            sh = (short)ll;
            return map_addp(m, key, &sh);
        case 4:
            i = (int)ll;
            return map_addp(m, key, &i);
        case 8:
            return map_addp(m, key, &ll);
        default:
            assert(0 && "unsupported value data size");
    }
    return NULL;
}

map_iterator_t map_iterator(const map_t map) {
    const _map* m = map;
    map_iterator_t it;

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

int map_next(map_iterator_t* it) {
    const _map* m = it->_map;

    while (it->_bpos < m->nb_buckets) {
        _map_bucket* b = it->_b;
        for (; b != NULL; it->_b = b = b->next, it->_kpos = 0) {
            if (it->_kpos < b->len) {
                it->key = b->keys[it->_kpos];
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
