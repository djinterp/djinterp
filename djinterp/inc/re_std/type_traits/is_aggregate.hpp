/******************************************************************************
* djinterp [re_std]                                         is_aggregate.hpp
*
* is_aggregate trait header:
*   is_aggregate<T>::value is true iff T is an array type or a class type
* eligible for aggregate initialisation -- no user-declared or inherited
* constructors, no private or protected non-static data members, no
* virtual functions, no virtual or private or protected base classes.
*
*   The rules moved between C++11, C++14, C++17 and C++20, so the
* intrinsic's answer is the compiler's own reading of the tier it is
* compiling for. That is the correct behaviour and a library-side
* reimplementation could not track it.
*
*   BACK-PORT: std added this in C++17; re_std surfaces it from C++11
* wherever the compiler publishes the intrinsic -- a six-year lead.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_aggregate.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_AGGREGATE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_AGGREGATE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_AGGREGATE  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_AGGREGATE
    #if defined(__has_builtin)
        #if __has_builtin(__is_aggregate)
            #define D_RE_STD_HAS_IS_AGGREGATE  1
        #else
            #define D_RE_STD_HAS_IS_AGGREGATE  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_AGGREGATE      1
    #else
        #define D_RE_STD_HAS_IS_AGGREGATE      0
    #endif
#endif  // D_RE_STD_HAS_IS_AGGREGATE


NS_RESTD


// =============================================================================
// I.   IS_AGGREGATE
// =============================================================================

#if D_RE_STD_HAS_IS_AGGREGATE

// is_aggregate
//   trait: intrinsic-backed -- an array or a class with no user-declared constructors.
template<typename _Type>
struct is_aggregate : integral_constant<bool, __is_aggregate(_Type)>
{};

#else

// is_aggregate
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is conservative: callers use a constructor call rather than
// brace initialisation.
template<typename _Type>
struct is_aggregate : false_type
{};

#endif  // D_RE_STD_HAS_IS_AGGREGATE


// =============================================================================
// II.  IS_AGGREGATE_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_aggregate_v = is_aggregate<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_AGGREGATE_
