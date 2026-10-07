/*******************************************************************************
* djinterp [config]                                              cfg_env_linux.h
*
* Configuration for env_linux.h, the Linux detection header under env/os/.
*   Resolves D_CFG_ENV_LINUX_MUSL, the build's word that its C library is
* musl, which by design defines no macro that identifies it.
*   targets:  env/os/env_linux.h -> D_CFG_ENV_LINUX_MUSL
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/os/cfg_env_linux.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_OS_CFG_ENV_LINUX_H
#define DJINTERP_CONFIG_CORE_ENV_OS_CFG_ENV_LINUX_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_LINUX_MUSL
//   configuration: 1 when the build's C library is musl, which env_linux.h
// reports as D_ENV_LINUX_LIBC_MUSL; 0, the default, otherwise. musl has no
// identifying macro, so the build says so; env_linux.h used to read a
// __MUSL__ the build had to define, a reserved name the guides forbid it to
// define (decision 41 of the register).
#ifndef D_CFG_ENV_LINUX_MUSL
    #define D_CFG_ENV_LINUX_MUSL 0
#endif  // D_CFG_ENV_LINUX_MUSL

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_LINUX_MUSL)
    #error "D_CFG_ENV_LINUX_MUSL must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_OS_CFG_ENV_LINUX_H
