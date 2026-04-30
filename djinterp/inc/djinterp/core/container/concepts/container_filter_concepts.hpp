/******************************************************************************
* djinterp [container]                           container_filter_concepts.hpp
*
* Filterability concepts:
*   C++20 concepts layered over container_filter_traits.hpp. These concepts
* provide readable constraints for filterable containers without replacing
* the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_filter_traits.hpp:
*   - filterability / input-only / source detection
*   - native filter and strategy wrappers
*   - invariant preservation
*   - result-type and shorthand concepts over container_filter_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_filter_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_FILTER_CONCEPTS_
#define DJINTERP_CONTAINER_FILTER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_filter_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_filter_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
// I.   core filterability concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept filterable_container_type =
    is_container_filterable_v<_Type>;

template<typename _Type>
concept filter_input_only_container_type =
    is_filter_input_only_v<_Type>;

template<typename _Type>
concept filter_source_container_type =
    is_filter_source_v<_Type>;

template<typename _Type>
concept natively_filterable_container =
    has_native_filter_v<_Type>;

///////////////////////////////////////////////////////////////////////////////
// II.  strategy-oriented concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept native_filter_strategy_container =
    ( container_filter_strategy_v<_Type> == filter_strategy::native );

template<typename _Type>
concept random_access_filter_strategy_container =
    ( container_filter_strategy_v<_Type> == filter_strategy::random_access );

template<typename _Type>
concept bidirectional_filter_strategy_container =
    ( container_filter_strategy_v<_Type> == filter_strategy::bidirectional );

template<typename _Type>
concept forward_only_filter_strategy_container =
    ( container_filter_strategy_v<_Type> == filter_strategy::forward_only );

template<typename _Type>
concept external_filter_strategy_container =
    ( container_filter_strategy_v<_Type> == filter_strategy::external );

///////////////////////////////////////////////////////////////////////////////
// III. invariant-preservation concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept order_preserving_filter_container =
    filter_preserves_order_v<_Type>;

template<typename _Type>
concept sortedness_preserving_filter_container =
    filter_preserves_sortedness_v<_Type>;

template<typename _Type>
concept uniqueness_preserving_filter_container =
    filter_preserves_uniqueness_v<_Type>;

template<typename _Type>
concept upper_bound_preserving_filter_container =
    filter_preserves_upper_bound_v<_Type>;

template<typename _Type>
concept lower_bound_preserving_filter_container =
    filter_preserves_lower_bound_v<_Type>;

///////////////////////////////////////////////////////////////////////////////
// IV.  classification-based shorthand concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept classified_filterable_container =
    container_filter_class<_Type>::is_filterable;

template<typename _Type>
concept classified_input_only_filter_container =
    container_filter_class<_Type>::is_input_only;

template<typename _Type>
concept classified_filter_source_container =
    container_filter_class<_Type>::is_source;

template<typename _Type>
concept classified_native_filter_container =
    container_filter_class<_Type>::has_native;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_FILTER_CONCEPTS_