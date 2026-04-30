/******************************************************************************
* djinterp [container]                             container_view_concepts.hpp
*
* View-conversion concepts:
*   C++20 concepts layered over container_view_traits.hpp.
*
* 
* path:      /inc/djinterp/core/container/concepts/container_view_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_VIEW_CONCEPTS_
#define DJINTERP_CONTAINER_VIEW_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_view_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_view_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept sorted_view_source_container = is_sorted_view_source_v<_Type>;

template<typename _Type>
concept unique_view_source_container = is_unique_view_source_v<_Type>;

template<typename _Type>
concept constexpr_lifetime_view_source =
    lifetime_of_v<_Type> == lifetime::constexpr_storage;

template<typename _Type>
concept immutable_lifetime_view_source =
    lifetime_of_v<_Type> == lifetime::immutable;

template<typename _Type>
concept mutable_lifetime_view_source =
    lifetime_of_v<_Type> == lifetime::mutable_storage;

template<typename _From, typename _To>
concept element_view_compatible_container_pair =
    is_element_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept lifetime_view_compatible_container_pair =
    is_lifetime_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept iteration_view_compatible_container_pair =
    is_iteration_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept ordering_view_compatible_container_pair =
    is_ordering_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept bounds_view_compatible_container_pair =
    is_bounds_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept multiplicity_view_compatible_container_pair =
    is_multiplicity_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept threadsafe_view_compatible_container_pair =
    is_threadsafe_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept hierarchy_view_compatible_container_pair =
    is_hierarchy_view_compatible_v<_From, _To>;

template<typename _From, typename _To>
concept view_convertible_container_pair =
    is_view_convertible_v<_From, _To>;

template<typename _From, typename _To>
concept symmetric_view_container_pair =
    is_symmetric_view_v<_From, _To>;

template<typename _From, typename _To>
concept classified_view_container_pair =
    container_view_class<_From, _To>::is_view;

template<typename _From, typename _To>
concept classified_symmetric_view_container_pair =
    container_view_class<_From, _To>::is_symmetric;

template<typename _From, typename _To>
concept partially_view_compatible_container_pair =
    (container_view_class<_From, _To>::view_axes > 0);

template<typename _From, typename _To>
concept blocked_view_container_pair =
    (container_view_class<_From, _To>::blocked_axes > 0);

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_VIEW_CONCEPTS_