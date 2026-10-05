/*******************************************************************************
* djinterp [core]                            container_multiplicity_concepts.hpp
*
* C++20 concepts for the MULTIPLICITY axis -- the `requires`-facing view of
* container_multiplicity_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/container_multiplicity_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_MULTIPLICITY_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_MULTIPLICITY_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_multiplicity_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  THE OCCURRENCE BOUND m
// ==========================================================================


// UniqueContainer
//   concept: one occurrence per equivalence class, m = 1 -- set semantics.
template<typename Type>
concept UniqueContainer = is_unique_container_v<Type>;


// MultisetContainer
//   concept: a genuine equivalence with m > 1 -- duplicates admitted.
template<typename Type>
concept MultisetContainer = is_multiset_container_v<Type>;


// SequenceMultiplicityContainer
//   concept: comparator-less: identity equivalence, m = infinity, copies
// distinguished by position. Named in PascalCase; every concept is, so a
// concept and a same-named snake_case class cannot collide and the natural
// name is safe.
template<typename Type>
concept SequenceMultiplicityContainer = is_sequence_container_v<Type>;


// ==========================================================================
//  SIGNALS
// ==========================================================================


// CountableContainer
//   concept: the axis guard -- a value_type and a const-callable size().
template<typename Type>
concept CountableContainer = is_countable_container_v<Type>;


// DeclaresMultiplicityBound
//   concept: carries the opt-in static `multiplicity` -- the authoritative
// override, and the only way to state a bound with no structural tell (a
// bounded multiset, or a keyed container whose insert takes a key and a value
// rather than one value_type).
template<typename Type>
concept DeclaresMultiplicityBound = has_multiplicity_bound_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_MULTIPLICITY_CONCEPTS_HPP
