/*******************************************************************************
* djinterp [env]                                                     env_c_lib.h
*
* djinterp C standard-library feature detection.
*   Compile-time detection of C standard-library features, POSIX headers,
* threading support, SIMD intrinsics, and other platform-specific runtime
* capabilities. Despite the D_ENV_C_HAS_* naming, these flags describe the C
* runtime that both C and C++ translation units rely on, so the block is gated
* on hosted versus freestanding (__STDC_HOSTED__), not on the source language.
* It covers:
*     - C standard-library headers and feature detection
*     - POSIX header and function availability
*     - threading, networking, file I/O, and memory management
*     - SIMD intrinsic detection (SSE, AVX, NEON)
*   Naming: D_ENV_C_HAS_<FEATURE> is 1 if available, 0 otherwise.
*   It reads the D_ENV_LANG_*, D_ENV_OS_*, D_ENV_ARCH_*, D_ENV_COMPILER_*, and
* D_ENV_IS_OS_POSIX_LIKE* families, and includes the four sections that define
* them as well as cfg_env.h, so it gives the same answers whether a unit
* includes it directly or through env.h.
*
*
* path:      /inc/djinterp/env/c/env_c_lib.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.08
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THREADING AND CONCURRENCY
    -------------------------
    1.  Threads and atomics
         1.  D_ENV_C_HAS_C11_THREADS
         2.  D_ENV_C_HAS_PTHREAD
         3.  D_ENV_C_HAS_WINDOWS_THREADS
         4.  D_ENV_C_HAS_STDATOMIC
2.  HEADERS
    -------
    1.  Standard headers
         1.  D_ENV_C_HAS_STDBOOL_H
         2.  D_ENV_C_HAS_STDINT_H
         3.  D_ENV_C_HAS_INTTYPES_H
         4.  D_ENV_C_HAS_STDALIGN_H
         5.  D_ENV_C_HAS_UCHAR_H
    2.  POSIX headers
         1.  D_ENV_C_HAS_UNISTD_H
         2.  D_ENV_C_HAS_SYS_TYPES_H
         3.  D_ENV_C_HAS_SYS_STAT_H
         4.  D_ENV_C_HAS_DIRENT_H
3.  LIBRARY FUNCTIONS
    -----------------
    1.  Strings and memory
         1.  D_ENV_C_HAS_STRTOK_R
         2.  D_ENV_C_HAS_STRTOK_S
         3.  D_ENV_C_HAS_SNPRINTF
         4.  D_ENV_C_HAS_STRDUP
         5.  D_ENV_C_HAS_STRNDUP
         6.  D_ENV_C_HAS_STRCASECMP
         7.  D_ENV_C_HAS_STRICMP
         8.  D_ENV_C_HAS_MEMCCPY
    2.  File system and I/O
         1.  D_ENV_C_HAS_FLOCK
         2.  D_ENV_C_HAS_FOPEN_S
         3.  D_ENV_C_HAS_FSYNC
         4.  D_ENV_C_HAS_LOCKFILE
         5.  D_ENV_C_HAS_MMAP
         6.  D_ENV_C_HAS_SCANF_S
    3.  Time and date
         1.  D_ENV_C_HAS_TIMESPEC_GET
         2.  D_ENV_C_HAS_CLOCK_GETTIME
         3.  D_ENV_C_HAS_GETTIMEOFDAY
         4.  D_ENV_C_HAS_QUERYPERFORMANCECOUNTER
    4.  Math
         1.  D_ENV_C_HAS_TGMATH_H
         2.  D_ENV_C_HAS_COMPLEX_H
         3.  D_ENV_C_HAS_FENV_H
4.  NETWORKING, PROCESSES, AND MEMORY
    ---------------------------------
    1.  Networking
         1.  D_ENV_C_HAS_WINSOCK
         2.  D_ENV_C_HAS_BSD_SOCKETS
         3.  D_ENV_C_HAS_GETADDRINFO
    2.  Processes and signals
         1.  D_ENV_C_HAS_FORK
         2.  D_ENV_C_HAS_EXECVE
         3.  D_ENV_C_HAS_GETPID
         4.  D_ENV_C_HAS_SIGNAL_H
    3.  Memory management
         1.  D_ENV_C_HAS_ALIGNED_ALLOC
         2.  D_ENV_C_HAS_POSIX_MEMALIGN
         3.  D_ENV_C_HAS_ALIGNED_MALLOC
         4.  D_ENV_C_HAS_ALLOCA
5.  HARDWARE AND LANGUAGE FEATURES
    ------------------------------
    1.  SIMD intrinsics
         1.  D_ENV_C_HAS_SSE
         2.  D_ENV_C_HAS_SSE2
         3.  D_ENV_C_HAS_AVX
         4.  D_ENV_C_HAS_AVX2
         5.  D_ENV_C_HAS_NEON
    2.  Variable-length arrays
         1.  D_ENV_C_HAS_VLA
6.  SECURITY
    --------
    1.  Security functions
         1.  D_ENV_C_HAS_SECURE_STRING_LIB
         2.  D_ENV_C_HAS_GETENTROPY
*/

#ifndef DJINTERP_ENV_C_ENV_C_LIB_H
#define DJINTERP_ENV_C_ENV_C_LIB_H 1

// djinterp
#include "../../config/core/env/cfg_env.h"  // D_CFG_ENV_ISO_STRICT
#include "../env_lang.h"                    // D_ENV_LANG_IS_*_OR_HIGHER
#include "../env_arch.h"                    // D_ENV_ARCH_TYPE
#include "../env_os.h"                      // D_ENV_OS_ID, D_ENV_IS_OS_*
#include "../env_compiler.h"                // D_ENV_COMPILER_*


#ifdef __STDC_HOSTED__

//==============================================================================
// 1.  THREADING AND CONCURRENCY
//==============================================================================


// 1.1    Threads and atomics
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_C_HAS_C11_THREADS
//   feature: detect if we can use C11 threads.h
#ifndef D_ENV_C_HAS_C11_THREADS
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #if (!defined(__STDC_NO_THREADS__))
            #define D_ENV_C_HAS_C11_THREADS 1
        #else
            #define D_ENV_C_HAS_C11_THREADS 0
        #endif
    #else
        #define D_ENV_C_HAS_C11_THREADS 0
    #endif
#endif  // D_ENV_C_HAS_C11_THREADS

// 1.1.2
// D_ENV_C_HAS_PTHREAD
//   feature: detect if POSIX threads (pthreads) are available
#ifndef D_ENV_C_HAS_PTHREAD
    #if D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(D_ENV_OS_ID)
        #define D_ENV_C_HAS_PTHREAD 1
    #else
        #define D_ENV_C_HAS_PTHREAD 0
    #endif
#endif  // D_ENV_C_HAS_PTHREAD

// 1.1.3
// D_ENV_C_HAS_WINDOWS_THREADS
//   feature: detect Windows threading API
#ifndef D_ENV_C_HAS_WINDOWS_THREADS
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_WINDOWS_THREADS 1
    #else
        #define D_ENV_C_HAS_WINDOWS_THREADS 0
    #endif
#endif  // D_ENV_C_HAS_WINDOWS_THREADS

// 1.1.4
// D_ENV_C_HAS_STDATOMIC
//   feature: detect if we can use C11 stdatomic.h
#ifndef D_ENV_C_HAS_STDATOMIC
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        // check if stdatomic.h is actually available
        #if !defined(__STDC_NO_ATOMICS__)
            #define D_ENV_C_HAS_STDATOMIC 1
        #else
            #define D_ENV_C_HAS_STDATOMIC 0
        #endif
    #elif D_ENV_LANG_IS_CPP11_OR_HIGHER
        // C++11 and later have atomic support
        #define D_ENV_C_HAS_STDATOMIC 1
    #else
        #define D_ENV_C_HAS_STDATOMIC 0
    #endif
#endif  // D_ENV_C_HAS_STDATOMIC


//==============================================================================
// 2.  HEADERS
//==============================================================================


// 2.1    Standard headers
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_C_HAS_STDBOOL_H
//   feature: detect if stdbool.h is available (C99+)
#ifndef D_ENV_C_HAS_STDBOOL_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_STDBOOL_H 1
    #else
        #define D_ENV_C_HAS_STDBOOL_H 0
    #endif
#endif  // D_ENV_C_HAS_STDBOOL_H

// 2.1.2
// D_ENV_C_HAS_STDINT_H
//   feature: detect if stdint.h is available. It is part of C99 and C++11,
// freestanding implementations included. Below those it is a platform header:
// found with __has_include where the preprocessor has it, and assumed on GCC
// and on MSVC from Visual Studio 2010, which ship one. D_CFG_ENV_ISO_STRICT
// reports it absent below C99 and C++11, where it is not ISO.
#ifndef D_ENV_C_HAS_STDINT_H
    #if ( (D_ENV_LANG_IS_C99_OR_HIGHER) ||                                     \
          (D_ENV_LANG_IS_CPP11_OR_HIGHER) )
        #define D_ENV_C_HAS_STDINT_H 1
    #elif D_CFG_IS_ON(D_CFG_ENV_ISO_STRICT)
        #define D_ENV_C_HAS_STDINT_H 0
    #elif defined(__has_include)
        #if __has_include(<stdint.h>)
            #define D_ENV_C_HAS_STDINT_H 1
        #else
            #define D_ENV_C_HAS_STDINT_H 0
        #endif
    #elif ( (defined(D_ENV_COMPILER_GCC))     ||                               \
            ( (defined(D_ENV_COMPILER_MSVC)) &&                                \
              (D_ENV_COMPILER_MAJOR >= 10) ) )
        #define D_ENV_C_HAS_STDINT_H 1
    #else
        #define D_ENV_C_HAS_STDINT_H 0
    #endif
#endif  // D_ENV_C_HAS_STDINT_H

// 2.1.3
// D_ENV_C_HAS_INTTYPES_H
//   feature: detect if inttypes.h is available, on the same terms as
// D_ENV_C_HAS_STDINT_H, except that no freestanding implementation has to
// provide it, so none is taken to; pre-define this to 1 for one that does.
// __has_include cannot settle that case: Clang's own <inttypes.h> is found
// even where the C library's, which it includes in turn, is missing. MSVC
// ships the header from Visual Studio 2013.
#ifndef D_ENV_C_HAS_INTTYPES_H
    #if ( (defined(__STDC_HOSTED__)) &&                                        \
          (__STDC_HOSTED__ == 0) )
        #define D_ENV_C_HAS_INTTYPES_H 0
    #elif ( (D_ENV_LANG_IS_C99_OR_HIGHER) ||                                   \
            (D_ENV_LANG_IS_CPP11_OR_HIGHER) )
        #define D_ENV_C_HAS_INTTYPES_H 1
    #elif D_CFG_IS_ON(D_CFG_ENV_ISO_STRICT)
        #define D_ENV_C_HAS_INTTYPES_H 0
    #elif defined(__has_include)
        #if __has_include(<inttypes.h>)
            #define D_ENV_C_HAS_INTTYPES_H 1
        #else
            #define D_ENV_C_HAS_INTTYPES_H 0
        #endif
    #elif ( (defined(D_ENV_COMPILER_GCC))     ||                               \
            ( (defined(D_ENV_COMPILER_MSVC)) &&                                \
              (D_ENV_COMPILER_MAJOR >= 12) ) )
        #define D_ENV_C_HAS_INTTYPES_H 1
    #else
        #define D_ENV_C_HAS_INTTYPES_H 0
    #endif
#endif  // D_ENV_C_HAS_INTTYPES_H

// 2.1.4
// D_ENV_C_HAS_STDALIGN_H
//   feature: detect if stdalign.h is available (C11+; deprecated in C23
// where alignof/alignas are keywords, but header still exists)
#ifndef D_ENV_C_HAS_STDALIGN_H
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #define D_ENV_C_HAS_STDALIGN_H 1
    #else
        #define D_ENV_C_HAS_STDALIGN_H 0
    #endif
#endif  // D_ENV_C_HAS_STDALIGN_H

// 2.1.5
// D_ENV_C_HAS_UCHAR_H
//   feature: detect if uchar.h is available (C11+)
#ifndef D_ENV_C_HAS_UCHAR_H
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #define D_ENV_C_HAS_UCHAR_H 1
    #else
        #define D_ENV_C_HAS_UCHAR_H 0
    #endif
#endif  // D_ENV_C_HAS_UCHAR_H

// 2.2    POSIX headers
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_C_HAS_UNISTD_H
//   feature: detect if unistd.h is available (POSIX systems)
#ifndef D_ENV_C_HAS_UNISTD_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(D_ENV_OS_ID)
        #define D_ENV_C_HAS_UNISTD_H 1
    #else
        #define D_ENV_C_HAS_UNISTD_H 0
    #endif
#endif  // D_ENV_C_HAS_UNISTD_H

// 2.2.2
// D_ENV_C_HAS_SYS_TYPES_H
//   feature: detect if sys/types.h is available
#ifndef D_ENV_C_HAS_SYS_TYPES_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_SYS_TYPES_H 1
    #else
        #define D_ENV_C_HAS_SYS_TYPES_H 0
    #endif
#endif  // D_ENV_C_HAS_SYS_TYPES_H

// 2.2.3
// D_ENV_C_HAS_SYS_STAT_H
//   feature: detect if sys/stat.h is available
#ifndef D_ENV_C_HAS_SYS_STAT_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_SYS_STAT_H 1
    #else
        #define D_ENV_C_HAS_SYS_STAT_H 0
    #endif
#endif  // D_ENV_C_HAS_SYS_STAT_H

// 2.2.4
// D_ENV_C_HAS_DIRENT_H
//   feature: detect if dirent.h is available for directory operations
#ifndef D_ENV_C_HAS_DIRENT_H
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_DIRENT_H 1
    #else
        #define D_ENV_C_HAS_DIRENT_H 0
    #endif
#endif  // D_ENV_C_HAS_DIRENT_H


//==============================================================================
// 3.  LIBRARY FUNCTIONS
//==============================================================================


// 3.1    Strings and memory
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_C_HAS_STRTOK_R
//   feature: detect if strtok_r (reentrant strtok) is available
#ifndef D_ENV_C_HAS_STRTOK_R
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_STRTOK_R 1
    #else
        #define D_ENV_C_HAS_STRTOK_R 0
    #endif
#endif  // D_ENV_C_HAS_STRTOK_R

// 3.1.2
// D_ENV_C_HAS_STRTOK_S
//   feature: detect if strtok_s (C11 Annex K / MSVC reentrant strtok)
// is available
#ifndef D_ENV_C_HAS_STRTOK_S
    #if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) ||                                \
          (defined(D_ENV_COMPILER_MSVC))     ||                                \
          (defined(__STDC_LIB_EXT1__)) )
        #define D_ENV_C_HAS_STRTOK_S 1
    #else
        #define D_ENV_C_HAS_STRTOK_S 0
    #endif
#endif  // D_ENV_C_HAS_STRTOK_S

// 3.1.3
// D_ENV_C_HAS_SNPRINTF
//   feature: detect if snprintf or a close equivalent is available (C99+)
#ifndef D_ENV_C_HAS_SNPRINTF
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_SNPRINTF 1
    #elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        // Windows has _snprintf (not fully C99-conforming; conforming
        // snprintf available in MSVC 2015+ / _MSC_VER >= 1900)
        #define D_ENV_C_HAS_SNPRINTF 1
    #else
        #define D_ENV_C_HAS_SNPRINTF 0
    #endif
#endif  // D_ENV_C_HAS_SNPRINTF

// 3.1.4
// D_ENV_C_HAS_STRDUP
//   feature: detect if strdup is available (POSIX, standardized in C23)
#ifndef D_ENV_C_HAS_STRDUP
    #if ( (D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)) ||                             \
          (D_ENV_LANG_IS_C23_OR_HIGHER) )
        #define D_ENV_C_HAS_STRDUP 1
    #else
        #define D_ENV_C_HAS_STRDUP 0
    #endif
#endif  // D_ENV_C_HAS_STRDUP

// 3.1.5
// D_ENV_C_HAS_STRNDUP
//   feature: detect if strndup is available (POSIX, standardized in C23)
#ifndef D_ENV_C_HAS_STRNDUP
    #if ( (D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)) ||                             \
          (D_ENV_LANG_IS_C23_OR_HIGHER) )
        #define D_ENV_C_HAS_STRNDUP 1
    #else
        #define D_ENV_C_HAS_STRNDUP 0
    #endif
#endif  // D_ENV_C_HAS_STRNDUP

// 3.1.6
// D_ENV_C_HAS_STRCASECMP
//   feature: detect if strcasecmp is available (POSIX)
#ifndef D_ENV_C_HAS_STRCASECMP
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_STRCASECMP 1
    #else
        #define D_ENV_C_HAS_STRCASECMP 0
    #endif
#endif  // D_ENV_C_HAS_STRCASECMP

// 3.1.7
// D_ENV_C_HAS_STRICMP
//   feature: detect if _stricmp is available (Windows)
#ifndef D_ENV_C_HAS_STRICMP
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_STRICMP 1
    #else
        #define D_ENV_C_HAS_STRICMP 0
    #endif
#endif  // D_ENV_C_HAS_STRICMP

// 3.1.8
// D_ENV_C_HAS_MEMCCPY
//   feature: detect if memccpy is available (POSIX)
#ifndef D_ENV_C_HAS_MEMCCPY
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_MEMCCPY 1
    #else
        #define D_ENV_C_HAS_MEMCCPY 0
    #endif
#endif  // D_ENV_C_HAS_MEMCCPY

// 3.2    File system and I/O
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_C_HAS_FLOCK
//   feature: detect if flock (file locking) is available
#ifndef D_ENV_C_HAS_FLOCK
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_FLOCK 1
    #else
        #define D_ENV_C_HAS_FLOCK 0
    #endif
#endif  // D_ENV_C_HAS_FLOCK

// 3.2.2
// D_ENV_C_HAS_FOPEN_S
//   feature: detect if fopen_s is available (C11 Annex K / MSVC)
#ifndef D_ENV_C_HAS_FOPEN_S
    #if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) ||                                \
          (defined(__STDC_LIB_EXT1__)) )
        #define D_ENV_C_HAS_FOPEN_S 1
    #else
        #define D_ENV_C_HAS_FOPEN_S 0
    #endif
#endif  // D_ENV_C_HAS_FOPEN_S

// 3.2.3
// D_ENV_C_HAS_FSYNC
//   feature: detect if fsync is available (POSIX)
#ifndef D_ENV_C_HAS_FSYNC
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_FSYNC 1
    #else
        #define D_ENV_C_HAS_FSYNC 0
    #endif
#endif  // D_ENV_C_HAS_FSYNC

// 3.2.4
// D_ENV_C_HAS_LOCKFILE
//   feature: detect if LockFile API is available (Windows)
#ifndef D_ENV_C_HAS_LOCKFILE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_LOCKFILE 1
    #else
        #define D_ENV_C_HAS_LOCKFILE 0
    #endif
#endif  // D_ENV_C_HAS_LOCKFILE

// 3.2.5
// D_ENV_C_HAS_MMAP
//   feature: detect if mmap (memory-mapped files) is available
#ifndef D_ENV_C_HAS_MMAP
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_MMAP 1
    #else
        #define D_ENV_C_HAS_MMAP 0
    #endif
#endif  // D_ENV_C_HAS_MMAP

// 3.2.6
// D_ENV_C_HAS_SCANF_S
//   feature: detect if scanf_s is available (C11 Annex K / MSVC)
#ifndef D_ENV_C_HAS_SCANF_S
    #if ( (defined(__STDC_LIB_EXT1__)) ||                                      \
          (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) )
        #define D_ENV_C_HAS_SCANF_S 1
    #else
        #define D_ENV_C_HAS_SCANF_S 0
    #endif
#endif  // D_ENV_C_HAS_SCANF_S

// 3.3    Time and date
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_C_HAS_TIMESPEC_GET
//   feature: detect if timespec_get is available (C11)
#ifndef D_ENV_C_HAS_TIMESPEC_GET
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #if !defined(__STDC_NO_TIMESPEC_GET__)
            #define D_ENV_C_HAS_TIMESPEC_GET 1
        #else
            #define D_ENV_C_HAS_TIMESPEC_GET 0
        #endif
    #else
        #define D_ENV_C_HAS_TIMESPEC_GET 0
    #endif
#endif  // D_ENV_C_HAS_TIMESPEC_GET

// 3.3.2
// D_ENV_C_HAS_CLOCK_GETTIME
//   feature: detect if clock_gettime is available (POSIX)
#ifndef D_ENV_C_HAS_CLOCK_GETTIME
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_CLOCK_GETTIME 1
    #else
        #define D_ENV_C_HAS_CLOCK_GETTIME 0
    #endif
#endif  // D_ENV_C_HAS_CLOCK_GETTIME

// 3.3.3
// D_ENV_C_HAS_GETTIMEOFDAY
//   feature: detect if gettimeofday is available (POSIX)
#ifndef D_ENV_C_HAS_GETTIMEOFDAY
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_GETTIMEOFDAY 1
    #else
        #define D_ENV_C_HAS_GETTIMEOFDAY 0
    #endif
#endif  // D_ENV_C_HAS_GETTIMEOFDAY

// 3.3.4
// D_ENV_C_HAS_QUERYPERFORMANCECOUNTER
//   feature: detect if QueryPerformanceCounter is available (Windows)
#ifndef D_ENV_C_HAS_QUERYPERFORMANCECOUNTER
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_QUERYPERFORMANCECOUNTER 1
    #else
        #define D_ENV_C_HAS_QUERYPERFORMANCECOUNTER 0
    #endif
#endif  // D_ENV_C_HAS_QUERYPERFORMANCECOUNTER

// 3.4    Math
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_C_HAS_TGMATH_H
//   feature: detect if tgmath.h (type-generic math) is available (C99+)
#ifndef D_ENV_C_HAS_TGMATH_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_TGMATH_H 1
    #else
        #define D_ENV_C_HAS_TGMATH_H 0
    #endif
#endif  // D_ENV_C_HAS_TGMATH_H

// 3.4.2
// D_ENV_C_HAS_COMPLEX_H
//   feature: detect if complex.h is available (C99+)
#ifndef D_ENV_C_HAS_COMPLEX_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #if !defined(__STDC_NO_COMPLEX__)
            #define D_ENV_C_HAS_COMPLEX_H 1
        #else
            #define D_ENV_C_HAS_COMPLEX_H 0
        #endif
    #else
        #define D_ENV_C_HAS_COMPLEX_H 0
    #endif
#endif  // D_ENV_C_HAS_COMPLEX_H

// 3.4.3
// D_ENV_C_HAS_FENV_H
//   feature: detect if fenv.h (floating-point environment) is available
#ifndef D_ENV_C_HAS_FENV_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_FENV_H 1
    #else
        #define D_ENV_C_HAS_FENV_H 0
    #endif
#endif  // D_ENV_C_HAS_FENV_H


//==============================================================================
// 4.  NETWORKING, PROCESSES, AND MEMORY
//==============================================================================


// 4.1    Networking
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_C_HAS_WINSOCK
//   feature: detect if Winsock (Windows sockets) is available
#ifndef D_ENV_C_HAS_WINSOCK
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_WINSOCK 1
    #else
        #define D_ENV_C_HAS_WINSOCK 0
    #endif
#endif  // D_ENV_C_HAS_WINSOCK

// 4.1.2
// D_ENV_C_HAS_BSD_SOCKETS
//   feature: detect if BSD sockets are available
#ifndef D_ENV_C_HAS_BSD_SOCKETS
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_BSD_SOCKETS 1
    #else
        #define D_ENV_C_HAS_BSD_SOCKETS 0
    #endif
#endif  // D_ENV_C_HAS_BSD_SOCKETS

// 4.1.3
// D_ENV_C_HAS_GETADDRINFO
//   feature: detect if getaddrinfo is available (modern socket API)
#ifndef D_ENV_C_HAS_GETADDRINFO
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_GETADDRINFO 1
    #else
        #define D_ENV_C_HAS_GETADDRINFO 0
    #endif
#endif  // D_ENV_C_HAS_GETADDRINFO

// 4.2    Processes and signals
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_C_HAS_FORK
//   feature: detect if fork() is available (POSIX)
#ifndef D_ENV_C_HAS_FORK
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_FORK 1
    #else
        #define D_ENV_C_HAS_FORK 0
    #endif
#endif  // D_ENV_C_HAS_FORK

// 4.2.2
// D_ENV_C_HAS_EXECVE
//   feature: detect if execve() is available (POSIX)
#ifndef D_ENV_C_HAS_EXECVE
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_EXECVE 1
    #else
        #define D_ENV_C_HAS_EXECVE 0
    #endif
#endif  // D_ENV_C_HAS_EXECVE

// 4.2.3
// D_ENV_C_HAS_GETPID
//   feature: detect if getpid() is available
#ifndef D_ENV_C_HAS_GETPID
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_GETPID 1
    #else
        #define D_ENV_C_HAS_GETPID 0
    #endif
#endif  // D_ENV_C_HAS_GETPID

// 4.2.4
// D_ENV_C_HAS_SIGNAL_H
//   feature: detect if signal.h is available
#ifndef D_ENV_C_HAS_SIGNAL_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_SIGNAL_H 1
    #else
        #define D_ENV_C_HAS_SIGNAL_H 0
    #endif
#endif  // D_ENV_C_HAS_SIGNAL_H

// 4.3    Memory management
//------------------------------------------------------------------------------
// 4.3.1
// D_ENV_C_HAS_ALIGNED_ALLOC
//   feature: detect if aligned_alloc is available (C11)
#ifndef D_ENV_C_HAS_ALIGNED_ALLOC
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #if !defined(__APPLE__)
            // TODO: Apple supports aligned_alloc from macOS 10.15+;
            // refine with __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500
            // if targeting 10.15+ only.
            #define D_ENV_C_HAS_ALIGNED_ALLOC 1
        #else
            #define D_ENV_C_HAS_ALIGNED_ALLOC 0
        #endif
    #else
        #define D_ENV_C_HAS_ALIGNED_ALLOC 0
    #endif
#endif  // D_ENV_C_HAS_ALIGNED_ALLOC

// 4.3.2
// D_ENV_C_HAS_POSIX_MEMALIGN
//   feature: detect if posix_memalign is available (POSIX)
#ifndef D_ENV_C_HAS_POSIX_MEMALIGN
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_POSIX_MEMALIGN 1
    #else
        #define D_ENV_C_HAS_POSIX_MEMALIGN 0
    #endif
#endif  // D_ENV_C_HAS_POSIX_MEMALIGN

// 4.3.3
// D_ENV_C_HAS_ALIGNED_MALLOC
//   feature: detect if _aligned_malloc is available (Windows)
#ifndef D_ENV_C_HAS_ALIGNED_MALLOC
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_ALIGNED_MALLOC 1
    #else
        #define D_ENV_C_HAS_ALIGNED_MALLOC 0
    #endif
#endif  // D_ENV_C_HAS_ALIGNED_MALLOC

// 4.3.4
// D_ENV_C_HAS_ALLOCA
//   feature: detect if alloca (stack allocation) is available
#ifndef D_ENV_C_HAS_ALLOCA
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_ALLOCA 1
    #else
        #define D_ENV_C_HAS_ALLOCA 0
    #endif
#endif  // D_ENV_C_HAS_ALLOCA


//==============================================================================
// 5.  HARDWARE AND LANGUAGE FEATURES
//==============================================================================


// 5.1    SIMD intrinsics
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_C_HAS_SSE
//   feature: detect if SSE intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_SSE
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||                          \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if ( (defined(__SSE__)) ||                                            \
              ((defined(_M_IX86_FP)) && (_M_IX86_FP >= 1)) )
            #define D_ENV_C_HAS_SSE 1
        #else
            #define D_ENV_C_HAS_SSE 0
        #endif
    #else
        #define D_ENV_C_HAS_SSE 0
    #endif
#endif  // D_ENV_C_HAS_SSE

// 5.1.2
// D_ENV_C_HAS_SSE2
//   feature: detect if SSE2 intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_SSE2
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||                          \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if ( (defined(__SSE2__))                      ||                      \
              (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) ||                      \
              ((defined(_M_IX86_FP)) && (_M_IX86_FP >= 2)) )
            #define D_ENV_C_HAS_SSE2 1
        #else
            #define D_ENV_C_HAS_SSE2 0
        #endif
    #else
        #define D_ENV_C_HAS_SSE2 0
    #endif
#endif  // D_ENV_C_HAS_SSE2

// 5.1.3
// D_ENV_C_HAS_AVX
//   feature: detect if AVX intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_AVX
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||                          \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if defined(__AVX__)
            #define D_ENV_C_HAS_AVX 1
        #else
            #define D_ENV_C_HAS_AVX 0
        #endif
    #else
        #define D_ENV_C_HAS_AVX 0
    #endif
#endif  // D_ENV_C_HAS_AVX

// 5.1.4
// D_ENV_C_HAS_AVX2
//   feature: detect if AVX2 intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_AVX2
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||                          \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if defined(__AVX2__)
            #define D_ENV_C_HAS_AVX2 1
        #else
            #define D_ENV_C_HAS_AVX2 0
        #endif
    #else
        #define D_ENV_C_HAS_AVX2 0
    #endif
#endif  // D_ENV_C_HAS_AVX2

// 5.1.5
// D_ENV_C_HAS_NEON
//   feature: detect if ARM NEON intrinsics are available
#ifndef D_ENV_C_HAS_NEON
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM) ||                          \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM64) )
        #if ( (defined(__ARM_NEON)) ||                                         \
              (defined(__ARM_NEON__)) )
            #define D_ENV_C_HAS_NEON 1
        #else
            #define D_ENV_C_HAS_NEON 0
        #endif
    #else
        #define D_ENV_C_HAS_NEON 0
    #endif
#endif  // D_ENV_C_HAS_NEON

// 5.2    Variable-length arrays
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_C_HAS_VLA
//   feature: detect if variable-length arrays are supported (C99+)
#ifndef D_ENV_C_HAS_VLA
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #if !defined(__STDC_NO_VLA__)
            #define D_ENV_C_HAS_VLA 1
        #else
            #define D_ENV_C_HAS_VLA 0
        #endif
    #else
        #define D_ENV_C_HAS_VLA 0
    #endif
#endif  // D_ENV_C_HAS_VLA


//==============================================================================
// 6.  SECURITY
//==============================================================================


// 6.1    Security functions
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_C_HAS_SECURE_STRING_LIB
//   feature: detect if secure string library (Annex K) is available
#ifndef D_ENV_C_HAS_SECURE_STRING_LIB
    #if ( (defined(__STDC_LIB_EXT1__)) ||                                      \
          (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) )
        #define D_ENV_C_HAS_SECURE_STRING_LIB 1
    #else
        #define D_ENV_C_HAS_SECURE_STRING_LIB 0
    #endif
#endif  // D_ENV_C_HAS_SECURE_STRING_LIB

// 6.1.2
// D_ENV_C_HAS_GETENTROPY
//   feature: detect if getentropy (secure random) is available.
// note: intentionally narrower than D_ENV_IS_OS_POSIX_LIKE - getentropy
// is relatively recent (glibc 2.25 / OpenBSD 5.6) and not available on
// all generic Unix systems, so the Unix block (0x1) is omitted.
#ifndef D_ENV_C_HAS_GETENTROPY
    #if ( (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)          ||                     \
          (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)) ||                     \
          (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)) )
        #define D_ENV_C_HAS_GETENTROPY 1
    #else
        #define D_ENV_C_HAS_GETENTROPY 0
    #endif
#endif  // D_ENV_C_HAS_GETENTROPY

#endif  // __STDC_HOSTED__


#endif  // DJINTERP_ENV_C_ENV_C_LIB_H
