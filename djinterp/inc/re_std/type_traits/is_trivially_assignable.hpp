/*******************************************************************************
* djinterp [re_std]                                  is_trivially_assignable.hpp
*
* is_trivially_assignable trait header:
*   Yields true_type if `To = From` is well-formed and the assignment
* is trivial. Intrinsic-backed; degrades to false_type otherwise.
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE.
*
*
* path:      /inc/re_std/type_traits/is_trivially_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_TRIVIALLY_ASSIGNABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_TRIVIALLY_ASSIGNABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./false_type.hpp"


#ifndef RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_assignable)
            #define RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE     1
        #else
            #define RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE     0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE         1
    #else
        #define RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE         0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_TRIVIALLY_ASSIGNABLE
// =============================================================================

#if RE_STD_HAS_IS_TRIVIALLY_ASSIGNABLE

    template<typename To,
             typename From>
    struct is_trivially_assignable
        : integral_constant<bool, __is_trivially_assignable(To, From)>
    {};

#else

    template<typename To,
             typename From>
    struct is_trivially_assignable : false_type
    {};

#endif


// =============================================================================
// II.  IS_TRIVIALLY_ASSIGNABLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename To,
             typename From>
    RE_STD_CONSTEXPR bool is_trivially_assignable_v =
        is_trivially_assignable<To, From>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_TRIVIALLY_ASSIGNABLE_HPP
