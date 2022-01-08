#ifndef DELTA_STRMAP_H_
#define DELTA_STRMAP_H_

#include <stddef.h>
#include <stdint.h>

#include "delta/str.h"

#define strmap_t(ValueType) ValueType*

/*
 * Returns a new map.
 * It is heap allocated to store at least the given capacity.
 * NULL is returned in case of error.
 */
#define strmap_make(ValueType, capacity) \
    (strmap_t(ValueType))(strmap_make_impl(sizeof(ValueType), (capacity)))

strmap_t(void) strmap_make_impl(size_t value_size, size_t capacity);

/*
 * Deletes the map.
 * The underlying memoty is freed.
 */
void strmap_del(strmap_t(void) map);

/*
 * Returns the length of the map (the number of elements the map holds).
 */
size_t strmap_len(const strmap_t(void) m);

/*
 * Removes the given key and its associated value from the map.
 * Returns whether the key was removed or not.
 */
int strmap_erase(strmap_t(void) map, str_t key);

// TODO:
// strmap_get(&map, key) -> *ValueType : Insert if not exists and return a ptr.
// strmap_contains(map, key) -> bool

void* strmap_getp_impl(void* map_ptr, str_t key);

/*
#define strmap_get(map_ptr, key) \
    (*map_ptr) - (*map_ptr) + ((intptr_t)strmap_getp_impl((map_ptr), (key)))
*/

#define strmap_add(map_ptr, key, value)                             \
    do {                                                            \
        void* v_ptr##__line__ = strmap_getp_impl((map_ptr), (key)); \
        intptr_t v_addr##__line__ = (intptr_t)v_ptr##__line__;      \
        void* map_ptr_tmp##__line__ = *(map_ptr);                   \
        *(map_ptr) = (void*)v_addr##__line__;                       \
        **(map_ptr) = (value);                                      \
        *(map_ptr) = map_ptr_tmp##__line__;                         \
    } while (0)

/*
 * Searches the map for the given key and if found copies the associated value
 * at the address pointed to by v. Returns 0 if the key wasn't found in the map.
 */
int strmap_get(const strmap_t(void) m, str_t key, void* v);

/*
 * Searches the map for the given key and if found returns a pointer on the
 * associated value. Returns NULL if the key wasn't found in the map.
 */
void* strmap_at(const strmap_t(void) m, str_t key);

/*
 * An iterator on a map.
 */
typedef struct _strmap_iterator {
    /* Current key. */
    str_t key;
    /* Pointer on the current value. */
    void* val_ptr;

    /* Internal state. */
    strmap_t(void) _map;
    void* _b;
    size_t _bpos;
    size_t _kpos;
} strmap_iterator_t;

/*
 * Returns an iterator on the map.
 * The returned iterator is initialized to iterate on the map, but doesn't
 * points to any key/value pair yet. A call to strmap_next is required to set
 * the iterator on the first key/value pair.
 */
strmap_iterator_t strmap_iterator(strmap_t(void) map);

/*
 * Move the given iterator to the next key/value pair of the map, or to the
 * first key/value pair if the iterator was just initialized by strmap_iterator.
 * If the function returns 0, the iteration reached the end of the map and the
 * iterator state is undefined, otherwise the iterator is pointing to a
 * key/value pair of the map.
 */
int strmap_next(strmap_iterator_t* it);

#endif /* DELTA_STRMAP_H_ */
