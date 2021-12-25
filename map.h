#ifndef __DELTA_MAP_H
#define __DELTA_MAP_H

#include <stddef.h>

typedef void* map_t;

/*
 * Returns a new map.
 * It is heap allocated to store at least the given capacity.
 */
map_t map_make(size_t value_size, size_t capacity);

/*
 * Deletes the map.
 * The underlying memoty is freed.
 * If NULL is given, nothing is deleted and no error is returned.
 */
void map_del(map_t map);

/*
 * Returns the length of the map (the number of elements the map holds).
 * If NULL is passed, 0 is returned.
 */
size_t map_len(const map_t m);

/*
 * Searches the map for the given key and if found copies the associated value at the address
 * pointed to by v.
 * Returns 0 if the key wasn't found in the map.
 */
int map_get(const map_t m, const char* key, void* v);

/*
 * Returns whether the map contains the given key of not.
 */
#define map_contains(map, key) (map_get((map), (key), NULL))

/*
 * Removes the given key and its associated value from the map.
 * Returns whether the key was removed or not.
 */
int map_erase(map_t map, const char* key);

/*
 * Stores the given key and the associated value pointed to by val_ptr in the map.
 *
 * Use this function to map strings to structured values.
 * Store to a NULL map is undefined behavior.
 *
 * The map is reallocated if it has not enough capacity to hold the new value.
 *
 * The input map may be invalidated. Do not attempt to use it after calling this
 * function.
 */
map_t map_add(map_t map, const char* key, const void* val_ptr);

/*
 * Adds the given key and the associated value in the map.
 * Unlike map_add, the value must be passed by value (m = map_addv(m, "one", 1) adds the pair
 * ["one", 1]).
 *
 * Use this function to map strings to literal values like integers or pointers.
 * Add to a NULL map is undefined behavior.
 *
 *
 * The map is reallocated if it has not enough capacity to hold the new value.
 *
 * The input map may be invalidated. Do not attempt to use it after calling this
 * function.
 */
map_t map_addv(map_t map, const char* key, ...);

/*
 * Iterate on each mapped key/value pairs.
 */
void map_each(const map_t map, void (*iter_func)(const char* /* key */, void* /* value */));

/*
 * Iterate on each mapped key/value pairs with an associated context.
 */
void map_each_ctx(const map_t map,
                  void (*iter_func)(void* /* ctx */, const char* /* key */, void* /* value */),
                  void* ctx);

void map_print_internals(const map_t map);

#endif /* __DELTA_MAP_H */
