/******************************************************************************
* djinterp [core]                                                 djinterp.hpp
*
*   C++ core header for the djinterp framework. Extends the C core with
* namespace macros, constexpr support, and foundational type utilities
* including type cleaning, compile-time repetition, and self-referential
* type resolution.
*
* path:      /inc/cpp/djinterp.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2023.11.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    C++ KEYWORDS & NAMESPACE MACROS
      --------------------------------
      i.    C++ keywords
            a. D_KEYWORD_CPP
            b. D_KEYWORD_STL
            c. D_KEYWORD_TRAITS
      ii.   namespace macros
            a. D_NAMESPACE
            b. NS_END
            c. NS_CLI
            d. NS_DJINTERP
            e. NS_ERROR
            f. NS_EXCEPTION
            g. NS_FUNCTIONAL
            h. NS_INTERNAL
            i. NS_MATH
            j. NS_MESSAGE
            k. NS_STL
            l. NS_TEST
            m. NS_TRAITS

II.   CONSTEXPR SUPPORT
      ------------------
      a. D_CONSTEXPR
      b. static constexpr
      c. D_CONSTEXPR_INLINE
      d. static constexpr_INLINE
      e. noexcept

III.  CORE TYPE UTILITIES (namespace djinterp)
      -----------------------------------------
      i.    void_t
      ii.   abs_value
            a. abs_value_v
            b. abs_value_to_size_t
      iii.  clean
            a. clean_t
      iv.   constexpr_swap
      v.    repeat
            a. repeat_type_helper (internal)
            b. repeat_t
      vi.   self / resolve_self
            a. self
            b. is_self
            c. is_self<self>
            d. is_self_v
            e. resolve_self (+ specializations)
            f. resolve_self_t
*/

#ifndef DJINTERP_CPP_
#define DJINTERP_CPP_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <memory>
// djinterp
#include "../c/djinterp.h"   // also pulls in env.h + env_attributes.h + env_vendor_attributes.h
#include "./env/cpp/env_cpp98.h"


#if (D_ENV_LANG_IS_CPP11_OR_HIGHER)
    #include "./env/cpp/env_cpp_features.h"
#endif


///////////////////////////////////////////////////////////////////////////////
///                I.   C++ KEYWORDS & NAMESPACE MACROS                     ///
///////////////////////////////////////////////////////////////////////////////

// i.   C++ keywords
//////////////////////////////////////////

// D_KEYWORD_CPP
//   keyword: resolves to `cpp`.
// Used to specify that a unit of code pertains to the C++
// standard.
#define D_KEYWORD_CPP               cpp

// D_KEYWORD_PARADIGM
//   keyword: resolves to `paradigm`.
// Used to specify that a unit of code pertains to a pattern or software
// paradigm.
#define D_KEYWORD_PARADIGM          paradigm

// D_KEYWORD_RESTD
//   keyword: resolves to `restd`.
// Used to specify the re-std, a more-portable, backwards-compatible version 
// of the std library.
#define D_KEYWORD_RESTD             restd

// D_KEYWORD_STL
//   keyword: resolves to `stl`.
// Used to specify that a unit of code pertains to the STL
// (Standard Template Library) part of the C++ standard.
#define D_KEYWORD_STL               stl

// D_KEYWORD_TRAITS
//   keyword: resolves to `traits`.
// Used to specify that a unit of code uses template
// metaprogramming and SFINAE for compile-time logic.
#define D_KEYWORD_TRAITS            traits

// D_KEYWORD_CONCEPTS
//   keyword: resolves to `concepts`.
// Used to specify that a unit of code provides C++20
// concept definitions.
#define D_KEYWORD_CONCEPTS          concepts

// D_KEYWORD_CONTAINER
//   keyword: resolves to `container`.
// Used to specify that a unit of code pertains to the
// container subsystem.
#define D_KEYWORD_CONTAINER         container


// ii.  namespace macros
//////////////////////////////////////////

// D_NAMESPACE
//   macro: wraps a block of code in a namespace with the name
// specified by parameter `NAME`.
#define D_NAMESPACE(NAME)           namespace NAME {

// NS_END
//   macro: namespace idiom; used to close any namespace.
#define NS_END                      };

// NS_CONCEPTS
//   namespace: the `concepts` namespace for C++20 concept
// definitions layered over a subsystem's trait surface.
#define NS_CONCEPTS                 D_NAMESPACE(D_KEYWORD_CONCEPTS)

// NS_CONTAINER
//   namespace: the `container` namespace for the container
// subsystem and its trait/concept surface.
#define NS_CONTAINER                D_NAMESPACE(D_KEYWORD_CONTAINER)

// NS_DATABASE
//   namespace: database namespace containing functionality pertaining to
// databases and database systems.
#define NS_DATABASE                 D_NAMESPACE(D_KEYWORD_DATABASE)

// NS_DJINTERP
//   namespace: the `djinterp` top-level namespace. Should be
// used as the top-level namespace of any and all modules
// within the djinterp tool-chain.
#define NS_DJINTERP                 D_NAMESPACE(D_KEYWORD_FRAMEWORK_NAME)

// NS_ERROR
//   namespace: the `error` namespace for error-handling types
// and utilities.
#define NS_ERROR                    D_NAMESPACE(D_KEYWORD_ERROR)

// NS_EXCEPTION
//   namespace: the `exception` namespace for severe
// error-handling types and utilities.
#define NS_EXCEPTION                D_NAMESPACE(D_KEYWORD_EXCEPTION)

// NS_INTERNAL
//   namespace: declares an `internal` namespace. Used to hide
// implementation details from regular use, such as "helper"
// types, structs and functions. Should be closed with `NS_END`.
#define NS_INTERNAL                 D_NAMESPACE(D_KEYWORD_INTERNAL)

// NS_MATH
//   namespace: the `math` namespace for mathematical
// utilities.
#define NS_MATH                     D_NAMESPACE(D_KEYWORD_MATH)

// NS_MESSAGE
//   namespace: the `message` namespace for variables, macros,
// and types that convey (usually string-based) human-readable
// information. Often (but not limited to) debugging and
// error-handling.
#define NS_MESSAGE                  D_NAMESPACE(D_KEYWORD_MESSAGE)

// NS_PARADIGM
//   namespace: resolves to `namespace paradigm {`
// This namespace contains common software patterns or idioms.
#define NS_PARADIGM                 D_NAMESPACE(D_KEYWORD_PARADIGM)

// NS_RESTD
//   keyword: the re-std namespace, containing more-portable, 
// backwards-compatible versions of std library types, functions,
// and more.
#define NS_RESTD                    D_NAMESPACE(D_KEYWORD_RESTD)

// NS_TEST
//   namespace: the `test` namespace for unit testing
// utilities.
#define NS_TEST                     D_NAMESPACE(D_KEYWORD_TEST)

// NS_TESTING
//   namespace: the `testing` namespace for holding unit tests.
#define NS_TESTING                  D_NAMESPACE(D_KEYWORD_TESTING)

// NS_TRAITS
//   namespace: the `traits` namespace for the compile-time
// trait surface of a subsystem (e.g. djinterp::container::traits).
#define NS_TRAITS                   D_NAMESPACE(D_KEYWORD_TRAITS)

//////////////////////////////////////////////////////////////////////////////
///                       II.   QUALIFIER SUPPORT                         ///
//////////////////////////////////////////////////////////////////////////////
//   C++-only qualifiers: the constexpr family. The storage / inlining
// qualifiers (D_STATIC, D_INLINE, D_STATIC_INLINE) are defined ONCE in
// djinterp.h for both languages and inherited here -- not redefined, so there
// is no C vs. C++ drift. Gates come from qual_cfg.h (pulled in via djinterp.h).

// D_CONSTEXPR
//   qualifier: `constexpr` on C++11+. Expands to nothing when the config layer
// sets D_INTERNAL_QUAL_STRIP_CONSTEXPR (test instrumentation), or on pre-C++11.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR
        #if D_INTERNAL_QUAL_STRIP_CONSTEXPR
            #define D_CONSTEXPR             // stripped for test instrumentation
        #elif D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_CONSTEXPR             constexpr
        #else
            #define D_CONSTEXPR             // pre-C++11: no constexpr
        #endif
    #endif
#endif

// -- constexpr compound qualifiers (order: static constexpr inline) --
//   Composed from D_STATIC / D_INLINE (from djinterp.h) and D_CONSTEXPR above;
// each honors its own toggle. Valid in C++ because D_INLINE carries no
// `static` here. (The non-constexpr D_STATIC_INLINE lives in djinterp.h.)
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_STATIC_CONSTEXPR
        #define D_STATIC_CONSTEXPR          D_STATIC D_CONSTEXPR
    #endif
    #ifndef D_CONSTEXPR_INLINE
        #define D_CONSTEXPR_INLINE          D_CONSTEXPR D_INLINE
    #endif
    #ifndef D_STATIC_CONSTEXPR_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   D_STATIC D_CONSTEXPR D_INLINE
    #endif
#endif

// D_NOEXCEPT
//   Portable no-throw specifier.  In C++11 and later `noexcept` is a
//   first-class keyword; older C++ compilers accept the deprecated
//   `throw()` as a close equivalent.  In C mode the concept does not
//   exist, so the macro expands to nothing.
//
//   Resolution order:
//     1. C++11 - noexcept is standard.
//     2. Pre-C++11 C++ - throw() (deprecated but widely supported).
//     3. C / everything else - no-op fallback.
//
//   Pre-definable: users may #define D_NOEXCEPT before including this
//   header to override the detected value.
#ifndef D_NOEXCEPT
    #if defined(__cplusplus)
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_NOEXCEPT noexcept
        #else
            #define D_NOEXCEPT throw()
        #endif
    #else
        #define D_NOEXCEPT
    #endif
#endif  // D_NOEXCEPT

// D_NOEXCEPT_IF
//   conditional exception specification. Expands to noexcept(cond)
//   on C++11+. Pre-C++11 has no conditional spelling (throw() is
//   unconditional), so the condition is dropped to a no-op; in C
//   mode it is likewise a no-op.
//   Variadic so a condition containing a top-level comma (e.g. a
//   multi-arg trait) passes through intact.
#ifndef D_NOEXCEPT_IF
    #if defined(__cplusplus) && D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NOEXCEPT_IF(...) noexcept(__VA_ARGS__)
    #else
        #define D_NOEXCEPT_IF(...)
    #endif
#endif  // D_NOEXCEPT_IF


///////////////////////////////////////////////////////////////////////////////
///            III.   CORE TYPE UTILITIES (namespace djinterp)              ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP

// void_t
//   type: maps any type sequence to void. Used as the SFINAE
// sink in detection idioms. Pre-C++17 replacement for
// std::void_t.
template<typename...>
using void_t = void;

// abs_value
//   trait: computes the absolute value of a compile-time
// integral constant.
template<typename _Type,
         _Type    _N>
struct abs_value
{
    static_assert(std::is_integral<_Type>::value,
                  "Type parameter `_Type` must be an integral type.");

    static constexpr _Type value = (_N < 0)
                                   ? -(_N)
                                   : _N;
};

// abs_value_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // abs_value_v
    //   value: convenience variable template for
    // abs_value<_Type, _N>::value.
    template<typename _Type,
             _Type    _N>
    constexpr _Type abs_value_v = abs_value<_Type, _N>::value;
#endif

// abs_value_to_size_t (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // abs_value_to_size_t
    //   value: convenience variable template that yields the
    // absolute value of _N as a std::size_t.
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
// C++20+  -- delegates to std::swap (already constexpr).
// C++14+  -- manual move-based swap under relaxed constexpr.
// C++11   -- identical body, but not constexpr (C++11 constexpr forbids local
//            variables and assignments).

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
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
    constexpr_swap(
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
    constexpr_swap(
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
    //   trait: internal recursive builder for repeat<>
    // (general case).
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
    //   trait: base case specialization producing the final
    // tuple.
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
        typename internal::repeat_type_helper<_Type,
                                              _NumTimes>::type,
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
D_CONSTEXPR_INLINE bool is_self_v = is_self<_Type>::value;

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
//   trait: base case -- self resolves to _ResolveTo.
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