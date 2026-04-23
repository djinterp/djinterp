/******************************************************************************
* djinterp [core]                                                env_c_lib.h
* 
* djinterp C standard library feature detection:
* This header provides compile-time detection of C standard library
* features, POSIX headers, threading support, SIMD intrinsics, and other
* platform-specific capabilities. It relies on macros defined by env.h for
* OS, compiler, architecture, and language standard detection.
*
* SCOPE:
*   - C standard library headers and feature detection
*   - POSIX header and function availability
*   - Threading, networking, file I/O, and memory management
*   - SIMD intrinsic detection (SSE, AVX, NEON)
*
* USAGE:
*   Included automatically by env.h when C language is detected:
*     #ifdef D_ENV_LANG_DETECTED_C
*         #include ".\core\env\env_c_lib.h"
*     #endif
*
* NAMING CONVENTION:
*   D_ENV_C_HAS_[FEATURE_NAME] - 1 if available, 0 otherwise
*
* path:      \inc\core\env\env_c_lib.h                                           
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.02.08
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_C_
#define DJINTERP_ENVIRONMENT_C_ 1

// -----------------------------------------------------------------------------
// A.  Threading and Concurrency Support
// -----------------------------------------------------------------------------

#if defined(__STDC_HOSTED__)

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
#endif

// D_ENV_C_HAS_PTHREAD
//   feature: detect if POSIX threads (pthreads) are available
#ifndef D_ENV_C_HAS_PTHREAD
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID)          )
        #define D_ENV_C_HAS_PTHREAD 1
    #else
        #define D_ENV_C_HAS_PTHREAD 0
    #endif
#endif

// D_ENV_C_HAS_WINDOWS_THREADS
//   feature: detect Windows threading API
#ifndef D_ENV_C_HAS_WINDOWS_THREADS
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_WINDOWS_THREADS 1
    #else
        #define D_ENV_C_HAS_WINDOWS_THREADS 0
    #endif
#endif

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
    #elif defined(__cplusplus) && (__cplusplus >= 201103L)
        // C++11 and later have atomic support
        #define D_ENV_C_HAS_STDATOMIC 1
    #else
        #define D_ENV_C_HAS_STDATOMIC 0
    #endif
#endif


// =============================================================================
//      STANDARD LIBRARY FEATURE DETECTION
// =============================================================================

// -----------------------------------------------------------------------------
// B. Standard Headers Availability
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_STDBOOL_H
//   feature: detect if stdbool.h is available (C99+)
#ifndef D_ENV_C_HAS_STDBOOL_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_STDBOOL_H 1
    #else
        #define D_ENV_C_HAS_STDBOOL_H 0
    #endif
#endif

// D_ENV_C_HAS_STDINT_H
//   feature: detect if stdint.h is available (C99+)
#ifndef D_ENV_C_HAS_STDINT_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_STDINT_H 1
    #else
        #define D_ENV_C_HAS_STDINT_H 0
    #endif
#endif

// D_ENV_C_HAS_INTTYPES_H
//   feature: detect if inttypes.h is available (C99+)
#ifndef D_ENV_C_HAS_INTTYPES_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_INTTYPES_H 1
    #else
        #define D_ENV_C_HAS_INTTYPES_H 0
    #endif
#endif

// D_ENV_C_HAS_STDALIGN_H
//   feature: detect if stdalign.h is available (C11+)
#ifndef D_ENV_C_HAS_STDALIGN_H
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #if !defined(__STDC_NO_ALIGNOF__)
            #define D_ENV_C_HAS_STDALIGN_H 1
        #else
            #define D_ENV_C_HAS_STDALIGN_H 0
        #endif
    #else
        #define D_ENV_C_HAS_STDALIGN_H 0
    #endif
#endif

// D_ENV_C_HAS_UCHAR_H
//   feature: detect if uchar.h is available (C11+)
#ifndef D_ENV_C_HAS_UCHAR_H
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #define D_ENV_C_HAS_UCHAR_H 1
    #else
        #define D_ENV_C_HAS_UCHAR_H 0
    #endif
#endif


// -----------------------------------------------------------------------------
// C.  POSIX Headers and Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_UNISTD_H
//   feature: detect if unistd.h is available (POSIX systems)
#ifndef D_ENV_C_HAS_UNISTD_H
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID)          )
        #define D_ENV_C_HAS_UNISTD_H 1
    #else
        #define D_ENV_C_HAS_UNISTD_H 0
    #endif
#endif

// D_ENV_C_HAS_SYS_TYPES_H
//   feature: detect if sys/types.h is available
#ifndef D_ENV_C_HAS_SYS_TYPES_H
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_SYS_TYPES_H 1
    #else
        #define D_ENV_C_HAS_SYS_TYPES_H 0
    #endif
#endif

// D_ENV_C_HAS_SYS_STAT_H
//   feature: detect if sys/stat.h is available
#ifndef D_ENV_C_HAS_SYS_STAT_H
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_SYS_STAT_H 1
    #else
        #define D_ENV_C_HAS_SYS_STAT_H 0
    #endif
#endif

// D_ENV_C_HAS_DIRENT_H
//   feature: detect if dirent.h is available for directory operations
#ifndef D_ENV_C_HAS_DIRENT_H
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_DIRENT_H 1
    #else
        #define D_ENV_C_HAS_DIRENT_H 0
    #endif
#endif


// -----------------------------------------------------------------------------
// D.  String and Memory Functions
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_STRTOK_R
//   feature: detect if strtok_r (reentrant strtok) is available
#ifndef D_ENV_C_HAS_STRTOK_R
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_STRTOK_R 1
    #else
        #define D_ENV_C_HAS_STRTOK_R 0
    #endif
#endif

// D_ENV_C_HAS_STRTOK_S
//   feature: detect if strtok_s (Microsoft's reentrant strtok) is available
#ifndef D_ENV_C_HAS_STRTOK_S
    #if ( D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) ||  \
          defined(D_ENV_COMPILER_MSVC) )
        #define D_ENV_C_HAS_STRTOK_S 1
    #else
        #define D_ENV_C_HAS_STRTOK_S 0
    #endif
#endif

// D_ENV_C_HAS_SNPRINTF
//   feature: detect if snprintf is available (C99+)
#ifndef D_ENV_C_HAS_SNPRINTF
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_SNPRINTF 1
    #elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        // Windows has _snprintf
        #define D_ENV_C_HAS_SNPRINTF 1
    #else
        #define D_ENV_C_HAS_SNPRINTF 0
    #endif
#endif

// D_ENV_C_HAS_STRDUP
//   feature: detect if strdup is available (POSIX)
#ifndef D_ENV_C_HAS_STRDUP
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_STRDUP 1
    #else
        #define D_ENV_C_HAS_STRDUP 0
    #endif
#endif

// D_ENV_C_HAS_STRNDUP
//   feature: detect if strndup is available (POSIX)
#ifndef D_ENV_C_HAS_STRNDUP
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_STRNDUP 1
    #else
        #define D_ENV_C_HAS_STRNDUP 0
    #endif
#endif

// D_ENV_C_HAS_STRCASECMP
//   feature: detect if strcasecmp is available (POSIX)
#ifndef D_ENV_C_HAS_STRCASECMP
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_STRCASECMP 1
    #else
        #define D_ENV_C_HAS_STRCASECMP 0
    #endif
#endif

// D_ENV_C_HAS_STRICMP
//   feature: detect if _stricmp is available (Windows)
#ifndef D_ENV_C_HAS_STRICMP
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_STRICMP 1
    #else
        #define D_ENV_C_HAS_STRICMP 0
    #endif
#endif

// D_ENV_C_HAS_MEMCCPY
//   feature: detect if memccpy is available (POSIX)
#ifndef D_ENV_C_HAS_MEMCCPY
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_MEMCCPY 1
    #else
        #define D_ENV_C_HAS_MEMCCPY 0
    #endif
#endif


// -----------------------------------------------------------------------------
// E.  File System and I/O Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_FOPEN_S
//   feature: detect if fopen_s is available (C11 Annex K / MSVC)
#ifndef D_ENV_C_HAS_FOPEN_S
    #if ( D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) ||  \
          (defined(__STDC_LIB_EXT1__)) )
        #define D_ENV_C_HAS_FOPEN_S 1
    #else
        #define D_ENV_C_HAS_FOPEN_S 0
    #endif
#endif

// D_ENV_C_HAS_MMAP
//   feature: detect if mmap (memory-mapped files) is available
#ifndef D_ENV_C_HAS_MMAP
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_MMAP 1
    #else
        #define D_ENV_C_HAS_MMAP 0
    #endif
#endif

// D_ENV_C_HAS_FSYNC
//   feature: detect if fsync is available (POSIX)
#ifndef D_ENV_C_HAS_FSYNC
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_FSYNC 1
    #else
        #define D_ENV_C_HAS_FSYNC 0
    #endif
#endif

// D_ENV_C_HAS_FLOCK
//   feature: detect if flock (file locking) is available
#ifndef D_ENV_C_HAS_FLOCK
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_FLOCK 1
    #else
        #define D_ENV_C_HAS_FLOCK 0
    #endif
#endif

// D_ENV_C_HAS_LOCKFILE
//   feature: detect if LockFile API is available (Windows)
#ifndef D_ENV_C_HAS_LOCKFILE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_LOCKFILE 1
    #else
        #define D_ENV_C_HAS_LOCKFILE 0
    #endif
#endif


// -----------------------------------------------------------------------------
// F.  Time and Date Features
// -----------------------------------------------------------------------------

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
#endif

// D_ENV_C_HAS_CLOCK_GETTIME
//   feature: detect if clock_gettime is available (POSIX)
#ifndef D_ENV_C_HAS_CLOCK_GETTIME
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_CLOCK_GETTIME 1
    #else
        #define D_ENV_C_HAS_CLOCK_GETTIME 0
    #endif
#endif

// D_ENV_C_HAS_GETTIMEOFDAY
//   feature: detect if gettimeofday is available (POSIX)
#ifndef D_ENV_C_HAS_GETTIMEOFDAY
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_GETTIMEOFDAY 1
    #else
        #define D_ENV_C_HAS_GETTIMEOFDAY 0
    #endif
#endif

// D_ENV_C_HAS_QUERYPERFORMANCECOUNTER
//   feature: detect if QueryPerformanceCounter is available (Windows)
#ifndef D_ENV_C_HAS_QUERYPERFORMANCECOUNTER
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_QUERYPERFORMANCECOUNTER 1
    #else
        #define D_ENV_C_HAS_QUERYPERFORMANCECOUNTER 0
    #endif
#endif


// -----------------------------------------------------------------------------
// G.  Math Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_TGMATH_H
//   feature: detect if tgmath.h (type-generic math) is available (C99+)
#ifndef D_ENV_C_HAS_TGMATH_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_TGMATH_H 1
    #else
        #define D_ENV_C_HAS_TGMATH_H 0
    #endif
#endif

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
#endif

// D_ENV_C_HAS_FENV_H
//   feature: detect if fenv.h (floating-point environment) is available
#ifndef D_ENV_C_HAS_FENV_H
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        #define D_ENV_C_HAS_FENV_H 1
    #else
        #define D_ENV_C_HAS_FENV_H 0
    #endif
#endif


// -----------------------------------------------------------------------------
// H.  Network Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_WINSOCK
//   feature: detect if Winsock (Windows sockets) is available
#ifndef D_ENV_C_HAS_WINSOCK
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_WINSOCK 1
    #else
        #define D_ENV_C_HAS_WINSOCK 0
    #endif
#endif

// D_ENV_C_HAS_BSD_SOCKETS
//   feature: detect if BSD sockets are available
#ifndef D_ENV_C_HAS_BSD_SOCKETS
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_BSD_SOCKETS 1
    #else
        #define D_ENV_C_HAS_BSD_SOCKETS 0
    #endif
#endif

// D_ENV_C_HAS_GETADDRINFO
//   feature: detect if getaddrinfo is available (modern socket API)
#ifndef D_ENV_C_HAS_GETADDRINFO
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_GETADDRINFO 1
    #else
        #define D_ENV_C_HAS_GETADDRINFO 0
    #endif
#endif


// -----------------------------------------------------------------------------
// I.  Process and System Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_FORK
//   feature: detect if fork() is available (POSIX)
#ifndef D_ENV_C_HAS_FORK
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_FORK 1
    #else
        #define D_ENV_C_HAS_FORK 0
    #endif
#endif

// D_ENV_C_HAS_EXECVE
//   feature: detect if execve() is available (POSIX)
#ifndef D_ENV_C_HAS_EXECVE
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_EXECVE 1
    #else
        #define D_ENV_C_HAS_EXECVE 0
    #endif
#endif

// D_ENV_C_HAS_GETPID
//   feature: detect if getpid() is available
#ifndef D_ENV_C_HAS_GETPID
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_GETPID 1
    #else
        #define D_ENV_C_HAS_GETPID 0
    #endif
#endif

// D_ENV_C_HAS_SIGNAL_H
//   feature: detect if signal.h is available
#ifndef D_ENV_C_HAS_SIGNAL_H
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_SIGNAL_H 1
    #else
        #define D_ENV_C_HAS_SIGNAL_H 0
    #endif
#endif


// -----------------------------------------------------------------------------
// J.  Memory Management Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_ALIGNED_ALLOC
//   feature: detect if aligned_alloc is available (C11)
#ifndef D_ENV_C_HAS_ALIGNED_ALLOC
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #if !defined(__APPLE__)
            // Apple doesn't support aligned_alloc until macOS 10.15
            #define D_ENV_C_HAS_ALIGNED_ALLOC 1
        #else
            #define D_ENV_C_HAS_ALIGNED_ALLOC 0
        #endif
    #else
        #define D_ENV_C_HAS_ALIGNED_ALLOC 0
    #endif
#endif

// D_ENV_C_HAS_POSIX_MEMALIGN
//   feature: detect if posix_memalign is available (POSIX)
#ifndef D_ENV_C_HAS_POSIX_MEMALIGN
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     )
        #define D_ENV_C_HAS_POSIX_MEMALIGN 1
    #else
        #define D_ENV_C_HAS_POSIX_MEMALIGN 0
    #endif
#endif

// D_ENV_C_HAS_ALIGNED_MALLOC
//   feature: detect if _aligned_malloc is available (Windows)
#ifndef D_ENV_C_HAS_ALIGNED_MALLOC
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_ALIGNED_MALLOC 1
    #else
        #define D_ENV_C_HAS_ALIGNED_MALLOC 0
    #endif
#endif

// D_ENV_C_HAS_ALLOCA
//   feature: detect if alloca (stack allocation) is available
#ifndef D_ENV_C_HAS_ALLOCA
    #if ( D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)              ||  \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)     ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_ALLOCA 1
    #else
        #define D_ENV_C_HAS_ALLOCA 0
    #endif
#endif


// -----------------------------------------------------------------------------
// K.  SIMD and Hardware Intrinsics
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_SSE
//   feature: detect if SSE intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_SSE
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||  \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if ( defined(__SSE__)  ||  \
              defined(_M_IX86_FP) && (_M_IX86_FP >= 1) )
            #define D_ENV_C_HAS_SSE 1
        #else
            #define D_ENV_C_HAS_SSE 0
        #endif
    #else
        #define D_ENV_C_HAS_SSE 0
    #endif
#endif

// D_ENV_C_HAS_SSE2
//   feature: detect if SSE2 intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_SSE2
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||  \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if ( defined(__SSE2__)  ||  \
              (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) ||  \
              defined(_M_IX86_FP) && (_M_IX86_FP >= 2) )
            #define D_ENV_C_HAS_SSE2 1
        #else
            #define D_ENV_C_HAS_SSE2 0
        #endif
    #else
        #define D_ENV_C_HAS_SSE2 0
    #endif
#endif

// D_ENV_C_HAS_AVX
//   feature: detect if AVX intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_AVX
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||  \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if defined(__AVX__)
            #define D_ENV_C_HAS_AVX 1
        #else
            #define D_ENV_C_HAS_AVX 0
        #endif
    #else
        #define D_ENV_C_HAS_AVX 0
    #endif
#endif

// D_ENV_C_HAS_AVX2
//   feature: detect if AVX2 intrinsics are available (x86/x64)
#ifndef D_ENV_C_HAS_AVX2
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86) ||  \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64) )
        #if defined(__AVX2__)
            #define D_ENV_C_HAS_AVX2 1
        #else
            #define D_ENV_C_HAS_AVX2 0
        #endif
    #else
        #define D_ENV_C_HAS_AVX2 0
    #endif
#endif

// D_ENV_C_HAS_NEON
//   feature: detect if ARM NEON intrinsics are available
#ifndef D_ENV_C_HAS_NEON
    #if ( (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM)   ||  \
          (D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM64) )
        #if ( defined(__ARM_NEON) || defined(__ARM_NEON__) )
            #define D_ENV_C_HAS_NEON 1
        #else
            #define D_ENV_C_HAS_NEON 0
        #endif
    #else
        #define D_ENV_C_HAS_NEON 0
    #endif
#endif


// -----------------------------------------------------------------------------
// L.  Variable-Length Arrays (VLA)
// -----------------------------------------------------------------------------

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
#endif


// -----------------------------------------------------------------------------
// M.  Security Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_SECURE_STRING_LIB
//   feature: detect if secure string library (Annex K) is available
#ifndef D_ENV_C_HAS_SECURE_STRING_LIB
    #if ( defined(__STDC_LIB_EXT1__)  ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_SECURE_STRING_LIB 1
    #else
        #define D_ENV_C_HAS_SECURE_STRING_LIB 0
    #endif
#endif

// D_ENV_C_HAS_GETENTROPY
//   feature: detect if getentropy (secure random) is available
#ifndef D_ENV_C_HAS_GETENTROPY
    #if ( (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)            ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)     ||  \
          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4) )
        #define D_ENV_C_HAS_GETENTROPY 1
    #else
        #define D_ENV_C_HAS_GETENTROPY 0
    #endif
#endif  // D_ENV_C_HAS_GETENTROPY

#endif  // __STDC_HOSTED__


#endif  // DJINTERP_ENVIRONMENT_C_
