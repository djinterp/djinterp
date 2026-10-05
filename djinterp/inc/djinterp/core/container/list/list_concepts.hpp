/*******************************************************************************
* djinterp [core]                                              list_concepts.hpp
*
* C++20 concepts for the abstract list module:
*   This header layers concept syntax over the SFINAE traits in
* list_traits.hpp.  Every trait there has a corresponding concept
* here, plus a small set of composite concepts for the most common
* constraint patterns.  The concepts allow `requires` clauses and
* shorthand template constraints in code that targets C++20+.
*
*   When concepts are unavailable (pre-C++20 or compilers without
* __cpp_concepts >= 201907L), the entire concept block is elided.
* The trait surface in list_traits.hpp remains the canonical
* dispatch mechanism — concepts are a syntactic convenience.
*
*   Concepts for specific list shapes (singly_linked / doubly_linked /
* xor_linked / skip-list) live in linked_list_concepts.hpp.  This
* header focuses on the abstract list contract that is shared by
* every list implementation.
*
*
* path:      /inc/djinterp/core/container/list/list_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    method-presence concepts
      ------------------------

II.   iterator-return-shape concepts
      ------------------------------

III.  composite list concepts
      -----------------------

IV.   capability concepts
      -------------------

V.    stability concepts
      ------------------
*/

#ifndef DJINTERP_CONTAINER_LIST_LIST_CONCEPTS_HPP
#define DJINTERP_CONTAINER_LIST_LIST_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"

#ifndef D_ENV_LANG_DETECTED_CPP
    #error "list_concepts.hpp requires C++ compilation"
#endif  // D_ENV_LANG_DETECTED_CPP

#include "./list_traits.hpp"

// below C++20 concepts this header is empty: a facility above its tier
// is absent, never an #error
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20


NS_DJINTERP


// ===========================================================================
// I.   METHOD-PRESENCE CONCEPTS
// ===========================================================================

// has_splice
//   concept: matches types exposing splice(const_iterator, T&).
template<typename Type>
concept has_splice = has_splice_method<Type>::value;

// has_splice_after
//   concept: matches types exposing splice_after(const_iterator, T&).
// Forward-list-style.
template<typename Type>
concept has_splice_after = has_splice_after_method<Type>::value;

// has_unique
//   concept: matches types exposing unique() (consecutive- duplicate
// collapse).
template<typename Type>
concept has_unique = has_unique_method<Type>::value;

// has_unique_pred
//   concept: matches types exposing unique(BinaryPred).
template<typename Type>
concept has_unique_pred = has_unique_pred_method<Type>::value;

// has_merge
//   concept: matches types exposing merge(T&).
template<typename Type>
concept has_merge = has_merge_method<Type>::value;

// has_merge_cmp
//   concept: matches types exposing merge(T&, BinaryCmp).
template<typename Type>
concept has_merge_cmp = has_merge_cmp_method<Type>::value;

// has_list_remove
//   concept: matches types exposing remove(value_type) — the list-style remove
// that erases ALL matches.
template<typename Type>
concept has_list_remove = has_list_remove_method<Type>::value;

// has_list_remove_if
//   concept: matches types exposing remove_if(UnaryPred).
template<typename Type>
concept has_list_remove_if = has_list_remove_if_method<Type>::value;

// has_list_sort
//   concept: matches types exposing sort() as a member.
template<typename Type>
concept has_list_sort = has_list_sort_method<Type>::value;

// has_list_sort_cmp
//   concept: matches types exposing sort(BinaryCmp) as a member.
template<typename Type>
concept has_list_sort_cmp = has_list_sort_cmp_method<Type>::value;

// has_list_reverse
//   concept: matches types exposing reverse() as a member.
template<typename Type>
concept has_list_reverse = has_list_reverse_method<Type>::value;


// ===========================================================================
// II.  ITERATOR-RETURN-SHAPE CONCEPTS
// ===========================================================================

// has_iter_erase
//   concept: matches types exposing erase(const_iterator) returning iterator.
template<typename Type>
concept has_iter_erase = has_iter_erase_method<Type>::value;

// has_iter_insert
//   concept: matches types exposing insert(const_iterator, value_type)
// returning iterator.
template<typename Type>
concept has_iter_insert = has_iter_insert_method<Type>::value;


// ===========================================================================
// III. COMPOSITE LIST CONCEPTS
// ===========================================================================

// list_container_type
//   concept: matches any container satisfying the list contract. The defining
// signal is splice (or splice_after for forward-list- shaped lists) — the
// operation that distinguishes lists from sequence containers.
template<typename Type>
concept list_container_type = is_list_container<Type>::value;

// list_like
//   concept: looser shorthand for list_container_type. Useful in auto-deduced
// `requires` clauses where the spelling is preferred.
template<typename Type>
concept list_like = is_list_container<Type>::value;

// full_list_type
//   concept: matches a list exposing the complete std::list operation set
// (splice, merge, sort, unique, remove, remove_if, reverse). Useful for code
// that wants to require std::list- equivalent capability.
template<typename Type>
concept full_list_type = is_full_list<Type>::value;


// ===========================================================================
// IV.  CAPABILITY CONCEPTS
// ===========================================================================

// spliceable_list_container
//   concept: list_container with a splice operation (trivially satisfied by
// every list_container_type).
template<typename Type>
concept spliceable_list_container = is_spliceable_list<Type>::value;

// mergeable_list_container
//   concept: list_container that exposes merge.
template<typename Type>
concept mergeable_list_container = is_mergeable_list<Type>::value;

// sortable_list_container
//   concept: list_container with a member sort().
template<typename Type>
concept sortable_list_container = is_sortable_list<Type>::value;

// unique_capable_list_container
//   concept: list_container with a member unique().
template<typename Type>
concept unique_capable_list_container =
    is_unique_capable_list<Type>::value;

// removable_list_container
//   concept: list_container with remove or remove_if.
template<typename Type>
concept removable_list_container = is_removable_list<Type>::value;

// reversible_list_container
//   concept: list_container with member reverse().
template<typename Type>
concept reversible_list_container = is_reversible_list<Type>::value;


// ===========================================================================
// V.   STABILITY CONCEPTS
// ===========================================================================

// node_stable_list_container
//   concept: list_container that opts in to the node-stability guarantee via
// the is_node_stable static bool.
template<typename Type>
concept node_stable_list_container =
    is_node_stable_list<Type>::value;


NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_LIST_LIST_CONCEPTS_HPP
