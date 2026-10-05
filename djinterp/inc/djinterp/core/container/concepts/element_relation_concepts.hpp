/*******************************************************************************
* djinterp [core]                                  element_relation_concepts.hpp
*
* C++20 concepts for the ELEMENT RELATIONS axis -- the `requires`-facing view
* of element_relation_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/element_relation_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_ELEMENT_RELATION_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_ELEMENT_RELATION_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/element_relation_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHAT THE ELEMENTS SUPPORT
// ==========================================================================


// EqualityComparableElements
//   concept: the ELEMENTS compare for equality. This is the precondition for
// content equality of the containers -- not the same thing as the containers
// themselves being comparable.
template<typename Type>
concept EqualityComparableElements = has_equality_comparable_elements_v<Type>;


// OrderedComparableElements
//   concept: the elements admit <, so the container can be ordered by content.
template<typename Type>
concept OrderedComparableElements = has_less_than_comparable_elements_v<Type>;


// TotallyOrderedElements
//   concept: the elements are TOTALLY ordered -- every pair is comparable,
// which is what a sort actually needs and what < alone does not promise.
template<typename Type>
concept TotallyOrderedElements = has_totally_ordered_elements_v<Type>;


// ==========================================================================
//  WHAT THE CONTAINER ITSELF SUPPORTS
// ==========================================================================


// EqualityComparableContainer
//   concept: the CONTAINER compares for equality, directly.
template<typename Type>
concept EqualityComparableContainer = is_equality_comparable_container_v<Type>;


// OrderedComparableContainer
//   concept: the container compares with <.
template<typename Type>
concept OrderedComparableContainer = is_ordered_comparable_container_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_ELEMENT_RELATION_CONCEPTS_HPP
