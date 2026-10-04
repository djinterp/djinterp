/******************************************************************************
* djinterp [re_std]                                 is_nothrow_constructible.hpp
*
* is_nothrow_constructible trait header:
*   Yields true_type if `_Type t(declval<_Args>()...);` is well-formed
* AND the construction is `noexcept`, false_type otherwise. Implemented
* via the `__is_nothrow_constructible` builtin where available; falls
* back to `is_constructible && noexcept(_Type(declval<_Args>()...))`.
*
*     is_nothrow_constructible<int>::value             -> true
*     is_nothrow_constructible<int, int>::value        -> true
*     struct A { A() noexcept; };
*     is_nothrow_constructible<A>::value               -> true
*
*   DETECTION MACRO:
*   D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_CONSTRUCTIBLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: variadic templates + noexcept (effectively C++11+)
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_constructible.hpp"
#include "./add_rvalue_reference.hpp"


#ifndef D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_nothrow_constructible)
            #define D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE    1
        #else
            #define D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE    0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE        1
    #else
        #define D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE        0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_NOTHROW_CONSTRUCTIBLE
// =============================================================================

#if D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE

    template<typename    _Type,
             typename... _Args>
    struct is_nothrow_constructible
        : integral_constant<bool,
              __is_nothrow_constructible(_Type, _Args...)>
    {};

#else


    NS_INTERNAL

        // declval shim for noexcept probe
        template<typename _T>
        typename add_rvalue_reference<_T>::type
            is_nothrow_ctor_declval() D_NOEXCEPT;

        // is_nothrow_ctor_probe
        //   helper: gated on is_constructible. When constructible, tests
        // whether the construction expression is noexcept.
        template<typename    _Type,
                 bool        _IsCtor,
                 typename... _Args>
        struct is_nothrow_ctor_probe
        {
            D_STATIC_CONSTEXPR bool value = false;
        };

        template<typename    _Type,
                 typename... _Args>
        struct is_nothrow_ctor_probe<_Type, true, _Args...>
        {
            D_STATIC_CONSTEXPR bool value =
                noexcept(_Type(is_nothrow_ctor_declval<_Args>()...));
        };

    NS_END  // internal


    template<typename    _Type,
             typename... _Args>
    struct is_nothrow_constructible
        : integral_constant<bool,
              internal::is_nothrow_ctor_probe<
                  _Type,
                  is_constructible<_Type, _Args...>::value,
                  _Args...
              >::value>
    {};


#endif  // D_RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE


// =============================================================================
// II.  IS_NOTHROW_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename    _Type,
             typename... _Args>
    D_CONSTEXPR bool is_nothrow_constructible_v =
        is_nothrow_constructible<_Type, _Args...>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_CONSTRUCTIBLE_
