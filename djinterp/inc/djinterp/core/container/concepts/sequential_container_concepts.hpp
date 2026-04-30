/******************************************************************************
* djinterp [container]                       sequential_container_concepts.hpp
*
* Sequential container concepts:
*   C++20 concepts layered over sequential_container_traits.hpp. These
* concepts provide readable constraints for sequential containers without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* sequential_container_traits.hpp:
*   - sequential identity
*   - sequential storage-kind wrappers
*   - shorthand concepts over sequential_kind<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                sequential_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_SEQUENTIAL_CONTAINER_CONCEPTS_
#define DJINTERP_SEQUENTIAL_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "sequential_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/sequential_container_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept sequential_container_type =
    is_sequential_container_v<_Type>;

template<typename _Type>
concept array_like_sequential_container =
    sequential_kind_v<_Type> == DSequentialKind::array_like;

template<typename _Type>
concept list_like_sequential_container =
    sequential_kind_v<_Type> == DSequentialKind::list_like;

template<typename _Type>
concept forward_list_like_sequential_container =
    sequential_kind_v<_Type> == DSequentialKind::forward_list_like;

template<typename _Type>
concept deque_like_sequential_container =
    sequential_kind_v<_Type> == DSequentialKind::deque_like;

template<typename _Type>
concept string_like_sequential_container =
    sequential_kind_v<_Type> == DSequentialKind::string_like;

template<typename _Type>
concept classified_sequential_container =
    sequential_container_type<_Type>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_SEQUENTIAL_CONTAINER_CONCEPTS_