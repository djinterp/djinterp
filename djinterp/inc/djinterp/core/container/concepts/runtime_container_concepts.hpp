/******************************************************************************
* djinterp [container]                          runtime_container_concepts.hpp
*
* Runtime-only container concepts:
*   C++20 concepts layered over runtime_container_traits.hpp. These concepts
* provide readable constraints for runtime-only containers without replacing
* the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* runtime_container_traits.hpp:
*   - size / allocator / reserve runtime signals
*   - runtime-only and requires-runtime-storage classification
*   - shorthand concepts over runtime_container_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                runtime_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RUNTIME_CONTAINER_CONCEPTS_
#define DJINTERP_RUNTIME_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "runtime_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/runtime_container_traits.hpp"


NS_DJINTERP


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept runtime_size_signaled_container =
    has_size_accessor_signal_v<_Type>;

template<typename _Type>
concept allocator_backed_runtime_container =
    has_allocator_alias_v<_Type>;

template<typename _Type>
concept reservable_runtime_container =
    has_reserve_method_signal_v<_Type>;

template<typename _Type>
concept runtime_container_type =
    is_runtime_container_v<_Type>;

template<typename _Type>
concept requires_runtime_storage_container =
    requires_runtime_storage_v<_Type>;

template<typename _Type>
concept classified_runtime_container =
    runtime_container_class<_Type>::is_runtime;

template<typename _Type>
concept strongly_runtime_container =
    runtime_container_class<_Type>::is_runtime &&
    runtime_container_class<_Type>::requires_runtime;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_RUNTIME_CONTAINER_CONCEPTS_