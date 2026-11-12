/*******************************************************************************
* djinterp [cpp]                                                        sort.hpp
*
*   The sort subsystem in one include, with its seams into the functional
* layer.  It brings in:
*
*     - functional/comparator.hpp : the comparator algebra -- natural, by_key,
*                                   by_member, by_function, reversed, then,
*                                   lifted -- whose every model drops into
*                                   every comparator parameter below
*     - sort_common.hpp           : sort_order and internal::DOrderComparator
*     - sort_dispatch.hpp         : the six algorithm headers, and over them
*                                   the algorithm-agnostic DSorter and sort
*     - sort_monoid.hpp           : DSortedRun, the merge monoid
*
*   sort_view.hpp, the bridge to the lazy view pipeline, is deliberately NOT
* included.  It couples the sort subsystem to the view subsystem, which is
* large, so it is opt-in: include it to drain a pipeline into a sorted vector
* with `... | sorted_by(comp)`.
*
*   THE THREE SEAMS, in one place:
*     1. consume      sort takes the comparator algebra as its ordering
*                     (comparator.hpp, sort_dispatch.hpp)
*     2. participate  sort is a Semigroup and a Monoid through DSortedRun, so
*                     mappend, mconcat and fold_monoid work on it
*                     (sort_monoid.hpp)
*     3. connect      sort is a terminal of the lazy view pipeline -- opt-in
*                     (sort_view.hpp)
*
*   DIALECTS.  The include is valid from C++98.  Before C++11 it contributes
* the six algorithms' three-argument and _ordered entry points, sort_order and
* the comparator algebra's C++98 part; the dispatch facility and the monoid
* need C++11.
*
*
* path:      /inc/djinterp/cpp/util/sort/sort.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_UTIL_SORT_HPP
#define DJINTERP_UTIL_SORT_HPP 1

// djinterp
#include "../../functional/comparator.hpp"  // the comparator algebra
#include "./sort_common.hpp"                // sort_order
#include "./sort_dispatch.hpp"              // the algorithms, DSorter, sort
#include "./sort_monoid.hpp"                // DSortedRun, the merge monoid

//   sort_view.hpp is opt-in; include it directly to bridge into the view
// subsystem.


#endif  // DJINTERP_UTIL_SORT_HPP
