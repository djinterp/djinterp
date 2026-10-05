/*******************************************************************************
* djinterp [re_std]                               is_trivially_constructible.hpp
*
* is_trivially_constructible trait header:
*   Yields true_type if `Type t(declval<Args>()...);` is well-formed
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
*   RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE.
*
*
* path:      /inc/re_std/type_traits/is_trivially_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_TRIVIALLY_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_TRIVIALLY_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: variadic templates required
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./false_type.hpp"


#ifndef RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_constructible)
            #define RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE  1
        #else
            #define RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE      1
    #else
        #define RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE      0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_TRIVIALLY_CONSTRUCTIBLE
// =============================================================================

#if RE_STD_HAS_IS_TRIVIALLY_CONSTRUCTIBLE

    template<typename    Type,
             typename... Args>
    struct is_trivially_constructible
        : integral_constant<bool,
              __is_trivially_constructible(Type, Args...)>
    {};

#else

    template<typename    Type,
             typename... Args>
    struct is_trivially_constructible : false_type
    {};

#endif


// =============================================================================
// II.  IS_TRIVIALLY_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename    Type,
             typename... Args>
    RE_STD_CONSTEXPR bool is_trivially_constructible_v =
        is_trivially_constructible<Type, Args...>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_TRIVIALLY_CONSTRUCTIBLE_HPP
