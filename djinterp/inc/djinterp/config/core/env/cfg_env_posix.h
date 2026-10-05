/*******************************************************************************
* djinterp [config]                                              cfg_env_posix.h
*
* Configuration for env_posix.h, the POSIX / XSI detection section of env.h.
*   Resolves D_CFG_ENV_POSIX_ENABLED, which chooses between that section's
* automatic detection and the D_ENV_DETECTED_POSIX_* result a build pre-defines,
* from D_CFG_ENV_CUSTOM.
*   targets:  env/env_posix.h -> D_CFG_ENV_POSIX_ENABLED
*   requires: cfg_common.h; cfg_env.h (D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_POSIX)
*
*
* path:      /inc/djinterp/config/core/env/cfg_env_posix.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_POSIX_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_POSIX_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset
#include "./cfg_env.h"         // D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_POSIX


// D_CFG_ENV_POSIX_ENABLED
//   configuration: 1 runs env_posix.h's automatic detection; 0 skips it, and
// the POSIX level is the one the D_ENV_DETECTED_POSIX_* macros select. Defaults
// to 0 when D_CFG_ENV_CUSTOM sets D_CFG_ENV_BIT_POSIX, or when D_CFG_ENV_CUSTOM
// is nonzero and the build pre-defines any D_ENV_DETECTED_POSIX_* macro; to 1
// otherwise.
#ifndef D_CFG_ENV_POSIX_ENABLED
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_POSIX)
        // the section's bit is set
        #define D_CFG_ENV_POSIX_ENABLED 0
    #elif ( (D_CFG_ENV_CUSTOM > 0) &&                                          \
            ( (defined(D_ENV_DETECTED_POSIX_1988))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_1990))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_1993))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_1996))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_2001))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_2008))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_2017))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_2024))     ||                      \
              (defined(D_ENV_DETECTED_POSIX_XSI))      ||                      \
              (defined(D_ENV_DETECTED_POSIX_THREADS))  ||                      \
              (defined(D_ENV_DETECTED_POSIX_REALTIME)) ||                      \
              (defined(D_ENV_DETECTED_POSIX_SOCKETS))  ||                      \
              (defined(D_ENV_DETECTED_POSIX_NONE)) ) )
        // a pre-defined result switches the section off
        #define D_CFG_ENV_POSIX_ENABLED 0
    #else
        #define D_CFG_ENV_POSIX_ENABLED 1
    #endif
#endif  // D_CFG_ENV_POSIX_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_POSIX_ENABLED)
    #error "D_CFG_ENV_POSIX_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_POSIX_H
