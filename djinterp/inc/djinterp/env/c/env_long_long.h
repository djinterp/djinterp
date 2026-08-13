/******************************************************************************
* djinterp [core]                                              env_long_long.h
*
* djinterp `long long` feature detection:
*   Detects availability of the `long long` / `unsigned long long`
* integral types.  Standard since C++11 and C99, but available as a
* widespread compiler extension under earlier standards.
*
*   Requires:  env.h  (for the D_ENV_LANG_IS_* macros).  This header is an
*              internal component of env.h and is #included by it after the
*              language standard has been detected -- do NOT #include it
*              directly.
*
* 
* path:      /inc/djinterp/core/env/c/env_long_long.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_ENV_LONG_LONG_
#define DJINTERP_ENV_LONG_LONG_ 1


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


#endif  // DJINTERP_ENV_LONG_LONG_
