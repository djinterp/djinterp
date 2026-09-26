/*******************************************************************************
* djinterp [env]                                                     env_posix.h
*
* djinterp POSIX / XSI standards detection.
*   Compile-time detection of the POSIX and X/Open System Interface levels and
* of individual POSIX features (threads, real-time, sockets, shared memory,
* semaphores, message queues, memory mapping), exposing the D_ENV_POSIX_*
* interface and its utility macros.
*   Requires cfg_env.h (for the D_CFG_ENV_* switches). This header is an
* internal component of env.h and is #included by it; do not #include it
* directly.
*
* path:      /inc/djinterp/env/env_posix.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  POSIX CONSTANTS
    ---------------
    1.  POSIX versions
         1.  POSIX version identifiers
              1.  D_ENV_POSIX_VERSION_1988
              2.  D_ENV_POSIX_VERSION_1990
              3.  D_ENV_POSIX_VERSION_1993
              4.  D_ENV_POSIX_VERSION_1996
              5.  D_ENV_POSIX_VERSION_2001
              6.  D_ENV_POSIX_VERSION_2008
              7.  D_ENV_POSIX_VERSION_2017
              8.  D_ENV_POSIX_VERSION_2024
    2.  _POSIX_C_SOURCE levels
         1.  _POSIX_C_SOURCE identifiers
              1.  D_ENV_POSIX_C_SOURCE_1
              2.  D_ENV_POSIX_C_SOURCE_2
              3.  D_ENV_POSIX_C_SOURCE_199309L
              4.  D_ENV_POSIX_C_SOURCE_199506L
              5.  D_ENV_POSIX_C_SOURCE_200112L
              6.  D_ENV_POSIX_C_SOURCE_200809L
    3.  XSI levels
         1.  XSI version identifiers
              1.  D_ENV_POSIX_XSI_VERSION_3
              2.  D_ENV_POSIX_XSI_VERSION_4
              3.  D_ENV_POSIX_XSI_VERSION_500
              4.  D_ENV_POSIX_XSI_VERSION_600
              5.  D_ENV_POSIX_XSI_VERSION_700
2.  POSIX DETECTION
    ---------------
    1.  Automatic detection
         1.  <unistd.h>
         2.  D_ENV_POSIX_VERSION
         3.  D_ENV_POSIX_XSI_VERSION
         4.  D_ENV_POSIX_FEATURE_THREADS
         5.  D_ENV_POSIX_FEATURE_REALTIME
         6.  D_ENV_POSIX_FEATURE_SOCKETS
         7.  D_ENV_POSIX_FEATURE_SHARED_MEMORY
         8.  D_ENV_POSIX_FEATURE_SEMAPHORES
         9.  D_ENV_POSIX_FEATURE_MESSAGE_QUEUES
         10. D_ENV_POSIX_FEATURE_MEMORY_MAPPING
    2.  Predefined detection
         1.  D_ENV_DETECTED_POSIX_* version overrides
         2.  D_ENV_DETECTED_POSIX_XSI override
         3.  D_ENV_DETECTED_POSIX_* feature overrides
3.  POSIX UTILITIES
    ---------------
    1.  Availability and version checks
         1.  D_ENV_POSIX_IS_AVAILABLE
         2.  D_ENV_POSIX_IS_MODERN
         3.  D_ENV_POSIX_HAS_FEATURE
         4.  D_ENV_POSIX_VERSION_AT_LEAST
         5.  D_ENV_XSI_IS_AVAILABLE
         6.  D_ENV_XSI_VERSION_AT_LEAST
*/

#ifndef DJINTERP_ENV_ENV_POSIX_H
#define DJINTERP_ENV_ENV_POSIX_H 1


//==============================================================================
// 1.  POSIX CONSTANTS
//==============================================================================
// The values the POSIX and X/Open feature macros take, so a detected level
// compares directly against these.


// 1.1    POSIX versions
//------------------------------------------------------------------------------
// 1.1.1
// POSIX version identifiers

// 1.1.1.1
// D_ENV_POSIX_VERSION_1988
//   constant: _POSIX_VERSION value for POSIX.1-1988 (IEEE 1003.1).
#define D_ENV_POSIX_VERSION_1988        198808L

// 1.1.1.2
// D_ENV_POSIX_VERSION_1990
//   constant: _POSIX_VERSION value for POSIX.1-1990 (ISO/IEC 9945-1).
#define D_ENV_POSIX_VERSION_1990        199009L

// 1.1.1.3
// D_ENV_POSIX_VERSION_1993
//   constant: _POSIX_VERSION value for POSIX.1b-1993 (Real-time extensions).
#define D_ENV_POSIX_VERSION_1993        199309L

// 1.1.1.4
// D_ENV_POSIX_VERSION_1996
//   constant: _POSIX_VERSION value for POSIX.1c-1995 (Threads).
#define D_ENV_POSIX_VERSION_1996        199506L

// 1.1.1.5
// D_ENV_POSIX_VERSION_2001
//   constant: _POSIX_VERSION value for POSIX.1-2001 (SUSv3).
#define D_ENV_POSIX_VERSION_2001        200112L

// 1.1.1.6
// D_ENV_POSIX_VERSION_2008
//   constant: _POSIX_VERSION value for POSIX.1-2008 (SUSv4).
#define D_ENV_POSIX_VERSION_2008        200809L

// 1.1.1.7
// D_ENV_POSIX_VERSION_2017
//   constant: _POSIX_VERSION value for POSIX.1-2017 (SUSv5).
#define D_ENV_POSIX_VERSION_2017        201700L

// 1.1.1.8
// D_ENV_POSIX_VERSION_2024
//   constant: _POSIX_VERSION value for POSIX.1-2024 (SUSv5.1).
#define D_ENV_POSIX_VERSION_2024        202405L

// 1.2    _POSIX_C_SOURCE levels
//------------------------------------------------------------------------------
// 1.2.1
// _POSIX_C_SOURCE identifiers

// 1.2.1.1
// D_ENV_POSIX_C_SOURCE_1
//   constant: _POSIX_C_SOURCE value requesting basic POSIX.1 functionality.
#define D_ENV_POSIX_C_SOURCE_1          1L

// 1.2.1.2
// D_ENV_POSIX_C_SOURCE_2
//   constant: _POSIX_C_SOURCE value requesting POSIX.2 functionality.
#define D_ENV_POSIX_C_SOURCE_2          2L

// 1.2.1.3
// D_ENV_POSIX_C_SOURCE_199309L
//   constant: _POSIX_C_SOURCE value requesting POSIX.1b (real-time).
#define D_ENV_POSIX_C_SOURCE_199309L    199309L

// 1.2.1.4
// D_ENV_POSIX_C_SOURCE_199506L
//   constant: _POSIX_C_SOURCE value requesting POSIX.1c (threads).
#define D_ENV_POSIX_C_SOURCE_199506L    199506L

// 1.2.1.5
// D_ENV_POSIX_C_SOURCE_200112L
//   constant: _POSIX_C_SOURCE value requesting POSIX.1-2001.
#define D_ENV_POSIX_C_SOURCE_200112L    200112L

// 1.2.1.6
// D_ENV_POSIX_C_SOURCE_200809L
//   constant: _POSIX_C_SOURCE value requesting POSIX.1-2008.
#define D_ENV_POSIX_C_SOURCE_200809L    200809L

// 1.3    XSI levels
//------------------------------------------------------------------------------
// 1.3.1
// XSI version identifiers

// 1.3.1.1
// D_ENV_POSIX_XSI_VERSION_3
//   constant: _XOPEN_VERSION value for XPG3.
#define D_ENV_POSIX_XSI_VERSION_3       3L

// 1.3.1.2
// D_ENV_POSIX_XSI_VERSION_4
//   constant: _XOPEN_VERSION value for XPG4.
#define D_ENV_POSIX_XSI_VERSION_4       4L

// 1.3.1.3
// D_ENV_POSIX_XSI_VERSION_500
//   constant: _XOPEN_VERSION value for SUSv2/Unix 98.
#define D_ENV_POSIX_XSI_VERSION_500     500L

// 1.3.1.4
// D_ENV_POSIX_XSI_VERSION_600
//   constant: _XOPEN_VERSION value for SUSv3/Unix 03.
#define D_ENV_POSIX_XSI_VERSION_600     600L

// 1.3.1.5
// D_ENV_POSIX_XSI_VERSION_700
//   constant: _XOPEN_VERSION value for SUSv4/Unix 08.
#define D_ENV_POSIX_XSI_VERSION_700     700L


//==============================================================================
// 2.  POSIX DETECTION
//==============================================================================
// Reads the POSIX and X/Open macros from <unistd.h>, which item 2.1.1
// includes wherever it exists, so the result does not depend on what the
// translation unit included before env.h.


#if D_CFG_ENV_POSIX_ENABLED

// 2.1    Automatic detection
//------------------------------------------------------------------------------
    // 2.1.1
    // <unistd.h>
    //   the macros read below -- _POSIX_VERSION, _XOPEN_VERSION, and the
    // _POSIX_* feature options -- are defined by <unistd.h>, so it is included
    // here wherever it exists. Without it, the result would depend on whether
    // the translation unit happened to include <unistd.h> first. Compilers
    // without __has_include fall back to the Unix-family predefines.
    #if defined(__has_include)
        #if __has_include(<unistd.h>)
            // posix
            #include <unistd.h>  // _POSIX_VERSION, _XOPEN_VERSION, _POSIX_*
        #endif
    #elif ( defined(__unix__)    ||                                            \
            defined(__unix)      ||                                            \
            defined(unix)        ||                                            \
            defined(__linux__)   ||                                            \
            defined(__APPLE__)   ||                                            \
            defined(__FreeBSD__) ||                                            \
            defined(__OpenBSD__) ||                                            \
            defined(__NetBSD__)  ||                                            \
            defined(__sun) )
        // posix
        #include <unistd.h>  // _POSIX_VERSION, _XOPEN_VERSION, _POSIX_*
    #endif

    // 2.1.2
    // D_ENV_POSIX_VERSION
    //   constant: the detected POSIX version, with D_ENV_POSIX_NAME and one
    // D_ENV_POSIX_<year> flag. Taken from _POSIX_VERSION, else from
    // _POSIX_C_SOURCE, else D_ENV_POSIX_LIKELY (version 0) on a Unix-like
    // target, else D_ENV_POSIX_NONE.
    #ifndef D_ENV_POSIX_VERSION
        // primary POSIX version detection from _POSIX_VERSION
        #ifdef _POSIX_VERSION
            #if (_POSIX_VERSION >= D_ENV_POSIX_VERSION_2024)
                #define D_ENV_POSIX_2024        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1-2024"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_2017)
                #define D_ENV_POSIX_2017        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1-2017"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_2008)
                #define D_ENV_POSIX_2008        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1-2008"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001)
                #define D_ENV_POSIX_2001        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1-2001"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_1996)
                #define D_ENV_POSIX_1996        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1c-1995"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_1993)
                #define D_ENV_POSIX_1993        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1b-1993"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_1990)
                #define D_ENV_POSIX_1990        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1-1990"
            #elif (_POSIX_VERSION >= D_ENV_POSIX_VERSION_1988)
                #define D_ENV_POSIX_1988        1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX.1-1988"
            #else
                #define D_ENV_POSIX_UNKNOWN     1
                #define D_ENV_POSIX_VERSION     _POSIX_VERSION
                #define D_ENV_POSIX_NAME        "POSIX (Unknown)"
            #endif

        // fallback detection using _POSIX_C_SOURCE
        #elif defined(_POSIX_C_SOURCE)
            #if (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_200809L)
                #define D_ENV_POSIX_2008        1
                #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_2008
                #define D_ENV_POSIX_NAME        "POSIX.1-2008"
            #elif (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_200112L)
                #define D_ENV_POSIX_2001        1
                #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_2001
                #define D_ENV_POSIX_NAME        "POSIX.1-2001"
            #elif (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_199506L)
                #define D_ENV_POSIX_1996        1
                #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1996
                #define D_ENV_POSIX_NAME        "POSIX.1c-1995"
            #elif (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_199309L)
                #define D_ENV_POSIX_1993        1
                #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1993
                #define D_ENV_POSIX_NAME        "POSIX.1b-1993"
            #elif (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_2)
                #define D_ENV_POSIX_1990        1
                #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1990
                #define D_ENV_POSIX_NAME        "POSIX.1-1990"
            #elif (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_1)
                #define D_ENV_POSIX_1988        1
                #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1988
                #define D_ENV_POSIX_NAME        "POSIX.1-1988"
            #else
                #define D_ENV_POSIX_UNKNOWN     1
                #define D_ENV_POSIX_VERSION     0L
                #define D_ENV_POSIX_NAME        "POSIX (Minimal)"
            #endif

        // check for basic POSIX symbols if no version macros
        #elif ( defined(__unix__)   ||  \
                defined(__unix)     ||  \
                defined(unix)       ||  \
                defined(__linux__)  ||  \
                defined(__APPLE__)  ||  \
                defined(__FreeBSD__)||  \
                defined(__OpenBSD__)||  \
                defined(__NetBSD__) ||  \
                defined(__sun) )
            #define D_ENV_POSIX_LIKELY      1
            #define D_ENV_POSIX_VERSION     0L
            #define D_ENV_POSIX_NAME        "POSIX (Likely)"

        // no POSIX detected
        #else
            #define D_ENV_POSIX_NONE        1
            #define D_ENV_POSIX_VERSION     0L
            #define D_ENV_POSIX_NAME        "None"
        #endif  // _POSIX_VERSION
    #endif  // D_ENV_POSIX_VERSION

    // 2.1.3
    // D_ENV_POSIX_XSI_VERSION
    //   constant: the detected X/Open System Interface level, from
    // _XOPEN_VERSION, with D_ENV_POSIX_XSI_NAME and one D_ENV_POSIX_XSI_<level>
    // flag; D_ENV_POSIX_XSI_NONE when _XOPEN_VERSION is not visible.
    #ifndef D_ENV_POSIX_XSI_VERSION
        #ifdef _XOPEN_VERSION
            #if (_XOPEN_VERSION >= D_ENV_POSIX_XSI_VERSION_700)
                #define D_ENV_POSIX_XSI_700     1
                #define D_ENV_POSIX_XSI_VERSION _XOPEN_VERSION
                #define D_ENV_POSIX_XSI_NAME    "XSI 700"
            #elif (_XOPEN_VERSION >= D_ENV_POSIX_XSI_VERSION_600)
                #define D_ENV_POSIX_XSI_600     1
                #define D_ENV_POSIX_XSI_VERSION _XOPEN_VERSION
                #define D_ENV_POSIX_XSI_NAME    "XSI 600"
            #elif (_XOPEN_VERSION >= D_ENV_POSIX_XSI_VERSION_500)
                #define D_ENV_POSIX_XSI_500     1
                #define D_ENV_POSIX_XSI_VERSION _XOPEN_VERSION
                #define D_ENV_POSIX_XSI_NAME    "XSI 500"
            #elif (_XOPEN_VERSION >= D_ENV_POSIX_XSI_VERSION_4)
                #define D_ENV_POSIX_XSI_4       1
                #define D_ENV_POSIX_XSI_VERSION _XOPEN_VERSION
                #define D_ENV_POSIX_XSI_NAME    "XPG4"
            #elif (_XOPEN_VERSION >= D_ENV_POSIX_XSI_VERSION_3)
                #define D_ENV_POSIX_XSI_3       1
                #define D_ENV_POSIX_XSI_VERSION _XOPEN_VERSION
                #define D_ENV_POSIX_XSI_NAME    "XPG3"
            #else
                #define D_ENV_POSIX_XSI_UNKNOWN 1
                #define D_ENV_POSIX_XSI_VERSION _XOPEN_VERSION
                #define D_ENV_POSIX_XSI_NAME    "XSI (Unknown)"
            #endif
        #else
            #define D_ENV_POSIX_XSI_NONE    1
            #define D_ENV_POSIX_XSI_VERSION 0L
            #define D_ENV_POSIX_XSI_NAME    "None"
        #endif  // _XOPEN_VERSION
    #endif  // D_ENV_POSIX_XSI_VERSION

    // 2.1.4
    // D_ENV_POSIX_FEATURE_THREADS
    //   feature: POSIX threads (pthreads) support detection.
    #ifdef _POSIX_THREADS
        #define D_ENV_POSIX_FEATURE_THREADS 1
    #else
        #define D_ENV_POSIX_FEATURE_THREADS 0
    #endif  // _POSIX_THREADS

    // 2.1.5
    // D_ENV_POSIX_FEATURE_REALTIME
    //   feature: POSIX real-time extensions support detection.
    #if ( defined(_POSIX_REALTIME_SIGNALS) ||  \
          defined(_POSIX_TIMERS)            ||  \
          defined(_POSIX_ASYNCHRONOUS_IO)   ||  \
          defined(_POSIX_PRIORITY_SCHEDULING) )
        #define D_ENV_POSIX_FEATURE_REALTIME 1
    #else
        #define D_ENV_POSIX_FEATURE_REALTIME 0
    #endif

    // 2.1.6
    // D_ENV_POSIX_FEATURE_SOCKETS
    //   feature: POSIX sockets/networking support detection.
    #if ( defined(_POSIX_NETWORKING) ||  \
          defined(__unix__)          ||  \
          defined(__linux__)         ||  \
          defined(__APPLE__)         ||  \
          defined(__FreeBSD__)       ||  \
          defined(__OpenBSD__)       ||  \
          defined(__NetBSD__) )
        #define D_ENV_POSIX_FEATURE_SOCKETS 1
    #else
        #define D_ENV_POSIX_FEATURE_SOCKETS 0
    #endif

    // 2.1.7
    // D_ENV_POSIX_FEATURE_SHARED_MEMORY
    //   feature: POSIX shared memory support detection.
    #ifdef _POSIX_SHARED_MEMORY_OBJECTS
        #define D_ENV_POSIX_FEATURE_SHARED_MEMORY 1
    #else
        #define D_ENV_POSIX_FEATURE_SHARED_MEMORY 0
    #endif  // _POSIX_SHARED_MEMORY_OBJECTS

    // 2.1.8
    // D_ENV_POSIX_FEATURE_SEMAPHORES
    //   feature: POSIX semaphores support detection.
    #ifdef _POSIX_SEMAPHORES
        #define D_ENV_POSIX_FEATURE_SEMAPHORES 1
    #else
        #define D_ENV_POSIX_FEATURE_SEMAPHORES 0
    #endif  // _POSIX_SEMAPHORES

    // 2.1.9
    // D_ENV_POSIX_FEATURE_MESSAGE_QUEUES
    //   feature: POSIX message queues support detection.
    #ifdef _POSIX_MESSAGE_PASSING
        #define D_ENV_POSIX_FEATURE_MESSAGE_QUEUES 1
    #else
        #define D_ENV_POSIX_FEATURE_MESSAGE_QUEUES 0
    #endif  // _POSIX_MESSAGE_PASSING

    // 2.1.10
    // D_ENV_POSIX_FEATURE_MEMORY_MAPPING
    //   feature: POSIX memory mapping (mmap) support detection.
    #ifdef _POSIX_MAPPED_FILES
        #define D_ENV_POSIX_FEATURE_MEMORY_MAPPING 1
    #else
        #define D_ENV_POSIX_FEATURE_MEMORY_MAPPING 0
    #endif  // _POSIX_MAPPED_FILES

#else

// 2.2    Predefined detection
//------------------------------------------------------------------------------
    // 2.2.1
    // D_ENV_DETECTED_POSIX_* version overrides
    //   the newest D_ENV_DETECTED_POSIX_<year> defined selects the version.
    #ifdef D_ENV_DETECTED_POSIX_2024
        #define D_ENV_POSIX_2024        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_2024
        #define D_ENV_POSIX_NAME        "POSIX.1-2024"
    #elif defined(D_ENV_DETECTED_POSIX_2017)
        #define D_ENV_POSIX_2017        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_2017
        #define D_ENV_POSIX_NAME        "POSIX.1-2017"
    #elif defined(D_ENV_DETECTED_POSIX_2008)
        #define D_ENV_POSIX_2008        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_2008
        #define D_ENV_POSIX_NAME        "POSIX.1-2008"
    #elif defined(D_ENV_DETECTED_POSIX_2001)
        #define D_ENV_POSIX_2001        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_2001
        #define D_ENV_POSIX_NAME        "POSIX.1-2001"
    #elif defined(D_ENV_DETECTED_POSIX_1996)
        #define D_ENV_POSIX_1996        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1996
        #define D_ENV_POSIX_NAME        "POSIX.1c-1995"
    #elif defined(D_ENV_DETECTED_POSIX_1993)
        #define D_ENV_POSIX_1993        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1993
        #define D_ENV_POSIX_NAME        "POSIX.1b-1993"
    #elif defined(D_ENV_DETECTED_POSIX_1990)
        #define D_ENV_POSIX_1990        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1990
        #define D_ENV_POSIX_NAME        "POSIX.1-1990"
    #elif defined(D_ENV_DETECTED_POSIX_1988)
        #define D_ENV_POSIX_1988        1
        #define D_ENV_POSIX_VERSION     D_ENV_POSIX_VERSION_1988
        #define D_ENV_POSIX_NAME        "POSIX.1-1988"
    #elif defined(D_ENV_DETECTED_POSIX_NONE)
        #define D_ENV_POSIX_NONE        1
        #define D_ENV_POSIX_VERSION     0L
        #define D_ENV_POSIX_NAME        "None"
    #endif  // D_ENV_DETECTED_POSIX_2024

    // 2.2.2
    // D_ENV_DETECTED_POSIX_XSI override
    //   defined: XSI 700; otherwise no XSI.
    #ifdef D_ENV_DETECTED_POSIX_XSI
        #define D_ENV_POSIX_XSI_700     1
        #define D_ENV_POSIX_XSI_VERSION D_ENV_POSIX_XSI_VERSION_700
        #define D_ENV_POSIX_XSI_NAME    "XSI 700"
    #else
        #define D_ENV_POSIX_XSI_NONE    1
        #define D_ENV_POSIX_XSI_VERSION 0L
        #define D_ENV_POSIX_XSI_NAME    "None"
    #endif  // D_ENV_DETECTED_POSIX_XSI

    // 2.2.3
    // D_ENV_DETECTED_POSIX_* feature overrides
    //   THREADS, REALTIME, and SOCKETS follow their D_ENV_DETECTED_POSIX_*
    // switch; the remaining features default to 0.
    #ifdef D_ENV_DETECTED_POSIX_THREADS
        #define D_ENV_POSIX_FEATURE_THREADS 1
    #else
        #define D_ENV_POSIX_FEATURE_THREADS 0
    #endif  // D_ENV_DETECTED_POSIX_THREADS

    #ifdef D_ENV_DETECTED_POSIX_REALTIME
        #define D_ENV_POSIX_FEATURE_REALTIME 1
    #else
        #define D_ENV_POSIX_FEATURE_REALTIME 0
    #endif  // D_ENV_DETECTED_POSIX_REALTIME

    #ifdef D_ENV_DETECTED_POSIX_SOCKETS
        #define D_ENV_POSIX_FEATURE_SOCKETS 1
    #else
        #define D_ENV_POSIX_FEATURE_SOCKETS 0
    #endif  // D_ENV_DETECTED_POSIX_SOCKETS

    #ifndef D_ENV_POSIX_FEATURE_SHARED_MEMORY
        #define D_ENV_POSIX_FEATURE_SHARED_MEMORY 0
    #endif  // D_ENV_POSIX_FEATURE_SHARED_MEMORY

    #ifndef D_ENV_POSIX_FEATURE_SEMAPHORES
        #define D_ENV_POSIX_FEATURE_SEMAPHORES 0
    #endif  // D_ENV_POSIX_FEATURE_SEMAPHORES

    #ifndef D_ENV_POSIX_FEATURE_MESSAGE_QUEUES
        #define D_ENV_POSIX_FEATURE_MESSAGE_QUEUES 0
    #endif  // D_ENV_POSIX_FEATURE_MESSAGE_QUEUES

    #ifndef D_ENV_POSIX_FEATURE_MEMORY_MAPPING
        #define D_ENV_POSIX_FEATURE_MEMORY_MAPPING 0
    #endif  // D_ENV_POSIX_FEATURE_MEMORY_MAPPING

#endif  // D_CFG_ENV_POSIX_ENABLED


//==============================================================================
// 3.  POSIX UTILITIES
//==============================================================================


// 3.1    Availability and version checks
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_POSIX_IS_AVAILABLE
//   macro: evaluates to 1 if any POSIX standard is detected, 0 otherwise.
#ifdef D_ENV_POSIX_NONE
    #define D_ENV_POSIX_IS_AVAILABLE 0
#else
    #define D_ENV_POSIX_IS_AVAILABLE (D_ENV_POSIX_VERSION > 0)
#endif  // D_ENV_POSIX_NONE

// 3.1.2
// D_ENV_POSIX_IS_MODERN
//   macro: evaluates to 1 if POSIX.1-2001 or later is detected.
#define D_ENV_POSIX_IS_MODERN     \
    (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001)

// 3.1.3
// D_ENV_POSIX_HAS_FEATURE
//   macro: checks if a specific POSIX feature is available.
#define D_ENV_POSIX_HAS_FEATURE(feature_macro) (feature_macro)

// 3.1.4
// D_ENV_POSIX_VERSION_AT_LEAST
//   macro: checks if POSIX version is at least the specified version.
#define D_ENV_POSIX_VERSION_AT_LEAST(version) \
    (D_ENV_POSIX_VERSION >= (version))

// 3.1.5
// D_ENV_XSI_IS_AVAILABLE
//   macro: evaluates to 1 if XSI extensions are detected.
#ifdef D_ENV_POSIX_XSI_NONE
    #define D_ENV_XSI_IS_AVAILABLE 0
#else
    #define D_ENV_XSI_IS_AVAILABLE (D_ENV_POSIX_XSI_VERSION > 0)
#endif  // D_ENV_POSIX_XSI_NONE

// 3.1.6
// D_ENV_XSI_VERSION_AT_LEAST
//   macro: checks if XSI version is at least the specified version.
#define D_ENV_XSI_VERSION_AT_LEAST(version) \
    (D_ENV_POSIX_XSI_VERSION >= (version))


#endif  // DJINTERP_ENV_ENV_POSIX_H
