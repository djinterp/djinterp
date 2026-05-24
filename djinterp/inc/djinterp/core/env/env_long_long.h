/******************************************************************************
* djinterp [core]                                            env_long_long.h
*
* djinterp `long long` feature detection:
*   Detects availability of the `long long` / `unsigned long long`
* integral types.  Standard since C++11 and C99, but available as a
* widespread compiler extension under earlier standards.
*
*
* path:      /inc/env_long_long.h
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_LONG_LONG_DETECTION_
#define DJINTERP_LONG_LONG_DETECTION_ 1

#include "./env.h"


// D_ENV_HAS_LONG_LONG
//   feature: 1 if `long long` and `unsigned long long` are available
// as built-in integral types, 0 otherwise.
#ifndef D_ENV_HAS_LONG_LONG
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_ENV_HAS_LONG_LONG 1
    #elif D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_HAS_LONG_LONG 1
    #elif defined(__GNUC__)         ||  \
          defined(__clang__)        ||  \
          defined(_MSC_VER)         ||  \
          defined(__INTEL_COMPILER) ||  \
          defined(__IBMCPP__)       ||  \
          defined(__SUNPRO_CC)
        #define D_ENV_HAS_LONG_LONG 1
    #else
        #define D_ENV_HAS_LONG_LONG 0
    #endif
#endif


#endif  // DJINTERP_LONG_LONG_DETECTION_
