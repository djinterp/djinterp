/******************************************************************************
* djinterp [re_std]                                                 is_final.hpp
*
* is_final trait header:
*   is_final<T>::value is true iff T is a class type marked `final`.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_final.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_FINAL_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_FINAL_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_FINAL  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_FINAL
    #if defined(__has_builtin)
        #if __has_builtin(__is_final)
            #define D_RE_STD_HAS_IS_FINAL  1
        #else
            #define D_RE_STD_HAS_IS_FINAL  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_FINAL      1
    #else
        #define D_RE_STD_HAS_IS_FINAL      0
    #endif
#endif  // D_RE_STD_HAS_IS_FINAL


NS_RESTD


// =============================================================================
// I.   IS_FINAL
// =============================================================================

#if D_RE_STD_HAS_IS_FINAL

// is_final
//   trait: intrinsic-backed -- a class marked final.
template<typename _Type>
struct is_final : integral_constant<bool, __is_final(_Type)>
{};

#else

// is_final
//   trait: degraded fallback (always false) when the intrinsic is absent;
// this property is not observable at the library level.
template<typename _Type>
struct is_final : false_type
{};

#endif  // D_RE_STD_HAS_IS_FINAL


// =============================================================================
// II.  IS_FINAL_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_final_v = is_final<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_FINAL_
