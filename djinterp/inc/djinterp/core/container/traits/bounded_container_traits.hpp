/******************************************************************************
* djinterp [container]                            bounded_container_traits.hpp
*
* SFINAE structural traits for the bounded / unbounded axis.
*   A container is "bounded" when it has a hard upper limit on its
* element count, expressible at the type or instance level.  Three
* kinds of evidence promote a container to bounded:
*     1. compile-time fixed extent  (static `extent` constant)
*     2. a max_size() accessor returning a finite, non-pathological
*        value
*     3. a capacity() accessor without a paired reserve() - i.e.
*        capacity is fixed, not user-extensible.
*   The negation gives the unbounded classification (heap-backed
* growable storage like std::list, std::deque, growable
* std::vector - whose nominal max_size() is SIZE_MAX and is treated
* as effectively unbounded).
*   The bounded axis is orthogonal to all other axes.
*
*  PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/bounded_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_BOUNDED_CONTAINER_TRAITS_
#define DJINTERP_BOUNDED_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   SFINAE method / extent detection
// ===========================================================================

// has_fixed_extent_signal
//   trait: detects a compile-time `extent` static constexpr
// member.
template<typename _Type,
         typename = void>
struct has_fixed_extent_signal : std::false_type
{};

template<typename _Type>
struct has_fixed_extent_signal<_Type, void_t<
    decltype(_Type::extent)
>> : std::true_type
{};


// has_max_size_signal
//   trait: detects a max_size() accessor on a const lvalue.
template<typename _Type,
         typename = void>
struct has_max_size_signal : std::false_type
{};

template<typename _Type>
struct has_max_size_signal<_Type, void_t<
    decltype(std::declval<const _Type&>().max_size())
>> : std::true_type
{};


// has_capacity_signal
//   trait: detects a capacity() accessor on a const lvalue.
template<typename _Type,
         typename = void>
struct has_capacity_signal : std::false_type
{};

template<typename _Type>
struct has_capacity_signal<_Type, void_t<
    decltype(std::declval<const _Type&>().capacity())
>> : std::true_type
{};


// has_reserve_signal
//   trait: detects a reserve(size_type) call.  Anti-signal for
// "bounded by fixed capacity".
template<typename _Type,
         typename = void>
struct has_reserve_signal : std::false_type
{};

template<typename _Type>
struct has_reserve_signal<_Type, void_t<
    decltype(std::declval<_Type&>().reserve(std::size_t{}))
>> : std::true_type
{};


// has_size_signal
//   trait: detects a size() accessor; used as the umbrella
// "looks like a container" guard.
template<typename _Type,
         typename = void>
struct has_size_signal : std::false_type
{};

template<typename _Type>
struct has_size_signal<_Type, void_t<
    decltype(std::declval<const _Type&>().size())
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_fixed_extent_signal_v =
        has_fixed_extent_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_max_size_signal_v =
        has_max_size_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_capacity_signal_v =
        has_capacity_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_reserve_signal_v =
        has_reserve_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_size_signal_v =
        has_size_signal<_Type>::value;
#endif


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_bounded_container
//   trait: true if the container has any hard upper bound signal.
template<typename _Type>
struct is_bounded_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_fixed_extent_signal<clean_type>::value
          || has_max_size_signal<clean_type>::value
          || (    has_capacity_signal<clean_type>::value
               && !has_reserve_signal<clean_type>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_bounded_container_v =
        is_bounded_container<_Type>::value;
#endif


// is_unbounded_container
//   trait: true if the type looks like a container (has size())
// but exposes none of the bounding signals.
template<typename _Type>
struct is_unbounded_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_size_signal<clean_type>::value
          && !is_bounded_container<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_unbounded_container_v =
        is_unbounded_container<_Type>::value;
#endif


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct bounded_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_extent =
        has_fixed_extent_signal<clean_type>::value;
    static constexpr bool has_max_size =
        has_max_size_signal<clean_type>::value;
    static constexpr bool has_capacity =
        has_capacity_signal<clean_type>::value;
    static constexpr bool has_reserve =
        has_reserve_signal<clean_type>::value;
    static constexpr bool is_bounded =
        is_bounded_container<clean_type>::value;
    static constexpr bool is_unbounded =
        is_unbounded_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_BOUNDED_CONTAINER_TRAITS_