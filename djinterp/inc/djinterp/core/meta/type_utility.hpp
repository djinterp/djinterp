/*******************************************************************************
* djinterp [core]                                               type_utility.hpp
*
* The framework's foundational C++ type utilities: the void_t detection sink,
* compile-time absolute value, type cleaning, a constexpr swap, type
* repetition, and self-reference resolution. They lived in the C++ root,
* djinterp.hpp, until decision 4.4 moved them here: the root is the macro kit,
* and these are metaprogramming.
*   Each is present from the language level that can express it and absent
* below, never an error: abs_value, clean, self and resolve_self (with its raw
* pointer form) from C++98; void_t, clean_t, constexpr_swap, repeat_type,
* is_self, resolve_self_t, and resolve_self's smart-pointer and variadic
* forms from C++11; the variable templates from C++14. A utility present at
* several levels has one implementation for all of them.
*
*
* path:      /inc/djinterp/core/meta/type_utility.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.01
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPE UTILITIES
    --------------
    1.  Detection idiom
         1.  void_t
    2.  Absolute value
         1.  abs_value
         2.  abs_value_v
         3.  abs_value_to_size_t
    3.  Type cleaning
         1.  clean_remove_reference / clean_remove_cv (internal)
         2.  clean
         3.  clean_t
    4.  Constexpr swap
         1.  constexpr_swap
    5.  Type repetition
         1.  repeat_type_helper (internal)
         2.  repeat_type
         3.  repeat_type_t
    6.  Self-reference resolution
         1.  self
         2.  is_self
         3.  is_self_v
         4.  resolve_self
         5.  resolve_self_t
         6.  resolve_self recursive specializations
*/

#ifndef DJINTERP_META_TYPE_UTILITY_HPP
#define DJINTERP_META_TYPE_UTILITY_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root
// std
#include <cstddef>             // std::size_t
#include <limits>              // std::numeric_limits
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <memory>          // std::unique_ptr, std::shared_ptr,
                               // std::weak_ptr
    #include <tuple>           // std::tuple
    #include <type_traits>     // std::conditional, std::integral_constant,
                               // std::is_nothrow_move_*
    #include <utility>         // std::swap
#endif


//==============================================================================
// 1.  TYPE UTILITIES
//==============================================================================
// Foundational type utilities, all declared in namespace djinterp.


NS_DJINTERP

// 1.1    Detection idiom
//------------------------------------------------------------------------------
// 1.1.1
// void_t
//   type: maps any type sequence to void. Used as the SFINAE sink in
// detection idioms; pre-C++17 replacement for std::void_t. An alias template
// over a pack, so C++11 and up; at C++98 detection is sizeof-based
// (core/meta/trait_detect.hpp).
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename...>
    using void_t = void;
#endif

// 1.2    Absolute value
//------------------------------------------------------------------------------
// 1.2.1
// abs_value
//   trait: computes the absolute value of a compile-time integral constant.
// std::numeric_limits, not std::is_integral, checks the type: the two agree
// on every type a template parameter can have, and <limits> is C++98.
template<typename Type,
         Type     N>
struct abs_value
{
    D_STATIC_ASSERT(std::numeric_limits<Type>::is_integer,
                    "Type parameter `Type` must be an integral type.");

    static D_CONSTEXPR_VAR Type value = (N < 0)
                                        ? -(N)
                                        : N;
};

// 1.2.2
// abs_value_v
//   value: convenience variable template for abs_value<Type, N>::value.
// Requires variable templates (C++14).
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type,
             Type     N>
    constexpr Type abs_value_v = abs_value<Type, N>::value;
#endif

// 1.2.3
// abs_value_to_size_t
//   value: convenience variable template that yields the absolute value of N
// as a std::size_t. Requires variable templates (C++14).
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type,
             Type     N>
    constexpr std::size_t abs_value_to_size_t =
        std::integral_constant<std::size_t,
                               abs_value<Type, N>::value>::value;
#endif

// 1.3    Type cleaning
//------------------------------------------------------------------------------
// 1.3.1
// clean_remove_reference / clean_remove_cv (internal)
NS_INTERNAL

    // clean_remove_reference
    //   trait: strips one reference (lvalue; rvalue from C++11, where the
    // kind exists). Partial specialization, so it needs no <type_traits>.
    template<typename Type>
    struct clean_remove_reference
    {
        typedef Type type;
    };

    // clean_remove_reference<Type&>
    //   trait: the lvalue-reference case.
    template<typename Type>
    struct clean_remove_reference<Type&>
    {
        typedef Type type;
    };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // clean_remove_reference<Type&&>
    //   trait: the rvalue-reference case.
    template<typename Type>
    struct clean_remove_reference<Type&&>
    {
        typedef Type type;
    };
#endif

    // clean_remove_cv
    //   trait: strips top-level const and volatile; an array's element
    // qualifiers count as the array's, as they do for std::remove_cv.
    template<typename Type>
    struct clean_remove_cv
    {
        typedef Type type;
    };

    // clean_remove_cv<const Type>
    //   trait: the const case.
    template<typename Type>
    struct clean_remove_cv<const Type>
    {
        typedef Type type;
    };

    // clean_remove_cv<volatile Type>
    //   trait: the volatile case.
    template<typename Type>
    struct clean_remove_cv<volatile Type>
    {
        typedef Type type;
    };

    // clean_remove_cv<const volatile Type>
    //   trait: the const volatile case.
    template<typename Type>
    struct clean_remove_cv<const volatile Type>
    {
        typedef Type type;
    };

NS_END  // internal

// 1.3.2
// clean
//   trait: strips references, then cv-qualifiers: the type std::remove_cv_t<
// std::remove_reference_t<Type>> names, at every level.
template<typename Type>
struct clean
{
    typedef typename internal::clean_remove_cv<
        typename internal::clean_remove_reference<Type>::type>::type type;
};

// 1.3.3
// clean_t
//   type: convenience alias for clean<Type>::type; an alias template, so
// C++11 and up.
//   The `typename` is required below C++20 -- P0634 made it optional in this
// position, so omitting it silently pins the header to C++20.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Type>
    using clean_t = typename clean<Type>::type;
#endif

// 1.4    Constexpr swap
//------------------------------------------------------------------------------
// 1.4.1
// constexpr_swap
//   function: swaps two values in a constexpr context. std::swap is not
// constexpr until C++20, so this provides the same semantics for C++14 and
// C++17; C++11 gets the same body without constexpr, since C++11 constexpr
// forbids local variables and assignment. A move-based swap with a noexcept
// condition, so C++11 and up: below, std::swap is the swap.
//   each tier's declaration carries its own contract: clang attaches a doc
// comment only to a declaration that directly follows it, never across an
// #if.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    /**
     * @brief Swaps two values in a constant expression, via `std::swap`.
     *
     * @param[in,out] _a  the first value; receives the second's value.
     * @param[in,out] _b  the second value; receives the first's value.
     */
    template<typename Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
        Type& _a,
        Type& _b
    ) noexcept( std::is_nothrow_move_constructible<Type>::value &&
                std::is_nothrow_move_assignable<Type>::value )
    {
        std::swap(_a,
                  _b);

        return;
    }

#elif D_ENV_LANG_IS_CPP14_OR_HIGHER

    /**
     * @brief Swaps two values in a constant expression, via a move-based
     *        swap under relaxed constexpr.
     *
     * @param[in,out] _a  the first value; receives the second's value.
     * @param[in,out] _b  the second value; receives the first's value.
     */
    template<typename Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
        Type& _a,
        Type& _b
    ) noexcept( std::is_nothrow_move_constructible<Type>::value &&
                std::is_nothrow_move_assignable<Type>::value )
    {
        Type temp = static_cast<Type&&>(_a);
        _a        = static_cast<Type&&>(_b);
        _b        = static_cast<Type&&>(temp);

        return;
    }

#elif D_ENV_LANG_IS_CPP11_OR_HIGHER

    /**
     * @brief Swaps two values via a move-based swap; not constexpr, since
     *        C++11 constexpr forbids local variables and assignment.
     *
     * @param[in,out] _a  the first value; receives the second's value.
     * @param[in,out] _b  the second value; receives the first's value.
     */
    template<typename Type>
    D_INLINE void
    constexpr_swap(
        Type& _a,
        Type& _b
    ) noexcept( std::is_nothrow_move_constructible<Type>::value &&
                std::is_nothrow_move_assignable<Type>::value )
    {
        Type temp = static_cast<Type&&>(_a);
        _a        = static_cast<Type&&>(_b);
        _b        = static_cast<Type&&>(temp);

        return;
    }

#endif

// 1.5    Type repetition
//------------------------------------------------------------------------------
// variadic templates and std::tuple, so C++11 and up.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.5.1
// repeat_type_helper (internal)
NS_INTERNAL

    // repeat_type_helper
    //   trait: internal recursive builder for repeat_type<>
    // (general case).
    template<typename    Type,
             std::size_t N,
             typename... Types>
    struct repeat_type_helper
    {
        using type = typename repeat_type_helper<Type,
                                                 (N - 1),
                                                 Type,
                                                 Types...>::type;
    };

    // repeat_type_helper<Type, 0, Types...>
    //   trait: base case specialization producing the final
    // tuple.
    template<typename    Type,
             typename... Types>
    struct repeat_type_helper<Type, 0, Types...>
    {
        using type = std::tuple<Types...>;
    };

NS_END  // internal

// 1.5.2
// repeat_type
//   trait: produces a std::tuple containing Type repeated
// NumTimes times.
//
//   NOT named `repeat`: functional/producer.hpp carries an infinite
// producer factory `repeat(_Value&&)` at this same namespace scope,
// and a class template and a function template of one name in one
// namespace is ill-formed - a class NAME may be hidden by a function
// name, a class TEMPLATE name may not.  This trait is the type-level
// one, so it takes the qualified spelling and the factory keeps the
// bare verb.
template<typename    Type,
         std::size_t NumTimes>
struct repeat_type
{
    // std::conditional_t is C++14; ::type is the C++11 spelling.
    typedef typename std::conditional<(NumTimes > 0),
        typename internal::repeat_type_helper<Type,
                                              NumTimes>::type,
        std::tuple<>
    >::type type;
};

// 1.5.3
// repeat_type_t
//   type: convenience alias for repeat_type<Type, NumTimes>::type.
template<typename    Type,
         std::size_t NumTimes>
using repeat_type_t = typename repeat_type<Type, NumTimes>::type;

#endif  // C++11: type repetition

// 1.6    Self-reference resolution
//------------------------------------------------------------------------------
// 1.6.1
// self
//   type: self-reference marker for recursive type definitions.
struct self
{};

// 1.6.2
// is_self
//   trait: detects the self marker type (primary template). A
// std::integral_constant, as the standard's traits are, so C++11 and up:
// below, resolve_self is the self-aware facility.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Type>
    struct is_self : std::false_type
    {};

    // is_self<self>
    //   trait: specialization recognizing the self marker.
    template<>
    struct is_self<self> : std::true_type
    {};
#endif

// 1.6.3
// is_self_v
//   value: convenience alias for is_self<Type>::value.
//
//   C++14 AND ABOVE. This is a variable TEMPLATE, and variable templates are a
// C++14 feature -- there is no C++11 spelling to fall back to, so the gate is
// D1 working as intended rather than a portability workaround: a higher tier
// ADDS the convenience and the floor keeps the trait it is a convenience for.
// Below 14, write is_self<T>::value, which is what this expands to anyway.
//
//   D_CONSTEXPR, not D_CONSTEXPR_INLINE. A variable template's instantiations
// already merge under vague linkage, so `inline` buys nothing -- and D_INLINE's
// always_inline is a FUNCTION attribute the compiler discards on a variable,
// warning on every TU that includes this header.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    template<typename Type>
    D_CONSTEXPR bool is_self_v = is_self<Type>::value;
#endif

// 1.6.4
// resolve_self
//   trait: resolves the self marker within a type to a concrete
// target type (primary template, passthrough).
template<typename Type,
         typename ResolveTo>
struct resolve_self
{
    typedef Type type;
};

// resolve_self<self, ResolveTo>
//   trait: base case -- self resolves to ResolveTo.
template<typename ResolveTo>
struct resolve_self<self, ResolveTo>
{
    typedef ResolveTo type;
};

// resolve_self<self*, ResolveTo>
//   trait: raw pointer specialization.
template<typename ResolveTo>
struct resolve_self<self*, ResolveTo>
{
    typedef ResolveTo* type;
};

// the smart pointers are C++11's
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // resolve_self<std::unique_ptr<self>, ResolveTo>
    //   trait: smart pointer specialization for unique_ptr.
    template<typename ResolveTo>
    struct resolve_self<std::unique_ptr<self>, ResolveTo>
    {
        typedef std::unique_ptr<ResolveTo> type;
    };

    // resolve_self<std::shared_ptr<self>, ResolveTo>
    //   trait: smart pointer specialization for shared_ptr.
    template<typename ResolveTo>
    struct resolve_self<std::shared_ptr<self>, ResolveTo>
    {
        typedef std::shared_ptr<ResolveTo> type;
    };

    // resolve_self<std::weak_ptr<self>, ResolveTo>
    //   trait: smart pointer specialization for weak_ptr.
    template<typename ResolveTo>
    struct resolve_self<std::weak_ptr<self>, ResolveTo>
    {
        typedef std::weak_ptr<ResolveTo> type;
    };
#endif

// 1.6.5
// resolve_self_t
//   type: convenience alias for resolve_self<...>::type; C++11 and up.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename Type,
             typename ResolveTo>
    using resolve_self_t = typename resolve_self<Type, ResolveTo>::type;
#endif

// 1.6.6
// resolve_self recursive specializations
//   variadic templates, so C++11 and up.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// resolve_self<Template<Args...>, ResolveTo>
//   trait: variadic catch-all for any class template whose
// parameters are all types. Recursively resolves self within
// each template argument, enabling detection of self inside
// arbitrary std:: containers, wrappers, and user templates
// (e.g. std::vector<self>, std::optional<self>,
// std::pair<int, self>, etc.).
//
// Note: templates with non-type parameters (e.g. std::array)
// require their own dedicated specializations, as this partial
// specialization only matches template<typename...> forms.
//
//   `class`, not `typename`, introduces the template template parameter: the
// C++11 floor accepts only `class` there (`typename` in that position is
// C++17), which is the one exception to the always-`typename` rule.
template<template<typename...> class Template,
         typename...                 Args,
         typename                    ResolveTo>
struct resolve_self<Template<Args...>, ResolveTo>
{
    using type = Template<resolve_self_t<Args, ResolveTo>...>;
};

// resolve_self<repeat_type<Type, NumTimes>, ResolveTo>
//   trait: recursively resolves self within a repeated type.
template<typename    Type,
         std::size_t NumTimes,
         typename    ResolveTo>
struct resolve_self<repeat_type<Type, NumTimes>, ResolveTo>
{
    using resolved_inner = resolve_self_t<Type, ResolveTo>;
    using type           = repeat_type_t<resolved_inner, NumTimes>;
};

#endif  // C++11: recursive specializations


NS_END  // djinterp


#endif  // DJINTERP_META_TYPE_UTILITY_HPP
