/*******************************************************************************
* djinterp [core]                                                     dtuple.hpp
*
* djinterp tuple module
*   This header is intended to supplement the `std::tuple` general utility
* library.
*
*   All metafunctions herein are designed to work with C++11 and later, using
* portable trait access patterns i.e. (`::value` instead of `_v` suffixes)
* where appropriate.
*
*   compile-time tuple operations. It includes:
*   - tuple joining and concatenation (tuple_join, tuple_concat)
*   - element access (tuple_type_at, tuple_type_at_value)
*   - type counting and removal (tuple_count_type, tuple_count_and_remove)
*   - tuple splitting (tuple_split, tuple_subsequence)
*   - type transformation (tuple_apply_all, tuple_consolidate_types)
*   - type selection utilities (type_case, type_selector)
*   - 2D/jagged tuple support (is_2d_tuple, tuple_inner_sizes)
*
* PORTABILITY:
*   version: C++11 or higher
*   dependencies:
*   - `env.h`:          for C++ version detection.
*   - `cpp_features.h`: fine-grained C++ feature detection.
*
*   All metafunctions herein are designed to work with C++11 and later, using
* portable trait access patterns i.e. (`::value` instead of `_v` suffixes)
* where appropriate.
*
*
provides advanced tuple manipulation metafunctions for
* compile-time tuple operations. It includes:
*   - tuple joining and concatenation (tuple_join, tuple_concat)
*   - element access (tuple_type_at, tuple_type_at_value)
*   - type counting and removal (tuple_count_type, tuple_count_and_remove)
*   - tuple splitting (tuple_split, tuple_subsequence)
*   - type transformation (tuple_apply_all, tuple_consolidate_types)
*   - type selection utilities (type_case, type_selector)
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and cpp_features.h for
* fine-grained feature detection. All metafunctions use portable trait access
* patterns (::value instead of _v suffixes) for compatibility with C++11 and
* later.
*
* djinterp tuple utility header:
*   This header provides tuple-related type trait utilities and metafunctions
* for compile-time tuple manipulation. It includes:
*   - tuple construction (make_tuple_of, repeat)
*   - type modifiers (wrap_all, to_lvalue_reference, to_pointer, etc.)
*
*   Note: first_arg, is_tuple, is_single_tuple_arg, and to_tuple now live
* in type_traits.hpp.
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and cpp_features.h for
* fine-grained feature detection. All metafunctions are designed to work with
* C++11 and later, using portable trait access patterns.
*
*
* path:      /inc/djinterp/core/meta/dtuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_DTUPLE_HPP
#define DJINTERP_META_DTUPLE_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <algorithm>
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./type_utility.hpp"  // repeat_type_t, clean_t
#include "./type_traits.hpp"
#include "./trait_detect.hpp"   // D_TYPE_TRAIT_TYPE_ALIAS, _TRUE_AS, _IS_SPECIALIZATION_OF_AS


NS_DJINTERP

// =========================================================================
// FORWARD DECLARATIONS
// =========================================================================
// Note: first_arg, is_tuple, is_single_tuple_arg, and to_tuple are defined
// in type_traits.hpp (included above).

template<template<typename> class...    Modifiers>
struct wrap_all;

template<template<typename> class       Modifier,
            template<typename> class...    Modifiers>
struct wrap_all<Modifier, Modifiers...>;

template<>
struct wrap_all<>;


// =========================================================================
// II.  PARAMETER PACK UTILITIES
// =========================================================================

// is_tuple_single_arg
//   type trait: evaluates to `std::true_type` if the parameter pack
// consists of exactly one argument; otherwise `std::false_type`.
template<typename... Types>
struct is_tuple_single_arg : std::false_type
{};

template<typename Type>
struct is_tuple_single_arg<Type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_tuple_single_arg
//
template<typename... Types>
constexpr bool is_tuple_single_arg_v = is_tuple_single_arg<Types...>::value;
#endif



// =========================================================================
// V.   TUPLE CONSTRUCTION
// =========================================================================

// make_tuple_of
//   type trait: creates a `std::tuple` containing `Count` copies of
// `Type`.
template<typename    Type,
         std::size_t Count>
struct make_tuple_of;

template<typename Type>
struct make_tuple_of<Type, 0>
{
    using type = std::tuple<>;
};

template<typename Type>
struct make_tuple_of<Type, 1>
{
    using type = to_tuple_t<Type>;
};

// make_tuple_of  (general case, Count >= 2)
//   type trait: produces std::tuple<Type, Type, ...> with
// Count copies of Type.  The earlier implementation used a
// comma-operator trick that, on some compilers, caused the
// element type to decay to `Type&&` - and a tuple of rvalue
// references cannot be default-constructed, producing the
// diagnostic "no instance of constructor std::tuple<...>
// matches the argument list" at the `{}` initialization site.
//
//   The replacement delegates directly to `repeat_type_t` (from
// djinterp.hpp), which produces a well-formed
// `std::tuple<Type, Type, ...>` without any reference
// qualification or runtime construction step.  The
// specializations for Count == 0 and Count == 1 above
// are retained for clarity and produce identical results
// to `repeat_type_t<Type, 0>` and `repeat_type_t<Type, 1>`
// respectively, so more-specific matching picks them up
// first when applicable.
template<typename    Type,
         std::size_t Count>
struct make_tuple_of
{
    using type = repeat_type_t<Type, Count>;
};

// make_tuple_of_t
//   alias template: shorthand for `make_tuple_of<Type, Count>::type`.
template<typename    Type,
         std::size_t Count>
using make_tuple_of_t = typename make_tuple_of<Type, Count>::type;


// =========================================================================
// VI.  TYPE MODIFIERS
// =========================================================================

// wrap_all
//   type trait: applies a series of type transformations left-to-right,
// where right is the innermost and left is the outermost.
// Example: wrap_all<X, Y, Z>::template type<int> == X<Y<Z<int>>>
template<template<typename> class...    Modifiers>
struct wrap_all
{
    template<typename Type>
    using type = Type;
};

template<template<typename> class       Modifier,
         template<typename> class...    Modifiers>
struct wrap_all<Modifier, Modifiers...>
{
    template<typename Type>
    using type = typename Modifier<
        typename wrap_all<Modifiers...>::template type<Type>
    >::type;
};

template<>
struct wrap_all<>
{
    template<typename Type>
    using type = Type;
};

// wrap_all_t
//   alias template: shorthand for applying wrap_all.
template<typename                        Type,
         template<typename> class...    Modifiers>
using wrap_all_t = typename wrap_all<Modifiers...>::template type<Type>;

// to_lvalue_reference
//   type modifier: converts a type to an lvalue reference, removing any
// existing reference first.
struct to_lvalue_reference
{
    template<typename Type>
    using type = typename wrap_all<
        std::add_lvalue_reference,
        std::remove_reference
    >::template type<Type>;
};

// to_rvalue_reference
//   type modifier: converts a type to an rvalue reference, removing any
// existing reference first.
struct to_rvalue_reference
{
    template<typename Type>
    using type = typename wrap_all<
        std::add_rvalue_reference,
        std::remove_reference
    >::template type<Type>;
};

// to_pointer
//   type modifier: converts a type to a pointer, removing any existing
// pointer first.
struct to_pointer
{
    template<typename Type>
    using type = typename wrap_all<
        std::add_pointer,
        std::remove_pointer
    >::template type<Type>;
};

// to_type
//   type trait: identity wrapper that simply exposes a `type` member alias.
// Useful for metaprogramming contexts where a type wrapper is expected.
template<typename Type>
struct to_type
{
    using type = Type;
};

// to_type_t
//   alias template: shorthand for `to_type<Type>::type`.
template<typename Type>
using to_type_t = typename to_type<Type>::type;

// forward declaractions
template<typename    Type,
         typename... Types>
struct tuple_count_and_remove;

template<typename    Type,
         typename... Types>
struct tuple_first_type;

NS_INTERNAL
    // tuple_join_helper
    //   internal helper: flattens a `std::tuple` whose elements may be
    // `std::tuple<...>` or single types, producing one `std::tuple<...>`.
    template<typename Tuple,
             typename... Types>
    struct tuple_join_helper;

    // base case: no more elements to process
    template<typename... Types>
    struct tuple_join_helper<std::tuple<>, Types...>
    {
        using type = std::tuple<Types...>;
    };

    // case: head is a std::tuple<...> => append its elements
    template<typename... Head,
             typename... Tail,
             typename... Types>
    struct tuple_join_helper<std::tuple<std::tuple<Head...>, Tail...>, Types...>
    {
        using type = typename tuple_join_helper<std::tuple<Tail...>, Types..., Head...>::type;
    };

    // case: head is a single type => append it
    template<typename    Head,
             typename... Tail,
             typename... Types>
    struct tuple_join_helper<std::tuple<Head, Tail...>, Types...>
    {
        using type = typename tuple_join_helper<std::tuple<Tail...>, Types..., Head>::type;
    };
NS_END  // internal

// tuple_join
//   type trait: joins/concatenates the types of one or more tuple-like
// inputs. Each argument that is a `std::tuple<...>` contributes its
// elements; non-tuple arguments contribute themselves as one element.
template<typename... Tuples>
struct tuple_join
{
    using type = typename internal::tuple_join_helper<std::tuple<Tuples...>>::type;
};



// =========================================================================
// II.  TUPLE TRANSFORMATION
// =========================================================================

// tuple_apply_all (internal helper)
NS_INTERNAL

    template<template<typename...> class    UnaryTrait,
             typename                       Tuple,
             typename...                    Types>
    struct tuple_apply_all_helper;

    template<template<typename...> class    UnaryTrait,
             typename...                    Types>
    struct tuple_apply_all_helper<UnaryTrait, std::tuple<>, Types...>
    {
        using type = std::tuple<Types...>;
    };

    template<template<typename...> class    UnaryTrait,
             typename                       Head,
             typename...                    Tails,
             typename...                    Types>
    struct tuple_apply_all_helper<UnaryTrait, std::tuple<Head, Tails...>, Types...>
    {
        using type = typename tuple_apply_all_helper<
            UnaryTrait,
            std::tuple<Tails...>,
            Types...,
            UnaryTrait<Head>
        >::type;
    };

NS_END  // internal

// tuple_apply_all
//   type trait: applies a unary type trait to all elements of a tuple.
template<template<typename...> class    UnaryTrait,
         typename...                    Types>
struct tuple_apply_all
{
    using type = typename internal::tuple_apply_all_helper<
        UnaryTrait,
        typename to_tuple<Types...>::type
    >::type;
};

// tuple_apply_all_t
//   alias template: shorthand for `tuple_apply_all<...>::type`.
template<template<typename...> class    UnaryTrait,
         typename...                    Types>
using tuple_apply_all_t = typename tuple_apply_all<UnaryTrait, Types...>::type;


// =========================================================================
// III. ELEMENT ACCESS
// =========================================================================

// tuple_type_at (internal helper)
NS_INTERNAL
    template<std::size_t Index,
             typename    Tuple>
    struct tuple_type_at_helper;

    template<typename    Head,
             typename... Tail>
    struct tuple_type_at_helper<0, std::tuple<Head, Tail...>>
    {
        using type = Head;

        static constexpr auto value(const std::tuple<Head, Tail...>& _t)
        {
            return std::get<0>(_t);
        }
    };

    template<std::size_t Index,
             typename    Head,
             typename... Tail>
    struct tuple_type_at_helper<Index, std::tuple<Head, Tail...>>
    {
        using type = typename tuple_type_at_helper<Index - 1, std::tuple<Tail...>>::type;

        // `value` mirrors the base-case specialization above so that
        // `tuple_type_at_value<Index>(_t)` works for any in-bounds
        // index, not just 0.  Originally this specialization defined
        // only `type` and left `value` to the base case, but the
        // public wrapper `tuple_type_at_value` calls
        // `tuple_type_at_helper<Index, Tuple>::value(_t)` directly
        // (without bottoming out the index recursion at the call
        // site), so any non-zero index hit a missing-member error.
        // Since `_t` is the FULL original tuple at every recursion
        // level (the recursion is purely over `Index`, not over a
        // tail view of the tuple), `std::get<Index>(_t)` is the
        // direct and correct retrieval.
        static constexpr auto value(const std::tuple<Head, Tail...>& _t)
        {
            return std::get<Index>(_t);
        }
    };

NS_END  // internal

// tuple_type_at
//   type trait: retrieves the type at a specific index in a tuple.
template<std::size_t Index,
         typename... Types>
struct tuple_type_at
{
private:
    using tuple_type = to_tuple_t<Types...>;

    static_assert((Index < std::tuple_size<tuple_type>::value),
                    "Non-type parameter `Index` cannot be greater than or "
                    "equal to the tuple size of type parameter `Tuple`.");

public:
    using type = typename internal::tuple_type_at_helper<Index, tuple_type>::type;
};

// tuple_type_at_t
//   alias template: shorthand for `tuple_type_at<...>::type`.
template<std::size_t Index,
         typename... Types>
using tuple_type_at_t = typename tuple_type_at<Index, Types...>::type;

// tuple_type_at_value
//   function: retrieves the value at a specific index in a tuple instance.
template<std::size_t Index,
         typename    Tuple>
constexpr auto tuple_type_at_value(const Tuple& _t)
{
    return internal::tuple_type_at_helper<Index, Tuple>::value(_t);
}

// tuple_concat
//   function: concatenates multiple tuples at compile-time.
template<typename... Tuples>
static constexpr auto tuple_concat(Tuples&&... _tuples)
{
    return std::tuple_cat(std::forward<Tuples>(_tuples)...);
}


// =========================================================================
// IV.  TYPE COUNTING AND FILTERING
// =========================================================================

// tuple_consolidate_types (internal helper)
NS_INTERNAL

    template<typename Tuple,
             typename... Result>
    struct tuple_consolidate_types_helper;

    template<typename... Types>
    struct tuple_consolidate_types_helper<std::tuple<>, Types...>
    {
        using type = std::tuple<Types...>;
    };

    template<typename    Head,
             typename... Tails,
             typename... Types>
    struct tuple_consolidate_types_helper<std::tuple<Head, Tails...>, Types...>
    {
        using removed = tuple_count_and_remove<Head, Tails...>;
        using type = typename tuple_consolidate_types_helper<
         typename removed::type,
            Types...,
            std::conditional_t<
                (removed::value > 0),
                std::array<Head, removed::value + 1>,
                Head
            >
        >::type;
    };

NS_END  // internal

// tuple_consolidate_types
//   type trait: consolidates repeated types in a tuple into arrays.
template<typename... Types>
struct tuple_consolidate_types
{
    using type = typename internal::tuple_consolidate_types_helper<
        typename to_tuple<Types...>::type
    >::type;
};

// tuple_consolidate_types_t
//   alias template: shorthand for `tuple_consolidate_types<...>::type`.
template<typename... Types>
using tuple_consolidate_types_t = typename tuple_consolidate_types<Types...>::type;

// tuple_count_and_remove (internal helper)
NS_INTERNAL
    template<typename    Type,
             typename    Tuple,
             std::size_t Count,
             typename    Filtered>
    struct tuple_count_and_remove_helper;

    template<typename    Type,
             std::size_t Count,
             typename... Filtered>
    struct tuple_count_and_remove_helper<Type, std::tuple<>, Count, std::tuple<Filtered...>>
    {
        static constexpr std::size_t value = Count;
        using type = std::tuple<Filtered...>;
    };

    template<typename    Type,
             typename    Head,
             typename... Tail,
             std::size_t Count,
             typename... Filtered>
    struct tuple_count_and_remove_helper<Type, std::tuple<Head, Tail...>, Count, std::tuple<Filtered...>>
    {
        using recursive_type = std::conditional_t<
            std::is_same<Type, Head>::value,
            tuple_count_and_remove_helper<Type, std::tuple<Tail...>, Count + 1, std::tuple<Filtered...>>,
            tuple_count_and_remove_helper<Type, std::tuple<Tail...>, Count, std::tuple<Filtered..., Head>>
        >;

        using                        type  = typename recursive_type::type;
        static constexpr std::size_t value = recursive_type::value;
    };

NS_END  // internal

// tuple_count_and_remove
//   type trait: counts occurrences of a type and removes them from a tuple.
template<typename    Type,
         typename... Types>
struct tuple_count_and_remove
{
protected:
    using _type = typename internal::tuple_count_and_remove_helper<
        Type,
        typename to_tuple<Types...>::type,
        0,
        std::tuple<>
    >;

public:
    using type = typename _type::type;
    static constexpr std::size_t value = _type::value;
};

// tuple_count_and_remove_t
//   alias template: shorthand for `tuple_count_and_remove<...>::type`.
template<typename    Type,
         typename... Types>
using tuple_count_and_remove_t = typename tuple_count_and_remove<Type, Types...>::type;

// tuple_count_and_remove_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename    Type,
             typename... Types>
    constexpr std::size_t tuple_count_and_remove_v = tuple_count_and_remove<Type, Types...>::value;
#endif

// tuple_count_type (internal helper)
NS_INTERNAL

    template<typename    Type,
             typename    Tuple,
             std::size_t Count>
    struct tuple_count_type_helper;

    // case: empty
    template<typename    Type,
             std::size_t Count>
    struct tuple_count_type_helper<Type, std::tuple<>, Count>
    {
        static constexpr std::size_t value = 0;
    };

    // case: last element (or tuple of size 1)
    template<typename    Type,
             typename    Head,
             std::size_t Count>
    struct tuple_count_type_helper<Type, std::tuple<Head>, Count>
    {
        static constexpr std::size_t value = std::conditional_t<
            std::is_same<Type, Head>::value,
            std::integral_constant<std::size_t, Count + 1>,
            std::integral_constant<std::size_t, Count>
        >::value;
    };

    // case: recursive
    template<typename    Type,
             typename    Head,
             typename... Tail,
             std::size_t Count>
    struct tuple_count_type_helper<Type, std::tuple<Head, Tail...>, Count>
    {
        static constexpr std::size_t value =
            tuple_count_type_helper<
                Type,
                std::tuple<Tail...>,
                std::conditional_t<
                    std::is_same<Type, Head>::value,
                    std::integral_constant<std::size_t, Count + 1>,
                    std::integral_constant<std::size_t, Count>
                >::value
            >::value;
    };

NS_END  // internal

// tuple_count_type
//   type trait: counts the number of occurrences of a type in a tuple.
template<typename    Type,
         typename... Types>
struct tuple_count_type
{
    static constexpr std::size_t value =
        internal::tuple_count_type_helper<Type,
                                          typename to_tuple<Types...>::type,
                                          0>::value;
};

// tuple_count_type_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename    Type,
                typename... Types>
    constexpr std::size_t tuple_count_type_v = tuple_count_type<Type, Types...>::value;
#endif


// =========================================================================
// V.   TUPLE SPLITTING
// =========================================================================

// tuple_split (internal helper)
NS_INTERNAL

    template<std::size_t Index,
             typename    Before,
             typename    After,
             typename = void>
    struct tuple_split_helper;

    // index = 0
    template<typename... Before,
             typename... After>
    struct tuple_split_helper<0, std::tuple<Before...>, std::tuple<After...>>
    {
        using before = std::tuple<Before...>;
        using after  = std::tuple<After...>;
    };

    // recursive case
    template<std::size_t Index,
             typename... Before,
             typename    Type,
             typename... After>
    struct tuple_split_helper<Index,
                              std::tuple<Before...>,
                              std::tuple<Type, After...>,
                              std::enable_if_t<(Index > 0)>>
    {
        using type = tuple_split_helper<Index - 1, std::tuple<Before..., Type>, std::tuple<After...>>;

        using before = typename type::before;
        using after  = typename type::after;
    };

NS_END  // internal

// tuple_split
//   type trait: splits a tuple at a specified index.
// Given an index I and a tuple of type T with N elements:
// 1. An index between 0 and N-1 results in `before` of size I and `after`
//    of size (N-I).
// 2. An index of 0 results in an empty `before` and `after` equal to T.
// 3. An index of N results in `before` equal to T and an empty `after`.
template<std::size_t Index,
         typename... Types>
struct tuple_split
{
private:
    using tuple_type = to_tuple_t<Types...>;
    static_assert((Index <= std::tuple_size<tuple_type>::value),
                    "`Index` must be less than or equal to the `std::tuple` size.");

public:
    using type   = internal::tuple_split_helper<Index, std::tuple<>, tuple_type>;
    using before = typename type::before;
    using after  = typename type::after;
};

// tuple_split_t
//   alias template: shorthand for `tuple_split<...>::type`.
template<std::size_t Index,
         typename    Tuple>
using tuple_split_t = typename tuple_split<Index, Tuple>::type;

// tuple_subsequence (internal helper)
//
//   The earlier implementation used a recursive helper with two
// partial specializations:
//
//     <End, End, Tuple, Types...>             // base case
//     <Start, End, Tuple, First, Rest...>    // recursive
//
// When mid-recursion reached a state where `Start == End` but
// types remained in the pack, both partial specializations
// matched.  gcc and clang's partial ordering pick the base case
// as more specific, but MSVC reports the instantiation as
// ambiguous.
//
//   The replacement avoids recursion entirely: a single
// index-sequence helper computes the subsequence via direct
// `std::tuple_element` lookups, producing a well-defined
// result on every major compiler and reducing template
// instantiation depth from O(tuple_size) to O(1).
NS_INTERNAL

    template<std::size_t Start,
             typename    Tuple,
             std::size_t... Is>
    auto
    tuple_subsequence_select(
        std::index_sequence<Is...>
    )
        -> std::tuple<typename std::tuple_element<
                            (Start + Is), Tuple>::type...>;

NS_END  // internal

// tuple_subsequence
//   type trait: extracts a subsequence from a tuple between two indices.
template<std::size_t Start,
         std::size_t End,
         typename    Tuple>
struct tuple_subsequence;

template<std::size_t Start,
         std::size_t End,
         typename... Types>
struct tuple_subsequence<Start, End, std::tuple<Types...>>
{
    static_assert(Start <= End,
                  "Start index must be less than or equal to End index.");
    static_assert(End <= sizeof...(Types),
                  "End index must be less than or equal to tuple size.");

    using type = decltype(
        internal::tuple_subsequence_select<Start, std::tuple<Types...>>(
            std::make_index_sequence<(End - Start)>{}
        )
    );
};

// tuple_subsequence_t
//   alias template: shorthand for `tuple_subsequence<...>::type`.
template<std::size_t Start,
         std::size_t End,
         typename    Tuple>
using tuple_subsequence_t = typename tuple_subsequence<Start, End, Tuple>::type;


// =========================================================================
// VI.  TUPLE UTILITIES
// =========================================================================

// tuple_to_pack (internal helper)
NS_INTERNAL

    template<typename       Tuple,
             typename       Fn,
             std::size_t... I>
    void
    tuple_to_pack_helper(
        Tuple&& _tuple,
        Fn&&    _func,
        std::index_sequence<I...>
    )
    {
        _func(std::get<I>(_tuple)...);
    }

NS_END  // internal

// tuple_to_pack
//   function: expands a tuple into a function call with the tuple elements
// as arguments.
template<typename Tuple,
         typename Fn>
void
tuple_to_pack(
    Tuple&& _tuple,
    Fn&&    _func
)
{
    constexpr std::size_t N = std::tuple_size<typename std::decay<Tuple>::type>::value;
    internal::tuple_to_pack_helper(
        std::forward<Tuple>(_tuple),
        std::forward<Fn>(_func),
        std::make_index_sequence<N>{}
    );
}


// =========================================================================
// VII. TYPE SELECTION
// =========================================================================

// type_case
//   type trait: represents a condition-type pair for use with type_selector.
template<bool     Condition,
         typename Type>
struct type_case
{
    static constexpr bool condition = Condition;
    using type = Type;
};

// type_selector
//   type trait: selects a type based on the first matching condition in a
// sequence of type_case instances.
template<typename... TypeCases>
struct type_selector
{
    using type = void;  // Default when no type_cases match
    static constexpr bool matched = false;
};

// Base case: no type_cases left
template<>
struct type_selector<>
{
    using type = void;
    static constexpr bool matched = false;
};

// Recursive case: process type_cases sequentially
template<bool        Condition,
         typename    Type,
         typename... RestTypeCases>
struct type_selector<type_case<Condition, Type>, RestTypeCases...>
{
private:
    using next_selector = type_selector<RestTypeCases...>;

public:
    using type = std::conditional_t<Condition, Type, typename next_selector::type>;
    static constexpr bool matched = (Condition || next_selector::matched);
};

// type_select_t
//   alias template: shorthand for `type_selector<...>::type`.
template<typename... TypeCases>
using type_select_t = typename type_selector<TypeCases...>::type;

// type_matched_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename... TypeCases>
    constexpr bool type_matched_v = type_selector<TypeCases...>::matched;
#endif


// =========================================================================
// VIII. TUPLE HOMOGENEITY
// =========================================================================

// is_tuple_homogeneous
//   type trait: evaluates whether all types in a tuple are the same.
template<typename Tuple>
struct is_tuple_homogeneous : std::false_type
{};

template<typename Type>
struct is_tuple_homogeneous<std::tuple<Type>> : std::true_type
{};

template<typename    Type,
         typename    Type2,
         typename... Types>
struct is_tuple_homogeneous<std::tuple<Type, Type2, Types...>>
    : std::integral_constant<bool,
        std::is_same<Type, Type2>::value &&
        is_tuple_homogeneous<std::tuple<Type2, Types...>>::value>
{};

// is_tuple_homogeneous_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Tuple>
constexpr bool is_tuple_homogeneous_v = is_tuple_homogeneous<Tuple>::value;
#endif

// is_homogeneous
//   function: runtime helper to check tuple homogeneity.
template<typename... Types>
static inline constexpr bool is_homogeneous(std::tuple<Types...> const&)
{
    return is_tuple_homogeneous<std::tuple<Types...>>::value;
}


// =========================================================================
// IX.  2D TUPLE / JAGGED TUPLE SUPPORT
// =========================================================================

// is_2d_tuple (internal helper)
NS_INTERNAL

    // is_2d_tuple_helper
    //   trait: checks if all elements of a tuple are tuples (primary).
    template<typename Tuple>
    struct is_2d_tuple_helper : std::false_type
    {};

    // is_2d_tuple_helper<std::tuple<>>
    //   trait: empty tuple vacuously satisfies 2D tuple property.
    template<>
    struct is_2d_tuple_helper<std::tuple<>> : std::true_type
    {};

    // is_2d_tuple_helper<std::tuple<Head, Tail...>>
    //   trait: recursive check that head is a tuple and recurse on tail.
    template<typename    Head,
             typename... Tail>
    struct is_2d_tuple_helper<std::tuple<Head, Tail...>>
        : std::integral_constant<bool,
            ( is_tuple<clean_t<Head>>::value &&
                is_2d_tuple_helper<std::tuple<Tail...>>::value )>
    {};

NS_END  // internal

// is_2d_tuple
//   trait: evaluates to `std::true_type` if `Tuple` is a tuple where
// every element is itself a tuple (i.e., a 2D structure). Empty tuples
// vacuously satisfy this property. This is the foundation for jagged and
// uniform 2D tuple detection.
D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS(is_2d_tuple, std::tuple,
    internal::is_2d_tuple_helper<std::tuple<Types...>>)

// is_2d_tuple_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Tuple>
constexpr bool is_2d_tuple_v = is_2d_tuple<Tuple>::value;
#endif

// tuple_inner_sizes (internal helper)
NS_INTERNAL

    // tuple_inner_sizes_helper
    //   trait: extracts sizes of each inner tuple as an index_sequence.
    template<typename Tuple>
    struct tuple_inner_sizes_helper;

    template<typename... Rows>
    struct tuple_inner_sizes_helper<std::tuple<Rows...>>
    {
        using type = std::index_sequence<
            std::tuple_size<clean_t<Rows>>::value...
        >;
    };

    template<>
    struct tuple_inner_sizes_helper<std::tuple<>>
    {
        using type = std::index_sequence<>;
    };

NS_END  // internal

// tuple_inner_sizes
//   trait: provides an `std::index_sequence` containing the size of each
// inner tuple. Only valid for 2D tuples (tuple-of-tuples).
template<typename Tuple>
struct tuple_inner_sizes
{
    static_assert(is_2d_tuple<Tuple>::value,
                    "`Tuple` must be a 2D tuple (tuple of tuples).");
    using type = typename internal::tuple_inner_sizes_helper<Tuple>::type;
};

// tuple_inner_sizes_t
//   type: convenience alias for tuple_inner_sizes<...>::type.
D_TYPE_TRAIT_TYPE_ALIAS(tuple_inner_sizes)

// tuple_outer_size
//   trait: returns the number of inner tuples (rows) in a 2D tuple.
// For non-2D-tuple types, value is 0.
template<typename Tuple>
struct tuple_outer_size : std::integral_constant<std::size_t, 0>
{};

template<typename... Rows>
struct tuple_outer_size<std::tuple<Rows...>>
    : std::integral_constant<std::size_t,
        is_2d_tuple<std::tuple<Rows...>>::value
            ? sizeof...(Rows)
            : 0>
{};

// tuple_outer_size_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Tuple>
constexpr std::size_t tuple_outer_size_v = tuple_outer_size<Tuple>::value;
#endif

// tuple_flatten_types
//   trait: flattens a 2D tuple's element types into a single tuple type.
// Given tuple<tuple<A,B>, tuple<C,D,E>>, produces tuple<A,B,C,D,E>.
template<typename Tuple>
struct tuple_flatten_types;

template<>
struct tuple_flatten_types<std::tuple<>>
{
    using type = std::tuple<>;
};

template<typename... InnerTypes,
         typename... Rest>
struct tuple_flatten_types<std::tuple<std::tuple<InnerTypes...>, Rest...>>
{
private:
    using tail_flat = typename tuple_flatten_types<std::tuple<Rest...>>::type;

public:
    using type = typename tuple_join<std::tuple<InnerTypes...>, tail_flat>::type;
};

// tuple_flatten_types_t
//   type: convenience alias for tuple_flatten_types<...>::type.
D_TYPE_TRAIT_TYPE_ALIAS(tuple_flatten_types)


// -------------------------------------------------------------------------
//  uniform vs jagged 2D detection
// -------------------------------------------------------------------------

NS_INTERNAL

    // all_sizes_equal
    //   helper: true if all sizes in an index_sequence are equal. Vacuously
    // true for empty and single-element sequences.
    template<typename Seq>
    struct all_sizes_equal;

    template<>
    struct all_sizes_equal<std::index_sequence<>> : std::true_type
    {};

    template<std::size_t Size>
    struct all_sizes_equal<std::index_sequence<Size>> : std::true_type
    {};

    template<std::size_t    First,
             std::size_t    Second,
             std::size_t... Rest>
    struct all_sizes_equal<std::index_sequence<First, Second, Rest...>>
        : std::integral_constant<bool,
            ( (First == Second) &&
              all_sizes_equal<std::index_sequence<Second, Rest...>>::value )>
    {};

NS_END  // internal

// is_uniform_2d_tuple
//   trait: evaluates to `std::true_type` if `Tuple` is a 2D tuple whose
// inner tuples all share the same size. Empty 2D tuples and single-row
// 2D tuples are vacuously uniform.
//
//   IMPLEMENTATION NOTE: this trait used to delegate via
// `std::conditional<is_2d_tuple<...>::value, all_sizes_equal<...>,
// std::false_type>::type`, but `std::conditional` is NOT lazy -- both
// branch types must be valid type expressions for the conditional to
// name `::type`.  Naming the `all_sizes_equal<...>` branch caused
// `tuple_inner_sizes_helper<std::tuple<Rows...>>::type` to be
// instantiated even on non-2D inputs, which expands
// `std::tuple_size<Rows>::value...` -- and `std::tuple_size<int>` is
// undefined by the standard (the primary template has no body), so
// passing `std::tuple<int>` would error out deep in the helper before
// the conditional could pick the false branch.  The fix is the same
// tag-dispatched gate idiom used elsewhere in this header (see
// `to_tuple<>`): an extra internal layer parameterised on a `bool`,
// where the helper is only named on the path where it is safe.
NS_INTERNAL

    // uniform_2d_dispatch
    //   helper: gated dispatch.  Primary template (false) inherits
    // from `std::false_type` and never names `tuple_inner_sizes_helper`,
    // so non-2D inputs short-circuit without instantiating it.  The
    // `true` specialization is the only one that ever touches the
    // helper, and is only reachable when `is_2d_tuple<Tuple>::value`
    // has already been confirmed.
    template<typename Tuple, bool Is2D>
    struct uniform_2d_dispatch : std::false_type
    {};

    template<typename Tuple>
    struct uniform_2d_dispatch<Tuple, true>
        : all_sizes_equal<
            typename tuple_inner_sizes_helper<Tuple>::type>
    {};

NS_END  // internal

template<typename Tuple>
struct is_uniform_2d_tuple : std::false_type
{};

template<typename... Rows>
struct is_uniform_2d_tuple<std::tuple<Rows...>>
    : internal::uniform_2d_dispatch<
        std::tuple<Rows...>,
        is_2d_tuple<std::tuple<Rows...>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_uniform_2d_tuple_v
    //
    template<typename Tuple>
    constexpr bool is_uniform_2d_tuple_v = is_uniform_2d_tuple<Tuple>::value;
#endif

// is_jagged_tuple
//   trait: evaluates to `std::true_type` if `Tuple` is a 2D tuple whose
// inner tuples differ in size (i.e., 2D but NOT uniform) and has at
// least two rows. Single-row 2D tuples are uniform by definition; the
// jagged classification is reserved for the >= 2 rows case.
template<typename Tuple>
struct is_jagged_tuple : std::false_type
{};

template<typename... Rows>
struct is_jagged_tuple<std::tuple<Rows...>>
    : std::integral_constant<bool,
        ( is_2d_tuple<std::tuple<Rows...>>::value &&
          (sizeof...(Rows) > 1)                   &&
          !is_uniform_2d_tuple<std::tuple<Rows...>>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_jagged_tuple_v
    //
    template<typename Tuple>
    constexpr bool is_jagged_tuple_v = is_jagged_tuple<Tuple>::value;
#endif


// -------------------------------------------------------------------------
//  tuple_total_elements
// -------------------------------------------------------------------------

NS_INTERNAL

    // sum_sizes
    //   helper: portable sum over a parameter pack of `std::size_t`. C++17
    // fold-expressions would shorten this but break C++11/14 compatibility.
    template<std::size_t... Sizes>
    struct sum_sizes;

    template<>
    struct sum_sizes<>
        : std::integral_constant<std::size_t, 0>
    {};

    template<std::size_t    First,
             std::size_t... Rest>
    struct sum_sizes<First, Rest...>
        : std::integral_constant<std::size_t,
            First + sum_sizes<Rest...>::value>
    {};

    // sum_sizes_from_seq
    //   helper: sum over a `std::index_sequence`.
    template<typename Seq>
    struct sum_sizes_from_seq;

    template<std::size_t... Sizes>
    struct sum_sizes_from_seq<std::index_sequence<Sizes...>>
        : sum_sizes<Sizes...>
    {};

NS_END  // internal

// tuple_total_elements
//   trait: returns the total number of elements across all inner tuples
// of a 2D tuple. For `std::tuple<std::tuple<A,B>, std::tuple<C,D,E>>`
// this is 5.
template<typename Tuple>
struct tuple_total_elements
    : internal::sum_sizes_from_seq<typename tuple_inner_sizes<Tuple>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tuple_total_elements_v
    //
    template<typename Tuple>
    constexpr std::size_t tuple_total_elements_v =
        tuple_total_elements<Tuple>::value;
#endif


// -------------------------------------------------------------------------
//  tuple_common_element_type
// -------------------------------------------------------------------------

NS_INTERNAL

    // common_type_from_tuple
    //   helper: extracts `std::common_type<Ts...>::type` from a
    // `std::tuple<Ts...>`. Empty tuple maps to `void`.
    template<typename Tuple>
    struct common_type_from_tuple;

    template<>
    struct common_type_from_tuple<std::tuple<>>
    {
        using type = void;
    };

    template<typename... Types>
    struct common_type_from_tuple<std::tuple<Types...>>
    {
        using type = typename std::common_type<Types...>::type;
    };

NS_END  // internal

// tuple_common_element_type
//   trait: determines the common element type across all inner tuples
// of a 2D tuple, using `std::common_type` semantics. Compilation fails
// if no common type exists across the flattened element pack.
template<typename Tuple>
struct tuple_common_element_type
{
    static_assert(is_2d_tuple<Tuple>::value,
                  "`Tuple` must be a 2D tuple (tuple of tuples).");

    using type = typename internal::common_type_from_tuple<
        typename tuple_flatten_types<Tuple>::type
    >::type;
};

// tuple_common_element_type_t
//   type: convenience alias for tuple_common_element_type<...>::type.
D_TYPE_TRAIT_TYPE_ALIAS(tuple_common_element_type)


// -------------------------------------------------------------------------
//  make_2d_tuple_of
// -------------------------------------------------------------------------

// make_2d_tuple_of
//   trait: constructs a 2D tuple type from a single element type and a
// pack of row sizes. Given type `T` and sizes `N1, N2, ..., Nk` produces
// `std::tuple<repeat_type_t<T, N1>, repeat_type_t<T, N2>, ..., repeat_type_t<T, Nk>>`,
// i.e. a 2D tuple of `T` with k rows of the specified widths.
//
//   When all `Ni` are equal, the result is a uniform 2D tuple; when they
// differ (and k > 1), it is a jagged tuple.
template<typename       Type,
         std::size_t... RowSizes>
struct make_2d_tuple_of
{
    using type = std::tuple<repeat_type_t<Type, RowSizes>...>;
};

// make_2d_tuple_of_t
//   type: convenience alias for make_2d_tuple_of<...>::type.
template<typename       Type,
         std::size_t... RowSizes>
using make_2d_tuple_of_t = typename make_2d_tuple_of<Type, RowSizes...>::type;


// -------------------------------------------------------------------------
//  tuple_row_type / tuple_row_size
// -------------------------------------------------------------------------

// tuple_row_type
//   trait: extracts the row (inner tuple) type at a given index from a
// 2D tuple. The result is `clean_t`-stripped so callers reliably receive
// the bare `std::tuple<...>` regardless of how the row appeared in the
// outer tuple (cv- or reference-qualified).
template<std::size_t RowIndex,
         typename    Tuple>
struct tuple_row_type
{
    static_assert(is_2d_tuple<Tuple>::value,
                  "`Tuple` must be a 2D tuple (tuple of tuples).");
    static_assert(RowIndex < std::tuple_size<Tuple>::value,
                  "`RowIndex` must be less than the number of rows.");

    using type = clean_t<
        typename std::tuple_element<RowIndex, Tuple>::type>;
};

// tuple_row_type_t
//   type: convenience alias for tuple_row_type<...>::type.
template<std::size_t RowIndex,
         typename    Tuple>
using tuple_row_type_t = typename tuple_row_type<RowIndex, Tuple>::type;

// tuple_row_size
//   trait: returns the size of a specific row in a 2D tuple.
template<std::size_t RowIndex,
         typename    Tuple>
struct tuple_row_size
    : std::integral_constant<std::size_t,
        std::tuple_size<tuple_row_type_t<RowIndex, Tuple>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tuple_row_size_v
    //
    template<std::size_t RowIndex,
             typename    Tuple>
    constexpr std::size_t tuple_row_size_v =
        tuple_row_size<RowIndex, Tuple>::value;
#endif


// =========================================================================
// [extra] TUPLE OF TUPLES TYPE RELATION TRAITS
// =========================================================================

// normalize_tuple
//   type trait: maps `std::tuple<Ts...>` to `std::tuple<clean_t<Ts>...>`.
template<typename Tuple>
struct normalize_tuple;

template<typename... Ts>
struct normalize_tuple<std::tuple<Ts...>>
{
    using type = std::tuple<clean_t<Ts>...>;
};

template<typename Tuple>
using normalize_tuple_t = typename normalize_tuple<clean_t<Tuple>>::type;

// tuple_all_elements_same_as
//   type trait: true if all elements in `Tuple` (a `std::tuple`) are
// the same as `Type` after applying `clean_t`.
template<typename Tuple,
         typename Type>
struct tuple_all_elements_same_as;

template<typename Type>
struct tuple_all_elements_same_as<std::tuple<>, Type> : std::true_type
{};

template<typename Head,
         typename... Tail,
         typename Type>
struct tuple_all_elements_same_as<std::tuple<Head, Tail...>, Type>
    : std::integral_constant<bool,
        std::is_same<clean_t<Head>, Type>::value &&
        tuple_all_elements_same_as<std::tuple<Tail...>, Type>::value>
{};

NS_INTERNAL

    // inner_is_empty_tuple
    //   internal helper: true if `Type` is `std::tuple<>` (after clean).
    //
    //   NOTE: bool_constant<...> (single template arg) is used here
    // instead of std::integral_constant<bool, ...> because the comma
    // between `bool` and the value-expression would be seen by the
    // preprocessor as a macro-argument separator -- D_TYPE_TRAIT_TRUE_AS
    // takes exactly three arguments and angle brackets do NOT shield
    // commas (only parentheses do).  bool_constant<X> has no top-level
    // comma in its argument list, so the macro receives the expected
    // three arguments.
    D_TYPE_TRAIT_TRUE_AS(inner_is_empty_tuple,
        std::enable_if_t<is_tuple<clean_t<Type>>::value>,
        bool_constant<(std::tuple_size<normalize_tuple_t<Type>>::value == 0)>)

    // all_inners_empty
    template<typename... Inners>
    struct all_inners_empty : std::true_type
    {};

    template<typename    Head,
             typename... Tail>
    struct all_inners_empty<Head, Tail...>
        : std::integral_constant<bool,
            inner_is_empty_tuple<Head>::value && all_inners_empty<Tail...>::value>
    {};

    // inner_nonempty_all_elements_same
    template<typename Inner,
             typename Type,
             typename = void>
    struct inner_nonempty_all_elements_same : std::false_type
    {};

    template<typename Inner,
             typename Type>
    struct inner_nonempty_all_elements_same<Inner, Type,
        typename std::enable_if<is_tuple<clean_t<Inner>>::value>::type>
        : std::integral_constant<bool,
            (std::tuple_size<normalize_tuple_t<Inner>>::value > 0) &&
            tuple_all_elements_same_as<normalize_tuple_t<Inner>, Type>::value>
    {};

    // all_inners_nonempty_all_elements_same
    template<typename Type,
                typename... Inners>
    struct all_inners_nonempty_all_elements_same : std::true_type
    {};

    template<typename Type,
             typename Head,
             typename... Tail>
    struct all_inners_nonempty_all_elements_same<Type, Head, Tail...>
        : std::integral_constant<bool,
            inner_nonempty_all_elements_same<Head, Type>::value &&
            all_inners_nonempty_all_elements_same<Type, Tail...>::value>
    {};

NS_END  // internal

// all_inner_tuple_elements_one_type
//   type trait: true if `Outer` is a `std::tuple` of `std::tuple`s and
// all element types across all inner tuples are one common type.
template<typename Outer>
struct all_inner_tuple_elements_one_type : std::false_type
{};

template<>
struct all_inner_tuple_elements_one_type<std::tuple<>> : std::true_type
{};

// first inner is empty => true iff all inners are empty tuples
template<typename... Inners>
struct all_inner_tuple_elements_one_type<std::tuple<std::tuple<>, Inners...>>
    : std::integral_constant<bool, internal::all_inners_empty<Inners...>::value>
{};

// first inner is non-empty => all inners non-empty tuples and all elements match
template<typename    E0,
         typename... Erest,
         typename... Inners>
struct all_inner_tuple_elements_one_type<std::tuple<std::tuple<E0, Erest...>, Inners...>>
    : std::integral_constant<bool,
        tuple_all_elements_same_as<std::tuple<E0, Erest...>, clean_t<E0>>::value &&
        internal::all_inners_nonempty_all_elements_same<clean_t<E0>, Inners...>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // all_inner_tuple_elements_one_type_v
    //
    template<typename Outer>
    constexpr bool all_inner_tuple_elements_one_type_v = all_inner_tuple_elements_one_type<Outer>::value;
#endif


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_DTUPLE_HPP
