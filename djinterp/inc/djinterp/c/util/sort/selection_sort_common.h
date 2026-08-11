/******************************************************************************
* djinterp [utility]                                    selection_sort_common.h
*
*   The primitives every selection sort is built from.
* A selection sort is a driver wrapped around two operations: find the element
* that belongs first in a run, and exchange it into the run's front.  What
* distinguishes one selection sort from another is only which runs get scanned
* and by how many threads.
*
*   WHAT THE DRIVERS DO WITH THEM:
*
*     - The sequential driver (selection_sort.h) scans [i, count) for each i,
*       exchanging the winner into position i.
*
*     - A concurrent driver parallelises the SCAN, not the sort: the search for
*       a run's minimum is a reduction, so each worker takes a chunk, reports
*       its chunk's winner, and one comparison per chunk settles the overall
*       winner.  That is why the search is a separate published primitive with
*       its own range rather than an inner loop of the pass -- it is the piece
*       that partitions.
*
*   WHY THIS ALGORITHM EXISTS: MINIMAL WRITES.  Selection sort always performs
* n(n-1)/2 comparisons, which is worse than insertion sort on every input.
* What it buys is exchanges: at most n-1 of them, one per position, regardless
* of how disordered the input is.  Bubble and insertion move an element k
* places by touching k slots; selection moves it in one exchange no matter how
* far it travels.  When elements are large and comparison is cheap -- and only
* then -- that trade is worth making.
*
*   THIS FAMILY IS NOT STABLE, AND THAT CHANGES WHAT AGREEMENT COSTS.
* Exchanging the winner into position i evicts whatever was at i to the
* winner's old slot, arbitrarily far away, past any number of elements equal to
* it.  No scan order fixes this; it is the exchange itself.
*
*   The consequence is not academic.  For bubble and insertion, "sorted and
* stable" describes exactly one arrangement, so the C and C++ modules agree no
* matter how each is written.  Here the sorted arrangement is not unique, so
* agreement is a discipline rather than a consequence, and it rests on two
* choices that must be made the same way in both:
*
*     1. THE SCAN RUNS FORWARD, from the run's front toward its end.
*     2. A CANDIDATE MUST STRICTLY PRECEDE the incumbent to displace it, so
*        among equal elements the EARLIEST wins.
*
*   Both are normative for every implementation in this family, in either
* language.  Relaxing (2) to "precedes or equals" is a one-character change
* that still sorts correctly and produces a different array.
*
*   If a caller needs stability, the fix is not to patch this algorithm -- a
* stable selection sort must rotate the run rather than exchange, which spends
* the O(n) writes that were the entire reason to choose it.  Use insertion
* sort, which is stable and faster besides.
*
*   complexity:
*     best:       O(n^2)    (the scan cannot be cut short)
*     average:    O(n^2)
*     worst:      O(n^2)
*     exchanges:  O(n)      (at most n-1; the reason to choose it)
*     space:      O(1)
*     stable:     no
*
*
* path:      /djinterp/c/util/sort/selection_sort_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_SELECTION_COMMON_
#define DJINTERP_UTILITY_SORT_SELECTION_COMMON_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "../swap.h"
#include "./sort_common.h"


D_EXTERN_C_BEGIN


// I.     properties

// D_SELECTION_SORT_IS_STABLE
//   macro: 0 -- exchanging the winner into place evicts the element that was
// there to an arbitrary slot, past any number of elements equal to it.
#define D_SELECTION_SORT_IS_STABLE     0

// D_SELECTION_SORT_IS_IN_PLACE
//   macro: 1 -- O(1) auxiliary storage, none of it allocated.
#define D_SELECTION_SORT_IS_IN_PLACE   1

// D_SELECTION_SORT_IS_ADAPTIVE
//   macro: 0 -- the scan must see every remaining element to know the run's
// minimum, so ordered input costs exactly what reversed input costs.
#define D_SELECTION_SORT_IS_ADAPTIVE   0


// II.    selection
size_t d_selection_min_index(const void*                     _base,
                             size_t                          _begin,
                             size_t                          _end,
                             size_t                          _elem_size,
                             const struct d_sort_comparator* _comparator,
                             enum d_sort_order               _order);
void   d_selection_pass(void*                           _base,
                        size_t                          _begin,
                        size_t                          _end,
                        size_t                          _elem_size,
                        const struct d_sort_comparator* _comparator,
                        enum d_sort_order               _order);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_SELECTION_COMMON_
