/******************************************************************************
* djinterp [re_std]                                                is_enum.hpp
*
* is_enum trait header:
*   Detects whether a type is an enumeration (scoped or unscoped).
* Implemented via the __is_enum compiler builtin, which is supported by
* GCC 4.3+, all Clang, MSVC 2012+ (v11), and Intel 13.0+.
*
*     enum E   { a, b };  is_enum<E>::value  -> true
*     enum class F : int; is_enum<F>::value  -> true   (C++11+)
*     is_enum<int>::value                    -> false
*     struct C {};        is_enum<C>::value  -> false
*
*   FALLBACK BEHAVIOR:
*   When no intrinsic is available, is_enum reports false_type for all
* types. This is functionally safe: callers that gate enum-specific
* logic on is_enum will simply never enter that path. In the `any`
* module, this causes enum values to route to heap storage rather than
* the SBO (still functional, just no SBO for enums).
*
*   DETECTION MACRO:
*   D_RE_STD_HAS_IS_ENUM is set to 1 if the intrinsic is available, 0
* otherwise. Users may pre-define it to override detection.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_enum.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_ENUM_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_ENUM_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_ENUM DETECTION
// =============================================================================

// D_RE_STD_HAS_IS_ENUM
//   constant: 1 if the __is_enum compiler intrinsic is available, 0
// otherwise. Users may pre-define to override.
#ifndef D_RE_STD_HAS_IS_ENUM
    #if defined(__has_builtin)
        #if __has_builtin(__is_enum)
            #define D_RE_STD_HAS_IS_ENUM 1
        #else
            #define D_RE_STD_HAS_IS_ENUM 0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC) ||                                   \
            defined(D_ENV_COMPILER_CLANG) ||                                 \
            defined(D_ENV_COMPILER_MSVC) ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        // __is_enum has been universally supported on these vendors for
        // long enough that further version gating is unnecessary in
        // practice. Override D_RE_STD_HAS_IS_ENUM if you encounter a
        // toolchain that lacks it.
        #define D_RE_STD_HAS_IS_ENUM 1
    #else
        #define D_RE_STD_HAS_IS_ENUM 0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_ENUM
// =============================================================================

#if D_RE_STD_HAS_IS_ENUM

    // is_enum
    //   trait: true if _Type is an enumeration. Uses __is_enum builtin.
    template<typename _Type>
    struct is_enum : integral_constant<bool, __is_enum(_Type)>
    {};

#else

    // is_enum
    //   trait: fallback - always false when intrinsic is unavailable.
    template<typename _Type>
    struct is_enum : false_type
    {};

#endif  // D_RE_STD_HAS_IS_ENUM


// =============================================================================
// II.  IS_ENUM_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_enum_v
    //   variable: convenience for is_enum<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_enum_v = is_enum<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_ENUM_
