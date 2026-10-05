/*******************************************************************************
* djinterp [env]                                                     env_build.h
*
* djinterp build-configuration detection.
*   Compile-time detection of Debug vs. Release builds, exposing the
* D_ENV_BUILD_* interface.
*   It includes its own configuration, cfg_env_build.h, and reads no other env
* section, so it gives the same answers whether a unit includes it directly or
* through env.h.
*
*
* path:      /inc/djinterp/env/env_build.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_ENV_ENV_BUILD_H
#define DJINTERP_ENV_ENV_BUILD_H 1

// djinterp
#include "../config/core/env/cfg_env_build.h"  // D_CFG_ENV_BUILD_ENABLED


// D_ENV_BUILD_DEBUG / D_ENV_BUILD_RELEASE / D_ENV_BUILD_TYPE
//   constant: exactly one of D_ENV_BUILD_DEBUG and D_ENV_BUILD_RELEASE is
// defined, to 1, and D_ENV_BUILD_TYPE names it ("Debug" or "Release"). With
// detection disabled, the D_ENV_DETECTED_BUILD_* overrides decide, and neither
// is defined if neither override is.
#if D_CFG_IS_ON(D_CFG_ENV_BUILD_ENABLED)
    // automatic detection
    // note: !defined(NDEBUG) means builds that define neither DEBUG nor
    // NDEBUG will be classified as Debug. If this is too aggressive for
    // your build system, consider requiring an affirmative debug signal.
    #if ( (defined(DEBUG))  ||                                                 \
          (defined(_DEBUG)) ||                                                 \
          (!defined(NDEBUG)) )
        #define D_ENV_BUILD_DEBUG   1
        #define D_ENV_BUILD_TYPE    "Debug"
    #else
        #define D_ENV_BUILD_RELEASE 1
        #define D_ENV_BUILD_TYPE    "Release"
    #endif
#else
    // manual detection
    #ifdef D_ENV_DETECTED_BUILD_DEBUG
        #define D_ENV_BUILD_DEBUG   1
        #define D_ENV_BUILD_TYPE    "Debug"
    #elif defined(D_ENV_DETECTED_BUILD_RELEASE)
        #define D_ENV_BUILD_RELEASE 1
        #define D_ENV_BUILD_TYPE    "Release"
    #endif  // D_ENV_DETECTED_BUILD_DEBUG
#endif  // D_CFG_ENV_BUILD_ENABLED


#endif  // DJINTERP_ENV_ENV_BUILD_H
