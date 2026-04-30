/******************************************************************************
* djinterp [container]                flat_hierarchical_container_concepts.hpp
*
* Flat / hierarchical axis concepts:
*   C++20 concepts layered over flat_hierarchical_container_traits.hpp.
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                flat_hierarchical_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_FLAT_HIERARCHICAL_CONTAINER_CONCEPTS_
#define DJINTERP_FLAT_HIERARCHICAL_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "flat_hierarchical_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/flat_hierarchical_container_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept flat_shape_container_surface =
    (container_depth<_Type>::value >= 1);

template<typename _Type>
concept flat_container_type_structural =
    is_flat_container_v<_Type>;

template<typename _Type>
concept hierarchical_container_type_structural =
    is_hierarchical_container_v<_Type>;

template<typename _Type>
concept any_nested_container_type =
    (container_depth<_Type>::value >= 1);

template<typename _Type>
concept multi_level_hierarchical_container_type =
    (container_depth<_Type>::value >= 3);

template<typename _Type, std::size_t _N>
concept depth_bounded_hierarchical_container =
    is_depth_bounded_container<_Type, _N>::value;

template<typename _Type>
concept classified_flat_container_type =
    flat_hierarchical_container_class<_Type>::is_flat;

template<typename _Type>
concept classified_hierarchical_container_type =
    flat_hierarchical_container_class<_Type>::is_hierarchical;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END


#endif  // DJINTERP_FLAT_HIERARCHICAL_CONTAINER_CONCEPTS_