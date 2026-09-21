/******************************************************************************
* djinterp [parse]                                                      pool.h
*
* An intern pool for the variable-length operands an instruction cannot hold.
*   An instruction is a fixed-size POD, which is what makes a program copyable,
* hashable, serialisable, and handed to C without a fix-up. Character classes
* and names do not fit in one, so they live here and the instruction carries an
* index. Interning means an identical blob stored twice gets one entry, so a
* grammar full of `[0-9]` costs one class, and equality of two operands is an
* integer comparison rather than a memcmp.
*
*   The pool is two arrays and no pointers into itself: a byte blob and a table
* of (offset, length). That is deliberate -- it means the pool serialises as
* its own bytes, and an index means the same thing after a round trip.
*
*   WHAT THIS IS NOT. It is not a hash table. Lookup is a linear scan filtered
* on length, which is an interning cost paid at BUILD time against a table of
* tens of entries, not a run-time cost. A hash index drops in behind the same
* API the day a grammar makes it matter; nothing above would change.
*
*   Requires: c/djinterp.h (qualifier kit) and config/parse/cfg_parse.h.
*
* path:      /inc/djinterp/parse/pool.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_PARSE_POOL_NONE
    2.  The entry
         1.  d_parse_pool_entry
    3.  The pool
         1.  d_parse_pool
         2.  Pool flags
              a. D_PARSE_POOL_OWNS_BYTES
              b. D_PARSE_POOL_OWNS_ENTRIES
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Interning
    3.  Access
    4.  Identity
*/

#ifndef DJINTERP_PARSE_POOL_
#define DJINTERP_PARSE_POOL_ 1

// std
#include <stddef.h>                     // size_t, NULL
#include <stdint.h>                     // uint8_t, uint32_t, uint64_t
// djinterp
#include "../c/djinterp.h"              // framework root
#include "../config/parse/cfg_parse.h"  // D_INTERNAL_PARSE_POOL_* knobs


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_PARSE_POOL_NONE
//   constant: the index meaning "no entry" -- returned by a failed intern and
// by a lookup that found nothing, and stored in an instruction operand that
// refers to no blob.
#define D_PARSE_POOL_NONE           0xFFFFFFFFu


// 1.2    The entry
//------------------------------------------------------------------------------
// 1.2.1
// d_parse_pool_entry
//   struct: where one blob sits in the byte store. An offset rather than a
// pointer, so the table survives a reallocation of the bytes and a round trip
// through a file unchanged.
struct d_parse_pool_entry
{
    uint32_t offset;
    uint32_t length;
};


// 1.3    The pool
//------------------------------------------------------------------------------
// 1.3.1
// d_parse_pool
//   struct: the blob store and its table. Either array may be caller-supplied
// or owned; a pool with neither is valid and interns nothing, which is the
// right shape for a family whose instructions carry no variable operands.
struct d_parse_pool
{
    char*                      bytes;
    uint32_t                   used;
    uint32_t                   capacity;
    struct d_parse_pool_entry* entries;
    uint32_t                   count;
    uint32_t                   entry_capacity;
    uint8_t                    flags;
    uint8_t                    reserved[3];
};

// 1.3.2
// D_PARSE_POOL_OWNS_BYTES
//   constant: the byte store was allocated by the pool and is freed by
// d_parse_pool_release.
#define D_PARSE_POOL_OWNS_BYTES     0x01u
// D_PARSE_POOL_OWNS_ENTRIES
//   constant: the entry table was allocated by the pool.
#define D_PARSE_POOL_OWNS_ENTRIES   0x02u


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Lifetime
//------------------------------------------------------------------------------
void            d_parse_pool_init(struct d_parse_pool*       _pool,
                                  char*                      _bytes,
                                  uint32_t                   _byte_capacity,
                                  struct d_parse_pool_entry* _entries,
                                  uint32_t                   _entry_capacity);
#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
D_NODISCARD int d_parse_pool_init_heap(struct d_parse_pool* _pool,
                                       uint32_t             _byte_capacity,
                                       uint32_t             _entry_capacity);
#endif  // D_INTERNAL_PARSE_POOL_HEAP
void            d_parse_pool_reset(struct d_parse_pool* _pool);
void            d_parse_pool_release(struct d_parse_pool* _pool);

// 2.2    Interning
//------------------------------------------------------------------------------
uint32_t        d_parse_pool_find(const struct d_parse_pool* _pool,
                                  const void*                _data,
                                  uint32_t                   _length);
uint32_t        d_parse_pool_intern(struct d_parse_pool* _pool,
                                    const void*          _data,
                                    uint32_t             _length);
uint32_t        d_parse_pool_intern_string(struct d_parse_pool* _pool,
                                           const char*          _text);

// 2.3    Access
//------------------------------------------------------------------------------
/*
d_parse_pool_length
  The length in bytes of an interned blob.

Parameter(s):
  _pool:  the pool to read; may be NULL.
  _index: the index an intern returned.
Return:
  The length, or 0 when the index names no entry.
*/
D_INLINE uint32_t
d_parse_pool_length(
    const struct d_parse_pool* _pool,
    uint32_t                   _index
)
{
    // reject a missing pool, an unset index, and one past the table
    if ( (!_pool)                   ||
         (!_pool->entries)          ||
         (_index >= _pool->count)   )
    {
        return 0u;
    }

    return _pool->entries[_index].length;
}

/*
d_parse_pool_data
  The bytes of an interned blob.

Parameter(s):
  _pool:  the pool to read; may be NULL.
  _index: the index an intern returned.
Return:
  A pointer to the blob, or NULL when the index names no entry.
*/
D_INLINE const void*
d_parse_pool_data(
    const struct d_parse_pool* _pool,
    uint32_t                   _index
)
{
    // reject a missing pool, absent storage, and an index past the table
    if ( (!_pool)                   ||
         (!_pool->entries)          ||
         (!_pool->bytes)            ||
         (_index >= _pool->count)   )
    {
        return NULL;
    }

    return _pool->bytes + _pool->entries[_index].offset;
}

/*
d_parse_pool_string
  An interned string, ready to print.
NOTE:
  d_parse_pool_intern_string stores the terminator, so this is the blob
pointer with no copy. Interning raw bytes that happen not to end in a NUL and
reading them back here would not be safe, which is why the two spellings are
separate entry points.

Parameter(s):
  _pool:  the pool to read; may be NULL.
  _index: the index an intern returned.
Return:
  The string, or "" when the index names no entry. Never NULL, so a caller may
print the result unconditionally.
*/
D_INLINE const char*
d_parse_pool_string(
    const struct d_parse_pool* _pool,
    uint32_t                   _index
)
{
    const void* const data = d_parse_pool_data(_pool, _index);

    return (data != NULL) ? (const char*)data : "";
}

// 2.4    Identity
//------------------------------------------------------------------------------
uint64_t        d_parse_pool_hash(const struct d_parse_pool* _pool);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_POOL_
