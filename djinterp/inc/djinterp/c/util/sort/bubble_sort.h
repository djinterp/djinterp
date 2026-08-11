/******************************************************************************
* djinterp [utility]                                             bubble_sort.h
*
*   Bubble sort: the sequential driver.
* In-place, iterative, stable, comparison-based.  A pass sweeps the adjacent
* pairs of the unsorted region and exchanges those out of order, which carries
* the largest remaining element to the region's end; the sorted suffix grows by
* at least one element per pass until the whole range is ordered.
*
*   The driver is three lines of loop.  Everything it does is in
* bubble_sort_common.h, so a concurrent driver added later shares the pass and
* the compare-exchange rather than restating them.
*
*   THE SHRINKING BOUND.  The next pass stops at the index of the previous
* pass's LAST exchange, not one short of where that pass stopped.  Nothing at
* or after that index moved, so it is already in final position.  A
* nearly-sorted range therefore converges in far fewer passes, and an ordered
* one in exactly one.
*
*   complexity:
*     best:     O(n)        (already sorted -- one pass, no exchanges)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (in place; the exchange temporary is a stack block)
*     stable:   yes         (only a STRICT precedence exchanges a pair)
*
*   WHEN TO REACH FOR IT.  Almost never on size alone -- quadratic is
* quadratic.  It earns its place on ranges already nearly ordered, where the
* shrinking bound makes it linear and beats an algorithm that cannot exploit
* existing order, and on ranges small enough that the constant factor decides.
*
*   COST OF THE ERASED INTERFACE.  Ordering is an indirect call and exchange is
* width-driven, which together cost roughly 3x a bubble sort compiled against a
* known element type.  That is inherent to a C function generic over its
* element type; a caller in C++ who needs the difference should use
* bubble_sort.hpp, whose templates monomorphise.
*
*
* path:      /djinterp/c/util/sort/bubble_sort.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.07
*                                                         revised: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_
#define DJINTERP_UTILITY_SORT_BUBBLE_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "./sort_common.h"
#include "./bubble_sort_common.h"


D_EXTERN_C_BEGIN


// I.     sorting
enum d_sort_status d_bubble_sort(void*                           _base,
                                 size_t                          _count,
                                 size_t                          _elem_size,
                                 const struct d_sort_comparator* _comparator,
                                 enum d_sort_order               _order);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_
