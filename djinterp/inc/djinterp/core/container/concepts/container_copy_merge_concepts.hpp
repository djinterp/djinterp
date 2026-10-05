/*******************************************************************************
* djinterp [core]                              container_copy_merge_concepts.hpp
*
* C++20 concepts for the COPY AND MERGE axis -- the `requires`-facing view of
* container_copy_merge_traits.hpp.
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
* path:      /inc/djinterp/core/container/concepts/container_copy_merge_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_CONTAINER_COPY_MERGE_CONCEPTS_HPP
#define DJINTERP_CONTAINER_CONCEPTS_CONTAINER_COPY_MERGE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../traits/container_copy_merge_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  COPY
// ==========================================================================


// copyable_container
//   concept: iterable AND copy-constructible. Iterability is not decoration
// here:
// a container you cannot walk is one you cannot copy elementwise, and the
// trait says so.
template<typename Type>
concept copyable_container =
    is_copyable_container_v<clean_t<Type>>;


// ==========================================================================
//  MERGE
// ==========================================================================


// mergeable_with
//   concept: the two can be merged -- their elements are compatible and a
// merge discipline exists.
template<typename From, typename To>
concept mergeable_with =
    is_mergeable<clean_t<From>, clean_t<To>>::value;


// merge_elements_compatible_with
//   concept: just the element half of mergeability, for constraining the value
// type without committing to a discipline.
template<typename From, typename To>
concept merge_elements_compatible_with =
    merge_elements_compatible<clean_t<From>, clean_t<To>>::value;


// merge_may_overflow_into
//   concept: the merge CAN exceed To's capacity. Bounded targets need this
// checked; unbounded ones never trip it.
template<typename From, typename To>
concept merge_may_overflow_into =
    merge_may_overflow<clean_t<From>, clean_t<To>>::value;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONCEPTS_CONTAINER_COPY_MERGE_CONCEPTS_HPP
