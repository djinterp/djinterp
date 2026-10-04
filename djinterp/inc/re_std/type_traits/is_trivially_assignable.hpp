/******************************************************************************
* djinterp [re_std]                                 is_trivially_assignable.hpp
*
* is_trivially_assignable trait header:
*   Yields true_type if `_To = _From` is well-formed and the assignment
* is trivial. Intrinsic-backed; degrades to false_type otherwise.
*
*   DETECTION MACRO:
*   D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_trivially_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIALLY_ASSIGNABLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIALLY_ASSIGNABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./false_type.hpp"


#ifndef D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_assignable)
            #define D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE     1
        #else
            #define D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE     0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE         1
    #else
        #define D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE         0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_TRIVIALLY_ASSIGNABLE
// =============================================================================

#if D_RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE

    template<typename _To,
             typename _From>
    struct is_trivially_assignable
        : integral_constant<bool, __is_trivially_assignable(_To, _From)>
    {};

#else

    template<typename _To,
             typename _From>
    struct is_trivially_assignable : false_type
    {};

#endif


// =============================================================================
// II.  IS_TRIVIALLY_ASSIGNABLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _To,
             typename _From>
    D_CONSTEXPR bool is_trivially_assignable_v =
        is_trivially_assignable<_To, _From>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIALLY_ASSIGNABLE_
