/******************************************************************************
* djinterp [parse]                                                   storage.c
*
*   Definitions for the non-inline declarations in storage.h.
*
*
* path:      /src/djinterp/parse/storage.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parse/storage.h"  // corresponding header


#if (D_INTERNAL_PARSE_HEAP == 1)

// std
#include <stdlib.h>  // realloc
#include <string.h>  // memset


// D_INTERNAL_STORAGE_FLOOR
//   macro: the capacity growth starts from, so a container that begins empty
// does not spend its first few appends reallocating.
#define D_INTERNAL_STORAGE_FLOOR    16u


/*
d_parse_grow
  Ensures an array can hold a given number of elements, reallocating if it must.
NOTE:
  The new tail is zeroed. The operator registry depends on that -- an undefined
opcode must read as a hole, not as whatever the allocator handed back -- and
making it part of the contract rather than one caller's afterthought removes a
class of bug from every container that grows.
CAUTION:
  Growth doubles, so the caller's own pointers into the old storage are invalid
afterwards. Every container here stores offsets rather than interior pointers
for exactly this reason.

Parameter(s):
  _data:     the current storage; may be NULL for a container that has none.
  _capacity: in, the elements the storage holds; out, what it holds afterwards.
  _needed:   the elements it must hold; must be at least 1.
  _stride:   the size of one element in bytes.
  _owned:    non-zero when this storage was allocated by its container, and may
             therefore be reallocated. Caller-supplied storage never is.
Pre-condition(s):
  - _capacity is not NULL and _needed is not 0, so NULL unambiguously means
    failure rather than "nothing to do".
Return:
  The storage to use -- the same pointer when it was already large enough, a
new one when it grew -- or NULL when the storage is not growable, the
requirement overflows, or the allocator refused.
*/
void*
d_parse_grow(
    void*     _data,
    uint32_t* _capacity,
    uint32_t  _needed,
    uint32_t  _stride,
    int       _owned
)
{
    // reject the argument shapes that would make NULL ambiguous
    if ( (!_capacity)    ||
         (_needed == 0u) ||
         (_stride == 0u) )
    {
        return NULL;
    }

    // the common case: it already fits, and nothing needs to happen
    if (*_capacity >= _needed)
    {
        return _data;
    }

    // caller storage is fixed by contract; refusing is the whole point of the
    // ownership flag
    if (!_owned)
    {
        return NULL;
    }

    uint32_t capacity = (*_capacity > 0u)
                        ? *_capacity
                        : (uint32_t)D_INTERNAL_STORAGE_FLOOR;

    // double until the requirement fits, so repeated appends stay amortised
    while (capacity < _needed)
    {
        // refuse rather than wrap; a container this large is a runaway, and
        // the caller's diagnostic will say so more usefully than a crash
        if (capacity > (0xFFFFFFFFu / 2u))
        {
            return NULL;
        }

        capacity *= 2u;
    }

    // refuse a byte count that would not fit a size_t on a small target
    if (capacity > (0xFFFFFFFFu / _stride))
    {
        return NULL;
    }

    void* grown = realloc(_data, (size_t)capacity * (size_t)_stride);

    // check if memory allocation was successful
    if (!grown)
    {
        return NULL;
    }

    // zero the tail, so a freshly grown slot reads as absent rather than as
    // whatever the allocator last left there
    memset((char*)grown + ((size_t)(*_capacity) * (size_t)_stride),
           0,
           (size_t)(capacity - *_capacity) * (size_t)_stride);

    *_capacity = capacity;

    return grown;
}

#endif  // D_INTERNAL_PARSE_HEAP
