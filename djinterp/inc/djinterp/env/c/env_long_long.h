/*******************************************************************************
* djinterp [env]                                                 env_long_long.h
*
* djinterp `long long` feature detection.
*   Detects whether the `long long` and `unsigned long long` integral types are
* available. They are standard from C99 and C++11, and a widespread compiler
* extension under earlier standards, which this header reports as available
* unless the build asks for ISO strictness (D_CFG_ENV_ISO_STRICT, cfg_env.h).
*   It is a component of env_lang.h, which includes it at its end, once the
* language standard is detected: it reads D_ENV_LANG_IS_C99_OR_HIGHER and
* D_ENV_LANG_IS_CPP11_OR_HIGHER, and the compilers' own predefined macros. It
* includes env_lang.h and cfg_env.h itself, so it gives the same answer whether
* a unit includes it directly or through env.h: included first, it reaches
* env_lang.h through its own include, and env_lang.h's include of this file is
* then a no-op under the guard.
*
*
* path:      /inc/djinterp/env/c/env_long_long.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_ENV_C_ENV_LONG_LONG_H
#define DJINTERP_ENV_C_ENV_LONG_LONG_H 1

// djinterp
#include "../../config/core/env/cfg_env.h"  // D_CFG_ENV_ISO_STRICT
#include "../env_lang.h"                    // D_ENV_LANG_IS_*_OR_HIGHER


// D_ENV_HAS_LONG_LONG
//   feature: 1 if `long long` and `unsigned long long` are available as
// built-in integral types, 0 otherwise.
#ifndef D_ENV_HAS_LONG_LONG
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_ENV_HAS_LONG_LONG 1
    #elif D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_HAS_LONG_LONG 1
    #elif D_CFG_IS_ON(D_CFG_ENV_ISO_STRICT)
        // below C99 and C++11 the type is not ISO, and the build asked for ISO
        #define D_ENV_HAS_LONG_LONG 0
    #elif ( (defined(__GNUC__))         ||                                     \
            (defined(__clang__))        ||                                     \
            (defined(_MSC_VER))         ||                                     \
            (defined(__INTEL_COMPILER)) ||                                     \
            (defined(__IBMCPP__))       ||                                     \
            (defined(__SUNPRO_CC)) )
        // before C99 and C++11: a common compiler extension
        #define D_ENV_HAS_LONG_LONG 1
    #else
        #define D_ENV_HAS_LONG_LONG 0
    #endif
#endif  // D_ENV_HAS_LONG_LONG


#endif  // DJINTERP_ENV_C_ENV_LONG_LONG_H
