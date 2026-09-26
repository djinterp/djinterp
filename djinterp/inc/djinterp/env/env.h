/*******************************************************************************
* djinterp [env]                                                           env.h
*
* djinterp environment detection (umbrella header).
*   Compile-time detection of the compilation environment, exposing a unified
* D_ENV_* interface so code can adapt to platform, compiler, and architecture
* with no runtime cost. This header includes the per-concern detection headers
* in dependency order: language standards (C95-C23, C++98-C++23) in
* env_lang.h; POSIX / XSI levels and features in env_posix.h; CPU
* architecture, width, and endianness in env_arch.h; the OS block / flag
* classification in env_os.h; the compiler, preprocessor limits, and
* __VA_OPT__ in env_compiler.h; C runtime features in c/env_c_lib.h; and the
* build configuration in env_build.h.
*   Include order matters. env_compiler.h comes after env_os.h because its
* preprocessor-limits block reads the D_ENV_PLATFORM_* flags env_os.h
* establishes. c/env_c_lib.h comes last before env_build.h, since it depends
* on the language, architecture, OS, compiler, and D_ENV_IS_OS_POSIX_LIKE*
* macros of every preceding header.
*   Custom environments can be simulated through cfg_env.h: disabling a
* detection section and pre-defining D_ENV_DETECTED_* macros selects the
* result that section reports, for testing code against environments other
* than the host.
*
* path:      /inc/djinterp/env/env.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.23
*******************************************************************************/

#ifndef DJINTERP_ENV_ENV_H
#define DJINTERP_ENV_ENV_H 1

// djinterp
#include "../config/core/env/cfg_env.h"  // D_CFG_ENV_* detection switches
#include "./env_lang.h"                  // language standards (D_ENV_LANG_*)
#include "./env_posix.h"                 // POSIX / XSI levels (D_ENV_POSIX_*)
#include "./env_arch.h"                  // CPU architecture (D_ENV_ARCH_*)
#include "./env_os.h"                    // operating system (D_ENV_OS_*)
#include "./env_compiler.h"              // compiler, preprocessor limits
#include "./c/env_c_lib.h"               // C runtime; must precede env_build.h
#include "./env_build.h"                 // Debug / Release (D_ENV_BUILD_*)


#ifdef D_DEBUG_
    // C linkage for C++ callers. env.h sits below djinterp.h, so the
    // D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
    #if D_ENV_LANG_USING_CPP
        extern "C" {
    #endif

    /**
     * @brief Prints the compiler information detected by the env layer.
     *
     * @note Declared only when D_DEBUG_ is defined.
     */
    void d_env_print_compiler_info(void);

    #if D_ENV_LANG_USING_CPP
        }
    #endif
#endif  // D_DEBUG_


#endif  // DJINTERP_ENV_ENV_H
