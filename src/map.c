#include "delta/map.h"

#include "delta/allocator.h"
#include "delta/hash.h"
#include "delta/vec.h"
#include "string.h"

#define MAP_BUCKET_CAPACITY 8
#define MAP_MAX_LOAD_FACTOR 6.5
#define MAP_DEFAULT_SEED 13
#define MAP_BUCKET_DEFAULT_BUFFER_CAPACITY 1024

typedef struct map_entry_t {
    size_t pos;
    size_t len;
    size_t hash;
} map_entry_t;

typedef struct map_bucket_t {
    map_entry_t entries[MAP_BUCKET_CAPACITY];
    size_t len;
    struct map_bucket_t* next;

    // _keys_data_buffer stores the mapped keys data. The data of a key starts
    // at map_entry_t.data_pos and is of length map_entry_t.data_len.
    vec_t(char) _keys_data_buffer;
    // _values_buffer stores the mapped values. Each stored value takes exactly
    // map_desc_t.value_size bytes.
    vec_t(char) _values_buffer;
} map_bucket_t;

typedef struct map_desc_t {
    size_t value_size;
    size_t capacity;
    bool valid;
    size_t len;

    const allocator_t* _allocator;
    vec_t(map_bucket_t) _buckets;
} map_desc_t;

static map_desc_t* map_alloc(const allocator_t* allocator) {
    return allocator_alloc(allocator, sizeof(map_desc_t));
}

static bool map_bucket_init(const map_desc_t* m, map_bucket_t* b) {
    b->len = 0;
    b->next = NULL;

    b->_keys_data_buffer =
        vec_make(char, 0, MAP_BUCKET_DEFAULT_BUFFER_CAPACITY);
    if (b->_keys_data_buffer == NULL) {
        return false;
    }

    const size_t vb_size = MAP_BUCKET_DEFAULT_BUFFER_CAPACITY * m->value_size;
    b->_values_buffer = vec_make(char, vb_size, vb_size);
    if (b->_values_buffer == NULL) {
        vec_del(b->_keys_data_buffer);
        return false;
    }

    return true;
}

static void map_bucket_deinit(const map_desc_t* m, map_bucket_t* b) {
    vec_del(b->_keys_data_buffer);
    vec_del(b->_values_buffer);

    if (b->next) {
        map_bucket_deinit(m, b->next);
        allocator_dealloc(m->_allocator, b->next);
    }
}

map_key_t map_key_from_cstr(const char* s) {
    const size_t len = strlen(s);
    return (map_key_t){
        .data = s,
        .len = len + 1,
        .hash = hash_bytes(s, len, MAP_DEFAULT_SEED),
    };
}

void* map_make_alloc_impl(size_t value_size, size_t capacity,
                          const allocator_t* allocator) {
    map_desc_t* m = map_alloc(allocator);
    if (m == NULL) {
        return NULL;
    }

    m->value_size = value_size;
    m->capacity = capacity;
    m->valid = true;
    m->len = 0;

    m->_allocator = allocator;

    if (m->capacity == 0) {
        ++m->capacity;
    }
    while (m->capacity % 8 != 0) {
        ++m->capacity;
    }

    const size_t nb_buckets = m->capacity / MAP_BUCKET_CAPACITY;
    assert(nb_buckets > 0 && "given invalid capacity");
    m->_buckets =
        vec_make_alloc(map_bucket_t, nb_buckets, nb_buckets, allocator);
    if (m->_buckets == NULL) {
        map_del(m);
        return NULL;
    }

    for (size_t i = 0; i < nb_buckets; ++i) {
        map_bucket_t* b = &m->_buckets[i];
        if (!map_bucket_init(m, b)) {
            map_del(m);
            return NULL;
        }
    }

    return m;
}

bool map_valid(const void* map) {
    if (map == NULL) {
        return false;
    }
    const map_desc_t* const m = map;
    return m->valid;
}

void map_del(void* map) {
    if (map == NULL) {
        return;
    }

    map_desc_t* m = map;

    for (size_t i = 0; i < vec_len(m->_buckets); ++i) {
        map_bucket_deinit(m, &m->_buckets[i]);
    }
    vec_del(m->_buckets);

    allocator_dealloc(m->_allocator, m);
}

size_t map_len(const void* map) {
    if (map == NULL) {
        return 0;
    }
    const map_desc_t* const m = map;
    return m->len;
}

void* map_copy_impl(const void* map) {
    const map_desc_t* const m = map;

    map_desc_t* n =
        map_make_alloc_impl(m->value_size, m->capacity, m->_allocator);
    if (n == NULL) {
        return NULL;
    }

    for (map_iterator_t it = map_iterator_make(m); map_iterator_next(m, &it);) {
        if (!it.valid) {
            map_del(n);
            return NULL;
        }

        void* value = map_insert_impl(&n, it.key);
        if (!map_valid(n) || value == NULL) {
            map_del(n);
            return NULL;
        }
        memcpy(value, it.value, n->value_size);
    }

    return n;
}

typedef struct map_bucket_pos_t {
    map_bucket_t* bucket;
    map_bucket_t* prev;
    size_t pos;
} map_bucket_pos_t;

static map_bucket_pos_t map_bucket_pos(const map_desc_t* m, map_key_t key) {
    const size_t nb_buckets = vec_len(m->_buckets);
    const size_t bucket_pos = key.hash & (nb_buckets - 1);
    map_bucket_t* prev = &m->_buckets[bucket_pos];

    for (map_bucket_t* b = prev; b != NULL; prev = b, b = b->next) {
        for (size_t i = 0; i < b->len; ++i) {
            const map_entry_t* entry = &b->entries[i];
            if (key.hash == entry->hash) {
                if (entry->len != key.len) {
                    continue;
                }
                char* entry_data = &b->_keys_data_buffer[entry->pos];
                if (memcmp(entry_data, key.data, entry->len) != 0) {
                    continue;
                }

                // Entry found.
                return (map_bucket_pos_t){
                    .bucket = b,
                    .prev = prev,
                    .pos = i,
                };
            }
        }
    }

    return (map_bucket_pos_t){.bucket = NULL, .prev = prev};
}

void* map_at_impl(const void* map, map_key_t key) {
    const map_desc_t* const m = map;
    const map_bucket_pos_t bp = map_bucket_pos(m, key);
    const map_bucket_t* b = bp.bucket;
    return b ? &b->_values_buffer[bp.pos * m->value_size] : NULL;
}

void* map_insert_impl(void* map_ptr, map_key_t key) {
    map_desc_t** m_ptr = map_ptr;
    const map_bucket_pos_t bp = map_bucket_pos(*m_ptr, key);
    map_bucket_t* b = bp.bucket;

    if (b != NULL) {
        // Entry already present.
        return &b->_values_buffer[bp.pos * (*m_ptr)->value_size];
    }

    map_desc_t* m = *m_ptr;
    const size_t nb_buckets = vec_len((*m_ptr)->_buckets);
    const double load_factor = (double)((*m_ptr)->len) / (double)nb_buckets;
    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        (*m_ptr)->capacity *= 2;
        map_desc_t* n = map_copy_impl(*m_ptr);
        if (n == NULL) {
            (*m_ptr)->valid = false;
            return NULL;
        }
        map_del(*m_ptr);
        *m_ptr = m = n;
    }

    b = bp.prev;
    assert(b != NULL && "couldn't find a bucket for the given key");

    if (b->len == MAP_BUCKET_CAPACITY) {
        b->next = allocator_alloc(m->_allocator, sizeof(map_bucket_t));
        if (b->next == NULL) {
            m->valid = false;
            return NULL;
        }
        b = b->next;
        if (!map_bucket_init(m, b)) {
            m->valid = false;
            return NULL;
        }
    }
    const size_t pos = b->len;

    ++b->len;
    ++m->len;

    // Copy the inserted value.
    char* value = &b->_values_buffer[pos * m->value_size];
    memset(value, 0, m->value_size);

    // Copy the key data.
    const size_t entries_data_size = vec_len(b->_keys_data_buffer);
    if (key.len > 0) {
        vec_resize(&b->_keys_data_buffer, entries_data_size + key.len);
        if (!vec_valid(b->_keys_data_buffer)) {
            m->valid = false;
            return value;
        }
        memcpy((char*)b->_keys_data_buffer + entries_data_size, key.data,
               key.len);
    }

    // Insert the new entry.
    b->entries[pos].hash = key.hash;
    b->entries[pos].len = key.len;
    b->entries[pos].pos = entries_data_size;

    return value;
}

bool map_erase_impl(void* map_ptr, map_key_t key) {
    map_desc_t** m_ptr = map_ptr;
    map_desc_t* m = *m_ptr;
    const map_bucket_pos_t bp = map_bucket_pos(m, key);
    map_bucket_t* b = bp.bucket;

    if (b == NULL) {
        return false;
    }

    if (b->len > 1) {
        // Swap erased entry and value with the last one. Keys data are not
        // swapped, which means that erased keys are kept in the map until a
        // rehash happens.
        b->entries[bp.pos] = b->entries[b->len - 1];
        char* erased = &b->_values_buffer[bp.pos * m->value_size];
        const char* last = &b->_values_buffer[(b->len - 1) * m->value_size];
        memcpy(erased, last, m->value_size);
    }

    --b->len;
    --m->len;

    return true;
}

map_iterator_t map_iterator_make(const void* map) {
    const map_desc_t* m = map;
    map_iterator_t it;

    it.key = (map_key_t){.data = NULL, .len = 0, .hash = 0};
    it.value = NULL;
    it.valid = false;
    it._bucket = NULL;
    it._bucket_pos = 0;
    it._key_pos = 0;

    if (m->len == 0) {
        return it;
    }
    it._bucket = &m->_buckets[0];

    return it;
}

bool map_iterator_next(const void* map, map_iterator_t* it) {
    const map_desc_t* m = map;
    const size_t nb_buckets = vec_len(m->_buckets);

    while (it->_bucket_pos < nb_buckets) {
        map_bucket_t* b = it->_bucket;

        for (; b != NULL; it->_bucket = b = b->next, it->_key_pos = 0) {
            if (it->_key_pos < b->len) {
                const map_entry_t* e = &b->entries[it->_key_pos];
                it->key = (map_key_t){
                    .data = &b->_keys_data_buffer[e->pos],
                    .len = e->len,
                    .hash = e->hash,
                };
                it->value = &b->_values_buffer[it->_key_pos * m->value_size];
                it->valid = true;
                ++it->_key_pos;
                return true;
            }
        }
        it->_key_pos = 0;
        if (++it->_bucket_pos == nb_buckets) {
            it->valid = false;
            return false;
        }
        it->_bucket = &m->_buckets[it->_bucket_pos];
    }

    it->valid = false;
    return false;
}
