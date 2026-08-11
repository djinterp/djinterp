/******************************************************************************
* djinterp [utility]                                    insertion_sort_common.h
*
*   The primitives every insertion sort is built from, sequential or otherwise.
* An insertion sort is a driver wrapped around two operations: place one
* element into the ordered run behind it, and do that for each element of a
* run.  What distinguishes one insertion sort from another is only which runs
* get built, in what order, and by how many threads.
*
*   WHAT THE DRIVERS DO WITH THEM:
*
*     - The sequential driver (insertion_sort.h) places every element of
*       [0, count) into the run growing behind it.  One sweep is the whole
*       sort, which is the difference between this family and the bubble
*       family: both move elements one place at a time between adjacent
*       neighbours, but bubble needs O(n) sweeps to finish and insertion needs
*       exactly one.
*
*     - A block-partitioned concurrent driver gives each worker a sub-range and
*       calls d_insertion_pass on it, then merges the ordered blocks.  Sorting
*       a block is the standard first phase of a parallel merge sort, and it is
*       the reason this is a range operation rather than a whole-array one.
*
*     - A small-range fallback inside quicksort or merge sort calls
*       d_insertion_pass on the sub-range it has stopped subdividing.
*
*   PLACEMENT SHIFTS; IT DOES NOT SWAP.  This is what distinguishes insertion
* sort from bubble sort in cost rather than in shape.  Bubble moves an element
* k places with k exchanges, so 3k element writes; insertion lifts the element
* out once, moves the block over it, and puts it back, so k+2.  On a range
* where every element travels far, that is most of the difference between the
* two algorithms.
*
*   The lift needs somewhere to hold the element, and the width is a run-time
* value here, so d_insertion_place holds it in a fixed stack block when it fits
* and falls back to a chain of exchanges when it does not.  Nothing is
* allocated on either path, and the two produce the same arrangement.
*
*   STABILITY LIVES IN THE SCAN.  Placement stops at the first element the
* candidate does NOT strictly precede, so a candidate never travels past an
* equivalent element and equal elements keep their input order.  Every driver
* built on it inherits that.
*
*   complexity of one sweep over m elements:
*     comparisons: m-1 (ordered input) to m(m-1)/2 (reversed)
*     writes:      0   (ordered input) to m(m-1)/2 + 2(m-1)
*     space:       O(1)
*
*
* path:      /djinterp/c/util/sort/insertion_sort_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_INSERTION_COMMON_
#define DJINTERP_UTILITY_SORT_INSERTION_COMMON_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "../swap.h"
#include "./sort_common.h"


D_EXTERN_C_BEGIN


// I.     properties

// D_INSERTION_SORT_IS_STABLE
//   macro: 1 -- placement stops at the first element the candidate does not
// strictly precede, so equivalent elements keep their input order.
#define D_INSERTION_SORT_IS_STABLE     1

// D_INSERTION_SORT_IS_IN_PLACE
//   macro: 1 -- O(1) auxiliary storage, none of it allocated.
#define D_INSERTION_SORT_IS_IN_PLACE   1

// D_INSERTION_SORT_IS_ADAPTIVE
//   macro: 1 -- an element already in order relative to its predecessor is
// left alone, so ordered input costs one comparison per element and no writes.
#define D_INSERTION_SORT_IS_ADAPTIVE   1

// D_INSERTION_HOLD_SIZE
//   macro: the largest element d_insertion_place lifts into a stack block
// while shifting the run over it. Wider elements are placed by a chain of
// exchanges instead, which needs no storage at all and reaches the same
// arrangement for more writes.
#define D_INSERTION_HOLD_SIZE          256


// II.    placement
size_t d_insertion_place(void*                           _base,
                         size_t                          _lower,
                         size_t                          _index,
                         size_t                          _elem_size,
                         const struct d_sort_comparator* _comparator,
                         enum d_sort_order               _order);
void   d_insertion_pass(void*                           _base,
                        size_t                          _begin,
                        size_t                          _end,
                        size_t                          _elem_size,
                        const struct d_sort_comparator* _comparator,
                        enum d_sort_order               _order);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_INSERTION_COMMON_
