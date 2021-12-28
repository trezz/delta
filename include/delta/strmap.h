#ifndef DELTA_STRMAP_H_
#define DELTA_STRMAP_H_

#include <stddef.h>

typedef void* strmap_t;

/*
 * Configuration of a strmap.
 */
typedef struct _strmap_config {
    /* Size of the mapped value type. */
    size_t value_size;
    /* Initial capacity of the map. */
    size_t capacity;
    /* Allocator function (the default config uses malloc). */
    void* (*malloc_func)(size_t);
    /* Re-allocator function (the default config uses realloc). */
    void* (*realloc_func)(void*, size_t);
    /* String comparison function (the default config uses strncmp). */
    int (*strncmp_func)(const char*, const char*, size_t);
} strmap_config_t;

/*
 * Returns a new configuration of an strmap.
 */
strmap_config_t strmap_config(size_t value_size, size_t capacity);

/*
 * Returns a new map configured according to the provided configuration.
 * NULL is returned in case of error.
 */
strmap_t strmap_make_from_config(const strmap_config_t* config);

/*
 * Returns a new map.
 * It is heap allocated to store at least the given capacity.
 * NULL is returned in case of error.
 */
strmap_t strmap_make(size_t value_size, size_t capacity);

/*
 * Deletes the map.
 * The underlying memoty is freed.
 */
void strmap_del(strmap_t map);

/*
 * Returns the length of the map (the number of elements the map holds).
 */
size_t strmap_len(const strmap_t m);

/*
 * Searches the map for the given key and if found copies the associated value at the address
 * pointed to by v.
 * Returns 0 if the key wasn't found in the map.
 */
int strmap_get(const strmap_t m, const char* key, void* v);

/*
 * Returns whether the map contains the given key of not.
 */
#define strmap_contains(map, key) (strmap_get((map), (key), NULL))

/*
 * Removes the given key and its associated value from the map.
 * Returns whether the key was removed or not.
 */
int strmap_erase(strmap_t map, const char* key);

/*
 * Stores the given key and the associated value pointed to by val_ptr in the map.
 *
 * Use this function to map strings to structured values.
 *
 * The map is reallocated if it has not enough capacity to hold the new value.
 *
 * The input map may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * NULL is returned in case of error.
 */
strmap_t strmap_addp(strmap_t map, const char* key, const void* val_ptr);

/*
 * Stores the given key and the associated value in the map.
 * Unlike strmap_addp, the value must be passed by value (m = strmap_addv(m, "one", 1) adds the pair
 * ["one", 1]).
 *
 * Use this function to map strings to literal values like integers or pointers.
 *
 * The map is reallocated if it has not enough capacity to hold the new value.
 *
 * The input map may be invalidated. Do not attempt to use it after calling this
 * function.
 *
 * NULL is returned in case of error.
 */
strmap_t strmap_addv(strmap_t map, const char* key, ...);

/*
 * An iterator on a map.
 */
typedef struct _strmap_iterator {
    /* Current key. */
    const char* key;
    /* Pointer on the current value. */
    void* val_ptr;

    /* Internal state. */
    strmap_t _map;
    void* _b;
    size_t _bpos;
    size_t _kpos;
} strmap_iterator_t;

/*
 * Returns an iterator on the map.
 * The returned iterator is initialized to iterate on the map, but doesn't points to any key/value
 * pair yet. A call to strmap_next is required to set the iterator on the first key/value pair.
 */
strmap_iterator_t strmap_iterator(const strmap_t map);

/*
 * Move the given iterator to the next key/value pair of the map, or to the first key/value pair if
 * the iterator was just initialized by strmap_iterator.
 * If the function returns 0, the iteration reached the end of the map and the iterator state is
 * undefined, otherwise the iterator is pointing to a key/value pair of the map.
 */
int strmap_next(strmap_iterator_t* it);

#endif /* DELTA_STRMAP_H_ */
