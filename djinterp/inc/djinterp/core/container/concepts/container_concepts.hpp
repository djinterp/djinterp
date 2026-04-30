/******************************************************************************
* djinterp [container]                                  container_concepts.hpp
*
* Core container concepts:
*   C++20 concepts layered over container_traits.hpp. These concepts provide
* readable constraints for the core container classification surface without
* replacing the existing SFINAE trait surface.
*
*   This header targets the current umbrella `container_traits.hpp` revision,
* which owns generic method/type detection and forwards several axis-specific
* traits through included specialized headers.
*
* 
* path:      /inc/djinterp/core/container/concepts/container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONCEPTS_
#define DJINTERP_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_traits.hpp"

NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept sized_container_surface = has_size_accessor_v<_Type>;

template<typename _Type>
concept max_sized_container_surface = has_max_size_accessor_v<_Type>;

template<typename _Type>
concept min_sized_container_surface = has_min_size_accessor_v<_Type>;

template<typename _Type>
concept capacity_container_surface = has_capacity_accessor_v<_Type>;

template<typename _Type>
concept data_access_container_surface =
    has_data_accessor_v<_Type> || has_data_method_v<_Type>;

template<typename _Type>
concept allocator_aware_container_surface = has_allocator_type_v<_Type>;

template<typename _Type>
concept keyed_container_surface = has_key_type_v<_Type>;

template<typename _Type>
concept mapped_container_surface = has_mapped_type_v<_Type>;

template<typename _Type>
concept hash_named_container_surface = has_hasher_type_v<_Type>;

template<typename _Type>
concept key_compare_named_container_surface = has_key_compare_v<_Type>;

template<typename _Type>
concept value_type_named_container_surface = has_value_type_v<_Type>;

template<typename _Type>
concept depth_typed_container_surface = has_depth_type_v<_Type>;

template<typename _Type>
concept parented_container_surface = has_parent_accessor_v<_Type>;

template<typename _Type>
concept child_access_container_surface = has_children_accessor_v<_Type>;

template<typename _Type>
concept root_access_container_surface = has_root_accessor_v<_Type>;

template<typename _Type>
concept depth_access_container_surface = has_depth_accessor_v<_Type>;

template<typename _Type>
concept mutable_insert_container_surface = has_insert_v<_Type>;

template<typename _Type>
concept mutable_push_container_surface = has_push__v<_Type>;

template<typename _Type>
concept erasable_container_surface = has_erase_v<_Type>;

template<typename _Type>
concept clearable_container_surface = has_clear_v<_Type>;

template<typename _Type>
concept reverse_iterable_container_surface =
    has_rbegin_accessor_v<_Type> && has_rend_accessor_v<_Type>;

template<typename _Type>
concept cbegin_iterable_container_surface =
    has_cbegin_accessor_v<_Type> && has_cend_accessor_v<_Type>;

template<typename _Type>
concept size_interval_container_surface = has_size_interval_type_v<_Type>;

template<typename _Type>
concept depth_interval_container_surface = has_depth_interval_type_v<_Type>;

template<typename _Type>
concept multiplicity_interval_container_surface =
    has_multiplicity_interval_type_v<_Type>;

template<typename _Type>
concept compile_time_container_type = is_compile_time_container_v<_Type>;

template<typename _Type>
concept not_compile_time_container_type = is_not_compile_time_container_v<_Type>;

template<typename _Type>
concept bounded_container_type_interval_aware = is_bounded_v<_Type>;

template<typename _Type>
concept unbounded_container_type_interval_aware = is_unbounded_v<_Type>;

template<typename _Type>
concept lower_bounded_container_type = is_lower_bounded_v<_Type>;

template<typename _Type>
concept upper_bounded_container_type = is_upper_bounded_v<_Type>;

template<typename _Type>
concept static_storage_container_type_core = is_static_storage_v<_Type>;

template<typename _Type>
concept dynamic_storage_container_type_core = is_dynamic_storage_v<_Type>;

template<typename _Type>
concept const_iterable_container_type = is_const_iterable_container_v<_Type>;

template<typename _Type>
concept ordered_container_type = is_ordered_container_v<_Type>;

template<typename _Type>
concept unordered_container_type = is_unordered_container_v<_Type>;

template<typename _Type>
concept sorted_container_type = is_sorted_container_v<_Type>;

template<typename _Type>
concept unique_container_type = is_unique_container_v<_Type>;

template<typename _Type>
concept multi_container_type = is_multi_container_v<_Type>;

template<typename _Type>
concept hierarchical_container_type = is_hierarchical_container_v<_Type>;

template<typename _Type>
concept flat_container_type = is_flat_container_v<_Type>;

template<typename _Type>
concept overlay_container_type_core = is_overlay_container_v<_Type>;

template<typename _Type>
concept fundamental_container_type_core = is_fundamental_container_v<_Type>;

template<typename _Type>
concept readable_container_type = container_class<_Type>::readable;

template<typename _Type>
concept writable_container_type = container_class<_Type>::writable;

template<typename _Type>
concept iterable_container_type_core = container_class<_Type>::iterable;

template<typename _Type>
concept contiguous_container_type = container_class<_Type>::contiguous;

template<typename _Type>
concept hierarchical_classified_container_type = container_class<_Type>::hierarchical;

template<typename _Type>
concept bounded_classified_container_type = container_class<_Type>::bounded;

template<typename _Type>
concept sorted_unique_container_type =
    container_class<_Type>::sorted && container_class<_Type>::unique;

template<typename _Type>
concept mutable_sequence_like_container =
    readable_container_type<_Type> &&
    writable_container_type<_Type> &&
    iterable_container_type_core<_Type>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONCEPTS_