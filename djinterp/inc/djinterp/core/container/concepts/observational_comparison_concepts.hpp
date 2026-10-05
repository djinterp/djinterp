/*******************************************************************************
* djinterp [core]                          observational_comparison_concepts.hpp
*
* C++20 concepts for the OBSERVATIONAL COMPARISON axis -- the
* `requires`-facing view of observational_comparison_traits.hpp.
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
*   Gated on C++20 + concepts. Below that the header is empty and callers use
* the `::value` / `_v` forms directly.
*
*
* path:      /inc/djinterp/core/container/concepts/observational_comparison_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_OBSERVATIONAL_COMPARISON_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_OBSERVATIONAL_COMPARISON_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/observational_comparison_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  WHAT CAN BE OBSERVED
// ==========================================================================


// SizeObservable
//   concept: exposes size() -- the weakest observation, and value-free:
// knowing HOW MANY is not observing any element.
template<typename Type>
concept SizeObservable = has_size_member_v<Type>;


// MembershipObservable
//   concept: exposes contains() -- membership without extraction.
template<typename Type>
concept MembershipObservable = has_contains_member_v<Type>;


// CountObservable
//   concept: exposes count() -- multiplicity without extraction.
template<typename Type>
concept CountObservable = has_count_member_v<Type>;


// IndexObservable
//   concept: exposes indexed access -- the observation that distinguishes a
// positional container from a bag.
template<typename Type>
concept IndexObservable = has_indexed_access_v<Type>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_OBSERVATIONAL_COMPARISON_CONCEPTS_HPP
