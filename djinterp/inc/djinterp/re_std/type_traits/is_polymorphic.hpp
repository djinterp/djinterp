/******************************************************************************
* djinterp [re_std]                                           is_polymorphic.hpp
*
* is_polymorphic trait header:
*   is_polymorphic<T>::value is true iff T is a class type declaring or
* inheriting at least one virtual function.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_polymorphic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_POLYMORPHIC_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_POLYMORPHIC_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_POLYMORPHIC  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_POLYMORPHIC
    #if defined(__has_builtin)
        #if __has_builtin(__is_polymorphic)
            #define D_RE_STD_HAS_IS_POLYMORPHIC  1
        #else
            #define D_RE_STD_HAS_IS_POLYMORPHIC  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_POLYMORPHIC      1
    #else
        #define D_RE_STD_HAS_IS_POLYMORPHIC      0
    #endif
#endif  // D_RE_STD_HAS_IS_POLYMORPHIC


NS_RESTD


// =============================================================================
// I.   IS_POLYMORPHIC
// =============================================================================

#if D_RE_STD_HAS_IS_POLYMORPHIC

// is_polymorphic
//   trait: intrinsic-backed -- a class with a virtual function.
template<typename _Type>
struct is_polymorphic : integral_constant<bool, __is_polymorphic(_Type)>
{};

#else

// is_polymorphic
//   trait: degraded fallback (always false) when the intrinsic is absent;
// this property is not observable at the library level.
template<typename _Type>
struct is_polymorphic : false_type
{};

#endif  // D_RE_STD_HAS_IS_POLYMORPHIC


// =============================================================================
// II.  IS_POLYMORPHIC_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_polymorphic_v = is_polymorphic<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_POLYMORPHIC_
