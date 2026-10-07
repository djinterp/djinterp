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
*   Each of them includes its own configuration, cfg_env_<section>.h, and the
* other sections it reads, so it gives the same answers whether a unit includes
* it directly or through this header. The order below is the order of those
* dependencies, but nothing relies on it.
*   The detail headers are opt-in, and this header includes none of them: a
* unit that wants one platform's detail includes it -- env/os/env_apple.h,
* env_ios.h, env_linux.h, env_bsd.h or env_windows.h -- as it does env/db,
* env/net, env/ui and env/util (decision 3 of the register).
*   Custom environments can be simulated through D_CFG_ENV_CUSTOM (cfg_env.h):
* switching a detection section off and pre-defining D_ENV_DETECTED_* macros
* selects the result that section reports, for testing code against environments
* other than the host.
*
*
* path:      /inc/djinterp/env/env.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_ENV_ENV_H
#define DJINTERP_ENV_ENV_H 1

// djinterp
#include "../config/cfg_common.h"  // D_DEBUG_, from the testing preset
#include "./env_lang.h"            // language standards (D_ENV_LANG_*)
#include "./env_posix.h"           // POSIX / XSI levels (D_ENV_POSIX_*)
#include "./env_arch.h"            // CPU architecture (D_ENV_ARCH_*)
#include "./env_os.h"              // operating system (D_ENV_OS_*)
#include "./env_compiler.h"        // compiler, preprocessor limits
#include "./c/env_c_lib.h"         // C runtime features (D_ENV_C_HAS_*)
#include "./env_build.h"           // Debug / Release (D_ENV_BUILD_*)


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
