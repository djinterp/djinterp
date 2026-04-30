/******************************************************************************
* djinterp [container]                           sorted_container_concepts.hpp
*
* Sorted-axis concepts:
*   C++20 concepts layered over sorted_container_traits.hpp. These concepts
* provide readable constraints for sorted / unsorted classification without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* sorted_container_traits.hpp:
*   - key_compare / value_compare / hasher / sorted-tag signals
*   - sorted / unsorted classification
*   - shorthand concepts over sorted_container_class<T>
*
* path:      /inc/djinterp/core/container/concepts/
*                concepts/sorted_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_SORTED_CONTAINER_CONCEPTS_
#define DJINTERP_SORTED_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "sorted_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/sorted_container_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept key_compare_sorted_surface =
    has_key_compare_alias_v<_Type>;

template<typename _Type>
concept value_compare_sorted_surface =
    has_value_compare_alias_v<_Type>;

template<typename _Type>
concept hashed_unsorted_surface =
    has_hasher_alias_v<_Type>;

template<typename _Type>
concept opt_in_sorted_surface =
    has_sorted_invariant_tag_v<_Type>;

template<typename _Type>
concept sorted_container_type_structural =
    is_sorted_container_v<_Type>;

template<typename _Type>
concept unsorted_container_type_structural =
    is_unsorted_container_v<_Type>;

template<typename _Type>
concept classified_sorted_container =
    sorted_container_class<_Type>::is_sorted;

template<typename _Type>
concept classified_unsorted_container =
    sorted_container_class<_Type>::is_unsorted;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_SORTED_CONTAINER_CONCEPTS_