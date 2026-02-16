/******************************************************************************
* djinterp [container]                                          min_enum_map.h
*
*   A min-enum-map (minimal enum map) is a lightweight associative container
* optimized to consume minimal space and code complexity.
*   This module only supports basic operations: put, get, remove, contains,
* and clear. The map is always maintained in sorted order by key, enabling
* O(log n) lookups via binary search.
*
*
* path:      \inc\container\map\min_enum_map.h 
* link:      TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_MIN_ENUM_MAP_
#define	DJINTERP_C_CONTAINER_MIN_ENUM_MAP_ 1

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "..\..\djinterp.h"
#include "..\container.h"
#include ".\enum_map_entry.h"


// D_MIN_ENUM_MAP_DEFAULT_CAPACITY
//   constant: default initial capacity for a new minimal enum map.
#ifndef D_MIN_ENUM_MAP_DEFAULT_CAPACITY
    #define D_MIN_ENUM_MAP_DEFAULT_CAPACITY 8
#endif

// d_min_enum_map
//   struct: a bare-bones associative container mapping integer keys
// to pointer values, optimized to consume minimal space.
struct d_min_enum_map
{
    struct d_enum_map_entry* entries;
    size_t                   count;
    size_t                   capacity;
};

// creation function
struct d_min_enum_map* d_min_enum_map_new(void);

// manipulation functions
void   d_min_enum_map_clear(struct d_min_enum_map* _map);
bool   d_min_enum_map_contains(const struct d_min_enum_map* _map, int _key);
size_t d_min_enum_map_count(const struct d_min_enum_map* _map);
void*  d_min_enum_map_get(const struct d_min_enum_map* _map, int _key);
bool   d_min_enum_map_merge(struct d_min_enum_map* _dst, const struct d_min_enum_map* _src, bool _overwrite); 
bool   d_min_enum_map_put(struct d_min_enum_map* _map, int _key, void* _value);
bool   d_min_enum_map_remove(struct d_min_enum_map* _map, int _key);

// destruction functions
void   d_min_enum_map_free(struct d_min_enum_map* _map);


#endif	// DJINTERP_C_CONTAINER_MIN_ENUM_MAP_