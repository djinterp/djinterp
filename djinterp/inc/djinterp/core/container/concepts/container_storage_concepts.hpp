/******************************************************************************
* djinterp [container]                          container_storage_concepts.hpp
*
* Storage-kind concepts:
*   C++20 concepts layered over container_storage_traits.hpp. These concepts
* provide readable constraints for storage-kind classification without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_storage_traits.hpp:
*   - compile-time extent, tuple_size, capacity, and fixed-capacity-tag signals
*   - static / fixed / dynamic storage classification
*   - storage_kind wrappers and shorthand concepts over 
*     container_storage_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_storage_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_STORAGE_CONCEPTS_
#define DJINTERP_CONTAINER_STORAGE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_storage_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_storage_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
// I.   primitive signal concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept compile_time_extent_storage_surface =
    has_compile_time_extent<_Type>::value;

template<typename _Type>
concept tuple_sized_storage_surface =
    has_tuple_size<_Type>::value;

template<typename _Type>
concept capacity_storage_surface =
    has_capacity_method_signal<_Type>::value;

template<typename _Type>
concept fixed_capacity_tagged_storage_surface =
    has_fixed_capacity_tag<_Type>::value;

///////////////////////////////////////////////////////////////////////////////
// II.  storage-kind identity concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept static_storage_container_type =
    is_static_storage_container<_Type>::value;

template<typename _Type>
concept fixed_storage_container_type =
    is_fixed_storage_container<_Type>::value;

template<typename _Type>
concept dynamic_storage_container_type =
    is_dynamic_storage_container<_Type>::value;

template<typename _Type>
concept known_storage_container_type =
    ( storage_kind_of<_Type>::value != storage_kind::unknown );

///////////////////////////////////////////////////////////////////////////////
// III. storage-kind wrapper concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept storage_kind_static_container =
    ( storage_kind_of<_Type>::value == storage_kind::static_storage );

template<typename _Type>
concept storage_kind_fixed_container =
    ( storage_kind_of<_Type>::value == storage_kind::fixed_storage );

template<typename _Type>
concept storage_kind_dynamic_container =
    ( storage_kind_of<_Type>::value == storage_kind::dynamic_storage );

///////////////////////////////////////////////////////////////////////////////
// IV.  classification-based shorthand concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept classified_static_storage_container =
    container_storage_class<_Type>::kind == storage_kind::static_storage;

template<typename _Type>
concept classified_fixed_storage_container =
    container_storage_class<_Type>::kind == storage_kind::fixed_storage;

template<typename _Type>
concept classified_dynamic_storage_container =
    container_storage_class<_Type>::kind == storage_kind::dynamic_storage;

template<typename _Type>
concept classified_known_storage_container =
    container_storage_class<_Type>::kind != storage_kind::unknown;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_STORAGE_CONCEPTS_