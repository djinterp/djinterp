/******************************************************************************
* djinterp [container]                       container_conversion_concepts.hpp
*
* Conversion-tier concepts:
*   C++20 concepts layered over container_conversion_traits.hpp. These
* concepts provide readable constraints for full container conversion-tier
* classification without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_conversion_traits.hpp:
*   - lossy and structural per-axis detection
*   - overall tier deduction
*   - convenience predicates
*   - shorthand concepts over container_conversion_class<F, T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_conversion_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONVERSION_CONCEPTS_
#define DJINTERP_CONTAINER_CONVERSION_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_conversion_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_conversion_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   Tier-2 / Tier-3 per-axis concepts
// ===========================================================================

template<typename _From, typename _To>
concept deduplicating_conversion_pair =
    needs_deduplication_v<_From, _To>;

template<typename _From, typename _To>
concept bound_clamping_conversion_pair =
    needs_bound_clamp_v<_From, _To>;

template<typename _From, typename _To>
concept lossy_conversion_pair =
    is_lossy_convertible_v<_From, _To>;

template<typename _From, typename _To>
concept hierarchy_changing_conversion_pair =
    needs_hierarchy_change_v<_From, _To>;

template<typename _From, typename _To>
concept backing_changing_conversion_pair =
    needs_ing_change_v<_From, _To>;

template<typename _From, typename _To>
concept incompatible_element_conversion_pair =
    has_incompatible_elements_v<_From, _To>;

template<typename _From, typename _To>
concept structural_conversion_pair =
    is_structural_convertible_v<_From, _To>;


// ===========================================================================
// II.  Tier identity concepts
// ===========================================================================

template<typename _From, typename _To>
concept view_conversion_pair =
    ( conversion_tier_v<_From, _To> == DConversionTier::view );

template<typename _From, typename _To>
concept constructive_conversion_pair =
    ( conversion_tier_v<_From, _To> == DConversionTier::constructive );

template<typename _From, typename _To>
concept lossy_tier_conversion_pair =
    ( conversion_tier_v<_From, _To> == DConversionTier::lossy );

template<typename _From, typename _To>
concept structural_tier_conversion_pair =
    ( conversion_tier_v<_From, _To> == DConversionTier::structural );

template<typename _From, typename _To>
concept impossible_conversion_pair =
    ( conversion_tier_v<_From, _To> == DConversionTier::impossible );

template<typename _From, typename _To>
concept convertible_container_pair =
    is_convertible_container_pair_v<_From, _To>;

template<typename _From, typename _To>
concept lossless_conversion_pair =
    is_lossless_convertible_v<_From, _To>;


// ===========================================================================
// III. Classification-based shorthand concepts
// ===========================================================================

template<typename _From, typename _To>
concept classified_conversion_pair =
    container_conversion_class<_From, _To>::is_convertible;

template<typename _From, typename _To>
concept classified_view_conversion_pair =
    container_conversion_class<_From, _To>::is_view;

template<typename _From, typename _To>
concept classified_constructive_conversion_pair =
    container_conversion_class<_From, _To>::is_constructive;

template<typename _From, typename _To>
concept classified_lossy_conversion_pair =
    container_conversion_class<_From, _To>::is_lossy;

template<typename _From, typename _To>
concept classified_structural_conversion_pair =
    container_conversion_class<_From, _To>::is_structural;

template<typename _From, typename _To>
concept classified_impossible_conversion_pair =
    container_conversion_class<_From, _To>::is_impossible;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONVERSION_CONCEPTS_