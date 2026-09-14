/******************************************************************************
* djinterp [core]                                                  env_build.h
*
* djinterp build-configuration detection:
*   Compile-time detection of Debug vs. Release builds, exposing the
* D_ENV_BUILD_* interface.
*
*   Requires:  cfg_env.h (for the D_CFG_ENV_BUILD_IS_ENABLED switch). This
*              header is an internal component of env.h and is #included by it;
*              do NOT #include it directly.
*
*
* path:      /inc/djinterp/env/env_build.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.03.27
******************************************************************************/

#ifndef DJINTERP_ENV_BUILD_
#define DJINTERP_ENV_BUILD_ 1


// build configuration detection logic
#if D_CFG_ENV_BUILD_IS_ENABLED
    // automatic detection
    // NOTE: !defined(NDEBUG) means builds that define neither DEBUG nor
    // NDEBUG will be classified as Debug. If this is too aggressive for
    // your build system, consider requiring an affirmative debug signal.
    #if ( defined(DEBUG)  ||  \
          defined(_DEBUG) ||  \
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
    #endif
#endif  // D_CFG_ENV_BUILD_IS_ENABLED


#endif  // DJINTERP_ENV_BUILD_
