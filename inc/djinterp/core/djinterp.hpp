/******************************************************************************
* djinterp [core]                                                 djinterp.hpp
*
*
*
*
* path:      /inc/cpp/djinterp.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2023.11.12
******************************************************************************/

#ifndef DJINTERP_CPP_
#define DJINTERP_CPP_ 1

#include <cstddef>
#include <initializer_list>
#include <memory>
#include "../c/djinterp.h"


// only include C++98 header detection if not using modern C++ features
#if (!D_ENV_LANG_IS_CPP11_OR_HIGHER)
    #include "./core/env/env_cpp98.h"
#else
    #include "./core/env/env_cpp_features.h"
#endif

// D_KEYWORD_CPP
//   keyword: resolves to `cpp`.
// Used to specify that a unit of code pertains to the C++ standard.
#define D_KEYWORD_CPP               cpp

// D_KEYWORD_STL
//   keyword: resolves to `stl`.
// Used to specify that a unit of code pertains to the STL (Standard
// Template Library) part of the C++ standard.
#define D_KEYWORD_STL               stl

// D_KEYWORD_TRAITS
//   keyword: resolves to `traits`.
// Used to specify that a unit of code uses template metaprogramming
// and SFINAE for compile-time logic.
#define D_KEYWORD_TRAITS            traits

// D_NAMESPACE
//   macro: wraps a block of code in a namespace with the name
// specified by parameter `NAME`.
#define D_NAMESPACE(NAME)	        namespace NAME {

// NS_END
//   Namespace idiom; used to close any namespace.
#define NS_END				        };

// NS_CLI
//   namespace: the CLI namespace
#define NS_CLI		        D_NAMESPACE(D_KEYWORD_CLI)

// NS_CONTAINER
//   namespace: used to indicate the `djinterp` top-level namespace.
// Should be used as the top-level namespace of any and all modules
// within the djinterp tool-chain.
#define NS_CONTAINER		D_NAMESPACE(D_KEYWORD_CONTAINER)

// NS_DJINTERP
//   namespace: used to indicate the `djinterp` top-level namespace.
// Should be used as the top-level namespace of any and all modules
// within the djinterp tool-chain.
#define NS_DJINTERP			D_NAMESPACE(D_KEYWORD_FRAMEWORK_NAME)

// NS_ERROR
//   
#define NS_ERROR			D_NAMESPACE(D_KEYWORD_ERROR)

// NS_EXCEPTION
//   
#define NS_EXCEPTION		D_NAMESPACE(D_KEYWORD_EXCEPTION)

// NS_FUNCTIONAL
//   
#define NS_FUNCTIONAL		D_NAMESPACE(D_KEYWORD_FUNCTIONAL)

// NS_INTERNAL
//   namespace: declares an `internal` namespace.
// Used to hide implementation details from regular use, such as
// "helper" types, structs and functions. Should be closed with
// `NS_END`.
#define NS_INTERNAL			D_NAMESPACE(D_KEYWORD_INTERNAL)

// NS_MATHS
//   namespace: used for the `maths` submodule namespace for C++.
#define NS_MATH 			D_NAMESPACE(D_KEYWORD_MATH)

// NS_MESSAGE
//   namespace: used for variables, macros, namespaces, etc. that
// convey (usually string-based) human-readable information. These
// messages are often (but not limited to) debugging and
// error-handling.
#define NS_MESSAGE			D_NAMESPACE(D_KEYWORD_MESSAGE)

// NS_STL
//   
#define NS_STL			    D_NAMESPACE(D_KEYWORD_STL)

// NS_TEST
//   
#define NS_TEST			    D_NAMESPACE(D_KEYWORD_TEST)

// NS_TRAITS
//   
#define NS_TRAITS			D_NAMESPACE(D_KEYWORD_TRAITS)


// ================================================================
//  D_CONSTEXPR test-stripping override
// ================================================================
// D_CONSTEXPR is now defined in env.h (Section VII-C). When
// D_TESTING_CONSTEXPR is set to 1, strip constexpr to allow
// runtime instrumentation of otherwise-constexpr code paths.
#if ( defined(D_TESTING_CONSTEXPR) &&                                         \
      (D_TESTING_CONSTEXPR == 1) )
    #ifdef D_CONSTEXPR
        #undef D_CONSTEXPR
    #endif  // D_CONSTEXPR

    #define D_CONSTEXPR
#else
    #ifndef D_CONSTEXPR
        #define D_CONSTEXPR             constexpr
    #endif  // D_CONSTEXPR
#endif // D_TESTING_CONSTEXPR

#define D_STATIC_CONSTEXPR              D_STATIC D_CONSTEXPR
#define D_CONSTEXPR_INLINE              D_CONSTEXPR D_INLINE
#define D_STATIC_CONSTEXPR_INLINE       D_STATIC D_CONSTEXPR D_INLINE

// D_NO_UNIQUE_ADDRESS
//   Indicates that a non-static data member need not have an address
// distinct from all other non-static data members of its class.
// Allows the compiler to optimize empty members to occupy no space,
// which is particularly useful for storing stateless allocators,
// comparators, and policy objects.
//
//   Resolution order:
//     1. C++20 — [[no_unique_address]] is standard.
//     2. __has_cpp_attribute(no_unique_address) — catches compilers
//        that support the attribute before the standard mandates it.
//     3. MSVC — [[msvc::no_unique_address]] is used in place of the
//        standard spelling; MSVC accepted the vendor-prefixed form
//        before recognising the standard attribute.
//     4. Everything else — empty (member occupies at least one byte,
//        but no breakage).
//
//   Pre-definable: users may #define D_NO_UNIQUE_ADDRESS before
// including this header to override the detected value.
#ifndef D_NO_UNIQUE_ADDRESS
    #if defined(__cplusplus)
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            #define D_NO_UNIQUE_ADDRESS [[no_unique_address]]
        #elif defined(__has_cpp_attribute)
            #if __has_cpp_attribute(no_unique_address)
                #define D_NO_UNIQUE_ADDRESS [[no_unique_address]]
            #endif
        #endif

        // ---- MSVC vendor-prefixed fallback ----
        #ifndef D_NO_UNIQUE_ADDRESS
            #if defined(D_ENV_COMPILER_MSVC)
                #if defined(__has_cpp_attribute)
                    #if __has_cpp_attribute(msvc::no_unique_address)
                        #define D_NO_UNIQUE_ADDRESS \
                            [[msvc::no_unique_address]]
                    #endif
                #endif
            #endif
        #endif  // D_NO_UNIQUE_ADDRESS (MSVC fallback)
    #endif  // __cplusplus

    // ---- no-op fallback ----
    #ifndef D_NO_UNIQUE_ADDRESS
        #define D_NO_UNIQUE_ADDRESS
    #endif  // D_NO_UNIQUE_ADDRESS (final fallback)
#endif  // D_NO_UNIQUE_ADDRESS (outer guard)

NS_DJINTERP

// ================================================================
//  void_t
// ================================================================

// void_t
//   type: maps any type sequence to void. Used as the SFINAE
// sink in detection idioms. Pre-C++17 replacement for
// std::void_t.
template<typename...>
using void_t = void;


// ================================================================
//  abs_value
// ================================================================

// abs_value
//   trait: computes the absolute value of a compile-time integral
// constant.
template<typename _Type,
            _Type   _N>
struct abs_value
{
    static_assert(std::is_integral<_Type>::value,
                    "Type parameter `_Type` must be an integral type.");

    static constexpr _Type value = (_N < 0)
                                    ? -(_N)
                                    : _N;
};

//
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // abs_value_v
    //   
    template<typename _Type,
             _Type    _N>
    constexpr _Type abs_value_v = abs_value<_Type, _N>::value;
#endif

// 
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // abs_value_to_size_t
    //   
    template<typename _Type,
             _Type    _N>
    constexpr std::size_t abs_value_to_size_t =
        std::integral_constant<
            std::size_t,
            abs_value<_Type, _N>::value
        >::value;
#endif


// ================================================================
//  clean
// ================================================================

// clean
//   type modifier: strips cv-qualifiers and references.
template<typename _Type>
struct clean
{
    using type = std::remove_cv_t<std::remove_reference_t<_Type>>;
};

// clean_t
//   type: convenience alias for clean<_Type>::type.
template<typename _Type>
using clean_t = clean<_Type>::type;

// constexpr_swap
//   function: swaps two values in a constexpr context.
// std::swap is not constexpr until C++20, so this provides
// the same semantics for C++14 and C++17.
//
// C++20+  — delegates to std::swap (already constexpr).
// C++14+  — manual move-based swap under relaxed constexpr.
// C++11   — identical body, but not constexpr (C++11 constexpr
//           forbids local variables and assignments).

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap
    (
        _Type& _a,
        _Type& _b
    )
    noexcept( std::is_nothrow_move_constructible<_Type>::value &&
              std::is_nothrow_move_assignable<_Type>::value )
    {
        std::swap(_a, _b);

        return;
    }

#elif D_ENV_LANG_IS_CPP14_OR_HIGHER

    template<typename _Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap
    (
        _Type& _a,
        _Type& _b
    )
    noexcept( std::is_nothrow_move_constructible<_Type>::value &&
              std::is_nothrow_move_assignable<_Type>::value )
    {
        _Type temp = static_cast<_Type&&>(_a);
        _a         = static_cast<_Type&&>(_b);
        _b         = static_cast<_Type&&>(temp);

        return;
    }

#else  // C++11

    template<typename _Type>
    D_INLINE void
    constexpr_swap
    (
        _Type& _a,
        _Type& _b
    )
    noexcept( std::is_nothrow_move_constructible<_Type>::value &&
              std::is_nothrow_move_assignable<_Type>::value )
    {
        _Type temp = static_cast<_Type&&>(_a);
        _a         = static_cast<_Type&&>(_b);
        _b         = static_cast<_Type&&>(temp);

        return;
    }

#endif

// ================================================================
//  repeat
// ================================================================

NS_INTERNAL

    // repeat_type_helper
    //   trait: internal recursive builder for repeat<> (general case).
    template<typename    _Type,
             std::size_t _N,
             typename... _Types>
    struct repeat_type_helper
    {
        using type = typename repeat_type_helper<_Type,
                                                 (_N - 1),
                                                 _Type, 
                                                 _Types...>::type;
    };

    // repeat_type_helper<_Type, 0, _Types...>
    //   trait: base case specialization producing the final tuple.
    template<typename    _Type,
             typename... _Types>
    struct repeat_type_helper<_Type, 0, _Types...>
    {
        using type = std::tuple<_Types...>;
    };

NS_END  // internal

// repeat
//   trait: produces a std::tuple containing _Type repeated
// _NumTimes times.
template<typename    _Type,
         std::size_t _NumTimes>
struct repeat
{
    using type = std::conditional_t<(_NumTimes > 0),
        typename internal::repeat_type_helper<_Type, _NumTimes>::type,
        std::tuple<>
    >;
};

// repeat_t
//   type: convenience alias for repeat<_Type, _NumTimes>::type.
template<typename    _Type,
            std::size_t _NumTimes>
using repeat_t = typename repeat<_Type, _NumTimes>::type;


// ================================================================
//  self  /  resolve_self
// ================================================================

// self
//   type: self-reference marker for recursive type definitions.
struct self
{};

// is_self
//   trait: detects the self marker type (primary template).
template<typename _Type>
struct is_self : std::false_type
{};

// is_self<self>
//   trait: specialization recognizing the self marker.
template<>
struct is_self<self> : std::true_type
{};

// is_self_v
//   value: convenience alias for is_self<_Type>::value.
template<typename _Type>
inline constexpr bool is_self_v = is_self<_Type>::value;

// resolve_self
//   trait: resolves the self marker within a type to a concrete
// target type (primary template, passthrough).
template<typename _Type,
         typename _ResolveTo>
struct resolve_self
{
    using type = _Type;
};

// resolve_self<self, _ResolveTo>
//   trait: base case — self resolves to _ResolveTo.
template<typename _ResolveTo>
struct resolve_self<self, _ResolveTo>
{
    using type = _ResolveTo;
};

// resolve_self<std::unique_ptr<self>, _ResolveTo>
//   trait: smart pointer specialization for unique_ptr.
template<typename _ResolveTo>
struct resolve_self<std::unique_ptr<self>, _ResolveTo>
{
    using type = std::unique_ptr<_ResolveTo>;
};

// resolve_self<std::shared_ptr<self>, _ResolveTo>
//   trait: smart pointer specialization for shared_ptr.
template<typename _ResolveTo>
struct resolve_self<std::shared_ptr<self>, _ResolveTo>
{
    using type = std::shared_ptr<_ResolveTo>;
};

// resolve_self<std::weak_ptr<self>, _ResolveTo>
//   trait: smart pointer specialization for weak_ptr.
template<typename _ResolveTo>
struct resolve_self<std::weak_ptr<self>, _ResolveTo>
{
    using type = std::weak_ptr<_ResolveTo>;
};

// resolve_self<self*, _ResolveTo>
//   trait: raw pointer specialization.
template<typename _ResolveTo>
struct resolve_self<self*, _ResolveTo>
{
    using type = _ResolveTo*;
};

// resolve_self_t
//   type: convenience alias for resolve_self<...>::type.
template<typename _Type,
            typename _ResolveTo>
using resolve_self_t = typename resolve_self<_Type, _ResolveTo>::type;

// resolve_self<_Template<_Args...>, _ResolveTo>
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
template<template<typename...> class _Template,
         typename...                 _Args,
         typename                    _ResolveTo>
struct resolve_self<_Template<_Args...>, _ResolveTo>
{
    using type = _Template<resolve_self_t<_Args, _ResolveTo>...>;
};

// resolve_self<repeat<_Type, _NumTimes>, _ResolveTo>
//   trait: recursively resolves self within a repeated type.
template<typename    _Type,
         std::size_t _NumTimes,
         typename    _ResolveTo>
struct resolve_self<repeat<_Type, _NumTimes>, _ResolveTo>
{
    using resolved_inner = resolve_self_t<_Type, _ResolveTo>;
    using type           = repeat_t<resolved_inner, _NumTimes>;
};

NS_END  // djinterp


#endif  // DJINTERP_CPP_
