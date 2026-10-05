/*******************************************************************************
* djinterp [re_std]                                                 is_union.hpp
*
* is_union trait header:
*   Yields true_type if Type is a union, false_type otherwise.
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
*   RE_STD_HAS_IS_UNION is defined to 1 when the intrinsic is present,
* 0 otherwise. Predefinable.
*
*
* path:      /inc/re_std/type_traits/is_union.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_UNION_HPP
#define RE_STD_TYPE_TRAITS_IS_UNION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_UNION  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_UNION
    #if defined(__has_builtin)
        #if __has_builtin(__is_union)
            #define RE_STD_HAS_IS_UNION    1
        #else
            #define RE_STD_HAS_IS_UNION    0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_UNION        1
    #else
        #define RE_STD_HAS_IS_UNION        0
    #endif
#endif  // RE_STD_HAS_IS_UNION


namespace re_std
{


// =============================================================================
// I.   IS_UNION
// =============================================================================

#if RE_STD_HAS_IS_UNION

    // is_union
    //   trait: true if Type is a union.
    template<typename Type>
    struct is_union : integral_constant<bool, __is_union(Type)>
    {};

#else

    // is_union
    //   trait: degraded fallback (always false) when intrinsic is absent.
    template<typename Type>
    struct is_union : false_type
    {};

#endif  // RE_STD_HAS_IS_UNION


// =============================================================================
// II.  IS_UNION_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // is_union_v
    //   variable: convenience for is_union<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR bool is_union_v = is_union<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_UNION_HPP
