/******************************************************************************
* djinterp [container]                         iterable_container_concepts.hpp
*
* Iterability-axis concepts:
*   C++20 concepts layered over iterable_container_traits.hpp.
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                iterable_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_ITERABLE_CONTAINER_CONCEPTS_
#define DJINTERP_ITERABLE_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "iterable_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/iterable_container_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept begin_iterable_container_surface =
    has_begin_method_v<_Type>;

template<typename _Type>
concept end_iterable_container_surface =
    has_end_method_v<_Type>;

template<typename _Type>
concept iterator_alias_container_surface =
    has_iterator_alias_v<_Type>;

template<typename _Type>
concept const_iterator_alias_container_surface =
    has_const_iterator_alias_v<_Type>;

template<typename _Type>
concept value_type_alias_container_surface =
    has_value_type_alias_v<_Type>;

template<typename _Type>
concept iterable_container_type_structural =
    is_iterable_container_v<_Type>;

template<typename _Type>
concept non_iterable_container_type_structural =
    is_non_iterable_container_v<_Type>;

template<typename _Type>
concept fully_named_iterable_container_surface =
    has_iterator_alias_v<_Type> &&
    has_const_iterator_alias_v<_Type>;

template<typename _Type>
concept classified_iterable_container_type =
    iterable_container_class<_Type>::is_iterable;

template<typename _Type>
concept classified_non_iterable_container_type =
    iterable_container_class<_Type>::is_non_iterable;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END


#endif  // DJINTERP_ITERABLE_CONTAINER_CONCEPTS_