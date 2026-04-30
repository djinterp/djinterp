/******************************************************************************
* djinterp [container]                          container_compare_concepts.hpp
*
* Comparison concepts:
*   C++20 concepts layered over container_compare_traits.hpp. These concepts
* provide readable constraints for element-comparable, container-comparable,
* and cross-container-comparable types without replacing the existing SFINAE
* trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_compare_traits.hpp:
*   - element-level comparison support
*   - container-level comparison support
*   - comparison-degree wrappers
*   - cross-container compatibility and comparison
*   - shorthand concepts over container_compare_class<T> and
*     container_cross_compare_class<A, B>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_compare_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_COMPARE_CONCEPTS_
#define DJINTERP_CONTAINER_COMPARE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_compare_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_compare_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   Element-level concepts
// ===========================================================================

template<typename _Type>
concept equality_element_comparable_container =
    has_equality_comparable_elements_v<_Type>;

template<typename _Type>
concept less_than_element_comparable_container =
    has_less_than_comparable_elements_v<_Type>;

template<typename _Type>
concept totally_ordered_element_container =
    has_totally_ordered_elements_v<_Type>;

template<typename _Type>
concept three_way_element_container =
    has_three_way_comparable_elements_v<_Type>;


// ===========================================================================
// II.  Container-level concepts
// ===========================================================================

template<typename _Type>
concept equality_container_comparable =
    is_equality_comparable_container_v<_Type>;

template<typename _Type>
concept less_than_container_comparable =
    is_less_than_comparable_container_v<_Type>;

template<typename _Type>
concept three_way_container_comparable =
    is_three_way_comparable_container_v<_Type>;

template<typename _Type>
concept sortable_container_type =
    is_sortable_container_v<_Type>;

template<typename _Type>
concept comparator_typed_container =
    has_custom_comparator_type_v<_Type>;

template<typename _Type>
concept hashed_comparator_container =
    has_hasher_type_v<_Type>;


// ===========================================================================
// III. Degree-oriented concepts
// ===========================================================================

template<typename _Type>
concept equality_degree_element_container =
    ( element_comparison_degree_v<_Type> == DComparisonDegree::equality );

template<typename _Type>
concept partial_order_element_container =
    ( element_comparison_degree_v<_Type> == DComparisonDegree::partial_order );

template<typename _Type>
concept total_order_element_container =
    ( element_comparison_degree_v<_Type> == DComparisonDegree::total_order );

template<typename _Type>
concept three_way_degree_element_container =
    ( element_comparison_degree_v<_Type> == DComparisonDegree::three_way );

template<typename _Type>
concept equality_degree_container =
    ( container_comparison_degree_v<_Type> == DComparisonDegree::equality );

template<typename _Type>
concept partial_order_container =
    ( container_comparison_degree_v<_Type> == DComparisonDegree::partial_order );

template<typename _Type>
concept total_order_container =
    ( container_comparison_degree_v<_Type> == DComparisonDegree::total_order );

template<typename _Type>
concept three_way_degree_container =
    ( container_comparison_degree_v<_Type> == DComparisonDegree::three_way );


// ===========================================================================
// IV.  Cross-container compatibility concepts
// ===========================================================================

template<typename _A, typename _B>
concept same_element_type_container_pair =
    elements_same_type_v<_A, _B>;

template<typename _A, typename _B>
concept element_convertible_container_pair =
    elements_convertible_v<_A, _B>;

template<typename _A, typename _B>
concept mutually_convertible_container_pair =
    elements_mutually_convertible_v<_A, _B>;

template<typename _A, typename _B>
concept equality_cross_comparable_container_pair =
    elements_cross_equality_comparable_v<_A, _B>;

template<typename _A, typename _B>
concept less_than_cross_comparable_container_pair =
    elements_cross_less_than_comparable_v<_A, _B>;

template<typename _A, typename _B>
concept three_way_cross_comparable_container_pair =
    elements_cross_three_way_comparable_v<_A, _B>;

template<typename _A, typename _B>
concept cross_comparable_container_pair =
    containers_element_comparable_v<_A, _B>;


// ===========================================================================
// V.   Classification-based shorthand concepts
// ===========================================================================

template<typename _Type>
concept classified_comparable_container =
    container_compare_class<_Type>::is_comparable;

template<typename _Type>
concept classified_sortable_container =
    container_compare_class<_Type>::is_sortable;

template<typename _A, typename _B>
concept classified_cross_comparable_container_pair =
    container_cross_compare_class<_A, _B>::is_comparable;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_COMPARE_CONCEPTS_