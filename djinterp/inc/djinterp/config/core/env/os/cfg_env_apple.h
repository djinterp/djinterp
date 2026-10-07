/*******************************************************************************
* djinterp [config]                                              cfg_env_apple.h
*
* Configuration for env_apple.h, the Apple detection header under env/os/.
*   Resolves D_CFG_ENV_APPLE_UNIVERSAL, the build's word that it compiles one
* slice of a Universal Binary. Each slice compiles for one architecture, and
* lipo joins them only afterwards, so no compiler macro can say so.
*   targets:  env/os/env_apple.h -> D_CFG_ENV_APPLE_UNIVERSAL
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/os/cfg_env_apple.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_OS_CFG_ENV_APPLE_H
#define DJINTERP_CONFIG_CORE_ENV_OS_CFG_ENV_APPLE_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_APPLE_UNIVERSAL
//   configuration: 1 when the build compiles one slice of a Universal Binary,
// which env_apple.h reports as D_ENV_APPLE_IS_UNIVERSAL; 0, the default,
// otherwise. It was D_CFG_APPLE_UNIVERSAL, which env_apple.h tested with
// defined(), so even a 0 turned it on; the name now follows D_CFG_ENV_* and
// the value is read as 0 or 1, like every other switch (decision 9).
#ifndef D_CFG_ENV_APPLE_UNIVERSAL
    #define D_CFG_ENV_APPLE_UNIVERSAL 0
#endif  // D_CFG_ENV_APPLE_UNIVERSAL

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_APPLE_UNIVERSAL)
    #error "D_CFG_ENV_APPLE_UNIVERSAL must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_OS_CFG_ENV_APPLE_H
