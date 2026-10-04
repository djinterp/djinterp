/******************************************************************************
* djinterp [re_std]                                         is_constructible.hpp
*
* is_constructible trait header:
*   Yields true_type if a hypothetical variable definition
* `_Type t(declval<_Args>()...);` is well-formed, false_type otherwise.
* Implemented via the `__is_constructible` builtin where available;
* otherwise via a SFINAE probe on direct-initialization syntax.
*
*     is_constructible<int>::value                 -> true   (default)
*     is_constructible<int, int>::value            -> true   (from int)
*     is_constructible<int, void*>::value          -> false
*
*     struct A { A(int, char); };
*     is_constructible<A, int, char>::value        -> true
*     is_constructible<A>::value                   -> false (no default ctor)
*
*   PORTABILITY:
*   The C++11+ portable fallback uses variadic templates and decltype.
* The trait is omitted entirely on C++98/03; consumer code must gate
* on D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES.
*
*   DETECTION MACRO:
*   D_RE_STD_HAS_IS_CONSTRUCTIBLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_CONSTRUCTIBLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_CONSTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: variadic templates required
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// djinterp
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./add_rvalue_reference.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_CONSTRUCTIBLE  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_CONSTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_constructible)
            #define D_RE_STD_HAS_IS_CONSTRUCTIBLE    1
        #else
            #define D_RE_STD_HAS_IS_CONSTRUCTIBLE    0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_CONSTRUCTIBLE        1
    #else
        #define D_RE_STD_HAS_IS_CONSTRUCTIBLE        0
    #endif
#endif  // D_RE_STD_HAS_IS_CONSTRUCTIBLE


NS_RESTD


// =============================================================================
// I.   IS_CONSTRUCTIBLE
// =============================================================================

#if D_RE_STD_HAS_IS_CONSTRUCTIBLE

    // is_constructible (intrinsic)
    template<typename    _Type,
             typename... _Args>
    struct is_constructible
        : integral_constant<bool, __is_constructible(_Type, _Args...)>
    {};

#else


    NS_INTERNAL

        // declval shim for is_constructible (private to this header).
        template<typename _T>
        typename add_rvalue_reference<_T>::type
            is_ctor_declval() D_NOEXCEPT;

        // is_ctor_probe
        //   helper: SFINAE probe on `_Type(declval<_Args>()...)`.
        template<typename    _Type,
                 typename... _Args>
        struct is_ctor_probe
        {
        private:
            template<typename    _T,
                     typename... _A,
                     typename = decltype(_T(is_ctor_declval<_A>()...))>
            static true_type test(int);

            template<typename, typename...>
            static false_type test(...);

        public:
            typedef decltype(test<_Type, _Args...>(0)) type;
            D_STATIC_CONSTEXPR bool value = type::value;
        };

    NS_END  // internal


    // is_constructible (portable C++11+)
    template<typename    _Type,
             typename... _Args>
    struct is_constructible
        : integral_constant<bool,
              internal::is_ctor_probe<_Type, _Args...>::value>
    {};


#endif  // D_RE_STD_HAS_IS_CONSTRUCTIBLE


// =============================================================================
// II.  IS_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename    _Type,
             typename... _Args>
    D_CONSTEXPR bool is_constructible_v =
        is_constructible<_Type, _Args...>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_CONSTRUCTIBLE_
