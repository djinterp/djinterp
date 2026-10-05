/*******************************************************************************
* djinterp [re_std]                                 is_nothrow_constructible.hpp
*
* is_nothrow_constructible trait header:
*   Yields true_type if `Type t(declval<Args>()...);` is well-formed
* AND the construction is `noexcept`, false_type otherwise. Implemented
* via the `__is_nothrow_constructible` builtin where available; falls
* back to `is_constructible && noexcept(Type(declval<Args>()...))`.
*
*     is_nothrow_constructible<int>::value             -> true
*     is_nothrow_constructible<int, int>::value        -> true
*     struct A { A() noexcept; };
*     is_nothrow_constructible<A>::value               -> true
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: variadic templates + noexcept (effectively C++11+)
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_constructible.hpp"
#include "./add_rvalue_reference.hpp"


#ifndef RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_nothrow_constructible)
            #define RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE    1
        #else
            #define RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE    0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE        1
    #else
        #define RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE        0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_NOTHROW_CONSTRUCTIBLE
// =============================================================================

#if RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE

    template<typename    Type,
             typename... Args>
    struct is_nothrow_constructible
        : integral_constant<bool,
              __is_nothrow_constructible(Type, Args...)>
    {};

#else


    namespace internal
    {

        // declval shim for noexcept probe
        template<typename T>
        typename add_rvalue_reference<T>::type
            is_nothrow_ctor_declval() RE_STD_NOEXCEPT;

        // is_nothrow_ctor_probe
        //   trait: gated on is_constructible. When constructible, tests
        // whether the construction expression is noexcept.
        template<typename    Type,
                 bool        IsCtor,
                 typename... Args>
        struct is_nothrow_ctor_probe
        {
            RE_STD_STATIC_CONSTEXPR bool value = false;
        };

        template<typename    Type,
                 typename... Args>
        struct is_nothrow_ctor_probe<Type, true, Args...>
        {
            RE_STD_STATIC_CONSTEXPR bool value =
                noexcept(Type(is_nothrow_ctor_declval<Args>()...));
        };

    }  // internal


    template<typename    Type,
             typename... Args>
    struct is_nothrow_constructible
        : integral_constant<bool,
              internal::is_nothrow_ctor_probe<
                  Type,
                  is_constructible<Type, Args...>::value,
                  Args...
              >::value>
    {};


#endif  // RE_STD_HAS_IS_NOTHROW_CONSTRUCTIBLE


// =============================================================================
// II.  IS_NOTHROW_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename    Type,
             typename... Args>
    RE_STD_CONSTEXPR bool is_nothrow_constructible_v =
        is_nothrow_constructible<Type, Args...>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_CONSTRUCTIBLE_HPP
