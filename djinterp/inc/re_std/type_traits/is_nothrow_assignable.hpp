/*******************************************************************************
* djinterp [re_std]                                    is_nothrow_assignable.hpp
*
* is_nothrow_assignable trait header:
*   Yields true_type if `To = From` is well-formed AND the assignment
* expression is `noexcept`. Intrinsic-backed; falls back to
* `is_assignable && noexcept(...)` probe.
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_NOTHROW_ASSIGNABLE.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_ASSIGNABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_ASSIGNABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_assignable.hpp"
#include "./add_rvalue_reference.hpp"


#ifndef RE_STD_HAS_IS_NOTHROW_ASSIGNABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_nothrow_assignable)
            #define RE_STD_HAS_IS_NOTHROW_ASSIGNABLE       1
        #else
            #define RE_STD_HAS_IS_NOTHROW_ASSIGNABLE       0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_NOTHROW_ASSIGNABLE           1
    #else
        #define RE_STD_HAS_IS_NOTHROW_ASSIGNABLE           0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_NOTHROW_ASSIGNABLE
// =============================================================================

#if RE_STD_HAS_IS_NOTHROW_ASSIGNABLE

    template<typename To,
             typename From>
    struct is_nothrow_assignable
        : integral_constant<bool, __is_nothrow_assignable(To, From)>
    {};

#else


    namespace internal
    {

        template<typename T>
        typename add_rvalue_reference<T>::type
            is_nothrow_assign_declval() RE_STD_NOEXCEPT;

        template<typename To,
                 typename From,
                 bool     IsAssignable>
        struct is_nothrow_assign_probe
        {
            RE_STD_STATIC_CONSTEXPR bool value = false;
        };

        template<typename To,
                 typename From>
        struct is_nothrow_assign_probe<To, From, true>
        {
            RE_STD_STATIC_CONSTEXPR bool value =
                noexcept(is_nothrow_assign_declval<To>() =
                         is_nothrow_assign_declval<From>());
        };

    }  // internal


    template<typename To,
             typename From>
    struct is_nothrow_assignable
        : integral_constant<bool,
              internal::is_nothrow_assign_probe<
                  To,
                  From,
                  is_assignable<To, From>::value
              >::value>
    {};


#endif  // RE_STD_HAS_IS_NOTHROW_ASSIGNABLE


// =============================================================================
// II.  IS_NOTHROW_ASSIGNABLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename To,
             typename From>
    RE_STD_CONSTEXPR bool is_nothrow_assignable_v =
        is_nothrow_assignable<To, From>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_ASSIGNABLE_HPP
