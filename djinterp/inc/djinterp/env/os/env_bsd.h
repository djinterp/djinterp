/*******************************************************************************
* djinterp [env]                                                       env_bsd.h
*
* djinterp BSD environment detection.
*   Compile-time detection of the BSD compilation environment across the major
* BSD variants:
*     - FreeBSD (including derivatives such as GhostBSD and MidnightBSD)
*     - OpenBSD
*     - NetBSD
*     - DragonFly BSD
*     - BSD/OS (historical)
*   It covers:
*     - BSD variant identification and version detection
*     - BSD-common features (kqueue, arc4random, pledge, jails, capsicum)
*     - per-variant version constants and version-gated APIs
*     - C library and libc feature availability
*     - security frameworks (Capsicum, pledge/unveil, securelevel, pf)
*     - networking (pf, CARP, routing sockets, BPF, sendfile variants)
*     - filesystem features (ZFS, HAMMER2, FFS/UFS, nullfs, tmpfs)
*     - memory and process management (mmap flags, rfork, kinfo_proc)
*     - hardware and device access (sysctl, devctl, devd)
*     - display server indicators (X11, Wayland)
*     - BSD-specific header availability
*   Include it where BSD detail is needed, guarded by the BSD OS block; from
* env.h's directory, for example:
*     #if D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)
*         #include "./os/env_bsd.h"
*     #endif
*   Version detection reads <sys/param.h>'s macros; see section 2.
*   Naming: D_ENV_BSD_<CATEGORY>_<FEATURE> is common across the variants,
* while D_ENV_FBSD_, D_ENV_OBSD_, D_ENV_NBSD_, and D_ENV_DBSD_ prefixes are
* FreeBSD-, OpenBSD-, NetBSD-, and DragonFly-specific.
*
* path:      /inc/djinterp/env/os/env_bsd.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.28
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VARIANT IDENTIFICATION
    ----------------------
    1.  Variant detection
         1.  D_ENV_BSD_IS_FREEBSD
         2.  D_ENV_BSD_IS_OPENBSD
         3.  D_ENV_BSD_IS_NETBSD
         4.  D_ENV_BSD_IS_DRAGONFLY
         5.  D_ENV_BSD_IS_BSDOS
    2.  Variant name
         1.  D_ENV_BSD_VARIANT_NAME
2.  VARIANT VERSIONS
    ----------------
    1.  FreeBSD version constants
         1.  D_ENV_FBSD_VER_9
         2.  D_ENV_FBSD_VER_10
         3.  D_ENV_FBSD_VER_10_3
         4.  D_ENV_FBSD_VER_11
         5.  D_ENV_FBSD_VER_11_1
         6.  D_ENV_FBSD_VER_12
         7.  D_ENV_FBSD_VER_12_2
         8.  D_ENV_FBSD_VER_13
         9.  D_ENV_FBSD_VER_13_1
         10. D_ENV_FBSD_VER_13_2
         11. D_ENV_FBSD_VER_13_3
         12. D_ENV_FBSD_VER_14
         13. D_ENV_FBSD_VER_14_1
         14. D_ENV_FBSD_VER_14_2
         15. D_ENV_FBSD_VER_15
    2.  Detected FreeBSD version
         1.  D_ENV_FBSD_VER / D_ENV_FBSD_VER_DETECTED
         2.  D_ENV_FBSD_AT_LEAST
    3.  OpenBSD version constants
         1.  D_ENV_OBSD_VER_5_5
         2.  D_ENV_OBSD_VER_5_7
         3.  D_ENV_OBSD_VER_5_9
         4.  D_ENV_OBSD_VER_6_0
         5.  D_ENV_OBSD_VER_6_4
         6.  D_ENV_OBSD_VER_6_5
         7.  D_ENV_OBSD_VER_6_8
         8.  D_ENV_OBSD_VER_7_0
         9.  D_ENV_OBSD_VER_7_1
         10. D_ENV_OBSD_VER_7_2
         11. D_ENV_OBSD_VER_7_3
         12. D_ENV_OBSD_VER_7_4
         13. D_ENV_OBSD_VER_7_5
         14. D_ENV_OBSD_VER_7_6
         15. D_ENV_OBSD_VER_7_7
    4.  Detected OpenBSD version
         1.  D_ENV_OBSD_VER / D_ENV_OBSD_VER_DETECTED
         2.  D_ENV_OBSD_AT_LEAST
    5.  NetBSD version constants
         1.  D_ENV_NBSD_VER_7
         2.  D_ENV_NBSD_VER_8
         3.  D_ENV_NBSD_VER_9
         4.  D_ENV_NBSD_VER_9_3
         5.  D_ENV_NBSD_VER_10
         6.  D_ENV_NBSD_VER_10_1
    6.  Detected NetBSD version
         1.  D_ENV_NBSD_VER / D_ENV_NBSD_VER_DETECTED
         2.  D_ENV_NBSD_AT_LEAST
    7.  DragonFly BSD version constants
         1.  D_ENV_DBSD_VER_5_8
         2.  D_ENV_DBSD_VER_6_0
         3.  D_ENV_DBSD_VER_6_2
         4.  D_ENV_DBSD_VER_6_4
    8.  Detected DragonFly BSD version
         1.  D_ENV_DBSD_VER / D_ENV_DBSD_VER_DETECTED
         2.  D_ENV_DBSD_AT_LEAST
3.  BSD-COMMON FEATURES
    -------------------
    1.  kqueue event notification
         1.  D_ENV_BSD_HAS_KQUEUE
         2.  D_ENV_BSD_HAS_KQUEUE1
    2.  Random number generation
         1.  D_ENV_BSD_HAS_ARC4RANDOM
         2.  D_ENV_BSD_HAS_ARC4RANDOM_BUF
         3.  D_ENV_BSD_HAS_GETENTROPY
    3.  Memory management
         1.  D_ENV_BSD_HAS_MMAP
         2.  D_ENV_BSD_HAS_MINHERIT
         3.  D_ENV_BSD_HAS_MIMMUTABLE
         4.  D_ENV_BSD_HAS_POSIX_MEMALIGN
         5.  D_ENV_BSD_HAS_REALLOCARRAY
         6.  D_ENV_BSD_HAS_FREEZERO
         7.  D_ENV_BSD_HAS_RECALLOCARRAY
    4.  String and C library extensions
         1.  D_ENV_BSD_HAS_STRLCPY
         2.  D_ENV_BSD_HAS_STRTONUM
         3.  D_ENV_BSD_HAS_EXPLICIT_BZERO
         4.  D_ENV_BSD_HAS_FLOCK
         5.  D_ENV_BSD_HAS_CLOSEFROM
    5.  sysctl
         1.  D_ENV_BSD_HAS_SYSCTL
         2.  D_ENV_BSD_HAS_SYSCTLBYNAME
4.  SECURITY FRAMEWORKS
    -------------------
    1.  OpenBSD pledge and unveil
         1.  D_ENV_BSD_HAS_PLEDGE
         2.  D_ENV_BSD_HAS_UNVEIL
    2.  FreeBSD Capsicum
         1.  D_ENV_BSD_HAS_CAPSICUM
         2.  D_ENV_BSD_HAS_CAP_RIGHTS
         3.  D_ENV_BSD_HAS_CAP_ENTER
    3.  FreeBSD jails
         1.  D_ENV_BSD_HAS_JAIL
    4.  securelevel
         1.  D_ENV_BSD_HAS_SECURELEVEL
    5.  Packet filter (pf)
         1.  D_ENV_BSD_HAS_PF
         2.  D_ENV_BSD_HAS_IPFW
         3.  D_ENV_BSD_HAS_NPF
5.  NETWORKING AND FILESYSTEMS
    --------------------------
    1.  Networking
         1.  D_ENV_BSD_HAS_BPF
         2.  D_ENV_BSD_HAS_ROUTING_SOCKETS
         3.  D_ENV_BSD_HAS_SENDFILE
         4.  D_ENV_BSD_HAS_ACCEPT_FILTER
         5.  D_ENV_BSD_HAS_CARP
         6.  D_ENV_BSD_HAS_NETLINK
         7.  D_ENV_BSD_HAS_UNIX_CMSG
         8.  D_ENV_BSD_HAS_SO_REUSEPORT
    2.  ZFS
         1.  D_ENV_BSD_HAS_ZFS
         2.  D_ENV_BSD_HAS_OPENZFS
    3.  HAMMER / HAMMER2
         1.  D_ENV_BSD_HAS_HAMMER2
    4.  FFS / UFS
         1.  D_ENV_BSD_HAS_FFS
         2.  D_ENV_BSD_HAS_UFS2
         3.  D_ENV_BSD_HAS_SOFTDEP
    5.  General filesystem features
         1.  D_ENV_BSD_HAS_NULLFS
         2.  D_ENV_BSD_HAS_TMPFS
         3.  D_ENV_BSD_HAS_DEVFS
         4.  D_ENV_BSD_HAS_FDESCFS
         5.  D_ENV_BSD_HAS_PROCFS
         6.  D_ENV_BSD_HAS_EXTATTR
6.  PROCESSES AND DESKTOP
    ---------------------
    1.  Process and threading
         1.  D_ENV_BSD_HAS_RFORK
         2.  D_ENV_BSD_HAS_PTHREAD
         3.  D_ENV_BSD_HAS_PTHREAD_NP
         4.  D_ENV_BSD_HAS_KINFO_PROC
         5.  D_ENV_BSD_HAS_KTRACE
         6.  D_ENV_BSD_HAS_DTRACE
         7.  D_ENV_BSD_HAS_CPUSET
    2.  Display server and desktop
         1.  D_ENV_BSD_HAS_X11
         2.  D_ENV_BSD_HAS_WAYLAND
         3.  D_ENV_BSD_HAS_DRM
         4.  D_ENV_BSD_HAS_WSCONS
7.  HEADERS AND TOOLCHAIN
    ---------------------
    1.  BSD-specific headers
         1.  D_ENV_BSD_HAS_SYS_EVENT_H
         2.  D_ENV_BSD_HAS_SYS_SYSCTL_H
         3.  D_ENV_BSD_HAS_SYS_MOUNT_H
         4.  D_ENV_BSD_HAS_SYS_CAPSICUM_H
         5.  D_ENV_BSD_HAS_SYS_JAIL_H
         6.  D_ENV_BSD_HAS_SYS_PTRACE_H
         7.  D_ENV_BSD_HAS_SYS_TREE_H
         8.  D_ENV_BSD_HAS_SYS_QUEUE_H
         9.  D_ENV_BSD_HAS_SYS_ENDIAN_H
         10. D_ENV_BSD_HAS_DLFCN_H
         11. D_ENV_BSD_HAS_LIBUTIL_H
         12. D_ENV_BSD_HAS_NET_BPF_H
         13. D_ENV_BSD_HAS_NET_ROUTE_H
         14. D_ENV_BSD_HAS_NET_IF_DL_H
    2.  Compiler and toolchain
         1.  D_ENV_BSD_DEFAULT_COMPILER
         2.  D_ENV_BSD_HAS_PORTS
         3.  D_ENV_BSD_HAS_PKGSRC
8.  RUNTIME DETECTION AND CONVENIENCE
    ---------------------------------
    1.  Runtime queries
    2.  Combined predicates
         1.  D_ENV_BSD_IS_ANY
         2.  D_ENV_BSD_IS_MODERN
         3.  D_ENV_BSD_HAS_STRONG_SANDBOXING
         4.  D_ENV_BSD_HAS_SECURE_RANDOM
         5.  D_ENV_BSD_HAS_SAFE_STRING
         6.  D_ENV_BSD_HAS_FULL_EVENT_SYSTEM
*/

#ifndef DJINTERP_ENV_OS_ENV_BSD_H
#define DJINTERP_ENV_OS_ENV_BSD_H 1

// djinterp
#include "../env.h"  // D_ENV_OS_ID, D_ENV_OS_FLAG_BSD_*, D_ENV_LANG_USING_CPP


//==============================================================================
// 1.  VARIANT IDENTIFICATION
//==============================================================================


// 1.1    Variant detection
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_BSD_IS_FREEBSD
//   feature: detect if building on FreeBSD.
#if ( defined(__FreeBSD__)  ||                                                 \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_FREE) )
    #define D_ENV_BSD_IS_FREEBSD        1
#else
    #define D_ENV_BSD_IS_FREEBSD        0
#endif

// 1.1.2
// D_ENV_BSD_IS_OPENBSD
//   feature: detect if building on OpenBSD.
#if ( defined(__OpenBSD__)  ||                                                 \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_OPEN) )
    #define D_ENV_BSD_IS_OPENBSD        1
#else
    #define D_ENV_BSD_IS_OPENBSD        0
#endif

// 1.1.3
// D_ENV_BSD_IS_NETBSD
//   feature: detect if building on NetBSD.
#if ( defined(__NetBSD__)  ||                                                  \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_NET) )
    #define D_ENV_BSD_IS_NETBSD         1
#else
    #define D_ENV_BSD_IS_NETBSD         0
#endif

// 1.1.4
// D_ENV_BSD_IS_DRAGONFLY
//   feature: detect if building on DragonFly BSD.
#if ( defined(__DragonFly__)  ||                                               \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_DRAGONFLY) )
    #define D_ENV_BSD_IS_DRAGONFLY      1
#else
    #define D_ENV_BSD_IS_DRAGONFLY      0
#endif

// 1.1.5
// D_ENV_BSD_IS_BSDOS
//   feature: detect if building on BSD/OS (historical).
#if ( defined(__bsdi__)  ||                                                    \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_OS) )
    #define D_ENV_BSD_IS_BSDOS          1
#else
    #define D_ENV_BSD_IS_BSDOS          0
#endif

// 1.2    Variant name
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_BSD_VARIANT_NAME
//   constant: the detected variant's display name, or "BSD (Unknown)".
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_VARIANT_NAME      "FreeBSD"
#elif D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_VARIANT_NAME      "OpenBSD"
#elif D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_VARIANT_NAME      "NetBSD"
#elif D_ENV_BSD_IS_DRAGONFLY
    #define D_ENV_BSD_VARIANT_NAME      "DragonFly BSD"
#elif D_ENV_BSD_IS_BSDOS
    #define D_ENV_BSD_VARIANT_NAME      "BSD/OS"
#else
    #define D_ENV_BSD_VARIANT_NAME      "BSD (Unknown)"
#endif


//==============================================================================
// 2.  VARIANT VERSIONS
//==============================================================================
// Each variant encodes its version differently, and each version comes from
// <sys/param.h>, which this header does not include. FreeBSD alone falls back
// to the compiler's __FreeBSD__ major version; for the others, unless
// <sys/param.h> was included first, the version reads 0 and each *_AT_LEAST
// gate as not met. Most feature flags assume availability when the version is
// unknown; a few, NetBSD's especially, read 0 instead.


// 2.1    FreeBSD version constants
//------------------------------------------------------------------------------
//   __FreeBSD_version encodes MMNNNPP: MM major, NNN minor or release, PP
// patch. FreeBSD 14.0-RELEASE is 1400000.
// 2.1.1
// D_ENV_FBSD_VER_9
//   constant: __FreeBSD_version for FreeBSD 9.0-RELEASE.
#define D_ENV_FBSD_VER_9                900000

// 2.1.2
// D_ENV_FBSD_VER_10
//   constant: __FreeBSD_version for FreeBSD 10.0-RELEASE.
#define D_ENV_FBSD_VER_10              1000000

// 2.1.3
// D_ENV_FBSD_VER_10_3
//   constant: __FreeBSD_version for FreeBSD 10.3-RELEASE.
#define D_ENV_FBSD_VER_10_3           1003000

// 2.1.4
// D_ENV_FBSD_VER_11
//   constant: __FreeBSD_version for FreeBSD 11.0-RELEASE.
#define D_ENV_FBSD_VER_11             1100000

// 2.1.5
// D_ENV_FBSD_VER_11_1
//   constant: __FreeBSD_version for FreeBSD 11.1-RELEASE.
#define D_ENV_FBSD_VER_11_1           1101000

// 2.1.6
// D_ENV_FBSD_VER_12
//   constant: __FreeBSD_version for FreeBSD 12.0-RELEASE.
#define D_ENV_FBSD_VER_12             1200000

// 2.1.7
// D_ENV_FBSD_VER_12_2
//   constant: __FreeBSD_version for FreeBSD 12.2-RELEASE.
#define D_ENV_FBSD_VER_12_2           1202000

// 2.1.8
// D_ENV_FBSD_VER_13
//   constant: __FreeBSD_version for FreeBSD 13.0-RELEASE
// (WireGuard, in-kernel TLS, AArch64 tier 1).
#define D_ENV_FBSD_VER_13             1300000

// 2.1.9
// D_ENV_FBSD_VER_13_1
//   constant: __FreeBSD_version for FreeBSD 13.1-RELEASE.
#define D_ENV_FBSD_VER_13_1           1301000

// 2.1.10
// D_ENV_FBSD_VER_13_2
//   constant: __FreeBSD_version for FreeBSD 13.2-RELEASE.
#define D_ENV_FBSD_VER_13_2           1302000

// 2.1.11
// D_ENV_FBSD_VER_13_3
//   constant: __FreeBSD_version for FreeBSD 13.3-RELEASE.
#define D_ENV_FBSD_VER_13_3           1303000

// 2.1.12
// D_ENV_FBSD_VER_14
//   constant: __FreeBSD_version for FreeBSD 14.0-RELEASE
// (OpenZFS 2.2, 64-bit inode numbers, netlink support).
#define D_ENV_FBSD_VER_14             1400000

// 2.1.13
// D_ENV_FBSD_VER_14_1
//   constant: __FreeBSD_version for FreeBSD 14.1-RELEASE.
#define D_ENV_FBSD_VER_14_1           1401000

// 2.1.14
// D_ENV_FBSD_VER_14_2
//   constant: __FreeBSD_version for FreeBSD 14.2-RELEASE.
#define D_ENV_FBSD_VER_14_2           1402000

// 2.1.15
// D_ENV_FBSD_VER_15
//   constant: __FreeBSD_version for FreeBSD 15.0 (development).
#define D_ENV_FBSD_VER_15             1500000

// 2.2    Detected FreeBSD version
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_FBSD_VER / D_ENV_FBSD_VER_DETECTED
//   constant: __FreeBSD_version, or the compiler's __FreeBSD__ major version
// scaled to match when <sys/param.h> was not included, with
// D_ENV_FBSD_VER_DETECTED set to 1 when either is known.
#if D_ENV_BSD_IS_FREEBSD
    #ifdef __FreeBSD_version
        #define D_ENV_FBSD_VER          __FreeBSD_version
        #define D_ENV_FBSD_VER_DETECTED 1
    #elif defined(__FreeBSD__)
        // __FreeBSD__ is the major version number
        #define D_ENV_FBSD_VER          (__FreeBSD__ * 100000)
        #define D_ENV_FBSD_VER_DETECTED 1
    #else
        #define D_ENV_FBSD_VER          0
        #define D_ENV_FBSD_VER_DETECTED 0
    #endif  // __FreeBSD_version

    // D_ENV_FBSD_MAJOR
    //   constant: FreeBSD major version number.
    #ifdef __FreeBSD__
        #define D_ENV_FBSD_MAJOR        __FreeBSD__
    #else
        #define D_ENV_FBSD_MAJOR        (D_ENV_FBSD_VER / 100000)
    #endif  // __FreeBSD__

#else
    #define D_ENV_FBSD_VER              0
    #define D_ENV_FBSD_VER_DETECTED     0
    #define D_ENV_FBSD_MAJOR            0
#endif

// 2.2.2
// D_ENV_FBSD_AT_LEAST
//   macro: evaluates to 1 if the detected FreeBSD version is at least
// the specified __FreeBSD_version constant.
#define D_ENV_FBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_FREEBSD &&                                                  \
      D_ENV_FBSD_VER_DETECTED &&                                               \
      (D_ENV_FBSD_VER >= (version)) )

// 2.3    OpenBSD version constants
//------------------------------------------------------------------------------
//   <sys/param.h> defines OpenBSD as the release's YYYYMM; OpenBSD 7.5,
// released April 2024, is 202404.
// 2.3.1
// D_ENV_OBSD_VER_5_5
//   constant: OpenBSD version for 5.5 (May 2014).
#define D_ENV_OBSD_VER_5_5             201405

// 2.3.2
// D_ENV_OBSD_VER_5_7
//   constant: OpenBSD version for 5.7 (May 2015, signify).
#define D_ENV_OBSD_VER_5_7             201505

// 2.3.3
// D_ENV_OBSD_VER_5_9
//   constant: OpenBSD version for 5.9 (March 2016, pledge introduced).
#define D_ENV_OBSD_VER_5_9             201603

// 2.3.4
// D_ENV_OBSD_VER_6_0
//   constant: OpenBSD version for 6.0 (September 2016).
#define D_ENV_OBSD_VER_6_0             201609

// 2.3.5
// D_ENV_OBSD_VER_6_4
//   constant: OpenBSD version for 6.4 (October 2018, unveil introduced).
#define D_ENV_OBSD_VER_6_4             201810

// 2.3.6
// D_ENV_OBSD_VER_6_5
//   constant: OpenBSD version for 6.5 (April 2019, sysctl kern.video).
#define D_ENV_OBSD_VER_6_5             201905

// 2.3.7
// D_ENV_OBSD_VER_6_8
//   constant: OpenBSD version for 6.8 (October 2020).
#define D_ENV_OBSD_VER_6_8             202010

// 2.3.8
// D_ENV_OBSD_VER_7_0
//   constant: OpenBSD version for 7.0 (October 2021).
#define D_ENV_OBSD_VER_7_0             202110

// 2.3.9
// D_ENV_OBSD_VER_7_1
//   constant: OpenBSD version for 7.1 (April 2022).
#define D_ENV_OBSD_VER_7_1             202204

// 2.3.10
// D_ENV_OBSD_VER_7_2
//   constant: OpenBSD version for 7.2 (October 2022).
#define D_ENV_OBSD_VER_7_2             202210

// 2.3.11
// D_ENV_OBSD_VER_7_3
//   constant: OpenBSD version for 7.3 (April 2023).
#define D_ENV_OBSD_VER_7_3             202304

// 2.3.12
// D_ENV_OBSD_VER_7_4
//   constant: OpenBSD version for 7.4 (October 2023).
#define D_ENV_OBSD_VER_7_4             202310

// 2.3.13
// D_ENV_OBSD_VER_7_5
//   constant: OpenBSD version for 7.5 (April 2024).
#define D_ENV_OBSD_VER_7_5             202404

// 2.3.14
// D_ENV_OBSD_VER_7_6
//   constant: OpenBSD version for 7.6 (October 2024).
#define D_ENV_OBSD_VER_7_6             202410

// 2.3.15
// D_ENV_OBSD_VER_7_7
//   constant: OpenBSD version for 7.7 (April 2025).
#define D_ENV_OBSD_VER_7_7             202504

// 2.4    Detected OpenBSD version
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_OBSD_VER / D_ENV_OBSD_VER_DETECTED
//   constant: <sys/param.h>'s OpenBSD, with D_ENV_OBSD_VER_DETECTED set to 1
// when it is known.
#if D_ENV_BSD_IS_OPENBSD
    #ifdef OpenBSD
        #define D_ENV_OBSD_VER          OpenBSD
        #define D_ENV_OBSD_VER_DETECTED 1
    #else
        #define D_ENV_OBSD_VER          0
        #define D_ENV_OBSD_VER_DETECTED 0
    #endif  // OpenBSD
#else
    #define D_ENV_OBSD_VER              0
    #define D_ENV_OBSD_VER_DETECTED     0
#endif

// 2.4.2
// D_ENV_OBSD_AT_LEAST
//   macro: evaluates to 1 if the detected OpenBSD version is at least
// the specified YYYYMM version constant.
#define D_ENV_OBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_OPENBSD &&                                                  \
      D_ENV_OBSD_VER_DETECTED &&                                               \
      (D_ENV_OBSD_VER >= (version)) )

// 2.5    NetBSD version constants
//------------------------------------------------------------------------------
//   __NetBSD_Version__ encodes MMmmrrpp00: MM major, mm minor, rr release,
// pp patch. NetBSD 10.0.0 is 1000000000.
// 2.5.1
// D_ENV_NBSD_VER_7
//   constant: __NetBSD_Version__ for NetBSD 7.0.
#define D_ENV_NBSD_VER_7               700000000

// 2.5.2
// D_ENV_NBSD_VER_8
//   constant: __NetBSD_Version__ for NetBSD 8.0.
#define D_ENV_NBSD_VER_8               800000000

// 2.5.3
// D_ENV_NBSD_VER_9
//   constant: __NetBSD_Version__ for NetBSD 9.0 (ZFS, improved audio).
#define D_ENV_NBSD_VER_9               900000000

// 2.5.4
// D_ENV_NBSD_VER_9_3
//   constant: __NetBSD_Version__ for NetBSD 9.3.
#define D_ENV_NBSD_VER_9_3             903000000

// 2.5.5
// D_ENV_NBSD_VER_10
//   constant: __NetBSD_Version__ for NetBSD 10.0 (WireGuard, compat_90,
// reworked audio, significant performance improvements).
#define D_ENV_NBSD_VER_10             1000000000

// 2.5.6
// D_ENV_NBSD_VER_10_1
//   constant: __NetBSD_Version__ for NetBSD 10.1.
#define D_ENV_NBSD_VER_10_1           1000100000

// 2.6    Detected NetBSD version
//------------------------------------------------------------------------------
// 2.6.1
// D_ENV_NBSD_VER / D_ENV_NBSD_VER_DETECTED
//   constant: __NetBSD_Version__, with D_ENV_NBSD_VER_DETECTED set to 1 when
// it is known.
#if D_ENV_BSD_IS_NETBSD
    #ifdef __NetBSD_Version__
        #define D_ENV_NBSD_VER          __NetBSD_Version__
        #define D_ENV_NBSD_VER_DETECTED 1
    #else
        #define D_ENV_NBSD_VER          0
        #define D_ENV_NBSD_VER_DETECTED 0
    #endif  // __NetBSD_Version__
#else
    #define D_ENV_NBSD_VER              0
    #define D_ENV_NBSD_VER_DETECTED     0
#endif

// 2.6.2
// D_ENV_NBSD_AT_LEAST
//   macro: evaluates to 1 if the detected NetBSD version is at least
// the specified __NetBSD_Version__ constant.
#define D_ENV_NBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_NETBSD &&                                                   \
      D_ENV_NBSD_VER_DETECTED &&                                               \
      (D_ENV_NBSD_VER >= (version)) )

// 2.7    DragonFly BSD version constants
//------------------------------------------------------------------------------
//   __DragonFly_version encodes MMmmPP: MM major, mm minor, PP patch.
// DragonFly 6.4 is 600400.
// 2.7.1
// D_ENV_DBSD_VER_5_8
//   constant: __DragonFly_version for DragonFly 5.8.
#define D_ENV_DBSD_VER_5_8             500800

// 2.7.2
// D_ENV_DBSD_VER_6_0
//   constant: __DragonFly_version for DragonFly 6.0 (MAP_VPAGETABLE
// removal, DRM updates).
#define D_ENV_DBSD_VER_6_0             600000

// 2.7.3
// D_ENV_DBSD_VER_6_2
//   constant: __DragonFly_version for DragonFly 6.2.
#define D_ENV_DBSD_VER_6_2             600200

// 2.7.4
// D_ENV_DBSD_VER_6_4
//   constant: __DragonFly_version for DragonFly 6.4.
#define D_ENV_DBSD_VER_6_4             600400

// 2.8    Detected DragonFly BSD version
//------------------------------------------------------------------------------
// 2.8.1
// D_ENV_DBSD_VER / D_ENV_DBSD_VER_DETECTED
//   constant: __DragonFly_version, with D_ENV_DBSD_VER_DETECTED set to 1
// when it is known.
#if D_ENV_BSD_IS_DRAGONFLY
    #ifdef __DragonFly_version
        #define D_ENV_DBSD_VER          __DragonFly_version
        #define D_ENV_DBSD_VER_DETECTED 1
    #else
        #define D_ENV_DBSD_VER          0
        #define D_ENV_DBSD_VER_DETECTED 0
    #endif  // __DragonFly_version
#else
    #define D_ENV_DBSD_VER              0
    #define D_ENV_DBSD_VER_DETECTED     0
#endif

// 2.8.2
// D_ENV_DBSD_AT_LEAST
//   macro: evaluates to 1 if the detected DragonFly version is at least
// the specified __DragonFly_version constant.
#define D_ENV_DBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_DRAGONFLY &&                                                \
      D_ENV_DBSD_VER_DETECTED &&                                               \
      (D_ENV_DBSD_VER >= (version)) )


//==============================================================================
// 3.  BSD-COMMON FEATURES
//==============================================================================
// Features present across all, or most, BSD variants.


// 3.1    kqueue event notification
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_BSD_HAS_KQUEUE
//   feature: detect if kqueue/kevent is available.
// kqueue is present on all modern BSDs. it is the BSD equivalent of
// Linux's epoll.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_KQUEUE        1
#else
    #define D_ENV_BSD_HAS_KQUEUE        0
#endif

// 3.1.2
// D_ENV_BSD_HAS_KQUEUE1
//   feature: detect if kqueue1() (flags argument variant) is available.
// NetBSD introduced kqueue1(); other BSDs use kqueue() + fcntl.
#if D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_HAS_KQUEUE1       1
#else
    #define D_ENV_BSD_HAS_KQUEUE1       0
#endif

// 3.2    Random number generation
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_BSD_HAS_ARC4RANDOM
//   feature: detect if arc4random() is available.
// present on all modern BSDs; provides cryptographically secure random.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_ARC4RANDOM    1
#else
    #define D_ENV_BSD_HAS_ARC4RANDOM    0
#endif

// 3.2.2
// D_ENV_BSD_HAS_ARC4RANDOM_BUF
//   feature: detect if arc4random_buf() (fill buffer variant) is
// available. present on all modern BSDs.
#define D_ENV_BSD_HAS_ARC4RANDOM_BUF    D_ENV_BSD_HAS_ARC4RANDOM

// 3.2.3
// D_ENV_BSD_HAS_GETENTROPY
//   feature: detect if getentropy() is available.
// OpenBSD 5.6+, FreeBSD 12+, NetBSD 10+.
#if D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_HAS_GETENTROPY    1
#elif D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_12)
    #define D_ENV_BSD_HAS_GETENTROPY    1
#elif D_ENV_NBSD_AT_LEAST(D_ENV_NBSD_VER_10)
    #define D_ENV_BSD_HAS_GETENTROPY    1
#else
    #define D_ENV_BSD_HAS_GETENTROPY    0
#endif

// 3.3    Memory management
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_BSD_HAS_MMAP
//   feature: detect if mmap() is available. universal on all BSDs.
#define D_ENV_BSD_HAS_MMAP              1

// 3.3.2
// D_ENV_BSD_HAS_MINHERIT
//   feature: detect if minherit() (mmap inheritance control) is
// available. present on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_MINHERIT      1
#else
    #define D_ENV_BSD_HAS_MINHERIT      0
#endif

// 3.3.3
// D_ENV_BSD_HAS_MIMMUTABLE
//   feature: detect if mimmutable() (make mapping immutable) is
// available. OpenBSD 7.3+ only.
#if D_ENV_OBSD_AT_LEAST(D_ENV_OBSD_VER_7_3)
    #define D_ENV_BSD_HAS_MIMMUTABLE    1
#else
    #define D_ENV_BSD_HAS_MIMMUTABLE    0
#endif

// 3.3.4
// D_ENV_BSD_HAS_POSIX_MEMALIGN
//   feature: detect if posix_memalign() is available.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_POSIX_MEMALIGN 1
#else
    #define D_ENV_BSD_HAS_POSIX_MEMALIGN 0
#endif

// 3.3.5
// D_ENV_BSD_HAS_REALLOCARRAY
//   feature: detect if reallocarray() (overflow-safe realloc) is
// available. originated in OpenBSD; now in FreeBSD 11+, NetBSD 8+.
#if D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_HAS_REALLOCARRAY  1
#elif D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_11)
    #define D_ENV_BSD_HAS_REALLOCARRAY  1
#elif D_ENV_NBSD_AT_LEAST(D_ENV_NBSD_VER_8)
    #define D_ENV_BSD_HAS_REALLOCARRAY  1
#elif D_ENV_BSD_IS_DRAGONFLY
    #define D_ENV_BSD_HAS_REALLOCARRAY  1
#else
    #define D_ENV_BSD_HAS_REALLOCARRAY  0
#endif

// 3.3.6
// D_ENV_BSD_HAS_FREEZERO
//   feature: detect if freezero() (zero-and-free) is available.
// OpenBSD 6.2+, FreeBSD 12+.
#if D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_HAS_FREEZERO      1
#elif D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_12)
    #define D_ENV_BSD_HAS_FREEZERO      1
#else
    #define D_ENV_BSD_HAS_FREEZERO      0
#endif

// 3.3.7
// D_ENV_BSD_HAS_RECALLOCARRAY
//   feature: detect if recallocarray() (overflow-safe, zeroing realloc)
// is available. OpenBSD 6.1+.
#if D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_HAS_RECALLOCARRAY 1
#else
    #define D_ENV_BSD_HAS_RECALLOCARRAY 0
#endif

// 3.4    String and C library extensions
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_BSD_HAS_STRLCPY
//   feature: detect if strlcpy() / strlcat() are available.
// originated in OpenBSD; present on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_STRLCPY       1
    #define D_ENV_BSD_HAS_STRLCAT       1
#else
    #define D_ENV_BSD_HAS_STRLCPY       0
    #define D_ENV_BSD_HAS_STRLCAT       0
#endif

// 3.4.2
// D_ENV_BSD_HAS_STRTONUM
//   feature: detect if strtonum() (safe string-to-number) is available.
// originated in OpenBSD; present on FreeBSD 6+, OpenBSD 3.6+.
#if ( D_ENV_BSD_IS_OPENBSD ||                                                 \
      D_ENV_BSD_IS_FREEBSD )
    #define D_ENV_BSD_HAS_STRTONUM      1
#else
    #define D_ENV_BSD_HAS_STRTONUM      0
#endif

// 3.4.3
// D_ENV_BSD_HAS_EXPLICIT_BZERO
//   feature: detect if explicit_bzero() is available.
// OpenBSD 5.5+, FreeBSD 11+, NetBSD 7.2+.
#if D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_HAS_EXPLICIT_BZERO 1
#elif D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_11)
    #define D_ENV_BSD_HAS_EXPLICIT_BZERO 1
#elif D_ENV_NBSD_AT_LEAST(D_ENV_NBSD_VER_7)
    #define D_ENV_BSD_HAS_EXPLICIT_BZERO 1
#elif D_ENV_BSD_IS_DRAGONFLY
    #define D_ENV_BSD_HAS_EXPLICIT_BZERO 1
#else
    #define D_ENV_BSD_HAS_EXPLICIT_BZERO 0
#endif

// 3.4.4
// D_ENV_BSD_HAS_FLOCK
//   feature: detect if flock() is available. universal on BSD.
#define D_ENV_BSD_HAS_FLOCK             1

// 3.4.5
// D_ENV_BSD_HAS_CLOSEFROM
//   feature: detect if closefrom() is available.
// OpenBSD 3.5+, FreeBSD 8+, NetBSD 3+, DragonFly.
#if ( D_ENV_BSD_IS_OPENBSD   ||                                               \
      D_ENV_BSD_IS_FREEBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_CLOSEFROM     1
#else
    #define D_ENV_BSD_HAS_CLOSEFROM     0
#endif

// 3.5    sysctl
//------------------------------------------------------------------------------
// 3.5.1
// D_ENV_BSD_HAS_SYSCTL
//   feature: detect if sysctl() / sysctlbyname() are available.
// sysctl is the primary kernel parameter interface on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYSCTL        1
#else
    #define D_ENV_BSD_HAS_SYSCTL        0
#endif

// 3.5.2
// D_ENV_BSD_HAS_SYSCTLBYNAME
//   feature: detect if sysctlbyname() (string-based sysctl) is
// available. FreeBSD, DragonFly, NetBSD 6+. not available on OpenBSD.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYSCTLBYNAME  1
#elif D_ENV_NBSD_AT_LEAST(D_ENV_NBSD_VER_7)
    #define D_ENV_BSD_HAS_SYSCTLBYNAME  1
#else
    #define D_ENV_BSD_HAS_SYSCTLBYNAME  0
#endif


//==============================================================================
// 4.  SECURITY FRAMEWORKS
//==============================================================================


// 4.1    OpenBSD pledge and unveil
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_BSD_HAS_PLEDGE
//   feature: detect if pledge() (process promise restriction) is
// available. OpenBSD 5.9+ only.
#if D_ENV_OBSD_AT_LEAST(D_ENV_OBSD_VER_5_9)
    #define D_ENV_BSD_HAS_PLEDGE        1
#elif D_ENV_BSD_IS_OPENBSD
    // OpenBSD detected but version unknown; pledge is likely available
    // on any modern OpenBSD
    #define D_ENV_BSD_HAS_PLEDGE        1
#else
    #define D_ENV_BSD_HAS_PLEDGE        0
#endif

// 4.1.2
// D_ENV_BSD_HAS_UNVEIL
//   feature: detect if unveil() (filesystem visibility restriction) is
// available. OpenBSD 6.4+ only.
#if D_ENV_OBSD_AT_LEAST(D_ENV_OBSD_VER_6_4)
    #define D_ENV_BSD_HAS_UNVEIL        1
#elif ( D_ENV_BSD_IS_OPENBSD &&                                                \
        !D_ENV_OBSD_VER_DETECTED )
    // OpenBSD detected but version unknown; assume modern
    #define D_ENV_BSD_HAS_UNVEIL        1
#else
    #define D_ENV_BSD_HAS_UNVEIL        0
#endif

// 4.2    FreeBSD Capsicum
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_BSD_HAS_CAPSICUM
//   feature: detect if Capsicum (capability-mode sandboxing) is
// available. FreeBSD 10+ only.
#if D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_10)
    #define D_ENV_BSD_HAS_CAPSICUM      1
#elif ( D_ENV_BSD_IS_FREEBSD &&                                                \
        !D_ENV_FBSD_VER_DETECTED )
    // FreeBSD detected but version unknown; assume modern
    #define D_ENV_BSD_HAS_CAPSICUM      1
#else
    #define D_ENV_BSD_HAS_CAPSICUM      0
#endif

// 4.2.2
// D_ENV_BSD_HAS_CAP_RIGHTS
//   feature: detect if cap_rights_limit() is available (Capsicum
// rights). FreeBSD 10+.
#define D_ENV_BSD_HAS_CAP_RIGHTS        D_ENV_BSD_HAS_CAPSICUM

// 4.2.3
// D_ENV_BSD_HAS_CAP_ENTER
//   feature: detect if cap_enter() (enter capability mode) is
// available. FreeBSD 10+.
#define D_ENV_BSD_HAS_CAP_ENTER         D_ENV_BSD_HAS_CAPSICUM

// 4.3    FreeBSD jails
//------------------------------------------------------------------------------
// 4.3.1
// D_ENV_BSD_HAS_JAIL
//   feature: detect if FreeBSD jail API (jail, jail_attach, etc.) is
// available. FreeBSD 4+ (mature since FreeBSD 5.1).
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_JAIL          1
#else
    #define D_ENV_BSD_HAS_JAIL          0
#endif

// 4.4    securelevel
//------------------------------------------------------------------------------
// 4.4.1
// D_ENV_BSD_HAS_SECURELEVEL
//   feature: detect if the kern.securelevel sysctl is available.
// present on all BSDs (originated in 4.4BSD).
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SECURELEVEL   1
#else
    #define D_ENV_BSD_HAS_SECURELEVEL   0
#endif

// 4.5    Packet filter (pf)
//------------------------------------------------------------------------------
// 4.5.1
// D_ENV_BSD_HAS_PF
//   feature: detect if pf (packet filter) is available.
// originated in OpenBSD 3.0; ported to FreeBSD, NetBSD, DragonFly.
#if ( D_ENV_BSD_IS_OPENBSD   ||                                               \
      D_ENV_BSD_IS_FREEBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_PF            1
#else
    #define D_ENV_BSD_HAS_PF            0
#endif

// 4.5.2
// D_ENV_BSD_HAS_IPFW
//   feature: detect if IPFW (IP Firewall) is available.
// FreeBSD and DragonFly only.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_IPFW          1
#else
    #define D_ENV_BSD_HAS_IPFW          0
#endif

// 4.5.3
// D_ENV_BSD_HAS_NPF
//   feature: detect if NPF (NetBSD Packet Filter) is available.
// NetBSD 6+ only.
#if D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_HAS_NPF           1
#else
    #define D_ENV_BSD_HAS_NPF           0
#endif


//==============================================================================
// 5.  NETWORKING AND FILESYSTEMS
//==============================================================================


// 5.1    Networking
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_BSD_HAS_BPF
//   feature: detect if BPF (Berkeley Packet Filter) device is
// available. present on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_BPF           1
#else
    #define D_ENV_BSD_HAS_BPF           0
#endif

// 5.1.2
// D_ENV_BSD_HAS_ROUTING_SOCKETS
//   feature: detect if routing sockets (PF_ROUTE) are available.
// present on all BSDs.
#define D_ENV_BSD_HAS_ROUTING_SOCKETS   D_ENV_BSD_HAS_BPF

// 5.1.3
// D_ENV_BSD_HAS_SENDFILE
//   feature: detect if sendfile() is available.
// note: BSD sendfile differs from Linux sendfile in signature and
// semantics. FreeBSD, DragonFly have it. OpenBSD and NetBSD do not.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SENDFILE      1
#else
    #define D_ENV_BSD_HAS_SENDFILE      0
#endif

// 5.1.4
// D_ENV_BSD_HAS_ACCEPT_FILTER
//   feature: detect if accept filters (SO_ACCEPTFILTER) are available.
// FreeBSD and DragonFly only.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_ACCEPT_FILTER 1
#else
    #define D_ENV_BSD_HAS_ACCEPT_FILTER 0
#endif

// 5.1.5
// D_ENV_BSD_HAS_CARP
//   feature: detect if CARP (Common Address Redundancy Protocol) is
// available. present on OpenBSD, FreeBSD, NetBSD.
#if ( D_ENV_BSD_IS_OPENBSD ||                                                 \
      D_ENV_BSD_IS_FREEBSD ||                                                  \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_CARP          1
#else
    #define D_ENV_BSD_HAS_CARP          0
#endif

// 5.1.6
// D_ENV_BSD_HAS_NETLINK
//   feature: detect if Netlink socket support is available.
// FreeBSD 13+ added Netlink support for routing.
#if D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_14)
    #define D_ENV_BSD_HAS_NETLINK       1
#else
    #define D_ENV_BSD_HAS_NETLINK       0
#endif

// 5.1.7
// D_ENV_BSD_HAS_UNIX_CMSG
//   feature: detect if SCM_RIGHTS / SCM_CREDS credential passing over
// UNIX domain sockets is available. universal on BSD.
#define D_ENV_BSD_HAS_UNIX_CMSG         1

// 5.1.8
// D_ENV_BSD_HAS_SO_REUSEPORT
//   feature: detect if SO_REUSEPORT is available.
// originated in BSD; available on all variants.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SO_REUSEPORT  1
#else
    #define D_ENV_BSD_HAS_SO_REUSEPORT  0
#endif

// 5.2    ZFS
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_BSD_HAS_ZFS
//   feature: detect if ZFS (OpenZFS) is available.
// FreeBSD has ZFS in base since FreeBSD 7; DragonFly has HAMMER instead.
// OpenBSD and NetBSD do not ship ZFS in base.
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_ZFS           1
#else
    #define D_ENV_BSD_HAS_ZFS           0
#endif

// 5.2.2
// D_ENV_BSD_HAS_OPENZFS
//   feature: detect if OpenZFS (modern ZFS fork) is the ZFS variant.
// FreeBSD 13+ uses OpenZFS.
#if D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_13)
    #define D_ENV_BSD_HAS_OPENZFS       1
#else
    #define D_ENV_BSD_HAS_OPENZFS       0
#endif

// 5.3    HAMMER / HAMMER2
//------------------------------------------------------------------------------
// 5.3.1
// D_ENV_BSD_HAS_HAMMER2
//   feature: detect if HAMMER2 filesystem is available.
// DragonFly BSD only (HAMMER2 is the default root filesystem).
#if D_ENV_BSD_IS_DRAGONFLY
    #define D_ENV_BSD_HAS_HAMMER2       1
#else
    #define D_ENV_BSD_HAS_HAMMER2       0
#endif

// 5.4    FFS / UFS
//------------------------------------------------------------------------------
// 5.4.1
// D_ENV_BSD_HAS_FFS
//   feature: detect if FFS (Fast File System / UFS) is available.
// FFS/UFS is the traditional BSD filesystem, present on all variants.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_FFS           1
#else
    #define D_ENV_BSD_HAS_FFS           0
#endif

// 5.4.2
// D_ENV_BSD_HAS_UFS2
//   feature: detect if UFS2 is available (FreeBSD 5+, NetBSD 5+).
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_UFS2          1
#else
    #define D_ENV_BSD_HAS_UFS2          0
#endif

// 5.4.3
// D_ENV_BSD_HAS_SOFTDEP
//   feature: detect if soft-dependency (soft updates) journaling is
// available. FreeBSD and OpenBSD support soft updates on FFS.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_OPENBSD )
    #define D_ENV_BSD_HAS_SOFTDEP       1
#else
    #define D_ENV_BSD_HAS_SOFTDEP       0
#endif

// 5.5    General filesystem features
//------------------------------------------------------------------------------
// 5.5.1
// D_ENV_BSD_HAS_NULLFS
//   feature: detect if nullfs (loopback mount / bind mount) is
// available. present on FreeBSD, NetBSD, DragonFly.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_NULLFS        1
#else
    #define D_ENV_BSD_HAS_NULLFS        0
#endif

// 5.5.2
// D_ENV_BSD_HAS_TMPFS
//   feature: detect if tmpfs (in-memory filesystem) is available.
// present on all modern BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_TMPFS         1
#else
    #define D_ENV_BSD_HAS_TMPFS         0
#endif

// 5.5.3
// D_ENV_BSD_HAS_DEVFS
//   feature: detect if devfs (device filesystem) is available.
// FreeBSD and DragonFly use devfs; OpenBSD and NetBSD do not.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_DEVFS         1
#else
    #define D_ENV_BSD_HAS_DEVFS         0
#endif

// 5.5.4
// D_ENV_BSD_HAS_FDESCFS
//   feature: detect if fdescfs (/dev/fd filesystem) is available.
// FreeBSD, NetBSD.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_FDESCFS       1
#else
    #define D_ENV_BSD_HAS_FDESCFS       0
#endif

// 5.5.5
// D_ENV_BSD_HAS_PROCFS
//   feature: detect if procfs is available.
// FreeBSD and NetBSD have procfs; OpenBSD removed it in 5.7;
// DragonFly has it.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_PROCFS        1
#else
    #define D_ENV_BSD_HAS_PROCFS        0
#endif

// 5.5.6
// D_ENV_BSD_HAS_EXTATTR
//   feature: detect if extended attributes (extattr) are available.
// FreeBSD 5+ and NetBSD 3+ support extended attributes.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_EXTATTR       1
#else
    #define D_ENV_BSD_HAS_EXTATTR       0
#endif


//==============================================================================
// 6.  PROCESSES AND DESKTOP
//==============================================================================


// 6.1    Process and threading
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_BSD_HAS_RFORK
//   feature: detect if rfork() is available.
// FreeBSD and DragonFly support rfork (Plan 9-style process creation).
// note: rfork is largely superseded by pthreads on FreeBSD.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_RFORK         1
#else
    #define D_ENV_BSD_HAS_RFORK         0
#endif

// 6.1.2
// D_ENV_BSD_HAS_PTHREAD
//   feature: detect if POSIX threads are available. universal on BSD.
#define D_ENV_BSD_HAS_PTHREAD           1

// 6.1.3
// D_ENV_BSD_HAS_PTHREAD_NP
//   feature: detect if BSD non-portable pthread extensions are
// available (pthread_set_name_np, pthread_getthreadid_np, etc.).
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_PTHREAD_NP    1
#else
    #define D_ENV_BSD_HAS_PTHREAD_NP    0
#endif

// 6.1.4
// D_ENV_BSD_HAS_KINFO_PROC
//   feature: detect if struct kinfo_proc (process info via sysctl) is
// available. present on all BSDs, but struct layout differs.
#define D_ENV_BSD_HAS_KINFO_PROC        D_ENV_BSD_HAS_SYSCTL

// 6.1.5
// D_ENV_BSD_HAS_KTRACE
//   feature: detect if ktrace (kernel trace) is available.
// present on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_KTRACE        1
#else
    #define D_ENV_BSD_HAS_KTRACE        0
#endif

// 6.1.6
// D_ENV_BSD_HAS_DTRACE
//   feature: detect if DTrace (dynamic tracing) is available.
// FreeBSD 7.1+ and NetBSD 7+ include DTrace.
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_DTRACE        1
#elif D_ENV_NBSD_AT_LEAST(D_ENV_NBSD_VER_7)
    #define D_ENV_BSD_HAS_DTRACE        1
#else
    #define D_ENV_BSD_HAS_DTRACE        0
#endif

// 6.1.7
// D_ENV_BSD_HAS_CPUSET
//   feature: detect if cpuset_setaffinity / cpuset_getaffinity (CPU
// affinity) is available. FreeBSD 7.1+ only.
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_CPUSET        1
#else
    #define D_ENV_BSD_HAS_CPUSET        0
#endif

// 6.2    Display server and desktop
//------------------------------------------------------------------------------
//   as with Linux, compile-time display server detection indicates header
// availability, not the running server.
// 6.2.1
// D_ENV_BSD_HAS_X11
//   feature: detect if X11/Xlib development headers are available.
#if ( defined(_X11_XLIB_H_)  ||                                               \
      defined(_X11_X_H_)     ||                                                \
      defined(_XLIB_H_) )
    #define D_ENV_BSD_HAS_X11           1
#else
    #define D_ENV_BSD_HAS_X11           0
#endif

// 6.2.2
// D_ENV_BSD_HAS_WAYLAND
//   feature: detect if Wayland client headers are available.
// Wayland support on BSDs is newer and less universal than on Linux.
#if ( defined(__wayland_client_h)      ||                                      \
      defined(WAYLAND_CLIENT_H)        ||                                      \
      defined(__wayland_client_core_h) )
    #define D_ENV_BSD_HAS_WAYLAND       1
#else
    #define D_ENV_BSD_HAS_WAYLAND       0
#endif

// 6.2.3
// D_ENV_BSD_HAS_DRM
//   feature: detect if DRM headers are available.
// FreeBSD and DragonFly have DRM/KMS in base; OpenBSD has xenocara;
// NetBSD has DRM in-tree.
#if defined(__DRM_H__)
    #define D_ENV_BSD_HAS_DRM           1
#else
    #define D_ENV_BSD_HAS_DRM           0
#endif

// 6.2.4
// D_ENV_BSD_HAS_WSCONS
//   feature: detect if wscons (workstation console framework) is
// available. NetBSD and OpenBSD only.
#if ( D_ENV_BSD_IS_NETBSD ||                                                  \
      D_ENV_BSD_IS_OPENBSD )
    #define D_ENV_BSD_HAS_WSCONS        1
#else
    #define D_ENV_BSD_HAS_WSCONS        0
#endif


//==============================================================================
// 7.  HEADERS AND TOOLCHAIN
//==============================================================================


// 7.1    BSD-specific headers
//------------------------------------------------------------------------------
// 7.1.1
// D_ENV_BSD_HAS_SYS_EVENT_H
//   feature: sys/event.h (kqueue) is available on all modern BSDs.
#define D_ENV_BSD_HAS_SYS_EVENT_H       D_ENV_BSD_HAS_KQUEUE

// 7.1.2
// D_ENV_BSD_HAS_SYS_SYSCTL_H
//   feature: sys/sysctl.h is available on all BSDs.
#define D_ENV_BSD_HAS_SYS_SYSCTL_H      D_ENV_BSD_HAS_SYSCTL

// 7.1.3
// D_ENV_BSD_HAS_SYS_MOUNT_H
//   feature: sys/mount.h (mount/unmount/statfs) is available.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYS_MOUNT_H   1
#else
    #define D_ENV_BSD_HAS_SYS_MOUNT_H   0
#endif

// 7.1.4
// D_ENV_BSD_HAS_SYS_CAPSICUM_H
//   feature: sys/capsicum.h is available on FreeBSD 10+.
#define D_ENV_BSD_HAS_SYS_CAPSICUM_H    D_ENV_BSD_HAS_CAPSICUM

// 7.1.5
// D_ENV_BSD_HAS_SYS_JAIL_H
//   feature: sys/jail.h is available on FreeBSD.
#define D_ENV_BSD_HAS_SYS_JAIL_H        D_ENV_BSD_HAS_JAIL

// 7.1.6
// D_ENV_BSD_HAS_SYS_PTRACE_H
//   feature: sys/ptrace.h is available on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYS_PTRACE_H  1
#else
    #define D_ENV_BSD_HAS_SYS_PTRACE_H  0
#endif

// 7.1.7
// D_ENV_BSD_HAS_SYS_TREE_H
//   feature: sys/tree.h (red-black tree / splay tree macros) is
// available. present on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYS_TREE_H    1
#else
    #define D_ENV_BSD_HAS_SYS_TREE_H    0
#endif

// 7.1.8
// D_ENV_BSD_HAS_SYS_QUEUE_H
//   feature: sys/queue.h (TAILQ, LIST, SLIST macros) is available.
// originated in 4.4BSD; present on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYS_QUEUE_H   1
#else
    #define D_ENV_BSD_HAS_SYS_QUEUE_H   0
#endif

// 7.1.9
// D_ENV_BSD_HAS_SYS_ENDIAN_H
//   feature: sys/endian.h (byte-order macros: be16toh, le32toh, etc.)
// is available. present on all modern BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_SYS_ENDIAN_H  1
#else
    #define D_ENV_BSD_HAS_SYS_ENDIAN_H  0
#endif

// 7.1.10
// D_ENV_BSD_HAS_DLFCN_H
//   feature: dlfcn.h (dynamic loading) is available on all modern BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_DLFCN_H       1
#else
    #define D_ENV_BSD_HAS_DLFCN_H       0
#endif

// 7.1.11
// D_ENV_BSD_HAS_LIBUTIL_H
//   feature: detect if libutil.h (login_cap, openpty, etc.) is
// available. FreeBSD and DragonFly use libutil.h; OpenBSD and NetBSD
// use util.h.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_LIBUTIL_H     1
    #define D_ENV_BSD_HAS_UTIL_H        0
#elif ( D_ENV_BSD_IS_OPENBSD ||                                                \
        D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_LIBUTIL_H     0
    #define D_ENV_BSD_HAS_UTIL_H        1
#else
    #define D_ENV_BSD_HAS_LIBUTIL_H     0
    #define D_ENV_BSD_HAS_UTIL_H        0
#endif

// 7.1.12
// D_ENV_BSD_HAS_NET_BPF_H
//   feature: net/bpf.h (Berkeley Packet Filter) is available.
#define D_ENV_BSD_HAS_NET_BPF_H         D_ENV_BSD_HAS_BPF

// 7.1.13
// D_ENV_BSD_HAS_NET_ROUTE_H
//   feature: net/route.h (routing sockets) is available on all BSDs.
#define D_ENV_BSD_HAS_NET_ROUTE_H       D_ENV_BSD_HAS_ROUTING_SOCKETS

// 7.1.14
// D_ENV_BSD_HAS_NET_IF_DL_H
//   feature: net/if_dl.h (data link address) is available on all BSDs.
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_NET_IF_DL_H   1
#else
    #define D_ENV_BSD_HAS_NET_IF_DL_H   0
#endif

// 7.2    Compiler and toolchain
//------------------------------------------------------------------------------
// 7.2.1
// D_ENV_BSD_DEFAULT_COMPILER
//   feature: identify the default base system compiler.
// FreeBSD 10+ and DragonFly use Clang; OpenBSD uses Clang 13+;
// NetBSD traditionally uses GCC.
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_DEFAULT_COMPILER_NAME "Clang"
    #define D_ENV_BSD_DEFAULT_IS_CLANG  1
    #define D_ENV_BSD_DEFAULT_IS_GCC    0
#elif D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_DEFAULT_COMPILER_NAME "Clang"
    #define D_ENV_BSD_DEFAULT_IS_CLANG  1
    #define D_ENV_BSD_DEFAULT_IS_GCC    0
#elif D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_DEFAULT_COMPILER_NAME "GCC"
    #define D_ENV_BSD_DEFAULT_IS_CLANG  0
    #define D_ENV_BSD_DEFAULT_IS_GCC    1
#elif D_ENV_BSD_IS_DRAGONFLY
    #define D_ENV_BSD_DEFAULT_COMPILER_NAME "GCC"
    #define D_ENV_BSD_DEFAULT_IS_CLANG  0
    #define D_ENV_BSD_DEFAULT_IS_GCC    1
#else
    #define D_ENV_BSD_DEFAULT_COMPILER_NAME "Unknown"
    #define D_ENV_BSD_DEFAULT_IS_CLANG  0
    #define D_ENV_BSD_DEFAULT_IS_GCC    0
#endif

// 7.2.2
// D_ENV_BSD_HAS_PORTS
//   feature: detect if a ports/packages system is expected.
// FreeBSD has ports + pkg; OpenBSD has ports + pkg_add; NetBSD has
// pkgsrc; DragonFly uses dports (based on FreeBSD ports).
#if ( D_ENV_BSD_IS_FREEBSD   ||                                               \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_PORTS         1
#else
    #define D_ENV_BSD_HAS_PORTS         0
#endif

// 7.2.3
// D_ENV_BSD_HAS_PKGSRC
//   feature: detect if pkgsrc is the primary package system.
// NetBSD (and optionally DragonFly).
#if D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_HAS_PKGSRC        1
#else
    #define D_ENV_BSD_HAS_PKGSRC        0
#endif


//==============================================================================
// 8.  RUNTIME DETECTION AND CONVENIENCE
//==============================================================================


// 8.1    Runtime queries
//------------------------------------------------------------------------------
//   C linkage for C++ callers. The env headers sit below djinterp.h, so the
// D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
#if D_ENV_LANG_USING_CPP
    extern "C" {
#endif

/**
 * @brief Returns the running OS's version string.
 *
 * @return the version from uname or sysctl, or "Unknown" on failure.
 */
const char* d_env_bsd_get_version_string(void);
/**
 * @brief Returns the running BSD variant's name.
 *
 * @return "FreeBSD", "OpenBSD", "NetBSD", "DragonFly BSD", or
 *         "BSD (Unknown)".
 */
const char* d_env_bsd_get_variant_name(void);
/**
 * @brief Tests at runtime, through sysctl, whether a feature is present.
 *
 * @param[in] _feature_name  the sysctl MIB name, e.g. "kern.securelevel".
 * @return `1` if the sysctl exists and is readable, `0` otherwise.
 */
int         d_env_bsd_has_feature(const char* _feature_name);
/**
 * @brief Returns the current kern.securelevel value.
 *
 * @return the securelevel, or `-2` on failure.
 */
int         d_env_bsd_get_securelevel(void);
/**
 * @brief Tests at runtime that kqueue works, not merely that it compiles.
 *
 * @return `1` if kqueue() succeeds, `0` otherwise.
 */
int         d_env_bsd_has_kqueue(void);
/**
 * @brief Prints detailed information about the detected BSD environment:
 *        variant, version, security features, filesystem support, and
 *        available APIs.
 */
void        d_env_bsd_print_info(void);

#if D_ENV_LANG_USING_CPP
    }
#endif

// 8.2    Combined predicates
//------------------------------------------------------------------------------
// 8.2.1
// D_ENV_BSD_IS_ANY
//   macro: evaluates to 1 if any recognized BSD variant is detected.
#define D_ENV_BSD_IS_ANY()                                                     \
    ( D_ENV_BSD_IS_FREEBSD   ||                                                \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY ||                                                \
      D_ENV_BSD_IS_BSDOS )

// 8.2.2
// D_ENV_BSD_IS_MODERN
//   macro: evaluates to 1 if the detected BSD is a modern,
// actively-maintained variant with recent releases (excludes BSD/OS).
#define D_ENV_BSD_IS_MODERN()                                                  \
    ( D_ENV_BSD_IS_FREEBSD   ||                                                \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )

// 8.2.3
// D_ENV_BSD_HAS_STRONG_SANDBOXING
//   macro: evaluates to 1 if the BSD variant has a mature, built-in
// application sandboxing mechanism (Capsicum on FreeBSD, pledge/unveil
// on OpenBSD).
#define D_ENV_BSD_HAS_STRONG_SANDBOXING()                                      \
    ( D_ENV_BSD_HAS_CAPSICUM ||                                                \
      ( D_ENV_BSD_HAS_PLEDGE && D_ENV_BSD_HAS_UNVEIL ) )

// 8.2.4
// D_ENV_BSD_HAS_SECURE_RANDOM
//   macro: evaluates to 1 if a strong, non-blocking secure random
// source is available (arc4random or getentropy).
#define D_ENV_BSD_HAS_SECURE_RANDOM()                                          \
    ( D_ENV_BSD_HAS_ARC4RANDOM ||                                              \
      D_ENV_BSD_HAS_GETENTROPY )

// 8.2.5
// D_ENV_BSD_HAS_SAFE_STRING
//   macro: evaluates to 1 if safe string functions (strlcpy, strlcat,
// explicit_bzero) are available.
#define D_ENV_BSD_HAS_SAFE_STRING()                                            \
    ( D_ENV_BSD_HAS_STRLCPY      &&                                            \
      D_ENV_BSD_HAS_EXPLICIT_BZERO )

// 8.2.6
// D_ENV_BSD_HAS_FULL_EVENT_SYSTEM
//   macro: evaluates to 1 if the BSD provides a complete event
// notification system (kqueue is sufficient on BSD — it handles
// sockets, files, processes, signals, and timers).
#define D_ENV_BSD_HAS_FULL_EVENT_SYSTEM()                                      \
    ( D_ENV_BSD_HAS_KQUEUE )


#endif  // DJINTERP_ENV_OS_ENV_BSD_H
