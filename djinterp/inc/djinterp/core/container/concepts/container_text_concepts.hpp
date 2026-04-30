/******************************************************************************
* djinterp [container]                             container_text_concepts.hpp
*
* Text / stream concepts:
*   C++20 concepts layered over container_text_traits.hpp.
*
* 
* path:      /inc/djinterp/core/container/concepts/container_text_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TEXT_CONCEPTS_
#define DJINTERP_CONTAINER_TEXT_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_text_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_text_traits.hpp"

NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept native_text_container_surface =
    has_to_text_method_v<_Type>;

template<typename _Type>
concept native_stream_container_surface =
    has_stream_to_method_v<_Type>;

template<typename _Type>
concept to_string_container_surface =
    has_to_string_method_v<_Type>;

template<typename _Type>
concept ostream_insertable_container_surface =
    is_ostream_insertable_v<_Type>;

template<typename _Type>
concept text_convertible_elements_container =
    has_text_convertible_elements_v<_Type>;

template<typename _Type>
concept streamable_elements_container =
    has_streamable_elements_v<_Type>;

template<typename _Type>
concept ostream_insertable_elements_container =
    has_ostream_insertable_elements_v<_Type>;

template<typename _Type>
concept native_text_strategy_container =
    ( container_text_strategy_v<_Type> ==
      container_text_strategy::native );

template<typename _Type>
concept native_to_string_strategy_container =
    ( container_text_strategy_v<_Type> ==
      container_text_strategy::native_to_string );

template<typename _Type>
concept ostream_text_strategy_container =
    ( container_text_strategy_v<_Type> ==
      container_text_strategy::ostream );

template<typename _Type>
concept element_text_strategy_container =
    ( container_text_strategy_v<_Type> ==
      container_text_strategy::element );

template<typename _Type>
concept native_stream_strategy_container =
    ( container_stream_strategy_v<_Type> ==
      stream_strategy::native );

template<typename _Type>
concept element_stream_strategy_container =
    ( container_stream_strategy_v<_Type> ==
      stream_strategy::element );

template<typename _Type>
concept via_text_stream_strategy_container =
    ( container_stream_strategy_v<_Type> ==
      stream_strategy::via_text );

template<typename _Type>
concept text_convertible_container =
    is_text_convertible_v<_Type>;

template<typename _Type>
concept streamable_container =
    is_streamable_v<_Type>;

template<typename _Type>
concept round_trip_text_like_container =
    text_convertible_container<_Type> &&
    streamable_container<_Type>;

template<typename _Type>
concept classified_text_container =
    container_text_class<_Type>::is_text_convertible;

template<typename _Type>
concept classified_stream_container =
    container_text_class<_Type>::is_streamable;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TEXT_CONCEPTS_