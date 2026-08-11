/******************************************************************************
* djinterp [utility]                                          selection_sort.h
*
*   Selection sort: the sequential driver.
* In-place, iterative, comparison-based, NOT stable.  Each position takes the
* minimum of everything that remains, exchanged in from wherever it was found;
* the sorted prefix extends by exactly one element per step.
*
*   The driver is one call.  Everything it does is in selection_sort_common.h,
* so a concurrent driver -- which parallelises the SCAN rather than the sort --
* shares the search and the sweep rather than restating them.
*
*   CHOOSE IT FOR THE WRITES, NEVER FOR THE COMPARISONS.  It always performs
* n(n-1)/2 comparisons, so insertion sort beats it on every input, ordered or
* not.  What it offers is at most n-1 exchanges, one per position, no matter
* how disordered the input: an element travelling from the end of the array to
* the front costs one exchange, where bubble and insertion would touch every
* slot between.  That trade pays only when elements are large and comparison is
* cheap.
*
*   NOT STABLE.  Exchanging the winner into place evicts whatever was there to
* the winner's old slot, past any number of equal elements.  If stability
* matters, use insertion sort, which is both stable and faster; making this
* algorithm stable requires rotating instead of exchanging, which spends the
* writes that were the entire reason to choose it.
*
*   complexity:
*     best:       O(n^2)    (the scan cannot be cut short)
*     average:    O(n^2)
*     worst:      O(n^2)
*     exchanges:  O(n)      (at most n-1; the reason to choose it)
*     space:      O(1)      (in place; the exchange temporary is a stack block)
*     stable:     no
*
*   COST OF THE ERASED INTERFACE.  Ordering is an indirect call, and this
* algorithm is nothing but comparisons, so the erased path pays for that more
* heavily than the others do.  A caller in C++ should use selection_sort.hpp,
* whose templates monomorphise.
*
*
* path:      /djinterp/c/util/sort/selection_sort.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_SELECTION_
#define DJINTERP_UTILITY_SORT_SELECTION_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "./sort_common.h"
#include "./selection_sort_common.h"


D_EXTERN_C_BEGIN


// I.     sorting
enum d_sort_status d_selection_sort(void*                           _base,
                                    size_t                          _count,
                                    size_t                          _elem_size,
                                    const struct d_sort_comparator* _comparator,
                                    enum d_sort_order               _order);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_SELECTION_
