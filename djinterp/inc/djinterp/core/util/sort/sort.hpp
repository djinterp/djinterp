/******************************************************************************
* djinterp [utility]                                                  sort.hpp
*
*   djinterp portable sorting -- umbrella header.
* This is the single include that brings in the sort subsystem and its
* integration with the functional layer.  It re-exports:
*
*     - sort_common.hpp    : the shared core, which now pulls in the
*                            functional comparator algebra (natural, by_key,
*                            by_member, reversed, then, lifted) and the six
*                            algorithm headers' shared primitives.
*     - sort_dispatch.hpp  : the algorithm-agnostic dispatch facility -- the
*                            sorter type and the sort<tag>(...) / sort(algo, ...)
*                            entry points, every one of which accepts any model
*                            of is_comparator (so the whole comparator algebra
*                            drops straight in).
*     - sort_monoid.hpp    : sorted_run<T, Compare>, the merge monoid -- the
*                            sort subsystem's seat in the typeclass algebra.
*                            Two sorted runs combine (mappend) by merging, the
*                            empty run is the identity (mempty), and mconcat
*                            over a Foldable of singleton runs is a merge sort.
*
*   The view/dataflow bridge (sort_view.hpp) is deliberately NOT re-exported
* here.  It couples the sort subsystem to the (large) view subsystem, so it is
* left as an explicit opt-in: include sort_view.hpp directly to drain a lazy
* view pipeline into a sorted vector via `... | sorted_by(comp)`.
*
*   THE THREE SEAMS, in one place:
*     1. consume   -- sort takes the functional comparator algebra as its
*                     ordering argument                       (sort_common +
*                                                              sort_dispatch).
*     2. participate -- sort registers as a Semigroup / Monoid via sorted_run,
*                       so the algebra's mappend / mconcat / fold_monoid
*                       operate on it                          (sort_monoid).
*     3. connect   -- sort plugs into the lazy view pipeline as a terminal
*                     (opt-in)                                 (sort_view).
*
*   Everything here past the dispatch facility requires C++11 or later; the
* umbrella include itself is valid in every supported language mode (in older
* modes it contributes the C++98-clean parts of sort_common and the algorithm
* headers).
*
*   overview:
*     -- sort_common.hpp     (core types + comparator algebra)
*     -- sort_dispatch.hpp   (sorter + sort() entry points)
*     -- sort_monoid.hpp     (sorted_run merge monoid)
*     [opt-in] sort_view.hpp (lazy-pipeline sort terminal)
*
*
* path:      /inc/djinterp/core/util/sort/sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_FACADE_
#define DJINTERP_UTILITY_SORT_FACADE_ 1

// djinterp -- the integrated sort surface
#include "./sort_common.hpp"     // core types + functional comparator algebra
#include "./sort_dispatch.hpp"   // algorithm-agnostic dispatch facility
#include "./sort_monoid.hpp"     // sorted_run merge monoid (typeclass algebra)

//   sort_view.hpp (the lazy-pipeline sort terminal) is intentionally omitted;
// include it explicitly when bridging into the view subsystem.

#endif  // DJINTERP_UTILITY_SORT_FACADE_
