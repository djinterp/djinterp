/******************************************************************************
* djinterp [parse]                                                      pool.c
*
*   Definitions for the non-inline declarations in pool.h.
*
*
* path:      /src/djinterp/parse/pool.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parse/pool.h"  // corresponding header
// std
#include <string.h>  // memset, memcpy, memcmp, strlen
// djinterp
#include "../../../inc/djinterp/parse/storage.h"  // d_parse_grow, the shared
                                                  // growth policy
#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSE_POOL_HEAP


/*
d_parse_pool_init
  Initialises a pool over caller-supplied storage.
NOTE:
  Both arrays are optional and independent. A pool with neither is valid: it
interns nothing and every intern reports D_PARSE_POOL_NONE, which is the right
answer for a family whose instructions carry no variable operands.

Parameter(s):
  _pool:           the pool to initialise; ignored if NULL.
  _bytes:          storage for blob data; may be NULL.
  _byte_capacity:  how many bytes _bytes holds.
  _entries:        storage for the entry table; may be NULL.
  _entry_capacity: how many entries _entries holds.
Return:
  none.
*/
void
d_parse_pool_init(
    struct d_parse_pool*       _pool,
    char*                      _bytes,
    uint32_t                   _byte_capacity,
    struct d_parse_pool_entry* _entries,
    uint32_t                   _entry_capacity
)
{
    if (!_pool)
    {
        return;
    }

    memset(_pool, 0, sizeof(*_pool));

    _pool->bytes          = _bytes;
    _pool->capacity       = (_bytes != NULL) ? _byte_capacity : 0u;
    _pool->entries        = _entries;
    _pool->entry_capacity = (_entries != NULL) ? _entry_capacity : 0u;

    return;
}


#if (D_INTERNAL_PARSE_POOL_HEAP == 1)

/*
d_parse_pool_init_heap
  Initialises a pool over storage it allocates and owns, which then grows on
demand.

Parameter(s):
  _pool:           the pool to initialise; ignored if NULL.
  _byte_capacity:  bytes to reserve; 0 selects D_PARSE_POOL_DEFAULT_BYTES.
  _entry_capacity: entries to reserve; 0 selects D_PARSE_POOL_DEFAULT_ENTRIES.
Post-condition(s):
  - on success the pool owns both arrays; on failure it is left valid, empty,
    and owning nothing.
Return:
  0 on success; -1 if _pool is NULL or an allocation was refused.
*/
int
d_parse_pool_init_heap(
    struct d_parse_pool* _pool,
    uint32_t             _byte_capacity,
    uint32_t             _entry_capacity
)
{
    if (!_pool)
    {
        return -1;
    }

    d_parse_pool_init(_pool, NULL, 0u, NULL, 0u);

    const uint32_t bytes = (_byte_capacity > 0u)
                           ? _byte_capacity
                           : (uint32_t)D_PARSE_POOL_DEFAULT_BYTES;
    const uint32_t slots = (_entry_capacity > 0u)
                           ? _entry_capacity
                           : (uint32_t)D_PARSE_POOL_DEFAULT_ENTRIES;

    char* store = (char*)malloc((size_t)bytes);

    // check if the blob allocation was successful
    if (!store)
    {
        return -1;
    }

    struct d_parse_pool_entry* table =
        (struct d_parse_pool_entry*)malloc((size_t)slots * sizeof(*table));

    // check if the table allocation was successful
    if (!table)
    {
        free(store);

        return -1;
    }

    _pool->bytes          = store;
    _pool->capacity       = bytes;
    _pool->entries        = table;
    _pool->entry_capacity = slots;
    _pool->flags          = (uint8_t)(D_PARSE_POOL_OWNS_BYTES |
                                      D_PARSE_POOL_OWNS_ENTRIES);

    return 0;
}


/*
d_parse_pool_internal_reserve
  Grows an owned pool so one more blob of a given length fits.
NOTE:
  Two arrays, one policy. The blob store must hold the bytes already used plus
the incoming ones; the entry table must hold one more entry.

Parameter(s):
  _pool:   the pool to grow.
  _length: the length of the blob about to be stored.
Return:
  0 on success; -1 when the pool does not own the storage that must grow, or a
reallocation was refused.
*/
static int
d_parse_pool_internal_reserve(
    struct d_parse_pool* _pool,
    uint32_t             _length
)
{
    // refuse a total that would wrap before the growth policy ever sees it
    if (_length > (0xFFFFFFFFu - _pool->used))
    {
        return -1;
    }

    void* const bytes = d_parse_grow(_pool->bytes,
                                     &_pool->capacity,
                                     _pool->used + _length,
                                     1u,
                                     (_pool->flags &
                                      D_PARSE_POOL_OWNS_BYTES) != 0u);

    // check if the blob store could be made large enough
    if (!bytes)
    {
        return -1;
    }

    _pool->bytes = (char*)bytes;

    void* const entries = d_parse_grow(_pool->entries,
                                       &_pool->entry_capacity,
                                       _pool->count + 1u,
                                       (uint32_t)sizeof(*_pool->entries),
                                       (_pool->flags &
                                        D_PARSE_POOL_OWNS_ENTRIES) != 0u);

    // check if the entry table could be made large enough
    if (!entries)
    {
        return -1;
    }

    _pool->entries = (struct d_parse_pool_entry*)entries;

    return 0;
}

#endif  // D_INTERNAL_PARSE_POOL_HEAP


/*
d_parse_pool_reset
  Empties a pool for reuse, keeping its storage.
CAUTION:
  Every index handed out before this call becomes meaningless. Reset a pool
only alongside the program whose operands refer into it.

Parameter(s):
  _pool: the pool to empty; ignored if NULL.
Return:
  none.
*/
void
d_parse_pool_reset(
    struct d_parse_pool* _pool
)
{
    if (!_pool)
    {
        return;
    }

    _pool->used  = 0u;
    _pool->count = 0u;

    return;
}


/*
d_parse_pool_release
  Releases any storage the pool owns and leaves it empty.

Parameter(s):
  _pool: the pool to release; ignored if NULL.
Return:
  none.
*/
void
d_parse_pool_release(
    struct d_parse_pool* _pool
)
{
    if (!_pool)
    {
        return;
    }

#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
    // free only what this pool allocated; caller storage is never touched
    if ( (_pool->bytes != NULL) &&
         ((_pool->flags & D_PARSE_POOL_OWNS_BYTES) != 0u) )
    {
        free(_pool->bytes);
    }

    if ( (_pool->entries != NULL) &&
         ((_pool->flags & D_PARSE_POOL_OWNS_ENTRIES) != 0u) )
    {
        free(_pool->entries);
    }
#endif  // D_INTERNAL_PARSE_POOL_HEAP

    memset(_pool, 0, sizeof(*_pool));

    return;
}


/*
d_parse_pool_find
  The index of a blob already interned, without adding one.

Parameter(s):
  _pool:   the pool to search; may be NULL.
  _data:   the bytes to look for; may be NULL when _length is 0.
  _length: how many bytes to compare.
Return:
  The index of the matching entry, or D_PARSE_POOL_NONE when there is none.
*/
uint32_t
d_parse_pool_find(
    const struct d_parse_pool* _pool,
    const void*                _data,
    uint32_t                   _length
)
{
    // reject a missing pool or storage; a zero-length blob is not interned,
    // since it carries no information an index could not
    if ( (!_pool)           ||
         (!_pool->entries)  ||
         (!_pool->bytes)    ||
         (!_data)           ||
         (_length == 0u)    )
    {
        return D_PARSE_POOL_NONE;
    }

    // scan, filtering on length first so most candidates cost one comparison
    for (uint32_t index = 0u; index < _pool->count; index++)
    {
        if (_pool->entries[index].length != _length)
        {
            continue;
        }

        const char* const candidate = _pool->bytes +
                                      _pool->entries[index].offset;

        if (memcmp(candidate, _data, (size_t)_length) == 0)
        {
            return index;
        }
    }

    return D_PARSE_POOL_NONE;
}


/*
d_parse_pool_intern
  The index of a blob, adding it if it is not already present.
NOTE:
  Interning is what makes an operand comparable as an integer: two terms that
name the same class get the same index, so a later pass can ask whether two
alternatives begin with the same set without looking at any bytes.

Parameter(s):
  _pool:   the pool to intern into; may be NULL.
  _data:   the bytes to store; may be NULL when _length is 0.
  _length: how many bytes to store.
Return:
  The index of the entry, or D_PARSE_POOL_NONE when the pool is absent, the
blob is empty, or storage could not be found for it.
*/
uint32_t
d_parse_pool_intern(
    struct d_parse_pool* _pool,
    const void*          _data,
    uint32_t             _length
)
{
    if ( (!_pool)        ||
         (!_data)        ||
         (_length == 0u) )
    {
        return D_PARSE_POOL_NONE;
    }

    const uint32_t existing = d_parse_pool_find(_pool, _data, _length);

    // an identical blob already stored is the answer; that is the whole point
    if (existing != D_PARSE_POOL_NONE)
    {
        return existing;
    }

#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
    if (d_parse_pool_internal_reserve(_pool, _length) != 0)
    {
        return D_PARSE_POOL_NONE;
    }
#else
    // without growth, refuse rather than overrun the caller's storage
    if ( (!_pool->bytes)                               ||
         (!_pool->entries)                             ||
         ((_pool->capacity - _pool->used) < _length)   ||
         (_pool->count >= _pool->entry_capacity)       )
    {
        return D_PARSE_POOL_NONE;
    }
#endif  // D_INTERNAL_PARSE_POOL_HEAP

    const uint32_t index = _pool->count;

    memcpy(_pool->bytes + _pool->used, _data, (size_t)_length);

    _pool->entries[index].offset = _pool->used;
    _pool->entries[index].length = _length;

    _pool->used += _length;
    _pool->count++;

    return index;
}


/*
d_parse_pool_intern_string
  The index of a string, adding it if it is not already present.
NOTE:
  The terminator is stored with the text, so d_parse_pool_string hands back a
printable pointer with no copy.

Parameter(s):
  _pool: the pool to intern into; may be NULL.
  _text: the string to store; may be NULL.
Return:
  The index of the entry, or D_PARSE_POOL_NONE on failure. An empty string
interns successfully, since the terminator alone is a byte.
*/
uint32_t
d_parse_pool_intern_string(
    struct d_parse_pool* _pool,
    const char*          _text
)
{
    if (!_text)
    {
        return D_PARSE_POOL_NONE;
    }

    const size_t length = strlen(_text) + 1u;

    return d_parse_pool_intern(_pool, _text, (uint32_t)length);
}


/*
d_parse_pool_hash
  A 64-bit digest of everything the pool holds.
NOTE:
  FNV-1a over the entry lengths and the blob bytes, in index order -- not over
the raw byte store, because two pools reaching the same contents by different
intern orders would otherwise digest differently. This is a cache key, not a
cryptographic hash.

Parameter(s):
  _pool: the pool to digest; may be NULL, which digests as the empty pool.
Return:
  The digest.
*/
uint64_t
d_parse_pool_hash(
    const struct d_parse_pool* _pool
)
{
    uint64_t digest = 1469598103934665603ULL;

    if ( (!_pool)           ||
         (!_pool->entries)  ||
         (!_pool->bytes)    )
    {
        return digest;
    }

    // fold each blob in index order, length first, so a run of equal-looking
    // entries cannot alias by concatenation
    for (uint32_t index = 0u; index < _pool->count; index++)
    {
        const uint32_t length = _pool->entries[index].length;
        const char* const data = _pool->bytes + _pool->entries[index].offset;

        digest ^= (uint64_t)length;
        digest *= 1099511628211ULL;

        for (uint32_t byte = 0u; byte < length; byte++)
        {
            digest ^= (uint64_t)(unsigned char)data[byte];
            digest *= 1099511628211ULL;
        }
    }

    return digest;
}
