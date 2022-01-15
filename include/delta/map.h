#ifndef DELTA_MAP_H_
#define DELTA_MAP_H_

#include <stdbool.h>
#include <stddef.h>

#include "delta/allocator.h"

// map_key_t is a map key. It stores a pointer on the key data, the
// length of the data and a hash of the data.
//
// If the data is not NULL, the map is comparing the key data with the
// mapped key data to ensure that the key is the same in case of hash collision.
// NULL data is for literal keys that have their hash equal to their value (like
// an integer for example).
typedef struct map_key_t {
    const void* data;
    size_t len;
    size_t hash;
} map_key_t;

#define map_key_from(V) \
    _Generic((V),\
        char:               ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        unsigned char:      ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        short:              ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        unsigned short:     ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        int:                ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        unsigned int:       ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        long:               ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        unsigned long:      ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        long long:          ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        unsigned long long: ((map_key_t){.data = NULL, .len = 0, .hash = (size_t)V}), \
        char*:              (map_key_from_cstr((void*)(intptr_t)(V))), \
        const char*:        (map_key_from_cstr((const void*)(intptr_t)(V))), \
        default:            (V))

map_key_t map_key_from_cstr(const char* s);

#define map_key_as(T, key)                                  \
    (*((T const*)((key).len == 0 ? (const void*)&(key).hash \
                                 : (const void*)&(key).data)))

// map_t(ValueType) is the type for a map of values of type ValueType.
//
// Type type map_t(void) is a map of values of any type. The type
// map_t(const void) is a map of values of an constant type.
#define map_t(ValueType) ValueType*

// map_make_alloc behaves as map_make, but set the new map to manage
// memory using the provided allocator instead of the default one.
//
// NULL is returned in case of error.
#define map_make_alloc(ValueType, capacity, allocator)                    \
    ((map_t(ValueType))map_make_alloc_impl(sizeof(ValueType), (capacity), \
                                           (allocator)))

// map_make returns a new map_t(ValueType) of the given internal storage
// capacity.
//
// NULL is returned in case of error.
#define map_make(ValueType, capacity) \
    map_make_alloc(ValueType, capacity, &default_allocator)

// map_valid returns whether the given map is valid or not.
// Returns false if NULL is given.
bool map_valid(map_t(const void) map);

// map_del deletes the given map. The underlying memory is deallocated.
// Deleting a NULL map is a noop.
void map_del(map_t(void) map);

// map_len returns the number of elements the given map holds.
// Returns 0 if NULL is given.
size_t map_len(map_t(const void) map);

// map_copy returns a copy of the given map.
//
// NULL is returned in case of error.
#define map_copy(map) ((__typeof__(map))map_copy_impl(map))

// map_at returns a pointer on the value mapped at the given key, or NULL if
// the key is not present in the map.
#define map_at(map, key) ((__typeof__(map))map_at_impl((map), (key)))

// map_get takes **a pointer on a map** and returns the value mapped at
// the given key. If the key wasn't present in the map, it is inserted and
// the zero-initialized value mapped to the inserted key is returned.
//
// If an error occurs, the map is set as invalid. Use map_valid to check
// the validity status of the returned map.
#define map_get(map_ptr, key) \
    (*((__typeof__(*map_ptr))map_insert_impl((map_ptr), (key))))

// map_erase takes **a pointer on a map** and removes the given key and
// its mapped value from the map and returns whether the key was present in
// the map or not.
//
// If an error occurs, the map is set as invalid. Use map_valid to check
// the validity status of the returned map.
#define map_erase(map_ptr, key) map_erase_impl((map_ptr), (key))

// map_iterator_t is an iterator on a map containing the current key,
// a pointer on the current value, a valid field that can be checked to have the
// validity status of the iterator and internal data used to iterate on the
// map.
typedef struct map_iterator_t {
    map_key_t key;
    void* value;
    bool valid;

    void* _bucket;
    size_t _bucket_pos;
    size_t _key_pos;
} map_iterator_t;

// map_iterator_valid returns whether the iterator is valid or not.
#define map_iterator_valid(it) (it.valid)

// map_iterator_make creates a new uninitialized iterator on the map.
// The returned iterator must be initialized with map_iterator_next.
map_iterator_t map_iterator_make(map_t(const void) map);

// map_iterator_next moves the given iterator to the next key/value mapping
// and returns true, unless the iterator reaches the end of the map, in
// which case the iterator is set as invalid and the function returns false.
bool map_iterator_next(map_t(const void) map, map_iterator_t* it);

// map_iterator_value returns the current value of the iterator.
#define map_iterator_value(map, it) (*((__typeof__(map))it.value))

// Private implementations.

map_t(void) map_make_alloc_impl(size_t value_size, size_t capacity,
                                const allocator_t* allocator);

map_t(void) map_copy_impl(map_t(const void) map);

void* map_at_impl(map_t(const void) map, map_key_t key);

void* map_insert_impl(void* map_ptr, map_key_t key);

bool map_erase_impl(void* map_ptr, map_key_t key);

#endif  // DELTA_MAP_H_
