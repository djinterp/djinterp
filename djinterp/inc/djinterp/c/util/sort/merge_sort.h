/******************************************************************************
* djinterp [utility]                                              merge_sort.h
*
*   Merge sort: the sequential driver.
* Stable, comparison-based, O(n log n) in every case -- no input defeats it,
* which is what separates it from quicksort.  It is the only algorithm in the
* subsystem that is not in place, and the only one whose entry point can fail
* for a reason that is not a caller error.
*
*   THE TWO-CALL PROTOCOL.  Nothing here allocates, so the caller supplies the
* scratch:
*
*       size_t bytes = d_merge_sort_scratch_size(count, sizeof(item));
*       void*  work  = malloc(bytes);
*       enum d_sort_status s = d_merge_sort(items, count, sizeof(item),
*                                           &ordering, D_SORT_ORDER_ASCENDING,
*                                           work, bytes);
*
*   Measure, then sort.  This is where the status enum's split earns itself: a
* buffer that is too small returns D_SORT_STATUS_BUFFER_TOO_SMALL, which is
* MECHANICAL -- the request was well formed and the machinery ran short, so the
* caller can supply more and retry.  Every other failure is FORMAL and means
* the request was never sortable.  A caller that distinguishes them with
* D_SORT_STATUS_IS_MECHANICAL can retry the first and must not retry the
* second.
*
*   THE SCRATCH MUST BE ALIGNED FOR THE ELEMENT TYPE.  Elements are copied into
* it and then handed to the comparator, which dereferences them as their real
* type.  Storage from malloc is suitably aligned for any type and is always
* safe; an array of the element type is safe; a `char` array on the stack is
* NOT, and is the one way to get this wrong.
*
*   complexity:
*     best:       O(n)      comparisons (ordered input -- ordered run pairs
*                           are detected and copied rather than merged)
*     average:    O(n log n)
*     worst:      O(n log n)   (guaranteed)
*     space:      O(n)      auxiliary, supplied by the caller
*     stable:     yes
*
*   WHEN TO REACH FOR IT.  When the worst case matters -- it has no bad input,
* where quicksort does -- or when stability is required at O(n log n), which no
* other algorithm here offers.  The price is the buffer.
*
*
* path:      /djinterp/c/util/sort/merge_sort.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_MERGE_
#define DJINTERP_UTILITY_SORT_MERGE_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "./sort_common.h"
#include "./merge_sort_common.h"


D_EXTERN_C_BEGIN


// I.     sorting
size_t             d_merge_sort_scratch_size(size_t _count,
                                             size_t _elem_size);
enum d_sort_status d_merge_sort(void*                           _base,
                                size_t                          _count,
                                size_t                          _elem_size,
                                const struct d_sort_comparator* _comparator,
                                enum d_sort_order               _order,
                                void*                           _scratch,
                                size_t                          _scratch_size);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_MERGE_
