/*******************************************************************************
* djinterp [env]                                                     env_linux.h
*
* djinterp Linux environment detection.
*   Compile-time detection of the Linux compilation environment: kernel
* version targeting, the C library and its version (glibc, musl, uClibc,
* Bionic), GNU feature-test macros, Linux-specific system call availability,
* security frameworks, filesystem features, IPC mechanisms, and header
* availability. It covers:
*     - Linux kernel version detection (LINUX_VERSION_CODE)
*     - C library detection and version (glibc, musl, uClibc, Bionic)
*     - GNU feature-test macro configuration (_GNU_SOURCE, _DEFAULT_SOURCE)
*     - kernel API availability (epoll, io_uring, inotify, eventfd, etc.)
*     - security frameworks (seccomp, capabilities, namespaces, landlock)
*     - filesystem features (/proc, /sys, cgroups, fanotify, statx)
*     - IPC and scheduling (futex, memfd, userfaultfd, SCHED_DEADLINE)
*     - display server indicators (X11, Wayland)
*     - init system detection (systemd)
*     - Linux-specific header availability
*   Include it where Linux detail is needed, guarded by the Linux OS check;
* from env.h's directory, for example:
*     #if (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
*         #include "./os/env_linux.h"
*     #endif
*   Kernel-version detection reads LINUX_VERSION_CODE, which comes from
* <linux/version.h>; see 1.2.
*   Naming: D_ENV_LINUX_<CATEGORY>_<FEATURE> is 1 if available, 0 otherwise.
*
* path:      /inc/djinterp/env/os/env_linux.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.28
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KERNEL VERSION
    --------------
    1.  Version constants
         1.  D_ENV_LINUX_KERNEL_VER
         2.  D_ENV_LINUX_KVER_2_6
         3.  D_ENV_LINUX_KVER_2_6_28
         4.  D_ENV_LINUX_KVER_2_6_32
         5.  D_ENV_LINUX_KVER_3_0
         6.  D_ENV_LINUX_KVER_3_2
         7.  D_ENV_LINUX_KVER_3_10
         8.  D_ENV_LINUX_KVER_3_17
         9.  D_ENV_LINUX_KVER_3_19
         10. D_ENV_LINUX_KVER_4_0
         11. D_ENV_LINUX_KVER_4_3
         12. D_ENV_LINUX_KVER_4_5
         13. D_ENV_LINUX_KVER_4_8
         14. D_ENV_LINUX_KVER_4_11
         15. D_ENV_LINUX_KVER_4_14
         16. D_ENV_LINUX_KVER_4_18
         17. D_ENV_LINUX_KVER_4_19
         18. D_ENV_LINUX_KVER_5_0
         19. D_ENV_LINUX_KVER_5_1
         20. D_ENV_LINUX_KVER_5_3
         21. D_ENV_LINUX_KVER_5_4
         22. D_ENV_LINUX_KVER_5_6
         23. D_ENV_LINUX_KVER_5_8
         24. D_ENV_LINUX_KVER_5_9
         25. D_ENV_LINUX_KVER_5_10
         26. D_ENV_LINUX_KVER_5_13
         27. D_ENV_LINUX_KVER_5_14
         28. D_ENV_LINUX_KVER_5_15
         29. D_ENV_LINUX_KVER_6_0
         30. D_ENV_LINUX_KVER_6_1
         31. D_ENV_LINUX_KVER_6_4
         32. D_ENV_LINUX_KVER_6_6
         33. D_ENV_LINUX_KVER_6_8
         34. D_ENV_LINUX_KVER_6_12
    2.  Detected kernel version
         1.  D_ENV_LINUX_KVER / D_ENV_LINUX_KVER_DETECTED
         2.  D_ENV_LINUX_KVER_AT_LEAST
         3.  D_ENV_LINUX_KVER_AT_LEAST_HEX
2.  C LIBRARY
    ---------
    1.  glibc
         1.  D_ENV_LINUX_LIBC_GLIBC
         2.  D_ENV_LINUX_GLIBC_AT_LEAST
    2.  musl
         1.  D_ENV_LINUX_LIBC_MUSL
    3.  uClibc
         1.  D_ENV_LINUX_LIBC_UCLIBC
    4.  Bionic
         1.  D_ENV_LINUX_LIBC_BIONIC
    5.  Library name
         1.  D_ENV_LINUX_LIBC_NAME
3.  FEATURE-TEST MACROS AND GLIBC FUNCTIONS
    ---------------------------------------
    1.  GNU feature-test macros
         1.  D_ENV_LINUX_HAS_GNU_SOURCE
         2.  D_ENV_LINUX_HAS_DEFAULT_SOURCE
         3.  D_ENV_LINUX_HAS_BSD_SOURCE
         4.  D_ENV_LINUX_HAS_SVID_SOURCE
         5.  D_ENV_LINUX_HAS_LARGEFILE_SOURCE
         6.  D_ENV_LINUX_HAS_LARGEFILE64_SOURCE
         7.  D_ENV_LINUX_HAS_FILE_OFFSET_BITS_64
         8.  D_ENV_LINUX_HAS_FORTIFY_SOURCE
    2.  Version-gated glibc functions
         1.  D_ENV_LINUX_GLIBC_HAS_PIPE2
         2.  D_ENV_LINUX_GLIBC_HAS_DUP3
         3.  D_ENV_LINUX_GLIBC_HAS_ACCEPT4
         4.  D_ENV_LINUX_GLIBC_HAS_RECVMMSG
         5.  D_ENV_LINUX_GLIBC_HAS_SENDMMSG
         6.  D_ENV_LINUX_GLIBC_HAS_SYNCFS
         7.  D_ENV_LINUX_GLIBC_HAS_GETAUXVAL
         8.  D_ENV_LINUX_GLIBC_HAS_SECURE_GETENV
         9.  D_ENV_LINUX_GLIBC_HAS_EXPLICIT_BZERO
         10. D_ENV_LINUX_GLIBC_HAS_GETENTROPY
         11. D_ENV_LINUX_GLIBC_HAS_REALLOCARRAY
         12. D_ENV_LINUX_GLIBC_HAS_COPY_FILE_RANGE
         13. D_ENV_LINUX_GLIBC_HAS_STATX
         14. D_ENV_LINUX_GLIBC_HAS_CLOSE_RANGE
4.  KERNEL APIS
    -----------
    1.  Events and notification
         1.  D_ENV_LINUX_HAS_EPOLL
         2.  D_ENV_LINUX_HAS_EVENTFD
         3.  D_ENV_LINUX_HAS_SIGNALFD
         4.  D_ENV_LINUX_HAS_TIMERFD
         5.  D_ENV_LINUX_HAS_INOTIFY
         6.  D_ENV_LINUX_HAS_FANOTIFY
    2.  io_uring
         1.  D_ENV_LINUX_HAS_IO_URING
    3.  Memory management
         1.  D_ENV_LINUX_HAS_MEMFD_CREATE
         2.  D_ENV_LINUX_HAS_MEMFD_SECRET
         3.  D_ENV_LINUX_HAS_MLOCK2
         4.  D_ENV_LINUX_HAS_USERFAULTFD
    4.  Process and thread management
         1.  D_ENV_LINUX_HAS_CLONE3
         2.  D_ENV_LINUX_HAS_PIDFD
         3.  D_ENV_LINUX_HAS_WAITID
         4.  D_ENV_LINUX_HAS_PRCTL
         5.  D_ENV_LINUX_HAS_SCHED_DEADLINE
    5.  File and filesystem operations
         1.  D_ENV_LINUX_HAS_OPENAT2
         2.  D_ENV_LINUX_HAS_STATX
         3.  D_ENV_LINUX_HAS_RENAMEAT2
         4.  D_ENV_LINUX_HAS_COPY_FILE_RANGE
         5.  D_ENV_LINUX_HAS_CLOSE_RANGE
5.  SECURITY FRAMEWORKS
    -------------------
    1.  seccomp
         1.  D_ENV_LINUX_HAS_SECCOMP
    2.  Capabilities
         1.  D_ENV_LINUX_HAS_CAPABILITIES
    3.  Namespaces
         1.  D_ENV_LINUX_HAS_NAMESPACES
         2.  D_ENV_LINUX_HAS_USER_NAMESPACES
    4.  Landlock
         1.  D_ENV_LINUX_HAS_LANDLOCK
6.  FILESYSTEMS AND NETWORKING
    --------------------------
    1.  Virtual filesystems
         1.  D_ENV_LINUX_HAS_PROCFS
         2.  D_ENV_LINUX_HAS_SYSFS
         3.  D_ENV_LINUX_HAS_CGROUPS_V1
         4.  D_ENV_LINUX_HAS_CGROUPS_V2
         5.  D_ENV_LINUX_HAS_TMPFS
         6.  D_ENV_LINUX_HAS_DEVTMPFS
    2.  Networking
         1.  D_ENV_LINUX_HAS_NETLINK
         2.  D_ENV_LINUX_HAS_UNIX_DIAG
         3.  D_ENV_LINUX_HAS_SPLICE
         4.  D_ENV_LINUX_HAS_SO_REUSEPORT
7.  DESKTOP AND INIT SYSTEM
    -----------------------
    1.  Display server and desktop
         1.  D_ENV_LINUX_HAS_X11
         2.  D_ENV_LINUX_HAS_XCB
         3.  D_ENV_LINUX_HAS_WAYLAND
         4.  D_ENV_LINUX_HAS_DRM
         5.  D_ENV_LINUX_HAS_KMS
         6.  D_ENV_LINUX_HAS_GBM
         7.  D_ENV_LINUX_HAS_EGL
    2.  Init system and service manager
         1.  D_ENV_LINUX_HAS_SYSTEMD
         2.  D_ENV_LINUX_HAS_SD_BUS
         3.  D_ENV_LINUX_HAS_SD_JOURNAL
8.  HEADERS AND TOOLCHAIN
    ---------------------
    1.  Linux-specific headers
         1.  D_ENV_LINUX_HAS_SYS_EPOLL_H
         2.  D_ENV_LINUX_HAS_SYS_INOTIFY_H
         3.  D_ENV_LINUX_HAS_SYS_EVENTFD_H
         4.  D_ENV_LINUX_HAS_SYS_SIGNALFD_H
         5.  D_ENV_LINUX_HAS_SYS_TIMERFD_H
         6.  D_ENV_LINUX_HAS_SYS_SENDFILE_H
         7.  D_ENV_LINUX_HAS_SYS_PRCTL_H
         8.  D_ENV_LINUX_HAS_SYS_CAPABILITY_H
         9.  D_ENV_LINUX_HAS_SYS_SYSINFO_H
         10. D_ENV_LINUX_HAS_LINUX_FUTEX_H
         11. D_ENV_LINUX_HAS_LINUX_SECCOMP_H
         12. D_ENV_LINUX_HAS_LINUX_LANDLOCK_H
         13. D_ENV_LINUX_HAS_LINUX_IO_URING_H
         14. D_ENV_LINUX_HAS_DLFCN_H
         15. D_ENV_LINUX_HAS_ELF_H
         16. D_ENV_LINUX_HAS_LINK_H
         17. D_ENV_LINUX_HAS_SYS_XATTR_H
    2.  Compiler and toolchain
         1.  D_ENV_LINUX_IS_GCC
         2.  D_ENV_LINUX_IS_CLANG
         3.  D_ENV_LINUX_HAS_SANITIZERS
         4.  D_ENV_LINUX_HAS_STACK_PROTECTOR
         5.  D_ENV_LINUX_HAS_PIE
         6.  D_ENV_LINUX_HAS_RELRO
9.  RUNTIME DETECTION AND CONVENIENCE
    ---------------------------------
    1.  Runtime queries
    2.  Combined predicates
         1.  D_ENV_LINUX_IS_MODERN
         2.  D_ENV_LINUX_IS_EMBEDDED
         3.  D_ENV_LINUX_IS_HARDENED
         4.  D_ENV_LINUX_HAS_MODERN_IO
         5.  D_ENV_LINUX_HAS_SANDBOXING
*/

#ifndef DJINTERP_ENV_OS_ENV_LINUX_H
#define DJINTERP_ENV_OS_ENV_LINUX_H 1

// djinterp
#include "../env.h"  // D_ENV_OS_ID, D_ENV_LANG_USING_CPP


//==============================================================================
// 1.  KERNEL VERSION
//==============================================================================


// 1.1    Version constants
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_LINUX_KERNEL_VER
//   macro: (major, minor, patch) packed as LINUX_VERSION_CODE packs them. It
// is always defined, so version comparisons work without <linux/version.h>;
// when that header was included first, it defers to its KERNEL_VERSION.
#ifndef KERNEL_VERSION
    #define D_ENV_LINUX_KERNEL_VER(a, b, c)                                    \
        (((a) << 16) + ((b) << 8) + (c))
#else
    #define D_ENV_LINUX_KERNEL_VER(a, b, c) KERNEL_VERSION(a, b, c)
#endif  // KERNEL_VERSION

// 1.1.2
// D_ENV_LINUX_KVER_2_6
//   constant: LINUX_VERSION_CODE for kernel 2.6.0.
#define D_ENV_LINUX_KVER_2_6            D_ENV_LINUX_KERNEL_VER(2, 6, 0)

// 1.1.3
// D_ENV_LINUX_KVER_2_6_28
//   constant: LINUX_VERSION_CODE for kernel 2.6.28 (epoll improvements,
// namespace enhancements).
#define D_ENV_LINUX_KVER_2_6_28         D_ENV_LINUX_KERNEL_VER(2, 6, 28)

// 1.1.4
// D_ENV_LINUX_KVER_2_6_32
//   constant: LINUX_VERSION_CODE for kernel 2.6.32 (RHEL 6 base).
#define D_ENV_LINUX_KVER_2_6_32         D_ENV_LINUX_KERNEL_VER(2, 6, 32)

// 1.1.5
// D_ENV_LINUX_KVER_3_0
//   constant: LINUX_VERSION_CODE for kernel 3.0.
#define D_ENV_LINUX_KVER_3_0            D_ENV_LINUX_KERNEL_VER(3, 0, 0)

// 1.1.6
// D_ENV_LINUX_KVER_3_2
//   constant: LINUX_VERSION_CODE for kernel 3.2 (Debian 7 base).
#define D_ENV_LINUX_KVER_3_2            D_ENV_LINUX_KERNEL_VER(3, 2, 0)

// 1.1.7
// D_ENV_LINUX_KVER_3_10
//   constant: LINUX_VERSION_CODE for kernel 3.10 (RHEL 7 base).
#define D_ENV_LINUX_KVER_3_10           D_ENV_LINUX_KERNEL_VER(3, 10, 0)

// 1.1.8
// D_ENV_LINUX_KVER_3_17
//   constant: LINUX_VERSION_CODE for kernel 3.17 (seccomp improvements,
// getrandom() syscall).
#define D_ENV_LINUX_KVER_3_17           D_ENV_LINUX_KERNEL_VER(3, 17, 0)

// 1.1.9
// D_ENV_LINUX_KVER_3_19
//   constant: LINUX_VERSION_CODE for kernel 3.19 (memfd_create).
#define D_ENV_LINUX_KVER_3_19           D_ENV_LINUX_KERNEL_VER(3, 19, 0)

// 1.1.10
// D_ENV_LINUX_KVER_4_0
//   constant: LINUX_VERSION_CODE for kernel 4.0.
#define D_ENV_LINUX_KVER_4_0            D_ENV_LINUX_KERNEL_VER(4, 0, 0)

// 1.1.11
// D_ENV_LINUX_KVER_4_3
//   constant: LINUX_VERSION_CODE for kernel 4.3 (userfaultfd).
#define D_ENV_LINUX_KVER_4_3            D_ENV_LINUX_KERNEL_VER(4, 3, 0)

// 1.1.12
// D_ENV_LINUX_KVER_4_5
//   constant: LINUX_VERSION_CODE for kernel 4.5 (copy_file_range).
#define D_ENV_LINUX_KVER_4_5            D_ENV_LINUX_KERNEL_VER(4, 5, 0)

// 1.1.13
// D_ENV_LINUX_KVER_4_8
//   constant: LINUX_VERSION_CODE for kernel 4.8 (SCHED_DEADLINE
// improvements, mlock2).
#define D_ENV_LINUX_KVER_4_8            D_ENV_LINUX_KERNEL_VER(4, 8, 0)

// 1.1.14
// D_ENV_LINUX_KVER_4_11
//   constant: LINUX_VERSION_CODE for kernel 4.11 (statx syscall).
#define D_ENV_LINUX_KVER_4_11           D_ENV_LINUX_KERNEL_VER(4, 11, 0)

// 1.1.15
// D_ENV_LINUX_KVER_4_14
//   constant: LINUX_VERSION_CODE for kernel 4.14 (first LTS in modern
// scheme, KPTI/Meltdown mitigations).
#define D_ENV_LINUX_KVER_4_14           D_ENV_LINUX_KERNEL_VER(4, 14, 0)

// 1.1.16
// D_ENV_LINUX_KVER_4_18
//   constant: LINUX_VERSION_CODE for kernel 4.18 (RHEL 8 base, io_uring
// precursors).
#define D_ENV_LINUX_KVER_4_18           D_ENV_LINUX_KERNEL_VER(4, 18, 0)

// 1.1.17
// D_ENV_LINUX_KVER_4_19
//   constant: LINUX_VERSION_CODE for kernel 4.19 (LTS).
#define D_ENV_LINUX_KVER_4_19           D_ENV_LINUX_KERNEL_VER(4, 19, 0)

// 1.1.18
// D_ENV_LINUX_KVER_5_0
//   constant: LINUX_VERSION_CODE for kernel 5.0.
#define D_ENV_LINUX_KVER_5_0            D_ENV_LINUX_KERNEL_VER(5, 0, 0)

// 1.1.19
// D_ENV_LINUX_KVER_5_1
//   constant: LINUX_VERSION_CODE for kernel 5.1 (io_uring introduced).
#define D_ENV_LINUX_KVER_5_1            D_ENV_LINUX_KERNEL_VER(5, 1, 0)

// 1.1.20
// D_ENV_LINUX_KVER_5_3
//   constant: LINUX_VERSION_CODE for kernel 5.3 (io_uring matured,
// pidfd_open).
#define D_ENV_LINUX_KVER_5_3            D_ENV_LINUX_KERNEL_VER(5, 3, 0)

// 1.1.21
// D_ENV_LINUX_KVER_5_4
//   constant: LINUX_VERSION_CODE for kernel 5.4 (LTS, Debian 11 base).
#define D_ENV_LINUX_KVER_5_4            D_ENV_LINUX_KERNEL_VER(5, 4, 0)

// 1.1.22
// D_ENV_LINUX_KVER_5_6
//   constant: LINUX_VERSION_CODE for kernel 5.6 (openat2, pidfd_getfd).
#define D_ENV_LINUX_KVER_5_6            D_ENV_LINUX_KERNEL_VER(5, 6, 0)

// 1.1.23
// D_ENV_LINUX_KVER_5_8
//   constant: LINUX_VERSION_CODE for kernel 5.8 (faccessat2).
#define D_ENV_LINUX_KVER_5_8            D_ENV_LINUX_KERNEL_VER(5, 8, 0)

// 1.1.24
// D_ENV_LINUX_KVER_5_9
//   constant: LINUX_VERSION_CODE for kernel 5.9 (close_range).
#define D_ENV_LINUX_KVER_5_9            D_ENV_LINUX_KERNEL_VER(5, 9, 0)

// 1.1.25
// D_ENV_LINUX_KVER_5_10
//   constant: LINUX_VERSION_CODE for kernel 5.10 (LTS, RHEL 9 base).
#define D_ENV_LINUX_KVER_5_10           D_ENV_LINUX_KERNEL_VER(5, 10, 0)

// 1.1.26
// D_ENV_LINUX_KVER_5_13
//   constant: LINUX_VERSION_CODE for kernel 5.13 (Landlock LSM).
#define D_ENV_LINUX_KVER_5_13           D_ENV_LINUX_KERNEL_VER(5, 13, 0)

// 1.1.27
// D_ENV_LINUX_KVER_5_14
//   constant: LINUX_VERSION_CODE for kernel 5.14 (memfd_secret).
#define D_ENV_LINUX_KVER_5_14           D_ENV_LINUX_KERNEL_VER(5, 14, 0)

// 1.1.28
// D_ENV_LINUX_KVER_5_15
//   constant: LINUX_VERSION_CODE for kernel 5.15 (LTS, NTFS3 driver).
#define D_ENV_LINUX_KVER_5_15           D_ENV_LINUX_KERNEL_VER(5, 15, 0)

// 1.1.29
// D_ENV_LINUX_KVER_6_0
//   constant: LINUX_VERSION_CODE for kernel 6.0.
#define D_ENV_LINUX_KVER_6_0            D_ENV_LINUX_KERNEL_VER(6, 0, 0)

// 1.1.30
// D_ENV_LINUX_KVER_6_1
//   constant: LINUX_VERSION_CODE for kernel 6.1 (LTS, Rust support,
// Debian 12 base).
#define D_ENV_LINUX_KVER_6_1            D_ENV_LINUX_KERNEL_VER(6, 1, 0)

// 1.1.31
// D_ENV_LINUX_KVER_6_4
//   constant: LINUX_VERSION_CODE for kernel 6.4.
#define D_ENV_LINUX_KVER_6_4            D_ENV_LINUX_KERNEL_VER(6, 4, 0)

// 1.1.32
// D_ENV_LINUX_KVER_6_6
//   constant: LINUX_VERSION_CODE for kernel 6.6 (LTS).
#define D_ENV_LINUX_KVER_6_6            D_ENV_LINUX_KERNEL_VER(6, 6, 0)

// 1.1.33
// D_ENV_LINUX_KVER_6_8
//   constant: LINUX_VERSION_CODE for kernel 6.8 (Ubuntu 24.04 base).
#define D_ENV_LINUX_KVER_6_8            D_ENV_LINUX_KERNEL_VER(6, 8, 0)

// 1.1.34
// D_ENV_LINUX_KVER_6_12
//   constant: LINUX_VERSION_CODE for kernel 6.12 (LTS).
#define D_ENV_LINUX_KVER_6_12           D_ENV_LINUX_KERNEL_VER(6, 12, 0)

// 1.2    Detected kernel version
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_LINUX_KVER / D_ENV_LINUX_KVER_DETECTED
//   constant: LINUX_VERSION_CODE and its major, minor, and patch parts, with
// D_ENV_LINUX_KVER_DETECTED set to 1 when it is known. It describes the
// kernel headers the build uses, and comes from <linux/version.h>, which this
// header does not include: unless that header was included first, every
// value is 0 and every kernel-version gate below reads as not met.
#if defined(LINUX_VERSION_CODE)
    #define D_ENV_LINUX_KVER            LINUX_VERSION_CODE
    #define D_ENV_LINUX_KVER_MAJOR      ((LINUX_VERSION_CODE >> 16) & 0xFF)
    #define D_ENV_LINUX_KVER_MINOR      ((LINUX_VERSION_CODE >> 8) & 0xFF)
    #define D_ENV_LINUX_KVER_PATCH      (LINUX_VERSION_CODE & 0xFF)
    #define D_ENV_LINUX_KVER_DETECTED   1
#else
    #define D_ENV_LINUX_KVER            0
    #define D_ENV_LINUX_KVER_MAJOR      0
    #define D_ENV_LINUX_KVER_MINOR      0
    #define D_ENV_LINUX_KVER_PATCH      0
    #define D_ENV_LINUX_KVER_DETECTED   0
#endif

// 1.2.2
// D_ENV_LINUX_KVER_AT_LEAST
//   macro: evaluates to 1 if the targeted kernel version is at least
// the specified version.
#define D_ENV_LINUX_KVER_AT_LEAST(major, minor, patch)                         \
    ( D_ENV_LINUX_KVER_DETECTED &&                                             \
      (D_ENV_LINUX_KVER >= D_ENV_LINUX_KERNEL_VER(major, minor, patch)) )

// 1.2.3
// D_ENV_LINUX_KVER_AT_LEAST_HEX
//   macro: evaluates to 1 if the targeted kernel version is at least
// the specified hex constant (e.g. D_ENV_LINUX_KVER_5_4).
#define D_ENV_LINUX_KVER_AT_LEAST_HEX(hex_version)                             \
    ( D_ENV_LINUX_KVER_DETECTED &&                                             \
      (D_ENV_LINUX_KVER >= (hex_version)) )


//==============================================================================
// 2.  C LIBRARY
//==============================================================================


// 2.1    glibc
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_LINUX_LIBC_GLIBC
//   feature: detect if the GNU C Library (glibc) is in use.
#if ( defined(__GLIBC__) ||                                                    \
      defined(__GNU_LIBRARY__) )
    #define D_ENV_LINUX_LIBC_GLIBC      1

    // D_ENV_LINUX_GLIBC_MAJOR
    //   constant: glibc major version number.
    #ifdef __GLIBC__
        #define D_ENV_LINUX_GLIBC_MAJOR __GLIBC__
    #else
        #define D_ENV_LINUX_GLIBC_MAJOR 1
    #endif  // __GLIBC__

    // D_ENV_LINUX_GLIBC_MINOR
    //   constant: glibc minor version number.
    #ifdef __GLIBC_MINOR__
        #define D_ENV_LINUX_GLIBC_MINOR __GLIBC_MINOR__
    #else
        #define D_ENV_LINUX_GLIBC_MINOR 0
    #endif  // __GLIBC_MINOR__

    #define D_ENV_LINUX_LIBC_NAME       "glibc"

#else
    #define D_ENV_LINUX_LIBC_GLIBC      0
    #define D_ENV_LINUX_GLIBC_MAJOR     0
    #define D_ENV_LINUX_GLIBC_MINOR     0
#endif

// 2.1.2
// D_ENV_LINUX_GLIBC_AT_LEAST
//   macro: evaluates to 1 if the detected glibc version is at least the
// specified major.minor version.
#define D_ENV_LINUX_GLIBC_AT_LEAST(major, minor)                               \
    ( D_ENV_LINUX_LIBC_GLIBC &&                                                \
      ( (D_ENV_LINUX_GLIBC_MAJOR > (major)) ||                                 \
        ( (D_ENV_LINUX_GLIBC_MAJOR == (major)) &&                              \
          (D_ENV_LINUX_GLIBC_MINOR >= (minor)) ) ) )

// 2.2    musl
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_LINUX_LIBC_MUSL
//   feature: detect if musl libc is in use. musl defines no identifying
// macro, so this reads a __MUSL__ the build defines; without one, musl reads
// as 0.
#if defined(__MUSL__)
    // user/build-system defined
    #define D_ENV_LINUX_LIBC_MUSL       1
    #ifndef D_ENV_LINUX_LIBC_NAME
        #define D_ENV_LINUX_LIBC_NAME   "musl"
    #endif  // D_ENV_LINUX_LIBC_NAME
#else
    #define D_ENV_LINUX_LIBC_MUSL       0
#endif

// 2.3    uClibc
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_LINUX_LIBC_UCLIBC
//   feature: detect if uClibc / uClibc-ng is in use (embedded Linux), with
// D_ENV_LINUX_UCLIBC_MAJOR, _MINOR, and _PATCH giving its version.
#if defined(__UCLIBC__)
    #define D_ENV_LINUX_LIBC_UCLIBC     1

    #ifdef __UCLIBC_MAJOR__
        #define D_ENV_LINUX_UCLIBC_MAJOR __UCLIBC_MAJOR__
    #else
        #define D_ENV_LINUX_UCLIBC_MAJOR 0
    #endif  // __UCLIBC_MAJOR__

    #ifdef __UCLIBC_MINOR__
        #define D_ENV_LINUX_UCLIBC_MINOR __UCLIBC_MINOR__
    #else
        #define D_ENV_LINUX_UCLIBC_MINOR 0
    #endif  // __UCLIBC_MINOR__

    #ifdef __UCLIBC_SUBLEVEL__
        #define D_ENV_LINUX_UCLIBC_PATCH __UCLIBC_SUBLEVEL__
    #else
        #define D_ENV_LINUX_UCLIBC_PATCH 0
    #endif  // __UCLIBC_SUBLEVEL__

    #ifndef D_ENV_LINUX_LIBC_NAME
        #define D_ENV_LINUX_LIBC_NAME   "uClibc"
    #endif  // D_ENV_LINUX_LIBC_NAME

#else
    #define D_ENV_LINUX_LIBC_UCLIBC     0
    #define D_ENV_LINUX_UCLIBC_MAJOR    0
    #define D_ENV_LINUX_UCLIBC_MINOR    0
    #define D_ENV_LINUX_UCLIBC_PATCH    0
#endif

// 2.4    Bionic
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_LINUX_LIBC_BIONIC
//   feature: detect if Bionic, Android's libc, is in use; included for
// completeness when building against the Android NDK on a Linux host.
#if defined(__BIONIC__)
    #define D_ENV_LINUX_LIBC_BIONIC     1
    #ifndef D_ENV_LINUX_LIBC_NAME
        #define D_ENV_LINUX_LIBC_NAME   "Bionic"
    #endif  // D_ENV_LINUX_LIBC_NAME
#else
    #define D_ENV_LINUX_LIBC_BIONIC     0
#endif

// 2.5    Library name
//------------------------------------------------------------------------------
// 2.5.1
// D_ENV_LINUX_LIBC_NAME
//   constant: the detected C library's name, or "Unknown" when none of the
// above was detected.
#ifndef D_ENV_LINUX_LIBC_NAME
    #define D_ENV_LINUX_LIBC_NAME       "Unknown"
#endif  // D_ENV_LINUX_LIBC_NAME


//==============================================================================
// 3.  FEATURE-TEST MACROS AND GLIBC FUNCTIONS
//==============================================================================


// 3.1    GNU feature-test macros
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_LINUX_HAS_GNU_SOURCE
//   feature: detect if _GNU_SOURCE is defined, which enables glibc-specific
// extensions (GNU string functions, Linux-specific syscall wrappers, etc.).
#if defined(_GNU_SOURCE)
    #define D_ENV_LINUX_HAS_GNU_SOURCE  1
#else
    #define D_ENV_LINUX_HAS_GNU_SOURCE  0
#endif

// 3.1.2
// D_ENV_LINUX_HAS_DEFAULT_SOURCE
//   feature: detect if _DEFAULT_SOURCE is defined (glibc 2.19+
// replacement for _BSD_SOURCE and _SVID_SOURCE).
#if defined(_DEFAULT_SOURCE)
    #define D_ENV_LINUX_HAS_DEFAULT_SOURCE 1
#else
    #define D_ENV_LINUX_HAS_DEFAULT_SOURCE 0
#endif

// 3.1.3
// D_ENV_LINUX_HAS_BSD_SOURCE
//   feature: detect if _BSD_SOURCE is defined (deprecated in glibc 2.20,
// replaced by _DEFAULT_SOURCE).
#if defined(_BSD_SOURCE)
    #define D_ENV_LINUX_HAS_BSD_SOURCE  1
#else
    #define D_ENV_LINUX_HAS_BSD_SOURCE  0
#endif

// 3.1.4
// D_ENV_LINUX_HAS_SVID_SOURCE
//   feature: detect if _SVID_SOURCE is defined (deprecated in glibc 2.20,
// replaced by _DEFAULT_SOURCE).
#if defined(_SVID_SOURCE)
    #define D_ENV_LINUX_HAS_SVID_SOURCE 1
#else
    #define D_ENV_LINUX_HAS_SVID_SOURCE 0
#endif

// 3.1.5
// D_ENV_LINUX_HAS_LARGEFILE_SOURCE
//   feature: detect if _LARGEFILE_SOURCE is defined (enables fseeko,
// ftello on 32-bit).
#if defined(_LARGEFILE_SOURCE)
    #define D_ENV_LINUX_HAS_LARGEFILE_SOURCE 1
#else
    #define D_ENV_LINUX_HAS_LARGEFILE_SOURCE 0
#endif

// 3.1.6
// D_ENV_LINUX_HAS_LARGEFILE64_SOURCE
//   feature: detect if _LARGEFILE64_SOURCE is defined (enables explicit
// 64-bit file operations: open64, lseek64, etc.).
#if defined(_LARGEFILE64_SOURCE)
    #define D_ENV_LINUX_HAS_LARGEFILE64_SOURCE 1
#else
    #define D_ENV_LINUX_HAS_LARGEFILE64_SOURCE 0
#endif

// 3.1.7
// D_ENV_LINUX_HAS_FILE_OFFSET_BITS_64
//   feature: detect if _FILE_OFFSET_BITS is set to 64 (transparent
// large file support: off_t becomes 64-bit on 32-bit platforms).
#if ( defined(_FILE_OFFSET_BITS) &&                                            \
      (_FILE_OFFSET_BITS == 64) )
    #define D_ENV_LINUX_HAS_FILE_OFFSET_BITS_64 1
#else
    #define D_ENV_LINUX_HAS_FILE_OFFSET_BITS_64 0
#endif

// 3.1.8
// D_ENV_LINUX_HAS_FORTIFY_SOURCE
//   feature: detect if _FORTIFY_SOURCE is defined (enables compile-time
// and runtime buffer overflow detection in glibc).
#if defined(_FORTIFY_SOURCE)
    #define D_ENV_LINUX_HAS_FORTIFY_SOURCE _FORTIFY_SOURCE
#else
    #define D_ENV_LINUX_HAS_FORTIFY_SOURCE 0
#endif

// 3.2    Version-gated glibc functions
//------------------------------------------------------------------------------
//   many glibc functions were introduced at specific versions; these gates
// keep code portable across distributions.
// 3.2.1
// D_ENV_LINUX_GLIBC_HAS_PIPE2
//   feature: detect if pipe2() is available (glibc 2.9+, kernel 2.6.27+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 9)
    #define D_ENV_LINUX_GLIBC_HAS_PIPE2 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_PIPE2 0
#endif

// 3.2.2
// D_ENV_LINUX_GLIBC_HAS_DUP3
//   feature: detect if dup3() is available (glibc 2.9+, kernel 2.6.27+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 9)
    #define D_ENV_LINUX_GLIBC_HAS_DUP3  1
#else
    #define D_ENV_LINUX_GLIBC_HAS_DUP3  0
#endif

// 3.2.3
// D_ENV_LINUX_GLIBC_HAS_ACCEPT4
//   feature: detect if accept4() is available (glibc 2.10+, kernel
// 2.6.28+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 10)
    #define D_ENV_LINUX_GLIBC_HAS_ACCEPT4 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_ACCEPT4 0
#endif

// 3.2.4
// D_ENV_LINUX_GLIBC_HAS_RECVMMSG
//   feature: detect if recvmmsg() is available (glibc 2.12+, kernel
// 2.6.33+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 12)
    #define D_ENV_LINUX_GLIBC_HAS_RECVMMSG 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_RECVMMSG 0
#endif

// 3.2.5
// D_ENV_LINUX_GLIBC_HAS_SENDMMSG
//   feature: detect if sendmmsg() is available (glibc 2.14+, kernel
// 3.0+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 14)
    #define D_ENV_LINUX_GLIBC_HAS_SENDMMSG 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_SENDMMSG 0
#endif

// 3.2.6
// D_ENV_LINUX_GLIBC_HAS_SYNCFS
//   feature: detect if syncfs() is available (glibc 2.14+, kernel
// 2.6.39+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 14)
    #define D_ENV_LINUX_GLIBC_HAS_SYNCFS 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_SYNCFS 0
#endif

// 3.2.7
// D_ENV_LINUX_GLIBC_HAS_GETAUXVAL
//   feature: detect if getauxval() is available (glibc 2.16+).
// reads values from the ELF auxiliary vector (AT_HWCAP, etc.).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 16)
    #define D_ENV_LINUX_GLIBC_HAS_GETAUXVAL 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_GETAUXVAL 0
#endif

// 3.2.8
// D_ENV_LINUX_GLIBC_HAS_SECURE_GETENV
//   feature: detect if secure_getenv() is available (glibc 2.17+).
// returns NULL when running with elevated privileges (setuid/setgid).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 17)
    #define D_ENV_LINUX_GLIBC_HAS_SECURE_GETENV 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_SECURE_GETENV 0
#endif

// 3.2.9
// D_ENV_LINUX_GLIBC_HAS_EXPLICIT_BZERO
//   feature: detect if explicit_bzero() is available (glibc 2.25+).
// guaranteed not to be optimized away; for clearing sensitive data.
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 25)
    #define D_ENV_LINUX_GLIBC_HAS_EXPLICIT_BZERO 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_EXPLICIT_BZERO 0
#endif

// 3.2.10
// D_ENV_LINUX_GLIBC_HAS_GETENTROPY
//   feature: detect if getentropy() is available (glibc 2.25+).
// secure random bytes from the kernel entropy pool.
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 25)
    #define D_ENV_LINUX_GLIBC_HAS_GETENTROPY 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_GETENTROPY 0
#endif

// 3.2.11
// D_ENV_LINUX_GLIBC_HAS_REALLOCARRAY
//   feature: detect if reallocarray() is available (glibc 2.26+).
// overflow-checking realloc(nmemb * size).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 26)
    #define D_ENV_LINUX_GLIBC_HAS_REALLOCARRAY 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_REALLOCARRAY 0
#endif

// 3.2.12
// D_ENV_LINUX_GLIBC_HAS_COPY_FILE_RANGE
//   feature: detect if copy_file_range() wrapper is available
// (glibc 2.27+, kernel 4.5+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 27)
    #define D_ENV_LINUX_GLIBC_HAS_COPY_FILE_RANGE 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_COPY_FILE_RANGE 0
#endif

// 3.2.13
// D_ENV_LINUX_GLIBC_HAS_STATX
//   feature: detect if statx() wrapper is available (glibc 2.28+,
// kernel 4.11+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 28)
    #define D_ENV_LINUX_GLIBC_HAS_STATX 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_STATX 0
#endif

// 3.2.14
// D_ENV_LINUX_GLIBC_HAS_CLOSE_RANGE
//   feature: detect if close_range() wrapper is available (glibc 2.34+,
// kernel 5.9+).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 34)
    #define D_ENV_LINUX_GLIBC_HAS_CLOSE_RANGE 1
#else
    #define D_ENV_LINUX_GLIBC_HAS_CLOSE_RANGE 0
#endif


//==============================================================================
// 4.  KERNEL APIS
//==============================================================================


// 4.1    Events and notification
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_LINUX_HAS_EPOLL
//   feature: detect if epoll is available.
// epoll has been available since kernel 2.5.44; any glibc-based system
// on a 2.6+ kernel supports it.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL  ||                                               \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_EPOLL       1
#else
    #define D_ENV_LINUX_HAS_EPOLL       0
#endif

// 4.1.2
// D_ENV_LINUX_HAS_EVENTFD
//   feature: detect if eventfd/eventfd2 is available (kernel 2.6.22+,
// glibc 2.8+).
#if ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 8)  ||                                    \
      D_ENV_LINUX_LIBC_MUSL             ||                                     \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_EVENTFD     1
#else
    #define D_ENV_LINUX_HAS_EVENTFD     0
#endif

// 4.1.3
// D_ENV_LINUX_HAS_SIGNALFD
//   feature: detect if signalfd is available (kernel 2.6.22+,
// glibc 2.8+).
#if ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 8)  ||                                    \
      D_ENV_LINUX_LIBC_MUSL             ||                                     \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_SIGNALFD    1
#else
    #define D_ENV_LINUX_HAS_SIGNALFD    0
#endif

// 4.1.4
// D_ENV_LINUX_HAS_TIMERFD
//   feature: detect if timerfd_create/timerfd_settime is available
// (kernel 2.6.25+, glibc 2.8+).
#if ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 8)  ||                                    \
      D_ENV_LINUX_LIBC_MUSL             ||                                     \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_TIMERFD     1
#else
    #define D_ENV_LINUX_HAS_TIMERFD     0
#endif

// 4.1.5
// D_ENV_LINUX_HAS_INOTIFY
//   feature: detect if inotify (filesystem event monitoring) is available
// (kernel 2.6.13+).
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL  ||                                               \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_INOTIFY     1
#else
    #define D_ENV_LINUX_HAS_INOTIFY     0
#endif

// 4.1.6
// D_ENV_LINUX_HAS_FANOTIFY
//   feature: detect if fanotify is available (kernel 2.6.37+,
// glibc 2.13+).
#if ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 13) ||                                    \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_FANOTIFY    1
#else
    #define D_ENV_LINUX_HAS_FANOTIFY    0
#endif

// 4.2    io_uring
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_LINUX_HAS_IO_URING
//   feature: detect if io_uring is available.
// io_uring was introduced in kernel 5.1. compile-time detection checks
// for the liburing header or kernel header presence.
#if defined(LIBURING_H)
    // liburing is present (user has included <liburing.h>)
    #define D_ENV_LINUX_HAS_IO_URING    1
#elif D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_1)
    // kernel version is sufficient
    #define D_ENV_LINUX_HAS_IO_URING    1
#else
    #define D_ENV_LINUX_HAS_IO_URING    0
#endif

// 4.3    Memory management
//------------------------------------------------------------------------------
// 4.3.1
// D_ENV_LINUX_HAS_MEMFD_CREATE
//   feature: detect if memfd_create() is available (kernel 3.17+,
// glibc 2.27+).
#if ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 27) ||                                    \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_MEMFD_CREATE 1
#else
    #define D_ENV_LINUX_HAS_MEMFD_CREATE 0
#endif

// 4.3.2
// D_ENV_LINUX_HAS_MEMFD_SECRET
//   feature: detect if memfd_secret() is available (kernel 5.14+).
// creates a file descriptor backed by memory invisible to the kernel.
#if D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_14)
    #define D_ENV_LINUX_HAS_MEMFD_SECRET 1
#else
    #define D_ENV_LINUX_HAS_MEMFD_SECRET 0
#endif

// 4.3.3
// D_ENV_LINUX_HAS_MLOCK2
//   feature: detect if mlock2() is available (kernel 4.4+, glibc 2.27+).
// extends mlock() with flags (MLOCK_ONFAULT).
#if D_ENV_LINUX_GLIBC_AT_LEAST(2, 27)
    #define D_ENV_LINUX_HAS_MLOCK2      1
#else
    #define D_ENV_LINUX_HAS_MLOCK2      0
#endif

// 4.3.4
// D_ENV_LINUX_HAS_USERFAULTFD
//   feature: detect if userfaultfd() is available (kernel 4.3+).
// enables user-space page fault handling.
#if D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_4_3)
    #define D_ENV_LINUX_HAS_USERFAULTFD 1
#elif ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 21) &&                                  \
        !D_ENV_LINUX_KVER_DETECTED )
    // glibc has the wrapper; kernel version unknown — assume available
    // on modern systems
    #define D_ENV_LINUX_HAS_USERFAULTFD 1
#else
    #define D_ENV_LINUX_HAS_USERFAULTFD 0
#endif

// 4.4    Process and thread management
//------------------------------------------------------------------------------
// 4.4.1
// D_ENV_LINUX_HAS_CLONE3
//   feature: detect if clone3() syscall is available (kernel 5.3+).
#if D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_3)
    #define D_ENV_LINUX_HAS_CLONE3      1
#else
    #define D_ENV_LINUX_HAS_CLONE3      0
#endif

// 4.4.2
// D_ENV_LINUX_HAS_PIDFD
//   feature: detect if pidfd_open / pidfd_send_signal / pidfd_getfd
// are available (kernel 5.3+/5.6+).
#if D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_3)
    #define D_ENV_LINUX_HAS_PIDFD       1
#else
    #define D_ENV_LINUX_HAS_PIDFD       0
#endif

// 4.4.3
// D_ENV_LINUX_HAS_WAITID
//   feature: detect if waitid() is available (POSIX, glibc 2.1+).
// included here because Linux extends waitid with P_PIDFD.
#if D_ENV_LINUX_LIBC_GLIBC
    #define D_ENV_LINUX_HAS_WAITID      1
#else
    #define D_ENV_LINUX_HAS_WAITID      0
#endif

// 4.4.4
// D_ENV_LINUX_HAS_PRCTL
//   feature: detect if prctl() is available.
// prctl has been available since Linux 2.1.57.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL  ||                                               \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_PRCTL       1
#else
    #define D_ENV_LINUX_HAS_PRCTL       0
#endif

// 4.4.5
// D_ENV_LINUX_HAS_SCHED_DEADLINE
//   feature: detect if SCHED_DEADLINE scheduler policy is available
// (kernel 3.14+).
#if defined(SCHED_DEADLINE)
    #define D_ENV_LINUX_HAS_SCHED_DEADLINE 1
#elif D_ENV_LINUX_KVER_AT_LEAST(3, 14, 0)
    #define D_ENV_LINUX_HAS_SCHED_DEADLINE 1
#else
    #define D_ENV_LINUX_HAS_SCHED_DEADLINE 0
#endif

// 4.5    File and filesystem operations
//------------------------------------------------------------------------------
// 4.5.1
// D_ENV_LINUX_HAS_OPENAT2
//   feature: detect if openat2() syscall is available (kernel 5.6+).
// provides RESOLVE_* flags for race-free path resolution.
#if D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_6)
    #define D_ENV_LINUX_HAS_OPENAT2     1
#else
    #define D_ENV_LINUX_HAS_OPENAT2     0
#endif

// 4.5.2
// D_ENV_LINUX_HAS_STATX
//   feature: detect if statx() syscall is available (kernel 4.11+).
// provides extended file attributes (creation time, mount ID, etc.).
#if ( D_ENV_LINUX_GLIBC_HAS_STATX ||                                          \
      D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_4_11) )
    #define D_ENV_LINUX_HAS_STATX       1
#else
    #define D_ENV_LINUX_HAS_STATX       0
#endif

// 4.5.3
// D_ENV_LINUX_HAS_RENAMEAT2
//   feature: detect if renameat2() is available (kernel 3.15+).
// supports RENAME_NOREPLACE, RENAME_EXCHANGE, RENAME_WHITEOUT.
#if D_ENV_LINUX_KVER_AT_LEAST(3, 15, 0)
    #define D_ENV_LINUX_HAS_RENAMEAT2   1
#elif D_ENV_LINUX_GLIBC_AT_LEAST(2, 28)
    #define D_ENV_LINUX_HAS_RENAMEAT2   1
#else
    #define D_ENV_LINUX_HAS_RENAMEAT2   0
#endif

// 4.5.4
// D_ENV_LINUX_HAS_COPY_FILE_RANGE
//   feature: detect if copy_file_range() syscall is available
// (kernel 4.5+).
#if ( D_ENV_LINUX_GLIBC_HAS_COPY_FILE_RANGE ||                                \
      D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_4_5) )
    #define D_ENV_LINUX_HAS_COPY_FILE_RANGE 1
#else
    #define D_ENV_LINUX_HAS_COPY_FILE_RANGE 0
#endif

// 4.5.5
// D_ENV_LINUX_HAS_CLOSE_RANGE
//   feature: detect if close_range() is available (kernel 5.9+).
#if ( D_ENV_LINUX_GLIBC_HAS_CLOSE_RANGE ||                                    \
      D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_9) )
    #define D_ENV_LINUX_HAS_CLOSE_RANGE 1
#else
    #define D_ENV_LINUX_HAS_CLOSE_RANGE 0
#endif


//==============================================================================
// 5.  SECURITY FRAMEWORKS
//==============================================================================


// 5.1    seccomp
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_LINUX_HAS_SECCOMP
//   feature: detect if seccomp (secure computing mode) support is
// available. seccomp-bpf was introduced in kernel 3.5.
#if defined(SECCOMP_MODE_FILTER)
    #define D_ENV_LINUX_HAS_SECCOMP     1
#elif D_ENV_LINUX_KVER_AT_LEAST(3, 5, 0)
    #define D_ENV_LINUX_HAS_SECCOMP     1
#else
    // assume available on modern systems when kernel version is unknown
    #if ( D_ENV_LINUX_LIBC_GLIBC &&                                            \
          D_ENV_LINUX_GLIBC_AT_LEAST(2, 17) )
        #define D_ENV_LINUX_HAS_SECCOMP 1
    #else
        #define D_ENV_LINUX_HAS_SECCOMP 0
    #endif
#endif

// 5.2    Capabilities
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_LINUX_HAS_CAPABILITIES
//   feature: detect if POSIX capabilities are available.
// present since kernel 2.2; practically universal on modern Linux.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_CAPABILITIES 1
#else
    #define D_ENV_LINUX_HAS_CAPABILITIES 0
#endif

// 5.3    Namespaces
//------------------------------------------------------------------------------
// 5.3.1
// D_ENV_LINUX_HAS_NAMESPACES
//   feature: detect if Linux namespaces (mount, PID, network, user,
// UTS, IPC, cgroup, time) are available.
// the full set matured across kernels 2.6.24 through 5.6.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_NAMESPACES  1
#else
    #define D_ENV_LINUX_HAS_NAMESPACES  0
#endif

// 5.3.2
// D_ENV_LINUX_HAS_USER_NAMESPACES
//   feature: detect if user namespaces are available (kernel 3.8+).
// some distributions disable them; this is a compile-time hint only.
#if D_ENV_LINUX_KVER_AT_LEAST(3, 8, 0)
    #define D_ENV_LINUX_HAS_USER_NAMESPACES 1
#elif !D_ENV_LINUX_KVER_DETECTED
    // assume available on modern systems
    #define D_ENV_LINUX_HAS_USER_NAMESPACES 1
#else
    #define D_ENV_LINUX_HAS_USER_NAMESPACES 0
#endif

// 5.4    Landlock
//------------------------------------------------------------------------------
// 5.4.1
// D_ENV_LINUX_HAS_LANDLOCK
//   feature: detect if Landlock LSM is available (kernel 5.13+).
// unprivileged sandboxing framework.
#if defined(LANDLOCK_CREATE_RULESET_VERSION)
    #define D_ENV_LINUX_HAS_LANDLOCK    1
#elif D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_5_13)
    #define D_ENV_LINUX_HAS_LANDLOCK    1
#else
    #define D_ENV_LINUX_HAS_LANDLOCK    0
#endif


//==============================================================================
// 6.  FILESYSTEMS AND NETWORKING
//==============================================================================


// 6.1    Virtual filesystems
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_LINUX_HAS_PROCFS
//   feature: detect if /proc filesystem is expected to be available.
// procfs is effectively universal on non-embedded Linux.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_PROCFS      1
#else
    #define D_ENV_LINUX_HAS_PROCFS      0
#endif

// 6.1.2
// D_ENV_LINUX_HAS_SYSFS
//   feature: detect if /sys filesystem is expected to be available
// (kernel 2.6+).
#define D_ENV_LINUX_HAS_SYSFS          D_ENV_LINUX_HAS_PROCFS

// 6.1.3
// D_ENV_LINUX_HAS_CGROUPS_V1
//   feature: detect if cgroups v1 is expected to be available
// (kernel 2.6.24+).
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_CGROUPS_V1  1
#else
    #define D_ENV_LINUX_HAS_CGROUPS_V1  0
#endif

// 6.1.4
// D_ENV_LINUX_HAS_CGROUPS_V2
//   feature: detect if cgroups v2 (unified hierarchy) is expected to be
// available (kernel 4.5+, widely adopted 5.x+).
#if D_ENV_LINUX_KVER_AT_LEAST_HEX(D_ENV_LINUX_KVER_4_5)
    #define D_ENV_LINUX_HAS_CGROUPS_V2  1
#elif !D_ENV_LINUX_KVER_DETECTED
    // assume available on modern glibc systems
    #if D_ENV_LINUX_GLIBC_AT_LEAST(2, 28)
        #define D_ENV_LINUX_HAS_CGROUPS_V2 1
    #else
        #define D_ENV_LINUX_HAS_CGROUPS_V2 0
    #endif
#else
    #define D_ENV_LINUX_HAS_CGROUPS_V2  0
#endif

// 6.1.5
// D_ENV_LINUX_HAS_TMPFS
//   feature: tmpfs is effectively always available on Linux.
#define D_ENV_LINUX_HAS_TMPFS           1

// 6.1.6
// D_ENV_LINUX_HAS_DEVTMPFS
//   feature: detect if devtmpfs is expected (kernel 2.6.32+).
#define D_ENV_LINUX_HAS_DEVTMPFS        1

// 6.2    Networking
//------------------------------------------------------------------------------
// 6.2.1
// D_ENV_LINUX_HAS_NETLINK
//   feature: detect if Netlink sockets are available (kernel 2.2+).
// used for kernel-userspace IPC: routing, firewall, audit, etc.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL  ||                                               \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_NETLINK     1
#else
    #define D_ENV_LINUX_HAS_NETLINK     0
#endif

// 6.2.2
// D_ENV_LINUX_HAS_UNIX_DIAG
//   feature: detect if UNIX domain socket diagnostics (sock_diag) are
// available (kernel 3.3+).
#if D_ENV_LINUX_KVER_AT_LEAST(3, 3, 0)
    #define D_ENV_LINUX_HAS_UNIX_DIAG   1
#elif !D_ENV_LINUX_KVER_DETECTED
    #define D_ENV_LINUX_HAS_UNIX_DIAG   1
#else
    #define D_ENV_LINUX_HAS_UNIX_DIAG   0
#endif

// 6.2.3
// D_ENV_LINUX_HAS_SPLICE
//   feature: detect if splice / tee / vmsplice are available
// (kernel 2.6.17+).
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_SPLICE      1
#else
    #define D_ENV_LINUX_HAS_SPLICE      0
#endif

// 6.2.4
// D_ENV_LINUX_HAS_SO_REUSEPORT
//   feature: detect if SO_REUSEPORT socket option is available
// (kernel 3.9+).
#if defined(SO_REUSEPORT)
    #define D_ENV_LINUX_HAS_SO_REUSEPORT 1
#elif D_ENV_LINUX_KVER_AT_LEAST(3, 9, 0)
    #define D_ENV_LINUX_HAS_SO_REUSEPORT 1
#elif !D_ENV_LINUX_KVER_DETECTED
    #define D_ENV_LINUX_HAS_SO_REUSEPORT 1
#else
    #define D_ENV_LINUX_HAS_SO_REUSEPORT 0
#endif


//==============================================================================
// 7.  DESKTOP AND INIT SYSTEM
//==============================================================================
// note: display server detection is inherently runtime-dependent. these
// macros detect compile-time linkage and header availability, not the actual
// running display server.


// 7.1    Display server and desktop
//------------------------------------------------------------------------------
// 7.1.1
// D_ENV_LINUX_HAS_X11
//   feature: detect if X11/Xlib development headers are available.
#if ( defined(_X11_XLIB_H_)  ||                                               \
      defined(_X11_X_H_)     ||                                                \
      defined(_XLIB_H_) )
    #define D_ENV_LINUX_HAS_X11         1
#else
    #define D_ENV_LINUX_HAS_X11         0
#endif

// 7.1.2
// D_ENV_LINUX_HAS_XCB
//   feature: detect if XCB (X protocol C-language Binding) headers are
// available.
#if defined(__XCB_H__)
    #define D_ENV_LINUX_HAS_XCB         1
#else
    #define D_ENV_LINUX_HAS_XCB         0
#endif

// 7.1.3
// D_ENV_LINUX_HAS_WAYLAND
//   feature: detect if Wayland client development headers are available.
#if ( defined(__wayland_client_h)      ||                                      \
      defined(WAYLAND_CLIENT_H)        ||                                      \
      defined(__wayland_client_core_h) )
    #define D_ENV_LINUX_HAS_WAYLAND     1
#else
    #define D_ENV_LINUX_HAS_WAYLAND     0
#endif

// 7.1.4
// D_ENV_LINUX_HAS_DRM
//   feature: detect if DRM (Direct Rendering Manager) headers are
// available.
#if defined(__DRM_H__)
    #define D_ENV_LINUX_HAS_DRM         1
#else
    #define D_ENV_LINUX_HAS_DRM         0
#endif

// 7.1.5
// D_ENV_LINUX_HAS_KMS
//   feature: detect if KMS (Kernel Mode Setting) headers are available.
#if ( defined(__DRM_MODE_H__)  ||                                              \
      defined(_DRM_MODE_H) )
    #define D_ENV_LINUX_HAS_KMS         1
#else
    #define D_ENV_LINUX_HAS_KMS         0
#endif

// 7.1.6
// D_ENV_LINUX_HAS_GBM
//   feature: detect if GBM (Generic Buffer Management) is available.
#if defined(__GBM_H__)
    #define D_ENV_LINUX_HAS_GBM         1
#else
    #define D_ENV_LINUX_HAS_GBM         0
#endif

// 7.1.7
// D_ENV_LINUX_HAS_EGL
//   feature: detect if EGL headers are available.
#if ( defined(__egl_h_)  ||                                                    \
      defined(EGL_EGL_H) )
    #define D_ENV_LINUX_HAS_EGL         1
#else
    #define D_ENV_LINUX_HAS_EGL         0
#endif

// 7.2    Init system and service manager
//------------------------------------------------------------------------------
// 7.2.1
// D_ENV_LINUX_HAS_SYSTEMD
//   feature: detect if systemd development headers (sd-daemon, sd-bus,
// sd-journal) are available.
#if ( defined(SD_LISTEN_FDS_START)  ||                                         \
      defined(_SD_COMMON_H)        ||                                          \
      defined(SD_ID128_NULL) )
    #define D_ENV_LINUX_HAS_SYSTEMD     1
#else
    #define D_ENV_LINUX_HAS_SYSTEMD     0
#endif

// 7.2.2
// D_ENV_LINUX_HAS_SD_BUS
//   feature: detect if sd-bus (systemd D-Bus library) headers are
// available.
#if defined(_SD_BUS_H)
    #define D_ENV_LINUX_HAS_SD_BUS      1
#else
    #define D_ENV_LINUX_HAS_SD_BUS      0
#endif

// 7.2.3
// D_ENV_LINUX_HAS_SD_JOURNAL
//   feature: detect if sd-journal (systemd journal API) headers are
// available.
#if defined(SD_JOURNAL_H)
    #define D_ENV_LINUX_HAS_SD_JOURNAL  1
#else
    #define D_ENV_LINUX_HAS_SD_JOURNAL  0
#endif


//==============================================================================
// 8.  HEADERS AND TOOLCHAIN
//==============================================================================


// 8.1    Linux-specific headers
//------------------------------------------------------------------------------
// 8.1.1
// D_ENV_LINUX_HAS_SYS_EPOLL_H
//   feature: sys/epoll.h is available on all modern Linux.
#define D_ENV_LINUX_HAS_SYS_EPOLL_H    D_ENV_LINUX_HAS_EPOLL

// 8.1.2
// D_ENV_LINUX_HAS_SYS_INOTIFY_H
//   feature: sys/inotify.h is available on all modern Linux.
#define D_ENV_LINUX_HAS_SYS_INOTIFY_H  D_ENV_LINUX_HAS_INOTIFY

// 8.1.3
// D_ENV_LINUX_HAS_SYS_EVENTFD_H
//   feature: sys/eventfd.h is available when eventfd is supported.
#define D_ENV_LINUX_HAS_SYS_EVENTFD_H  D_ENV_LINUX_HAS_EVENTFD

// 8.1.4
// D_ENV_LINUX_HAS_SYS_SIGNALFD_H
//   feature: sys/signalfd.h is available when signalfd is supported.
#define D_ENV_LINUX_HAS_SYS_SIGNALFD_H D_ENV_LINUX_HAS_SIGNALFD

// 8.1.5
// D_ENV_LINUX_HAS_SYS_TIMERFD_H
//   feature: sys/timerfd.h is available when timerfd is supported.
#define D_ENV_LINUX_HAS_SYS_TIMERFD_H  D_ENV_LINUX_HAS_TIMERFD

// 8.1.6
// D_ENV_LINUX_HAS_SYS_SENDFILE_H
//   feature: detect if sys/sendfile.h (zero-copy file transfer) is
// available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_SYS_SENDFILE_H 1
#else
    #define D_ENV_LINUX_HAS_SYS_SENDFILE_H 0
#endif

// 8.1.7
// D_ENV_LINUX_HAS_SYS_PRCTL_H
//   feature: sys/prctl.h is available when prctl is supported.
#define D_ENV_LINUX_HAS_SYS_PRCTL_H    D_ENV_LINUX_HAS_PRCTL

// 8.1.8
// D_ENV_LINUX_HAS_SYS_CAPABILITY_H
//   feature: detect if sys/capability.h (libcap) is available.
// note: this requires the libcap-dev package; the header guard is
// checked if the header has been included.
#if defined(_SYS_CAPABILITY_H)
    #define D_ENV_LINUX_HAS_SYS_CAPABILITY_H 1
#else
    #define D_ENV_LINUX_HAS_SYS_CAPABILITY_H 0
#endif

// 8.1.9
// D_ENV_LINUX_HAS_SYS_SYSINFO_H
//   feature: detect if sys/sysinfo.h (sysinfo struct) is available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL  ||                                               \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_SYS_SYSINFO_H 1
#else
    #define D_ENV_LINUX_HAS_SYS_SYSINFO_H 0
#endif

// 8.1.10
// D_ENV_LINUX_HAS_LINUX_FUTEX_H
//   feature: detect if linux/futex.h is available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_LINUX_FUTEX_H 1
#else
    #define D_ENV_LINUX_HAS_LINUX_FUTEX_H 0
#endif

// 8.1.11
// D_ENV_LINUX_HAS_LINUX_SECCOMP_H
//   feature: detect if linux/seccomp.h is available.
#define D_ENV_LINUX_HAS_LINUX_SECCOMP_H D_ENV_LINUX_HAS_SECCOMP

// 8.1.12
// D_ENV_LINUX_HAS_LINUX_LANDLOCK_H
//   feature: detect if linux/landlock.h is available.
#define D_ENV_LINUX_HAS_LINUX_LANDLOCK_H D_ENV_LINUX_HAS_LANDLOCK

// 8.1.13
// D_ENV_LINUX_HAS_LINUX_IO_URING_H
//   feature: detect if linux/io_uring.h is available.
#define D_ENV_LINUX_HAS_LINUX_IO_URING_H D_ENV_LINUX_HAS_IO_URING

// 8.1.14
// D_ENV_LINUX_HAS_DLFCN_H
//   feature: detect if dlfcn.h (dynamic loading) is available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL  ||                                               \
      D_ENV_LINUX_LIBC_BIONIC )
    #define D_ENV_LINUX_HAS_DLFCN_H     1
#else
    #define D_ENV_LINUX_HAS_DLFCN_H     0
#endif

// 8.1.15
// D_ENV_LINUX_HAS_ELF_H
//   feature: detect if elf.h (ELF format definitions) is available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_ELF_H       1
#else
    #define D_ENV_LINUX_HAS_ELF_H       0
#endif

// 8.1.16
// D_ENV_LINUX_HAS_LINK_H
//   feature: detect if link.h (dynamic linker interface) is available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_LINK_H      1
#else
    #define D_ENV_LINUX_HAS_LINK_H      0
#endif

// 8.1.17
// D_ENV_LINUX_HAS_SYS_XATTR_H
//   feature: detect if sys/xattr.h (extended attributes) is available.
#if ( D_ENV_LINUX_LIBC_GLIBC ||                                               \
      D_ENV_LINUX_LIBC_MUSL )
    #define D_ENV_LINUX_HAS_SYS_XATTR_H 1
#else
    #define D_ENV_LINUX_HAS_SYS_XATTR_H 0
#endif

// 8.2    Compiler and toolchain
//------------------------------------------------------------------------------
// 8.2.1
// D_ENV_LINUX_IS_GCC
//   feature: detect if building with GCC on Linux.
#if ( defined(__GNUC__) &&                                                     \
      (!defined(__clang__)) )
    #define D_ENV_LINUX_IS_GCC          1
#else
    #define D_ENV_LINUX_IS_GCC          0
#endif

// 8.2.2
// D_ENV_LINUX_IS_CLANG
//   feature: detect if building with Clang on Linux.
#if defined(__clang__)
    #define D_ENV_LINUX_IS_CLANG        1
#else
    #define D_ENV_LINUX_IS_CLANG        0
#endif

// 8.2.3
// D_ENV_LINUX_HAS_SANITIZERS
//   feature: detect if address/thread/undefined sanitizers are active.
#if ( defined(__SANITIZE_ADDRESS__)  ||                                        \
      defined(__SANITIZE_THREAD__) )
    #define D_ENV_LINUX_HAS_SANITIZERS  1
#elif ( defined(__has_feature) )
    #if ( __has_feature(address_sanitizer) ||                                 \
          __has_feature(thread_sanitizer)  ||                                 \
          __has_feature(undefined_behavior_sanitizer) )
        #define D_ENV_LINUX_HAS_SANITIZERS 1
    #else
        #define D_ENV_LINUX_HAS_SANITIZERS 0
    #endif
#else
    #define D_ENV_LINUX_HAS_SANITIZERS  0
#endif

// 8.2.4
// D_ENV_LINUX_HAS_STACK_PROTECTOR
//   feature: detect if stack protector (stack canary / SSP) is enabled.
#if defined(__SSP__)
    #define D_ENV_LINUX_HAS_STACK_PROTECTOR 1
#elif ( defined(__SSP_STRONG__)  ||                                            \
        defined(__SSP_ALL__) )
    #define D_ENV_LINUX_HAS_STACK_PROTECTOR 1
#else
    #define D_ENV_LINUX_HAS_STACK_PROTECTOR 0
#endif

// 8.2.5
// D_ENV_LINUX_HAS_PIE
//   feature: detect if Position Independent Executable is being built.
#if defined(__PIE__)
    #define D_ENV_LINUX_HAS_PIE         1
    #define D_ENV_LINUX_PIE_LEVEL       __PIE__
#else
    #define D_ENV_LINUX_HAS_PIE         0
    #define D_ENV_LINUX_PIE_LEVEL       0
#endif

// 8.2.6
// D_ENV_LINUX_HAS_RELRO
//   feature: detect if RELRO (RELocation Read-Only) is requested.
// note: compile-time detection is limited. full RELRO is a linker
// option (-Wl,-z,relro,-z,now); this checks for common indicators.
#if ( D_ENV_LINUX_HAS_PIE &&                                                  \
      D_ENV_LINUX_HAS_STACK_PROTECTOR )
    // likely a hardened build; RELRO is probable but not guaranteed
    #define D_ENV_LINUX_LIKELY_RELRO    1
#else
    #define D_ENV_LINUX_LIKELY_RELRO    0
#endif


//==============================================================================
// 9.  RUNTIME DETECTION AND CONVENIENCE
//==============================================================================


// 9.1    Runtime queries
//------------------------------------------------------------------------------
//   C linkage for C++ callers. The env headers sit below djinterp.h, so the
// D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
#if D_ENV_LANG_USING_CPP
    extern "C" {
#endif

/**
 * @brief Returns the running kernel's version string.
 *
 * @return uname's release string (e.g. "6.1.0-23-amd64"), or "Unknown" if
 *         uname fails.
 */
const char* d_env_linux_get_kernel_version(void);
/**
 * @brief Returns the distribution's name, read from /etc/os-release at
 *        runtime.
 *
 * @return the distribution's pretty name, or "Unknown" if it cannot be
 *         determined.
 */
const char* d_env_linux_get_distro_name(void);
/**
 * @brief Returns the running C library's version string.
 *
 * @return gnu_get_libc_version()'s string under glibc, or a heuristic result
 *         for other C libraries.
 */
const char* d_env_linux_get_libc_version(void);
/**
 * @brief Tests at runtime whether the kernel supports a system call.
 *
 * @param[in] _syscall_nr  the system call's number, from <sys/syscall.h>.
 * @return `1` if the system call is supported, `0` otherwise.
 */
int         d_env_linux_has_syscall(long _syscall_nr);
/**
 * @brief Tests at runtime whether io_uring is available, by probing the
 *        io_uring_setup system call.
 *
 * @return `1` if io_uring is supported, `0` otherwise.
 */
int         d_env_linux_has_io_uring(void);
/**
 * @brief Detects the active cgroup version at runtime.
 *
 * @return `2` for cgroups v2 (unified), `1` for cgroups v1, or `0` if
 *         detection fails.
 */
int         d_env_linux_get_cgroup_version(void);
/**
 * @brief Prints detailed information about the detected Linux environment:
 *        kernel version, distribution, C library, security features, and
 *        available APIs.
 */
void        d_env_linux_print_info(void);

#if D_ENV_LANG_USING_CPP
    }
#endif

// 9.2    Combined predicates
//------------------------------------------------------------------------------
// 9.2.1
// D_ENV_LINUX_IS_MODERN
//   macro: evaluates to 1 if the build environment represents a
// "modern" Linux (glibc 2.17+ or musl, roughly RHEL 7+ era).
#define D_ENV_LINUX_IS_MODERN()                                                \
    ( D_ENV_LINUX_GLIBC_AT_LEAST(2, 17) ||                                     \
      D_ENV_LINUX_LIBC_MUSL )

// 9.2.2
// D_ENV_LINUX_IS_EMBEDDED
//   macro: evaluates to 1 if the build environment suggests an
// embedded Linux (uClibc or Bionic without glibc).
#define D_ENV_LINUX_IS_EMBEDDED()                                              \
    ( D_ENV_LINUX_LIBC_UCLIBC ||                                               \
      ( D_ENV_LINUX_LIBC_BIONIC && !D_ENV_LINUX_LIBC_GLIBC ) )

// 9.2.3
// D_ENV_LINUX_IS_HARDENED
//   macro: evaluates to 1 if common hardening features are detected
// (PIE, stack protector, FORTIFY_SOURCE).
#define D_ENV_LINUX_IS_HARDENED()                                              \
    ( D_ENV_LINUX_HAS_PIE              &&                                      \
      D_ENV_LINUX_HAS_STACK_PROTECTOR  &&                                      \
      (D_ENV_LINUX_HAS_FORTIFY_SOURCE >= 1) )

// 9.2.4
// D_ENV_LINUX_HAS_MODERN_IO
//   macro: evaluates to 1 if modern async I/O primitives are available
// (epoll + eventfd + timerfd + signalfd).
#define D_ENV_LINUX_HAS_MODERN_IO()                                            \
    ( D_ENV_LINUX_HAS_EPOLL    &&                                              \
      D_ENV_LINUX_HAS_EVENTFD  &&                                              \
      D_ENV_LINUX_HAS_TIMERFD  &&                                              \
      D_ENV_LINUX_HAS_SIGNALFD )

// 9.2.5
// D_ENV_LINUX_HAS_SANDBOXING
//   macro: evaluates to 1 if comprehensive sandboxing primitives are
// available (seccomp + namespaces + capabilities).
#define D_ENV_LINUX_HAS_SANDBOXING()                                           \
    ( D_ENV_LINUX_HAS_SECCOMP      &&                                          \
      D_ENV_LINUX_HAS_NAMESPACES   &&                                          \
      D_ENV_LINUX_HAS_CAPABILITIES )


#endif  // DJINTERP_ENV_OS_ENV_LINUX_H
