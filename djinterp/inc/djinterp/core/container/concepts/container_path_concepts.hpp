/******************************************************************************
* djinterp [container]                             container_path_concepts.hpp
*
* Container path concepts:
*   Refreshed C++20 concepts layered over the newly uploaded
* container_path_traits.hpp. These concepts provide readable constraints
* for path-policy-like types without replacing the existing SFINAE trait
* surface.
*
*   The concepts mirror the verified public classification axes from
* container_path_traits.hpp:
*   - policy type aliases
*   - per-method structural detection
*   - navigation / component / full path-policy satisfaction
*   - capability-level and operation-availability concepts
*
* 
* path:      /inc/djinterp/core/container/concepts/container_path_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_PATH_CONCEPTS_REFRESHED_
#define DJINTERP_CONTAINER_PATH_CONCEPTS_REFRESHED_ 1

#ifndef __cplusplus
    #error "container_path_concepts_refreshed.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_path_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept path_container_typed_policy =
    has_path_container_type_v<_Type>;

template<typename _Type>
concept path_index_typed_policy =
    has_path_index_type_v<_Type>;

template<typename _Type>
concept path_component_typed_policy =
    has_path_component_type_v<_Type>;

template<typename _Type>
concept path_alias_complete_policy =
    has_path_type_aliases_v<_Type>;

template<typename _Type>
concept null_index_path_policy =
    has_path_null_index_v<_Type>;

template<typename _Type>
concept is_null_path_policy =
    has_path_is_null_v<_Type>;

template<typename _Type>
concept parent_navigation_path_policy =
    has_path_parent_v<_Type>;

template<typename _Type>
concept first_child_navigation_path_policy =
    has_path_first_child_v<_Type>;

template<typename _Type>
concept next_sibling_navigation_path_policy =
    has_path_next_sibling_v<_Type>;

template<typename _Type>
concept component_access_path_policy =
    has_path_component_v<_Type>;

template<typename _Type>
concept navigation_path_policy_type =
    is_navigation_policy_v<_Type>;

template<typename _Type>
concept component_path_policy_type =
    is_component_policy_v<_Type>;

template<typename _Type>
concept full_path_policy_type =
    is_path_policy_v<_Type>;

template<typename _Type>
concept ancestry_capable_path_policy =
    ( policy_path_capability_v<_Type> >= path_capability::ancestry );

template<typename _Type>
concept component_capable_path_policy =
    ( policy_path_capability_v<_Type> >= path_capability::component );

template<typename _Type>
concept fully_capable_path_policy =
    ( policy_path_capability_v<_Type> == path_capability::full );

template<typename _Type>
concept path_capable_policy =
    has_path_capability_v<_Type>;

template<typename _Type>
concept depth_capable_path_policy =
    can_path_depth_v<_Type>;

template<typename _Type>
concept resolve_capable_path_policy =
    can_path_resolve_v<_Type>;

template<typename _Type>
concept collect_capable_path_policy =
    can_path_collect_v<_Type>;

template<typename _Type>
concept classified_path_policy =
    path_policy_class<_Type>::capability != path_capability::none;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_PATH_CONCEPTS_REFRESHED_
