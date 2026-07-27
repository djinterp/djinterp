/******************************************************************************
* djinterp [container] bounded_container_concepts.hpp C++20 concepts for the
* BOUNDEDNESS axis -- the `requires`-facing view of
* bounded_container_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly its
* trait, spelled so it can constrain a template instead of gating one through
* enable_if. The trait stays the single source of truth; if a classification is
* wrong, it is wrong in one place. That is the whole point of generating these
* rather than restating the detection logic in `requires` clauses. PORTABILITY:
* Gated on C++20 + concepts. Below that the header is empty and callers use the
* `::value` / `_v` forms directly -- which is why nothing else in the framework
* is allowed to depend on these. path:
* /inc/djinterp/core/container/concepts/bounded_container_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_BOUNDED_CONTAINER_CONCEPTS_
#define DJINTERP_BOUNDED_CONTAINER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../traits/bounded_container_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  CAPACITY  (kappa < infinity?)
// ==========================================================================


// bounded_container
// concept: kappa < infinity -- the container's total size is capped by its
// type. Positive evidence only: a fixed extent, a tuple_size, static interval
// bounds, or a capacity() that no reserve() can move.
template<typename _Type>
concept bounded_container =
    is_bounded_container_v<clean_t<_Type>>;


// unbounded_container
// concept: kappa = infinity -- it looks like a container (it has size()) and
// shows no capacity-bounding evidence at all.
template<typename _Type>
concept unbounded_container =
    is_unbounded_container_v<clean_t<_Type>>;


// ==========================================================================
//  DOMAIN  (an orthogonal sub-axis)
// ==========================================================================


// domain_bounded_container
// concept: every element value lies in a closed interval I = [x,y,z].
// Orthogonal to capacity: a fixed array is size-bounded but domain-free.
template<typename _Type>
concept domain_bounded_container =
    is_domain_bounded_container_v<clean_t<_Type>>;


// ==========================================================================
//  SIGNALS  (for constraining on the evidence, not the verdict)
// ==========================================================================


// fixed_extent_container
// concept: carries a static `extent` -- the compile-time fixed-capacity
// convention.
template<typename _Type>
concept fixed_extent_container =
    has_fixed_extent_signal_v<clean_t<_Type>>;


// growable_container
// concept: exposes reserve(n) -- the ANTI-signal that disqualifies a capacity()
// from meaning a FIXED capacity.
template<typename _Type>
concept growable_container =
    has_reserve_signal_v<clean_t<_Type>>;


// sized_container
// concept: exposes size(). The weakest 'is a container at all' guard, and what
// separates unbounded from unknown.
template<typename _Type>
concept sized_container =
    has_size_signal_v<clean_t<_Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_BOUNDED_CONTAINER_CONCEPTS_
