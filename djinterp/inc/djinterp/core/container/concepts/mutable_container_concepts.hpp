/******************************************************************************
* djinterp [container]                          mutable_container_concepts.hpp
*
* Mutability-axis concepts:
*   C++20 concepts layered over mutable_container_traits.hpp. These concepts
* provide readable constraints for mutable / immutable classification without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* mutable_container_traits.hpp:
*   - individual mutator signals
*   - mutable / immutable classification
*   - shorthand concepts over mutable_container_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                mutable_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_MUTABLE_CONTAINER_CONCEPTS_
#define DJINTERP_MUTABLE_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "mutable_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/mutable_container_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept push_back_mutable_container =
    has_push_back_signal_v<_Type>;

template<typename _Type>
concept push_front_mutable_container =
    has_push_front_signal_v<_Type>;

template<typename _Type>
concept insert_mutable_container =
    has_insert_signal_v<_Type>;

template<typename _Type>
concept erase_mutable_container =
    has_erase_signal_v<_Type>;

template<typename _Type>
concept clear_mutable_container =
    has_clear_signal_v<_Type>;

template<typename _Type>
concept resize_mutable_container =
    has_resize_signal_v<_Type>;

template<typename _Type>
concept mutable_subscript_container =
    has_mutable_subscript_signal_v<_Type>;

template<typename _Type>
concept mutable_data_container =
    has_mutable_data_signal_v<_Type>;

template<typename _Type>
concept mutable_container_type_structural =
    is_mutable_container_v<_Type>;

template<typename _Type>
concept immutable_container_type_structural =
    is_immutable_container_v<_Type>;

template<typename _Type>
concept classified_mutable_container =
    mutable_container_class<_Type>::is_mutable;

template<typename _Type>
concept classified_immutable_container =
    mutable_container_class<_Type>::is_immutable;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_MUTABLE_CONTAINER_CONCEPTS_