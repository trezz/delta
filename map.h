#ifndef __DELTA_MAP_H
#define __DELTA_MAP_H

#include <stddef.h>

typedef void* map_t;

map_t map_make(size_t value_size, size_t capacity);

void map_del(map_t map);

size_t map_len(const map_t m);

int map_get(const map_t m, const char* key, void* v);

#define map_contains(map, key) (map_get((map), (key), NULL))

int map_erase(map_t map, const char* key);

int map_store(map_t map, const char* key, const void* val_ptr);

int map_add(map_t map, const char* key, ...);

/*
 * Iterate on each mapped key/value pairs.
 */
void map_each(const map_t map, void (*iter_func)(const char* /* key */, const void* /* value */));

/*
 * Iterate on each mapped key/value pairs with an associated context.
 */
void map_each_ctx(const map_t map,
                  void (*iter_func)(void* /* ctx */, const char* /* key */,
                                    const void* /* value */),
                  void* ctx);

void map_print_internals(const map_t map);

#endif /* __DELTA_MAP_H */
