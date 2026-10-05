/*******************************************************************************
* djinterp [core]                                 mutable_container_concepts.hpp
*
* C++20 concepts for the MUTABILITY axis -- the `requires`-facing view of
* mutable_container_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/mutable_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_MUTABLE_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_MUTABLE_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/mutable_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHAT MAY CHANGE
// ==========================================================================


// MutableContainerType
//   concept: something may change -- values, structure, or both.
template<typename Type>
concept MutableContainerType = is_mutable_container_v<Type>;


// ImmutableContainerType
//   concept: container-shaped and nothing may change.
template<typename Type>
concept ImmutableContainerType = is_immutable_container_v<Type>;


// ElementMutableContainerType
//   concept: an EXISTING element may be overwritten in place. The probe is
// sequence- shaped (a settable operator[] or data()), so an associative
// container -- std::map included -- does not read as this. Not a bug: it is
// what the structural probe can see.
template<typename Type>
concept ElementMutableContainerType = is_element_mutable_container_v<Type>;


// StructurallyMutableContainerType
//   concept: the SET of elements may change -- insert, erase, clear, resize.
template<typename Type>
concept StructurallyMutableContainerType =
    is_structurally_mutable_container_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_MUTABLE_CONTAINER_CONCEPTS_HPP
