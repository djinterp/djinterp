/******************************************************************************
* djinterp [utility]                                        merge_sort_common.h
*
*   The primitives every merge sort is built from.
* A merge sort is a driver wrapped around two operations: merge two adjacent
* ordered runs into one, and sweep a whole range doing that at a fixed run
* width.  What distinguishes one merge sort from another is only how the runs
* are chosen and by how many threads.
*
*   BOTTOM-UP, NOT RECURSIVE.  Runs start at width 1 -- every single element is
* trivially ordered -- and each pass doubles the width until one run covers the
* range.  This costs the same comparisons as the recursive formulation and
* buys two things: no call stack at all, which matters where stack depth is
* budgeted, and a shape that partitions.  Within a pass the run pairs are
* DISJOINT, so they may be merged in any order or all at once, which is why a
* concurrent driver hands each worker a slice of the same pass rather than a
* branch of a recursion tree.  It is also the formulation sort_monoid.hpp
* describes: mconcat over a foldable of singleton runs IS this loop.
*
*   THE SOURCE AND DESTINATION ARE SEPARATE, AND THE DRIVER SWAPS THEM.
* A merge cannot write into the range it is reading, so each pass reads one
* buffer and writes the other; the driver exchanges their roles between passes
* and copies back at the end if the result landed in the scratch.  Merging into
* scratch and copying back every pass would be correct and would cost twice the
* copying, which is why these take a destination rather than assuming one.
*
*   STABILITY IS ONE COMPARISON'S DIRECTION.  The merge takes from the right
* run only when the right element STRICTLY precedes the left one; on
* equivalence it takes from the left.  Since the left run holds the earlier
* elements, equal elements keep their input order.  Reversing that test is a
* one-character change that still merges correctly and destroys stability.
*
*   ALREADY-ORDERED RUN PAIRS ARE COPIED, NOT MERGED.  If the first element of
* the right run does not precede the last element of the left, every element of
* the left run belongs before every element of the right, and the merge is a
* single block copy.  One comparison replaces a whole span of them, so a range
* that arrives ordered costs O(n) comparisons in total instead of O(n log n) --
* and in the erased C path, where every comparison is an indirect call, that is
* the difference between the algorithms.  The copies still happen, so the pass
* count is unchanged.
*
*   complexity:
*     best:       O(n)      comparisons (ordered input; see above)
*     average:    O(n log n)
*     worst:      O(n log n)   (guaranteed -- no input defeats it)
*     copies:     O(n log n)
*     space:      O(n)      auxiliary, supplied by the caller
*     stable:     yes
*
*
* path:      /djinterp/c/util/sort/merge_sort_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_MERGE_COMMON_
#define DJINTERP_UTILITY_SORT_MERGE_COMMON_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "./sort_common.h"


D_EXTERN_C_BEGIN


// I.     properties

// D_MERGE_SORT_IS_STABLE
//   macro: 1 -- the merge takes from the right run only on a strict
// precedence, so equal elements keep their input order.
#define D_MERGE_SORT_IS_STABLE     1

// D_MERGE_SORT_IS_IN_PLACE
//   macro: 0 -- the only algorithm in the subsystem that needs auxiliary
// storage, one element per element of the range.
#define D_MERGE_SORT_IS_IN_PLACE   0

// D_MERGE_SORT_IS_ADAPTIVE
//   macro: 1 -- in comparisons only. An ordered run pair is detected in one
// comparison and copied rather than merged, so ordered input costs O(n)
// comparisons instead of O(n log n). The copies are unaffected.
#define D_MERGE_SORT_IS_ADAPTIVE   1


// II.    merging
void   d_merge_runs(const void*                     _src,
                    void*                           _dst,
                    size_t                          _begin,
                    size_t                          _mid,
                    size_t                          _end,
                    size_t                          _elem_size,
                    const struct d_sort_comparator* _comparator,
                    enum d_sort_order               _order);
void   d_merge_pass(const void*                     _src,
                    void*                           _dst,
                    size_t                          _count,
                    size_t                          _width,
                    size_t                          _elem_size,
                    const struct d_sort_comparator* _comparator,
                    enum d_sort_order               _order);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_MERGE_COMMON_
