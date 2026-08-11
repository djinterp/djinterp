/******************************************************************************
* djinterp [utility]                                      bubble_sort_common.h
*
*   The primitives every bubble sort is built from, sequential or concurrent.
* A bubble sort is a driver wrapped around two operations: exchange an adjacent
* pair if it is out of order, and sweep a run of adjacent pairs doing so.  What
* distinguishes one bubble sort from another is only which pairs get swept, in
* what order, and by how many threads.  Those two operations live here so that
* the drivers do not each carry a copy.
*
*   WHAT THE DRIVERS DO WITH THEM:
*
*     - The sequential driver (bubble_sort.h) sweeps [1, end) repeatedly,
*       shrinking `end` to the last exchange each time.
*
*     - A block-partitioned concurrent driver gives each worker a sub-range and
*       calls d_bubble_pass on it, then reconciles the boundaries.
*
*     - An odd-even transposition driver -- the standard parallel bubble sort
*       -- alternates two phases: pairs (0,1),(2,3),... then (1,2),(3,4),...
*       Within a phase every pair is disjoint, so the pairs can be exchanged in
*       any order or all at once, which is what makes the algorithm parallel.
*       That driver uses d_bubble_compare_exchange directly and never needs a
*       sweep at all.
*
*   THE ATOM IS WHERE STABILITY LIVES.  d_bubble_compare_exchange exchanges
* only on a STRICT precedence, so equivalent elements are never moved past one
* another.  Every driver built on it inherits that, and any driver that
* open-codes the comparison instead may not -- which is the practical reason
* the atom is a function and not advice.
*
*   complexity of one pass:
*     comparisons: exactly (end - begin)
*     exchanges:   0 to (end - begin)
*     space:       O(1)
*
*
* path:      /djinterp/c/util/sort/bubble_sort_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_COMMON_
#define DJINTERP_UTILITY_SORT_BUBBLE_COMMON_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "../swap.h"
#include "./sort_common.h"


D_EXTERN_C_BEGIN


// I.     properties

// D_BUBBLE_SORT_IS_STABLE
//   macro: 1 -- only a strict precedence exchanges a pair, so equivalent
// elements never move past one another.
#define D_BUBBLE_SORT_IS_STABLE     1

// D_BUBBLE_SORT_IS_IN_PLACE
//   macro: 1 -- O(1) auxiliary storage, none of it allocated.
#define D_BUBBLE_SORT_IS_IN_PLACE   1

// D_BUBBLE_SORT_IS_ADAPTIVE
//   macro: 1 -- a sweep reports where it last exchanged, which lets a driver
// shrink its bound and finish ordered input in one pass.
#define D_BUBBLE_SORT_IS_ADAPTIVE   1


// II.    sweeping a run of pairs
size_t d_bubble_pass(void*                           _base,
                     size_t                          _begin,
                     size_t                          _end,
                     size_t                          _elem_size,
                     const struct d_sort_comparator* _comparator,
                     enum d_sort_order               _order);


// III.   the adjacent pair

/*
d_bubble_compare_exchange
  Exchanges the pair at `_earlier` and `_later` when the later element strictly
precedes the earlier one.
  The atom of every bubble sort. STRICT precedence is what makes the family
stable: an exchange on equivalence would reorder equal elements, so this is the
one comparison no driver should open-code.
  Inline because it is the innermost operation of every driver; the exchange it
delegates to is not, since that is a loop of its own.

Parameter(s):
  _earlier:      the earlier element of the pair; never NULL.
  _later:        the later element; never NULL.
  _elem_size: element width in bytes.
  _comparator:   the ordering; must be non-NULL with a non-NULL compare.
  _order:        the direction; NONE behaves as ascending.
Return:
  true when the pair was out of order and has been exchanged.
*/
D_STATIC_INLINE bool
d_bubble_compare_exchange
(
    void*                           _earlier,
    void*                           _later,
    size_t                          _elem_size,
    const struct d_sort_comparator* _comparator,
    enum d_sort_order               _order
)
{
    // out of order only when the LATER element strictly precedes the earlier
    if (!d_sort_precedes(_comparator, _order, _later, _earlier))
    {
        return false;
    }

    d_memswap(_earlier, _later, _elem_size);

    return true;
}


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_COMMON_
