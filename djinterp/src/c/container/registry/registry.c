#include "..\..\..\..\inc\c\container\registry\registry.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_registry_keycmp_case_sensitive
  Case-sensitive key comparison with NULL handling.

Parameter(s):
  _a: first key string; may be NULL.
  _b: second key string; may be NULL.
Return:
  Negative if _a < _b, 0 if equal, positive if _a > _b. NULL is ordered
  before any non-NULL string.
*/
D_STATIC int
d_registry_keycmp_case_sensitive
(
    const char* _a,
    const char* _b
)
{
    if (_a == _b)
    {
        return 0;
    }

    if (!_a)
    {
        return -1;
    }

    if (!_b)
    {
        return 1;
    }

    return strcmp(_a, _b);
}

/*
d_registry_keycmp_nocase
  Case-insensitive (ASCII) key comparison with NULL handling.

Parameter(s):
  _a: first key string; may be NULL.
  _b: second key string; may be NULL.
Return:
  Negative if _a < _b, 0 if equal, positive if _a > _b (ignoring case).
  NULL is ordered before any non-NULL string.
*/
D_STATIC int
d_registry_keycmp_nocase
(
    const char* _a,
    const char* _b
)
{
    unsigned char ca;
    unsigned char cb;

    if (_a == _b)
    {
        return 0;
    }

    if (!_a)
    {
        return -1;
    }

    if (!_b)
    {
        return 1;
    }

    while (*(_a) && 
           *(_b) )
    {
        ca = (unsigned char)*_a++;
        cb = (unsigned char)*_b++;

        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);

        if (ca != cb)
        {
            return (ca < cb) ? -1 : 1;
        }
    }

    if (*(_a) == *(_b))
    {
        return 0;
    }

    return (*_a)
        ? 1
        : -1;
}

/*
d_registry_is_frozen
  Checks whether the registry has the FROZEN flag set.

Parameter(s):
  registry: the registry to check; may be NULL.
Return:
  true if the registry is non-NULL and has the FROZEN flag set, false
  otherwise.
*/
D_STATIC_INLINE bool
d_registry_is_frozen
(
    const struct d_registry* registry
)
{
    return registry && ((registry->flags & D_REGISTRY_FLAG_FROZEN) != 0);
}

/*
d_registry_is_static
  Checks whether the registry has the STATIC_ROWS flag set.

Parameter(s):
  registry: the registry to check; may be NULL.
Return:
  true if the registry is non-NULL and has the STATIC_ROWS flag set, false
  otherwise.
*/
D_STATIC_INLINE bool
d_registry_is_static
(
    const struct d_registry* registry
)
{
    return registry && ((registry->flags & D_REGISTRY_FLAG_STATIC_ROWS) != 0);
}

/*
d_registry_lookup_entry_cmp_for_reg
  Compares two lookup entries using the comparison mode dictated by the
  registry's flags.

Parameter(s):
  _registry: the registry whose flags determine comparison mode; may be
             NULL (defaults to case-sensitive).
  _a:        first lookup entry.
  _b:        second lookup entry.
Return:
  Negative if _a < _b, 0 if equal, positive if _a > _b.
*/
D_STATIC_INLINE int
d_registry_lookup_entry_cmp_for_reg
(
    const struct d_registry*              _registry,
    const struct d_registry_lookup_entry* _a,
    const struct d_registry_lookup_entry* _b
)
{
    return ( (!_registry) ||
             ((_registry->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0) )
        ? d_registry_keycmp_case_sensitive(_a->key, _b->key)
        : d_registry_keycmp_nocase(_a->key, _b->key);
}

/*
d_registry_find_lookup_entry
  Binary searches the sorted lookup array for an entry matching _key.

Parameter(s):
  _registry: the registry to search; may be NULL.
  _key:      the key to search for; may be NULL.
Return:
  A pointer to the matching lookup entry, or NULL if the registry is
  invalid, the key is NULL, or no match is found.
*/
D_STATIC struct d_registry_lookup_entry*
d_registry_find_lookup_entry
(
    const struct d_registry* _registry,
    const char*              _key
)
{
    struct d_registry_lookup_entry probe;
    fn_comparator comparator = NULL;

    if ( (!_registry)         ||
         (!_key)              || 
         (!_registry->lookup) ||
         (!_registry->lookup_count) )
    {
        return NULL;
    }

    probe.key       = _key;
    probe.row_index = 0;

    comparator = ((_registry->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0)
                     ? d_registry_lookup_compare
                     : d_registry_lookup_compare_nocase;

    return (struct d_registry_lookup_entry*)bsearch(
        &probe,
        _registry->lookup,
        _registry->lookup_count,
        sizeof(struct d_registry_lookup_entry),
        comparator);
}

/*
d_registry_ensure_row_capacity
  Grows the row array to hold at least _needed rows, using the registry's
  growth factor.

Parameter(s):
  registry: the registry whose row array to grow; may be NULL.
  _needed:  the minimum required capacity.
Return:
  A boolean value corresponding to either:
  - true, if capacity was already sufficient or growth succeeded, or
  - false, if registry is NULL, overflow was detected, or realloc failed.
*/
D_STATIC bool
d_registry_ensure_row_capacity
(
    struct d_registry* registry,
    size_t             _needed
)
{
    void*  new_rows;
    size_t new_cap;

    if (!registry)
    {
        return false;
    }

    if (registry->capacity >= _needed)
    {
        return true;
    }

    new_cap = (registry->capacity == 0)
        ? D_REGISTRY_DEFAULT_CAPACITY
        : registry->capacity;

    while (new_cap < _needed)
    {
        new_cap *= D_REGISTRY_GROWTH_FACTOR;

        // overflow guard
        if (new_cap < registry->capacity)
        {
            return false;
        }
    }

    new_rows = realloc(registry->rows, new_cap * registry->row_size);

    if (!new_rows)
    {
        return false;
    }

    registry->rows     = new_rows;
    registry->capacity = new_cap;

    return true;
}

/*
d_registry_ensure_lookup_capacity
  Grows the lookup array to hold at least _needed entries, using the
  registry's growth factor.

Parameter(s):
  _registry: the registry whose lookup array to grow; may be NULL.
  _needed:   the minimum required lookup capacity.
Return:
  A boolean value corresponding to either:
  - true, if capacity was already sufficient or growth succeeded, or
  - false, if _registry is NULL, overflow was detected, or realloc failed.
*/
D_STATIC bool
d_registry_ensure_lookup_capacity
(
    struct d_registry* _registry,
    size_t             _needed
)
{
    struct d_registry_lookup_entry* new_lookup;
    size_t new_cap;

    if (!_registry)
    {
        return false;
    }

    if (_registry->lookup_capacity >= _needed)
    {
        return true;
    }

    new_cap = (_registry->lookup_capacity == 0)
        ? D_REGISTRY_DEFAULT_CAPACITY
        : _registry->lookup_capacity;

    while (new_cap < _needed)
    {
        new_cap *= D_REGISTRY_GROWTH_FACTOR;

        // overflow guard
        if (new_cap < _registry->lookup_capacity)
        {
            return false;
        }
    }

    new_lookup = realloc(_registry->lookup,
        (new_cap * sizeof(struct d_registry_lookup_entry)) );

    if (!new_lookup)
    {
        return false;
    }

    _registry->lookup          = new_lookup;
    _registry->lookup_capacity = new_cap;

    return true;
}

/*
d_registry_row_ptr
  Returns a pointer to the row at the given index in the row array.

Parameter(s):
  _registry: the registry containing the row array; may be NULL.
  _index:    the zero-based row index.
Return:
  A void pointer to the row, or NULL if the registry or its rows pointer
  is NULL.
*/
D_STATIC_INLINE void*
d_registry_row_ptr
(
    const struct d_registry* _registry,
    size_t                   _index
)
{
    return ( (_registry) &&
             (_registry->rows) )
        ? (void*)((char*)_registry->rows + (_index * _registry->row_size))
        : NULL;
}

/*
d_registry_lookup_entry_is_canonical
  Determines whether a lookup entry is a canonical key (i.e. its key
  string matches the key stored in the actual row) as opposed to an alias.

Parameter(s):
  registry: the registry containing the rows; may be NULL.
  _e:       the lookup entry to check; may be NULL.
Return:
  true if the entry's key matches the row's key (under the registry's
  comparison mode), false otherwise.
*/
D_STATIC bool
d_registry_lookup_entry_is_canonical
(
    const struct d_registry*              registry,
    const struct d_registry_lookup_entry* _e
)
{
    const char* row_key;
    int         eq;

    if ( (!registry) ||
         (!_e)       ||
         (_e->row_index >= registry->count) )
    {
        return false;
    }

    row_key = D_REGISTRY_ROW_KEY(
        d_registry_row_ptr(registry, _e->row_index));

    eq = ( (registry->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0)
            ? (d_registry_keycmp_case_sensitive(_e->key, row_key) == 0)
            : (d_registry_keycmp_nocase(_e->key, row_key) == 0);

    return eq;
}


/******************************************************************************
 * CONSTRUCTORS
 *****************************************************************************/

/*
d_registry_new
  Allocates a new registry with the default initial capacity.

Parameter(s):
  _row_size: the size in bytes of the user-defined row structure. Must be
             greater than zero.
Return:
  A pointer to the newly allocated registry, or NULL if _row_size is zero
  or allocation fails.
*/
struct d_registry*
d_registry_new
(
    size_t _row_size
)
{
    return d_registry_new_with_capacity(_row_size,
                                        D_REGISTRY_DEFAULT_CAPACITY);
}

/*
d_registry_new_with_capacity
  Allocates a new registry with the specified initial capacity for both
  the row array and the lookup array.

Parameter(s):
  _row_size: the size in bytes of the user-defined row structure. Must be
             greater than zero.
  _capacity: the initial number of rows to pre-allocate. Zero is valid and
             results in an empty registry with no pre-allocated storage.
Return:
  A pointer to the newly allocated registry, or NULL if _row_size is zero
  or allocation fails.
*/
struct d_registry*
d_registry_new_with_capacity
(
    size_t _row_size,
    size_t _capacity
)
{
    struct d_registry* new_registry;

    if (_row_size == 0)
    {
        return NULL;
    }

    new_registry = malloc(sizeof(struct d_registry));

    if (!new_registry)
    {
        return NULL;
    }

    // zero-initialize all fields before setting known values; this
    // ensures rows/lookup are NULL (safe for realloc and free) and
    // count/capacity fields are 0 on all paths including early errors.
    memset(new_registry, 0, sizeof(struct d_registry));

    new_registry->row_size = _row_size;
    new_registry->flags    = (uint8_t)D_REGISTRY_FLAG_DEFAULT;

    if (_capacity > 0)
    {
        // pre-allocate row storage
        if (!d_registry_reserve(new_registry, _capacity))
        {
            d_registry_free(new_registry);

            return NULL;
        }

        // pre-allocate lookup storage
        if (!d_registry_reserve_lookup(new_registry, _capacity))
        {
            d_registry_free(new_registry);

            return NULL;
        }
    }

    return new_registry;
}

/*
d_registry_new_copy
  Creates a deep copy of an existing registry. The STATIC_ROWS flag is
  cleared on the copy; all other flags are preserved. Row data and lookup
  entries are duplicated.

Parameter(s):
  _other: the source registry to copy; may be NULL.
Return:
  A pointer to the newly allocated copy, or NULL if _other is NULL or
  allocation fails.
*/
struct d_registry*
d_registry_new_copy
(
    const struct d_registry* _other
)
{
    struct d_registry* new_registry;

    if (!_other)
    {
        return NULL;
    }

    new_registry = d_registry_new_with_capacity(_other->row_size,
                                                _other->count);
    if (!new_registry)
    {
        return NULL;
    }

    new_registry->flags    = (uint8_t)(_other->flags
                                       & ~D_REGISTRY_FLAG_STATIC_ROWS);
    new_registry->row_free = _other->row_free;

    // copy row data
    if (_other->count > 0 && _other->rows)
    {
        memcpy(new_registry->rows,
               _other->rows,
               (_other->count * _other->row_size) );

        new_registry->count = _other->count;
    }

    // copy lookup entries (includes aliases)
    if ( (_other->lookup_count > 0) &&
         (_other->lookup) )
    {
        if (!d_registry_reserve_lookup(new_registry,
                                       _other->lookup_count))
        {
            d_registry_free(new_registry);

            return NULL;
        }

        memcpy(new_registry->lookup,
               _other->lookup,
               _other->lookup_count
                   * sizeof(struct d_registry_lookup_entry));

        new_registry->lookup_count = _other->lookup_count;
    }

    return new_registry;
}

/*
d_registry_new_from_array
  Creates a new registry pre-populated with rows copied from a raw array.
  The lookup table is automatically rebuilt and sorted after population.

Parameter(s):
  _rows:     pointer to the first element of a contiguous array of user-
             defined row structures. Must not be NULL.
  _row_size: the size in bytes of each row structure.
  _count:    the number of rows in the array.
Return:
  A pointer to the newly allocated and populated registry, or NULL if
  _rows is NULL, _row_size is zero, or allocation fails.
*/
struct d_registry*
d_registry_new_from_array
(
    const void* _rows,
    size_t      _row_size,
    size_t      _count
)
{
    struct d_registry* new_registry;

    if ( (!_rows) || 
         (_row_size == 0) )
    {
        return NULL;
    }

    new_registry = d_registry_new_with_capacity(_row_size, _count);

    if (!new_registry)
    {
        return NULL;
    }

    if (_count > 0)
    {
        memcpy(new_registry->rows,
               _rows, 
               (_count * _row_size) );

        new_registry->count = _count;

        d_registry_rebuild_lookup(new_registry);
    }

    return new_registry;
}


/******************************************************************************
 * LOOKUP / COMPARATORS
 *****************************************************************************/

/*
d_registry_lookup_compare
  qsort/bsearch comparator for lookup entries using case-sensitive key
  comparison.

Parameter(s):
  _a: pointer to the first d_registry_lookup_entry.
  _b: pointer to the second d_registry_lookup_entry.
Return:
  Negative if _a < _b, 0 if equal, positive if _a > _b.
*/
D_INLINE int
d_registry_lookup_compare
(
    const void* _a,
    const void* _b
)
{
    const struct d_registry_lookup_entry* a =
        (const struct d_registry_lookup_entry*)_a;
    const struct d_registry_lookup_entry* b =
        (const struct d_registry_lookup_entry*)_b;

    return d_registry_keycmp_case_sensitive(a->key, b->key);
}

/*
d_registry_lookup_compare_nocase
  qsort/bsearch comparator for lookup entries using case-insensitive
  (ASCII) key comparison.

Parameter(s):
  _a: pointer to the first d_registry_lookup_entry.
  _b: pointer to the second d_registry_lookup_entry.
Return:
  Negative if _a < _b, 0 if equal, positive if _a > _b (ignoring case).
*/
D_INLINE int
d_registry_lookup_compare_nocase
(
    const void* _a,
    const void* _b
)
{
    const struct d_registry_lookup_entry* a =
        (const struct d_registry_lookup_entry*)_a;
    const struct d_registry_lookup_entry* b =
        (const struct d_registry_lookup_entry*)_b;

    return d_registry_keycmp_nocase(a->key, b->key);
}

/*
d_registry_sort_lookup
  Sorts the lookup array in ascending key order using qsort. The
  comparison function is selected based on whether the CASE_INSENSITIVE
  flag is set.

Parameter(s):
  _registry: the registry whose lookup array to sort; may be NULL.
Return:
  none.
*/
void
d_registry_sort_lookup
(
    struct d_registry* _registry
)
{
    if ( (!_registry)         ||
         (!_registry->lookup) || 
         (_registry->lookup_count <= 1) )
    {
        return;
    }

    qsort(_registry->lookup,
          _registry->lookup_count,
          sizeof(struct d_registry_lookup_entry),
          ((_registry->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0)
              ? d_registry_lookup_compare
              : d_registry_lookup_compare_nocase);

    return;
}

/*
d_registry_rebuild_lookup
  Clears and rebuilds the lookup table from all row keys. Aliases are
  dropped; only canonical keys are re-added. The lookup is sorted after
  rebuilding.

Parameter(s):
  _registry: the registry whose lookup to rebuild; may be NULL.
Return:
  none.
*/
void
d_registry_rebuild_lookup
(
    struct d_registry* _registry
)
{
    size_t i;

    if (!_registry)
    {
        return;
    }

    // rebuilds from row keys only (aliases are dropped)
    _registry->lookup_count = 0;

    // ensure capacity for all row keys
    if ( (_registry->count == 0) ||
         (!d_registry_ensure_lookup_capacity(_registry,
                                             _registry->count)) )
    {
        return;
    }

    // populate lookup entries from row keys
    for (i = 0; i < _registry->count; ++i)
    {
        void* row = d_registry_row_ptr(_registry, i);

        _registry->lookup[i].key       = D_REGISTRY_ROW_KEY(row);
        _registry->lookup[i].row_index = i;
    }

    _registry->lookup_count = _registry->count;

    d_registry_sort_lookup(_registry);

    return;
}


/******************************************************************************
 * PRIMARY LOOKUP
 *****************************************************************************/

/*
d_registry_get
  Binary searches the lookup array for the given key and returns a pointer
  to the matching row. This is the primary access method.

Parameter(s):
  _registry: the registry to search; may be NULL.
  _key:      the key (or alias) to look up; may be NULL.
Return:
  A void pointer to the matching row, or NULL if _registry is NULL, _key
  is NULL, or no matching entry is found.
*/
void*
d_registry_get
(
    const struct d_registry* _registry,
    const char*              _key
)
{
    struct d_registry_lookup_entry* e;

    if ( (!_registry) || 
         (!_key) )
    {
        return NULL;
    }

    e = d_registry_find_lookup_entry(_registry, _key);

    if ( (!e) ||
         (e->row_index >= _registry->count) )
    {
        return NULL;
    }

    return d_registry_row_ptr(_registry, e->row_index);
}


/******************************************************************************
 * ROW MANIPULATION
 *****************************************************************************/

/*
d_registry_add
  Adds a new row to the registry. The row is copied into the internal
  array and a canonical lookup entry is created for its key. Duplicate
  keys are rejected.

Parameter(s):
  _registry: the registry to add the row to; may be NULL.
  _row:      pointer to the user-defined row to copy; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the row was successfully added, or
  - false, if any of the following were true:
    - _registry or _row is NULL,
    - the registry is frozen or static,
    - the row's key is NULL or already exists,
    - memory allocation failed.
*/
bool
d_registry_add
(
    struct d_registry* _registry,
    const void*        _row
)
{
    const char* key;
    const char* row_key;
    size_t      insert_at;
    size_t      i;

    if ( (!_registry) || 
         (!_row) )
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) ||
        d_registry_is_static(_registry))
    {
        return false;
    }

    key = D_REGISTRY_ROW_KEY(_row);

    if (!key)
    {
        return false;
    }

    // reject duplicate keys
    if (d_registry_contains(_registry, key))
    {
        return false;
    }

    insert_at = _registry->count;

    // optional sorted insertion by key
    if ( (_registry->flags & D_REGISTRY_FLAG_SORTED) != 0)
    {
        for (i = 0; i < _registry->count; ++i)
        {
            row_key = D_REGISTRY_ROW_KEY(
                d_registry_row_ptr(_registry, i));

            int cmp = ((_registry->flags
                        & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0)
                ? d_registry_keycmp_case_sensitive(key, row_key)
                : d_registry_keycmp_nocase(key, row_key);

            if (cmp < 0)
            {
                insert_at = i;
                break;
            }
        }
    }

    // ensure row and lookup capacity
    if (!d_registry_ensure_row_capacity(_registry,
                                        _registry->count + 1))
    {
        return false;
    }

    if (!d_registry_ensure_lookup_capacity(_registry,
                                           _registry->lookup_count + 1))
    {
        return false;
    }

    // make space in rows if inserting in the middle
    if (insert_at < _registry->count)
    {
        memmove(
            (char*)_registry->rows
                + ((insert_at + 1) * _registry->row_size),
            (char*)_registry->rows
                + (insert_at * _registry->row_size),
            (_registry->count - insert_at) * _registry->row_size
        );

        // shift lookup indices that point at or past the insertion
        for (i = 0; i < _registry->lookup_count; ++i)
        {
            if (_registry->lookup[i].row_index >= insert_at)
            {
                _registry->lookup[i].row_index += 1;
            }
        }
    }

    // copy the row into the array
    memcpy(d_registry_row_ptr(_registry, insert_at),
           _row,
           _registry->row_size);

    _registry->count += 1;

    // add canonical lookup entry
    _registry->lookup[_registry->lookup_count].key =
        D_REGISTRY_ROW_KEY(
            d_registry_row_ptr(_registry, insert_at));
    _registry->lookup[_registry->lookup_count].row_index = insert_at;
    _registry->lookup_count += 1;

    d_registry_sort_lookup(_registry);

    return true;
}

/*
d_registry_add_rows
  Adds multiple rows to the registry in sequence. Each row is added
  individually via d_registry_add. If any row fails to add (e.g. due
  to a duplicate key), the function returns false; partial adds may have
  occurred.

Parameter(s):
  _registry: the registry to add rows to; may be NULL.
  _rows:     pointer to a contiguous array of row structures; may be NULL.
  _count:    the number of rows to add.
Return:
  A boolean value corresponding to either:
  - true, if all rows were successfully added, or
  - false, if any row failed to add, or parameters are invalid.
*/
bool
d_registry_add_rows
(
    struct d_registry* _registry,
    const void*        _rows,
    size_t             _count
)
{
    size_t i;

    if ( (!_registry) ||
         (!_rows)     ||
         (_count == 0) )
    {
        return false;
    }

    for (i = 0; i < _count; ++i)
    {
        const void* row = (const void*)(
            (const char*)_rows + (i * _registry->row_size));

        // partial adds may have occurred on failure
        if (!d_registry_add(_registry, row))
        {
            return false;
        }
    }

    return true;
}

/*
d_registry_set
  Overwrites the data of an existing row identified by key. The canonical
  key stored in the row is preserved; only non-key fields are updated.

Parameter(s):
  _registry: the registry containing the row; may be NULL.
  _key:      the key identifying the row to update; may be NULL.
  _row:      pointer to the replacement row data; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the row was found and updated, or
  - false, if any parameter is NULL, the registry is frozen/static, or
    the key was not found.
*/
bool
d_registry_set
(
    struct d_registry* _registry,
    const char*        _key,
    const void*        _row
)
{
    struct d_registry_lookup_entry* e;
    void*                          dst;
    const char*                    existing_key;

    if ( (!_registry) ||
         (!_key)      ||
         (!_row) )
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) ||
        d_registry_is_static(_registry))
    {
        return false;
    }

    e = d_registry_find_lookup_entry(_registry, _key);

    if ( (!e) ||
         (e->row_index >= _registry->count) )
    {
        return false;
    }

    dst = d_registry_row_ptr(_registry, e->row_index);

    // preserve the canonical key (don't allow key changes via set)
    existing_key = D_REGISTRY_ROW_KEY(dst);

    memcpy(dst, _row, _registry->row_size);
    *((const char**)dst) = existing_key;

    return true;
}

/*
d_registry_remove_at
  Removes the row at the specified index. Row destructor is called if
  the OWNS_ROWS flag is set. Remaining rows are shifted down and lookup
  indices are adjusted accordingly.

Parameter(s):
  _registry: the registry to remove from; may be NULL.
  _index:    the zero-based index of the row to remove.
Return:
  A boolean value corresponding to either:
  - true, if the row was successfully removed, or
  - false, if _registry is NULL, the registry is frozen/static, or
    _index is out of range.
*/
bool
d_registry_remove_at
(
    struct d_registry* _registry,
    size_t             _index
)
{
    size_t i;
    size_t w;

    if (!_registry)
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) ||
        d_registry_is_static(_registry))
    {
        return false;
    }

    if (_index >= _registry->count)
    {
        return false;
    }

    // call row destructor if OWNS_ROWS is set
    if ( ((_registry->flags & D_REGISTRY_FLAG_OWNS_ROWS) != 0) &&
         (_registry->row_free) )
    {
        _registry->row_free(d_registry_row_ptr(_registry, _index));
    }

    // shift rows down
    if (_index + 1 < _registry->count)
    {
        memmove(
            (char*)_registry->rows
                + (_index * _registry->row_size),
            (char*)_registry->rows
                + ((_index + 1) * _registry->row_size),
            (_registry->count - (_index + 1)) * _registry->row_size
        );
    }

    _registry->count -= 1;

    // remove lookup entries pointing to this row, shift indices above
    w = 0;

    for (i = 0; i < _registry->lookup_count; ++i)
    {
        struct d_registry_lookup_entry e = _registry->lookup[i];

        // drop canonical + alias entries for the removed row
        if (e.row_index == _index)
        {
            continue;
        }

        // adjust indices for rows that shifted down
        if (e.row_index > _index)
        {
            e.row_index -= 1;
        }

        _registry->lookup[w++] = e;
    }

    _registry->lookup_count = w;

    d_registry_sort_lookup(_registry);

    return true;
}

/*
d_registry_remove
  Removes the row identified by key. Locates the row via lookup and
  delegates to d_registry_remove_at.

Parameter(s):
  _registry: the registry to remove from; may be NULL.
  _key:      the key (or alias) identifying the row; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the row was found and removed, or
  - false, if _registry or _key is NULL, or the key was not found.
*/
bool
d_registry_remove
(
    struct d_registry* _registry,
    const char*        _key
)
{
    struct d_registry_lookup_entry* e;

    if ( (!_registry) ||
         (!_key) )
    {
        return false;
    }

    e = d_registry_find_lookup_entry(_registry, _key);

    if (!e)
    {
        return false;
    }

    return d_registry_remove_at(_registry, e->row_index);
}

/*
d_registry_clear
  Removes all rows and lookup entries from the registry without freeing
  the underlying arrays. Capacity is preserved for reuse. If the
  OWNS_ROWS flag is set, row_free is called for each row before clearing.

Parameter(s):
  _registry: the registry to clear; may be NULL.
Return:
  none.
*/
void
d_registry_clear
(
    struct d_registry* _registry
)
{
    size_t i;

    if (!_registry)
    {
        return;
    }

    // static registry tables are immutable; just drop the lookup view
    if (d_registry_is_static(_registry))
    {
        _registry->lookup_count = 0;

        return;
    }

    // call row destructors if OWNS_ROWS is set
    if ( ((_registry->flags & D_REGISTRY_FLAG_OWNS_ROWS) != 0) &&
         (_registry->row_free) )
    {
        for (i = 0; i < _registry->count; ++i)
        {
            _registry->row_free(d_registry_row_ptr(_registry, i));
        }
    }

    _registry->count        = 0;
    _registry->lookup_count = 0;

    return;
}


/******************************************************************************
 * ALIASES
 *****************************************************************************/

/*
d_registry_add_alias
  Adds an alias for an existing row key. The alias is a new lookup entry
  pointing to the same row index as the canonical key. The alias must not
  already exist as a key or alias.

Parameter(s):
  _registry: the registry to modify; may be NULL.
  _key:      the existing canonical key to alias; may be NULL.
  _alias:    the alias string to add; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the alias was successfully added, or
  - false, if any parameter is NULL, the registry is frozen, the key was
    not found, the alias already exists, or allocation failed.
*/
bool
d_registry_add_alias
(
    struct d_registry* _registry,
    const char*        _key,
    const char*        _alias
)
{
    struct d_registry_lookup_entry* e;
    size_t row_index;

    if ( (!_registry) ||
         (!_key)      ||
         (!_alias) )
    {
        return false;
    }

    // frozen registries reject all modifications
    if (d_registry_is_frozen(_registry))
    {
        return false;
    }

    // find the canonical key
    e = d_registry_find_lookup_entry(_registry, _key);

    if (!e)
    {
        return false;
    }

    row_index = e->row_index;

    // reject if alias string already in use
    if (d_registry_contains(_registry, _alias))
    {
        return false;
    }

    // ensure lookup capacity for the new entry
    if (!d_registry_ensure_lookup_capacity(_registry,
                                           _registry->lookup_count + 1))
    {
        return false;
    }

    // insert the alias entry
    _registry->lookup[_registry->lookup_count].key       = _alias;
    _registry->lookup[_registry->lookup_count].row_index = row_index;
    _registry->lookup_count += 1;

    d_registry_sort_lookup(_registry);

    return true;
}

/*
d_registry_add_aliases
  Adds multiple aliases for a single key. Each alias is added individually
  via d_registry_add_alias. If any alias fails, the function returns
  false; partial adds may have occurred.

Parameter(s):
  _registry: the registry to modify; may be NULL.
  _key:      the canonical key to alias; may be NULL.
  _aliases:  array of alias strings; may be NULL.
  _count:    the number of aliases in the array.
Return:
  A boolean value corresponding to either:
  - true, if all aliases were successfully added, or
  - false, if any alias failed to add, or parameters are invalid.
*/
bool
d_registry_add_aliases
(
    struct d_registry* _registry,
    const char*        _key,
    const char**       _aliases,
    size_t             _count
)
{
    size_t i;

    if ( (!_registry) ||
         (!_key)      ||
         (!_aliases)  ||
         (_count == 0) )
    {
        return false;
    }

    for (i = 0; i < _count; ++i)
    {
        if (!d_registry_add_alias(_registry, _key, _aliases[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_registry_remove_alias
  Removes an alias entry from the lookup table. Canonical keys cannot be
  removed via this function; use d_registry_remove instead.

Parameter(s):
  _registry: the registry to modify; may be NULL.
  _alias:    the alias string to remove; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the alias was found and removed, or
  - false, if any parameter is NULL, the registry is frozen, the alias
    was not found, or the alias is actually a canonical key.
*/
bool
d_registry_remove_alias
(
    struct d_registry* _registry,
    const char*        _alias
)
{
    struct d_registry_lookup_entry* e;
    size_t i;
    size_t idx;

    if ( (!_registry) ||
         (!_alias) )
    {
        return false;
    }

    if (d_registry_is_frozen(_registry))
    {
        return false;
    }

    e = d_registry_find_lookup_entry(_registry, _alias);

    if (!e)
    {
        return false;
    }

    // don't remove canonical keys via remove_alias
    if (d_registry_lookup_entry_is_canonical(_registry, e))
    {
        return false;
    }

    // shift remaining entries down
    idx = (size_t)(e - _registry->lookup);

    for (i = idx + 1; i < _registry->lookup_count; ++i)
    {
        _registry->lookup[i - 1] = _registry->lookup[i];
    }

    _registry->lookup_count -= 1;

    return true;
}

/*
d_registry_clear_aliases
  Removes all alias entries from the lookup table, keeping only canonical
  key entries. The lookup is re-sorted after pruning.

Parameter(s):
  _registry: the registry to modify; may be NULL.
Return:
  none.
*/
void
d_registry_clear_aliases
(
    struct d_registry* _registry
)
{
    size_t i;
    size_t w;

    if (!_registry)
    {
        return;
    }

    if (d_registry_is_frozen(_registry))
    {
        return;
    }

    // compact: keep only canonical entries
    w = 0;

    for (i = 0; i < _registry->lookup_count; ++i)
    {
        if (d_registry_lookup_entry_is_canonical(_registry,
                                                  &_registry->lookup[i]))
        {
            _registry->lookup[w++] = _registry->lookup[i];
        }
    }

    _registry->lookup_count = w;

    d_registry_sort_lookup(_registry);

    return;
}

/*
d_registry_alias_count
  Counts the number of alias (non-canonical) entries in the lookup table.

Parameter(s):
  _registry: the registry to query; may be NULL.
Return:
  The number of alias entries, or 0 if _registry is NULL.
*/
size_t
d_registry_alias_count
(
    const struct d_registry* _registry
)
{
    size_t i;
    size_t n;

    n = 0;

    if (!_registry)
    {
        return 0;
    }

    for (i = 0; i < _registry->lookup_count; ++i)
    {
        if (!d_registry_lookup_entry_is_canonical(_registry,
                                                   &_registry->lookup[i]))
        {
            n += 1;
        }
    }

    return n;
}


/******************************************************************************
 * QUERIES
 *****************************************************************************/

/*
d_registry_contains
  Checks whether a key or alias exists in the registry.

Parameter(s):
  _registry: the registry to search; may be NULL.
  _key:      the key or alias to look up; may be NULL.
Return:
  true if the key is found, false otherwise.
*/
bool
d_registry_contains
(
    const struct d_registry* _registry,
    const char*              _key
)
{
    return (d_registry_index_of(_registry, _key) >= 0);
}

/*
d_registry_index_of
  Returns the row index associated with a key or alias.

Parameter(s):
  _registry: the registry to search; may be NULL.
  _key:      the key or alias to look up; may be NULL.
Return:
  The zero-based row index if found, or (ssize_t)-1 if the registry is
  NULL, the key is NULL, or the key was not found.
*/
ssize_t
d_registry_index_of
(
    const struct d_registry* _registry,
    const char*              _key
)
{
    struct d_registry_lookup_entry* e;

    if ( (!_registry) ||
         (!_key) )
    {
        return (ssize_t)-1;
    }

    e = d_registry_find_lookup_entry(_registry, _key);

    if (!e)
    {
        return (ssize_t)-1;
    }

    return (ssize_t)e->row_index;
}

/*
d_registry_at
  Returns a pointer to the row at the specified index.

Parameter(s):
  _registry: the registry to access; may be NULL.
  _index:    the zero-based row index.
Return:
  A void pointer to the row, or NULL if _registry is NULL or _index is
  out of range.
*/
void*
d_registry_at
(
    const struct d_registry* _registry,
    size_t                   _index
)
{
    if ( (!_registry) ||
         (_index >= _registry->count) )
    {
        return NULL;
    }

    return d_registry_row_ptr(_registry, _index);
}

/*
d_registry_count
  Returns the number of rows currently stored in the registry.

Parameter(s):
  _registry: the registry to query; may be NULL.
Return:
  The row count, or 0 if _registry is NULL.
*/
size_t
d_registry_count
(
    const struct d_registry* _registry
)
{
    return _registry ? _registry->count : 0;
}

/*
d_registry_capacity
  Returns the current allocated row capacity of the registry.

Parameter(s):
  _registry: the registry to query; may be NULL.
Return:
  The row capacity, or 0 if _registry is NULL.
*/
size_t
d_registry_capacity
(
    const struct d_registry* _registry
)
{
    return _registry ? _registry->capacity : 0;
}

/*
d_registry_is_empty
  Checks whether the registry contains zero rows.

Parameter(s):
  _registry: the registry to query; may be NULL.
Return:
  true if _registry is NULL or has zero rows, false otherwise.
*/
bool
d_registry_is_empty
(
    const struct d_registry* _registry
)
{
    return !_registry || _registry->count == 0;
}


/******************************************************************************
 * ITERATORS
 *****************************************************************************/

/*
d_registry_iterator_new
  Creates a new unfiltered iterator starting at row 0.

Parameter(s):
  _registry: the registry to iterate over; may be NULL.
Return:
  A d_registry_iterator value with current set to 0 and no filter.
*/
struct d_registry_iterator
d_registry_iterator_new
(
    struct d_registry* _registry
)
{
    struct d_registry_iterator it;

    it.registry   = _registry;
    it.current    = 0;
    it.filter     = NULL;
    it.filter_ctx = NULL;

    return it;
}

/*
d_registry_iterator_filtered
  Creates a new filtered iterator starting at row 0. Only rows that pass
  the predicate function will be yielded by iterator_next.

Parameter(s):
  _registry: the registry to iterate over; may be NULL.
  _filter:   predicate function that must return true for a row to be
             yielded; may be NULL (equivalent to no filter).
  _context:  opaque context pointer passed to _filter; may be NULL.
Return:
  A d_registry_iterator value with the filter and context stored.
*/
struct d_registry_iterator
d_registry_iterator_filtered
(
    struct d_registry*        _registry,
    fn_registry_row_predicate _filter,
    const void*               _context
)
{
    struct d_registry_iterator it;

    it.registry   = _registry;
    it.current    = 0;
    it.filter     = _filter;
    it.filter_ctx = _context;

    return it;
}

/*
d_registry_iterator_has_next
  Checks whether the iterator has any remaining rows to yield. For
  filtered iterators, this scans ahead to find the next matching row.

Parameter(s):
  _iter: the iterator to check; may be NULL.
Return:
  true if at least one more row can be yielded, false otherwise.
*/
bool
d_registry_iterator_has_next
(
    const struct d_registry_iterator* _iter
)
{
    size_t i;

    if ( (!_iter) ||
         (!_iter->registry) )
    {
        return false;
    }

    // unfiltered: simple index check
    if (!_iter->filter)
    {
        return _iter->current < _iter->registry->count;
    }

    // filtered: scan ahead for a matching row
    for (i = _iter->current; i < _iter->registry->count; ++i)
    {
        void* row = d_registry_row_ptr(_iter->registry, i);

        if (_iter->filter(row, _iter->filter_ctx))
        {
            return true;
        }
    }

    return false;
}

/*
d_registry_iterator_next
  Advances the iterator and returns the next row. For filtered iterators,
  non-matching rows are skipped automatically.

Parameter(s):
  _iter: the iterator to advance; may be NULL.
Return:
  A void pointer to the next row, or NULL if the iterator is exhausted
  or invalid.
*/
void*
d_registry_iterator_next
(
    struct d_registry_iterator* _iter
)
{
    if ( (!_iter) ||
         (!_iter->registry) )
    {
        return NULL;
    }

    while (_iter->current < _iter->registry->count)
    {
        void* row = d_registry_row_ptr(_iter->registry,
                                        _iter->current);
        _iter->current += 1;

        if ( (!_iter->filter) ||
             (_iter->filter(row, _iter->filter_ctx)) )
        {
            return row;
        }
    }

    return NULL;
}

/*
d_registry_iterator_reset
  Resets the iterator's current position back to 0, allowing re-iteration
  from the beginning. The filter and context are preserved.

Parameter(s):
  _iter: the iterator to reset; may be NULL.
Return:
  none.
*/
void
d_registry_iterator_reset
(
    struct d_registry_iterator* _iter
)
{
    if (_iter)
    {
        _iter->current = 0;
    }

    return;
}

/*
d_registry_foreach
  Visits every row in the registry by calling the visitor function. The
  visitor may return false to stop iteration early.

Parameter(s):
  _registry: the registry to iterate; may be NULL.
  _visitor:  callback invoked for each row; may be NULL.
  _context:  opaque context pointer passed to _visitor; may be NULL.
Return:
  none.
*/
void
d_registry_foreach
(
    struct d_registry*      _registry,
    fn_registry_row_visitor _visitor,
    void*                   _context
)
{
    size_t i;

    if ( (!_registry) ||
         (!_visitor) )
    {
        return;
    }

    for (i = 0; i < _registry->count; ++i)
    {
        void* row = d_registry_row_ptr(_registry, i);

        if (!_visitor(row, _context))
        {
            break;
        }
    }

    return;
}

/*
d_registry_foreach_if
  Visits rows that match a predicate by calling the visitor function. If
  the predicate is NULL, all rows are visited. The visitor may return
  false to stop iteration early.

Parameter(s):
  _registry:  the registry to iterate; may be NULL.
  _predicate: filter function; rows that return false are skipped. May
              be NULL to visit all rows.
  _pred_ctx:  opaque context for the predicate; may be NULL.
  _visitor:   callback invoked for each matching row; may be NULL.
  _visit_ctx: opaque context for the visitor; may be NULL.
Return:
  none.
*/
void
d_registry_foreach_if
(
    struct d_registry*        _registry,
    fn_registry_row_predicate _predicate,
    const void*               _pred_ctx,
    fn_registry_row_visitor   _visitor,
    void*                     _visit_ctx
)
{
    size_t i;

    if ( (!_registry) ||
         (!_visitor) )
    {
        return;
    }

    for (i = 0; i < _registry->count; ++i)
    {
        void* row = d_registry_row_ptr(_registry, i);

        // skip rows that don't match the predicate
        if ( (_predicate) &&
             (!_predicate(row, _pred_ctx)) )
        {
            continue;
        }

        // stop early if visitor returns false
        if (!_visitor(row, _visit_ctx))
        {
            break;
        }
    }

    return;
}


/******************************************************************************
 * UTILITIES
 *****************************************************************************/

/*
d_registry_reserve
  Ensures the row array has at least _capacity slots. Does not shrink.

Parameter(s):
  _registry: the registry to reserve capacity for; may be NULL.
  _capacity: the minimum required row capacity.
Return:
  A boolean value corresponding to either:
  - true, if capacity was already sufficient or growth succeeded, or
  - false, if _registry is NULL, the registry is frozen/static, or
    allocation failed.
*/
bool
d_registry_reserve
(
    struct d_registry* _registry,
    size_t             _capacity
)
{
    if (!_registry)
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) ||
        d_registry_is_static(_registry))
    {
        return false;
    }

    if (_capacity <= _registry->capacity)
    {
        return true;
    }

    return d_registry_ensure_row_capacity(_registry, _capacity);
}

/*
d_registry_reserve_lookup
  Ensures the lookup array has at least _capacity slots. Does not shrink.

Parameter(s):
  _registry: the registry to reserve lookup capacity for; may be NULL.
  _capacity: the minimum required lookup capacity.
Return:
  A boolean value corresponding to either:
  - true, if capacity was already sufficient or growth succeeded, or
  - false, if _registry is NULL, the registry is frozen/static, or
    allocation failed.
*/
bool
d_registry_reserve_lookup
(
    struct d_registry* _registry,
    size_t             _capacity
)
{
    if (!_registry)
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) ||
        d_registry_is_static(_registry))
    {
        return false;
    }

    if (_capacity <= _registry->lookup_capacity)
    {
        return true;
    }

    return d_registry_ensure_lookup_capacity(_registry, _capacity);
}

/*
d_registry_shrink_to_fit
  Reduces the row and lookup array allocations to exactly match their
  current counts.

Parameter(s):
  _registry: the registry to shrink; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if shrink succeeded (or no shrinking was needed), or
  - false, if _registry is NULL, the registry is frozen/static, or
    realloc failed.
*/
bool
d_registry_shrink_to_fit
(
    struct d_registry* _registry
)
{
    void*                           rows_new;
    struct d_registry_lookup_entry* lookup_new;

    if (!_registry)
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) ||
        d_registry_is_static(_registry))
    {
        return false;
    }

    // shrink row array
    if (_registry->capacity > _registry->count)
    {
        rows_new = realloc(_registry->rows,
                           _registry->count * _registry->row_size);

        if (_registry->count != 0 && !rows_new)
        {
            return false;
        }

        _registry->rows     = rows_new;
        _registry->capacity = _registry->count;
    }

    // shrink lookup array
    if (_registry->lookup_capacity > _registry->lookup_count)
    {
        lookup_new = (struct d_registry_lookup_entry*)realloc(
            _registry->lookup,
            _registry->lookup_count
                * sizeof(struct d_registry_lookup_entry)
        );

        if (_registry->lookup_count != 0 && !lookup_new)
        {
            return false;
        }

        _registry->lookup          = lookup_new;
        _registry->lookup_capacity = _registry->lookup_count;
    }

    return true;
}

/*
d_registry_freeze
  Sets the FROZEN flag, preventing all modifications to the registry.

Parameter(s):
  _registry: the registry to freeze; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the flag was set (or was already set), or
  - false, if _registry is NULL.
*/
bool
d_registry_freeze
(
    struct d_registry* _registry
)
{
    if (!_registry)
    {
        return false;
    }

    _registry->flags |= (uint8_t)D_REGISTRY_FLAG_FROZEN;

    return true;
}

/*
d_registry_thaw
  Clears the FROZEN flag, re-enabling modifications to the registry.

Parameter(s):
  _registry: the registry to thaw; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the flag was cleared (or was already clear), or
  - false, if _registry is NULL.
*/
bool
d_registry_thaw
(
    struct d_registry* _registry
)
{
    if (!_registry)
    {
        return false;
    }

    _registry->flags &= (uint8_t)~D_REGISTRY_FLAG_FROZEN;

    return true;
}

/*
d_registry_get_all_keys
  Allocates and returns an array of all key strings in the lookup table
  (including aliases). The caller is responsible for freeing the returned
  array.

Parameter(s):
  _registry:  the registry to query; may be NULL.
  _out_count: pointer to receive the number of keys; may be NULL.
Return:
  A newly allocated array of key string pointers, or NULL if the registry
  is NULL, is empty, or allocation fails. Sets *_out_count to the number
  of keys returned.
*/
const char**
d_registry_get_all_keys
(
    const struct d_registry* _registry,
    size_t*                  _out_count
)
{
    const char** out;
    size_t       i;

    if (_out_count)
    {
        *_out_count = 0;
    }

    if ( (!_registry) || 
         (_registry->lookup_count == 0) )
    {
        return NULL;
    }

    out = malloc(_registry->lookup_count * sizeof(const char*));

    if (!out)
    {
        return NULL;
    }

    for (i = 0; i < _registry->lookup_count; ++i)
    {
        out[i] = _registry->lookup[i].key;
    }

    if (_out_count) 
    {
        *(_out_count) = _registry->lookup_count;
    }

    return out;
}


/******************************************************************************
 * DESTRUCTOR
 *****************************************************************************/

/*
d_registry_free
  Frees a registry and all associated memory. If the OWNS_ROWS flag is
  set, row_free is called for each row via d_registry_clear. Static
  registries only free the d_registry struct itself, not the row or
  lookup arrays.

Parameter(s):
  _registry: the registry to free; may be NULL.
Return:
  none.
*/
void
d_registry_free
(
    struct d_registry* _registry
)
{
    if (!_registry)
    {
        return;
    }

    if (!d_registry_is_static(_registry))
    {
        d_registry_clear(_registry);
        free(_registry->rows);
        free(_registry->lookup);
    }

    free(_registry);

    return;
}