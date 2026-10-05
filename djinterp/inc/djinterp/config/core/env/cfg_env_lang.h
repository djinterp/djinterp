/*******************************************************************************
* djinterp [config]                                               cfg_env_lang.h
*
* Configuration for env_lang.h, the language-standard detection section of
* env.h.
*   Resolves D_CFG_ENV_LANG_ENABLED, which chooses between that section's
* automatic detection and the D_ENV_DETECTED_C* / D_ENV_DETECTED_CPP* result a
* build pre-defines, from D_CFG_ENV_CUSTOM.
*   targets:  env/env_lang.h -> D_CFG_ENV_LANG_ENABLED
*   requires: cfg_common.h; cfg_env.h (D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_LANG)
*
*
* path:      /inc/djinterp/config/core/env/cfg_env_lang.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_LANG_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_LANG_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset
#include "./cfg_env.h"         // D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_LANG


// D_CFG_ENV_DETECTED_CPP / D_CFG_ENV_DETECTED_C_ONLY
//   configuration: while D_CFG_ENV_CUSTOM is nonzero, _CPP is defined, to 1,
// when the build pre-defines any D_ENV_DETECTED_CPP* result, and otherwise
// _C_ONLY is, when it pre-defines any D_ENV_DETECTED_C* result.
#if (D_CFG_ENV_CUSTOM > 0)
    #if ( (defined(D_ENV_DETECTED_CPP98)) ||                                   \
          (defined(D_ENV_DETECTED_CPP11)) ||                                   \
          (defined(D_ENV_DETECTED_CPP14)) ||                                   \
          (defined(D_ENV_DETECTED_CPP17)) ||                                   \
          (defined(D_ENV_DETECTED_CPP20)) ||                                   \
          (defined(D_ENV_DETECTED_CPP23)) )
        #define D_CFG_ENV_DETECTED_CPP 1
    #elif ( (defined(D_ENV_DETECTED_C95)) ||                                   \
            (defined(D_ENV_DETECTED_C99)) ||                                   \
            (defined(D_ENV_DETECTED_C11)) ||                                   \
            (defined(D_ENV_DETECTED_C17)) ||                                   \
            (defined(D_ENV_DETECTED_C23)) )
        #define D_CFG_ENV_DETECTED_C_ONLY 1
    #endif
#endif

// D_CFG_ENV_LANG_ENABLED
//   configuration: 1 runs env_lang.h's automatic detection; 0 skips it, and the
// language level is the one the D_ENV_DETECTED_C* / D_ENV_DETECTED_CPP* macros
// select. Defaults to 0 when D_CFG_ENV_CUSTOM sets D_CFG_ENV_BIT_LANG, or when
// either macro above is defined; to 1 otherwise.
#ifndef D_CFG_ENV_LANG_ENABLED
    #if ( (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG) ||                           \
          (defined(D_CFG_ENV_DETECTED_CPP))       ||                           \
          (defined(D_CFG_ENV_DETECTED_C_ONLY)) )
        #define D_CFG_ENV_LANG_ENABLED 0
    #else
        #define D_CFG_ENV_LANG_ENABLED 1
    #endif
#endif  // D_CFG_ENV_LANG_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_LANG_ENABLED)
    #error "D_CFG_ENV_LANG_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_LANG_H
