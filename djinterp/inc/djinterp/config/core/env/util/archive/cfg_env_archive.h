/*******************************************************************************
* djinterp [config]                                            cfg_env_archive.h
*
* Configuration for env_archive.h, the archive-library detection header under
* env/util/archive/.
*   Resolves its master switch (decision 105 of the register, as decision 60
* did for TLS).
*   targets:  env/util/archive/env_archive.h -> D_CFG_ENV_ARCHIVE_ENABLED
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/util/archive/cfg_env_archive.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_CONFIG_CORE_ENV_UTIL_ARCHIVE_CFG_ENV_ARCHIVE_H
#define DJINTERP_CONFIG_CORE_ENV_UTIL_ARCHIVE_CFG_ENV_ARCHIVE_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_ARCHIVE_ENABLED
//   configuration: master toggle for archive-library detection. When 0, every
// D_ENV_ARCHIVE_HAVE_* flag resolves to 0 unless explicitly pre-defined.
#ifndef D_CFG_ENV_ARCHIVE_ENABLED
    #define D_CFG_ENV_ARCHIVE_ENABLED 1
#endif  // D_CFG_ENV_ARCHIVE_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_ARCHIVE_ENABLED)
    #error "D_CFG_ENV_ARCHIVE_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_UTIL_ARCHIVE_CFG_ENV_ARCHIVE_H
