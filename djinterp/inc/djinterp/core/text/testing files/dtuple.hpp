/******************************************************************************
* djinterp [meta]                                                   dtuple.hpp
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2024.04.25
******************************************************************************/

#ifndef DJINTERP_META_TUPLE_
#define DJINTERP_META_TUPLE_ 1

#include <algorithm>
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./type_traits.hpp"
#include "./trait_detect.hpp"   // D_TYPE_TRAIT_TYPE_ALIAS, _TRUE_AS, _IS_SPECIALIZATION_OF_AS


NS_DJINTERP

// =========================================================================
// FORWARD DECLARATIONS
// =========================================================================
// Note: first_arg, is_tuple, is_single_tuple_arg, and to_tuple are defined
// in type_traits.hpp (included above).

template<template<typename> typename... _Modifiers>
struct wrap_all;

template<template<typename> typename    _Modifier,
            template<typename> typename... _Modifiers>
struct wrap_all<_Modifier, _Modifiers...>;

template<>
struct wrap_all<>;


// =========================================================================
// II.  PARAMETER PACK UTILITIES
// =========================================================================

// is_tuple_single_arg
//   type trait: evaluates to `std::true_type` if the parameter pack
// consists of exactly one argument; otherwise `std::false_type`.
template<typename... _Types>
struct is_tuple_single_arg : std::false_type
{};

template<typename _Type>
struct is_tuple_single_arg<_Type> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// is_tuple_single_arg
//
template<typename... _Types>
constexpr bool is_tuple_single_arg_v = is_tuple_single_arg<_Types...>::value;
#endif



// =========================================================================
// V.   TUPLE CONSTRUCTION
// =========================================================================

// make_tuple_of
//   type trait: creates a `std::tuple` containing `_Count` copies of
// `_Type`.
template<typename    _Type,
         std::size_t _Count>
struct make_tuple_of;

template<typename _Type>
struct make_tuple_of<_Type, 0>
{
    using type = std::tuple<>;
};

template<typename _Type>
struct make_tuple_of<_Type, 1>
{
    using type = to_tuple_t<_Type>;
};

// make_tuple_of  (general case, _Count >= 2)
//   type trait: produces std::tuple<_Type, _Type, ...> with
// _Count copies of _Type.  The earlier implementation used a
// comma-operator trick that, on some compilers, caused the
// element type to decay to `_Type&&` - and a tuple of rvalue
// references cannot be default-constructed, producing the
// diagnostic "no instance of constructor std::tuple<...>
// matches the argument list" at the `{}` initialization site.
//
//   The replacement delegates directly to `repeat_t` (from
// djinterp.hpp), which produces a well-formed
// `std::tuple<_Type, _Type, ...>` without any reference
// qualification or runtime construction step.  The
// specializations for _Count == 0 and _Count == 1 above
// are retained for clarity and produce identical results
// to `repeat_t<_Type, 0>` and `repeat_t<_Type, 1>`
// respectively, so more-specific matching picks them up
// first when applicable.
template<typename    _Type,
         std::size_t _Count>
struct make_tuple_of
{
    using type = repeat_t<_Type, _Count>;
};

// make_tuple_of_t
//   alias template: shorthand for `make_tuple_of<_Type, _Count>::type`.
template<typename    _Type,
         std::size_t _Count>
using make_tuple_of_t = typename make_tuple_of<_Type, _Count>::type;


// =========================================================================
// VI.  TYPE MODIFIERS
// =========================================================================

// wrap_all
//   type trait: applies a series of type transformations left-to-right,
// where right is the innermost and left is the outermost.
// Example: wrap_all<X, Y, Z>::template type<int> == X<Y<Z<int>>>
template<template<typename> typename... _Modifiers>
struct wrap_all
{
    template<typename _Type>
    using type = _Type;
};

template<template<typename> typename    _Modifier,
         template<typename> typename... _Modifiers>
struct wrap_all<_Modifier, _Modifiers...>
{
    template<typename _Type>
    using type = typename _Modifier<
        typename wrap_all<_Modifiers...>::template type<_Type>
    >::type;
};

template<>
struct wrap_all<>
{
    template<typename _Type>
    using type = _Type;
};

// wrap_all_t
//   alias template: shorthand for applying wrap_all.
template<typename                        _Type,
         template<typename> typename... _Modifiers>
using wrap_all_t = typename wrap_all<_Modifiers...>::template type<_Type>;

// to_lvalue_reference
//   type modifier: converts a type to an lvalue reference, removing any
// existing reference first.
struct to_lvalue_reference
{
    template<typename _Type>
    using type = typename wrap_all<
        std::add_lvalue_reference,
        std::remove_reference
    >::template type<_Type>;
};

// to_rvalue_reference
//   type modifier: converts a type to an rvalue reference, removing any
// existing reference first.
struct to_rvalue_reference
{
    template<typename _Type>
    using type = typename wrap_all<
        std::add_rvalue_reference,
        std::remove_reference
    >::template type<_Type>;
};

// to_pointer
//   type modifier: converts a type to a pointer, removing any existing
// pointer first.
struct to_pointer
{
    template<typename _Type>
    using type = typename wrap_all<
        std::add_pointer,
        std::remove_pointer
    >::template type<_Type>;
};

// to_type
//   type trait: identity wrapper that simply exposes a `type` member alias.
// Useful for metaprogramming contexts where a type wrapper is expected.
template<typename _Type>
struct to_type
{
    using type = _Type;
};

// to_type_t
//   alias template: shorthand for `to_type<_Type>::type`.
template<typename _Type>
using to_type_t = typename to_type<_Type>::type;

// forward declaractions
template<typename    _Type, 
         typename... _Types> 
struct tuple_count_and_remove;

template<typename    _Type, 
         typename... _Types> 
struct tuple_first_type;

NS_INTERNAL
    // tuple_join_helper
    //   internal helper: flattens a `std::tuple` whose elements may be
    // `std::tuple<...>` or single types, producing one `std::tuple<...>`.
    template<typename _Tuple,
             typename... _Types>
    struct tuple_join_helper;

    // base case: no more elements to process
    template<typename... _Types>
    struct tuple_join_helper<std::tuple<>, _Types...>
    {
        using type = std::tuple<_Types...>;
    };

    // case: head is a std::tuple<...> => append its elements
    template<typename... _Head,
             typename... _Tail,
             typename... _Types>
    struct tuple_join_helper<std::tuple<std::tuple<_Head...>, _Tail...>, _Types...>
    {
        using type = typename tuple_join_helper<std::tuple<_Tail...>, _Types..., _Head...>::type;
    };

    // case: head is a single type => append it
    template<typename    _Head,
             typename... _Tail,
             typename... _Types>
    struct tuple_join_helper<std::tuple<_Head, _Tail...>, _Types...>
    {
        using type = typename tuple_join_helper<std::tuple<_Tail...>, _Types..., _Head>::type;
    };
NS_END  // internal

// tuple_join
//   type trait: joins/concatenates the types of one or more tuple-like
// inputs. Each argument that is a `std::tuple<...>` contributes its
// elements; non-tuple arguments contribute themselves as one element.
template<typename... _Tuples>
struct tuple_join
{
    using type = typename internal::tuple_join_helper<std::tuple<_Tuples...>>::type;
};



// =========================================================================
// II.  TUPLE TRANSFORMATION
// =========================================================================

// tuple_apply_all (internal helper)
NS_INTERNAL

    template<template<typename...> typename _UnaryTrait,
             typename                       _Tuple,
             typename...                    _Types>
    struct tuple_apply_all_helper;

    template<template<typename...> typename _UnaryTrait,
             typename...                    _Types>
    struct tuple_apply_all_helper<_UnaryTrait, std::tuple<>, _Types...>
    {
        using type = std::tuple<_Types...>;
    };

    template<template<typename...> typename _UnaryTrait,
             typename                       _Head,
             typename...                    _Tails,
             typename...                    _Types>
    struct tuple_apply_all_helper<_UnaryTrait, std::tuple<_Head, _Tails...>, _Types...>
    {
        using type = typename tuple_apply_all_helper<
            _UnaryTrait,
            std::tuple<_Tails...>,
            _Types...,
            _UnaryTrait<_Head>
        >::type;
    };

NS_END  // internal

// tuple_apply_all
//   type trait: applies a unary type trait to all elements of a tuple.
template<template<typename...> typename _UnaryTrait,
         typename...                    _Types>
struct tuple_apply_all
{
    using type = typename internal::tuple_apply_all_helper<
        _UnaryTrait,
        typename to_tuple<_Types...>::type
    >::type;
};

// tuple_apply_all_t
//   alias template: shorthand for `tuple_apply_all<...>::type`.
template<template<typename...> typename _UnaryTrait,
         typename...                    _Types>
using tuple_apply_all_t = typename tuple_apply_all<_UnaryTrait, _Types...>::type;


// =========================================================================
// III. ELEMENT ACCESS
// =========================================================================

// tuple_type_at (internal helper)
NS_INTERNAL
    template<std::size_t _Index,
             typename    _Tuple>
    struct tuple_type_at_helper;

    template<typename    _Head,
             typename... _Tail>
    struct tuple_type_at_helper<0, std::tuple<_Head, _Tail...>>
    {
        using type = _Head;

        static constexpr auto value(const std::tuple<_Head, _Tail...>& _t)
        {
            return std::get<0>(_t);
        }
    };

    template<std::size_t _Index,
             typename    _Head,
             typename... _Tail>
    struct tuple_type_at_helper<_Index, std::tuple<_Head, _Tail...>>
    {
        using type = typename tuple_type_at_helper<_Index - 1, std::tuple<_Tail...>>::type;

        // `value` mirrors the base-case specialization above so that
        // `tuple_type_at_value<_Index>(_t)` works for any in-bounds
        // index, not just 0.  Originally this specialization defined
        // only `type` and left `value` to the base case, but the
        // public wrapper `tuple_type_at_value` calls
        // `tuple_type_at_helper<_Index, _Tuple>::value(_t)` directly
        // (without bottoming out the index recursion at the call
        // site), so any non-zero index hit a missing-member error.
        // Since `_t` is the FULL original tuple at every recursion
        // level (the recursion is purely over `_Index`, not over a
        // tail view of the tuple), `std::get<_Index>(_t)` is the
        // direct and correct retrieval.
        static constexpr auto value(const std::tuple<_Head, _Tail...>& _t)
        {
            return std::get<_Index>(_t);
        }
    };

NS_END  // internal

// tuple_type_at
//   type trait: retrieves the type at a specific index in a tuple.
template<std::size_t _Index,
         typename... _Types>
struct tuple_type_at
{
private:
    using tuple_type = to_tuple_t<_Types...>;

    static_assert((_Index < std::tuple_size<tuple_type>::value),
                    "Non-type parameter `_Index` cannot be greater than or "
                    "equal to the tuple size of type parameter `_Tuple`.");

public:
    using type = typename internal::tuple_type_at_helper<_Index, tuple_type>::type;
};

// tuple_type_at_t
//   alias template: shorthand for `tuple_type_at<...>::type`.
template<std::size_t _Index,
         typename... _Types>
using tuple_type_at_t = typename tuple_type_at<_Index, _Types...>::type;

// tuple_type_at_value
//   function: retrieves the value at a specific index in a tuple instance.
template<std::size_t _Index,
         typename    _Tuple>
constexpr auto tuple_type_at_value(const _Tuple& _t)
{
    return internal::tuple_type_at_helper<_Index, _Tuple>::value(_t);
}

// tuple_concat
//   function: concatenates multiple tuples at compile-time.
template<typename... _Tuples>
static constexpr auto tuple_concat(_Tuples&&... _tuples)
{
    return std::tuple_cat(std::forward<_Tuples>(_tuples)...);
}


// =========================================================================
// IV.  TYPE COUNTING AND FILTERING
// =========================================================================

// tuple_consolidate_types (internal helper)
NS_INTERNAL

    template<typename _Tuple,
             typename... _Result>
    struct tuple_consolidate_types_helper;

    template<typename... _Types>
    struct tuple_consolidate_types_helper<std::tuple<>, _Types...>
    {
        using type = std::tuple<_Types...>;
    };

    template<typename    _Head,
             typename... _Tails,
             typename... _Types>
    struct tuple_consolidate_types_helper<std::tuple<_Head, _Tails...>, _Types...>
    {
        using removed = tuple_count_and_remove<_Head, _Tails...>;
        using type = typename tuple_consolidate_types_helper<
         typename removed::type,
            _Types...,
            std::conditional_t<
                (removed::value > 0),
                std::array<_Head, removed::value + 1>,
                _Head
            >
        >::type;
    };

NS_END  // internal

// tuple_consolidate_types
//   type trait: consolidates repeated types in a tuple into arrays.
template<typename... _Types>
struct tuple_consolidate_types
{
    using type = typename internal::tuple_consolidate_types_helper<
        typename to_tuple<_Types...>::type
    >::type;
};

// tuple_consolidate_types_t
//   alias template: shorthand for `tuple_consolidate_types<...>::type`.
template<typename... _Types>
using tuple_consolidate_types_t = typename tuple_consolidate_types<_Types...>::type;

// tuple_count_and_remove (internal helper)
NS_INTERNAL
    template<typename    _Type,
             typename    _Tuple,
             std::size_t _Count,
             typename    _Filtered>
    struct tuple_count_and_remove_helper;

    template<typename    _Type,
             std::size_t _Count,
             typename... _Filtered>
    struct tuple_count_and_remove_helper<_Type, std::tuple<>, _Count, std::tuple<_Filtered...>>
    {
        static constexpr std::size_t value = _Count;
        using type = std::tuple<_Filtered...>;
    };

    template<typename    _Type,
             typename    _Head,
             typename... _Tail,
             std::size_t _Count,
             typename... _Filtered>
    struct tuple_count_and_remove_helper<_Type, std::tuple<_Head, _Tail...>, _Count, std::tuple<_Filtered...>>
    {
        using recursive_type = std::conditional_t<
            std::is_same<_Type, _Head>::value,
            tuple_count_and_remove_helper<_Type, std::tuple<_Tail...>, _Count + 1, std::tuple<_Filtered...>>,
            tuple_count_and_remove_helper<_Type, std::tuple<_Tail...>, _Count, std::tuple<_Filtered..., _Head>>
        >;

        using                        type  = typename recursive_type::type;
        static constexpr std::size_t value = recursive_type::value;
    };

NS_END  // internal

// tuple_count_and_remove
//   type trait: counts occurrences of a type and removes them from a tuple.
template<typename    _Type,
         typename... _Types>
struct tuple_count_and_remove
{
protected:
    using _type = typename internal::tuple_count_and_remove_helper<
        _Type,
        typename to_tuple<_Types...>::type,
        0,
        std::tuple<>
    >;

public:
    using type = typename _type::type;
    static constexpr std::size_t value = _type::value;
};

// tuple_count_and_remove_t
//   alias template: shorthand for `tuple_count_and_remove<...>::type`.
template<typename    _Type,
         typename... _Types>
using tuple_count_and_remove_t = typename tuple_count_and_remove<_Type, _Types...>::type;

// tuple_count_and_remove_v
//   
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename    _Type,
             typename... _Types>
    constexpr std::size_t tuple_count_and_remove_v = tuple_count_and_remove<_Type, _Types...>::value;
#endif

// tuple_count_type (internal helper)
NS_INTERNAL

    template<typename    _Type,
             typename    _Tuple,
             std::size_t _Count>
    struct tuple_count_type_helper;

    // case: empty
    template<typename    _Type,
             std::size_t _Count>
    struct tuple_count_type_helper<_Type, std::tuple<>, _Count>
    {
        static constexpr std::size_t value = 0;
    };

    // case: last element (or tuple of size 1)
    template<typename    _Type,
             typename    _Head,
             std::size_t _Count>
    struct tuple_count_type_helper<_Type, std::tuple<_Head>, _Count>
    {
        static constexpr std::size_t value = std::conditional_t<
            std::is_same<_Type, _Head>::value,
            std::integral_constant<std::size_t, _Count + 1>,
            std::integral_constant<std::size_t, _Count>
        >::value;
    };

    // case: recursive
    template<typename    _Type,
             typename    _Head,
             typename... _Tail,
             std::size_t _Count>
    struct tuple_count_type_helper<_Type, std::tuple<_Head, _Tail...>, _Count>
    {
        static constexpr std::size_t value = 
            tuple_count_type_helper<
                _Type,
                std::tuple<_Tail...>,
                std::conditional_t<
                    std::is_same<_Type, _Head>::value,
                    std::integral_constant<std::size_t, _Count + 1>,
                    std::integral_constant<std::size_t, _Count>
                >::value
            >::value;
    };

NS_END  // internal

// tuple_count_type
//   type trait: counts the number of occurrences of a type in a tuple.
template<typename    _Type,
         typename... _Types>
struct tuple_count_type
{
    static constexpr std::size_t value = 
        internal::tuple_count_type_helper<_Type,
                                          typename to_tuple<_Types...>::type,
                                          0>::value;
};

// tuple_count_type_v
//   
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename    _Type,
                typename... _Types>
    constexpr std::size_t tuple_count_type_v = tuple_count_type<_Type, _Types...>::value;
#endif


// =========================================================================
// V.   TUPLE SPLITTING
// =========================================================================

// tuple_split (internal helper)
NS_INTERNAL

    template<std::size_t _Index,
             typename    _Before,
             typename    _After,
             typename = void>
    struct tuple_split_helper;

    // index = 0
    template<typename... _Before,
             typename... _After>
    struct tuple_split_helper<0, std::tuple<_Before...>, std::tuple<_After...>>
    {
        using before = std::tuple<_Before...>;
        using after  = std::tuple<_After...>;
    };

    // recursive case
    template<std::size_t _Index,
             typename... _Before,
             typename    _Type,
             typename... _After>
    struct tuple_split_helper<_Index,
                              std::tuple<_Before...>,
                              std::tuple<_Type, _After...>,
                              std::enable_if_t<(_Index > 0)>>
    {
        using type = tuple_split_helper<_Index - 1, std::tuple<_Before..., _Type>, std::tuple<_After...>>;

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
template<std::size_t _Index,
         typename... _Types>
struct tuple_split
{
private:
    using tuple_type = to_tuple_t<_Types...>;
    static_assert((_Index <= std::tuple_size<tuple_type>::value),
                    "`_Index` must be less than or equal to the `std::tuple` size.");

public:
    using type   = internal::tuple_split_helper<_Index, std::tuple<>, tuple_type>;
    using before = typename type::before;
    using after  = typename type::after;
};

// tuple_split_t
//   alias template: shorthand for `tuple_split<...>::type`.
template<std::size_t _Index,
         typename    _Tuple>
using tuple_split_t = typename tuple_split<_Index, _Tuple>::type;

// tuple_subsequence (internal helper)
//
//   The earlier implementation used a recursive helper with two
// partial specializations:
//
//     <_End, _End, _Tuple, _Types...>             // base case
//     <_Start, _End, _Tuple, _First, _Rest...>    // recursive
//
// When mid-recursion reached a state where `_Start == _End` but
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

    template<std::size_t _Start,
             typename    _Tuple,
             std::size_t... _Is>
    auto
    tuple_subsequence_select(
        std::index_sequence<_Is...>
    )
        -> std::tuple<typename std::tuple_element<
                            (_Start + _Is), _Tuple>::type...>;

NS_END  // internal

// tuple_subsequence
//   type trait: extracts a subsequence from a tuple between two indices.
template<std::size_t _Start,
         std::size_t _End,
         typename    _Tuple>
struct tuple_subsequence;

template<std::size_t _Start,
         std::size_t _End,
         typename... _Types>
struct tuple_subsequence<_Start, _End, std::tuple<_Types...>>
{
    static_assert(_Start <= _End,
                  "Start index must be less than or equal to End index.");
    static_assert(_End <= sizeof...(_Types),
                  "End index must be less than or equal to tuple size.");

    using type = decltype(
        internal::tuple_subsequence_select<_Start, std::tuple<_Types...>>(
            std::make_index_sequence<(_End - _Start)>{}
        )
    );
};

// tuple_subsequence_t
//   alias template: shorthand for `tuple_subsequence<...>::type`.
template<std::size_t _Start,
         std::size_t _End,
         typename    _Tuple>
using tuple_subsequence_t = typename tuple_subsequence<_Start, _End, _Tuple>::type;


// =========================================================================
// VI.  TUPLE UTILITIES
// =========================================================================

// tuple_to_pack (internal helper)
NS_INTERNAL

    template<typename       _Tuple,
             typename       _Fn,
             std::size_t... _I>
    void 
    tuple_to_pack_helper(
        _Tuple&& _tuple,
        _Fn&&    _func,
        std::index_sequence<_I...>
    )
    {
        _func(std::get<_I>(_tuple)...);
    }

NS_END  // internal

// tuple_to_pack
//   function: expands a tuple into a function call with the tuple elements
// as arguments.
template<typename _Tuple,
         typename _Fn>
void
tuple_to_pack(
    _Tuple&& _tuple,
    _Fn&&    _func
)
{
    constexpr std::size_t N = std::tuple_size<typename std::decay<_Tuple>::type>::value;
    internal::tuple_to_pack_helper(
        std::forward<_Tuple>(_tuple),
        std::forward<_Fn>(_func),
        std::make_index_sequence<N>{}
    );
}


// =========================================================================
// VII. TYPE SELECTION
// =========================================================================

// type_case
//   type trait: represents a condition-type pair for use with type_selector.
template<bool     _Condition,
         typename _Type>
struct type_case
{
    static constexpr bool condition = _Condition;
    using type = _Type;
};

// type_selector
//   type trait: selects a type based on the first matching condition in a
// sequence of type_case instances.
template<typename... _TypeCases>
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
template<bool        _Condition,
         typename    _Type,
         typename... _RestTypeCases>
struct type_selector<type_case<_Condition, _Type>, _RestTypeCases...>
{
private:
    using next_selector = type_selector<_RestTypeCases...>;

public:
    using type = std::conditional_t<_Condition, _Type, typename next_selector::type>;
    static constexpr bool matched = (_Condition || next_selector::matched);
};

// type_select_t
//   alias template: shorthand for `type_selector<...>::type`.
template<typename... _TypeCases>
using type_select_t = typename type_selector<_TypeCases...>::type;

// type_matched_v
//   
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename... _TypeCases>
    constexpr bool type_matched_v = type_selector<_TypeCases...>::matched;
#endif


// =========================================================================
// VIII. TUPLE HOMOGENEITY
// =========================================================================

// is_tuple_homogeneous
//   type trait: evaluates whether all types in a tuple are the same.
template<typename _Tuple>
struct is_tuple_homogeneous : std::false_type
{};

template<typename _Type>
struct is_tuple_homogeneous<std::tuple<_Type>> : std::true_type
{};

template<typename    _Type,
         typename    _Type2,
         typename... _Types>
struct is_tuple_homogeneous<std::tuple<_Type, _Type2, _Types...>>
    : std::integral_constant<bool,
        std::is_same<_Type, _Type2>::value &&
        is_tuple_homogeneous<std::tuple<_Type2, _Types...>>::value>
{};

// is_tuple_homogeneous_v
//   
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Tuple>
constexpr bool is_tuple_homogeneous_v = is_tuple_homogeneous<_Tuple>::value;
#endif

// is_homogeneous
//   function: runtime helper to check tuple homogeneity.
template<typename... _Types>
static inline constexpr bool is_homogeneous(std::tuple<_Types...> const&)
{
    return is_tuple_homogeneous<std::tuple<_Types...>>::value;
}


// =========================================================================
// IX.  2D TUPLE / JAGGED TUPLE SUPPORT
// =========================================================================

// is_2d_tuple (internal helper)
NS_INTERNAL

    // is_2d_tuple_helper
    //   trait: checks if all elements of a tuple are tuples (primary).
    template<typename _Tuple>
    struct is_2d_tuple_helper : std::false_type
    {};

    // is_2d_tuple_helper<std::tuple<>>
    //   trait: empty tuple vacuously satisfies 2D tuple property.
    template<>
    struct is_2d_tuple_helper<std::tuple<>> : std::true_type
    {};

    // is_2d_tuple_helper<std::tuple<_Head, _Tail...>>
    //   trait: recursive check that head is a tuple and recurse on tail.
    template<typename    _Head,
             typename... _Tail>
    struct is_2d_tuple_helper<std::tuple<_Head, _Tail...>>
        : std::integral_constant<bool,
            ( is_tuple<clean_t<_Head>>::value &&
                is_2d_tuple_helper<std::tuple<_Tail...>>::value )>
    {};

NS_END  // internal

// is_2d_tuple
//   trait: evaluates to `std::true_type` if `_Tuple` is a tuple where
// every element is itself a tuple (i.e., a 2D structure). Empty tuples
// vacuously satisfy this property. This is the foundation for jagged and
// uniform 2D tuple detection.
D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS(is_2d_tuple, std::tuple,
    internal::is_2d_tuple_helper<std::tuple<_Types...>>)

// is_2d_tuple_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Tuple>
constexpr bool is_2d_tuple_v = is_2d_tuple<_Tuple>::value;
#endif

// tuple_inner_sizes (internal helper)
NS_INTERNAL

    // tuple_inner_sizes_helper
    //   trait: extracts sizes of each inner tuple as an index_sequence.
    template<typename _Tuple>
    struct tuple_inner_sizes_helper;

    template<typename... _Rows>
    struct tuple_inner_sizes_helper<std::tuple<_Rows...>>
    {
        using type = std::index_sequence<
            std::tuple_size<clean_t<_Rows>>::value...
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
template<typename _Tuple>
struct tuple_inner_sizes
{
    static_assert(is_2d_tuple<_Tuple>::value,
                    "`_Tuple` must be a 2D tuple (tuple of tuples).");
    using type = typename internal::tuple_inner_sizes_helper<_Tuple>::type;
};

// tuple_inner_sizes_t
//   type: convenience alias for tuple_inner_sizes<...>::type.
D_TYPE_TRAIT_TYPE_ALIAS(tuple_inner_sizes)

// tuple_outer_size
//   trait: returns the number of inner tuples (rows) in a 2D tuple.
// For non-2D-tuple types, value is 0.
template<typename _Tuple>
struct tuple_outer_size : std::integral_constant<std::size_t, 0>
{};

template<typename... _Rows>
struct tuple_outer_size<std::tuple<_Rows...>>
    : std::integral_constant<std::size_t,
        is_2d_tuple<std::tuple<_Rows...>>::value
            ? sizeof...(_Rows)
            : 0>
{};

// tuple_outer_size_v
//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Tuple>
constexpr std::size_t tuple_outer_size_v = tuple_outer_size<_Tuple>::value;
#endif

// tuple_flatten_types
//   trait: flattens a 2D tuple's element types into a single tuple type.
// Given tuple<tuple<A,B>, tuple<C,D,E>>, produces tuple<A,B,C,D,E>.
template<typename _Tuple>
struct tuple_flatten_types;

template<>
struct tuple_flatten_types<std::tuple<>>
{
    using type = std::tuple<>;
};

template<typename... _InnerTypes,
         typename... _Rest>
struct tuple_flatten_types<std::tuple<std::tuple<_InnerTypes...>, _Rest...>>
{
private:
    using tail_flat = typename tuple_flatten_types<std::tuple<_Rest...>>::type;

public:
    using type = typename tuple_join<std::tuple<_InnerTypes...>, tail_flat>::type;
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
    template<typename _Seq>
    struct all_sizes_equal;

    template<>
    struct all_sizes_equal<std::index_sequence<>> : std::true_type
    {};

    template<std::size_t _Size>
    struct all_sizes_equal<std::index_sequence<_Size>> : std::true_type
    {};

    template<std::size_t    _First,
             std::size_t    _Second,
             std::size_t... _Rest>
    struct all_sizes_equal<std::index_sequence<_First, _Second, _Rest...>>
        : std::integral_constant<bool,
            ( (_First == _Second) &&
              all_sizes_equal<std::index_sequence<_Second, _Rest...>>::value )>
    {};

NS_END  // internal

// is_uniform_2d_tuple
//   trait: evaluates to `std::true_type` if `_Tuple` is a 2D tuple whose
// inner tuples all share the same size. Empty 2D tuples and single-row
// 2D tuples are vacuously uniform.
//
//   IMPLEMENTATION NOTE: this trait used to delegate via
// `std::conditional<is_2d_tuple<...>::value, all_sizes_equal<...>,
// std::false_type>::type`, but `std::conditional` is NOT lazy -- both
// branch types must be valid type expressions for the conditional to
// name `::type`.  Naming the `all_sizes_equal<...>` branch caused
// `tuple_inner_sizes_helper<std::tuple<_Rows...>>::type` to be
// instantiated even on non-2D inputs, which expands
// `std::tuple_size<_Rows>::value...` -- and `std::tuple_size<int>` is
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
    // helper, and is only reachable when `is_2d_tuple<_Tuple>::value`
    // has already been confirmed.
    template<typename _Tuple, bool _Is2D>
    struct uniform_2d_dispatch : std::false_type
    {};

    template<typename _Tuple>
    struct uniform_2d_dispatch<_Tuple, true>
        : all_sizes_equal<
            typename tuple_inner_sizes_helper<_Tuple>::type>
    {};

NS_END  // internal

template<typename _Tuple>
struct is_uniform_2d_tuple : std::false_type
{};

template<typename... _Rows>
struct is_uniform_2d_tuple<std::tuple<_Rows...>>
    : internal::uniform_2d_dispatch<
        std::tuple<_Rows...>,
        is_2d_tuple<std::tuple<_Rows...>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_uniform_2d_tuple_v
    //
    template<typename _Tuple>
    constexpr bool is_uniform_2d_tuple_v = is_uniform_2d_tuple<_Tuple>::value;
#endif

// is_jagged_tuple
//   trait: evaluates to `std::true_type` if `_Tuple` is a 2D tuple whose
// inner tuples differ in size (i.e., 2D but NOT uniform) and has at
// least two rows. Single-row 2D tuples are uniform by definition; the
// jagged classification is reserved for the >= 2 rows case.
template<typename _Tuple>
struct is_jagged_tuple : std::false_type
{};

template<typename... _Rows>
struct is_jagged_tuple<std::tuple<_Rows...>>
    : std::integral_constant<bool,
        ( is_2d_tuple<std::tuple<_Rows...>>::value &&
          (sizeof...(_Rows) > 1)                   &&
          !is_uniform_2d_tuple<std::tuple<_Rows...>>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_jagged_tuple_v
    //
    template<typename _Tuple>
    constexpr bool is_jagged_tuple_v = is_jagged_tuple<_Tuple>::value;
#endif


// -------------------------------------------------------------------------
//  tuple_total_elements
// -------------------------------------------------------------------------

NS_INTERNAL

    // sum_sizes
    //   helper: portable sum over a parameter pack of `std::size_t`. C++17
    // fold-expressions would shorten this but break C++11/14 compatibility.
    template<std::size_t... _Sizes>
    struct sum_sizes;

    template<>
    struct sum_sizes<>
        : std::integral_constant<std::size_t, 0>
    {};

    template<std::size_t    _First,
             std::size_t... _Rest>
    struct sum_sizes<_First, _Rest...>
        : std::integral_constant<std::size_t,
            _First + sum_sizes<_Rest...>::value>
    {};

    // sum_sizes_from_seq
    //   helper: sum over a `std::index_sequence`.
    template<typename _Seq>
    struct sum_sizes_from_seq;

    template<std::size_t... _Sizes>
    struct sum_sizes_from_seq<std::index_sequence<_Sizes...>>
        : sum_sizes<_Sizes...>
    {};

NS_END  // internal

// tuple_total_elements
//   trait: returns the total number of elements across all inner tuples
// of a 2D tuple. For `std::tuple<std::tuple<A,B>, std::tuple<C,D,E>>`
// this is 5.
template<typename _Tuple>
struct tuple_total_elements
    : internal::sum_sizes_from_seq<typename tuple_inner_sizes<_Tuple>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tuple_total_elements_v
    //
    template<typename _Tuple>
    constexpr std::size_t tuple_total_elements_v =
        tuple_total_elements<_Tuple>::value;
#endif


// -------------------------------------------------------------------------
//  tuple_common_element_type
// -------------------------------------------------------------------------

NS_INTERNAL

    // common_type_from_tuple
    //   helper: extracts `std::common_type<_Ts...>::type` from a
    // `std::tuple<_Ts...>`. Empty tuple maps to `void`.
    template<typename _Tuple>
    struct common_type_from_tuple;

    template<>
    struct common_type_from_tuple<std::tuple<>>
    {
        using type = void;
    };

    template<typename... _Types>
    struct common_type_from_tuple<std::tuple<_Types...>>
    {
        using type = typename std::common_type<_Types...>::type;
    };

NS_END  // internal

// tuple_common_element_type
//   trait: determines the common element type across all inner tuples
// of a 2D tuple, using `std::common_type` semantics. Compilation fails
// if no common type exists across the flattened element pack.
template<typename _Tuple>
struct tuple_common_element_type
{
    static_assert(is_2d_tuple<_Tuple>::value,
                  "`_Tuple` must be a 2D tuple (tuple of tuples).");

    using type = typename internal::common_type_from_tuple<
        typename tuple_flatten_types<_Tuple>::type
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
// `std::tuple<repeat_t<T, N1>, repeat_t<T, N2>, ..., repeat_t<T, Nk>>`,
// i.e. a 2D tuple of `T` with k rows of the specified widths.
//
//   When all `Ni` are equal, the result is a uniform 2D tuple; when they
// differ (and k > 1), it is a jagged tuple.
template<typename       _Type,
         std::size_t... _RowSizes>
struct make_2d_tuple_of
{
    using type = std::tuple<repeat_t<_Type, _RowSizes>...>;
};

// make_2d_tuple_of_t
//   type: convenience alias for make_2d_tuple_of<...>::type.
template<typename       _Type,
         std::size_t... _RowSizes>
using make_2d_tuple_of_t = typename make_2d_tuple_of<_Type, _RowSizes...>::type;


// -------------------------------------------------------------------------
//  tuple_row_type / tuple_row_size
// -------------------------------------------------------------------------

// tuple_row_type
//   trait: extracts the row (inner tuple) type at a given index from a
// 2D tuple. The result is `clean_t`-stripped so callers reliably receive
// the bare `std::tuple<...>` regardless of how the row appeared in the
// outer tuple (cv- or reference-qualified).
template<std::size_t _RowIndex,
         typename    _Tuple>
struct tuple_row_type
{
    static_assert(is_2d_tuple<_Tuple>::value,
                  "`_Tuple` must be a 2D tuple (tuple of tuples).");
    static_assert(_RowIndex < std::tuple_size<_Tuple>::value,
                  "`_RowIndex` must be less than the number of rows.");

    using type = clean_t<
        typename std::tuple_element<_RowIndex, _Tuple>::type>;
};

// tuple_row_type_t
//   type: convenience alias for tuple_row_type<...>::type.
template<std::size_t _RowIndex,
         typename    _Tuple>
using tuple_row_type_t = typename tuple_row_type<_RowIndex, _Tuple>::type;

// tuple_row_size
//   trait: returns the size of a specific row in a 2D tuple.
template<std::size_t _RowIndex,
         typename    _Tuple>
struct tuple_row_size
    : std::integral_constant<std::size_t,
        std::tuple_size<tuple_row_type_t<_RowIndex, _Tuple>>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // tuple_row_size_v
    //
    template<std::size_t _RowIndex,
             typename    _Tuple>
    constexpr std::size_t tuple_row_size_v =
        tuple_row_size<_RowIndex, _Tuple>::value;
#endif


// =========================================================================
// [extra] TUPLE OF TUPLES TYPE RELATION TRAITS
// =========================================================================

// normalize_tuple
//   type trait: maps `std::tuple<Ts...>` to `std::tuple<clean_t<Ts>...>`.
template<typename _Tuple>
struct normalize_tuple;

template<typename... _Ts>
struct normalize_tuple<std::tuple<_Ts...>>
{
    using type = std::tuple<clean_t<_Ts>...>;
};

template<typename _Tuple>
using normalize_tuple_t = typename normalize_tuple<clean_t<_Tuple>>::type;

// tuple_all_elements_same_as
//   type trait: true if all elements in `_Tuple` (a `std::tuple`) are
// the same as `_Type` after applying `clean_t`.
template<typename _Tuple,
         typename _Type>
struct tuple_all_elements_same_as;

template<typename _Type>
struct tuple_all_elements_same_as<std::tuple<>, _Type> : std::true_type
{};

template<typename _Head,
         typename... _Tail,
         typename _Type>
struct tuple_all_elements_same_as<std::tuple<_Head, _Tail...>, _Type>
    : std::integral_constant<bool,
        std::is_same<clean_t<_Head>, _Type>::value &&
        tuple_all_elements_same_as<std::tuple<_Tail...>, _Type>::value>
{};

NS_INTERNAL

    // inner_is_empty_tuple
    //   internal helper: true if `_Type` is `std::tuple<>` (after clean).
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
        std::enable_if_t<is_tuple<clean_t<_Type>>::value>,
        bool_constant<(std::tuple_size<normalize_tuple_t<_Type>>::value == 0)>)

    // all_inners_empty
    template<typename... _Inners>
    struct all_inners_empty : std::true_type
    {};

    template<typename    _Head, 
             typename... _Tail>
    struct all_inners_empty<_Head, _Tail...>
        : std::integral_constant<bool,
            inner_is_empty_tuple<_Head>::value && all_inners_empty<_Tail...>::value>
    {};

    // inner_nonempty_all_elements_same
    template<typename _Inner,
             typename _Type,
             typename = void>
    struct inner_nonempty_all_elements_same : std::false_type
    {};

    template<typename _Inner,
             typename _Type>
    struct inner_nonempty_all_elements_same<_Inner, _Type,
        typename std::enable_if<is_tuple<clean_t<_Inner>>::value>::type>
        : std::integral_constant<bool,
            (std::tuple_size<normalize_tuple_t<_Inner>>::value > 0) &&
            tuple_all_elements_same_as<normalize_tuple_t<_Inner>, _Type>::value>
    {};

    // all_inners_nonempty_all_elements_same
    template<typename _Type,
                typename... _Inners>
    struct all_inners_nonempty_all_elements_same : std::true_type
    {};

    template<typename _Type,
             typename _Head,
             typename... _Tail>
    struct all_inners_nonempty_all_elements_same<_Type, _Head, _Tail...>
        : std::integral_constant<bool,
            inner_nonempty_all_elements_same<_Head, _Type>::value &&
            all_inners_nonempty_all_elements_same<_Type, _Tail...>::value>
    {};

NS_END  // internal

// all_inner_tuple_elements_one_type
//   type trait: true if `_Outer` is a `std::tuple` of `std::tuple`s and
// all element types across all inner tuples are one common type.
template<typename _Outer>
struct all_inner_tuple_elements_one_type : std::false_type
{};

template<>
struct all_inner_tuple_elements_one_type<std::tuple<>> : std::true_type
{};

// first inner is empty => true iff all inners are empty tuples
template<typename... _Inners>
struct all_inner_tuple_elements_one_type<std::tuple<std::tuple<>, _Inners...>>
    : std::integral_constant<bool, internal::all_inners_empty<_Inners...>::value>
{};

// first inner is non-empty => all inners non-empty tuples and all elements match
template<typename    _E0,
         typename... _Erest,
         typename... _Inners>
struct all_inner_tuple_elements_one_type<std::tuple<std::tuple<_E0, _Erest...>, _Inners...>>
    : std::integral_constant<bool,
        tuple_all_elements_same_as<std::tuple<_E0, _Erest...>, clean_t<_E0>>::value &&
        internal::all_inners_nonempty_all_elements_same<clean_t<_E0>, _Inners...>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // all_inner_tuple_elements_one_type_v
    //
    template<typename _Outer>
    constexpr bool all_inner_tuple_elements_one_type_v = all_inner_tuple_elements_one_type<_Outer>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_META_TUPLE_