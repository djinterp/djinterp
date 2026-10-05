/*******************************************************************************
* djinterp [c]                                                    min_enum_map.h
*
*   A min-enum-map (minimal enum map) is a lightweight associative container
* optimized to consume minimal space and code complexity.
*   This module only supports basic operations: put, get, remove, contains,
* and clear. The map is always maintained in sorted order by key, enabling
* O(log n) lookups via binary search.
*
*
* path:      /inc/djinterp/c/container/map/min_enum_map.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.16
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_MAP_MIN_ENUM_MAP_H
#define DJINTERP_C_CONTAINER_MAP_MIN_ENUM_MAP_H 1

// std
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../container.h"
#include "./enum_map_entry.h"


// D_MIN_ENUM_MAP_DEFAULT_CAPACITY
//   constant: default initial capacity for a new minimal enum map.
#ifndef D_MIN_ENUM_MAP_DEFAULT_CAPACITY
    #define D_MIN_ENUM_MAP_DEFAULT_CAPACITY 8
#endif

// d_min_enum_map
//   struct: a bare-bones associative container mapping integer keys to pointer
// values, optimized to consume minimal space.
struct d_min_enum_map
{
    struct d_enum_map_entry* entries;
    size_t                   count;
    size_t                   capacity;
};

// creation function
// I.    creation and initialization
// I.    creation and initialization
struct d_min_enum_map* d_min_enum_map_new(void);
struct d_min_enum_map* d_min_enum_map_new_arr(
    struct d_enum_map_entry** _enum_map_entries,
    size_t                    _count);
struct d_min_enum_map* d_min_enum_map_new_copy(
    const struct d_min_enum_map* _source);

// II.   element access
void* d_min_enum_map_get(const struct d_min_enum_map* _map,
                         int                          _key);

// III.  insertion and removal
bool d_min_enum_map_put(struct d_min_enum_map* _map,
                        int                    _key,
                        void*                  _value);
bool d_min_enum_map_remove(struct d_min_enum_map* _map,
                           int                    _key);

// IV.   search
bool d_min_enum_map_contains(const struct d_min_enum_map* _map,
                             int                          _key);

// V.    capacity and state queries
size_t d_min_enum_map_count(const struct d_min_enum_map* _map);

// VI.   mutation and reordering
void d_min_enum_map_clear(struct d_min_enum_map* _map);
bool d_min_enum_map_merge(struct d_min_enum_map*       _destination,
                          const struct d_min_enum_map* _source,
                          bool                         _overwrite);

// VII.  destruction
void d_min_enum_map_free(struct d_min_enum_map* _map);

#endif  // DJINTERP_C_CONTAINER_MAP_MIN_ENUM_MAP_H
