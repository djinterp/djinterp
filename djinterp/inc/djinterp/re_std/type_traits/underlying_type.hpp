/******************************************************************************
* djinterp [restd]                                         underlying_type.hpp
*
* underlying_type trait header:
*   Yields the underlying integral type of an enumeration. Implemented
* via the __underlying_type compiler builtin, supported by GCC 4.7+,
* all Clang, MSVC 2012+ (v11), and Intel 13.0+.
*
*     enum E { a, b };
*     underlying_type<E>::type  -> int (typical default)
*
*     enum class F : unsigned long;
*     underlying_type<F>::type  -> unsigned long
*
*   FALLBACK BEHAVIOR:
*   When the intrinsic is unavailable, underlying_type is NOT defined
* (no fallback typedef). Code that uses it must be gated on
* D_RESTD_HAS_UNDERLYING_TYPE. This is safe in restd::any because the
* only consumer is itself gated on is_enum::value, which is false when
* intrinsics are absent.
*
*   DETECTION MACRO:
*   D_RESTD_HAS_UNDERLYING_TYPE is set to 1 if the intrinsic is
* available, 0 otherwise. Users may pre-define it to override detection.
*
*
* path:      /inc/djinterp/re_std/type_traits/underlying_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_UNDERLYING_TYPE_
#define DJINTERP_RESTD_TYPE_TRAITS_UNDERLYING_TYPE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// =============================================================================
// 0.   D_RESTD_HAS_UNDERLYING_TYPE DETECTION
// =============================================================================

// D_RESTD_HAS_UNDERLYING_TYPE
//   constant: 1 if the __underlying_type compiler intrinsic is
// available, 0 otherwise. Users may pre-define to override.
#ifndef D_RESTD_HAS_UNDERLYING_TYPE
    #if defined(__has_builtin)
        #if __has_builtin(__underlying_type)
            #define D_RESTD_HAS_UNDERLYING_TYPE 1
        #else
            #define D_RESTD_HAS_UNDERLYING_TYPE 0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC) ||                                   \
            defined(D_ENV_COMPILER_CLANG) ||                                 \
            defined(D_ENV_COMPILER_MSVC) ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RESTD_HAS_UNDERLYING_TYPE 1
    #else
        #define D_RESTD_HAS_UNDERLYING_TYPE 0
    #endif
#endif


#if D_RESTD_HAS_UNDERLYING_TYPE


NS_RESTD


// =============================================================================
// I.   UNDERLYING_TYPE
// =============================================================================

// underlying_type
//   trait: yields the underlying integral type of enumeration _Type.
// Uses __underlying_type builtin.
template<typename _Type>
struct underlying_type
{
    typedef __underlying_type(_Type) type;
};


// =============================================================================
// II.  UNDERLYING_TYPE_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // underlying_type_t
    //   alias: convenience alias for underlying_type<_Type>::type.
    template<typename _Type>
    using underlying_type_t = typename underlying_type<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // D_RESTD_HAS_UNDERLYING_TYPE


#endif  // DJINTERP_RESTD_TYPE_TRAITS_UNDERLYING_TYPE_
