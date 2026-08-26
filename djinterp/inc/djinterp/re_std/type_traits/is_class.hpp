/******************************************************************************
* djinterp [re_std]                                               is_class.hpp
*
* is_class trait header:
*   Yields true_type if _Type is a class type (struct or non-union
* class), false_type otherwise. Implemented via the `__is_class`
* compiler builtin where available.
*
*     struct S {};
*     is_class<S>::value           -> true
*     is_class<int>::value         -> false
*     is_class<S*>::value          -> false
*
*   FALLBACK (no intrinsic):
*   Without `__is_class`, no portable C++ implementation exists that
* covers all cases (the SFINAE-on-pointer-to-member trick fails for
* unions on some compilers). The fallback degrades to false_type --
* matching the conservative "don't claim a property without evidence"
* convention used by is_enum.
*
*   DETECTION MACRO:
*   D_RE_STD_HAS_IS_CLASS is defined to 1 when the intrinsic is present,
* 0 otherwise. Predefinable: users may #define it before inclusion to
* override autodetection.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_class.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_CLASS_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_CLASS_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_CLASS  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_CLASS
    #if defined(__has_builtin)
        #if __has_builtin(__is_class)
            #define D_RE_STD_HAS_IS_CLASS    1
        #else
            #define D_RE_STD_HAS_IS_CLASS    0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_CLASS        1
    #else
        #define D_RE_STD_HAS_IS_CLASS        0
    #endif
#endif  // D_RE_STD_HAS_IS_CLASS


NS_RESTD


// =============================================================================
// I.   IS_CLASS
// =============================================================================

#if D_RE_STD_HAS_IS_CLASS

    // is_class
    //   trait: true if _Type is a class or struct (not union).
    template<typename _Type>
    struct is_class : integral_constant<bool, __is_class(_Type)>
    {};

#else

    // is_class
    //   trait: degraded fallback (always false) when intrinsic is absent.
    template<typename _Type>
    struct is_class : false_type
    {};

#endif  // D_RE_STD_HAS_IS_CLASS


// =============================================================================
// II.  IS_CLASS_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_class_v
    //   variable: convenience for is_class<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR bool is_class_v = is_class<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_CLASS_
