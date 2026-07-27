/******************************************************************************
* djinterp [container] container_comparison_concepts.hpp C++20 concepts for the
* CROSS-CONTAINER COMPARISON axis -- the `requires`-facing view of
* container_comparison_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth; if a
* classification is wrong, it is wrong in one place. That is the whole point of
* generating these rather than restating the detection logic in `requires`
* clauses. PORTABILITY: Gated on C++20 + concepts. Below that the header is
* empty and callers use the `::value` / `_v` forms directly -- which is why
* nothing else in the framework is allowed to depend on these. path:
* /inc/djinterp/core/container/concepts/container_comparison_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONTAINER_COMPARISON_CONCEPTS_
#define DJINTERP_CONTAINER_COMPARISON_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../traits/container_comparison_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  REALIZATION
// ==========================================================================


// realization_comparable_with
//   concept: the two containers sit on axes that CAN be compared at all -- the
// precondition for every other concept here.
template<typename _From, typename _To>
concept realization_comparable_with =
    realization_comparable<clean_t<_From>, clean_t<_To>>::value;


// realization_equal_to
//   concept: they occupy the SAME point on every axis.
template<typename _From, typename _To>
concept realization_equal_to =
    realization_equal<clean_t<_From>, clean_t<_To>>::value;


// refines_container
// concept: _From is a REFINEMENT of _To: at least as restrictive on every axis.
// The order is what makes 'more specific' a checkable claim rather than a
// matter of taste.
template<typename _From, typename _To>
concept refines_container =
    refinement_of<clean_t<_From>, clean_t<_To>>::value;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONTAINER_COMPARISON_CONCEPTS_
