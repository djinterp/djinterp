/*******************************************************************************
* djinterp [core]                                 bounded_container_concepts.hpp
*
* C++20 concepts for the BOUNDEDNESS axis -- the `requires`-facing view of
* bounded_container_traits.hpp.
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
* the `::value` / `_v` forms directly.
*
*
* path:      /inc/djinterp/core/container/concepts/bounded_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_BOUNDED_CONTAINER_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_BOUNDED_CONTAINER_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/bounded_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  CAPACITY  (kappa < infinity?)
// ==========================================================================


// bounded_container
//   concept: kappa < infinity -- the container's total size is capped by its
// type. Positive evidence only: a fixed extent, a tuple_size, static interval
// bounds, or a capacity() that no reserve() can move.
template<typename Type>
concept bounded_container =
    is_bounded_container_v<clean_t<Type>>;


// unbounded_container
//   concept: kappa = infinity -- it looks like a container (it has size()) and
// shows no capacity-bounding evidence at all.
template<typename Type>
concept unbounded_container =
    is_unbounded_container_v<clean_t<Type>>;


// ==========================================================================
//  DOMAIN  (an orthogonal sub-axis)
// ==========================================================================


// domain_bounded_container
//   concept: every element value lies in a closed interval I = [x,y,z].
// Orthogonal to capacity: a fixed array is size-bounded but domain-free.
template<typename Type>
concept domain_bounded_container =
    is_domain_bounded_container_v<clean_t<Type>>;


// ==========================================================================
//  SIGNALS  (for constraining on the evidence, not the verdict)
// ==========================================================================


// fixed_extent_container
//   concept: carries a static `extent` -- the compile-time fixed-capacity
// convention.
template<typename Type>
concept fixed_extent_container =
    has_fixed_extent_signal_v<clean_t<Type>>;


// growable_container
//   concept: exposes reserve(n) -- the ANTI-signal that disqualifies a
// capacity() from meaning a FIXED capacity.
template<typename Type>
concept growable_container =
    has_reserve_signal_v<clean_t<Type>>;


// sized_container
//   concept: exposes size(). The weakest 'is a container at all' guard, and
// what separates unbounded from unknown.
template<typename Type>
concept sized_container =
    has_size_signal_v<clean_t<Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_BOUNDED_CONTAINER_CONCEPTS_HPP
