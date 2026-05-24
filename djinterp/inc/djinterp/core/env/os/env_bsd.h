/******************************************************************************
* djinterp [core]                                                  env_bsd.h
*
*   djinterp BSD environment detection header:
* This header provides comprehensive, compile-time detection of the BSD
* compilation environment across all major BSD variants:
*   - FreeBSD (including FreeBSD derivatives: GhostBSD, MidnightBSD, etc.)
*   - OpenBSD
*   - NetBSD
*   - DragonFly BSD
*   - BSD/OS (historical)
*
* scope:
*   - BSD variant identification and version detection
*   - BSD-common features (kqueue, arc4random, pledge, jails, capsicum)
*   - per-variant version constants and version-gated APIs
*   - C library and libc feature availability
*   - security frameworks (Capsicum, pledge/unveil, securelevel, pf)
*   - networking (pf, CARP, routing sockets, BPF, sendfile variants)
*   - filesystem features (ZFS, HAMMER2, FFS/UFS, nullfs, tmpfs)
*   - memory and process management (mmap flags, rfork, kinfo_proc)
*   - hardware and device access (sysctl, devctl, devd)
*   - display server indicators (X11, Wayland)
*   - BSD-specific header availability
*
* usage:
*   Included automatically by env.h when a BSD OS is detected:
*     #if D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)
*         #include ".\core\env\env_bsd.h"
*     #endif
*
* NAMING CONVENTION:
*   D_ENV_BSD_[CATEGORY]_[FEATURE]  - common across BSD variants
*   D_ENV_FBSD_[CATEGORY]_[FEATURE] - FreeBSD-specific
*   D_ENV_OBSD_[CATEGORY]_[FEATURE] - OpenBSD-specific
*   D_ENV_NBSD_[CATEGORY]_[FEATURE] - NetBSD-specific
*   D_ENV_DBSD_[CATEGORY]_[FEATURE] - DragonFly BSD-specific
*
*
* path:      \inc\core\env\env_bsd.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.28
******************************************************************************/

#ifndef DJINTERP_ENV_BSD_
#define DJINTERP_ENV_BSD_ 1

#include "./env.h"


// =============================================================================
// I.   BSD VARIANT IDENTIFICATION
// =============================================================================

// -----------------------------------------------------------------------------
// A.  variant detection
// -----------------------------------------------------------------------------

// D_ENV_BSD_IS_FREEBSD
//   feature: detect if building on FreeBSD.
#if ( defined(__FreeBSD__)  ||                                                 \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_FREE) )
    #define D_ENV_BSD_IS_FREEBSD        1
#else
    #define D_ENV_BSD_IS_FREEBSD        0
#endif

// D_ENV_BSD_IS_OPENBSD
//   feature: detect if building on OpenBSD.
#if ( defined(__OpenBSD__)  ||                                                 \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_OPEN) )
    #define D_ENV_BSD_IS_OPENBSD        1
#else
    #define D_ENV_BSD_IS_OPENBSD        0
#endif

// D_ENV_BSD_IS_NETBSD
//   feature: detect if building on NetBSD.
#if ( defined(__NetBSD__)  ||                                                  \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_NET) )
    #define D_ENV_BSD_IS_NETBSD         1
#else
    #define D_ENV_BSD_IS_NETBSD         0
#endif

// D_ENV_BSD_IS_DRAGONFLY
//   feature: detect if building on DragonFly BSD.
#if ( defined(__DragonFly__)  ||                                               \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_DRAGONFLY) )
    #define D_ENV_BSD_IS_DRAGONFLY      1
#else
    #define D_ENV_BSD_IS_DRAGONFLY      0
#endif

// D_ENV_BSD_IS_BSDOS
//   feature: detect if building on BSD/OS (historical).
#if ( defined(__bsdi__)  ||                                                    \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_OS) )
    #define D_ENV_BSD_IS_BSDOS          1
#else
    #define D_ENV_BSD_IS_BSDOS          0
#endif


// -----------------------------------------------------------------------------
// B.  variant name
// -----------------------------------------------------------------------------

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


// =============================================================================
// II.  FREEBSD VERSION DETECTION
// =============================================================================

// FreeBSD uses __FreeBSD_version (from <sys/param.h>) for feature gating.
// encoding: MMNNNPP where MM=major, NNN=minor/release, PP=patch.
// example: FreeBSD 14.0-RELEASE = 1400000

// -----------------------------------------------------------------------------
// A.  FreeBSD version constants
// -----------------------------------------------------------------------------

// D_ENV_FBSD_VER_9
//   constant: __FreeBSD_version for FreeBSD 9.0-RELEASE.
#define D_ENV_FBSD_VER_9                900000

// D_ENV_FBSD_VER_10
//   constant: __FreeBSD_version for FreeBSD 10.0-RELEASE.
#define D_ENV_FBSD_VER_10              1000000

// D_ENV_FBSD_VER_10_3
//   constant: __FreeBSD_version for FreeBSD 10.3-RELEASE.
#define D_ENV_FBSD_VER_10_3           1003000

// D_ENV_FBSD_VER_11
//   constant: __FreeBSD_version for FreeBSD 11.0-RELEASE.
#define D_ENV_FBSD_VER_11             1100000

// D_ENV_FBSD_VER_11_1
//   constant: __FreeBSD_version for FreeBSD 11.1-RELEASE.
#define D_ENV_FBSD_VER_11_1           1101000

// D_ENV_FBSD_VER_12
//   constant: __FreeBSD_version for FreeBSD 12.0-RELEASE.
#define D_ENV_FBSD_VER_12             1200000

// D_ENV_FBSD_VER_12_2
//   constant: __FreeBSD_version for FreeBSD 12.2-RELEASE.
#define D_ENV_FBSD_VER_12_2           1202000

// D_ENV_FBSD_VER_13
//   constant: __FreeBSD_version for FreeBSD 13.0-RELEASE
// (WireGuard, in-kernel TLS, AArch64 tier 1).
#define D_ENV_FBSD_VER_13             1300000

// D_ENV_FBSD_VER_13_1
//   constant: __FreeBSD_version for FreeBSD 13.1-RELEASE.
#define D_ENV_FBSD_VER_13_1           1301000

// D_ENV_FBSD_VER_13_2
//   constant: __FreeBSD_version for FreeBSD 13.2-RELEASE.
#define D_ENV_FBSD_VER_13_2           1302000

// D_ENV_FBSD_VER_13_3
//   constant: __FreeBSD_version for FreeBSD 13.3-RELEASE.
#define D_ENV_FBSD_VER_13_3           1303000

// D_ENV_FBSD_VER_14
//   constant: __FreeBSD_version for FreeBSD 14.0-RELEASE
// (OpenZFS 2.2, 64-bit inode numbers, netlink support).
#define D_ENV_FBSD_VER_14             1400000

// D_ENV_FBSD_VER_14_1
//   constant: __FreeBSD_version for FreeBSD 14.1-RELEASE.
#define D_ENV_FBSD_VER_14_1           1401000

// D_ENV_FBSD_VER_14_2
//   constant: __FreeBSD_version for FreeBSD 14.2-RELEASE.
#define D_ENV_FBSD_VER_14_2           1402000

// D_ENV_FBSD_VER_15
//   constant: __FreeBSD_version for FreeBSD 15.0 (development).
#define D_ENV_FBSD_VER_15             1500000


// -----------------------------------------------------------------------------
// B.  detected FreeBSD version
// -----------------------------------------------------------------------------

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
    #endif

    // D_ENV_FBSD_MAJOR
    //   constant: FreeBSD major version number.
    #ifdef __FreeBSD__
        #define D_ENV_FBSD_MAJOR        __FreeBSD__
    #else
        #define D_ENV_FBSD_MAJOR        (D_ENV_FBSD_VER / 100000)
    #endif

#else
    #define D_ENV_FBSD_VER              0
    #define D_ENV_FBSD_VER_DETECTED     0
    #define D_ENV_FBSD_MAJOR            0
#endif

// D_ENV_FBSD_AT_LEAST
//   macro: evaluates to 1 if the detected FreeBSD version is at least
// the specified __FreeBSD_version constant.
#define D_ENV_FBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_FREEBSD &&                                                  \
      D_ENV_FBSD_VER_DETECTED &&                                               \
      (D_ENV_FBSD_VER >= (version)) )


// =============================================================================
// III. OPENBSD VERSION DETECTION
// =============================================================================

// OpenBSD uses OpenBSD in <sys/param.h> as YYYYMM.
// example: OpenBSD 7.5 (released April 2024) = 202404

// -----------------------------------------------------------------------------
// A.  OpenBSD version constants
// -----------------------------------------------------------------------------

// D_ENV_OBSD_VER_5_5
//   constant: OpenBSD version for 5.5 (May 2014).
#define D_ENV_OBSD_VER_5_5             201405

// D_ENV_OBSD_VER_5_7
//   constant: OpenBSD version for 5.7 (May 2015, signify).
#define D_ENV_OBSD_VER_5_7             201505

// D_ENV_OBSD_VER_5_9
//   constant: OpenBSD version for 5.9 (March 2016, pledge introduced).
#define D_ENV_OBSD_VER_5_9             201603

// D_ENV_OBSD_VER_6_0
//   constant: OpenBSD version for 6.0 (September 2016).
#define D_ENV_OBSD_VER_6_0             201609

// D_ENV_OBSD_VER_6_4
//   constant: OpenBSD version for 6.4 (October 2018, unveil introduced).
#define D_ENV_OBSD_VER_6_4             201810

// D_ENV_OBSD_VER_6_5
//   constant: OpenBSD version for 6.5 (April 2019, sysctl kern.video).
#define D_ENV_OBSD_VER_6_5             201905

// D_ENV_OBSD_VER_6_8
//   constant: OpenBSD version for 6.8 (October 2020).
#define D_ENV_OBSD_VER_6_8             202010

// D_ENV_OBSD_VER_7_0
//   constant: OpenBSD version for 7.0 (October 2021).
#define D_ENV_OBSD_VER_7_0             202110

// D_ENV_OBSD_VER_7_1
//   constant: OpenBSD version for 7.1 (April 2022).
#define D_ENV_OBSD_VER_7_1             202204

// D_ENV_OBSD_VER_7_2
//   constant: OpenBSD version for 7.2 (October 2022).
#define D_ENV_OBSD_VER_7_2             202210

// D_ENV_OBSD_VER_7_3
//   constant: OpenBSD version for 7.3 (April 2023).
#define D_ENV_OBSD_VER_7_3             202304

// D_ENV_OBSD_VER_7_4
//   constant: OpenBSD version for 7.4 (October 2023).
#define D_ENV_OBSD_VER_7_4             202310

// D_ENV_OBSD_VER_7_5
//   constant: OpenBSD version for 7.5 (April 2024).
#define D_ENV_OBSD_VER_7_5             202404

// D_ENV_OBSD_VER_7_6
//   constant: OpenBSD version for 7.6 (October 2024).
#define D_ENV_OBSD_VER_7_6             202410

// D_ENV_OBSD_VER_7_7
//   constant: OpenBSD version for 7.7 (April 2025).
#define D_ENV_OBSD_VER_7_7             202504


// -----------------------------------------------------------------------------
// B.  detected OpenBSD version
// -----------------------------------------------------------------------------

#if D_ENV_BSD_IS_OPENBSD
    #ifdef OpenBSD
        #define D_ENV_OBSD_VER          OpenBSD
        #define D_ENV_OBSD_VER_DETECTED 1
    #else
        #define D_ENV_OBSD_VER          0
        #define D_ENV_OBSD_VER_DETECTED 0
    #endif
#else
    #define D_ENV_OBSD_VER              0
    #define D_ENV_OBSD_VER_DETECTED     0
#endif

// D_ENV_OBSD_AT_LEAST
//   macro: evaluates to 1 if the detected OpenBSD version is at least
// the specified YYYYMM version constant.
#define D_ENV_OBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_OPENBSD &&                                                  \
      D_ENV_OBSD_VER_DETECTED &&                                               \
      (D_ENV_OBSD_VER >= (version)) )


// =============================================================================
// IV.  NETBSD VERSION DETECTION
// =============================================================================

// NetBSD uses __NetBSD_Version__ (from <sys/param.h>).
// encoding: MMmmrrpp00 where MM=major, mm=minor, rr=release, pp=patch.
// example: NetBSD 10.0.0 = 1000000000

// -----------------------------------------------------------------------------
// A.  NetBSD version constants
// -----------------------------------------------------------------------------

// D_ENV_NBSD_VER_7
//   constant: __NetBSD_Version__ for NetBSD 7.0.
#define D_ENV_NBSD_VER_7               700000000

// D_ENV_NBSD_VER_8
//   constant: __NetBSD_Version__ for NetBSD 8.0.
#define D_ENV_NBSD_VER_8               800000000

// D_ENV_NBSD_VER_9
//   constant: __NetBSD_Version__ for NetBSD 9.0 (ZFS, improved audio).
#define D_ENV_NBSD_VER_9               900000000

// D_ENV_NBSD_VER_9_3
//   constant: __NetBSD_Version__ for NetBSD 9.3.
#define D_ENV_NBSD_VER_9_3             903000000

// D_ENV_NBSD_VER_10
//   constant: __NetBSD_Version__ for NetBSD 10.0 (WireGuard, compat_90,
// reworked audio, significant performance improvements).
#define D_ENV_NBSD_VER_10             1000000000

// D_ENV_NBSD_VER_10_1
//   constant: __NetBSD_Version__ for NetBSD 10.1.
#define D_ENV_NBSD_VER_10_1           1000100000


// -----------------------------------------------------------------------------
// B.  detected NetBSD version
// -----------------------------------------------------------------------------

#if D_ENV_BSD_IS_NETBSD
    #ifdef __NetBSD_Version__
        #define D_ENV_NBSD_VER          __NetBSD_Version__
        #define D_ENV_NBSD_VER_DETECTED 1
    #else
        #define D_ENV_NBSD_VER          0
        #define D_ENV_NBSD_VER_DETECTED 0
    #endif
#else
    #define D_ENV_NBSD_VER              0
    #define D_ENV_NBSD_VER_DETECTED     0
#endif

// D_ENV_NBSD_AT_LEAST
//   macro: evaluates to 1 if the detected NetBSD version is at least
// the specified __NetBSD_Version__ constant.
#define D_ENV_NBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_NETBSD &&                                                   \
      D_ENV_NBSD_VER_DETECTED &&                                               \
      (D_ENV_NBSD_VER >= (version)) )


// =============================================================================
// V.   DRAGONFLY BSD VERSION DETECTION
// =============================================================================

// DragonFly BSD uses __DragonFly_version (from <sys/param.h>).
// encoding: MMmmPP where MM=major, mm=minor, PP=patch.
// example: DragonFly 6.4 = 600400

// -----------------------------------------------------------------------------
// A.  DragonFly BSD version constants
// -----------------------------------------------------------------------------

// D_ENV_DBSD_VER_5_8
//   constant: __DragonFly_version for DragonFly 5.8.
#define D_ENV_DBSD_VER_5_8             500800

// D_ENV_DBSD_VER_6_0
//   constant: __DragonFly_version for DragonFly 6.0 (MAP_VPAGETABLE
// removal, DRM updates).
#define D_ENV_DBSD_VER_6_0             600000

// D_ENV_DBSD_VER_6_2
//   constant: __DragonFly_version for DragonFly 6.2.
#define D_ENV_DBSD_VER_6_2             600200

// D_ENV_DBSD_VER_6_4
//   constant: __DragonFly_version for DragonFly 6.4.
#define D_ENV_DBSD_VER_6_4             600400


// -----------------------------------------------------------------------------
// B.  detected DragonFly BSD version
// -----------------------------------------------------------------------------

#if D_ENV_BSD_IS_DRAGONFLY
    #ifdef __DragonFly_version
        #define D_ENV_DBSD_VER          __DragonFly_version
        #define D_ENV_DBSD_VER_DETECTED 1
    #else
        #define D_ENV_DBSD_VER          0
        #define D_ENV_DBSD_VER_DETECTED 0
    #endif
#else
    #define D_ENV_DBSD_VER              0
    #define D_ENV_DBSD_VER_DETECTED     0
#endif

// D_ENV_DBSD_AT_LEAST
//   macro: evaluates to 1 if the detected DragonFly version is at least
// the specified __DragonFly_version constant.
#define D_ENV_DBSD_AT_LEAST(version)                                           \
    ( D_ENV_BSD_IS_DRAGONFLY &&                                                \
      D_ENV_DBSD_VER_DETECTED &&                                               \
      (D_ENV_DBSD_VER >= (version)) )


// =============================================================================
// VI.  BSD-COMMON FEATURES
// =============================================================================

// features present across all (or most) BSD variants.

// -----------------------------------------------------------------------------
// A.  kqueue event notification
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_KQUEUE1
//   feature: detect if kqueue1() (flags argument variant) is available.
// NetBSD introduced kqueue1(); other BSDs use kqueue() + fcntl.
#if D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_HAS_KQUEUE1       1
#else
    #define D_ENV_BSD_HAS_KQUEUE1       0
#endif


// -----------------------------------------------------------------------------
// B.  random number generation
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_ARC4RANDOM_BUF
//   feature: detect if arc4random_buf() (fill buffer variant) is
// available. present on all modern BSDs.
#define D_ENV_BSD_HAS_ARC4RANDOM_BUF    D_ENV_BSD_HAS_ARC4RANDOM

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


// -----------------------------------------------------------------------------
// C.  memory management
// -----------------------------------------------------------------------------

// D_ENV_BSD_HAS_MMAP
//   feature: detect if mmap() is available. universal on all BSDs.
#define D_ENV_BSD_HAS_MMAP              1

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

// D_ENV_BSD_HAS_MIMMUTABLE
//   feature: detect if mimmutable() (make mapping immutable) is
// available. OpenBSD 7.3+ only.
#if D_ENV_OBSD_AT_LEAST(D_ENV_OBSD_VER_7_3)
    #define D_ENV_BSD_HAS_MIMMUTABLE    1
#else
    #define D_ENV_BSD_HAS_MIMMUTABLE    0
#endif

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

// D_ENV_BSD_HAS_RECALLOCARRAY
//   feature: detect if recallocarray() (overflow-safe, zeroing realloc)
// is available. OpenBSD 6.1+.
#if D_ENV_BSD_IS_OPENBSD
    #define D_ENV_BSD_HAS_RECALLOCARRAY 1
#else
    #define D_ENV_BSD_HAS_RECALLOCARRAY 0
#endif


// -----------------------------------------------------------------------------
// D.  string and C library extensions
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_STRTONUM
//   feature: detect if strtonum() (safe string-to-number) is available.
// originated in OpenBSD; present on FreeBSD 6+, OpenBSD 3.6+.
#if ( D_ENV_BSD_IS_OPENBSD ||                                                 \
      D_ENV_BSD_IS_FREEBSD )
    #define D_ENV_BSD_HAS_STRTONUM      1
#else
    #define D_ENV_BSD_HAS_STRTONUM      0
#endif

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

// D_ENV_BSD_HAS_FLOCK
//   feature: detect if flock() is available. universal on BSD.
#define D_ENV_BSD_HAS_FLOCK             1

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


// -----------------------------------------------------------------------------
// E.  sysctl
// -----------------------------------------------------------------------------

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


// =============================================================================
// VII. SECURITY FRAMEWORKS
// =============================================================================

// -----------------------------------------------------------------------------
// A.  OpenBSD pledge and unveil
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// B.  FreeBSD Capsicum
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_CAP_RIGHTS
//   feature: detect if cap_rights_limit() is available (Capsicum
// rights). FreeBSD 10+.
#define D_ENV_BSD_HAS_CAP_RIGHTS        D_ENV_BSD_HAS_CAPSICUM

// D_ENV_BSD_HAS_CAP_ENTER
//   feature: detect if cap_enter() (enter capability mode) is
// available. FreeBSD 10+.
#define D_ENV_BSD_HAS_CAP_ENTER         D_ENV_BSD_HAS_CAPSICUM


// -----------------------------------------------------------------------------
// C.  FreeBSD jails
// -----------------------------------------------------------------------------

// D_ENV_BSD_HAS_JAIL
//   feature: detect if FreeBSD jail API (jail, jail_attach, etc.) is
// available. FreeBSD 4+ (mature since FreeBSD 5.1).
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_JAIL          1
#else
    #define D_ENV_BSD_HAS_JAIL          0
#endif


// -----------------------------------------------------------------------------
// D.  securelevel
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// E.  packet filter (pf)
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_IPFW
//   feature: detect if IPFW (IP Firewall) is available.
// FreeBSD and DragonFly only.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_IPFW          1
#else
    #define D_ENV_BSD_HAS_IPFW          0
#endif

// D_ENV_BSD_HAS_NPF
//   feature: detect if NPF (NetBSD Packet Filter) is available.
// NetBSD 6+ only.
#if D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_HAS_NPF           1
#else
    #define D_ENV_BSD_HAS_NPF           0
#endif


// =============================================================================
// VIII. NETWORKING
// =============================================================================

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

// D_ENV_BSD_HAS_ROUTING_SOCKETS
//   feature: detect if routing sockets (PF_ROUTE) are available.
// present on all BSDs.
#define D_ENV_BSD_HAS_ROUTING_SOCKETS   D_ENV_BSD_HAS_BPF

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

// D_ENV_BSD_HAS_ACCEPT_FILTER
//   feature: detect if accept filters (SO_ACCEPTFILTER) are available.
// FreeBSD and DragonFly only.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_ACCEPT_FILTER 1
#else
    #define D_ENV_BSD_HAS_ACCEPT_FILTER 0
#endif

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

// D_ENV_BSD_HAS_NETLINK
//   feature: detect if Netlink socket support is available.
// FreeBSD 13+ added Netlink support for routing.
#if D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_14)
    #define D_ENV_BSD_HAS_NETLINK       1
#else
    #define D_ENV_BSD_HAS_NETLINK       0
#endif

// D_ENV_BSD_HAS_UNIX_CMSG
//   feature: detect if SCM_RIGHTS / SCM_CREDS credential passing over
// UNIX domain sockets is available. universal on BSD.
#define D_ENV_BSD_HAS_UNIX_CMSG         1

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


// =============================================================================
// IX.  FILESYSTEM FEATURES
// =============================================================================

// -----------------------------------------------------------------------------
// A.  ZFS
// -----------------------------------------------------------------------------

// D_ENV_BSD_HAS_ZFS
//   feature: detect if ZFS (OpenZFS) is available.
// FreeBSD has ZFS in base since FreeBSD 7; DragonFly has HAMMER instead.
// OpenBSD and NetBSD do not ship ZFS in base.
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_ZFS           1
#else
    #define D_ENV_BSD_HAS_ZFS           0
#endif

// D_ENV_BSD_HAS_OPENZFS
//   feature: detect if OpenZFS (modern ZFS fork) is the ZFS variant.
// FreeBSD 13+ uses OpenZFS.
#if D_ENV_FBSD_AT_LEAST(D_ENV_FBSD_VER_13)
    #define D_ENV_BSD_HAS_OPENZFS       1
#else
    #define D_ENV_BSD_HAS_OPENZFS       0
#endif


// -----------------------------------------------------------------------------
// B.  HAMMER / HAMMER2
// -----------------------------------------------------------------------------

// D_ENV_BSD_HAS_HAMMER2
//   feature: detect if HAMMER2 filesystem is available.
// DragonFly BSD only (HAMMER2 is the default root filesystem).
#if D_ENV_BSD_IS_DRAGONFLY
    #define D_ENV_BSD_HAS_HAMMER2       1
#else
    #define D_ENV_BSD_HAS_HAMMER2       0
#endif


// -----------------------------------------------------------------------------
// C.  FFS / UFS
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_UFS2
//   feature: detect if UFS2 is available (FreeBSD 5+, NetBSD 5+).
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_UFS2          1
#else
    #define D_ENV_BSD_HAS_UFS2          0
#endif

// D_ENV_BSD_HAS_SOFTDEP
//   feature: detect if soft-dependency (soft updates) journaling is
// available. FreeBSD and OpenBSD support soft updates on FFS.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_OPENBSD )
    #define D_ENV_BSD_HAS_SOFTDEP       1
#else
    #define D_ENV_BSD_HAS_SOFTDEP       0
#endif


// -----------------------------------------------------------------------------
// D.  general filesystem features
// -----------------------------------------------------------------------------

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

// D_ENV_BSD_HAS_DEVFS
//   feature: detect if devfs (device filesystem) is available.
// FreeBSD and DragonFly use devfs; OpenBSD and NetBSD do not.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_DEVFS         1
#else
    #define D_ENV_BSD_HAS_DEVFS         0
#endif

// D_ENV_BSD_HAS_FDESCFS
//   feature: detect if fdescfs (/dev/fd filesystem) is available.
// FreeBSD, NetBSD.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_FDESCFS       1
#else
    #define D_ENV_BSD_HAS_FDESCFS       0
#endif

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

// D_ENV_BSD_HAS_EXTATTR
//   feature: detect if extended attributes (extattr) are available.
// FreeBSD 5+ and NetBSD 3+ support extended attributes.
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_NETBSD )
    #define D_ENV_BSD_HAS_EXTATTR       1
#else
    #define D_ENV_BSD_HAS_EXTATTR       0
#endif


// =============================================================================
// X.   PROCESS AND THREADING
// =============================================================================

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

// D_ENV_BSD_HAS_PTHREAD
//   feature: detect if POSIX threads are available. universal on BSD.
#define D_ENV_BSD_HAS_PTHREAD           1

// D_ENV_BSD_HAS_PTHREAD_NP
//   feature: detect if BSD non-portable pthread extensions are
// available (pthread_set_name_np, pthread_getthreadid_np, etc.).
#if ( D_ENV_BSD_IS_FREEBSD ||                                                 \
      D_ENV_BSD_IS_DRAGONFLY )
    #define D_ENV_BSD_HAS_PTHREAD_NP    1
#else
    #define D_ENV_BSD_HAS_PTHREAD_NP    0
#endif

// D_ENV_BSD_HAS_KINFO_PROC
//   feature: detect if struct kinfo_proc (process info via sysctl) is
// available. present on all BSDs, but struct layout differs.
#define D_ENV_BSD_HAS_KINFO_PROC        D_ENV_BSD_HAS_SYSCTL

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

// D_ENV_BSD_HAS_CPUSET
//   feature: detect if cpuset_setaffinity / cpuset_getaffinity (CPU
// affinity) is available. FreeBSD 7.1+ only.
#if D_ENV_BSD_IS_FREEBSD
    #define D_ENV_BSD_HAS_CPUSET        1
#else
    #define D_ENV_BSD_HAS_CPUSET        0
#endif


// =============================================================================
// XI.  DISPLAY SERVER AND DESKTOP
// =============================================================================

// note: as with Linux, display server detection at compile time only
// indicates header availability, not the running server.

// D_ENV_BSD_HAS_X11
//   feature: detect if X11/Xlib development headers are available.
#if ( defined(_X11_XLIB_H_)  ||                                               \
      defined(_X11_X_H_)     ||                                                \
      defined(_XLIB_H_) )
    #define D_ENV_BSD_HAS_X11           1
#else
    #define D_ENV_BSD_HAS_X11           0
#endif

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

// D_ENV_BSD_HAS_DRM
//   feature: detect if DRM headers are available.
// FreeBSD and DragonFly have DRM/KMS in base; OpenBSD has xenocara;
// NetBSD has DRM in-tree.
#if defined(__DRM_H__)
    #define D_ENV_BSD_HAS_DRM           1
#else
    #define D_ENV_BSD_HAS_DRM           0
#endif

// D_ENV_BSD_HAS_WSCONS
//   feature: detect if wscons (workstation console framework) is
// available. NetBSD and OpenBSD only.
#if ( D_ENV_BSD_IS_NETBSD ||                                                  \
      D_ENV_BSD_IS_OPENBSD )
    #define D_ENV_BSD_HAS_WSCONS        1
#else
    #define D_ENV_BSD_HAS_WSCONS        0
#endif


// =============================================================================
// XII. BSD-SPECIFIC HEADERS AVAILABILITY
// =============================================================================

// D_ENV_BSD_HAS_SYS_EVENT_H
//   feature: sys/event.h (kqueue) is available on all modern BSDs.
#define D_ENV_BSD_HAS_SYS_EVENT_H       D_ENV_BSD_HAS_KQUEUE

// D_ENV_BSD_HAS_SYS_SYSCTL_H
//   feature: sys/sysctl.h is available on all BSDs.
#define D_ENV_BSD_HAS_SYS_SYSCTL_H      D_ENV_BSD_HAS_SYSCTL

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

// D_ENV_BSD_HAS_SYS_CAPSICUM_H
//   feature: sys/capsicum.h is available on FreeBSD 10+.
#define D_ENV_BSD_HAS_SYS_CAPSICUM_H    D_ENV_BSD_HAS_CAPSICUM

// D_ENV_BSD_HAS_SYS_JAIL_H
//   feature: sys/jail.h is available on FreeBSD.
#define D_ENV_BSD_HAS_SYS_JAIL_H        D_ENV_BSD_HAS_JAIL

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

// D_ENV_BSD_HAS_NET_BPF_H
//   feature: net/bpf.h (Berkeley Packet Filter) is available.
#define D_ENV_BSD_HAS_NET_BPF_H         D_ENV_BSD_HAS_BPF

// D_ENV_BSD_HAS_NET_ROUTE_H
//   feature: net/route.h (routing sockets) is available on all BSDs.
#define D_ENV_BSD_HAS_NET_ROUTE_H       D_ENV_BSD_HAS_ROUTING_SOCKETS

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


// =============================================================================
// XIII. COMPILER AND TOOLCHAIN
// =============================================================================

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

// D_ENV_BSD_HAS_PKGSRC
//   feature: detect if pkgsrc is the primary package system.
// NetBSD (and optionally DragonFly).
#if D_ENV_BSD_IS_NETBSD
    #define D_ENV_BSD_HAS_PKGSRC        1
#else
    #define D_ENV_BSD_HAS_PKGSRC        0
#endif


// =============================================================================
// XIV. RUNTIME DETECTION FUNCTIONS
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// d_env_bsd_get_version_string
//   function: returns the runtime BSD version string.
//   returns: OS version from uname or sysctl, or "Unknown" on failure.
const char* d_env_bsd_get_version_string(void);

// d_env_bsd_get_variant_name
//   function: returns the BSD variant name at runtime.
//   returns: "FreeBSD", "OpenBSD", "NetBSD", "DragonFly BSD", or
// "BSD (Unknown)".
const char* d_env_bsd_get_variant_name(void);

// d_env_bsd_has_feature
//   function: checks for a BSD feature at runtime via sysctl.
//   params:
//     feature_name - sysctl MIB string (e.g. "kern.securelevel")
//   returns: 1 if the sysctl exists and is readable, 0 otherwise.
int d_env_bsd_has_feature(const char* feature_name);

// d_env_bsd_get_securelevel
//   function: returns the current kern.securelevel value.
//   returns: securelevel integer value, or -2 on failure.
int d_env_bsd_get_securelevel(void);

// d_env_bsd_has_kqueue
//   function: runtime check that kqueue is functional (not just
// compilable).
//   returns: 1 if kqueue() succeeds, 0 otherwise.
int d_env_bsd_has_kqueue(void);

// d_env_bsd_print_info
//   function: prints detailed information about the detected BSD
// environment. includes variant, version, security features, filesystem
// support, and available APIs.
void d_env_bsd_print_info(void);

#ifdef __cplusplus
}
#endif


// =============================================================================
// XV.  CONVENIENCE MACROS
// =============================================================================

// D_ENV_BSD_IS_ANY
//   macro: evaluates to 1 if any recognized BSD variant is detected.
#define D_ENV_BSD_IS_ANY()                                                     \
    ( D_ENV_BSD_IS_FREEBSD   ||                                                \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY ||                                                \
      D_ENV_BSD_IS_BSDOS )

// D_ENV_BSD_IS_MODERN
//   macro: evaluates to 1 if the detected BSD is a modern,
// actively-maintained variant with recent releases (excludes BSD/OS).
#define D_ENV_BSD_IS_MODERN()                                                  \
    ( D_ENV_BSD_IS_FREEBSD   ||                                                \
      D_ENV_BSD_IS_OPENBSD   ||                                                \
      D_ENV_BSD_IS_NETBSD    ||                                                \
      D_ENV_BSD_IS_DRAGONFLY )

// D_ENV_BSD_HAS_STRONG_SANDBOXING
//   macro: evaluates to 1 if the BSD variant has a mature, built-in
// application sandboxing mechanism (Capsicum on FreeBSD, pledge/unveil
// on OpenBSD).
#define D_ENV_BSD_HAS_STRONG_SANDBOXING()                                      \
    ( D_ENV_BSD_HAS_CAPSICUM ||                                                \
      ( D_ENV_BSD_HAS_PLEDGE && D_ENV_BSD_HAS_UNVEIL ) )

// D_ENV_BSD_HAS_SECURE_RANDOM
//   macro: evaluates to 1 if a strong, non-blocking secure random
// source is available (arc4random or getentropy).
#define D_ENV_BSD_HAS_SECURE_RANDOM()                                          \
    ( D_ENV_BSD_HAS_ARC4RANDOM ||                                              \
      D_ENV_BSD_HAS_GETENTROPY )

// D_ENV_BSD_HAS_SAFE_STRING
//   macro: evaluates to 1 if safe string functions (strlcpy, strlcat,
// explicit_bzero) are available.
#define D_ENV_BSD_HAS_SAFE_STRING()                                            \
    ( D_ENV_BSD_HAS_STRLCPY      &&                                            \
      D_ENV_BSD_HAS_EXPLICIT_BZERO )

// D_ENV_BSD_HAS_FULL_EVENT_SYSTEM
//   macro: evaluates to 1 if the BSD provides a complete event
// notification system (kqueue is sufficient on BSD — it handles
// sockets, files, processes, signals, and timers).
#define D_ENV_BSD_HAS_FULL_EVENT_SYSTEM()                                      \
    ( D_ENV_BSD_HAS_KQUEUE )


#endif  // DJINTERP_ENV_BSD_
