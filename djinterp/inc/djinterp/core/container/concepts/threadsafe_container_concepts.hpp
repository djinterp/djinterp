/******************************************************************************
* djinterp [container]                       threadsafe_container_concepts.hpp
*
* Thread-safe container concepts:
*   C++20 concepts layered over threadsafe_container_traits.hpp. These
* concepts provide readable constraints for lock-aware and thread-safe
* containers without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* threadsafe_container_traits.hpp:
*   - lock policy and mutex detection
*   - direct locking and atomic-state detection
*   - thread-safety level wrappers
*   - convenience predicates
*   - shorthand concepts over container_threadsafe_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                threadsafe_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_CONTAINER_CONCEPTS_
#define DJINTERP_THREADSAFE_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "threadsafe_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/threadsafe_container_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept lock_policy_typed_container =
    has_lock_policy_type_v<_Type>;

template<typename _Type>
concept mutex_typed_container =
    has_mutex_type_alias_v<_Type>;

template<typename _Type>
concept valid_lock_policy_container =
    has_valid_lock_policy_v<_Type>;

template<typename _Type>
concept policy_threadsafe_container =
    policy_is_threadsafe_v<_Type>;

template<typename _Type>
concept policy_shared_container =
    policy_supports_shared_v<_Type>;

template<typename _Type>
concept policy_timed_container =
    policy_supports_timed_v<_Type>;

template<typename _Type>
concept directly_lockable_container =
    is_directly_lockable_v<_Type>;

template<typename _Type>
concept directly_shared_lockable_container =
    is_directly_shared_lockable_v<_Type>;

template<typename _Type>
concept directly_timed_lockable_container =
    is_directly_timed_lockable_v<_Type>;

template<typename _Type>
concept mutex_access_container =
    has_get_mutex_method_v<_Type> || has_get_mutex_accessor_v<_Type>;

template<typename _Type>
concept atomic_size_container =
    has_atomic_size_type_v<_Type>;

template<typename _Type>
concept atomic_version_container =
    has_atomic_version_type_v<_Type>;

template<typename _Type>
concept versioned_threadsafe_container =
    has_version_tracking_v<_Type>;

template<typename _Type>
concept threadsafe_container_type =
    is_threadsafe_container_v<_Type>;

template<typename _Type>
concept non_threadsafe_container_type =
    is_non_threadsafe_container_v<_Type>;

template<typename _Type>
concept concurrent_read_container =
    supports_concurrent_reads_v<_Type>;

template<typename _Type>
concept timed_locking_container =
    supports_timed_locking_v<_Type>;

template<typename _Type>
concept atomic_only_threadsafe_container =
    container_thread_safety_level_v<_Type> ==
    thread_safety_level::atomic_only;

template<typename _Type>
concept exclusive_threadsafe_container =
    container_thread_safety_level_v<_Type> ==
    thread_safety_level::exclusive;

template<typename _Type>
concept timed_threadsafe_container =
    container_thread_safety_level_v<_Type> ==
    thread_safety_level::timed;

template<typename _Type>
concept shared_threadsafe_container =
    container_thread_safety_level_v<_Type> ==
    thread_safety_level::shared;

template<typename _Type>
concept shared_timed_threadsafe_container =
    container_thread_safety_level_v<_Type> ==
    thread_safety_level::shared_timed;

template<typename _Type>
concept classified_threadsafe_container =
    container_threadsafe_class<_Type>::is_threadsafe;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_THREADSAFE_CONTAINER_CONCEPTS_