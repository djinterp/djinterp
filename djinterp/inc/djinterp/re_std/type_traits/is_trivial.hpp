/******************************************************************************
* djinterp [re_std]                                           is_trivial.hpp
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
* path:      /inc/djinterp/re_std/type_traits/is_trivial.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIAL_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIAL_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_TRIVIAL  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_TRIVIAL
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivial)
            #define D_RE_STD_HAS_IS_TRIVIAL  1
        #else
            #define D_RE_STD_HAS_IS_TRIVIAL  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_TRIVIAL      1
    #else
        #define D_RE_STD_HAS_IS_TRIVIAL      0
    #endif
#endif  // D_RE_STD_HAS_IS_TRIVIAL


NS_RESTD


// =============================================================================
// I.   IS_TRIVIAL
// =============================================================================

#if D_RE_STD_HAS_IS_TRIVIAL

// is_trivial
//   trait: intrinsic-backed -- trivially default constructible and trivially copyable.
template<typename _Type>
struct is_trivial : integral_constant<bool, __is_trivial(_Type)>
{};

#else

// is_trivial
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is the conservative answer: it sends callers down the general
// path rather than the optimised one, which is correct but slower. The
// reverse error would be silent and wrong.
template<typename _Type>
struct is_trivial : false_type
{};

#endif  // D_RE_STD_HAS_IS_TRIVIAL


// =============================================================================
// II.  IS_TRIVIAL_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_trivial_v = is_trivial<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIAL_
