/*******************************************************************************
* djinterp [c]                                                     text_intern.c
*
*   Text interning: an arena of names, each terminated, with its offset,
* length and hash, and an open-addressed bucket array mapping a hash to a
* symbol. The buckets double before they pass two thirds full, where linear
* probing starts to cluster, and a rehash reads no text: every symbol lands
* where its stored hash says.
*
*
* path:      /src/djinterp/c/text/text_intern.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/text_intern.h"  // corresponding
                                                          // header
// std
#include <stdlib.h>  // malloc, calloc, realloc, free
#include <string.h>  // memcpy, memcmp
// djinterp
#include "../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul


// dj_text_intern
//   struct: the arena of names; per symbol its offset, length and hash; and
// the bucket array mapping a hash to a symbol.
struct dj_text_intern
{
    char*     text;           // every name, each followed by a terminator
    size_t    text_used;
    size_t    text_capacity;
    uint32_t* offsets;        // where each symbol's name starts in `text`
    uint32_t* lengths;
    uint32_t* hashes;
    size_t    count;
    size_t    capacity;       // of offsets, lengths and hashes
    uint32_t* buckets;        // a symbol, or D_TEXT_INTERN_NONE
    size_t    bucket_mask;    // the bucket count less one; 0 before any
};


/*
d_internal_text_intern_hash
  FNV-1a over a counted string.
*/
static uint32_t
d_internal_text_intern_hash(
    const char* _text,
    size_t      _length
)
{
    uint32_t hash = 2166136261u;
    size_t   at;

    for (at = 0u; at < _length; ++at)
    {
        hash ^= (unsigned char)_text[at];
        hash *= 16777619u;
    }

    return hash;
}

/*
d_internal_text_intern_rehash
  Doubles the bucket array (64 to start) and reinserts every symbol by its
stored hash. False, with the table as it was, when the array cannot grow.
*/
static bool
d_internal_text_intern_rehash(
    struct dj_text_intern* _table
)
{
    size_t    buckets = 64u;
    size_t    bytes;
    size_t    at;
    uint32_t* grown;

    if ( (_table->bucket_mask != 0u) &&
         (d_ckd_mul(&buckets, _table->bucket_mask + 1u, 2u)) )
    {
        return false;
    }

    if (d_ckd_mul(&bytes, buckets, sizeof(uint32_t)))
    {
        return false;
    }

    grown = (uint32_t*)malloc(bytes);

    if (!grown)
    {
        return false;
    }

    for (at = 0u; at < buckets; ++at)
    {
        grown[at] = D_TEXT_INTERN_NONE;
    }

    // every symbol lands where its stored hash says; no text is read
    for (at = 0u; at < _table->count; ++at)
    {
        size_t slot = _table->hashes[at] & (buckets - 1u);

        while (grown[slot] != D_TEXT_INTERN_NONE)
        {
            slot = (slot + 1u) & (buckets - 1u);
        }

        grown[slot] = (uint32_t)at;
    }

    free(_table->buckets);
    _table->buckets     = grown;
    _table->bucket_mask = buckets - 1u;

    return true;
}

/*
d_internal_text_intern_slot
  A name's bucket slot: its symbol's, or the empty one it would take. The
table must have buckets.
*/
static size_t
d_internal_text_intern_slot(
    const struct dj_text_intern* _table,
    const char*                  _text,
    size_t                       _length,
    uint32_t                     _hash
)
{
    size_t slot = _hash & _table->bucket_mask;

    while (_table->buckets[slot] != D_TEXT_INTERN_NONE)
    {
        const uint32_t symbol = _table->buckets[slot];

        // hash and length first: text is compared only on a likely match
        if ( (_table->hashes[symbol] == _hash)    &&
             (_table->lengths[symbol] == _length) &&
             ( (_length == 0u) ||
               (memcmp(_table->text + _table->offsets[symbol],
                       _text,
                       _length) == 0) ) )
        {
            return slot;
        }

        slot = (slot + 1u) & _table->bucket_mask;
    }

    return slot;
}

/*
d_internal_text_intern_reserve
  Room for one more symbol in the per-symbol arrays. Each array is kept as
soon as it has grown, so a failure part way leaves the table whole.
*/
static bool
d_internal_text_intern_reserve(
    struct dj_text_intern* _table
)
{
    size_t    grown = 64u;
    size_t    bytes;
    uint32_t* array;

    if (_table->count < _table->capacity)
    {
        return true;
    }

    if ( ( (_table->capacity != 0u) &&
           (d_ckd_mul(&grown, _table->capacity, 2u)) ) ||
         (d_ckd_mul(&bytes, grown, sizeof(uint32_t))) )
    {
        return false;
    }

    array = (uint32_t*)realloc(_table->offsets, bytes);

    if (!array)
    {
        return false;
    }

    _table->offsets = array;
    array           = (uint32_t*)realloc(_table->lengths, bytes);

    if (!array)
    {
        return false;
    }

    _table->lengths = array;
    array           = (uint32_t*)realloc(_table->hashes, bytes);

    if (!array)
    {
        return false;
    }

    _table->hashes   = array;
    _table->capacity = grown;

    return true;
}

/*
d_internal_text_intern_room
  Room in the arena for _needed more bytes.
*/
static bool
d_internal_text_intern_room(
    struct dj_text_intern* _table,
    size_t                 _needed
)
{
    size_t wanted;
    size_t grown;
    char*  text;

    if (d_ckd_add(&wanted, _table->text_used, _needed))
    {
        return false;
    }

    if (wanted <= _table->text_capacity)
    {
        return true;
    }

    grown = (_table->text_capacity == 0u) ? 1024u : _table->text_capacity;

    while (grown < wanted)
    {
        if (d_ckd_mul(&grown, grown, 2u))
        {
            grown = wanted;

            break;
        }
    }

    text = (char*)realloc(_table->text, grown);

    if (!text)
    {
        return false;
    }

    _table->text          = text;
    _table->text_capacity = grown;

    return true;
}


/*
d_text_intern_new
  An empty table, allocated zeroed.
*/
struct dj_text_intern*
d_text_intern_new(void)
{
    return (struct dj_text_intern*)calloc(1u, sizeof(struct dj_text_intern));
}

/*
d_text_intern_free
  Accepts NULL, as free does.
*/
void
d_text_intern_free(
    struct dj_text_intern* _table
)
{
    if (!_table)
    {
        return;
    }

    free(_table->text);
    free(_table->offsets);
    free(_table->lengths);
    free(_table->hashes);
    free(_table->buckets);
    free(_table);

    return;
}

/*
d_text_intern_add
  Finds the name, or adds it: the buckets first, so that the slot found is
the one the name takes; then room for one more symbol and its text. The
arena keeps a terminator after each name, so d_text_intern_text can give a C
string. A name whose offset or number would not fit in 32 bits is refused:
D_TEXT_INTERN_NONE is the one number no name may have.
*/
uint32_t
d_text_intern_add(
    struct dj_text_intern* _table,
    const char*            _text,
    size_t                 _length
)
{
    uint32_t hash;
    size_t   slot;
    uint32_t symbol;
    size_t   needed;

    // parameter validation first
    if ( (!_table) ||
         (!_text) )
    {
        return D_TEXT_INTERN_NONE;
    }

    if ( (_table->bucket_mask == 0u) &&
         (!d_internal_text_intern_rehash(_table)) )
    {
        return D_TEXT_INTERN_NONE;
    }

    hash = d_internal_text_intern_hash(_text, _length);
    slot = d_internal_text_intern_slot(_table, _text, _length, hash);

    if (_table->buckets[slot] != D_TEXT_INTERN_NONE)
    {
        return _table->buckets[slot];
    }

    // a new name: its number, its offset and its length must fit 32 bits
    if ( (_table->count >= (size_t)(D_TEXT_INTERN_NONE - 1u)) ||
         (_length >= (size_t)UINT32_MAX)                     ||
         (_table->text_used > (size_t)UINT32_MAX)            ||
         (d_ckd_add(&needed, _length, 1u)) )
    {
        return D_TEXT_INTERN_NONE;
    }

    // two thirds full is where linear probing starts to cluster
    if ( ((_table->count + 1u) * 3u) > ((_table->bucket_mask + 1u) * 2u) )
    {
        if (!d_internal_text_intern_rehash(_table))
        {
            return D_TEXT_INTERN_NONE;
        }

        slot = d_internal_text_intern_slot(_table, _text, _length, hash);
    }

    if ( (!d_internal_text_intern_reserve(_table)) ||
         (!d_internal_text_intern_room(_table, needed)) )
    {
        return D_TEXT_INTERN_NONE;
    }

    symbol = (uint32_t)_table->count;

    if (_length > 0u)
    {
        memcpy(_table->text + _table->text_used, _text, _length);
    }

    _table->text[_table->text_used + _length] = '\0';
    _table->offsets[symbol] = (uint32_t)_table->text_used;
    _table->lengths[symbol] = (uint32_t)_length;
    _table->hashes[symbol]  = hash;
    _table->text_used      += needed;
    ++_table->count;
    _table->buckets[slot] = symbol;

    return symbol;
}

/*
d_text_intern_find
  The same probe as an add, with nothing added on a miss.
*/
uint32_t
d_text_intern_find(
    const struct dj_text_intern* _table,
    const char*                  _text,
    size_t                       _length
)
{
    if ( (!_table)                    ||
         (!_text)                     ||
         (_table->bucket_mask == 0u) )
    {
        return D_TEXT_INTERN_NONE;
    }

    return _table->buckets[d_internal_text_intern_slot(
                               _table,
                               _text,
                               _length,
                               d_internal_text_intern_hash(_text, _length))];
}

/*
d_text_intern_text
  "" rather than NULL for an unknown symbol, so that a caller printing a name
never has to guard.
*/
const char*
d_text_intern_text(
    const struct dj_text_intern* _table,
    uint32_t                     _symbol
)
{
    if ( (!_table) ||
         ((size_t)_symbol >= _table->count) )
    {
        return "";
    }

    return _table->text + _table->offsets[_symbol];
}

/*
d_text_intern_length
  0 for an unknown symbol, as its text is "".
*/
size_t
d_text_intern_length(
    const struct dj_text_intern* _table,
    uint32_t                     _symbol
)
{
    if ( (!_table) ||
         ((size_t)_symbol >= _table->count) )
    {
        return 0u;
    }

    return (size_t)_table->lengths[_symbol];
}

/*
d_text_intern_count
  Symbols are dense, so this is also one past the largest.
*/
size_t
d_text_intern_count(
    const struct dj_text_intern* _table
)
{
    return (_table) ? _table->count : 0u;
}
