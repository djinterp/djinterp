/******************************************************************************
* djinterp [container] container_copy_merge_concepts.hpp C++20 concepts for the
* COPY AND MERGE axis -- the `requires`-facing view of
* container_copy_merge_traits.hpp. THE CONCEPTS ADD NO POLICY. Each is exactly
* its trait, spelled so it can constrain a template instead of gating one
* through enable_if. The trait stays the single source of truth; if a
* classification is wrong, it is wrong in one place. That is the whole point of
* generating these rather than restating the detection logic in `requires`
* clauses. PORTABILITY: Gated on C++20 + concepts. Below that the header is
* empty and callers use the `::value` / `_v` forms directly -- which is why
* nothing else in the framework is allowed to depend on these. path:
* /inc/djinterp/core/container/concepts/container_copy_merge_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONTAINER_COPY_MERGE_CONCEPTS_
#define DJINTERP_CONTAINER_COPY_MERGE_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../traits/container_copy_merge_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  COPY
// ==========================================================================


// copyable_container
// concept: iterable AND copy-constructible. Iterability is not decoration here:
// a container you cannot walk is one you cannot copy elementwise, and the trait
// says so.
template<typename _Type>
concept copyable_container =
    is_copyable_container_v<clean_t<_Type>>;


// ==========================================================================
//  MERGE
// ==========================================================================


// mergeable_with
//   concept: the two can be merged -- their elements are compatible and a merge
// discipline exists.
template<typename _From, typename _To>
concept mergeable_with =
    is_mergeable<clean_t<_From>, clean_t<_To>>::value;


// merge_elements_compatible_with
// concept: just the element half of mergeability, for constraining the value
// type without committing to a discipline.
template<typename _From, typename _To>
concept merge_elements_compatible_with =
    merge_elements_compatible<clean_t<_From>, clean_t<_To>>::value;


// merge_may_overflow_into
// concept: the merge CAN exceed _To's capacity. Bounded targets need this
// checked; unbounded ones never trip it.
template<typename _From, typename _To>
concept merge_may_overflow_into =
    merge_may_overflow<clean_t<_From>, clean_t<_To>>::value;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONTAINER_COPY_MERGE_CONCEPTS_
