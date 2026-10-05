/*******************************************************************************
* djinterp [core]                              container_conversion_concepts.hpp
*
* C++20 concepts for the CONVERSION axis -- the `requires`-facing view of
* container_conversion_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/container_conversion_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_CONVERSION_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_CONVERSION_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_conversion_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  CONVERTIBILITY
// ==========================================================================


// container_convertible_to
//   concept: there is any path at all from From to To.
template<typename From, typename To>
concept container_convertible_to =
    is_convertible_between<clean_t<From>, clean_t<To>>::value;


// losslessly_convertible_to
//   concept: the conversion loses NOTHING -- no content, no topology, no
// capacity. The distinction from lossy is not a warning label; it is the
// difference between a rename and a projection.
template<typename From, typename To>
concept losslessly_convertible_to =
    is_lossless_conversion<clean_t<From>, clean_t<To>>::value;


// lossily_convertible_to
//   concept: the conversion is possible but DROPS something -- content,
// ordering, or elements over capacity.
template<typename From, typename To>
concept lossily_convertible_to =
    is_lossy_conversion<clean_t<From>, clean_t<To>>::value;


// ==========================================================================
//  PATHS
// ==========================================================================


// range_constructible_from
//   concept: To can be built from From's iterator range. Note the direction:
// the CONSTRAINT is on To, the RANGE is From.
template<typename From, typename To>
concept range_constructible_from =
    is_range_constructible<clean_t<To>, clean_t<From>>::value;


// range_insertable_from
//   concept: From's range can be inserted into an existing To.
template<typename From, typename To>
concept range_insertable_from =
    is_range_insertable<clean_t<To>, clean_t<From>>::value;


// view_convertible_to
//   concept: the conversion is a VIEW -- no copy, no ownership, and therefore
// no cost and no independence.
template<typename From, typename To>
concept view_convertible_to =
    is_view_conversion<clean_t<From>, clean_t<To>>::value;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_CONVERSION_CONCEPTS_HPP
