/*******************************************************************************
* djinterp [re_std]                                is_trivially_destructible.hpp
*
* is_trivially_destructible trait header:
*   Yields true_type if Type is destructible AND its destructor is
* trivial. Intrinsic-backed via `__is_trivially_destructible`
* (Clang) or `__has_trivial_destructor` (GCC/MSVC); degrades to
* false_type otherwise.
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE.
*
*
* path:      /inc/re_std/type_traits/is_trivially_destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./false_type.hpp"


#ifndef RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_destructible)
            #define RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE   1
            #define RE_STD_TRIVIALLY_DESTR_(T) __is_trivially_destructible(T)
        #elif __has_builtin(__has_trivial_destructor)
            #define RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE   1
            #define RE_STD_TRIVIALLY_DESTR_(T) __has_trivial_destructor(T)
        #else
            #define RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE   0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE       1
        #define RE_STD_TRIVIALLY_DESTR_(T)     __has_trivial_destructor(T)
    #else
        #define RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE       0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_TRIVIALLY_DESTRUCTIBLE
// =============================================================================

#if RE_STD_HAS_IS_TRIVIALLY_DESTRUCTIBLE

    template<typename Type>
    struct is_trivially_destructible
        : integral_constant<bool, RE_STD_TRIVIALLY_DESTR_(Type)>
    {};

#else

    template<typename Type>
    struct is_trivially_destructible : false_type
    {};

#endif


// =============================================================================
// II.  IS_TRIVIALLY_DESTRUCTIBLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_trivially_destructible_v =
        is_trivially_destructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_TRIVIALLY_DESTRUCTIBLE_HPP
