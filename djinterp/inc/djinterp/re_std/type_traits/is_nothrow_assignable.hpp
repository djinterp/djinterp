/******************************************************************************
* djinterp [restd]                                    is_nothrow_assignable.hpp
*
* is_nothrow_assignable trait header:
*   Yields true_type if `_To = _From` is well-formed AND the assignment
* expression is `noexcept`. Intrinsic-backed; falls back to
* `is_assignable && noexcept(...)` probe.
*
*   DETECTION MACRO:
*   D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_ASSIGNABLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_ASSIGNABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_assignable.hpp"
#include "./add_rvalue_reference.hpp"


#ifndef D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_nothrow_assignable)
            #define D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE       1
        #else
            #define D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE       0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE           1
    #else
        #define D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE           0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_NOTHROW_ASSIGNABLE
// =============================================================================

#if D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE

    template<typename _To,
             typename _From>
    struct is_nothrow_assignable
        : integral_constant<bool, __is_nothrow_assignable(_To, _From)>
    {};

#else


    NS_INTERNAL

        template<typename _T>
        typename add_rvalue_reference<_T>::type
            is_nothrow_assign_declval() D_NOEXCEPT;

        template<typename _To,
                 typename _From,
                 bool     _IsAssignable>
        struct is_nothrow_assign_probe
        {
            D_STATIC_CONSTEXPR bool value = false;
        };

        template<typename _To,
                 typename _From>
        struct is_nothrow_assign_probe<_To, _From, true>
        {
            D_STATIC_CONSTEXPR bool value =
                noexcept(is_nothrow_assign_declval<_To>() =
                         is_nothrow_assign_declval<_From>());
        };

    NS_END  // internal


    template<typename _To,
             typename _From>
    struct is_nothrow_assignable
        : integral_constant<bool,
              internal::is_nothrow_assign_probe<
                  _To,
                  _From,
                  is_assignable<_To, _From>::value
              >::value>
    {};


#endif  // D_RESTD_HAS_IS_NOTHROW_ASSIGNABLE


// =============================================================================
// II.  IS_NOTHROW_ASSIGNABLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _To,
             typename _From>
    D_CONSTEXPR bool is_nothrow_assignable_v =
        is_nothrow_assignable<_To, _From>::value;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_ASSIGNABLE_
