/******************************************************************************
* djinterp [restd]                                                is_union.hpp
*
* is_union trait header:
*   Yields true_type if _Type is a union, false_type otherwise.
* Implemented via the `__is_union` compiler builtin where available.
*
*     union U {};
*     is_union<U>::value           -> true
*     is_union<int>::value         -> false
*
*   FALLBACK (no intrinsic):
*   No portable C++ implementation exists. The fallback degrades to
* false_type.
*
*   DETECTION MACRO:
*   D_RESTD_HAS_IS_UNION is defined to 1 when the intrinsic is present,
* 0 otherwise. Predefinable.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_union.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_UNION_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_UNION_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RESTD_HAS_IS_UNION  (intrinsic detection)
// =============================================================================

#ifndef D_RESTD_HAS_IS_UNION
    #if defined(__has_builtin)
        #if __has_builtin(__is_union)
            #define D_RESTD_HAS_IS_UNION    1
        #else
            #define D_RESTD_HAS_IS_UNION    0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_IS_UNION        1
    #else
        #define D_RESTD_HAS_IS_UNION        0
    #endif
#endif  // D_RESTD_HAS_IS_UNION


NS_RESTD


// =============================================================================
// I.   IS_UNION
// =============================================================================

#if D_RESTD_HAS_IS_UNION

    // is_union
    //   trait: true if _Type is a union.
    template<typename _Type>
    struct is_union : integral_constant<bool, __is_union(_Type)>
    {};

#else

    // is_union
    //   trait: degraded fallback (always false) when intrinsic is absent.
    template<typename _Type>
    struct is_union : false_type
    {};

#endif  // D_RESTD_HAS_IS_UNION


// =============================================================================
// II.  IS_UNION_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_union_v
    //   variable: convenience for is_union<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_union_v = is_union<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_UNION_
