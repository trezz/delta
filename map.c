#include "./map.h"

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

typedef struct map {
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

static void map_del_data(_map* m) {
    int i = 0;
    for (i = 0; i < m->nb_buckets; ++i) {
        _map_bucket* b = &m->buckets[i];
        while (b != NULL) {
            int j = 0;
            for (j = 0; j < b->len; ++j) {
                free(b->keys[j]);
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
    map_del_data(m);
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

#define bucket_pos(m, h) (h & (m->nb_buckets - 1))
#define bucket_val(m, b, i) (b->values + ((i)*m->value_size))

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

static void map_inserter(void* ctx, const char* key, const void* v) {
    _map* m = ctx;
    map_store(m, key, v);
}

static _map* map_rehash(_map* m) {
    _map* n = map_make(m->value_size, m->capacity * 2);
    if (n == NULL) {
        return NULL;
    }

    map_each_ctx(m, map_inserter, n);
    map_del_data(m);
    *m = *n;
    return m;
}

int map_store(map_t map, const char* key, const void* val_ptr) {
    _map* m = map;
    _map_bucket* b = NULL;
    int pos = 0;
    unsigned long h = 0;
    float load_factor = m->len;

    load_factor /= (float)m->nb_buckets;
    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        if (map_rehash(m) == NULL) {
            return 0;
        }
    }

    if (find_bucket_pos(m, key, &h, &b, &pos)) {
        memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
        return 1;
    }
    pos = b->len;

    if (pos == MAPB_CAPA) {
        b->next = malloc(sizeof(_map_bucket));
        if (b->next == NULL || (init_new_bucket(m, b->next) == NULL)) {
            return 0;
        }
        b = b->next;
        pos = 0;
    }

    memcpy(bucket_val(m, b, pos), val_ptr, m->value_size);
    b->hash[pos] = h;
    b->keys[pos] = strdup(key);
    ++b->len;
    ++m->len;
    return 1;
}

int map_add(map_t map, const char* key, ...) {
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
            return map_store(m, key, &c);
        case 2:
            sh = (short)ll;
            return map_store(m, key, &sh);
        case 4:
            i = (int)ll;
            return map_store(m, key, &i);
        case 8:
            return map_store(m, key, &ll);
        default:
            assert(0 && "unsupported value data size");
    }
    return 0;
}

void map_each(const map_t map, void (*iter_func)(const char*, const void*)) {
    const _map* m = map;
    int i = 0;
    for (i = 0; i < m->nb_buckets; ++i) {
        const _map_bucket* b = &m->buckets[i];
        while (b != NULL) {
            int j = 0;
            for (j = 0; j < b->len; ++j) {
                iter_func(b->keys[j], bucket_val(m, b, j));
            }
            b = b->next;
        }
    }
}

void map_each_ctx(const map_t map, void (*iter_func)(void*, const char*, const void*), void* ctx) {
    const _map* m = map;
    int i = 0;
    for (i = 0; i < m->nb_buckets; ++i) {
        const _map_bucket* b = &m->buckets[i];
        while (b != NULL) {
            int j = 0;
            for (j = 0; j < b->len; ++j) {
                iter_func(ctx, b->keys[j], bucket_val(m, b, j));
            }
            b = b->next;
        }
    }
}

void map_print_internals(const map_t map) {
    const _map* m = map;
    int i = 0;

    printf("--\n");
    printf("hash seed: %lu\n", m->hash_seed);
    printf("value size: %zu\n", m->value_size);
    printf("len: %zu\n", m->len);
    printf("capacity: %zu\n", m->capacity);
    printf("nb buckets: %zu\n", m->nb_buckets);
    printf("--\n");

    for (i = 0; i < m->nb_buckets; ++i) {
        const _map_bucket* b = &m->buckets[i];
        int k = 0;
        for (k = 0; b != NULL; b = b->next, ++k) {
            int j = 0;
            printf("[%d|%d]\tlen=%zu\t{", i, k, b->len);
            for (j = 0; j < b->len; ++j) {
                printf(" [%d] \"%s\"", j, b->keys[j]);
                if (j + 1 < b->len) {
                    printf(",");
                }
            }
            printf(" }\n");
        }
    }
}
