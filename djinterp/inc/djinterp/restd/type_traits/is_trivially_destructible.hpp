/******************************************************************************
* djinterp [restd]                                is_trivially_destructible.hpp
*
* is_trivially_destructible trait header:
*   Yields true_type if _Type is destructible AND its destructor is
* trivial. Intrinsic-backed via `__is_trivially_destructible`
* (Clang) or `__has_trivial_destructor` (GCC/MSVC); degrades to
* false_type otherwise.
*
*   DETECTION MACRO:
*   D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE.
*
*
* path:      /inc/djinterp/restd/type_traits/is_trivially_destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./false_type.hpp"


#ifndef D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_destructible)
            #define D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE   1
            #define D_RESTD_TRIVIALLY_DESTR_(T) __is_trivially_destructible(T)
        #elif __has_builtin(__has_trivial_destructor)
            #define D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE   1
            #define D_RESTD_TRIVIALLY_DESTR_(T) __has_trivial_destructor(T)
        #else
            #define D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE   0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE       1
        #define D_RESTD_TRIVIALLY_DESTR_(T)     __has_trivial_destructor(T)
    #else
        #define D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE       0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_TRIVIALLY_DESTRUCTIBLE
// =============================================================================

#if D_RESTD_HAS_IS_TRIVIALLY_DESTRUCTIBLE

    template<typename _Type>
    struct is_trivially_destructible
        : integral_constant<bool, D_RESTD_TRIVIALLY_DESTR_(_Type)>
    {};

#else

    template<typename _Type>
    struct is_trivially_destructible : false_type
    {};

#endif


// =============================================================================
// II.  IS_TRIVIALLY_DESTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_trivially_destructible_v =
        is_trivially_destructible<_Type>::value;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_
