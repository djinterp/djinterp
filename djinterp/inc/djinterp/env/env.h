/******************************************************************************
* djinterp [env]                                                         env.h
*
* djinterp environmental detection header (umbrella):
*   This header provides comprehensive compile-time detection of the
* compilation environment. It is now an umbrella that includes the
* per-concern detection sub-headers in dependency order:
*   - language standards (C95-C23, C++98-C++23)        -> env_lang.h
*   - POSIX / XSI standards and features               -> env_posix.h
*   - CPU architectures, bit width, endianness         -> env_arch.h
*   - operating systems (block/flag classification)    -> env_os.h
*   - compilers + preprocessor limits + __VA_OPT__     -> env_compiler.h
*   - C runtime / standard-library features            -> c/env_c_lib.h
*   - build configuration (Debug/Release)              -> env_build.h
*
*   The header creates a unified D_ENV_* macro interface enabling portable
* code that adapts to different platforms, compilers, and architectures. All
* detection is performed at compile-time with zero runtime overhead.
*
*   INCLUDE ORDER (important):
*   The sub-headers are included below in an order that satisfies their
* cross-dependencies. In particular:
*     - env_compiler.h is included AFTER env_os.h, because the preprocessor-
*       limits block (folded into env_compiler.h) consults D_ENV_PLATFORM_*
*       which is established by env_os.h. (In the previous monolithic env.h
*       this was a latent forward-reference; the umbrella ordering fixes it.)
*     - c/env_c_lib.h is included last (before build), since it depends on the
*       language, architecture, OS, compiler, and D_ENV_IS_OS_POSIX_LIKE*
*       macros established by every preceding section.
*
*   CONFIGURATION SYSTEM:
*   This header supports custom environment simulation via D_CFG_ENV_CUSTOM.
* See cfg_env.h. Pre-defining D_ENV_DETECTED_* variables automatically
* sets corresponding section bits to enable testing different environments.
*
*
* path:      /inc/djinterp/env/env.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.03.27
*                                                          revised: 2026.09.12
******************************************************************************/

#ifndef DJINTERP_ENV_
#define DJINTERP_ENV_ 1


// ===========================================================================
// I.   CONFIGURATION SYSTEM
// ===========================================================================
//   All D_CFG_ENV_* macros (master custom flag, bitfield positions,
// section-enable helpers) are defined in the sibling config file below.

// djinterp
#include "../config/core/env/cfg_env.h"
#include "./c/env_c_lib.h"
#include "./env_lang.h"
#include "./env_posix.h"
#include "./env_arch.h"
#include "./env_os.h"
#include "./env_compiler.h"
#include "./env_build.h"

// ===========================================================================
// IX.  DEBUG UTILITIES
// ===========================================================================

#ifdef D_DEBUG_
    // std
    #include <stdio.h>

    void print_compiler_info(void);

#endif  // D_DEBUG_


#endif  // DJINTERP_ENV_