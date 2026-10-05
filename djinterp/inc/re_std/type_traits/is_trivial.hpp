/*******************************************************************************
* djinterp [re_std]                                               is_trivial.hpp
*
* is_trivial trait header:
*   is_trivial<T>::value is true iff T is trivially default constructible
* AND trivially copyable -- the two properties together, which is why the
* standard deprecated it in C++26: almost every caller wanted only one of
* them and got a stricter test than they meant. Prefer
* is_trivially_copyable or is_trivially_default_constructible directly.
*
*   NO LIBRARY-LEVEL IMPLEMENTATION EXISTS. Triviality depends on facts
* about a class's special member functions that no amount of template
* metaprogramming can observe, so this trait is intrinsic-or-nothing.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_trivial.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_TRIVIAL_HPP
#define RE_STD_TYPE_TRAITS_IS_TRIVIAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_TRIVIAL  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_TRIVIAL
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivial)
            #define RE_STD_HAS_IS_TRIVIAL  1
        #else
            #define RE_STD_HAS_IS_TRIVIAL  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_TRIVIAL      1
    #else
        #define RE_STD_HAS_IS_TRIVIAL      0
    #endif
#endif  // RE_STD_HAS_IS_TRIVIAL


namespace re_std
{


// =============================================================================
// I.   IS_TRIVIAL
// =============================================================================

#if RE_STD_HAS_IS_TRIVIAL

// is_trivial
//   trait: intrinsic-backed -- trivially default constructible and trivially copyable.
template<typename Type>
struct is_trivial : integral_constant<bool, __is_trivial(Type)>
{};

#else

// is_trivial
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is the conservative answer: it sends callers down the general
// path rather than the optimised one, which is correct but slower. The
// reverse error would be silent and wrong.
template<typename Type>
struct is_trivial : false_type
{};

#endif  // RE_STD_HAS_IS_TRIVIAL


// =============================================================================
// II.  IS_TRIVIAL_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_trivial_v = is_trivial<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_TRIVIAL_HPP
