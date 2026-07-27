/******************************************************************************
* djinterp [container] container_filter_concepts.hpp C++20 concepts for the
* FILTERING axis -- the `requires`-facing view of container_filter_traits.hpp.
* THE CONCEPTS ADD NO POLICY. Each is exactly its trait, spelled so it can
* constrain a template instead of gating one through enable_if. The trait stays
* the single source of truth; if a classification is wrong, it is wrong in one
* place. That is the whole point of generating these rather than restating the
* detection logic in `requires` clauses. PORTABILITY: Gated on C++20 + concepts.
* Below that the header is empty and callers use the `::value` / `_v` forms
* directly -- which is why nothing else in the framework is allowed to depend on
* these. path:
* /inc/djinterp/core/container/concepts/container_filter_concepts.hpp link(s):
* TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_CONTAINER_FILTER_CONCEPTS_
#define DJINTERP_CONTAINER_FILTER_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../traits/container_filter_traits.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ==========================================================================
//  FILTERABILITY
// ==========================================================================


// filterable_container
// concept: can be filtered into a container of its own kind -- it can be walked
// AND rebuilt.
template<typename _Type>
concept filterable_container =
    is_container_filterable_v<clean_t<_Type>>;


// natively_filterable_container
// concept: carries its OWN filter -- so filtering need not go through the
// generic walk-and-rebuild path, and can keep structure the generic path would
// have to discard.
template<typename _Type>
concept natively_filterable_container =
    has_native_filter_v<clean_t<_Type>>;


// filter_source_container
// concept: can be filtered FROM even if not INTO. A container that can be read
// but not rebuilt elementwise -- a radix tree is exactly this, because an
// element carries no key -- is a source and not a sink.
template<typename _Type>
concept filter_source_container =
    is_filter_source_v<clean_t<_Type>>;


// filter_input_only_container
// concept: the explicit statement of that asymmetry: readable, not rebuildable.
template<typename _Type>
concept filter_input_only_container =
    is_filter_input_only_v<clean_t<_Type>>;


// ==========================================================================
//  WHAT SURVIVES A FILTER
// ==========================================================================


// order_preserving_filter
//   concept: filtering keeps the order. A filter removes elements; whether the
// SURVIVORS keep their relations is a separate question per axis, and
// these three answer it.
template<typename _Type>
concept order_preserving_filter =
    filter_preserves_order_v<clean_t<_Type>>;


// uniqueness_preserving_filter
// concept: filtering cannot introduce duplicates -- a restriction of an
// injective map is injective.
template<typename _Type>
concept uniqueness_preserving_filter =
    filter_preserves_uniqueness_v<clean_t<_Type>>;


// sortedness_preserving_filter
//   concept: filtering keeps the sort -- a subsequence of a sorted sequence is
// sorted.
template<typename _Type>
concept sortedness_preserving_filter =
    filter_preserves_sortedness_v<clean_t<_Type>>;

NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_CONTAINER_FILTER_CONCEPTS_
