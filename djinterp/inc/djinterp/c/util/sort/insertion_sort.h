/******************************************************************************
* djinterp [utility]                                          insertion_sort.h
*
*   Insertion sort: the sequential driver.
* In-place, iterative, stable, comparison-based.  Each element is placed into
* the ordered run growing behind it, so the sorted prefix extends by exactly
* one element per step until it is the whole range.
*
*   The driver is one call.  Everything it does is in insertion_sort_common.h,
* so a concurrent driver, or a small-range fallback inside quicksort, shares
* the placement and the sweep rather than restating them.
*
*   ONE SWEEP, NOT MANY.  Insertion and bubble move elements the same way --
* one place at a time, between adjacent neighbours -- and differ in when they
* stop.  Bubble sweeps until a sweep exchanges nothing; insertion sweeps once,
* because each step leaves the prefix fully ordered rather than only pushing
* one element to its end.  That, plus shifting rather than exchanging, is why
* insertion beats bubble on the same input despite sharing its complexity
* class.
*
*   complexity:
*     best:     O(n)        (already sorted -- one comparison per element,
*                            and no writes at all)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (in place; the lift is a stack block)
*     stable:   yes         (a candidate never travels past an equivalent)
*
*   WHEN TO REACH FOR IT.  The default choice among the quadratic sorts: it
* beats bubble and selection on nearly every input, it is stable, and it is
* what the O(n log n) algorithms fall back to on small sub-ranges, where its
* low constant factor wins outright.
*
*   COST OF THE ERASED INTERFACE.  Ordering is an indirect call and placement
* is width-driven, which together cost several times an insertion sort compiled
* against a known element type.  That is inherent to a C function generic over
* its element type; a caller in C++ who needs the difference should use
* insertion_sort.hpp, whose templates monomorphise.
*
*
* path:      /djinterp/c/util/sort/insertion_sort.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_INSERTION_
#define DJINTERP_UTILITY_SORT_INSERTION_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "./sort_common.h"
#include "./insertion_sort_common.h"


D_EXTERN_C_BEGIN


// I.     sorting
enum d_sort_status d_insertion_sort(void*                           _base,
                                    size_t                          _count,
                                    size_t                          _elem_size,
                                    const struct d_sort_comparator* _comparator,
                                    enum d_sort_order               _order);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_INSERTION_
