/******************************************************************************
* djinterp [restd]                                is_trivially_constructible.hpp
*
* is_trivially_constructible trait header:
*   Yields true_type if `_Type t(declval<_Args>()...);` is well-formed
* and the construction is trivial (no user-defined or non-trivial
* operations called), false_type otherwise. Implemented via the
* `__is_trivially_constructible` builtin where available; degrades to
* false_type otherwise (no portable detection of triviality exists).
*
*     is_trivially_constructible<int>::value             -> true
*     is_trivially_constructible<int, int>::value        -> true
*     struct A { A() {} };
*     is_trivially_constructible<A>::value               -> false
*
*   DETECTION MACRO:
*   D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_trivially_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_CONSTRUCTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: variadic templates required
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./false_type.hpp"


#ifndef D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_constructible)
            #define D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE  1
        #else
            #define D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE      1
    #else
        #define D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE      0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_TRIVIALLY_CONSTRUCTIBLE
// =============================================================================

#if D_RESTD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE

    template<typename    _Type,
             typename... _Args>
    struct is_trivially_constructible
        : integral_constant<bool,
              __is_trivially_constructible(_Type, _Args...)>
    {};

#else

    template<typename    _Type,
             typename... _Args>
    struct is_trivially_constructible : false_type
    {};

#endif


// =============================================================================
// II.  IS_TRIVIALLY_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename    _Type,
             typename... _Args>
    D_CONSTEXPR bool is_trivially_constructible_v =
        is_trivially_constructible<_Type, _Args...>::value;

#endif


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_CONSTRUCTIBLE_
