/******************************************************************************
* djinterp [container]                           constexpr_container_traits.hpp
*
*   SFINAE structural traits for compile-time usable containers.
*   A container is "constexpr-capable" when its core query surface
* (size, data access, iteration) can be evaluated in a constant
* expression context.  The constexpr axis is orthogonal to:
*     - mutability  (a constexpr container may be either immutable
*                    or mutable; relaxed-constexpr in C++14+ permits
*                    constexpr mutators)
*     - iterability (a constexpr container may or may not expose
*                    begin()/end())
*     - storage     (a constexpr container is by definition static-
*                    storage, but the converse does not hold)
*   Detection priority:
*     1. Opt-in `is_constexpr_container` member alias on the type.
*     2. has_constexpr_iteration from constexpr_iterator_traits.hpp.
*     3. Structural test via std::integral_constant on T{}.size()
*        (requires _Type to be default-constructible AND a literal
*        type; falls back to false_type otherwise).
*
*   PORTABILITY:
*   C++11 baseline.  All `_v` aliases gated on
* D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES.
*
*
* path:      /inc/djinterp/core/container/traits/constexpr_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CONSTEXPR_CONTAINER_TRAITS_
#define DJINTERP_CONSTEXPR_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../iterator/constexpr_iterator_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   SFINAE method / tag detection
// ===========================================================================

// has_constexpr_container_tag
//   trait: detects an opt-in `is_constexpr_container` member alias
// equal to std::true_type.
template<typename _Type,
         typename = void>
struct has_constexpr_container_tag : std::false_type
{};

template<typename _Type>
struct has_constexpr_container_tag<_Type, void_t<
    typename _Type::is_constexpr_container
>> : std::is_same<typename _Type::is_constexpr_container,
                  std::true_type>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_constexpr_container_tag_v =
        has_constexpr_container_tag<_Type>::value;
#endif


// has_constexpr_extent
//   trait: detects a compile-time `extent` static constexpr member
// (mirrors our array<>::extent and std::span<>::extent conventions).
template<typename _Type,
         typename = void>
struct has_constexpr_extent : std::false_type
{};

template<typename _Type>
struct has_constexpr_extent<_Type, void_t<
    decltype(_Type::extent)
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_constexpr_extent_v =
        has_constexpr_extent<_Type>::value;
#endif


// has_constexpr_size_expression
//   trait: structural test that T{}.size() yields a value valid in
// a constant expression.  The test requires _Type to be both
// default-constructible AND a literal type; otherwise reports
// false_type - never compile errors.
NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct cx_size_check : std::false_type
    {};

    template<typename _Type>
    struct cx_size_check<_Type, void_t<
        std::integral_constant<std::size_t, _Type{}.size()>
    >> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct has_constexpr_size_expression
    : internal::cx_size_check<clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_constexpr_size_expression_v =
        has_constexpr_size_expression<_Type>::value;
#endif


// ===========================================================================
// II.  Classification umbrella
// ===========================================================================

// is_constexpr_container
//   trait: true if the container is usable in constant evaluation
// contexts.  Combines the opt-in tag, the constexpr_iteration
// trait from constexpr_iterator_traits.hpp, and the structural
// constexpr-size probe.
template<typename _Type>
struct is_constexpr_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_constexpr_container_tag<clean_type>::value    ||
          has_constexpr_iteration<clean_type>::value        ||
          has_constexpr_size_expression<clean_type>::value  ||
          has_constexpr_extent<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_constexpr_container_v =
        is_constexpr_container<_Type>::value;
#endif


// is_not_constexpr_container
//   trait: explicit negation; useful in disjoint requires-clauses
// and SFINAE branches.
template<typename _Type>
struct is_not_constexpr_container
    : std::integral_constant<bool,
          !is_constexpr_container<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_not_constexpr_container_v =
        is_not_constexpr_container<_Type>::value;
#endif


// ===========================================================================
// III. Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct constexpr_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_tag =
        has_constexpr_container_tag<clean_type>::value;
    static constexpr bool has_extent =
        has_constexpr_extent<clean_type>::value;
    static constexpr bool has_size_expr =
        has_constexpr_size_expression<clean_type>::value;
    static constexpr bool has_iteration =
        has_constexpr_iteration<clean_type>::value;
    static constexpr bool is_constexpr =
        is_constexpr_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONSTEXPR_CONTAINER_TRAITS_