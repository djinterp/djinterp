/*******************************************************************************
* djinterp [core]                              container_comparison_concepts.hpp
*
* C++20 concepts for the CROSS-CONTAINER COMPARISON axis -- the
* `requires`-facing view of container_comparison_traits.hpp.
*
*   THE CONCEPTS ADD NO POLICY.
*   Each is exactly its trait, spelled so it can constrain a template instead
* of gating one through enable_if. The trait stays the single source of truth;
* if a classification is wrong, it is wrong in one place. That is the whole
* point of generating these rather than restating the detection logic in
* `requires` clauses.
*
*   PORTABILITY:
*   Gated on C++20 + concepts. Below that the header is empty and callers use
* the `::value` / `_v` forms directly -- which is why
*
*
* path:      /inc/djinterp/core/container/concepts/container_comparison_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_COMPARISON_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_COMPARISON_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_comparison_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  REALIZATION
// ==========================================================================


// realization_comparable_with
//   concept: the two containers sit on axes that CAN be compared at all -- the
// precondition for every other concept here.
template<typename From, typename To>
concept realization_comparable_with =
    realization_comparable<clean_t<From>, clean_t<To>>::value;


// realization_equal_to
//   concept: they occupy the SAME point on every axis.
template<typename From, typename To>
concept realization_equal_to =
    realization_equal<clean_t<From>, clean_t<To>>::value;


// refines_container
//   concept: From is a REFINEMENT of To: at least as restrictive on every
// axis. The order is what makes 'more specific' a checkable claim rather than
// a matter of taste.
template<typename From, typename To>
concept refines_container =
    refinement_of<clean_t<From>, clean_t<To>>::value;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_COMPARISON_CONCEPTS_HPP
