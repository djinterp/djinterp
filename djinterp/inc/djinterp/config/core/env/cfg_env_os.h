/*******************************************************************************
* djinterp [config]                                                 cfg_env_os.h
*
* Configuration for env_os.h, the operating-system detection section of env.h.
*   Resolves D_CFG_ENV_OS_ENABLED, which chooses between that section's
* automatic detection and the D_ENV_DETECTED_OS_* result a build pre-defines,
* from D_CFG_ENV_CUSTOM.
*   targets:  env/env_os.h -> D_CFG_ENV_OS_ENABLED
*   requires: cfg_common.h; cfg_env.h (D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_OS)
*
*
* path:      /inc/djinterp/config/core/env/cfg_env_os.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_OS_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_OS_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset
#include "./cfg_env.h"         // D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_OS


// D_CFG_ENV_OS_ENABLED
//   configuration: 1 runs env_os.h's automatic detection; 0 skips it, and the
// operating system is the one the D_ENV_DETECTED_OS_* macros select. Defaults
// to 0 when D_CFG_ENV_CUSTOM sets D_CFG_ENV_BIT_OS, or when D_CFG_ENV_CUSTOM is
// nonzero and the build pre-defines any D_ENV_DETECTED_OS_* macro; to 1
// otherwise.
#ifndef D_CFG_ENV_OS_ENABLED
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_OS)
        // the section's bit is set
        #define D_CFG_ENV_OS_ENABLED 0
    #elif ( (D_CFG_ENV_CUSTOM > 0) &&                                          \
            ( (defined(D_ENV_DETECTED_OS_APPLE))   ||                          \
              (defined(D_ENV_DETECTED_OS_MACOS))   ||                          \
              (defined(D_ENV_DETECTED_OS_IOS))     ||                          \
              (defined(D_ENV_DETECTED_OS_LINUX))   ||                          \
              (defined(D_ENV_DETECTED_OS_ANDROID)) ||                          \
              (defined(D_ENV_DETECTED_OS_WINDOWS)) ||                          \
              (defined(D_ENV_DETECTED_OS_BSD))     ||                          \
              (defined(D_ENV_DETECTED_OS_SOLARIS)) ||                          \
              (defined(D_ENV_DETECTED_OS_UNIX))    ||                          \
              (defined(D_ENV_DETECTED_OS_MSDOS))   ||                          \
              (defined(D_ENV_DETECTED_OS_UNKNOWN)) ) )
        // a pre-defined result switches the section off
        #define D_CFG_ENV_OS_ENABLED 0
    #else
        #define D_CFG_ENV_OS_ENABLED 1
    #endif
#endif  // D_CFG_ENV_OS_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_OS_ENABLED)
    #error "D_CFG_ENV_OS_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_OS_H
