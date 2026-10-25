/*******************************************************************************
* djinterp [config]                                           cfg_env_compress.h
*
* Configuration for env_compress.h, the codec-library detection header under
* env/util/compress/.
*   Resolves its master switch (decision 105 of the register, as decision 60
* did for TLS).
*   targets:  env/util/compress/env_compress.h -> D_CFG_ENV_COMPRESSION_ENABLED
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/util/compress/cfg_env_compress.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_CONFIG_CORE_ENV_UTIL_COMPRESS_CFG_ENV_COMPRESS_H
#define DJINTERP_CONFIG_CORE_ENV_UTIL_COMPRESS_CFG_ENV_COMPRESS_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_COMPRESSION_ENABLED
//   configuration: master toggle for codec-library detection. When 0, every
// D_ENV_COMPRESSION_HAVE_* flag resolves to 0 unless explicitly pre-defined.
#ifndef D_CFG_ENV_COMPRESSION_ENABLED
    #define D_CFG_ENV_COMPRESSION_ENABLED 1
#endif  // D_CFG_ENV_COMPRESSION_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_COMPRESSION_ENABLED)
    #error "D_CFG_ENV_COMPRESSION_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_UTIL_COMPRESS_CFG_ENV_COMPRESS_H
