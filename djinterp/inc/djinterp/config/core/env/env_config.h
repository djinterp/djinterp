/******************************************************************************
* djinterp [core]                                                 env_config.h
*
* Per-module configuration for env.h. Owns all D_CFG_ENV_* macros related
* to the core environment detection bitfield (language, POSIX, compiler,
* OS, architecture, build). Database-related D_CFG_ENV_USING_*, *_C_PATH,
* and *_CPP_PATH macros live in their own per-vendor config files under
* ./db/.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
* path:      /inc/djinterp/config/core/env/env_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.09
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENVIRONMENT_
#define DJINTERP_CONFIG_ENVIRONMENT_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../dconfig.h"
#endif


// ===========================================================================
// I.   BITFIELD POSITIONS (D_CFG_ENV_CUSTOM sections)
// ===========================================================================

// D_CFG_ENV_BIT_LANG
//   constant: bit 0 of D_CFG_ENV_CUSTOM, flags language-standard detection.
#define D_CFG_ENV_BIT_LANG     0x01

// D_CFG_ENV_BIT_POSIX
//   constant: bit 1 of D_CFG_ENV_CUSTOM, flags POSIX-standard detection.
#define D_CFG_ENV_BIT_POSIX    0x02

// D_CFG_ENV_BIT_COMPILER
//   constant: bit 2 of D_CFG_ENV_CUSTOM, flags compiler detection.
#define D_CFG_ENV_BIT_COMPILER 0x04

// D_CFG_ENV_BIT_OS
//   constant: bit 3 of D_CFG_ENV_CUSTOM, flags OS detection.
#define D_CFG_ENV_BIT_OS       0x08

// D_CFG_ENV_BIT_ARCH
//   constant: bit 4 of D_CFG_ENV_CUSTOM, flags architecture detection.
#define D_CFG_ENV_BIT_ARCH     0x10

// D_CFG_ENV_BIT_BUILD
//   constant: bit 5 of D_CFG_ENV_CUSTOM, flags build-configuration detection.
#define D_CFG_ENV_BIT_BUILD    0x20


// ===========================================================================
// II.  MASTER CUSTOM FLAG
// ===========================================================================

// D_CFG_ENV_CUSTOM
//   configuration: master environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1 or bitfield: skip detection in the sections whose bits are set
// Pre-defining any D_ENV_DETECTED_* variable automatically sets the
// corresponding section bit below.
#ifndef D_CFG_ENV_CUSTOM
    #define D_CFG_ENV_CUSTOM 0
#endif


// ===========================================================================
// III. AUTO-DETECTION OF PRE-DEFINED D_ENV_DETECTED_* VARIABLES
// ===========================================================================
//   If the user has pre-defined any D_ENV_DETECTED_* variable for a given
// section, set the corresponding bit in D_CFG_ENV_CUSTOM so that section's
// auto-detection is skipped.

#if (D_CFG_ENV_CUSTOM > 0)
    // I - language settings: detect C and/or CPP
    #if ( defined(D_ENV_DETECTED_CPP98) ||  \
          defined(D_ENV_DETECTED_CPP11) ||  \
          defined(D_ENV_DETECTED_CPP14) ||  \
          defined(D_ENV_DETECTED_CPP17) ||  \
          defined(D_ENV_DETECTED_CPP20) ||  \
          defined(D_ENV_DETECTED_CPP23) )
        #define D_CFG_ENV_DETECTED_CPP  1

    #elif ( defined(D_ENV_DETECTED_C95) ||  \
            defined(D_ENV_DETECTED_C99) ||  \
            defined(D_ENV_DETECTED_C11) ||  \
            defined(D_ENV_DETECTED_C17) ||  \
            defined(D_ENV_DETECTED_C23) )
        #define D_CFG_ENV_DETECTED_C_ONLY   1
    #endif  // defined(D_CFG_ENV_DETECTED_CPP/C_ONLY)

    #if ( defined(D_CFG_ENV_DETECTED_CPP) ||  \
          defined(D_CFG_ENV_DETECTED_C_ONLY) )
        #define D_TEMP_CFG_ENV_CUSTOM_VAL D_CFG_ENV_CUSTOM
        #undef  D_CFG_ENV_CUSTOM
        #define D_CFG_ENV_CUSTOM \
                (D_TEMP_CFG_ENV_CUSTOM_VAL | D_CFG_ENV_BIT_LANG)
        #undef D_TEMP_CFG_ENV_CUSTOM_VAL
    #endif

    // POSIX standards detection variables
    #if ( defined(D_ENV_DETECTED_POSIX_1988)     ||  \
          defined(D_ENV_DETECTED_POSIX_1990)     ||  \
          defined(D_ENV_DETECTED_POSIX_1993)     ||  \
          defined(D_ENV_DETECTED_POSIX_1996)     ||  \
          defined(D_ENV_DETECTED_POSIX_2001)     ||  \
          defined(D_ENV_DETECTED_POSIX_2008)     ||  \
          defined(D_ENV_DETECTED_POSIX_2017)     ||  \
          defined(D_ENV_DETECTED_POSIX_2024)     ||  \
          defined(D_ENV_DETECTED_POSIX_XSI)      ||  \
          defined(D_ENV_DETECTED_POSIX_THREADS)  ||  \
          defined(D_ENV_DETECTED_POSIX_REALTIME) ||  \
          defined(D_ENV_DETECTED_POSIX_SOCKETS)  ||  \
          defined(D_ENV_DETECTED_POSIX_NONE) )
        #define D_TEMP_CFG_ENV_CUSTOM_VAL D_CFG_ENV_CUSTOM
        #undef  D_CFG_ENV_CUSTOM
        #define D_CFG_ENV_CUSTOM \
                (D_TEMP_CFG_ENV_CUSTOM_VAL | D_CFG_ENV_BIT_POSIX)
        #undef D_TEMP_CFG_ENV_CUSTOM_VAL
    #endif

    // compiler detection variables
    #if ( defined(D_ENV_DETECTED_COMPILER_CLANG)       ||  \
          defined(D_ENV_DETECTED_COMPILER_APPLE_CLANG) ||  \
          defined(D_ENV_DETECTED_COMPILER_GCC)         ||  \
          defined(D_ENV_DETECTED_COMPILER_MSVC)        ||  \
          defined(D_ENV_DETECTED_COMPILER_INTEL)       ||  \
          defined(D_ENV_DETECTED_COMPILER_BORLAND)     ||  \
          defined(D_ENV_DETECTED_COMPILER_UNKNOWN) )
        #define D_TEMP_CFG_ENV_CUSTOM_VAL D_CFG_ENV_CUSTOM
        #undef  D_CFG_ENV_CUSTOM
        #define D_CFG_ENV_CUSTOM  \
                (D_TEMP_CFG_ENV_CUSTOM_VAL | D_CFG_ENV_BIT_COMPILER)
        #undef D_TEMP_CFG_ENV_CUSTOM_VAL
    #endif

    #if defined(D_ENV_DETECTED_COMPILER_MSVC)
        #define D_ENV_CRT_MSVC 1
        #define D_ENV_MSC_VER  _MSC_VER
    #else
        #define D_ENV_CRT_MSVC 0
        #define D_ENV_MSC_VER  0
    #endif

    // operating system detection variables
    #if ( defined(D_ENV_DETECTED_OS_APPLE)   ||  \
          defined(D_ENV_DETECTED_OS_MACOS)   ||  \
          defined(D_ENV_DETECTED_OS_IOS)     ||  \
          defined(D_ENV_DETECTED_OS_LINUX)   ||  \
          defined(D_ENV_DETECTED_OS_ANDROID) ||  \
          defined(D_ENV_DETECTED_OS_WINDOWS) ||  \
          defined(D_ENV_DETECTED_OS_BSD)     ||  \
          defined(D_ENV_DETECTED_OS_SOLARIS) ||  \
          defined(D_ENV_DETECTED_OS_UNIX)    ||  \
          defined(D_ENV_DETECTED_OS_MSDOS)   ||  \
          defined(D_ENV_DETECTED_OS_UNKNOWN) )
        #define D_TEMP_CFG_ENV_CUSTOM_VAL D_CFG_ENV_CUSTOM
        #undef  D_CFG_ENV_CUSTOM
        #define D_CFG_ENV_CUSTOM \
                (D_TEMP_CFG_ENV_CUSTOM_VAL | D_CFG_ENV_BIT_OS)
        #undef D_TEMP_CFG_ENV_CUSTOM_VAL
    #endif

    // architecture detection variables
    #if ( defined(D_ENV_DETECTED_ARCH_X86)     ||  \
          defined(D_ENV_DETECTED_ARCH_X64)     ||  \
          defined(D_ENV_DETECTED_ARCH_ARM)     ||  \
          defined(D_ENV_DETECTED_ARCH_ARM64)   ||  \
          defined(D_ENV_DETECTED_ARCH_RISCV)   ||  \
          defined(D_ENV_DETECTED_ARCH_POWERPC) ||  \
          defined(D_ENV_DETECTED_ARCH_MIPS)    ||  \
          defined(D_ENV_DETECTED_ARCH_SPARC)   ||  \
          defined(D_ENV_DETECTED_ARCH_S390)    ||  \
          defined(D_ENV_DETECTED_ARCH_IA64)    ||  \
          defined(D_ENV_DETECTED_ARCH_ALPHA)   ||  \
          defined(D_ENV_DETECTED_ARCH_UNKNOWN) )
        #define D_TEMP_CFG_ENV_CUSTOM_VAL D_CFG_ENV_CUSTOM
        #undef  D_CFG_ENV_CUSTOM
        #define D_CFG_ENV_CUSTOM \
                (D_TEMP_CFG_ENV_CUSTOM_VAL | D_CFG_ENV_BIT_ARCH)
        #undef D_TEMP_CFG_ENV_CUSTOM_VAL
    #endif

    // build configuration detection variables
    #if ( defined(D_ENV_DETECTED_BUILD_DEBUG)   || \
          defined(D_ENV_DETECTED_BUILD_RELEASE) )
        #define D_TEMP_CFG_ENV_CUSTOM_VAL D_CFG_ENV_CUSTOM
        #undef  D_CFG_ENV_CUSTOM
        #define D_CFG_ENV_CUSTOM \
                (D_TEMP_CFG_ENV_CUSTOM_VAL | D_CFG_ENV_BIT_BUILD)
        #undef D_TEMP_CFG_ENV_CUSTOM_VAL
    #endif
#endif  // (D_CFG_ENV_CUSTOM > 0)


// ===========================================================================
// IV.  SECTION-ENABLE HELPER MACROS
// ===========================================================================
//   Each evaluates to a nonzero value when that section's auto-detection
// should run (either full detection is enabled, or the section's bit is
// clear in the custom bitfield).

#define D_CFG_ENV_LANG_ENABLED      \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG)) )

#define D_CFG_ENV_POSIX_ENABLED     \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_POSIX)) )

#define D_CFG_ENV_COMPILER_ENABLED  \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER)) )

#define D_CFG_ENV_OS_ENABLED        \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_OS)) )

#define D_CFG_ENV_ARCH_ENABLED      \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_ARCH)) )

#define D_CFG_ENV_BUILD_ENABLED     \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_BUILD)) )


#endif  // DJINTERP_CONFIG_ENVIRONMENT_
