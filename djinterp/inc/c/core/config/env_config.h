/******************************************************************************
* djinterp [core]                                                 env_config.h
*
* 
*
*
* path:      \inc\core\config\env_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.09
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENVIRONMENT_
#define DJINTERP_CONFIG_ENVIRONMENT_ 1

#include "..\..\dconfig.h"


// section bit positions for D_CFG_ENV_CUSTOM bitfield
#define D_CFG_ENV_BIT_LANG     0x01  // bit 0: language standard detection
#define D_CFG_ENV_BIT_POSIX    0x02  // bit 1: POSIX standards detection
#define D_CFG_ENV_BIT_COMPILER 0x04  // bit 2: compiler detection  
#define D_CFG_ENV_BIT_OS       0x08  // bit 3: operating system detection
#define D_CFG_ENV_BIT_ARCH     0x10  // bit 4: architecture detection
#define D_CFG_ENV_BIT_BUILD    0x20  // bit 5: build configuration detection

// auto-detection: check for pre-defined D_ENV_DETECTED_* variables
// and automatically set corresponding section bits in D_CFG_ENV_CUSTOM
#if (D_CFG_ENV_CUSTOM > 0)
    // I - language settings
    // detect C and/or CPP
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
#endif  // (D_CFG_ENV_CUSTOM != 1)

// individual section enable flags for easy checking
#define D_CFG_ENV_

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

#define D_CFG_ENV_BUILD_IS_ENABLED     \
    ( (D_CFG_ENV_CUSTOM == 0) ||    \
      (!(D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_BUILD)) )


#endif  // DJINTERP_CONFIG_ENVIRONMENT_