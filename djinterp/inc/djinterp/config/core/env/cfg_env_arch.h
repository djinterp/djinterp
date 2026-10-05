/*******************************************************************************
* djinterp [config]                                               cfg_env_arch.h
*
* Configuration for env_arch.h, the CPU architecture detection section of env.h.
*   Resolves D_CFG_ENV_ARCH_ENABLED, which chooses between that section's
* automatic detection and the D_ENV_DETECTED_ARCH_* result a build pre-defines,
* from D_CFG_ENV_CUSTOM.
*   targets:  env/env_arch.h -> D_CFG_ENV_ARCH_ENABLED
*   requires: cfg_common.h; cfg_env.h (D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_ARCH)
*
*
* path:      /inc/djinterp/config/core/env/cfg_env_arch.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_ARCH_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_ARCH_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset
#include "./cfg_env.h"         // D_CFG_ENV_CUSTOM, D_CFG_ENV_BIT_ARCH


// D_CFG_ENV_ARCH_ENABLED
//   configuration: 1 runs env_arch.h's automatic detection; 0 skips it, and the
// architecture is the one the D_ENV_DETECTED_ARCH_* macros select. Defaults to
// 0 when D_CFG_ENV_CUSTOM sets D_CFG_ENV_BIT_ARCH, or when D_CFG_ENV_CUSTOM is
// nonzero and the build pre-defines any D_ENV_DETECTED_ARCH_* macro; to 1
// otherwise.
#ifndef D_CFG_ENV_ARCH_ENABLED
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_ARCH)
        // the section's bit is set
        #define D_CFG_ENV_ARCH_ENABLED 0
    #elif ( (D_CFG_ENV_CUSTOM > 0) &&                                          \
            ( (defined(D_ENV_DETECTED_ARCH_X86))     ||                        \
              (defined(D_ENV_DETECTED_ARCH_X64))     ||                        \
              (defined(D_ENV_DETECTED_ARCH_ARM))     ||                        \
              (defined(D_ENV_DETECTED_ARCH_ARM64))   ||                        \
              (defined(D_ENV_DETECTED_ARCH_RISCV))   ||                        \
              (defined(D_ENV_DETECTED_ARCH_POWERPC)) ||                        \
              (defined(D_ENV_DETECTED_ARCH_MIPS))    ||                        \
              (defined(D_ENV_DETECTED_ARCH_SPARC))   ||                        \
              (defined(D_ENV_DETECTED_ARCH_S390))    ||                        \
              (defined(D_ENV_DETECTED_ARCH_IA64))    ||                        \
              (defined(D_ENV_DETECTED_ARCH_ALPHA))   ||                        \
              (defined(D_ENV_DETECTED_ARCH_UNKNOWN)) ) )
        // a pre-defined result switches the section off
        #define D_CFG_ENV_ARCH_ENABLED 0
    #else
        #define D_CFG_ENV_ARCH_ENABLED 1
    #endif
#endif  // D_CFG_ENV_ARCH_ENABLED

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_ARCH_ENABLED)
    #error "D_CFG_ENV_ARCH_ENABLED must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_ARCH_H
