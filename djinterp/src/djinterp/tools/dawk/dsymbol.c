/*******************************************************************************
* djinterp [tools]                                                     dsymbol.c
*
* Definitions for the non-inline declarations in dsymbol.h.
*   Open addressing with linear probing over a power-of-two bucket array,
* kept at most two-thirds full.  Each entry stores its hash and length, so a
* probe rejects a mismatch with two integer compares before touching text,
* and growth rehashes without reading a single name again.
*
*
* path:      /src/djinterp/tools/dawk/dsymbol.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dsymbol.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdlib.h>   // malloc, realloc, free, calloc
#include <string.h>   // memcpy, memcmp


// d_symbol_table
//   struct: the arena of names, per-symbol offset, length and hash, and the
// bucket array mapping a hash to a symbol.
struct d_symbol_table
{
    char*      text;
    size_t     text_used;
    size_t     text_capacity;

    uint32_t*  offsets;
    uint32_t*  lengths;
    uint32_t*  hashes;
    size_t     count;
    size_t     capacity;

    uint32_t*  buckets;
    size_t     bucket_mask;
};


/**
 * @brief FNV-1a over a counted string.
 *
 * @param[in] _text    the bytes.
 * @param[in] _length  their count.
 * @return the hash.
 */
static uint32_t
d_internal_symbol_hash(
    const char* _text,
    size_t      _length
)
{
    uint32_t hash = 2166136261u;

    for (size_t at = 0u; at < _length; ++at)
    {
        hash ^= (unsigned char)_text[at];
        hash *= 16777619u;
    }

    return hash;
}


/**
 * @brief Doubles the bucket array and reinserts every symbol by its stored
 *        hash.
 *
 * @param[in,out] _table  the table.
 * @return `true` on success, `false` if allocation failed.
 */
static bool
d_internal_symbol_rehash(
    struct d_symbol_table* _table
)
{
    const size_t buckets = (_table->bucket_mask == 0u)
                           ? 64u
                           : ((_table->bucket_mask + 1u) * 2u);
    uint32_t* const grown = malloc(buckets * sizeof(uint32_t));

    if (!grown)
    {
        return false;
    }

    for (size_t at = 0u; at < buckets; ++at)
    {
        grown[at] = D_SYMBOL_NONE;
    }

    // every symbol lands where its stored hash says, no text is read
    for (size_t symbol = 0u; symbol < _table->count; ++symbol)
    {
        size_t slot = _table->hashes[symbol] & (buckets - 1u);

        while (grown[slot] != D_SYMBOL_NONE)
        {
            slot = (slot + 1u) & (buckets - 1u);
        }

        grown[slot] = (uint32_t)symbol;
    }

    free(_table->buckets);
    _table->buckets     = grown;
    _table->bucket_mask = buckets - 1u;

    return true;
}


/**
 * @brief Finds a name's bucket slot: its symbol's, or the empty one it would
 *        take.
 *
 * @param[in] _table   the table, with a bucket array.
 * @param[in] _text    the name.
 * @param[in] _length  its length.
 * @param[in] _hash    its hash.
 * @return the slot.
 */
static size_t
d_internal_symbol_slot(
    const struct d_symbol_table* _table,
    const char*                  _text,
    size_t                       _length,
    uint32_t                     _hash
)
{
    size_t slot = _hash & _table->bucket_mask;

    while (_table->buckets[slot] != D_SYMBOL_NONE)
    {
        const uint32_t symbol = _table->buckets[slot];

        // hash and length first: text is compared only on a likely match
        if ( (_table->hashes[symbol] == _hash)    &&
             (_table->lengths[symbol] == _length) &&
             (memcmp(_table->text + _table->offsets[symbol],
                     _text,
                     _length) == 0) )
        {
            return slot;
        }

        slot = (slot + 1u) & _table->bucket_mask;
    }

    return slot;
}


/*
d_symbol_table_new
  An empty table allocates nothing until its first intern.
*/
struct d_symbol_table*
d_symbol_table_new(
    void
)
{
    return calloc(1u, sizeof(struct d_symbol_table));
}


/*
d_symbol_table_free
  Accepts NULL, as free does.
*/
void
d_symbol_table_free(
    struct d_symbol_table* _table
)
{
    if (_table)
    {
        free(_table->text);
        free(_table->offsets);
        free(_table->lengths);
        free(_table->hashes);
        free(_table->buckets);
        free(_table);
    }

    return;
}


/*
d_symbol_intern
  Grows the buckets before they pass two-thirds full, then the per-symbol
arrays and the arena as needed.  The arena keeps a terminator after each name
so d_symbol_text can hand out a C string.
*/
uint32_t
d_symbol_intern(
    struct d_symbol_table* _table,
    const char*            _text,
    size_t                 _length
)
{
    // parameter validation first
    if ( (!_table) || (!_text) || (_length >= UINT32_MAX) )
    {
        return D_SYMBOL_NONE;
    }

    // two thirds full is where linear probing starts to cluster
    if ( ( (_table->bucket_mask == 0u) ||
           (((_table->count + 1u) * 3u) > ((_table->bucket_mask + 1u) * 2u)) )
         && (!d_internal_symbol_rehash(_table)) )
    {
        return D_SYMBOL_NONE;
    }

    const uint32_t hash = d_internal_symbol_hash(_text, _length);
    const size_t   slot = d_internal_symbol_slot(_table, _text, _length, hash);

    if (_table->buckets[slot] != D_SYMBOL_NONE)
    {
        return _table->buckets[slot];
    }

    if (_table->count == _table->capacity)
    {
        const size_t grown    = (_table->capacity == 0u) ? 64u
                                                         : (_table->capacity * 2u);
        uint32_t* const offsets = realloc(_table->offsets,
                                          grown * sizeof(uint32_t));

        if (!offsets)
        {
            return D_SYMBOL_NONE;
        }

        _table->offsets = offsets;

        uint32_t* const lengths = realloc(_table->lengths,
                                          grown * sizeof(uint32_t));

        if (!lengths)
        {
            return D_SYMBOL_NONE;
        }

        _table->lengths = lengths;

        uint32_t* const hashes = realloc(_table->hashes,
                                         grown * sizeof(uint32_t));

        if (!hashes)
        {
            return D_SYMBOL_NONE;
        }

        _table->hashes   = hashes;
        _table->capacity = grown;
    }

    if ((_table->text_used + _length + 1u) > _table->text_capacity)
    {
        size_t grown = (_table->text_capacity == 0u) ? 1024u
                                                     : _table->text_capacity;

        while (grown < (_table->text_used + _length + 1u))
        {
            grown *= 2u;
        }

        char* const text = realloc(_table->text, grown);

        if (!text)
        {
            return D_SYMBOL_NONE;
        }

        _table->text          = text;
        _table->text_capacity = grown;
    }

    const uint32_t symbol = (uint32_t)_table->count;

    memcpy(_table->text + _table->text_used, _text, _length);
    _table->text[_table->text_used + _length] = '\0';

    _table->offsets[symbol] = (uint32_t)_table->text_used;
    _table->lengths[symbol] = (uint32_t)_length;
    _table->hashes[symbol]  = hash;
    _table->text_used      += _length + 1u;
    ++_table->count;

    _table->buckets[slot] = symbol;

    return symbol;
}


/*
d_symbol_find
  The same probe as an intern, with nothing added on a miss.
*/
uint32_t
d_symbol_find(
    const struct d_symbol_table* _table,
    const char*                  _text,
    size_t                       _length
)
{
    if ( (!_table) || (!_text) || (_table->bucket_mask == 0u) )
    {
        return D_SYMBOL_NONE;
    }

    const uint32_t hash = d_internal_symbol_hash(_text, _length);

    return _table->buckets[d_internal_symbol_slot(_table, _text, _length,
                                                  hash)];
}


/*
d_symbol_text
  "" rather than NULL for an unknown symbol, so a caller printing a name never
has to guard.
*/
const char*
d_symbol_text(
    const struct d_symbol_table* _table,
    uint32_t                     _symbol
)
{
    if ( (!_table) || (_symbol >= _table->count) )
    {
        return "";
    }

    return _table->text + _table->offsets[_symbol];
}


/*
d_symbol_count
  Symbols are dense, so this is also one past the largest.
*/
size_t
d_symbol_count(
    const struct d_symbol_table* _table
)
{
    return _table ? _table->count : 0u;
}
