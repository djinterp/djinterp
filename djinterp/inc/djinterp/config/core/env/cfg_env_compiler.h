/*******************************************************************************
* djinterp [config]                                           cfg_env_compiler.h
*
* Configuration for env_compiler.h, the compiler detection section of env.h.
*   Resolves D_CFG_ENV_COMPILER_ENABLED, which chooses between that section's
* automatic detection and the D_ENV_DETECTED_COMPILER_* result a build
* pre-defines, from D_CFG_ENV_CUSTOM.
*   It also carries D_ENV_CRT_MSVC and D_ENV_MSC_VER, which only a simulated
* environment defines.
*   targets:  env/env_compiler.h -> D_CFG_ENV_COMPILER_ENABLED
*   requires: cfg_common.h; cfg_env.h (D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_COMPILER)
*
*
* path:      /inc/djinterp/config/core/env/cfg_env_compiler.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_COMPILER_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_COMPILER_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset
#include "./cfg_env.h"         // D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_COMPILER


// D_CFG_ENV_COMPILER_ENABLED
//   configuration: 1 runs env_compiler.h's automatic detection; 0 skips it, and
// the compiler is the one the D_ENV_DETECTED_COMPILER_* macros select. Defaults
// to 0 when D_CFG_ENV_CUSTOM sets D_CFG_ENV_BIT_COMPILER, or when
// D_CFG_ENV_CUSTOM is nonzero and the build pre-defines any
// D_ENV_DETECTED_COMPILER_* macro; to 1 otherwise.
#ifndef D_CFG_ENV_COMPILER_ENABLED
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER)
        // the section's bit is set
        #define D_CFG_ENV_COMPILER_ENABLED 0
    #elif ( (D_CFG_ENV_CUSTOM > 0) &&                                          \
            ( (defined(D_ENV_DETECTED_COMPILER_CLANG))       ||                \
              (defined(D_ENV_DETECTED_COMPILER_APPLE_CLANG)) ||                \
              (defined(D_ENV_DETECTED_COMPILER_GCC))         ||                \
              (defined(D_ENV_DETECTED_COMPILER_MSVC))        ||                \
              (defined(D_ENV_DETECTED_COMPILER_INTEL))       ||                \
              (defined(D_ENV_DETECTED_COMPILER_BORLAND))     ||                \
              (defined(D_ENV_DETECTED_COMPILER_UNKNOWN)) ) )
        // a pre-defined result switches the section off
        #define D_CFG_ENV_COMPILER_ENABLED 0
    #else
        #define D_CFG_ENV_COMPILER_ENABLED 1
    #endif
#endif  // D_CFG_ENV_COMPILER_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_COMPILER_ENABLED)
    #error "D_CFG_ENV_COMPILER_ENABLED must be 0 or 1"
#endif

// D_ENV_CRT_MSVC / D_ENV_MSC_VER
//   constant: while D_CFG_ENV_CUSTOM is nonzero, whether the simulated
// compiler is MSVC (D_ENV_DETECTED_COMPILER_MSVC), and then its _MSC_VER;
// 0 and 0 otherwise.
// note: nothing defines them in an ordinary build, where #if reads both as
// 0; env_compiler.h's D_ENV_COMPILER_MSVC_FAMILY is the detected fact.
#if (D_CFG_ENV_CUSTOM > 0)
    #if defined(D_ENV_DETECTED_COMPILER_MSVC)
        #define D_ENV_CRT_MSVC 1
        #define D_ENV_MSC_VER  _MSC_VER
    #else
        #define D_ENV_CRT_MSVC 0
        #define D_ENV_MSC_VER  0
    #endif
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_COMPILER_H
