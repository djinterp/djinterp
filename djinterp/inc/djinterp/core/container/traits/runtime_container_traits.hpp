/******************************************************************************
* djinterp [container]                            runtime_container_traits.hpp
*
* SFINAE structural traits for runtime-only containers.
*   A container is "runtime-only" when its query surface cannot
* be exercised in a constant expression context - e.g. heap-
* allocated dynamic storage, classes with virtual functions, or
* containers whose accessors are not declared constexpr.
*   This module is the structural complement to
* constexpr_container_traits.hpp:
*     is_runtime_container_v<T>
*       == is_container_v<T> && !is_constexpr_container_v<T>
*   Detection signals:
*     1. The type "looks like a container" (has size() accessor).
*     2. is_constexpr_container reports false.
*     3. (Stronger signals) presence of heap-storage indicators
*        - has_allocator_alias, has_reserve_method.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/runtime_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_RUNTIME_CONTAINER_TRAITS_
#define DJINTERP_RUNTIME_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../env/cpp/env_cpp_features.h"
#include "./constexpr_container_traits.hpp"  // is_constexpr_container


NS_DJINTERP


// ===========================================================================
// I.   SFINAE method detection
// ===========================================================================

// has_size_accessor_signal
//   trait: structural detection of any size() accessor (constexpr
// or not).  Used as the "is a container" signal here.
template<typename _Type,
         typename = void>
struct has_size_accessor_signal : std::false_type
{};

template<typename _Type>
struct has_size_accessor_signal<_Type, void_t<
    decltype(std::declval<const _Type&>().size())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_size_accessor_signal_v =
        has_size_accessor_signal<_Type>::value;
#endif


// has_allocator_alias
//   trait: detects an `allocator_type` member alias, a strong
// signal of heap-backed dynamic storage.
template<typename _Type,
         typename = void>
struct has_allocator_alias : std::false_type
{};

template<typename _Type>
struct has_allocator_alias<_Type, void_t<
    typename _Type::allocator_type
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_allocator_alias_v =
        has_allocator_alias<_Type>::value;
#endif


// has_reserve_method_signal
//   trait: detects `reserve(size_type)`, present on growable
// runtime containers.
template<typename _Type,
         typename = void>
struct has_reserve_method_signal : std::false_type
{};

template<typename _Type>
struct has_reserve_method_signal<_Type, void_t<
    decltype(std::declval<_Type&>().reserve(std::size_t{}))
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_reserve_method_signal_v =
        has_reserve_method_signal<_Type>::value;
#endif


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_runtime_container
//   trait: true if _Type is a container that is NOT constexpr-
// capable.
template<typename _Type>
struct is_runtime_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_size_accessor_signal<clean_type>::value
          && !is_constexpr_container<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_runtime_container_v =
        is_runtime_container<_Type>::value;
#endif


// requires_runtime_storage
//   trait: stronger signal - the container actively requires
// runtime-only storage (heap allocator and/or growable capacity).
template<typename _Type>
struct requires_runtime_storage
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_allocator_alias<clean_type>::value
          || has_reserve_method_signal<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool requires_runtime_storage_v =
        requires_runtime_storage<_Type>::value;
#endif


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct runtime_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool is_container_shape =
        has_size_accessor_signal<clean_type>::value;
    static constexpr bool is_runtime =
        is_runtime_container<clean_type>::value;
    static constexpr bool requires_runtime =
        requires_runtime_storage<clean_type>::value;
    static constexpr bool has_allocator =
        has_allocator_alias<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_RUNTIME_CONTAINER_TRAITS_