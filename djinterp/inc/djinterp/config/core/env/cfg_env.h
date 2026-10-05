/*******************************************************************************
* djinterp [config]                                                    cfg_env.h
*
* Shared configuration for env's detection sections.
*   Owns D_CFG_ENV_CUSTOM, which switches detection sections off one by one so
* that each reports what the build pre-defines instead of what it detects; the
* bit that names each section; and D_CFG_ENV_ISO_STRICT, which limits what
* detection reports to what the ISO standard guarantees. Each section resolves
* its own switch, D_CFG_ENV_<SECTION>_ENABLED, in cfg_env_<section>.h, which
* includes this file. Database-related D_CFG_ENV_USING_*, *_C_PATH, and
* *_CPP_PATH macros live in their own per-vendor config files under ./db/.
*   targets:  cfg_env_lang.h, cfg_env_posix.h, cfg_env_arch.h, cfg_env_os.h,
*             cfg_env_compiler.h and cfg_env_build.h -> D_CFG_ENV_CUSTOM,
*             D_CFG_ENV_BIT_*; env/c/env_long_long.h and env/c/env_c_lib.h ->
*             D_CFG_ENV_ISO_STRICT
*   requires: cfg_common.h
*   Includes cfg_common.h first, like every config header, so a cfg_custom.h
* reaches detection whichever djinterp header a translation unit includes first.
*
*
* path:      /inc/djinterp/config/core/env/cfg_env.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.09
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DETECTION SECTIONS
    ------------------
    1.  Section bits
         1.  D_CFG_ENV_BIT_LANG
         2.  D_CFG_ENV_BIT_POSIX
         3.  D_CFG_ENV_BIT_COMPILER
         4.  D_CFG_ENV_BIT_OS
         5.  D_CFG_ENV_BIT_ARCH
         6.  D_CFG_ENV_BIT_BUILD
    2.  Switching sections off
         1.  D_CFG_ENV_CUSTOM
2.  ISO STRICTNESS
    --------------
    1.  Language-level guarantees
         1.  D_CFG_ENV_ISO_STRICT
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_CFG_ENV_H
#define DJINTERP_CONFIG_CORE_ENV_CFG_ENV_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_BOOL, user overrides, testing preset


//==============================================================================
// 1.  DETECTION SECTIONS
//==============================================================================
//   Each env detection section can be switched off, so that it reports the
// result a build pre-defines through D_ENV_DETECTED_* macros instead of what it
// detects, to test code against an environment other than the host.
// D_CFG_ENV_CUSTOM holds one bit per section. Each section's configuration,
// cfg_env_<section>.h, resolves its bit into the switch the section's header
// reads, D_CFG_ENV_<SECTION>_ENABLED.


// 1.1    Section bits
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_BIT_LANG
//   constant: bit 0 of D_CFG_ENV_CUSTOM, language-standard detection
// (env_lang.h).
#define D_CFG_ENV_BIT_LANG     0x01

// 1.1.2
// D_CFG_ENV_BIT_POSIX
//   constant: bit 1 of D_CFG_ENV_CUSTOM, POSIX-standard detection
// (env_posix.h).
#define D_CFG_ENV_BIT_POSIX    0x02

// 1.1.3
// D_CFG_ENV_BIT_COMPILER
//   constant: bit 2 of D_CFG_ENV_CUSTOM, compiler detection (env_compiler.h).
#define D_CFG_ENV_BIT_COMPILER 0x04

// 1.1.4
// D_CFG_ENV_BIT_OS
//   constant: bit 3 of D_CFG_ENV_CUSTOM, OS detection (env_os.h).
#define D_CFG_ENV_BIT_OS       0x08

// 1.1.5
// D_CFG_ENV_BIT_ARCH
//   constant: bit 4 of D_CFG_ENV_CUSTOM, architecture detection (env_arch.h).
#define D_CFG_ENV_BIT_ARCH     0x10

// 1.1.6
// D_CFG_ENV_BIT_BUILD
//   constant: bit 5 of D_CFG_ENV_CUSTOM, build-configuration detection
// (env_build.h).
#define D_CFG_ENV_BIT_BUILD    0x20

// 1.2    Switching sections off
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_ENV_CUSTOM
//   configuration: the detection sections to switch off. 0 (default) runs
// every section; any other value is a bitfield of D_CFG_ENV_BIT_* values, and
// each section whose bit is set skips its automatic detection and reports what
// its D_ENV_DETECTED_* macros select. While it is nonzero, pre-defining any of
// a section's D_ENV_DETECTED_* macros also switches that section off, whether
// or not its bit is set.
#ifndef D_CFG_ENV_CUSTOM
    #define D_CFG_ENV_CUSTOM 0
#endif  // D_CFG_ENV_CUSTOM


//==============================================================================
// 2.  ISO STRICTNESS
//==============================================================================
//   By default env reports what the compiler actually supports at the
// language level in use, extensions included: that is the most capable
// build. This knob asks for the ISO standard instead. It changes nothing at
// C99 or C++11 and above, where everything it covers is standard; below
// them it makes env report as absent:
//     - `long long` and `unsigned long long`      (D_ENV_HAS_LONG_LONG)
//     - <stdint.h> and <inttypes.h>               (D_ENV_C_HAS_STDINT_H,
//                                                   D_ENV_C_HAS_INTTYPES_H)
// Each of those detections can still be pre-defined individually, which
// wins over this knob.


// 2.1    Language-level guarantees
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_ISO_STRICT
//   configuration: 1 limits env's findings to what the ISO standard of the
// detected language level guarantees, for builds that must pass
// -pedantic-errors at that level; 0 (default) reports the compiler's
// extensions as available. Set it with -DD_CFG_ENV_ISO_STRICT=1.
#ifndef D_CFG_ENV_ISO_STRICT
    #define D_CFG_ENV_ISO_STRICT 0
#endif  // D_CFG_ENV_ISO_STRICT

#if !D_CFG_IS_BOOL(D_CFG_ENV_ISO_STRICT)
    #error "D_CFG_ENV_ISO_STRICT must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_CFG_ENV_H
