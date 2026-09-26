/*******************************************************************************
* djinterp [env]                                                        env_os.h
*
* djinterp operating-system detection (core classification).
*   The OS block / flag classification system, the D_ENV_IS_OS_* helper
* macros, OS detection, and the legacy D_ENV_PLATFORM_* backward-compatibility
* flags. This is the core OS layer; the per-OS feature headers (env_linux.h,
* env_windows.h, env_apple.h, env_bsd.h, env_ios.h) build on top of it.
*   Requires cfg_env.h and the architecture section (for the platform-flag
* derivation). This header is an internal component of env.h and is #included
* by it; do not #include it directly.
*
* path:      /inc/djinterp/env/env_os.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OS IDENTIFIERS
    --------------
    1.  OS flags
         1.  Apple flags (block 0x0)
              1.  D_ENV_OS_FLAG_APPLE
              2.  D_ENV_OS_FLAG_MACOS
         2.  Unix flags (block 0x1)
              1.  D_ENV_OS_FLAG_UNIX
              2.  D_ENV_OS_FLAG_LINUX
         3.  BSD flags (block 0x4)
              1.  D_ENV_OS_FLAG_BSD_DRAGONFLY
              2.  D_ENV_OS_FLAG_BSD_FREE
              3.  D_ENV_OS_FLAG_BSD_NET
              4.  D_ENV_OS_FLAG_BSD_OPEN
              5.  D_ENV_OS_FLAG_BSD_OS
         4.  Solaris flags (block 0x5)
              1.  D_ENV_OS_FLAG_SOLARIS
         5.  Microsoft desktop flags (block 0x6)
              1.  D_ENV_OS_FLAG_MSDOS
              2.  D_ENV_OS_FLAG_WIN_PC_PRE_XP
              3.  D_ENV_OS_FLAG_WIN_PC_XP
              4.  D_ENV_OS_FLAG_WIN_PC_VISTA
              5.  D_ENV_OS_FLAG_WIN_PC_7
              6.  D_ENV_OS_FLAG_WIN_PC_8
              7.  D_ENV_OS_FLAG_WIN_PC_10
              8.  D_ENV_OS_FLAG_WIN_PC_11
         6.  Windows Server flags (block 0x7)
              1.  D_ENV_OS_FLAG_WIN_SERVER_NT
              2.  D_ENV_OS_FLAG_WIN_SERVER_2000
              3.  D_ENV_OS_FLAG_WIN_SERVER_2003
              4.  D_ENV_OS_FLAG_WIN_SERVER_2003R2
              5.  D_ENV_OS_FLAG_WIN_SERVER_2008
              6.  D_ENV_OS_FLAG_WIN_SERVER_2008R2
              7.  D_ENV_OS_FLAG_WIN_SERVER_2012
              8.  D_ENV_OS_FLAG_WIN_SERVER_2012R2
              9.  D_ENV_OS_FLAG_WIN_SERVER_2016
              10. D_ENV_OS_FLAG_WIN_SERVER_2019
              11. D_ENV_OS_FLAG_WIN_SERVER_2022
         7.  Windows embedded and mobile flags (block 0x8)
              1.  D_ENV_OS_FLAG_WIN_EMBED
              2.  D_ENV_OS_FLAG_WIN_MOBILE
         8.  Mobile platform flags (blocks 0x9-0xB)
              1.  D_ENV_OS_FLAG_IOS
              2.  D_ENV_OS_FLAG_ANDROID
              3.  D_ENV_OS_FLAG_BADA
              4.  D_ENV_OS_FLAG_TIZEN
         9.  Discontinued OS flags (blocks 0xC-0xD)
              1.  D_ENV_OS_FLAG_APOLLO_AEGIS
              2.  D_ENV_OS_FLAG_BEOS
              3.  D_ENV_OS_FLAG_OS2
              4.  D_ENV_OS_FLAG_WINDU
         10. Other and unknown OS flags (blocks 0xE-0xF)
              1.  D_ENV_OS_FLAG_AIX
              2.  D_ENV_OS_FLAG_AMIGA
              3.  D_ENV_OS_FLAG_HP_UX
              4.  D_ENV_OS_FLAG_IRIX
              5.  D_ENV_OS_FLAG_QNX
              6.  D_ENV_OS_FLAG_VMS
              7.  D_ENV_OS_FLAG_ZOS
              8.  D_ENV_OS_FLAG_UNKNOWN
    2.  Flag ranges
         1.  D_ENV_OS_BLOCK_SIZE
         2.  Flag range bounds
              1.  D_ENV_OS_FLAG_DISCONTINUED_FIRST
              2.  D_ENV_OS_FLAG_DISCONTINUED_LAST
              3.  D_ENV_OS_UNSUPPORTED_FIRST
              4.  D_ENV_OS_UNSUPPORTED_LAST
              5.  D_ENV_OS_VENDOR_MS_FIRST
              6.  D_ENV_OS_VENDOR_MS_LAST
              7.  D_ENV_OS_FLAG_WIN_FIRST
              8.  D_ENV_OS_FLAG_WIN_LAST
2.  OS CLASSIFICATION
    -----------------
    1.  Block tests
         1.  D_ENV_IS_OS_FLAG_IN_BLOCK
         2.  D_ENV_IS_OS_FLAG_UNIX
    2.  Family tests
         1.  D_ENV_IS_OS_MOBILE
         2.  D_ENV_IS_OS_MSDOS
         3.  D_ENV_IS_OS_WINDOWS
         4.  D_ENV_IS_OS_DISCONTINUED
         5.  D_ENV_IS_OS_UNSUPPORTED
    3.  POSIX tests
         1.  D_ENV_IS_OS_POSIX_COMPLIANT
         2.  D_ENV_IS_OS_POSIX_LIKE
         3.  D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID
         4.  D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS
3.  OS DETECTION
    ------------
    1.  Automatic detection
         1.  D_ENV_OS_ID and D_ENV_OS_NAME
              1.  Android
              2.  BSD variants
              3.  Windows variants
              4.  MS-DOS
              5.  Apple platforms
              6.  Linux
              7.  Solaris
              8.  Unix variants
              9.  Legacy and discontinued systems
              10. Unknown
    2.  Predefined detection
         1.  D_ENV_DETECTED_OS_* overrides
              1.  macOS
              2.  iOS
              3.  Apple
              4.  Linux
              5.  Android
              6.  Windows
              7.  BSD
              8.  Solaris
              9.  Unix
              10. MS-DOS
              11. Unknown
4.  LEGACY PLATFORM FLAGS
    ---------------------
    1.  Platform flags
         1.  D_ENV_PLATFORM_*
         2.  D_ENV_PLATFORM_NAME
*/

#ifndef DJINTERP_ENV_ENV_OS_H
#define DJINTERP_ENV_ENV_OS_H 1


//==============================================================================
// 1.  OS IDENTIFIERS
//==============================================================================
// Every OS flag is one byte: the high nibble is a block that groups related
// systems, the low nibble the system's position within it. The block
// classification macros in section 2 test the high nibble.


// 1.1    OS flags
//------------------------------------------------------------------------------
// 1.1.1
// Apple flags (block 0x0)

// 1.1.1.1
// D_ENV_OS_FLAG_APPLE
//   constant: OS flag for a generic Apple platform.
#define D_ENV_OS_FLAG_APPLE             0x00

// 1.1.1.2
// D_ENV_OS_FLAG_MACOS
//   constant: OS flag for macOS.
#define D_ENV_OS_FLAG_MACOS             0x01

// 1.1.2
// Unix flags (block 0x1)

// 1.1.2.1
// D_ENV_OS_FLAG_UNIX
//   constant: OS flag for a generic Unix.
#define D_ENV_OS_FLAG_UNIX              0x10

// 1.1.2.2
// D_ENV_OS_FLAG_LINUX
//   constant: OS flag for Linux.
#define D_ENV_OS_FLAG_LINUX             0x11

// 1.1.3
// BSD flags (block 0x4)

// 1.1.3.1
// D_ENV_OS_FLAG_BSD_DRAGONFLY
//   constant: OS flag for DragonFly BSD.
#define D_ENV_OS_FLAG_BSD_DRAGONFLY     0x40

// 1.1.3.2
// D_ENV_OS_FLAG_BSD_FREE
//   constant: OS flag for FreeBSD.
#define D_ENV_OS_FLAG_BSD_FREE          0x41

// 1.1.3.3
// D_ENV_OS_FLAG_BSD_NET
//   constant: OS flag for NetBSD.
#define D_ENV_OS_FLAG_BSD_NET           0x42

// 1.1.3.4
// D_ENV_OS_FLAG_BSD_OPEN
//   constant: OS flag for OpenBSD.
#define D_ENV_OS_FLAG_BSD_OPEN          0x43

// 1.1.3.5
// D_ENV_OS_FLAG_BSD_OS
//   constant: OS flag for BSD/OS (BSDi).
#define D_ENV_OS_FLAG_BSD_OS            0x44

// 1.1.4
// Solaris flags (block 0x5)

// 1.1.4.1
// D_ENV_OS_FLAG_SOLARIS
//   constant: OS flag for Solaris.
#define D_ENV_OS_FLAG_SOLARIS           0x50

// 1.1.5
// Microsoft desktop flags (block 0x6)

// 1.1.5.1
// D_ENV_OS_FLAG_MSDOS
//   constant: OS flag for MS-DOS.
#define D_ENV_OS_FLAG_MSDOS             0x60

// 1.1.5.2
// D_ENV_OS_FLAG_WIN_PC_PRE_XP
//   constant: OS flag for desktop Windows before XP.
#define D_ENV_OS_FLAG_WIN_PC_PRE_XP     0x61

// 1.1.5.3
// D_ENV_OS_FLAG_WIN_PC_XP
//   constant: OS flag for Windows XP.
#define D_ENV_OS_FLAG_WIN_PC_XP         0x62

// 1.1.5.4
// D_ENV_OS_FLAG_WIN_PC_VISTA
//   constant: OS flag for Windows Vista.
#define D_ENV_OS_FLAG_WIN_PC_VISTA      0x63

// 1.1.5.5
// D_ENV_OS_FLAG_WIN_PC_7
//   constant: OS flag for Windows 7.
#define D_ENV_OS_FLAG_WIN_PC_7          0x64

// 1.1.5.6
// D_ENV_OS_FLAG_WIN_PC_8
//   constant: OS flag for Windows 8.
#define D_ENV_OS_FLAG_WIN_PC_8          0x65

// 1.1.5.7
// D_ENV_OS_FLAG_WIN_PC_10
//   constant: OS flag for Windows 10.
#define D_ENV_OS_FLAG_WIN_PC_10         0x66

// 1.1.5.8
// D_ENV_OS_FLAG_WIN_PC_11
//   constant: OS flag for Windows 11.
#define D_ENV_OS_FLAG_WIN_PC_11         0x67

// 1.1.6
// Windows Server flags (block 0x7)

// 1.1.6.1
// D_ENV_OS_FLAG_WIN_SERVER_NT
//   constant: OS flag for Windows NT Server.
#define D_ENV_OS_FLAG_WIN_SERVER_NT     0x70

// 1.1.6.2
// D_ENV_OS_FLAG_WIN_SERVER_2000
//   constant: OS flag for Windows 2000 Server.
#define D_ENV_OS_FLAG_WIN_SERVER_2000   0x71

// 1.1.6.3
// D_ENV_OS_FLAG_WIN_SERVER_2003
//   constant: OS flag for Windows Server 2003.
#define D_ENV_OS_FLAG_WIN_SERVER_2003   0x72

// 1.1.6.4
// D_ENV_OS_FLAG_WIN_SERVER_2003R2
//   constant: OS flag for Windows Server 2003 R2.
#define D_ENV_OS_FLAG_WIN_SERVER_2003R2 0x73

// 1.1.6.5
// D_ENV_OS_FLAG_WIN_SERVER_2008
//   constant: OS flag for Windows Server 2008.
#define D_ENV_OS_FLAG_WIN_SERVER_2008   0x74

// 1.1.6.6
// D_ENV_OS_FLAG_WIN_SERVER_2008R2
//   constant: OS flag for Windows Server 2008 R2.
#define D_ENV_OS_FLAG_WIN_SERVER_2008R2 0x75

// 1.1.6.7
// D_ENV_OS_FLAG_WIN_SERVER_2012
//   constant: OS flag for Windows Server 2012.
#define D_ENV_OS_FLAG_WIN_SERVER_2012   0x76

// 1.1.6.8
// D_ENV_OS_FLAG_WIN_SERVER_2012R2
//   constant: OS flag for Windows Server 2012 R2.
#define D_ENV_OS_FLAG_WIN_SERVER_2012R2 0x77

// 1.1.6.9
// D_ENV_OS_FLAG_WIN_SERVER_2016
//   constant: OS flag for Windows Server 2016.
#define D_ENV_OS_FLAG_WIN_SERVER_2016   0x78

// 1.1.6.10
// D_ENV_OS_FLAG_WIN_SERVER_2019
//   constant: OS flag for Windows Server 2019.
#define D_ENV_OS_FLAG_WIN_SERVER_2019   0x79

// 1.1.6.11
// D_ENV_OS_FLAG_WIN_SERVER_2022
//   constant: OS flag for Windows Server 2022.
#define D_ENV_OS_FLAG_WIN_SERVER_2022   0x7A

// 1.1.7
// Windows embedded and mobile flags (block 0x8)

// 1.1.7.1
// D_ENV_OS_FLAG_WIN_EMBED
//   constant: OS flag for Windows Embedded.
#define D_ENV_OS_FLAG_WIN_EMBED         0x80

// 1.1.7.2
// D_ENV_OS_FLAG_WIN_MOBILE
//   constant: OS flag for Windows Mobile.
#define D_ENV_OS_FLAG_WIN_MOBILE        0x81

// 1.1.8
// Mobile platform flags (blocks 0x9-0xB)

// 1.1.8.1
// D_ENV_OS_FLAG_IOS
//   constant: OS flag for iOS.
#define D_ENV_OS_FLAG_IOS               0x90

// 1.1.8.2
// D_ENV_OS_FLAG_ANDROID
//   constant: OS flag for Android.
#define D_ENV_OS_FLAG_ANDROID           0xA0

// 1.1.8.3
// D_ENV_OS_FLAG_BADA
//   constant: OS flag for Samsung Bada.
#define D_ENV_OS_FLAG_BADA              0xA1

// 1.1.8.4
// D_ENV_OS_FLAG_TIZEN
//   constant: OS flag for Tizen.
#define D_ENV_OS_FLAG_TIZEN             0xB0

// 1.1.9
// Discontinued OS flags (blocks 0xC-0xD)

// 1.1.9.1
// D_ENV_OS_FLAG_APOLLO_AEGIS
//   constant: OS flag for Apollo AEGIS (discontinued).
#define D_ENV_OS_FLAG_APOLLO_AEGIS      0xC0

// 1.1.9.2
// D_ENV_OS_FLAG_BEOS
//   constant: OS flag for BeOS (discontinued).
#define D_ENV_OS_FLAG_BEOS              0xC1

// 1.1.9.3
// D_ENV_OS_FLAG_OS2
//   constant: OS flag for OS/2 (discontinued).
#define D_ENV_OS_FLAG_OS2               0xC2

// 1.1.9.4
// D_ENV_OS_FLAG_WINDU
//   constant: OS flag for Windu (discontinued).
#define D_ENV_OS_FLAG_WINDU             0xD4

// 1.1.10
// Other and unknown OS flags (blocks 0xE-0xF)

// 1.1.10.1
// D_ENV_OS_FLAG_AIX
//   constant: OS flag for IBM AIX.
#define D_ENV_OS_FLAG_AIX               0xE0

// 1.1.10.2
// D_ENV_OS_FLAG_AMIGA
//   constant: OS flag for AmigaOS.
#define D_ENV_OS_FLAG_AMIGA             0xE1

// 1.1.10.3
// D_ENV_OS_FLAG_HP_UX
//   constant: OS flag for HP-UX.
#define D_ENV_OS_FLAG_HP_UX             0xE6

// 1.1.10.4
// D_ENV_OS_FLAG_IRIX
//   constant: OS flag for SGI IRIX.
#define D_ENV_OS_FLAG_IRIX              0xE8

// 1.1.10.5
// D_ENV_OS_FLAG_QNX
//   constant: OS flag for QNX.
#define D_ENV_OS_FLAG_QNX               0xF3

// 1.1.10.6
// D_ENV_OS_FLAG_VMS
//   constant: OS flag for OpenVMS.
#define D_ENV_OS_FLAG_VMS               0xF7

// 1.1.10.7
// D_ENV_OS_FLAG_ZOS
//   constant: OS flag for IBM z/OS.
#define D_ENV_OS_FLAG_ZOS               0xFB

// 1.1.10.8
// D_ENV_OS_FLAG_UNKNOWN
//   constant: OS flag for an unrecognized operating system.
#define D_ENV_OS_FLAG_UNKNOWN           0xFF

// 1.2    Flag ranges
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_OS_BLOCK_SIZE
//   constant: width in bits of a flag's position within its block; shifting a
// flag right by this many bits yields its block number.
#define D_ENV_OS_BLOCK_SIZE 4

// 1.2.2
// Flag range bounds

// 1.2.2.1
// D_ENV_OS_FLAG_DISCONTINUED_FIRST
//   constant: first flag of the discontinued systems.
#define D_ENV_OS_FLAG_DISCONTINUED_FIRST    D_ENV_OS_FLAG_APOLLO_AEGIS

// 1.2.2.2
// D_ENV_OS_FLAG_DISCONTINUED_LAST
//   constant: last flag of the discontinued systems.
#define D_ENV_OS_FLAG_DISCONTINUED_LAST     D_ENV_OS_FLAG_WINDU

// 1.2.2.3
// D_ENV_OS_UNSUPPORTED_FIRST
//   constant: first flag of the unsupported systems.
#define D_ENV_OS_UNSUPPORTED_FIRST          D_ENV_OS_FLAG_DISCONTINUED_FIRST

// 1.2.2.4
// D_ENV_OS_UNSUPPORTED_LAST
//   constant: last flag of the unsupported systems.
#define D_ENV_OS_UNSUPPORTED_LAST           D_ENV_OS_FLAG_ZOS

// 1.2.2.5
// D_ENV_OS_VENDOR_MS_FIRST
//   constant: first Microsoft flag.
#define D_ENV_OS_VENDOR_MS_FIRST            D_ENV_OS_FLAG_MSDOS

// 1.2.2.6
// D_ENV_OS_VENDOR_MS_LAST
//   constant: last Microsoft flag.
#define D_ENV_OS_VENDOR_MS_LAST             D_ENV_OS_FLAG_WIN_MOBILE

// 1.2.2.7
// D_ENV_OS_FLAG_WIN_FIRST
//   constant: first Windows flag.
#define D_ENV_OS_FLAG_WIN_FIRST             D_ENV_OS_FLAG_WIN_PC_PRE_XP

// 1.2.2.8
// D_ENV_OS_FLAG_WIN_LAST
//   constant: last Windows flag.
#define D_ENV_OS_FLAG_WIN_LAST              D_ENV_OS_FLAG_WIN_MOBILE


//==============================================================================
// 2.  OS CLASSIFICATION
//==============================================================================
// Predicates over an OS flag, usable in #if. Each takes the flag to test,
// normally D_ENV_OS_ID.


// 2.1    Block tests
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_IS_OS_FLAG_IN_BLOCK
//   macro: 1 when OS_FLAG lies in block BLOCK_NUM, 0 otherwise.
#define D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, BLOCK_NUM)                       \
    ( ((OS_FLAG) >> D_ENV_OS_BLOCK_SIZE) == (BLOCK_NUM) )

// 2.1.2
// D_ENV_IS_OS_FLAG_UNIX
//   macro: 1 when OS_FLAG lies in the Unix block (0x1), which includes Linux; 0
// otherwise.
#define D_ENV_IS_OS_FLAG_UNIX(OS_FLAG)                                      \
    ( ((OS_FLAG) >> D_ENV_OS_BLOCK_SIZE) == 0x1 )

// 2.2    Family tests
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_IS_OS_MOBILE
//   macro: 1 for iOS, Android, and Bada; 0 otherwise.
#define D_ENV_IS_OS_MOBILE(OS_FLAG)                                         \
    ( ((OS_FLAG) == D_ENV_OS_FLAG_IOS)     ||                               \
      ((OS_FLAG) == D_ENV_OS_FLAG_ANDROID) ||                               \
      ((OS_FLAG) == D_ENV_OS_FLAG_BADA) )

// 2.2.2
// D_ENV_IS_OS_MSDOS
//   macro: 1 for MS-DOS; 0 otherwise.
#define D_ENV_IS_OS_MSDOS(OS_FLAG)                                          \
    ( (OS_FLAG) == D_ENV_OS_FLAG_MSDOS )

// 2.2.3
// D_ENV_IS_OS_WINDOWS
//   macro: 1 for any Windows flag, desktop, server, embedded, or mobile; 0
// otherwise.
#define D_ENV_IS_OS_WINDOWS(OS_FLAG)                                        \
    ( ((OS_FLAG) >= D_ENV_OS_FLAG_WIN_FIRST) &&                             \
      ((OS_FLAG) <= D_ENV_OS_FLAG_WIN_LAST) )

// 2.2.4
// D_ENV_IS_OS_DISCONTINUED
//   macro: 1 for a discontinued system; 0 otherwise.
#define D_ENV_IS_OS_DISCONTINUED(OS_FLAG)                                   \
    ( ((OS_FLAG) >= D_ENV_OS_FLAG_DISCONTINUED_FIRST) &&                    \
      ((OS_FLAG) <= D_ENV_OS_FLAG_DISCONTINUED_LAST) )

// 2.2.5
// D_ENV_IS_OS_UNSUPPORTED
//   macro: 1 for a system the framework does not support; 0 otherwise.
#define D_ENV_IS_OS_UNSUPPORTED(OS_FLAG)                                    \
    ( ((OS_FLAG) >= D_ENV_OS_UNSUPPORTED_FIRST) &&                          \
      ((OS_FLAG) <= D_ENV_OS_UNSUPPORTED_LAST) )

// 2.3    POSIX tests
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_IS_OS_POSIX_COMPLIANT
//   macro: 1 when the OS is likely to be POSIX-compliant: the Unix, BSD, and
// Solaris blocks, macOS, and Android; 0 otherwise.
#define D_ENV_IS_OS_POSIX_COMPLIANT(OS_FLAG)                                \
    ( (D_ENV_IS_OS_FLAG_UNIX(OS_FLAG))           ||                         \
      (D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x4)) ||                          \
      (D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x5)) ||                          \
      ((OS_FLAG) == D_ENV_OS_FLAG_MACOS)        ||                          \
      ((OS_FLAG) == D_ENV_OS_FLAG_ANDROID) )

// 2.3.2
// D_ENV_IS_OS_POSIX_LIKE
//   macro: 1 for the Unix (Linux included), Apple, and BSD blocks, the systems
// expected to provide the traditional POSIX headers and functions; 0 otherwise.
#define D_ENV_IS_OS_POSIX_LIKE(OS_FLAG)                                     \
    ( (D_ENV_IS_OS_FLAG_UNIX(OS_FLAG))           ||                         \
      (D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x0)) ||                          \
      (D_ENV_IS_OS_FLAG_IN_BLOCK(OS_FLAG, 0x4)) )

// 2.3.3
// D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID
//   macro: D_ENV_IS_OS_POSIX_LIKE plus Android, which provides most POSIX APIs.
#define D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(OS_FLAG)                          \
    ( (D_ENV_IS_OS_POSIX_LIKE(OS_FLAG)) ||                                  \
      ((OS_FLAG) == D_ENV_OS_FLAG_ANDROID) )

// 2.3.4
// D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS
//   macro: D_ENV_IS_OS_POSIX_LIKE plus Windows, for APIs available on both.
#define D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(OS_FLAG)                          \
    ( (D_ENV_IS_OS_POSIX_LIKE(OS_FLAG)) ||                                  \
      (D_ENV_IS_OS_WINDOWS(OS_FLAG)) )


//==============================================================================
// 3.  OS DETECTION
//==============================================================================
// Selects exactly one OS flag, from the compiler's predefined macros or, when
// detection is disabled, from the D_ENV_DETECTED_OS_* overrides.


#if D_CFG_ENV_OS_ENABLED

// 3.1    Automatic detection
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_OS_ID and D_ENV_OS_NAME
//   constant: the detected system's OS flag and its human-readable name.
// The first matching case wins, so the order is significant: Android before
// Linux, and each specific Windows macro before the generic ones.

    #ifndef D_ENV_OS_ID
        // 3.1.1.1
        // Android
        //   checked first: Android also defines __linux__.
        #if defined(__ANDROID__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_ANDROID
            #define D_ENV_OS_NAME  "Android"

        // 3.1.1.2
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

        // 3.1.1.3
        // Windows variants
        #elif defined(_WIN64)
            // D_ENV_OS_USING_WINDOWS64
            //   constant: 1 on 64-bit Windows; undefined elsewhere.
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

        // 3.1.1.4
        // MS-DOS
        #elif ( defined(MSDOS)     ||  \
                defined(_MSDOS)    ||  \
                defined(__MSDOS__) ||  \
                defined(__DOS__) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_MSDOS
            #define D_ENV_OS_NAME  "MS-DOS"

        // 3.1.1.5
        // Apple platforms
        #elif defined(__APPLE__)
            // apple
            #include <TargetConditionals.h>  // TARGET_OS_IPHONE

            #if TARGET_OS_IPHONE
                #define D_ENV_OS_ID    D_ENV_OS_FLAG_IOS
                #define D_ENV_OS_NAME  "iOS"
            #else
                #define D_ENV_OS_ID    D_ENV_OS_FLAG_MACOS
                #define D_ENV_OS_NAME  "macOS"
            #endif

        // 3.1.1.6
        // Linux
        #elif defined(__linux__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_LINUX
            #define D_ENV_OS_NAME  "Linux"

        // 3.1.1.7
        // Solaris
        #elif ( defined(__sun)  ||  \
                defined(__SVR4) ||  \
                defined(__svr4__) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_SOLARIS
            #define D_ENV_OS_NAME  "solaris"

        // 3.1.1.8
        // Unix variants
        #elif ( defined(__unix__) ||  \
                defined(__unix)   ||  \
                defined(unix) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_UNIX
            #define D_ENV_OS_NAME  "Unix"

        // 3.1.1.9
        // Legacy and discontinued systems
        #elif ( defined(_AIX) ||  \
                defined(__TOS_AIX__) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_AIX
            #define D_ENV_OS_NAME  "AIX"

        #elif defined(__hpux)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_HP_UX
            #define D_ENV_OS_NAME  "HP-UX"

        #elif ( defined(sgi) ||  \
                defined(__sgi) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_IRIX
            #define D_ENV_OS_NAME  "IRIX"

        #elif defined(__QNX__)
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_QNX
            #define D_ENV_OS_NAME  "QNX"

        #elif ( defined(__VMS) ||  \
                defined(VMS) )
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_VMS
            #define D_ENV_OS_NAME  "OpenVMS"

        // 3.1.1.10
        // Unknown
        #else
            #define D_ENV_OS_ID    D_ENV_OS_FLAG_UNKNOWN
            #define D_ENV_OS_NAME  "Unknown"

        #endif
    #endif  // D_ENV_OS_ID
#else

// 3.2    Predefined detection
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_DETECTED_OS_* overrides

    // use pre-defined detection variables when OS detection is disabled
    // 3.2.1.1
    // macOS
    #ifdef D_ENV_DETECTED_OS_MACOS
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_MACOS
        #define D_ENV_OS_NAME  "macOS"
    // 3.2.1.2
    // iOS
    #elif defined(D_ENV_DETECTED_OS_IOS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_IOS
        #define D_ENV_OS_NAME  "iOS"
    // 3.2.1.3
    // Apple
    #elif defined(D_ENV_DETECTED_OS_APPLE)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_APPLE
        #define D_ENV_OS_NAME  "Apple"
    // 3.2.1.4
    // Linux
    #elif defined(D_ENV_DETECTED_OS_LINUX)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_LINUX
        #define D_ENV_OS_NAME  "Linux"
    // 3.2.1.5
    // Android
    #elif defined(D_ENV_DETECTED_OS_ANDROID)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_ANDROID
        #define D_ENV_OS_NAME  "Android"
    // 3.2.1.6
    // Windows
    #elif defined(D_ENV_DETECTED_OS_WINDOWS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_WIN_PC_10
        #define D_ENV_OS_NAME  "Windows"
    // 3.2.1.7
    // BSD
    #elif defined(D_ENV_DETECTED_OS_BSD)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_BSD_FREE
        #define D_ENV_OS_NAME  "BSD"
    // 3.2.1.8
    // Solaris
    #elif defined(D_ENV_DETECTED_OS_SOLARIS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_SOLARIS
        #define D_ENV_OS_NAME  "solaris"
    // 3.2.1.9
    // Unix
    #elif defined(D_ENV_DETECTED_OS_UNIX)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_UNIX
        #define D_ENV_OS_NAME  "Unix"
    // 3.2.1.10
    // MS-DOS
    #elif defined(D_ENV_DETECTED_OS_MSDOS)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_MSDOS
        #define D_ENV_OS_NAME  "MS-DOS"
    // 3.2.1.11
    // Unknown
    #elif defined(D_ENV_DETECTED_OS_UNKNOWN)
        #define D_ENV_OS_ID    D_ENV_OS_FLAG_UNKNOWN
        #define D_ENV_OS_NAME  "Unknown"
    #endif  // D_ENV_DETECTED_OS_MACOS

#endif  // D_CFG_ENV_OS_ENABLED


//==============================================================================
// 4.  LEGACY PLATFORM FLAGS
//==============================================================================
// Kept for backward compatibility; new code tests D_ENV_OS_ID with the
// section 2 predicates.


// 4.1    Platform flags
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_PLATFORM_*
//   constant: exactly one of D_ENV_PLATFORM_ANDROID, _WINDOWS, _LINUX,
// _MACOS, _UNIX, or _UNKNOWN is defined, to 1, from D_ENV_OS_ID.
#if (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID)
    #define D_ENV_PLATFORM_ANDROID 1
#elif ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) ||                                 \
        (D_ENV_IS_OS_MSDOS(D_ENV_OS_ID)) )
    #define D_ENV_PLATFORM_WINDOWS 1
#elif (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
    #define D_ENV_PLATFORM_LINUX 1
#elif (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS)
    #define D_ENV_PLATFORM_MACOS 1
#elif ( (D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)) ||                               \
        (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)) )
    #define D_ENV_PLATFORM_UNIX 1
#else
    #define D_ENV_PLATFORM_UNKNOWN 1
#endif

// 4.1.2
// D_ENV_PLATFORM_NAME
//   constant: legacy alias for D_ENV_OS_NAME.
#define D_ENV_PLATFORM_NAME D_ENV_OS_NAME


#endif  // DJINTERP_ENV_ENV_OS_H
