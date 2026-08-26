/******************************************************************************
* djinterp [re_std]                                             is_base_of.hpp
*
* is_base_of trait header:
*   is_base_of<_Base, _Derived>::value is true iff _Base is a base class of
* _Derived (or the two are the same class type), ignoring cv-qualification.
* Both must be complete class types for a meaningful answer; a non-class
* operand yields false.
*
*   IMPLEMENTATION:
*   Compiler intrinsic (__is_base_of) where available -- it is the only way to
* see private and ambiguous bases, which the standard requires to count.  The
* portable fallback is the classic conversion probe: a host type convertible to
* both `_Base*` and `_Derived*` is passed to an overload pair, and only a
* derived-to-base relation makes the `_Derived*` overload viable.  That
* fallback sees PUBLIC unambiguous bases only, which is the best a
* library-level implementation can do.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_base_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_BASE_OF_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_BASE_OF_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_class.hpp"
#include "./is_same.hpp"
#include "./remove_cv.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_BASE_OF  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_BASE_OF
    #if defined(__has_builtin)
        #if __has_builtin(__is_base_of)
            #define D_RE_STD_HAS_IS_BASE_OF  1
        #else
            #define D_RE_STD_HAS_IS_BASE_OF  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_BASE_OF      1
    #else
        #define D_RE_STD_HAS_IS_BASE_OF      0
    #endif
#endif  // D_RE_STD_HAS_IS_BASE_OF


NS_RESTD


// =============================================================================
// I.   IS_BASE_OF
// =============================================================================

#if D_RE_STD_HAS_IS_BASE_OF

// is_base_of
//   trait: true if _Base is a base of _Derived, or they are the same class.
template<typename _Base,
         typename _Derived>
struct is_base_of
    : integral_constant<bool, __is_base_of(_Base, _Derived)>
{};

#else

NS_INTERNAL

    // is_base_of_probe_
    //   helper: conversion probe.  `host_` converts to `const _Base*` always
    // and to `const _Derived*` only through its non-const operator; the
    // `_Derived*` overload of probe_ is therefore viable exactly when a
    // derived-to-base conversion exists.
    template<typename _Base,
             typename _Derived>
    struct is_base_of_probe_
    {
        typedef char yes_type_[1];
        typedef char no_type_[2];

        struct host_
        {
            operator const _Base*() const;
            operator const _Derived*();
        };

        template<typename _T>
        static yes_type_& probe_(const _Derived*, _T);
        static no_type_&  probe_(const _Base*, int);

        static D_CONSTEXPR bool value =
            ( sizeof(probe_(host_(), 0)) == sizeof(yes_type_) );
    };

NS_END  // internal

// is_base_of
//   trait: portable fallback -- public, unambiguous bases only.  A non-class
// operand is never a base; an identical class type counts as its own base.
template<typename _Base,
         typename _Derived>
struct is_base_of
    : integral_constant<bool,
        ( is_class<typename remove_cv<_Base>::type>::value    &&
          is_class<typename remove_cv<_Derived>::type>::value &&
          ( is_same<typename remove_cv<_Base>::type,
                    typename remove_cv<_Derived>::type>::value ||
            internal::is_base_of_probe_<
                typename remove_cv<_Base>::type,
                typename remove_cv<_Derived>::type>::value ) )>
{};

#endif  // D_RE_STD_HAS_IS_BASE_OF


// =============================================================================
// II.  IS_BASE_OF_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Base,
         typename _Derived>
D_CONSTEXPR bool is_base_of_v = is_base_of<_Base, _Derived>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_BASE_OF_
