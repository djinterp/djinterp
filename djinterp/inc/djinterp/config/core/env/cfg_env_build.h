/*******************************************************************************
* djinterp [config]                                              cfg_env_build.h
*
* Configuration for env_build.h, the build-configuration detection section of
* env.h.
*   Resolves D_CFG_ENV_BUILD_ENABLED, which chooses between that section's
* automatic detection and the D_ENV_DETECTED_BUILD_* result a build pre-defines,
* from D_CFG_ENV_CUSTOM.
*   targets:  env/env_build.h -> D_CFG_ENV_BUILD_ENABLED
*   requires: cfg_common.h; cfg_env.h (D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_BUILD)
*
*
* path:      /inc/djinterp/config/core/env/cfg_env_build.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_BUILD_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_BUILD_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset
#include "./cfg_env.h"         // D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_BUILD


// D_CFG_ENV_BUILD_ENABLED
//   configuration: 1 runs env_build.h's automatic detection; 0 skips it, and
// the build type is the one the D_ENV_DETECTED_BUILD_* macros select. Defaults
// to 0 when D_CFG_ENV_CUSTOM sets D_CFG_ENV_BIT_BUILD, or when D_CFG_ENV_CUSTOM
// is nonzero and the build pre-defines any D_ENV_DETECTED_BUILD_* macro; to 1
// otherwise.
#ifndef D_CFG_ENV_BUILD_ENABLED
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_BUILD)
        // the section's bit is set
        #define D_CFG_ENV_BUILD_ENABLED 0
    #elif ( (D_CFG_ENV_CUSTOM > 0) &&                                          \
            ( (defined(D_ENV_DETECTED_BUILD_DEBUG))   ||                       \
              (defined(D_ENV_DETECTED_BUILD_RELEASE)) ) )
        // a pre-defined result switches the section off
        #define D_CFG_ENV_BUILD_ENABLED 0
    #else
        #define D_CFG_ENV_BUILD_ENABLED 1
    #endif
#endif  // D_CFG_ENV_BUILD_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_BUILD_ENABLED)
    #error "D_CFG_ENV_BUILD_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_BUILD_H
