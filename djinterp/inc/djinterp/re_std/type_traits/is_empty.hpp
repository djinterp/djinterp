/******************************************************************************
* djinterp [restd]                                                  is_empty.hpp
*
* is_empty trait header:
*   is_empty<T>::value is true iff T is a class type with no non-static data
* members, no virtual functions, no virtual bases, and no non-empty bases.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/restd/type_traits/is_empty.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_EMPTY_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_EMPTY_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RESTD_HAS_IS_EMPTY  (intrinsic detection)
// =============================================================================

#ifndef D_RESTD_HAS_IS_EMPTY
    #if defined(__has_builtin)
        #if __has_builtin(__is_empty)
            #define D_RESTD_HAS_IS_EMPTY  1
        #else
            #define D_RESTD_HAS_IS_EMPTY  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_IS_EMPTY      1
    #else
        #define D_RESTD_HAS_IS_EMPTY      0
    #endif
#endif  // D_RESTD_HAS_IS_EMPTY


NS_RESTD


// =============================================================================
// I.   IS_EMPTY
// =============================================================================

#if D_RESTD_HAS_IS_EMPTY

// is_empty
//   trait: intrinsic-backed -- a class type occupying no storage of its own.
template<typename _Type>
struct is_empty : integral_constant<bool, __is_empty(_Type)>
{};

#else

// is_empty
//   trait: degraded fallback (always false) when the intrinsic is absent;
// this property is not observable at the library level.
template<typename _Type>
struct is_empty : false_type
{};

#endif  // D_RESTD_HAS_IS_EMPTY


// =============================================================================
// II.  IS_EMPTY_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_empty_v = is_empty<_Type>::value;

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_EMPTY_
