/******************************************************************************
* djinterp [container]                           iterable_container_traits.hpp
*
* SFINAE structural traits for the iterability axis.
*   A container is "iterable" when it exposes a begin/end pair
* compatible with range-based for loops and STL-style algorithms.
* "Non-iterable" containers expose data()/size()/operator[] only.
*   The iterability axis is fully orthogonal to:
*     - constexpr   (an iterable may or may not be constexpr; see
*                    constexpr_iterator_traits.hpp for the
*                    intersection)
*     - mutability  (iteration may be const-only or mutable)
*     - storage     (any storage kind may be iterable)
*
*   Detection signals:
*     1. has_begin_method         - begin() callable on const&
*     2. has_end_method           - end()   callable on const&
*     3. has_iterator_alias       - nested `iterator` type alias
*     4. has_const_iterator_alias - nested `const_iterator` alias
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/traits/iterable_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_ITERABLE_CONTAINER_TRAITS_
#define DJINTERP_ITERABLE_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   SFINAE method / alias detection
// ===========================================================================

// has_begin_method
template<typename _Type,
         typename = void>
struct has_begin_method : std::false_type
{};

template<typename _Type>
struct has_begin_method<_Type, void_t<
    decltype(std::declval<const _Type&>().begin())
>> : std::true_type
{};

// has_end_method
template<typename _Type,
         typename = void>
struct has_end_method : std::false_type
{};

template<typename _Type>
struct has_end_method<_Type, void_t<
    decltype(std::declval<const _Type&>().end())
>> : std::true_type
{};

// has_iterator_alias
template<typename _Type,
         typename = void>
struct has_iterator_alias : std::false_type
{};

template<typename _Type>
struct has_iterator_alias<_Type, void_t<
    typename _Type::iterator
>> : std::true_type
{};

// has_const_iterator_alias
template<typename _Type,
         typename = void>
struct has_const_iterator_alias : std::false_type
{};

template<typename _Type>
struct has_const_iterator_alias<_Type, void_t<
    typename _Type::const_iterator
>> : std::true_type
{};

// has_value_type_alias
template<typename _Type,
         typename = void>
struct has_value_type_alias : std::false_type
{};

template<typename _Type>
struct has_value_type_alias<_Type, void_t<
    typename _Type::value_type
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_begin_method_v =
        has_begin_method<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_end_method_v =
        has_end_method<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_iterator_alias_v =
        has_iterator_alias<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_const_iterator_alias_v =
        has_const_iterator_alias<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_value_type_alias_v =
        has_value_type_alias<_Type>::value;
#endif


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_iterable_container
//   trait: true if the container exposes both begin() and end()
// callable on a const lvalue.
template<typename _Type>
struct is_iterable_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_begin_method<clean_type>::value
          && has_end_method<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_iterable_container_v =
        is_iterable_container<_Type>::value;
#endif


// is_non_iterable_container
//   trait: true if the type looks like a container (has
// value_type) but does NOT expose begin()/end().  The conjunction
// with has_value_type_alias guards against arbitrary non-container
// types being mis-classified as non-iterable.
template<typename _Type>
struct is_non_iterable_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        (    has_value_type_alias<clean_type>::value
          && !is_iterable_container<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_non_iterable_container_v =
        is_non_iterable_container<_Type>::value;
#endif


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct iterable_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_begin =
        has_begin_method<clean_type>::value;
    static constexpr bool has_end =
        has_end_method<clean_type>::value;
    static constexpr bool has_iter_alias =
        has_iterator_alias<clean_type>::value;
    static constexpr bool has_const_iter_alias =
        has_const_iterator_alias<clean_type>::value;
    static constexpr bool is_iterable =
        is_iterable_container<clean_type>::value;
    static constexpr bool is_non_iterable =
        is_non_iterable_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_ITERABLE_CONTAINER_TRAITS_