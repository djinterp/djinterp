/*******************************************************************************
* djinterp [c]                                                    min_enum_map.c
*
*   Implementation of the dj_min_enum_map container - a minimal associative
* array mapping integer enum keys to arbitrary pointer values.
*
*
* path:      /src/djinterp/c/container/map/min_enum_map.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.16
*                                                            revised: 2026.11.04
*******************************************************************************/

#include "../../../../../inc/djinterp/c/container/map/min_enum_map.h"
#include "../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul


// =============================================================================
// internal helper functions
// =============================================================================

/*
d_internal_min_enum_map_find_index
  Finds the index of an entry with the given key using binary search.

Parameter(s):
  _map: pointer to the enum map
  _key: the key to search for
Return:
  The index of the entry if found, or -1 if not found.
*/
D_STATIC
ssize_t d_internal_min_enum_map_find_index(
    const struct dj_min_enum_map* _map,
    int                           _key
)
{
    ssize_t low;
    ssize_t high;
    ssize_t mid;
    int     mid_key;

    if ( (!_map)           ||
         (!_map->entries)  ||
         (_map->count == 0) )
    {
        return -1;
    }

    low  = 0;
    high = (ssize_t)_map->count - 1;

    while (low <= high)
    {
        mid     = low + (high - low) / 2;
        mid_key = _map->entries[mid].key;

        if (mid_key == _key)
        {
            return mid;
        }
        else if (mid_key < _key)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    return -1;
}

/*
d_internal_min_enum_map_find_insert_pos
  Finds the insertion position for a key to maintain sorted order.

Parameter(s):
  _map: pointer to the enum map
  _key: the key to insert
Return:
  The index where the key should be inserted.
*/
D_STATIC
size_t d_internal_min_enum_map_find_insert_pos(
    const struct dj_min_enum_map* _map,
    int                           _key
)
{
    size_t low;
    size_t high;
    size_t mid;

    if ( (!_map)           ||
         (!_map->entries)  ||
         (_map->count == 0) )
    {
        return 0;
    }

    low  = 0;
    high = _map->count;

    while (low < high)
    {
        mid = low + (high - low) / 2;

        if (_map->entries[mid].key < _key)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }

    return low;
}

/*
d_internal_min_enum_map_grow
  Doubles the capacity of the map's entries array.

Parameter(s):
  _map: pointer to the enum map
Return:
  true if successful, false if allocation failed.
*/
D_STATIC
bool d_internal_min_enum_map_grow(
    struct dj_min_enum_map* _map
)
{
    size_t                       new_capacity;
    struct dj_enum_map_entry* new_entries;

    if (!_map)
    {
        return false;
    }

    new_capacity = D_MIN_ENUM_MAP_DEFAULT_CAPACITY;

    size_t bytes = 0;

    // a doubled capacity or a size that wraps would shrink the entries
    if ( ( (_map->capacity != 0) &&
           (d_ckd_mul(&new_capacity, _map->capacity, 2)) ) ||
         (d_ckd_mul(&bytes,
                    new_capacity,
                    sizeof(struct dj_enum_map_entry))) )
    {
        return false;
    }

    new_entries = realloc(_map->entries, bytes);

    if (!new_entries)
    {
        return false;
    }

    _map->entries  = new_entries;
    _map->capacity = new_capacity;

    return true;
}


// =============================================================================
// public functions
// =============================================================================

/*
d_min_enum_map_new
  Allocates and initializes a new (empty) dj_min_enum_map.

Parameter(s):
  none
Return:
  Either a pointer to a new, empty dj_min_enum_map, or NULL if allocation failed.
*/
struct dj_min_enum_map* d_min_enum_map_new(void)
{
    struct dj_min_enum_map* new_min_enum_map =
        malloc(sizeof(struct dj_min_enum_map));

    // ensure that memory allocation was successful
    if (!new_min_enum_map)
    {
        return new_min_enum_map;
    }

    new_min_enum_map->entries  = NULL;
    new_min_enum_map->count    = 0;
    new_min_enum_map->capacity = 0;

    return new_min_enum_map;
}

/*
d_min_enum_map_new_arr
  A newly allocated `dj_min_enum_map` holding the given entries, or the result of
  the default creation function when the arguments are invalid; NULL if
  allocation failed.

Parameter(s):
  _enum_map_entries: array of entry pointers to seed the map with.
  _count:            the number of entries at `_enum_map_entries`.
Return:
  A newly allocated `dj_min_enum_map` holding the given entries, or the result of
  the default creation function when the arguments are invalid; NULL if
  allocation failed.
*/
struct dj_min_enum_map* d_min_enum_map_new_arr(
    struct dj_enum_map_entry** _enum_map_entries,
    size_t                     _count
)
{
    size_t i;
    struct dj_min_enum_map* new_min_enum_map;

    // ensure that parameters are valid; if not use default creation function
    if ( (!_enum_map_entries) ||
         (_count == 0) )
    {
        return d_min_enum_map_new();
    }

    new_min_enum_map = d_min_enum_map_new();

    // ensure that memory allocation of `struct dj_min_enum_map` was successful
    if (!new_min_enum_map)
    {
        return NULL;
    }

    // seed through put, which keeps the entries sorted by key, grows them
    // with checked sizes and lets a later duplicate key replace an earlier one
    for (i = 0; i < _count; ++i)
    {
        // a NULL slot in the source contributes nothing
        if (!_enum_map_entries[i])
        {
            continue;
        }

        if (!d_min_enum_map_put(new_min_enum_map,
                                _enum_map_entries[i]->key,
                                _enum_map_entries[i]->value))
        {
            d_min_enum_map_free(new_min_enum_map);

            return NULL;
        }
    }

    return new_min_enum_map;
}

/*
d_min_enum_map_new_copy
  A newly allocated `dj_min_enum_map` holding a copy of the source's entries, or
  NULL if `_source` was NULL, the bytes needed do not fit in a size_t, or
  allocation failed.

Parameter(s):
  _source: the map to copy; must not be NULL.
Return:
  A newly allocated `dj_min_enum_map` holding a copy of the source's entries, or
  NULL if `_source` was NULL or allocation failed.
*/
struct dj_min_enum_map* d_min_enum_map_new_copy(
    const struct dj_min_enum_map* _source
)
{
    size_t entries_size;
    struct dj_min_enum_map* new_min_enum_map;

    // cannot copy a `dj_min_enum_map` that points to NULL
    if (!_source)
    {
        return NULL;
    }

    // an empty source copies to an empty map, with nothing to allocate
    if (_source->count == 0)
    {
        return d_min_enum_map_new();
    }

    // a size that wraps means a corrupt count: the source's entries could not
    // exist
    if (d_ckd_mul(&entries_size,
                  sizeof(struct dj_enum_map_entry),
                  _source->count))
    {
        return NULL;
    }

    new_min_enum_map = malloc(sizeof(struct dj_min_enum_map));

    // ensure that memory allocation of `dj_min_enum_map` was successful
    if (!new_min_enum_map)
    {
        return NULL;
    }

    new_min_enum_map->entries = malloc(entries_size);

    //   ensure that memory allocation for `dj_enum_map_entry` array was
    // successful; d_memcpy_s returns 0 when it copies.
    if ( (!new_min_enum_map->entries) ||
         (d_memcpy_s(new_min_enum_map->entries,
                     entries_size,
                     _source->entries,
                     entries_size) != 0) )
    {
        free(new_min_enum_map->entries);
        free(new_min_enum_map);

        return NULL;
    }

    //   the capacity is what was allocated here, the source's count, not the
    // source's capacity: a put into spare room the copy does not have would
    // write past its entries
    new_min_enum_map->count    = _source->count;
    new_min_enum_map->capacity = _source->count;

    return new_min_enum_map;
}

/*
d_min_enum_map_clear
  Removes all entries from the map.
  After this operation, the map will be empty but still valid for use.

Parameter(s):
  _map: the map to be cleared
Return:
  none.
*/
void d_min_enum_map_clear(
    struct dj_min_enum_map* _map
)
{
    if (_map)
    {
        _map->count = 0;
    }

    return;
}

/*
d_min_enum_map_merge
  Merges all entries from _source into _destination.

  Both maps are assumed to be sorted by key (an invariant of this module).
  Duplicate keys are resolved by _overwrite:
    - if _overwrite == true  => _source wins
    - if _overwrite == false => _destination wins

  Note: this function only moves pointer values; it does NOT free any replaced
  values. If you need that, add a callback-based variant.

Parameter(s):
  _destination: destination map to be modified
  _source:      source map (not modified)
  _overwrite:   conflict policy when keys exist in both maps
Return:
  true if successful; false on invalid state, if the bytes needed do not
  fit in a size_t, or on allocation failure.
*/
bool d_min_enum_map_merge(
    struct dj_min_enum_map*       _destination,
    const struct dj_min_enum_map* _source,
    bool                          _overwrite
)
{
    struct dj_enum_map_entry* new_entries;
    size_t dst_i;
    size_t src_i;
    size_t out_i;
    size_t max_count;

    // if either of the following conditions is true: nothing to merge = done:
    // 1. both `_destination` and `_source` must point to different
    //    `dj_min_enum_map` addresses in memory,
    // 2. `_source` cannot NULL,
    // 3. `_source` cannot be non-NULL and empty.
    if ( (_destination == _source) ||
         (!_source)                ||
         (_source->count == 0) )
    {
        return true;
    }
    // in order to merge, all of the following must be true:
    // 1. `_destination` must be non-NULL,
    // 2. `_source` must have non-NULL `entries` if it has a non-zero count,
    // 3. `_destination` must have non-NULL `entries` if it has a non-zero
    //    count.
    else if ( (!_destination) ||
              ( (_source->count > 0) &&
                (!_source->entries) ) ||
              ( (_destination->count > 0) &&
                (!_destination->entries) ) )
    {
        return false;
    }

    size_t bytes = 0;

    // a count or a size that wraps would write past a short allocation
    if ( (d_ckd_add(&max_count, _destination->count, _source->count)) ||
         (d_ckd_mul(&bytes, max_count, sizeof(struct dj_enum_map_entry))) )
    {
        return false;
    }

    new_entries = malloc(bytes);

    if (!new_entries)
    {
        return false;
    }

    dst_i = 0;
    src_i = 0;
    out_i = 0;

    while ( (dst_i < _destination->count) &&
            (src_i < _source->count) )
    {
        const int dst_key = _destination->entries[dst_i].key;
        const int src_key = _source->entries[src_i].key;

        if (dst_key < src_key)
        {
            new_entries[out_i++] = _destination->entries[dst_i++];
        }
        else if (src_key < dst_key)
        {
            new_entries[out_i++] = _source->entries[src_i++];
        }
        else
        {
            // same key: pick winner based on overwrite policy
            new_entries[out_i].key = dst_key;
            new_entries[out_i].value = _overwrite
                ? _source->entries[src_i].value
                : _destination->entries[dst_i].value;

            out_i++;
            dst_i++;
            src_i++;
        }
    }

    // append remaining tails
    while (dst_i < _destination->count)
    {
        new_entries[out_i++] = _destination->entries[dst_i++];
    }

    // gives static analysis false positive in MVC
    while (src_i < _source->count)
    {
        new_entries[out_i++] = _source->entries[src_i++];
    }

    // commit (swap arrays)
    free(_destination->entries);

    _destination->entries  = new_entries;
    _destination->count    = out_i;
    _destination->capacity = max_count;

    return true;
}

/*
d_min_enum_map_put
  Inserts or updates a key-value pair in the map.
  If the key already exists, the value is updated.
  New entries are inserted in sorted order by key.

Parameter(s):
  _map:   pointer to the enum map
  _key:   the key to insert/update
  _value: the value to associate with the key
Return:
  true if successful, false otherwise.
*/
bool d_min_enum_map_put(
    struct dj_min_enum_map* _map,
    int                     _key,
    void*                   _value
)
{
    ssize_t index;
    size_t  insert_pos;

    if (!_map)
    {
        return false;
    }

    // check if key already exists
    index = d_internal_min_enum_map_find_index(_map, _key);

    if (index >= 0)
    {
        // update existing entry
        _map->entries[index].value = _value;

        return true;
    }

    // ensure capacity for new entry
    if (_map->count >= _map->capacity)
    {
        if (!d_internal_min_enum_map_grow(_map))
        {
            return false;
        }
    }

    // find insertion position to maintain sorted order
    insert_pos = d_internal_min_enum_map_find_insert_pos(_map, _key);

    // shift entries to make room
    if (insert_pos < _map->count)
    {
        memmove(
            &_map->entries[insert_pos + 1],
            &_map->entries[insert_pos],
            (_map->count - insert_pos) * sizeof(struct dj_enum_map_entry)
        );
    }

    _map->entries[insert_pos].key   = _key;
    _map->entries[insert_pos].value = _value;
    _map->count++;

    return true;
}

/*
d_min_enum_map_get
  Retrieves the value associated with a key.

Parameter(s):
  _map: pointer to the enum map
  _key: the key to look up
Return:
  The value associated with the key, or NULL if not found.
*/
void* d_min_enum_map_get(
    const struct dj_min_enum_map* _map,
    int                           _key
)
{
    ssize_t index;

    index = d_internal_min_enum_map_find_index(_map, _key);

    if (index < 0)
    {
        return NULL;
    }

    return _map->entries[index].value;
}

/*
d_min_enum_map_remove
  Removes an entry with the given key from the map.

Parameter(s):
  _map: pointer to the enum map
  _key: the key to remove
Return:
  true if the entry was removed, false if not found or error.
*/
bool d_min_enum_map_remove(
    struct dj_min_enum_map* _map,
    int                     _key
)
{
    ssize_t index;

    if (!_map)
    {
        return false;
    }

    index = d_internal_min_enum_map_find_index(_map, _key);

    if (index < 0)
    {
        return false;
    }

    // shift remaining entries to maintain sorted order
    if ((size_t)index < _map->count - 1)
    {
        memmove(
            &_map->entries[index],
            &_map->entries[index + 1],
            (_map->count - (size_t)index - 1) * sizeof(struct dj_enum_map_entry)
        );
    }

    _map->count--;

    return true;
}

/*
d_min_enum_map_contains
  Checks if the map contains an entry with the given key.

Parameter(s):
  _map: pointer to the enum map
  _key: the key to search for
Return:
  true if the key exists in the map, false otherwise.
*/
bool d_min_enum_map_contains(
    const struct dj_min_enum_map* _map,
    int                           _key
)
{
    return (d_internal_min_enum_map_find_index(_map, _key) >= 0);
}

/*
d_min_enum_map_count
  Returns the number of entries in the map.

Parameter(s):
  _map: pointer to the enum map
Return:
  The number of entries, or 0 if map is NULL.
*/
size_t d_min_enum_map_count(
    const struct dj_min_enum_map* _map
)
{
    return (_map) ? _map->count : 0;
}

/*
d_min_enum_map_free
  Frees the space allocated to the dj_min_enum_map and its entries array.

Parameter(s):
  _map: the dj_min_enum_map being freed
Return:
  none.
*/
void d_min_enum_map_free(
    struct dj_min_enum_map* _map
)
{
    if (_map)
    {
        free(_map->entries);
        free(_map);
    }

    return;
}
