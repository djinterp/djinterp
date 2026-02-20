#include "..\..\..\inc\container\registry\registry.h"


#include "..\..\..\inc\container\registry\registry.h"



/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

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

D_STATIC_INLINE bool
d_registry_is_frozen
(
    const struct d_registry* registry
)
{
    return registry && ((registry->flags & D_REGISTRY_FLAG_FROZEN) != 0);
}

D_STATIC_INLINE bool
d_registry_is_static
(
    const struct d_registry* registry
)
{
    return registry && ((registry->flags & D_REGISTRY_FLAG_STATIC_ROWS) != 0);
}

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

D_STATIC bool
d_registry_ensure_row_capacity
(
    struct d_registry* registry,
    size_t _needed
)
{
    void*  new_rows;
    size_t new_cap;

    if (!registry) return false;
    if (registry->capacity >= _needed) return true;

    new_cap = (registry->capacity == 0) ? D_REGISTRY_DEFAULT_CAPACITY : registry->capacity;

    while (new_cap < _needed)
    {
        new_cap *= D_REGISTRY_GROWTH_FACTOR;
        if (new_cap < registry->capacity)  // overflow guard
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
        if (new_cap < _registry->lookup_capacity)  // overflow guard
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

D_STATIC bool
d_registry_lookup_entry_is_canonical
(
    const struct d_registry* registry,
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

    row_key = D_REGISTRY_ROW_KEY(d_registry_row_ptr(registry, _e->row_index));

    eq = ( (registry->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0)
            ? (d_registry_keycmp_case_sensitive(_e->key, row_key) == 0)
            : (d_registry_keycmp_nocase(_e->key, row_key) == 0);

    return eq;
}


/******************************************************************************
 * CONSTRUCTORS
 *****************************************************************************/

struct d_registry*
d_registry_new
(
    size_t _row_size
)
{
    return d_registry_new_with_capacity(_row_size, D_REGISTRY_DEFAULT_CAPACITY);
}

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

    new_registry->row_size = _row_size;
    new_registry->flags    = (uint8_t)D_REGISTRY_FLAG_DEFAULT;

    if (_capacity > 0)
    {
        if (!d_registry_reserve(new_registry, _capacity))
        {
            d_registry_free(new_registry);

            return NULL;
        }
        if (!d_registry_reserve_lookup(new_registry, _capacity))
        {
            d_registry_free(new_registry);

            return NULL;
        }
    }

    return new_registry;
}

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

    new_registry->flags    = (uint8_t)(_other->flags & ~D_REGISTRY_FLAG_STATIC_ROWS);
    new_registry->row_free = _other->row_free;

    if (_other->count > 0 && _other->rows)
    {
        memcpy(new_registry->rows,
               _other->rows,
               (_other->count * _other->row_size) );

        new_registry->count = _other->count;
    }

    if ( (_other->lookup_count > 0) &&
         (_other->lookup) )
    {
        if (!d_registry_reserve_lookup(new_registry, _other->lookup_count))
        {
            d_registry_free(new_registry);

            return NULL;
        }

        memcpy(new_registry->lookup,
               _other->lookup,
               _other->lookup_count * sizeof(struct d_registry_lookup_entry));

        new_registry->lookup_count = _other->lookup_count;
    }

    return new_registry;
}

struct d_registry*
d_registry_new_from_array
(
    const void* _rows,
    size_t _row_size,
    size_t _count
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

D_INLINE int
d_registry_lookup_compare
(
    const void* _a,
    const void* _b
)
{
    const struct d_registry_lookup_entry* a = (const struct d_registry_lookup_entry*)_a;
    const struct d_registry_lookup_entry* b = (const struct d_registry_lookup_entry*)_b;

    return d_registry_keycmp_case_sensitive(a->key, b->key);
}

D_INLINE int
d_registry_lookup_compare_nocase
(
    const void* _a,
    const void* _b
)
{
    const struct d_registry_lookup_entry* a = (const struct d_registry_lookup_entry*)_a;
    const struct d_registry_lookup_entry* b = (const struct d_registry_lookup_entry*)_b;

    return d_registry_keycmp_nocase(a->key, b->key);
}

void
d_registry_sort_lookup
(
    struct d_registry* _registry
)
{
    fn_comparator comparator;

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
}

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

    // per header comment: rebuilds from row keys only (aliases are dropped).
    _registry->lookup_count = 0;

    if ( (_registry->count == 0) ||
         (!d_registry_ensure_lookup_capacity(_registry, _registry->count)) )
    {
        return;
    }

    for (i = 0; i < _registry->count; ++i)
    {
        void* row = d_registry_row_ptr(_registry, i);

        _registry->lookup[i].key       = D_REGISTRY_ROW_KEY(row);
        _registry->lookup[i].row_index = i;
    }

    _registry->lookup_count = _registry->count;

    d_registry_sort_lookup(_registry);
}


/******************************************************************************
 * PRIMARY LOOKUP
 *****************************************************************************/

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

bool
d_registry_add
(
    struct d_registry* _registry,
    const void* _row
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

    if (d_registry_contains(_registry, key))
    {
        return false;
    }

    insert_at = _registry->count;

    // Optional sorted insertion by key.
    if ( (_registry->flags & D_REGISTRY_FLAG_SORTED) != 0)
    {
        for (i = 0; i < _registry->count; ++i)
        {
            row_key = D_REGISTRY_ROW_KEY(d_registry_row_ptr(_registry, i));

            int cmp = ((_registry->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) == 0)
                        ? d_registry_keycmp_case_sensitive(key, row_key)
                        : d_registry_keycmp_nocase(key, row_key);

            if (cmp < 0)
            {
                insert_at = i;
                break;
            }
        }
    }

    if (!d_registry_ensure_row_capacity(_registry, _registry->count + 1))
    {
        return false;
    }

    if (!d_registry_ensure_lookup_capacity(_registry, _registry->lookup_count + 1))
    {
        return false;
    }

    // Make space in rows if inserting in the middle.
    if (insert_at < _registry->count)
    {
        memmove(
            (char*)_registry->rows + ((insert_at + 1) * _registry->row_size),
            (char*)_registry->rows + (insert_at * _registry->row_size),
            (_registry->count - insert_at) * _registry->row_size
        );

        // Any lookup entry that points at rows >= insert_at must shift.
        for (i = 0; i < _registry->lookup_count; ++i)
        {
            if (_registry->lookup[i].row_index >= insert_at)
            {
                _registry->lookup[i].row_index += 1;
            }
        }
    }

    memcpy(d_registry_row_ptr(_registry, insert_at), _row, _registry->row_size);
    _registry->count += 1;

    // Add canonical key entry.
    _registry->lookup[_registry->lookup_count].key       = D_REGISTRY_ROW_KEY(d_registry_row_ptr(_registry, insert_at));
    _registry->lookup[_registry->lookup_count].row_index = insert_at;
    _registry->lookup_count += 1;

    d_registry_sort_lookup(_registry);

    return true;
}

bool
d_registry_add_rows
(
    struct d_registry* _registry,
    const void* _rows,
    size_t _count
)
{
    size_t i;

    if (!_registry || !_rows || _count == 0)
    {
        return false;
    }

    for (i = 0; i < _count; ++i)
    {
        const void* row = (const void*)((const char*)_rows + (i * _registry->row_size));

        if (!d_registry_add(_registry, row))
        {
            return false;  // partial adds may have occurred
        }
    }

    return true;
}

bool
d_registry_set
(
    struct d_registry* _registry,
    const char* _key,
    const void* _row
)
{
    struct d_registry_lookup_entry* e;
    void* dst;
    const char* existing_key;

    if (!_registry || !_key || !_row)
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) || d_registry_is_static(_registry))
    {
        return false;
    }

    e = d_registry_find_lookup_entry(_registry, _key);
    if (!e || e->row_index >= _registry->count)
    {
        return false;
    }

    dst = d_registry_row_ptr(_registry, e->row_index);

    // Preserve the canonical key in the row (don't allow key changes via set).
    existing_key = D_REGISTRY_ROW_KEY(dst);

    memcpy(dst, _row, _registry->row_size);
    *((const char**)dst) = existing_key;

    return true;
}

bool
d_registry_remove_at
(
    struct d_registry* _registry,
    size_t _index
)
{
    size_t i;
    size_t w;

    if (!_registry)
    {
        return false;
    }

    if (d_registry_is_frozen(_registry) || d_registry_is_static(_registry))
    {
        return false;
    }

    if (_index >= _registry->count)
    {
        return false;
    }

    // Row destructor if enabled.
    if ((_registry->flags & D_REGISTRY_FLAG_OWNS_ROWS) != 0 && _registry->row_free)
    {
        _registry->row_free(d_registry_row_ptr(_registry, _index));
    }

    // Shift rows down.
    if (_index + 1 < _registry->count)
    {
        memmove(
            (char*)_registry->rows + (_index * _registry->row_size),
            (char*)_registry->rows + ((_index + 1) * _registry->row_size),
            (_registry->count - (_index + 1)) * _registry->row_size
        );
    }

    _registry->count -= 1;

    // Remove lookup entries pointing to this row and shift indices above it.
    w = 0;
    for (i = 0; i < _registry->lookup_count; ++i)
    {
        struct d_registry_lookup_entry e = _registry->lookup[i];

        if (e.row_index == _index)
        {
            continue; // drop canonical + aliases pointing to removed row
        }

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

bool
d_registry_remove
(
    struct d_registry* _registry,
    const char* _key
)
{
    struct d_registry_lookup_entry* e;

    if (!_registry || !_key)
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

    if (d_registry_is_static(_registry))
    {
        // D_STATIC registry tables are immutable; just drop lookup view.
        _registry->lookup_count = 0;
        _registry->count        = _registry->count; // no-op clarity
        return;
    }

    if ((_registry->flags & D_REGISTRY_FLAG_OWNS_ROWS) != 0 && _registry->row_free)
    {
        for (i = 0; i < _registry->count; ++i)
        {
            _registry->row_free(d_registry_row_ptr(_registry, i));
        }
    }

    _registry->count        = 0;
    _registry->lookup_count = 0;
}


/******************************************************************************
 * ALIASES
 *****************************************************************************/

bool
d_registry_add_alias
(
    struct d_registry* _registry,
    const char* _key,
    const char* _alias
)
{
    struct d_registry_lookup_entry* e;
    size_t row_index;

    if (!_registry || !_key || !_alias)
    {
        return false;
    }

    // frozen registries reject all modifications; static registries only
    // protect the row array -- aliases modify the lookup table, which may
    // have pre-allocated capacity (D_REGISTRY_DEFINE_WITH_ALIASES).
    if (d_registry_is_frozen(_registry))
    {
        return false;
    }

    e = d_registry_find_lookup_entry(_registry, _key);
    if (!e)
    {
        return false;
    }

    row_index = e->row_index;

    if (d_registry_contains(_registry, _alias))
    {
        return false; // alias already in use (as key or alias)
    }

    if (!d_registry_ensure_lookup_capacity(_registry, _registry->lookup_count + 1))
    {
        return false;
    }

    _registry->lookup[_registry->lookup_count].key       = _alias;
    _registry->lookup[_registry->lookup_count].row_index = row_index;
    _registry->lookup_count += 1;

    d_registry_sort_lookup(_registry);

    return true;
}

bool
d_registry_add_aliases
(
    struct d_registry* _registry,
    const char* _key,
    const char** _aliases,
    size_t _count
)
{
    size_t i;

    if (!_registry || !_key || !_aliases || _count == 0)
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

bool
d_registry_remove_alias
(
    struct d_registry* _registry,
    const char* _alias
)
{
    struct d_registry_lookup_entry* e;
    size_t i;
    size_t idx;

    if (!_registry || !_alias)
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

    // Don't remove canonical keys via remove_alias.
    if (d_registry_lookup_entry_is_canonical(_registry, e))
    {
        return false;
    }

    idx = (size_t)(e - _registry->lookup);

    for (i = idx + 1; i < _registry->lookup_count; ++i)
    {
        _registry->lookup[i - 1] = _registry->lookup[i];
    }

    _registry->lookup_count -= 1;

    return true;
}

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

    w = 0;
    for (i = 0; i < _registry->lookup_count; ++i)
    {
        if (d_registry_lookup_entry_is_canonical(_registry, &_registry->lookup[i]))
        {
            _registry->lookup[w++] = _registry->lookup[i];
        }
    }

    _registry->lookup_count = w;

    d_registry_sort_lookup(_registry);
}

size_t
d_registry_alias_count
(
    const struct d_registry* _registry
)
{
    size_t i;
    size_t n = 0;

    if (!_registry)
    {
        return 0;
    }

    for (i = 0; i < _registry->lookup_count; ++i)
    {
        if (!d_registry_lookup_entry_is_canonical(_registry, &_registry->lookup[i]))
        {
            n += 1;
        }
    }

    return n;
}


/******************************************************************************
 * QUERIES
 *****************************************************************************/

bool
d_registry_contains
(
    const struct d_registry* _registry,
    const char* _key
)
{
    return (d_registry_index_of(_registry, _key) >= 0);
}

ssize_t
d_registry_index_of
(
    const struct d_registry* _registry,
    const char* _key
)
{
    struct d_registry_lookup_entry* e;

    if (!_registry || !_key)
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

void*
d_registry_at
(
    const struct d_registry* _registry,
    size_t _index
)
{
    if (!_registry || _index >= _registry->count)
    {
        return NULL;
    }

    return d_registry_row_ptr(_registry, _index);
}

size_t
d_registry_count
(
    const struct d_registry* _registry
)
{
    return _registry ? _registry->count : 0;
}

size_t
d_registry_capacity
(
    const struct d_registry* _registry
)
{
    return _registry ? _registry->capacity : 0;
}

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

struct d_registry_iterator
d_registry_iterator_filtered
(
    struct d_registry* _registry,
    fn_registry_row_predicate _filter,
    const void* _context
)
{
    struct d_registry_iterator it;
    it.registry   = _registry;
    it.current    = 0;
    it.filter     = _filter;
    it.filter_ctx = _context;
    return it;
}

bool
d_registry_iterator_has_next
(
    const struct d_registry_iterator* _iter
)
{
    size_t i;

    if (!_iter || !_iter->registry)
    {
        return false;
    }

    if (!_iter->filter)
    {
        return _iter->current < _iter->registry->count;
    }

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

void*
d_registry_iterator_next
(
    struct d_registry_iterator* _iter
)
{
    if (!_iter || !_iter->registry)
    {
        return NULL;
    }

    while (_iter->current < _iter->registry->count)
    {
        void* row = d_registry_row_ptr(_iter->registry, _iter->current);
        _iter->current += 1;

        if (!_iter->filter || _iter->filter(row, _iter->filter_ctx))
        {
            return row;
        }
    }

    return NULL;
}

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
}

void
d_registry_foreach
(
    struct d_registry* _registry,
    fn_registry_row_visitor _visitor,
    void* _context
)
{
    size_t i;

    if (!_registry || !_visitor)
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
}

void
d_registry_foreach_if
(
    struct d_registry* _registry,
    fn_registry_row_predicate _predicate,
    const void* _pred_ctx,
    fn_registry_row_visitor _visitor,
    void* _visit_ctx
)
{
    size_t i;

    if (!_registry || !_visitor)
    {
        return;
    }

    for (i = 0; i < _registry->count; ++i)
    {
        void* row = d_registry_row_ptr(_registry, i);

        if (_predicate && !_predicate(row, _pred_ctx))
        {
            continue;
        }

        if (!_visitor(row, _visit_ctx))
        {
            break;
        }
    }
}


/******************************************************************************
 * UTILITIES
 *****************************************************************************/

bool
d_registry_reserve
(
    struct d_registry* _registry,
    size_t _capacity
)
{
    if (!_registry) return false;
    if (d_registry_is_frozen(_registry) || d_registry_is_static(_registry)) return false;

    if (_capacity <= _registry->capacity)
    {
        return true;
    }

    return d_registry_ensure_row_capacity(_registry, _capacity);
}

bool
d_registry_reserve_lookup
(
    struct d_registry* _registry,
    size_t _capacity
)
{
    if (!_registry) return false;
    if (d_registry_is_frozen(_registry) || d_registry_is_static(_registry)) return false;

    if (_capacity <= _registry->lookup_capacity)
    {
        return true;
    }

    return d_registry_ensure_lookup_capacity(_registry, _capacity);
}

bool
d_registry_shrink_to_fit
(
    struct d_registry* _registry
)
{
    void* rows_new;
    struct d_registry_lookup_entry* lookup_new;

    if (!_registry) return false;
    if (d_registry_is_frozen(_registry) || d_registry_is_static(_registry)) return false;

    if (_registry->capacity > _registry->count)
    {
        rows_new = realloc(_registry->rows, _registry->count * _registry->row_size);
        if (_registry->count != 0 && !rows_new) return false;
        _registry->rows = rows_new;
        _registry->capacity = _registry->count;
    }

    if (_registry->lookup_capacity > _registry->lookup_count)
    {
        lookup_new = (struct d_registry_lookup_entry*)realloc(
            _registry->lookup,
            _registry->lookup_count * sizeof(struct d_registry_lookup_entry)
        );
        if (_registry->lookup_count != 0 && !lookup_new) return false;
        _registry->lookup = lookup_new;
        _registry->lookup_capacity = _registry->lookup_count;
    }

    return true;
}

bool
d_registry_freeze
(
    struct d_registry* _registry
)
{
    if (!_registry) return false;
    _registry->flags |= (uint8_t)D_REGISTRY_FLAG_FROZEN;
    return true;
}

bool
d_registry_thaw
(
    struct d_registry* _registry
)
{
    if (!_registry) return false;
    _registry->flags &= (uint8_t)~D_REGISTRY_FLAG_FROZEN;
    return true;
}

const char**
d_registry_get_all_keys
(
    const struct d_registry* _registry,
    size_t* _out_count
)
{
    const char** out;
    size_t i;

    if (_out_count) *_out_count = 0;

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
}