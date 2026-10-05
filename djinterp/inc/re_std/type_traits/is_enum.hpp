/*******************************************************************************
* djinterp [re_std]                                                  is_enum.hpp
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
*   RE_STD_HAS_IS_ENUM is set to 1 if the intrinsic is available, 0
* otherwise. Users may pre-define it to override detection.
*
*
* path:      /inc/re_std/type_traits/is_enum.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_ENUM_HPP
#define RE_STD_TYPE_TRAITS_IS_ENUM_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_ENUM DETECTION
// =============================================================================

// RE_STD_HAS_IS_ENUM
//   constant: 1 if the __is_enum compiler intrinsic is available, 0
// otherwise. Users may pre-define to override.
#ifndef RE_STD_HAS_IS_ENUM
    #if defined(__has_builtin)
        #if __has_builtin(__is_enum)
            #define RE_STD_HAS_IS_ENUM 1
        #else
            #define RE_STD_HAS_IS_ENUM 0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC) ||                                   \
            defined(RE_STD_COMPILER_CLANG) ||                                 \
            defined(RE_STD_COMPILER_MSVC) ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        // __is_enum has been universally supported on these vendors for
        // long enough that further version gating is unnecessary in
        // practice. Override RE_STD_HAS_IS_ENUM if you encounter a
        // toolchain that lacks it.
        #define RE_STD_HAS_IS_ENUM 1
    #else
        #define RE_STD_HAS_IS_ENUM 0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_ENUM
// =============================================================================

#if RE_STD_HAS_IS_ENUM

    // is_enum
    //   trait: true if Type is an enumeration. Uses __is_enum builtin.
    template<typename Type>
    struct is_enum : integral_constant<bool, __is_enum(Type)>
    {};

#else

    // is_enum
    //   trait: fallback - always false when intrinsic is unavailable.
    template<typename Type>
    struct is_enum : false_type
    {};

#endif  // RE_STD_HAS_IS_ENUM


// =============================================================================
// II.  IS_ENUM_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_enum_v
    //   variable: convenience for is_enum<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_enum_v = is_enum<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_ENUM_HPP
