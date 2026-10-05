/*******************************************************************************
* djinterp [re_std]                                                 is_class.hpp
*
* is_class trait header:
*   Yields true_type if Type is a class type (struct or non-union
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
*   RE_STD_HAS_IS_CLASS is defined to 1 when the intrinsic is present,
* 0 otherwise. Predefinable: users may #define it before inclusion to
* override autodetection.
*
*
* path:      /inc/re_std/type_traits/is_class.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_CLASS_HPP
#define RE_STD_TYPE_TRAITS_IS_CLASS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_CLASS  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_CLASS
    #if defined(__has_builtin)
        #if __has_builtin(__is_class)
            #define RE_STD_HAS_IS_CLASS    1
        #else
            #define RE_STD_HAS_IS_CLASS    0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_CLASS        1
    #else
        #define RE_STD_HAS_IS_CLASS        0
    #endif
#endif  // RE_STD_HAS_IS_CLASS


namespace re_std
{


// =============================================================================
// I.   IS_CLASS
// =============================================================================

#if RE_STD_HAS_IS_CLASS

    // is_class
    //   trait: true if Type is a class or struct (not union).
    template<typename Type>
    struct is_class : integral_constant<bool, __is_class(Type)>
    {};

#else

    // is_class
    //   trait: degraded fallback (always false) when intrinsic is absent.
    template<typename Type>
    struct is_class : false_type
    {};

#endif  // RE_STD_HAS_IS_CLASS


// =============================================================================
// II.  IS_CLASS_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_class_v
    //   variable: convenience for is_class<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_class_v = is_class<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_CLASS_HPP
