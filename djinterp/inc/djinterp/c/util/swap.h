/*******************************************************************************
* djinterp [c]                                                            swap.h
*
*   Byte-wise exchange of two objects of equal size.
* The operation memcpy leaves out: a swap is neither a copy nor a move, and
* every generic container operation that reorders elements needs one.  It lives
* here rather than in the sort subsystem because sorting is not the only caller
* -- partitioning, rotation, reversal, shuffling and heap sift-down all
* exchange without copying.
*
*   The common widths are exchanged whole, since a caller reordering an array
* of scalars or pointers spends a visible fraction of its time here and a
* fixed-size memcpy compiles to a pair of register moves.  Anything else moves
* through a fixed stack block, which keeps the copy word-sized, allocates
* nothing, and loops rather than requiring an object to fit.
*
*
* path:      /inc/djinterp/c/util/swap.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.10
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_UTIL_SWAP_H
#define DJINTERP_C_UTIL_SWAP_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"


D_EXTERN_C_BEGIN


// I.     byte-wise exchange
void    d_memswap(void*  _first,
                  void*  _second,
                  size_t _size);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_SWAP_H
