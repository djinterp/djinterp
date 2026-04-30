/******************************************************************************
* djinterp [restd]                                            alignment_of.hpp
*
* alignment_of trait header:
*   Yields the alignment requirement of _Type as a `std::size_t` value.
* On C++11+ this is `alignof(_Type)`; on C++98/03 it falls back to
* compiler-specific intrinsics (`__alignof__` for GCC/Clang/Intel,
* `__alignof` for MSVC).
*
*     alignment_of<char>::value         -> 1
*     alignment_of<int>::value          -> typically 4
*     alignment_of<double>::value       -> typically 8
*     alignment_of<int[3]>::value       -> alignment_of element type
*
*   PORTABILITY:
*   The detection macro D_RESTD_HAS_ALIGNOF reflects whether a usable
* alignof or compiler intrinsic is available. When neither is present,
* the trait is omitted; consumer code must gate on that macro.
*
*
* path:      /inc/djinterp/restd/type_traits/alignment_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_ALIGNMENT_OF_
#define DJINTERP_RESTD_TYPE_TRAITS_ALIGNMENT_OF_ 1

// std
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"


// =============================================================================
// 0.   D_RESTD_HAS_ALIGNOF / RESOLUTION
// =============================================================================

#ifndef D_RESTD_HAS_ALIGNOF
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_RESTD_HAS_ALIGNOF             1
        #define D_RESTD_ALIGNOF_(T)             alignof(T)
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_ALIGNOF             1
        #define D_RESTD_ALIGNOF_(T)             __alignof__(T)
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_RESTD_HAS_ALIGNOF             1
        #define D_RESTD_ALIGNOF_(T)             __alignof(T)
    #else
        #define D_RESTD_HAS_ALIGNOF             0
    #endif
#endif  // D_RESTD_HAS_ALIGNOF


#if D_RESTD_HAS_ALIGNOF


NS_RESTD


// =============================================================================
// I.   ALIGNMENT_OF
// =============================================================================

// alignment_of
//   trait: yields the alignment requirement of _Type as ::value.
template<typename _Type>
struct alignment_of
    : integral_constant<std::size_t, D_RESTD_ALIGNOF_(_Type)>
{};


// =============================================================================
// II.  ALIGNMENT_OF_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // alignment_of_v
    //   variable: convenience for alignment_of<_Type>::value.
    template<typename _Type>
    D_CONSTEXPR std::size_t alignment_of_v = alignment_of<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // D_RESTD_HAS_ALIGNOF


#endif  // DJINTERP_RESTD_TYPE_TRAITS_ALIGNMENT_OF_
