/******************************************************************************
* djinterp [container]                             sorted_container_traits.hpp
*
* SFINAE structural traits for the sorted / unsorted axis.
*   A container is "sorted" when it maintains a comparison-based
* ordering invariant on its elements.  Detection signals:
*     1. nested key_compare alias
*           - present on associative containers like std::set,
*           std::map, std::multiset, std::multimap.
*     2. nested value_compare alias
*           - std::map / std::multimap expose this.
*     3. opt-in `is_sorted_container` member alias equal to
*        std::true_type.
*   Anti-signal:
*     - nested hasher alias  - hash-based containers exclude
*       themselves even if they happen to expose key_compare.
*   The unsorted classification fires when the type looks like a
* container (has size()) and exhibits none of the sort signals.
*   The sorted axis is orthogonal to all other axes.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/sorted_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_SORTED_CONTAINER_TRAITS_
#define DJINTERP_SORTED_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   SFINAE alias / tag detection
// ===========================================================================

// has_key_compare_alias
//   trait: detects nested `key_compare` alias.
template<typename _Type,
         typename = void>
struct has_key_compare_alias : std::false_type
{};

template<typename _Type>
struct has_key_compare_alias<_Type, void_t<
    typename _Type::key_compare
>> : std::true_type
{};


// has_value_compare_alias
//   trait: detects nested `value_compare` alias.
template<typename _Type,
         typename = void>
struct has_value_compare_alias : std::false_type
{};

template<typename _Type>
struct has_value_compare_alias<_Type, void_t<
    typename _Type::value_compare
>> : std::true_type
{};


// has_hasher_alias
//   trait: detects nested `hasher` alias.  Acts as anti-signal for
// the sorted classification.
template<typename _Type,
         typename = void>
struct has_hasher_alias : std::false_type
{};

template<typename _Type>
struct has_hasher_alias<_Type, void_t<
    typename _Type::hasher
>> : std::true_type
{};


// has_sorted_invariant_tag
//   trait: detects an opt-in `is_sorted_container` member alias
// equal to std::true_type.
template<typename _Type,
         typename = void>
struct has_sorted_invariant_tag : std::false_type
{};

template<typename _Type>
struct has_sorted_invariant_tag<_Type, void_t<
    typename _Type::is_sorted_container
>> : std::is_same<typename _Type::is_sorted_container,
                  std::true_type>
{};


// has_size_for_sort_signal
//   trait: detects size() accessor; the "is a container" guard.
template<typename _Type,
         typename = void>
struct has_size_for_sort_signal : std::false_type
{};

template<typename _Type>
struct has_size_for_sort_signal<_Type, void_t<
    decltype(std::declval<const _Type&>().size())
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_key_compare_alias_v =
        has_key_compare_alias<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_value_compare_alias_v =
        has_value_compare_alias<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_hasher_alias_v =
        has_hasher_alias<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_sorted_invariant_tag_v =
        has_sorted_invariant_tag<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_size_for_sort_signal_v =
        has_size_for_sort_signal<_Type>::value;
#endif


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_sorted_container
//   trait: true if the container maintains a sorted invariant.
template<typename _Type>
struct is_sorted_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    !has_hasher_alias<clean_type>::value
          && (    has_sorted_invariant_tag<clean_type>::value
               || has_key_compare_alias<clean_type>::value
               || has_value_compare_alias<clean_type>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_sorted_container_v =
        is_sorted_container<_Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// is_unsorted_container
//   trait: true if the type looks like a container but exposes
// no sorted invariant signal.
template<typename _Type>
struct is_unsorted_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_size_for_sort_signal<clean_type>::value
          && !is_sorted_container<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_unsorted_container_v =
        is_unsorted_container<_Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct sorted_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_key_compare =
        has_key_compare_alias<clean_type>::value;
    static constexpr bool has_value_compare =
        has_value_compare_alias<clean_type>::value;
    static constexpr bool has_hasher =
        has_hasher_alias<clean_type>::value;
    static constexpr bool has_sorted_tag =
        has_sorted_invariant_tag<clean_type>::value;
    static constexpr bool is_sorted =
        is_sorted_container<clean_type>::value;
    static constexpr bool is_unsorted =
        is_unsorted_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_SORTED_CONTAINER_TRAITS_