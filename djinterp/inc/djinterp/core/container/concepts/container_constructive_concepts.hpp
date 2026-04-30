/******************************************************************************
* djinterp [container]                     container_constructive_concepts.hpp
*
* Constructive conversion concepts:
*   C++20 concepts layered over container_constructive_traits.hpp. These
* concepts provide readable constraints for Tier 1 constructive conversions
* without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_constructive_traits.hpp:
*   - per-axis constructive needs
*   - iterator-range construction / insertion
*   - constructive path and convertibility
*   - shorthand concepts over container_constructive_class<F, T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_constructive_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONSTRUCTIVE_CONCEPTS_
#define DJINTERP_CONTAINER_CONSTRUCTIVE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_constructive_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_constructive_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   Per-axis constructive concepts
// ===========================================================================

template<typename _From, typename _To>
concept lifetime_copy_constructive_pair =
    needs_lifetime_copy_v<_From, _To>;

template<typename _From, typename _To>
concept sorting_constructive_pair =
    needs_sort_v<_From, _To>;

template<typename _From, typename _To>
concept lock_wrap_constructive_pair =
    needs_lock_wrap_v<_From, _To>;

template<typename _From, typename _To>
concept element_conversion_constructive_pair =
    needs_element_conversion_v<_From, _To>;


// ===========================================================================
// II.  Construction-path concepts
// ===========================================================================

template<typename _From, typename _To>
concept range_constructible_container_pair =
    is_range_constructible_v<_From, _To>;

template<typename _From, typename _To>
concept range_insertable_container_pair =
    is_range_insertable_v<_From, _To>;

template<typename _From, typename _To>
concept constructive_path_container_pair =
    has_constructive_path_v<_From, _To>;


// ===========================================================================
// III. Tier-1 constructive identity concepts
// ===========================================================================

template<typename _From, typename _To>
concept constructive_convertible_container_pair =
    is_constructive_convertible_v<_From, _To>;

template<typename _From, typename _To>
concept lossless_constructive_container_pair =
    constructive_convertible_container_pair<_From, _To>;

template<typename _From, typename _To>
concept any_constructive_work_container_pair =
    ( needs_lifetime_copy_v<_From, _To>      ||
      needs_sort_v<_From, _To>               ||
      needs_lock_wrap_v<_From, _To>          ||
      needs_element_conversion_v<_From, _To> );


// ===========================================================================
// IV.  Classification-based shorthand concepts
// ===========================================================================

template<typename _From, typename _To>
concept classified_constructive_container_pair =
    container_constructive_class<_From, _To>::is_constructive;

template<typename _From, typename _To>
concept classified_range_constructible_container_pair =
    container_constructive_class<_From, _To>::range_construct;

template<typename _From, typename _To>
concept classified_range_insertable_container_pair =
    container_constructive_class<_From, _To>::range_insert;

template<typename _From, typename _To>
concept classified_constructive_path_container_pair =
    container_constructive_class<_From, _To>::has_path;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONSTRUCTIVE_CONCEPTS_