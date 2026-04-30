/******************************************************************************
* djinterp [container]                            container_storage_traits.hpp
*
*   SFINAE structural traits for the storage-kind axis.
*   Three storage kinds, mutually exclusive:
*     static_storage    - capacity is fixed at compile time.
*                         Detection: presence of the `extent`
*                         compile-time constant or a
*                         std::tuple_size specialization.  Examples:
*                         std::array, our array<>, std::span<T,N>.
*     fixed_storage     - capacity is fixed at runtime (at
*                         construction or first reservation), but
*                         the type does not expose a way to grow
*                         past it.  Detection: capacity() exposed
*                         WITHOUT reserve(); or fixed_capacity tag.
*                         Examples: small_vector with no overflow,
*                         arenas, ring buffers.
*     dynamic_storage   - capacity grows at runtime via heap
*                         reallocation.  Detection: capacity()
*                         AND reserve() both present.  Examples:
*                         std::vector, std::string, std::deque
*                         (has resize but no capacity() - handled
*                         by the unbounded fallback).
*   Containers that do not match any of the three are reported as
* `storage_kind::unknown` so the caller can treat them as
* opaque.
*   The storage axis is orthogonal to constexpr / mutability / etc.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/container_storage_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SFINAE method / extent detection
II.   storage_kind enum
III.  per-kind classification umbrellas
IV.   storage_kind_of  (compile-time dispatch)
V.    aggregate snapshot
*/

#ifndef DJINTERP_CONTAINER_STORAGE_TRAITS_
#define DJINTERP_CONTAINER_STORAGE_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../env/cpp/env_cpp_features.h"


NS_DJINTERP

// ===========================================================================
// I.   SFINAE method / extent detection
// ===========================================================================

// has_compile_time_extent
//   trait: detects a static `extent` compile-time constant.
template<typename _Type,
         typename = void>
struct has_compile_time_extent : std::false_type
{};

template<typename _Type>
struct has_compile_time_extent<_Type, void_t<
    decltype(_Type::extent)
>> : std::true_type
{};


// has_tuple_size
//   trait: detects a std::tuple_size specialization for _Type.
// Identifies std::array-shaped types regardless of whether they
// expose `extent`.
NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct tuple_size_check : std::false_type
    {};

    template<typename _Type>
    struct tuple_size_check<_Type, void_t<
        decltype(std::tuple_size<_Type>::value)
    >> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct has_tuple_size
    : internal::tuple_size_check<clean_t<_Type>>
{};


// has_capacity_method_signal
template<typename _Type,
         typename = void>
struct has_capacity_method_signal : std::false_type
{};

template<typename _Type>
struct has_capacity_method_signal<_Type, void_t<
    decltype(std::declval<const _Type&>().capacity())
>> : std::true_type
{};


// has_fixed_capacity_tag
//   trait: opt-in `is_fixed_capacity` member alias equal to
// std::true_type, used to mark types like fixed-size ring buffers
// and small-vector-without-overflow whose capacity is decided at
// construction but does not grow.
template<typename _Type,
         typename = void>
struct has_fixed_capacity_tag : std::false_type
{};

template<typename _Type>
struct has_fixed_capacity_tag<_Type, void_t<
    typename _Type::is_fixed_capacity
>> : std::is_same<typename _Type::is_fixed_capacity,
                  std::true_type>
{};


// ===========================================================================
// II.  storage_kind enum
// ===========================================================================

// storage_kind
//   enum: classifies the container's underlying storage strategy.
enum class storage_kind
{
    static_storage,    // compile-time fixed capacity
    fixed_storage,     // runtime fixed capacity
    dynamic_storage,   // heap-allocated, growable
    unknown            // does not match any of the above
};


// ===========================================================================
// III. Per-kind classification umbrellas
// ===========================================================================

// is_static_storage_container
//   trait: capacity is fixed at compile time.
template<typename _Type>
struct is_static_storage_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_compile_time_extent<clean_type>::value ||
          has_tuple_size<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_static_storage_container_v =
        is_static_storage_container<_Type>::value;
#endif

// is_fixed_storage_container
//   trait: capacity is fixed at runtime - capacity() exposed but
// reserve() not, OR explicit fixed_capacity tag set.  Excludes
// static-storage matches.
template<typename _Type>
struct is_fixed_storage_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    !is_static_storage_container<clean_type>::value
          && (    has_fixed_capacity_tag<clean_type>::value
               || (    has_capacity_method_signal<clean_type>::value
                    && !has_reserve_method_signal<clean_type>::value ) ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_fixed_storage_container_v =
        is_fixed_storage_container<_Type>::value;
#endif


// is_dynamic_storage_container
//   trait: heap-allocated, growable storage.  Both capacity() and
// reserve() must be present, and the type must not also match
// static-storage.
template<typename _Type>
struct is_dynamic_storage_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    !is_static_storage_container<clean_type>::value
          &&  has_capacity_method_signal<clean_type>::value
          &&  has_reserve_method_signal<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_dynamic_storage_container_v =
        is_dynamic_storage_container<_Type>::value;
#endif


// ===========================================================================
// IV.  storage_kind_of
// ===========================================================================

NS_INTERNAL

    template<typename _Type>
    struct storage_kind_helper
    {
        using clean_type = clean_t<_Type>;

        static constexpr storage_kind value =
            is_static_storage_container<clean_type>::value
                ? storage_kind::static_storage
            : is_fixed_storage_container<clean_type>::value
                ? storage_kind::fixed_storage
            : is_dynamic_storage_container<clean_type>::value
                ? storage_kind::dynamic_storage
            : storage_kind::unknown;
    };

NS_END  // internal


// storage_kind_of
//   trait: returns the storage_kind classification of _Type.
template<typename _Type>
struct storage_kind_of
{
    static constexpr storage_kind value =
        internal::storage_kind_helper<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr storage_kind storage_kind_of_v =
        storage_kind_of<_Type>::value;
#endif


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct container_storage_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_extent =
        has_compile_time_extent<clean_type>::value;
    static constexpr bool has_capacity =
        has_capacity_method_signal<clean_type>::value;
    static constexpr bool has_reserve =
        has_reserve_method_signal<clean_type>::value;
    static constexpr bool is_static =
        is_static_storage_container<clean_type>::value;
    static constexpr bool is_fixed =
        is_fixed_storage_container<clean_type>::value;
    static constexpr bool is_dynamic =
        is_dynamic_storage_container<clean_type>::value;
    static constexpr storage_kind kind =
        storage_kind_of<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_STORAGE_TRAITS_