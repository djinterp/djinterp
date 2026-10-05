/*******************************************************************************
* djinterp [re_std]                                             alignment_of.hpp
*
* alignment_of trait header:
*   Yields the alignment requirement of Type as a `std::size_t` value.
* On C++11+ this is `alignof(Type)`; on C++98/03 it falls back to
* compiler-specific intrinsics (`__alignof__` for GCC/Clang/Intel,
* `__alignof` for MSVC).
*
*     alignment_of<char>::value         -> 1
*     alignment_of<int>::value          -> typically 4
*     alignment_of<double>::value       -> typically 8
*     alignment_of<int[3]>::value       -> alignment_of element type
*
*   PORTABILITY:
*   The detection macro RE_STD_HAS_ALIGNOF reflects whether a usable
* alignof or compiler intrinsic is available. When neither is present,
* the trait is omitted; consumer code must gate on that macro.
*
*
* path:      /inc/re_std/type_traits/alignment_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ALIGNMENT_OF_HPP
#define RE_STD_TYPE_TRAITS_ALIGNMENT_OF_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"


// =============================================================================
// 0.   RE_STD_HAS_ALIGNOF / RESOLUTION
// =============================================================================

#ifndef RE_STD_HAS_ALIGNOF
    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        #define RE_STD_HAS_ALIGNOF             1
        #define RE_STD_ALIGNOF_(T)             alignof(T)
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_ALIGNOF             1
        #define RE_STD_ALIGNOF_(T)             __alignof__(T)
    #elif defined(RE_STD_COMPILER_MSVC)
        #define RE_STD_HAS_ALIGNOF             1
        #define RE_STD_ALIGNOF_(T)             __alignof(T)
    #else
        #define RE_STD_HAS_ALIGNOF             0
    #endif
#endif  // RE_STD_HAS_ALIGNOF


#if RE_STD_HAS_ALIGNOF


namespace re_std
{


// =============================================================================
// I.   ALIGNMENT_OF
// =============================================================================

// alignment_of
//   trait: yields the alignment requirement of Type as ::value.
template<typename Type>
struct alignment_of
    : integral_constant<std::size_t, RE_STD_ALIGNOF_(Type)>
{};


// =============================================================================
// II.  ALIGNMENT_OF_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // alignment_of_v
    //   variable: convenience for alignment_of<Type>::value.
    template<typename Type>
    RE_STD_CONSTEXPR std::size_t alignment_of_v = alignment_of<Type>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_HAS_ALIGNOF


#endif  // RE_STD_TYPE_TRAITS_ALIGNMENT_OF_HPP
