/******************************************************************************
* djinterp [re_std]                                          is_abstract.hpp
*
* is_abstract trait header:
*   is_abstract<T>::value is true iff T is a class type with at least one
* pure virtual function that no derived override has supplied -- that is,
* iff T cannot be instantiated.
*
*   NO LIBRARY-LEVEL IMPLEMENTATION EXISTS. The classic
* is-it-constructible probe cannot answer this: an abstract class may
* still have a usable constructor signature, and forming the test
* expression is itself ill-formed for an abstract type.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_abstract.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_ABSTRACT_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_ABSTRACT_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_ABSTRACT  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_ABSTRACT
    #if defined(__has_builtin)
        #if __has_builtin(__is_abstract)
            #define D_RE_STD_HAS_IS_ABSTRACT  1
        #else
            #define D_RE_STD_HAS_IS_ABSTRACT  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_ABSTRACT      1
    #else
        #define D_RE_STD_HAS_IS_ABSTRACT      0
    #endif
#endif  // D_RE_STD_HAS_IS_ABSTRACT


NS_RESTD


// =============================================================================
// I.   IS_ABSTRACT
// =============================================================================

#if D_RE_STD_HAS_IS_ABSTRACT

// is_abstract
//   trait: intrinsic-backed -- a class type with at least one unoverridden pure virtual function.
template<typename _Type>
struct is_abstract : integral_constant<bool, __is_abstract(_Type)>
{};

#else

// is_abstract
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is the less useful degradation of the two available -- a caller
// told a type is not abstract may attempt to instantiate it and fail to
// compile. That is still a compile-time failure, not a silent one, and
// the intrinsic is present on every compiler re_std targets.
template<typename _Type>
struct is_abstract : false_type
{};

#endif  // D_RE_STD_HAS_IS_ABSTRACT


// =============================================================================
// II.  IS_ABSTRACT_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_abstract_v = is_abstract<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_ABSTRACT_
