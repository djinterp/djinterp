/******************************************************************************
* djinterp [container]                                     sorted_container.hpp
*
*   The SORTEDNESS axis (the spec, Sortedness), layered on the Order axis.
* Sortedness presupposes positions: a container is SORTED with respect to a
* comparator iff it is ordered and, along its positions, comp(e_i, e_{i+1}) is
* non-descending; it is UNSORTED iff ordered but not in the comparator's order
* (it still has an order, just not that one).  For an UNORDERED container
* positional sortedness is NOT APPLICABLE - it has no positions to be sorted -
* yet a comparator-equipped one (a set, a map) admits a unique MONOTONE
* ENUMERATION, "sorted by construction" as an invariant of how it enumerates
* rather than a checkable property of stored positions.
*
*   At the type level the axis reads:
*     non_container    - not an (iterable) container;
*     unordered        - unordered, no comparator: positional sortedness N/A, no
*                        monotone enumeration (a hash-ordered set / map);
*     monotone         - unordered but comparator-equipped: sorted-by-construction
*                        enumeration (an ordered set / map / multiset / multimap);
*     order_dependent  - ordered: sortedness is a property of the INSTANCE, not of
*                        the type (a plain sequence may or may not be sorted); and
*     sorted           - ordered AND guaranteed in comparator order - a closed
*                        interval (arithmetic, monotone by construction) or a
*                        sequence that asserts a sorted invariant (opt-in).
*
*   For an order_dependent container the property is checkable at runtime, which
* is what is_sorted_range performs; for a monotone or sorted type, enumeration
* yields comparator order with no check needed.  A comparator is detected as a
* key_compare alias; a sorted sequence opts in through a static `sorted_invariant`
* constant; and interval bounds mark the arithmetic sorted case.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/sorted_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_SORTED_
#define DJINTERP_CONTAINER_SORTED_ 1

// std
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"              // clean_t, NS_*, feature macros
#include "../meta/trait_detect.hpp"     // D_TYPE_TRAIT_* detection macros, D_VOID_T
#include "./traits/sorted_container_traits.hpp"  // sortedness classifiers (canonical home)


NS_DJINTERP


// ===========================================================================
// I.   Sortedness classifiers
// ===========================================================================
//   The sortedness signals (has_key_compare / interval-bounds / sorted_invariant
// helpers), the sortedness enum + sortedness_of, and the type-level predicates
// is_sorted_container / is_unsorted_container / is_monotone_container /
// admits_sorted_enumeration are owned by sorted_container_traits.hpp (included
// above) and re-exported through it.  This file provides only the instance-level
// check (section IV) that the traits header defers here.  Earlier revisions
// carried a verbatim copy of the classifiers; the duplicates were removed to end
// the one-definition-rule conflict.


// ===========================================================================
// IV.  Instance-level sortedness (the checkable property)
// ===========================================================================

// is_sorted_range
//   function: for an ORDERED container - whose sortedness is a property of the
// instance - reports whether THIS container's elements are in non-descending
// order along their positions.  (For a monotone or sorted type no check is
// needed; enumeration is already in comparator order.)
template<typename _Container>
typename std::enable_if<
    is_ordered_container<_Container>::value,
    bool
>::type
is_sorted_range(
    const _Container& _container
)
{
    return std::is_sorted(std::begin(_container), std::end(_container));
}

// is_sorted_range (custom comparator)
//   function: as above, against a supplied comparator.
template<typename _Container,
         typename _Compare>
typename std::enable_if<
    is_ordered_container<_Container>::value,
    bool
>::type
is_sorted_range(
    const _Container& _container,
    _Compare          _cmp
)
{
    return std::is_sorted(std::begin(_container), std::end(_container), _cmp);
}


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================
//   sorted_container_class is owned by sorted_container_traits.hpp (included
// above); the duplicate that lived here was removed.


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_SORTED_
