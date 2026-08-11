#include "./swap.h"

// std
#include <stdint.h>
#include <string.h>


// D_INTERNAL_SWAP_BLOCK
//   macro: the stack block wide objects are exchanged through. Sized to be
// worth a vectorised copy without making the frame notable; the exchange
// loops rather than requiring an object to fit, so no size is too large.
#define D_INTERNAL_SWAP_BLOCK   64


/*
d_memswap
  Exchanges the contents of two objects of `_size` bytes.
  The two most common widths are handled first and whole: a caller reordering
scalars or pointers spends real time here, and a memcpy of a constant size is a
pair of register moves rather than a call. Everything else moves a block at a
time so the copy stays word-sized without allocating.
  Aliased operands are a no-op rather than an error; exchanging an object with
itself is wasteful, not wrong. Overlapping-but-distinct operands are undefined,
as they are for memcpy.

Parameter(s):
  _first:  first object; must be non-NULL and `_size` bytes readable/writable.
  _second: second object; same requirements. May equal `_first`.
  _size:   the size of each object in bytes. Zero is a no-op.
Return:
  none.
*/
void
d_memswap
(
    void*  _first,
    void*  _second,
    size_t _size
)
{
    char*    left;
    char*    right;
    size_t   remaining;
    char     block[D_INTERNAL_SWAP_BLOCK];
    uint32_t word32;
    uint64_t word64;

    // nothing to exchange, or nothing to exchange it with
    if ( (!_first)          ||
         (!_second)         ||
         (_first == _second) )
    {
        return;
    }

    // the widths that dominate: a 32-bit scalar
    if (_size == sizeof(uint32_t))
    {
        memcpy(&word32, _first,  sizeof(uint32_t));
        memcpy(_first,  _second, sizeof(uint32_t));
        memcpy(_second, &word32, sizeof(uint32_t));

        return;
    }

    // ...and a 64-bit scalar or a pointer
    if (_size == sizeof(uint64_t))
    {
        memcpy(&word64, _first,  sizeof(uint64_t));
        memcpy(_first,  _second, sizeof(uint64_t));
        memcpy(_second, &word64, sizeof(uint64_t));

        return;
    }

    left      = (char*)_first;
    right     = (char*)_second;
    remaining = _size;

    // wide objects: a block at a time, so the copy stays word-sized
    while (remaining >= D_INTERNAL_SWAP_BLOCK)
    {
        memcpy(block, left,  D_INTERNAL_SWAP_BLOCK);
        memcpy(left,  right, D_INTERNAL_SWAP_BLOCK);
        memcpy(right, block, D_INTERNAL_SWAP_BLOCK);

        left      += D_INTERNAL_SWAP_BLOCK;
        right     += D_INTERNAL_SWAP_BLOCK;
        remaining -= D_INTERNAL_SWAP_BLOCK;
    }

    // the tail, if the size is not a whole number of blocks
    if (remaining != 0)
    {
        memcpy(block, left,  remaining);
        memcpy(left,  right, remaining);
        memcpy(right, block, remaining);
    }

    return;
}
