/******************************************************************************
* djinterp [core]                                                     env_os.h
*
* djinterp operating-system detection (core classification):
*   The OS block/flag classification system, the D_ENV_IS_OS_* helper macros,
* OS detection, and the legacy D_ENV_PLATFORM_* backward-compatibility flags.
* This is the CORE OS layer; deeper per-OS feature headers (env_linux.h,
* env_windows.h, env_apple.h, env_bsd.h, env_ios.h) build on top of it.
*
*   Requires:  cfg_env.h and the architecture section (for the platform-
*              flag derivation). This header is an internal component of env.h
*              and is #included by it; do NOT #include it directly.
*
*
* path:      /inc/djinterp/env/env_os.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.03.27
******************************************************************************/

#ifndef DJINTERP_ENV_OS_
#define DJINTERP_ENV_OS_ 1


// OS flag definitions
#define D_ENV_OS_FLAG_APPLE             0x00
#define D_ENV_OS_FLAG_MACOS             0x01

#define D_ENV_OS_FLAG_UNIX              0x10
#define D_ENV_OS_FLAG_LINUX             0x11

#define D_ENV_OS_FLAG_BSD_DRAGONFLY     0x40
#define D_ENV_OS_FLAG_BSD_FREE          0x41
#define D_ENV_OS_FLAG_BSD_NET           0x42
#define D_ENV_OS_FLAG_BSD_OPEN          0x43
#define D_ENV_OS_FLAG_BSD_OS            0x44

#define D_ENV_OS_FLAG_SOLARIS           0x50

#define D_ENV_OS_FLAG_MSDOS             0x60
#define D_ENV_OS_FLAG_WIN_PC_PRE_XP     0x61
#define D_ENV_OS_FLAG_WIN_PC_XP         0x62
#define D_ENV_OS_FLAG_WIN_PC_VISTA      0x63
#define D_ENV_OS_FLAG_WIN_PC_7          0x64
#define D_ENV_OS_FLAG_WIN_PC_8          0x65
#define D_ENV_OS_FLAG_WIN_PC_10         0x66
#define D_ENV_OS_FLAG_WIN_PC_11         0x67

#define D_ENV_OS_FLAG_WIN_SERVER_NT     0x70
#define D_ENV_OS_FLAG_WIN_SERVER_2000   0x71
#define D_ENV_OS_FLAG_WIN_SERVER_2003   0x72
#define D_ENV_OS_FLAG_WIN_SERVER_2003R2 0x73
#define D_ENV_OS_FLAG_WIN_SERVER_2008   0x74
#define D_ENV_OS_FLAG_WIN_SERVER_2008R2 0x75
#define D_ENV_OS_FLAG_WIN_SERVER_2012   0x76
#define D_ENV_OS_FLAG_WIN_SERVER_2012R2 0x77
#define D_ENV_OS_FLAG_WIN_SERVER_2016   0x78
#define D_ENV_OS_FLAG_WIN_SERVER_2019   0x79
#define D_ENV_OS_FLAG_WIN_SERVER_2022   0x7A

#define D_ENV_OS_FLAG_WIN_EMBED         0x80
#define D_ENV_OS_FLAG_WIN_MOBILE        0x81

#define D_ENV_OS_FLAG_IOS               0x90
#define D_ENV_OS_FLAG_ANDROID           0xA0
#define D_ENV_OS_FLAG_BADA              0xA1
#define D_ENV_OS_FLAG_TIZEN             0xB0

#define D_ENV_OS_FLAG_APOLLO_AEGIS      0xC0
#define D_ENV_OS_FLAG_BEOS              0xC1
#define D_ENV_OS_FLAG_OS2               0xC2
#define D_ENV_OS_FLAG_WINDU             0xD4

#define D_ENV_OS_FLAG_AIX               0xE0
#define D_ENV_OS_FLAG_AMIGA             0xE1
#define D_ENV_OS_FLAG_HP_UX             0xE6
#define D_ENV_OS_FLAG_IRIX              0xE8
#define D_ENV_OS_FLAG_QNX               0xF3
#define D_ENV_OS_FLAG_VMS               0xF7
#define D_ENV_OS_FLAG_ZOS               0xFB
#define D_ENV_OS_FLAG_UNKNOWN           0xFF

// utility constants
#define D_ENV_OS_BLOCK_SIZE 4

#define D_ENV_OS_FLAG_DISCONTINUED_FIRST    D_ENV_OS_FLAG_APOLLO_AEGIS
#define D_ENV_OS_FLAG_DISCONTINUED_LAST     D_ENV_OS_FLAG_WINDU
#define D_ENV_OS_UNSUPPORTED_FIRST          D_ENV_OS_FLAG_DISCONTINUED_FIRST
#define D_ENV_OS_UNSUPPORTED_LAST           D_ENV_OS_FLAG_ZOS
#define D_ENV_OS_VENDOR_MS_FIRST            D_ENV_OS_FLAG_MSDOS
#define D_ENV_OS_VENDOR_MS_LAST             D_ENV_OS_FLAG_WIN_MOBILE
#define D_ENV_OS_FLAG_WIN_FIRST             D_ENV_OS_FLAG_WIN_PC_PRE_XP
#define D_ENV_OS_FLAG_WIN_LAST              D_ENV_OS_FLAG_WIN_MOBILE

// utility macros
#define D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, BLOCK_NUM) \
    (((OS_FLAG) >> D_ENV_OS_BLOCK_SIZE) == (BLOCK_NUM))

#define D_ENV_IS_OS_FLAG_UNIX(OS_FLAG)          \
    (((OS_FLAG) >> D_ENV_OS_BLOCK_SIZE) == 0x1)

#define D_ENV_IS_OS_MOBILE(OS_FLAG)             \
    ((OS_FLAG) == D_ENV_OS_FLAG_IOS     ||      \
     (OS_FLAG) == D_ENV_OS_FLAG_ANDROID ||      \
     (OS_FLAG) == D_ENV_OS_FLAG_BADA)

#define D_ENV_IS_OS_MSDOS(OS_FLAG)              \
    ((OS_FLAG) == D_ENV_OS_FLAG_MSDOS)

#define D_ENV_IS_OS_WINDOWS(OS_FLAG)            \
    ((OS_FLAG) >= D_ENV_OS_FLAG_WIN_FIRST &&    \
     (OS_FLAG) <= D_ENV_OS_FLAG_WIN_LAST)

#define D_ENV_IS_OS_DISCONTINUED(OS_FLAG)               \
    ((OS_FLAG) >= D_ENV_OS_FLAG_DISCONTINUED_FIRST &&   \
     (OS_FLAG) <= D_ENV_OS_FLAG_DISCONTINUED_LAST)

#define D_ENV_IS_OS_UNSUPPORTED(OS_FLAG)                \
    ((OS_FLAG) >= D_ENV_OS_UNSUPPORTED_FIRST &&         \
     (OS_FLAG) <= D_ENV_OS_UNSUPPORTED_LAST)

// D_ENV_IS_OS_POSIX_COMPLIANT
//   macro: checks if the detected OS is likely to be POSIX-compliant.
#define D_ENV_IS_OS_POSIX_COMPLIANT(OS_FLAG)        \
    (D_ENV_IS_OS_FLAG_UNIX(OS_FLAG)             ||  \
     D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x4)    ||  \
     D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x5)    ||  \
     (OS_FLAG) == D_ENV_OS_FLAG_MACOS           ||  \
     (OS_FLAG) == D_ENV_OS_FLAG_ANDROID)

// D_ENV_IS_OS_POSIX_LIKE
//   macro: true for Unix-family (incl. Linux), Apple-family, and
// BSD-family OSes. covers the core set of OSes expected to provide
// traditional POSIX headers and functions.
#define D_ENV_IS_OS_POSIX_LIKE(OS_FLAG)             \
    (D_ENV_IS_OS_FLAG_UNIX(OS_FLAG)             ||  \
     D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x0)    ||  \
     D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x4))

// D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID
//   macro: POSIX-like plus Android (which provides most POSIX APIs).
#define D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(OS_FLAG)  \
    (D_ENV_IS_OS_POSIX_LIKE(OS_FLAG)            ||  \
     (OS_FLAG) == D_ENV_OS_FLAG_ANDROID)

// D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS
//   macro: POSIX-like plus Windows (for APIs available on both).
#define D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(OS_FLAG)  \
    (D_ENV_IS_OS_POSIX_LIKE(OS_FLAG)            ||  \
     D_ENV_IS_OS_WINDOWS(OS_FLAG))

// OS detection logic
#if D_CFG_ENV_OS_ENABLED
    #ifndef D_ENV_OS_ID
        // Android (check first, as it also defines __linux__)
        #if defined(__ANDROID__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_ANDROID
            #define D_ENV_OS_NAME  "Android"

        // BSD variants
        #elif defined(__DragonFly__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_DRAGONFLY
            #define D_ENV_OS_NAME  "DragonFly BSD"

        #elif defined(__FreeBSD__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_FREE
            #define D_ENV_OS_NAME  "FreeBSD"

        #elif defined(__OpenBSD__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_OPEN
            #define D_ENV_OS_NAME  "OpenBSD"

        #elif defined(__NetBSD__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_NET
            #define D_ENV_OS_NAME  "NetBSD"

        #elif defined(__bsdi__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_OS
            #define D_ENV_OS_NAME  "BSD/OS"

        // Windows variants
        #elif defined(_WIN64)
            #define D_ENV_OS_USING_WINDOWS64 1

            #define D_ENV_OS_ID    D_ENV_OS_FLAG_WIN_PC_10
            #define D_ENV_OS_NAME  "Windows (64-bit)"

        #elif defined(_WIN32)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_WIN_PC_10
            #define D_ENV_OS_NAME  "Windows (32-bit)"

        #elif defined(_WIN16)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_WIN_PC_PRE_XP
            #define D_ENV_OS_NAME  "Windows (16-bit)"

        #elif ( defined(__WIN32__)   ||  \
                defined(__TOS_WIN__) ||  \
                defined(__WINDOWS__) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_WIN_PC_10
            #define D_ENV_OS_NAME  "Windows"

        #elif ( defined(MSDOS)     ||  \
                defined(_MSDOS)    ||  \
                defined(__MSDOS__) ||  \
                defined(__DOS__) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_MSDOS
            #define D_ENV_OS_NAME  "MS-DOS"

        // Apple platforms
        #elif defined(__APPLE__)
            // apple
            #include <TargetConditionals.h>

            #if TARGET_OS_IPHONE
                #define D_ENV_OS_ID   D_ENV_OS_FLAG_IOS
                #define D_ENV_OS_NAME "iOS"
            #else
                #define D_ENV_OS_ID   D_ENV_OS_FLAG_MACOS
                #define D_ENV_OS_NAME "macOS"
            #endif

        // Linux
        #elif defined(__linux__)
            #define D_ENV_OS_ID       D_ENV_OS_FLAG_LINUX
            #define D_ENV_OS_NAME     "Linux"

        // Solaris
        #elif ( defined(__sun)  ||  \
                defined(__SVR4) ||  \
                defined(__svr4__) )
            #define D_ENV_OS_ID       D_ENV_OS_FLAG_SOLARIS
            #define D_ENV_OS_NAME     "solaris"

        // Unix variants
        #elif ( defined(__unix__) ||  \
                defined(__unix)   ||  \
                defined(unix) )
            #define D_ENV_OS_ID D_ENV_OS_FLAG_UNIX
            #define D_ENV_OS_NAME "Unix"

        // Legacy/Discontinued systems
        #elif ( defined(_AIX) ||  \
                defined(__TOS_AIX__) )
            #define D_ENV_OS_ID D_ENV_OS_FLAG_AIX
            #define D_ENV_OS_NAME "AIX"

        #elif defined(__hpux)
            #define D_ENV_OS_ID D_ENV_OS_FLAG_HP_UX
            #define D_ENV_OS_NAME "HP-UX"

        #elif ( defined(sgi) ||  \
                defined(__sgi) )
            #define D_ENV_OS_ID D_ENV_OS_FLAG_IRIX
            #define D_ENV_OS_NAME "IRIX"

        #elif defined(__QNX__)
            #define D_ENV_OS_ID D_ENV_OS_FLAG_QNX
            #define D_ENV_OS_NAME "QNX"

        #elif ( defined(__VMS) ||  \
                defined(VMS) )
            #define D_ENV_OS_ID D_ENV_OS_FLAG_VMS
            #define D_ENV_OS_NAME "OpenVMS"

        // Unknown
        #else
            #define D_ENV_OS_ID D_ENV_OS_FLAG_UNKNOWN
            #define D_ENV_OS_NAME "Unknown"

        #endif
    #endif  // D_ENV_OS_ID
#else
    // use pre-defined detection variables when OS detection is disabled
    #ifdef D_ENV_DETECTED_OS_MACOS
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_MACOS
        #define D_ENV_OS_NAME  "macOS"
    #elif defined(D_ENV_DETECTED_OS_IOS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_IOS
        #define D_ENV_OS_NAME  "iOS"
    #elif defined(D_ENV_DETECTED_OS_APPLE)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_APPLE
        #define D_ENV_OS_NAME  "Apple"
    #elif defined(D_ENV_DETECTED_OS_LINUX)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_LINUX
        #define D_ENV_OS_NAME  "Linux"
    #elif defined(D_ENV_DETECTED_OS_ANDROID)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_ANDROID
        #define D_ENV_OS_NAME  "Android"
    #elif defined(D_ENV_DETECTED_OS_WINDOWS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_WIN_PC_10
        #define D_ENV_OS_NAME  "Windows"
    #elif defined(D_ENV_DETECTED_OS_BSD)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_FREE
        #define D_ENV_OS_NAME  "BSD"
    #elif defined(D_ENV_DETECTED_OS_SOLARIS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_SOLARIS
        #define D_ENV_OS_NAME  "solaris"
    #elif defined(D_ENV_DETECTED_OS_UNIX)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_UNIX
        #define D_ENV_OS_NAME  "Unix"
    #elif defined(D_ENV_DETECTED_OS_MSDOS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_MSDOS
        #define D_ENV_OS_NAME  "MS-DOS"
    #elif defined(D_ENV_DETECTED_OS_UNKNOWN)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_UNKNOWN
        #define D_ENV_OS_NAME  "Unknown"
    #endif  //

#endif  // D_CFG_ENV_OS_ENABLED

// legacy platform detection macros for backward compatibility
#if (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID)
    #define D_ENV_PLATFORM_ANDROID 1
#elif ( D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) ||  \
        D_ENV_IS_OS_MSDOS(D_ENV_OS_ID))
    #define D_ENV_PLATFORM_WINDOWS 1
#elif (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
    #define D_ENV_PLATFORM_LINUX 1
#elif (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS)
    #define D_ENV_PLATFORM_MACOS 1
#elif ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID) ||  \
        D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4) )
    #define D_ENV_PLATFORM_UNIX 1
#else
    #define D_ENV_PLATFORM_UNKNOWN 1
#endif

#define D_ENV_PLATFORM_NAME D_ENV_OS_NAME


#endif  // DJINTERP_ENV_OS_
