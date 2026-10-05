/*******************************************************************************
* djinterp [core]                                  sorted_container_concepts.hpp
*
* C++20 concepts for the SORTEDNESS axis -- the `requires`-facing view of
* sorted_container_traits.hpp.
*
*   THE CONCEPTS ADD NO POLICY.
*   Each is exactly its trait, spelled so it can constrain a template instead
* of gating one through enable_if. The trait stays the single source of truth.
*
*   NAMES.
*   Where the obvious name is taken by a CONTAINER CLASS in this namespace,
* the concept takes an adjective form instead. A concept and a class of the
* same name in one namespace is a hard redeclaration, and this framework has
* already been bitten by that three times.
*
*   PORTABILITY:
*   Gated on C++20 + concepts.
*
*
* path:      /inc/djinterp/core/container/concepts/sorted_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_SORTED_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_SORTED_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/sorted_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE PROMISE THE TYPE MAKES
// ==========================================================================


// SortedEnumerable
//   concept: enumerating it is GUARANTEED to yield comparator order --
// monotone (keyed, comparator-equipped) or sorted (positional,
// invariant-held).
// This is the property that lets a container be PRESENTED in order without
// sorting it, and it is the whole point of a radix tree.
template<typename Type>
concept SortedEnumerable = admits_sorted_enumeration_v<Type>;


// MonotoneContainer
//   concept: keyed AND comparator-equipped: no positions, but the enumeration
// is sorted by construction. A hash-ordered container has no comparator and is
// not this.
template<typename Type>
concept MonotoneContainer = is_monotone_container_v<Type>;


// SortedContainer
//   concept: positional, and the positions are held in comparator order by
// invariant. Named for the guarantee -- `sorted_container` is a header and a
// mixin here.
template<typename Type>
concept SortedContainer = is_sorted_container_v<Type>;


// UnsortedContainer
//   concept: a container whose positions are NOT guaranteed in comparator
// order. Gated on being a container, so a non-container reports false rather
// than a vacuous true -- which is what separates this from a bare negation.
template<typename Type>
concept UnsortedContainer = is_unsorted_container_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_SORTED_CONTAINER_CONCEPTS_HPP
