/******************************************************************************
* djinterp [core]                                                        env.h
* 
* djinterp environmental detection header:
* This header provides comprehensive compile-time detection of the compilation
* environment, including:
*   - language standards (C95-C23, C++98-C++23)
*   - compilers (Clang, GCC, MSVC, Intel, Borland) with version info
*   - CPU architectures (x86/x64, ARM, RISC-V, PowerPC, MIPS, etc.)
*   - operating systems using a block/flag classification system
*   - build configuration (Debug/Release)
*   - platform characteristics (endianness, bit width)
*
*   The header creates a unified D_ENV_* macro interface enabling portable code
* that adapts to different platforms, compilers, and architectures. All
* detection is performed at compile-time with zero runtime overhead.
*
*   CONFIGURATION SYSTEM:
*   This header supports custom environment simulation via D_CFG_ENV_CUSTOM:
*   - 0 (default): full automatic detection
*   - 1: skip all detection (requires pre-defined D_ENV_DETECTED_* variables)  
*   - bitfield: selectively skip sections (bit 0=language, 1=compiler, 2=os, 
*               3=arch, 4=build)
*   Pre-defining D_ENV_DETECTED_* variables automatically sets corresponding
*   section bits to enable testing different environments.
* 
*
* path:      \inc\env.h                                           
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2023.03.27
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_
#define DJINTERP_ENVIRONMENT_ 1


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================

// D_CFG_ENV_CUSTOM
//   configuration: master environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_DETECTED_* variables)
//   bitfield: selectively enable/disable detection sections
#ifndef D_CFG_ENV_CUSTOM
    #define D_CFG_ENV_CUSTOM 0
#endif

#include ".\core\config\env_config.h"

// =============================================================================
// II.  LANGUAGE ENVIRONMENT 
// =============================================================================

// language standard version constants
#define D_ENV_LANG_C_STANDARD_C95        199409L
#define D_ENV_LANG_C_STANDARD_C99        199901L  
#define D_ENV_LANG_C_STANDARD_C11        201112L
#define D_ENV_LANG_C_STANDARD_C17        201710L
#define D_ENV_LANG_C_STANDARD_C23        202311L

#define D_ENV_LANG_CPP_STANDARD_CPP98    199711L
#define D_ENV_LANG_CPP_STANDARD_CPP11    201103L
#define D_ENV_LANG_CPP_STANDARD_CPP14    201402L
#define D_ENV_LANG_CPP_STANDARD_CPP17    201703L
#define D_ENV_LANG_CPP_STANDARD_CPP20    202002L
#define D_ENV_LANG_CPP_STANDARD_CPP23    202302L

// language detection logic
#if D_CFG_ENV_LANG_ENABLED
    #ifdef __cplusplus
        #define D_ENV_LANG_DETECTED_CPP

        // only include C++98 header detection if not using modern C++ features
        #if (__cplusplus < D_ENV_LANG_CPP_STANDARD_CPP11)
            #include ".\core\env\env_cpp98.h"
        #else
            #include ".\core\env\env_cpp_features.h"
        #endif

        // C++ standard detection
        #if __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP23
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP23
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++23"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP20
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP20
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++20"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP17
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP17
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++17"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP14
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP14
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++14"
        #elif __cplusplus >= D_ENV_LANG_CPP_STANDARD_CPP11
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP11
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++11"
        #else
            #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP98
            #define D_ENV_LANG_CPP_STANDARD_NAME "C++98"
        #endif
    #endif  // __cplusplus

    // C standard detection
    #ifdef __STDC_VERSION__
        #define D_ENV_LANG_DETECTED_C

        #if (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C23)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C23"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C17)
            #define D_ENV_LANG_C_STANDARD       __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C17"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C11)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C11"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C99)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C99"
        #elif (__STDC_VERSION__ >= D_ENV_LANG_C_STANDARD_C95)
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C95"
        #else
            #define D_ENV_LANG_C_STANDARD      __STDC_VERSION__
            #define D_ENV_LANG_C_STANDARD_NAME "C90"
        #endif  // #if __STDC_VERSION__ 
    #else
        #define D_ENV_LANG_C_STANDARD      199000L
        #define D_ENV_LANG_C_STANDARD_NAME "C90"
    #endif  // __STDC_VERSION__

#else
    // use pre-defined detection variables when language detection is disabled
    #ifdef D_ENV_DETECTED_CPP23
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP23
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++23"
    #elif defined(D_ENV_DETECTED_CPP20)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP20
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++20"
    #elif defined(D_ENV_DETECTED_CPP17)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP17
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++17"
    #elif defined(D_ENV_DETECTED_CPP14)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP14
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++14"
    #elif defined(D_ENV_DETECTED_CPP11)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP11
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++11"
    #elif defined(D_ENV_DETECTED_CPP98)
        #define D_ENV_LANG_CPP_STANDARD      D_ENV_LANG_CPP_STANDARD_CPP98
        #define D_ENV_LANG_CPP_STANDARD_NAME "C++98"
    #endif

    #if defined(D_ENV_DETECTED_C23)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C23
        #define D_ENV_LANG_C_STANDARD_NAME   "C23"
    #elif defined(D_ENV_DETECTED_C17)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C17
        #define D_ENV_LANG_C_STANDARD_NAME   "C17"
    #elif defined(D_ENV_DETECTED_C11)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C11
        #define D_ENV_LANG_C_STANDARD_NAME   "C11"
    #elif defined(D_ENV_DETECTED_C99)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C99
        #define D_ENV_LANG_C_STANDARD_NAME   "C99"
    #elif defined(D_ENV_DETECTED_C95)
        #define D_ENV_LANG_C_STANDARD        D_ENV_LANG_C_STANDARD_C95
        #define D_ENV_LANG_C_STANDARD_NAME   "C95"
    #else
        // fallback when no manual C standard is specified
        #define D_ENV_LANG_C_STANDARD        199000L
        #define D_ENV_LANG_C_STANDARD_NAME   "C90"
    #endif  

#endif  // D_CFG_ENV_LANG_ENABLED

// define convenience macros based on detected language
#ifdef D_ENV_LANG_CPP_STANDARD
    #define D_ENV_LANG_USING_CPP 1

    // D_ENV_LANG_IS_CPP98_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++98 or later.
    #define D_ENV_LANG_IS_CPP98_OR_HIGHER  \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98)

    // D_ENV_LANG_IS_CPP11_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++11 or later.
    #define D_ENV_LANG_IS_CPP11_OR_HIGHER  \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)

    // D_ENV_LANG_IS_CPP14_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++14 or later.
    #define D_ENV_LANG_IS_CPP14_OR_HIGHER  \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP14)

    // D_ENV_LANG_IS_CPP17_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++17 or later.
    #define D_ENV_LANG_IS_CPP17_OR_HIGHER  \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)

    // D_ENV_LANG_IS_CPP20_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++20 or later.
    #define D_ENV_LANG_IS_CPP20_OR_HIGHER  \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20)

    // D_ENV_LANG_IS_CPP23_OR_HIGHER
    //   macro: evaluates to 1 if detected C++ standard is C++23 or later.
    #define D_ENV_LANG_IS_CPP23_OR_HIGHER  \
        (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP23)
#else
    #define D_ENV_LANG_USING_CPP 0
#endif  // D_ENV_LANG_CPP_STANDARD

#if D_ENV_LANG_C_STANDARD
    #define D_ENV_LANG_USING_C 1
#else
    #define D_ENV_LANG_USING_C 0
#endif  // D_ENV_LANG_C_STANDARD

// D_ENV_LANG_IS_C95_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C95 or later.
#define D_ENV_LANG_IS_C95_OR_HIGHER  \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C95)

// D_ENV_LANG_IS_C99_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C99 or later.
#define D_ENV_LANG_IS_C99_OR_HIGHER  \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C99)

// D_ENV_LANG_IS_C11_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C11 or later.
#define D_ENV_LANG_IS_C11_OR_HIGHER  \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C11)

// D_ENV_LANG_IS_C17_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C17 or later.
#define D_ENV_LANG_IS_C17_OR_HIGHER  \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C17)

// D_ENV_LANG_IS_C23_OR_HIGHER
//   macro: evaluates to 1 if detected C standard is C23 or later.
#define D_ENV_LANG_IS_C23_OR_HIGHER  \
    (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C23)

// =============================================================================
// III. POSIX STANDARDS DETECTION
// =============================================================================

// POSIX version constants
#define D_ENV_POSIX_VERSION_1988        198808L  // POSIX.1-1988 (IEEE 1003.1)
#define D_ENV_POSIX_VERSION_1990        199009L  // POSIX.1-1990 (ISO/IEC 9945-1)
#define D_ENV_POSIX_VERSION_1993        199309L  // POSIX.1b-1993 (Real-time extensions)
#define D_ENV_POSIX_VERSION_1996        199506L  // POSIX.1c-1995 (Threads)
#define D_ENV_POSIX_VERSION_2001        200112L  // POSIX.1-2001 (SUSv3)
#define D_ENV_POSIX_VERSION_2008        200809L  // POSIX.1-2008 (SUSv4)
#define D_ENV_POSIX_VERSION_2017        201700L  // POSIX.1-2017 (SUSv5)
#define D_ENV_POSIX_VERSION_2024        202405L  // POSIX.1-2024 (SUSv5.1)

// POSIX feature detection constants
#define D_ENV_POSIX_C_SOURCE_1          1L       // basic POSIX.1 functionality
#define D_ENV_POSIX_C_SOURCE_2          2L       // POSIX.2 functionality
#define D_ENV_POSIX_C_SOURCE_199309L    199309L  // POSIX.1b (real-time)
#define D_ENV_POSIX_C_SOURCE_199506L    199506L  // POSIX.1c (threads)
#define D_ENV_POSIX_C_SOURCE_200112L    200112L  // POSIX.1-2001
#define D_ENV_POSIX_C_SOURCE_200809L    200809L  // POSIX.1-2008

// XSI (X/Open System Interface) levels
#define D_ENV_POSIX_XSI_VERSION_3       3L       // XPG3
#define D_ENV_POSIX_XSI_VERSION_4       4L       // XPG4
#define D_ENV_POSIX_XSI_VERSION_500     500L     // SUSv2/Unix 98
#define D_ENV_POSIX_XSI_VERSION_600     600L     // SUSv3/Unix 03
#define D_ENV_POSIX_XSI_VERSION_700     700L     // SUSv4/Unix 08

// POSIX detection logic
#if D_CFG_ENV_POSIX_ENABLED
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
        #endif
    #endif  // D_ENV_POSIX_VERSION

    // XSI (X/Open System Interface) detection
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
        #endif
    #endif  // D_ENV_POSIX_XSI_VERSION

    // specific POSIX feature detection
    
    // D_ENV_POSIX_FEATURE_THREADS
    //   feature: POSIX threads (pthreads) support detection.
    #ifdef _POSIX_THREADS
        #define D_ENV_POSIX_FEATURE_THREADS 1
    #else
        #define D_ENV_POSIX_FEATURE_THREADS 0
    #endif
    
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
    
    // D_ENV_POSIX_FEATURE_SHARED_MEMORY
    //   feature: POSIX shared memory support detection.
    #ifdef _POSIX_SHARED_MEMORY_OBJECTS
        #define D_ENV_POSIX_FEATURE_SHARED_MEMORY 1
    #else
        #define D_ENV_POSIX_FEATURE_SHARED_MEMORY 0
    #endif
    
    // D_ENV_POSIX_FEATURE_SEMAPHORES
    //   feature: POSIX semaphores support detection.
    #ifdef _POSIX_SEMAPHORES
        #define D_ENV_POSIX_FEATURE_SEMAPHORES 1
    #else
        #define D_ENV_POSIX_FEATURE_SEMAPHORES 0
    #endif
    
    // D_ENV_POSIX_FEATURE_MESSAGE_QUEUES
    //   feature: POSIX message queues support detection.
    #ifdef _POSIX_MESSAGE_PASSING
        #define D_ENV_POSIX_FEATURE_MESSAGE_QUEUES 1
    #else
        #define D_ENV_POSIX_FEATURE_MESSAGE_QUEUES 0
    #endif
    
    // D_ENV_POSIX_FEATURE_MEMORY_MAPPING
    //   feature: POSIX memory mapping (mmap) support detection.
    #ifdef _POSIX_MAPPED_FILES
        #define D_ENV_POSIX_FEATURE_MEMORY_MAPPING 1
    #else
        #define D_ENV_POSIX_FEATURE_MEMORY_MAPPING 0
    #endif

#else
    // use pre-defined detection variables when POSIX detection is disabled
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
    #endif

    // XSI detection (manual)
    #ifdef D_ENV_DETECTED_POSIX_XSI
        #define D_ENV_POSIX_XSI_700     1
        #define D_ENV_POSIX_XSI_VERSION D_ENV_POSIX_XSI_VERSION_700
        #define D_ENV_POSIX_XSI_NAME    "XSI 700"
    #else
        #define D_ENV_POSIX_XSI_NONE    1
        #define D_ENV_POSIX_XSI_VERSION 0L
        #define D_ENV_POSIX_XSI_NAME    "None"
    #endif

    // feature detection (manual)
    #ifdef D_ENV_DETECTED_POSIX_THREADS
        #define D_ENV_POSIX_FEATURE_THREADS 1
    #else
        #define D_ENV_POSIX_FEATURE_THREADS 0
    #endif

    #ifdef D_ENV_DETECTED_POSIX_REALTIME
        #define D_ENV_POSIX_FEATURE_REALTIME 1
    #else
        #define D_ENV_POSIX_FEATURE_REALTIME 0
    #endif

    #ifdef D_ENV_DETECTED_POSIX_SOCKETS
        #define D_ENV_POSIX_FEATURE_SOCKETS 1
    #else
        #define D_ENV_POSIX_FEATURE_SOCKETS 0
    #endif

    #ifndef D_ENV_POSIX_FEATURE_SHARED_MEMORY
        #define D_ENV_POSIX_FEATURE_SHARED_MEMORY 0
    #endif

    #ifndef D_ENV_POSIX_FEATURE_SEMAPHORES
        #define D_ENV_POSIX_FEATURE_SEMAPHORES 0
    #endif

    #ifndef D_ENV_POSIX_FEATURE_MESSAGE_QUEUES
        #define D_ENV_POSIX_FEATURE_MESSAGE_QUEUES 0
    #endif

    #ifndef D_ENV_POSIX_FEATURE_MEMORY_MAPPING
        #define D_ENV_POSIX_FEATURE_MEMORY_MAPPING 0
    #endif

#endif  // D_CFG_ENV_POSIX_ENABLED

// POSIX utility macros

// D_ENV_POSIX_IS_AVAILABLE
//   macro: evaluates to 1 if any POSIX standard is detected, 0 otherwise.
#ifdef D_ENV_POSIX_NONE
    #define D_ENV_POSIX_IS_AVAILABLE 0
#else
    #define D_ENV_POSIX_IS_AVAILABLE (D_ENV_POSIX_VERSION > 0)
#endif

// D_ENV_POSIX_IS_MODERN
//   macro: evaluates to 1 if POSIX.1-2001 or later is detected.
#define D_ENV_POSIX_IS_MODERN     \
    (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001)

// D_ENV_POSIX_HAS_FEATURE
//   macro: checks if a specific POSIX feature is available.
#define D_ENV_POSIX_HAS_FEATURE(feature_macro) (feature_macro)

// D_ENV_POSIX_VERSION_AT_LEAST
//   macro: checks if POSIX version is at least the specified version.
#define D_ENV_POSIX_VERSION_AT_LEAST(version) \
    (D_ENV_POSIX_VERSION >= (version))

// D_ENV_XSI_IS_AVAILABLE
//   macro: evaluates to 1 if XSI extensions are detected.
#ifdef D_ENV_POSIX_XSI_NONE
    #define D_ENV_XSI_IS_AVAILABLE 0
#else
    #define D_ENV_XSI_IS_AVAILABLE (D_ENV_POSIX_XSI_VERSION > 0)
#endif

// D_ENV_XSI_VERSION_AT_LEAST
//   macro: checks if XSI version is at least the specified version.
#define D_ENV_XSI_VERSION_AT_LEAST(version) \
    (D_ENV_POSIX_XSI_VERSION >= (version))

// =============================================================================
// IV.  COMPILER DETECTION
// =============================================================================

// compiler detection logic 
#if D_CFG_ENV_COMPILER_ENABLED
    // clang (check first as it can masquerade as GCC)
    #if defined(__clang__)
        #define D_ENV_COMPILER_CLANG 1
        
        #ifdef __apple_build_version__
            #define D_ENV_COMPILER_APPLE_CLANG  1
            #define D_ENV_COMPILER_NAME         "Apple Clang"
            #define D_ENV_COMPILER_FULL_NAME    "Apple Clang/LLVM"
        #else
            #define D_ENV_COMPILER_NAME         "Clang"
            #define D_ENV_COMPILER_FULL_NAME    "Clang/LLVM"
        #endif
        
        #define D_ENV_COMPILER_MAJOR            __clang_major__
        #define D_ENV_COMPILER_MINOR            __clang_minor__
        #define D_ENV_COMPILER_PATCHLEVEL       __clang_patchlevel__
        #define D_ENV_COMPILER_VERSION_STRING   __clang_version__

    // GCC
    #elif defined(__GNUC__)
        #define D_ENV_COMPILER_GCC 1
        #define D_ENV_COMPILER_NAME             "GCC"
        #define D_ENV_COMPILER_FULL_NAME        "GNU Compiler Collection"
        #define D_ENV_COMPILER_MAJOR            __GNUC__
        #define D_ENV_COMPILER_MINOR            __GNUC_MINOR__

        #ifdef __GNUC_PATCHLEVEL__
            #define D_ENV_COMPILER_PATCHLEVEL   __GNUC_PATCHLEVEL__
        #else
            #define D_ENV_COMPILER_PATCHLEVEL   0
        #endif

        #ifdef __VERSION__
            #define D_ENV_COMPILER_VERSION_STRING __VERSION__
        #else
            #define D_ENV_COMPILER_VERSION_STRING "GCC (version unknown)"
        #endif

    // Microsoft Visual C++
    #elif defined(_MSC_VER)
        #define D_ENV_COMPILER_MSVC             1
        #define D_ENV_COMPILER_NAME             "MSVC"
        #define D_ENV_COMPILER_FULL_NAME        "Microsoft Visual C++"
        
        // version mapping for MSVC
        #if _MSC_VER >= 1930
            #define D_ENV_COMPILER_MAJOR 17
        #elif _MSC_VER >= 1920
            #define D_ENV_COMPILER_MAJOR 16
        #elif _MSC_VER >= 1910
            #define D_ENV_COMPILER_MAJOR 15
        #elif _MSC_VER >= 1900
            #define D_ENV_COMPILER_MAJOR 14
        #elif _MSC_VER >= 1800
            #define D_ENV_COMPILER_MAJOR 12
        #elif _MSC_VER >= 1700
            #define D_ENV_COMPILER_MAJOR 11
        #elif _MSC_VER >= 1600
            #define D_ENV_COMPILER_MAJOR 10
        #else
            #define D_ENV_COMPILER_MAJOR 9
        #endif
        
        #define D_ENV_COMPILER_MINOR            ( (_MSC_VER % 100) / 10)
        #define D_ENV_COMPILER_PATCHLEVEL         (_MSC_VER % 10)
        #define D_ENV_COMPILER_VERSION_STRING   "MSVC"

    // Intel C++
    #elif ( defined(__INTEL_COMPILER) ||  \
            defined(__ICL)            ||  \
            defined(__ICC) )
        #define D_ENV_COMPILER_INTEL 1
        #define D_ENV_COMPILER_NAME            "Intel C++"
        #define D_ENV_COMPILER_FULL_NAME       "Intel C++ Compiler"
        
        #ifdef __INTEL_COMPILER
            #define D_ENV_COMPILER_MAJOR        (__INTEL_COMPILER / 100)
            #define D_ENV_COMPILER_MINOR        ((__INTEL_COMPILER % 100) / 10)
            #define D_ENV_COMPILER_PATCHLEVEL   (__INTEL_COMPILER % 10)
        #else                                   
            #define D_ENV_COMPILER_MAJOR        0
            #define D_ENV_COMPILER_MINOR        0
            #define D_ENV_COMPILER_PATCHLEVEL   0
        #endif
        #define D_ENV_COMPILER_VERSION_STRING   "Intel C++"

    // Borland/Turbo C++
    #elif ( defined(__BORLANDC__) ||    \
            defined(__TURBOC__) )
        #define D_ENV_COMPILER_BORLAND          1
        #define D_ENV_COMPILER_NAME             "Borland C++"
        #define D_ENV_COMPILER_FULL_NAME        "Borland C++ Compiler"
        
        #ifdef __BORLANDC__
            #define D_ENV_COMPILER_MAJOR        (__BORLANDC__ >> 8)
            #define D_ENV_COMPILER_MINOR        ((__BORLANDC__ & 0xFF) >> 4)
            #define D_ENV_COMPILER_PATCHLEVEL   (__BORLANDC__ & 0x0F)
        #else
            #define D_ENV_COMPILER_MAJOR        0
            #define D_ENV_COMPILER_MINOR        0
            #define D_ENV_COMPILER_PATCHLEVEL   0
        #endif

        #define D_ENV_COMPILER_VERSION_STRING   "Borland C++"

    // unknown compiler
    #else
        #define D_ENV_COMPILER_UNKNOWN          1
        #define D_ENV_COMPILER_NAME             "unknown"
        #define D_ENV_COMPILER_FULL_NAME        "unknown Compiler"
        #define D_ENV_COMPILER_MAJOR            0
        #define D_ENV_COMPILER_MINOR            0
        #define D_ENV_COMPILER_PATCHLEVEL       0
        #define D_ENV_COMPILER_VERSION_STRING   "unknown"
    #endif
#else
// use pre-defined detection variables when compiler detection is disabled
    #ifdef D_ENV_DETECTED_COMPILER_APPLE_CLANG
        #define D_ENV_COMPILER_APPLE_CLANG 1
        #define D_ENV_COMPILER_CLANG     1
        #define D_ENV_COMPILER_NAME      "Apple Clang"
        #define D_ENV_COMPILER_FULL_NAME "Apple Clang/LLVM"
    #elif defined(D_ENV_DETECTED_COMPILER_CLANG)
        #define D_ENV_COMPILER_CLANG     1
        #define D_ENV_COMPILER_NAME      "Clang"
        #define D_ENV_COMPILER_FULL_NAME "Clang/LLVM"
    #elif defined(D_ENV_DETECTED_COMPILER_GCC)
        #define D_ENV_COMPILER_GCC       1
        #define D_ENV_COMPILER_NAME      "GCC"
        #define D_ENV_COMPILER_FULL_NAME "GNU Compiler Collection"
    #elif defined(D_ENV_DETECTED_COMPILER_MSVC)
        #define D_ENV_COMPILER_MSVC      1
        #define D_ENV_COMPILER_NAME      "MSVC"
        #define D_ENV_COMPILER_FULL_NAME "Microsoft Visual C++"
    #elif defined(D_ENV_DETECTED_COMPILER_INTEL)
        #define D_ENV_COMPILER_INTEL     1
        #define D_ENV_COMPILER_NAME      "Intel C++"
        #define D_ENV_COMPILER_FULL_NAME "Intel C++ Compiler"
    #elif defined(D_ENV_DETECTED_COMPILER_BORLAND)
        #define D_ENV_COMPILER_BORLAND    1
        #define D_ENV_COMPILER_NAME      "Borland C++"
        #define D_ENV_COMPILER_FULL_NAME "Borland C++ Compiler"
    #elif defined(D_ENV_DETECTED_COMPILER_UNKNOWN)
        #define D_ENV_COMPILER_UNKNOWN   1
        #define D_ENV_COMPILER_NAME      "Unknown"
        #define D_ENV_COMPILER_FULL_NAME "Unknown Compiler"
    #endif

    // default version info when using detected variables
    #ifndef D_ENV_COMPILER_MAJOR
        #define D_ENV_COMPILER_MAJOR          0
        #define D_ENV_COMPILER_MINOR          0
        #define D_ENV_COMPILER_PATCHLEVEL     0
        #define D_ENV_COMPILER_VERSION_STRING "simulated"
    #endif  // D_ENV_COMPILER_MAJOR
#endif  // !D_CFG_ENV_COMPILER_ENABLED

// D_ENV_COMPILER_VERSION_AT_LEAST
//   macro: utility macro for version checking, ensuring that the compiler
// version is greater than, or equal to, the version specified.
#define D_ENV_COMPILER_VERSION_AT_LEAST(major, minor, patch) \
    ( (D_ENV_COMPILER_MAJOR > (major))     ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR > (minor)) )  ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR == (minor)) &&    \
       (D_ENV_COMPILER_PATCHLEVEL >= (patch)) ) )

// D_ENV_COMPILER_VERSION_AT_MOST
//   macro: utility macro for version checking, ensuring that the compiler
// version is less than, or equal to, the version specified.
#define D_ENV_COMPILER_VERSION_AT_MOST(major, minor, patch) \
    ( (D_ENV_COMPILER_MAJOR < (major))     ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR < (minor)) )  ||  \
     ( (D_ENV_COMPILER_MAJOR == (major)) &&    \
       (D_ENV_COMPILER_MINOR == (minor)) &&    \
       (D_ENV_COMPILER_PATCHLEVEL <= (patch)) ) )

#ifndef D_ENV_PP_HAS_VA_OPT
    #ifdef D_ENV_LANG_USING_CPP
        #if ( defined(__cpp_va_opt) &&  \
              (__cpp_va_opt >= 201803L) )
            #define D_ENV_PP_HAS_VA_OPT 1
        #else
            #define D_ENV_PP_HAS_VA_OPT 0
        #endif  // defined(__cpp_va_opt) && (__cpp_va_opt >= 201803L)
    #else
        // detection method: when __VA_OPT__ is supported and __VA_ARGS__ is non-empty,
        // __VA_OPT__(,) expands to ",", which shifts argument selection.
        // We pass a dummy argument (~) to ensure __VA_ARGS__ is non-empty.
        //
        // If __VA_OPT__ works:   __VA_OPT__(,) -> "," -> PP_THIRD selects arg 2 (1)
        // If __VA_OPT__ literal: no comma inserted   -> PP_THIRD selects arg 3 (0)
        
        #define D_VA_OPT_THIRD_ARG_(a, b, c, ...) c
        #define D_VA_OPT_THIRD_ARG(...)           D_VA_OPT_THIRD_ARG_(__VA_ARGS__)
        #define D_VA_OPT_PROBE_(...)              D_VA_OPT_THIRD_ARG(__VA_OPT__(,), 1, 0, )
        #define D_ENV_PP_HAS_VA_OPT               D_VA_OPT_PROBE_(~)
    #endif  // D_ENV_LANG_USING_CPP
#endif  // D_ENV_PP_HAS_VA_OPT

// D_ENV_PP_HAS_VA_OPT_ENABLED
//   macro: function-style wrapper for cleaner conditionals
#define D_ENV_PP_HAS_VA_OPT_ENABLED  \
    D_ENV_PP_HAS_VA_OPT

// =============================================================================
// IV-A. PREPROCESSOR LIMITS
// =============================================================================
// This section defines preprocessor translation limits based on the C/C++
// standard and compiler-specific implementations. These limits describe the
// maximum capabilities guaranteed or supported by the environment.
//
// The C standard specifies MINIMUM limits that conforming implementations
// must support. Actual implementations typically exceed these minimums.
//
// Key limits defined:
//   D_ENV_PP_MIN_MACRO_ARGS      - Standard-mandated minimum macro arguments
//   D_ENV_PP_MAX_MACRO_ARGS      - Compiler-specific practical maximum
//   D_ENV_PP_MIN_NESTING_DEPTH   - Standard-mandated minimum #include nesting
//   D_ENV_PP_MAX_NESTING_DEPTH   - Compiler-specific practical maximum
//   D_ENV_PP_MIN_MACRO_IDS       - Standard-mandated minimum macro identifiers
//   D_ENV_PP_MIN_PARAMS          - Standard-mandated minimum function parameters
//   D_ENV_PP_MIN_STRING_LENGTH   - Standard-mandated minimum string literal length

// -----------------------------------------------------------------------------
// Standard Translation Limits (from ISO C/C++)
// -----------------------------------------------------------------------------
// C89/C90 (ANSI C) minimum limits:
#define D_ENV_PP_LIMIT_C89_MACRO_ARGS       31
#define D_ENV_PP_LIMIT_C89_NESTING_DEPTH    8
#define D_ENV_PP_LIMIT_C89_MACRO_IDS        1024
#define D_ENV_PP_LIMIT_C89_PARAMS           31
#define D_ENV_PP_LIMIT_C89_STRING_LENGTH    509

// C99/C11/C17/C23 minimum limits:
#define D_ENV_PP_LIMIT_C99_MACRO_ARGS       127
#define D_ENV_PP_LIMIT_C99_NESTING_DEPTH    15
#define D_ENV_PP_LIMIT_C99_MACRO_IDS        4095
#define D_ENV_PP_LIMIT_C99_PARAMS           127
#define D_ENV_PP_LIMIT_C99_STRING_LENGTH    4095

// C++ minimum limits (similar to C99+ for modern standards):
#define D_ENV_PP_LIMIT_CPP_MACRO_ARGS       256
#define D_ENV_PP_LIMIT_CPP_NESTING_DEPTH    256
#define D_ENV_PP_LIMIT_CPP_MACRO_IDS        65536
#define D_ENV_PP_LIMIT_CPP_PARAMS           256
#define D_ENV_PP_LIMIT_CPP_STRING_LENGTH    65536

// -----------------------------------------------------------------------------
// Standard-Based Minimum Limits
// -----------------------------------------------------------------------------
// D_ENV_PP_MIN_MACRO_ARGS
//   The minimum number of arguments in a macro invocation that any conforming
//   implementation must support, based on the detected language standard.
#ifdef D_ENV_LANG_CPP_STANDARD
    #define D_ENV_PP_MIN_MACRO_ARGS     D_ENV_PP_LIMIT_CPP_MACRO_ARGS
    #define D_ENV_PP_MIN_NESTING_DEPTH  D_ENV_PP_LIMIT_CPP_NESTING_DEPTH
    #define D_ENV_PP_MIN_MACRO_IDS      D_ENV_PP_LIMIT_CPP_MACRO_IDS
    #define D_ENV_PP_MIN_PARAMS         D_ENV_PP_LIMIT_CPP_PARAMS
    #define D_ENV_PP_MIN_STRING_LENGTH  D_ENV_PP_LIMIT_CPP_STRING_LENGTH
#elif D_ENV_LANG_IS_C99_OR_HIGHER
    #define D_ENV_PP_MIN_MACRO_ARGS     D_ENV_PP_LIMIT_C99_MACRO_ARGS
    #define D_ENV_PP_MIN_NESTING_DEPTH  D_ENV_PP_LIMIT_C99_NESTING_DEPTH
    #define D_ENV_PP_MIN_MACRO_IDS      D_ENV_PP_LIMIT_C99_MACRO_IDS
    #define D_ENV_PP_MIN_PARAMS         D_ENV_PP_LIMIT_C99_PARAMS
    #define D_ENV_PP_MIN_STRING_LENGTH  D_ENV_PP_LIMIT_C99_STRING_LENGTH
#else
    #define D_ENV_PP_MIN_MACRO_ARGS     D_ENV_PP_LIMIT_C89_MACRO_ARGS
    #define D_ENV_PP_MIN_NESTING_DEPTH  D_ENV_PP_LIMIT_C89_NESTING_DEPTH
    #define D_ENV_PP_MIN_MACRO_IDS      D_ENV_PP_LIMIT_C89_MACRO_IDS
    #define D_ENV_PP_MIN_PARAMS         D_ENV_PP_LIMIT_C89_PARAMS
    #define D_ENV_PP_MIN_STRING_LENGTH  D_ENV_PP_LIMIT_C89_STRING_LENGTH
#endif

// -----------------------------------------------------------------------------
// Compiler-Specific Practical Limits  
// -----------------------------------------------------------------------------
// These values represent practical/documented limits for specific compilers.
// Note: GCC and Clang do not impose hard limits on macro arguments; they are
// constrained only by available memory and recursion depth. The values below
// are conservative practical limits tested to work reliably.

#if defined(D_ENV_COMPILER_GCC)
    // GCC: No hard-coded limit; memory-constrained
    // tested to reliably handle 10000+ arguments
    #define D_ENV_PP_MAX_MACRO_ARGS         10000
    #define D_ENV_PP_MAX_NESTING_DEPTH      200
    #define D_ENV_PP_MAX_MACRO_IDS          0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_MAX_STRING_LENGTH      0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_LIMIT_SOURCE           "GCC (practical)"

#elif defined(D_ENV_COMPILER_CLANG)
    // Clang: Similar to GCC, no hard limit
    // default macro recursion depth is 256, adjustable via -fmacro-backtrace-limit
    #define D_ENV_PP_MAX_MACRO_ARGS         10000
    #define D_ENV_PP_MAX_NESTING_DEPTH      256
    #define D_ENV_PP_MAX_MACRO_IDS          0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_MAX_STRING_LENGTH      0       // 0 = unlimited (memory-bound)
    #define D_ENV_PP_LIMIT_SOURCE           "Clang (practical)"

#elif defined(D_ENV_COMPILER_MSVC)
    // MSVC: More conservative, historically stricter limits
    // /Za (strict ANSI) mode may impose additional restrictions
    #define D_ENV_PP_MAX_MACRO_ARGS         127
    #define D_ENV_PP_MAX_NESTING_DEPTH      256
    #define D_ENV_PP_MAX_MACRO_IDS          16380
    #define D_ENV_PP_MAX_STRING_LENGTH      16380
    #define D_ENV_PP_LIMIT_SOURCE           "MSVC (documented)"

#elif defined(D_ENV_COMPILER_INTEL)
    // Intel C++: Generally follows GCC/Clang behavior on Linux,
    // MSVC behavior on Windows
    #if defined(D_ENV_PLATFORM_WINDOWS)
        #define D_ENV_PP_MAX_MACRO_ARGS     127
        #define D_ENV_PP_MAX_NESTING_DEPTH  256
        #define D_ENV_PP_MAX_MACRO_IDS      16380
        #define D_ENV_PP_MAX_STRING_LENGTH  16380
        #define D_ENV_PP_LIMIT_SOURCE       "Intel (Windows/MSVC-compat)"
    #else
        #define D_ENV_PP_MAX_MACRO_ARGS     10000
        #define D_ENV_PP_MAX_NESTING_DEPTH  200
        #define D_ENV_PP_MAX_MACRO_IDS      0
        #define D_ENV_PP_MAX_STRING_LENGTH  0
        #define D_ENV_PP_LIMIT_SOURCE       "Intel (Linux/GCC-compat)"
    #endif

#elif defined(D_ENV_COMPILER_BORLAND)
    // Borland/Turbo C++: Legacy compiler with stricter limits
    #define D_ENV_PP_MAX_MACRO_ARGS         32
    #define D_ENV_PP_MAX_NESTING_DEPTH      32
    #define D_ENV_PP_MAX_MACRO_IDS          1024
    #define D_ENV_PP_MAX_STRING_LENGTH      4096
    #define D_ENV_PP_LIMIT_SOURCE           "Borland (estimated)"

#else
    // unknown compiler: fall back to standard minimums
    #define D_ENV_PP_MAX_MACRO_ARGS         D_ENV_PP_MIN_MACRO_ARGS
    #define D_ENV_PP_MAX_NESTING_DEPTH      D_ENV_PP_MIN_NESTING_DEPTH
    #define D_ENV_PP_MAX_MACRO_IDS          D_ENV_PP_MIN_MACRO_IDS
    #define D_ENV_PP_MAX_STRING_LENGTH      D_ENV_PP_MIN_STRING_LENGTH
    #define D_ENV_PP_LIMIT_SOURCE           "Unknown (standard minimum)"

#endif

// -----------------------------------------------------------------------------
// Preprocessor Limit Utility Macros
// -----------------------------------------------------------------------------

// D_ENV_PP_ARGS_WITHIN_LIMIT
//   macro: evaluates to 1 if the given count is within the practical limit.
// usage:
//   #if D_ENV_PP_ARGS_WITHIN_LIMIT(64)
//       // safe to use 64-argument macro
//   #endif
#define D_ENV_PP_ARGS_WITHIN_LIMIT(count) \
    ((count) <= D_ENV_PP_MAX_MACRO_ARGS)

// D_ENV_PP_ARGS_WITHIN_STANDARD
//   macro: evaluates to 1 if the given count is within the standard minimum.
// usage:
//   #if D_ENV_PP_ARGS_WITHIN_STANDARD(31)
//       // portable across all conforming compilers
//   #endif
#define D_ENV_PP_ARGS_WITHIN_STANDARD(count) \
    ((count) <= D_ENV_PP_MIN_MACRO_ARGS)

// D_ENV_PP_IS_UNLIMITED
//   macro: evaluates to 1 if the compiler has no hard limit (value is 0).
#define D_ENV_PP_IS_UNLIMITED(limit) ((limit) == 0)

// D_ENV_PP_EFFECTIVE_LIMIT
//   macro: returns the effective limit, treating 0 as a large practical value.
#define D_ENV_PP_EFFECTIVE_LIMIT(limit) \
    (D_ENV_PP_IS_UNLIMITED(limit) ? 2147483647L : (limit))

// =============================================================================
// V.   ARCHITECTURE DETECTION
// =============================================================================

// architecture type constants
#define D_ENV_ARCH_TYPE_X86     0
#define D_ENV_ARCH_TYPE_X64     1  
#define D_ENV_ARCH_TYPE_ARM     2
#define D_ENV_ARCH_TYPE_ARM64   3
#define D_ENV_ARCH_TYPE_RISCV   4
#define D_ENV_ARCH_TYPE_POWERPC 5
#define D_ENV_ARCH_TYPE_MIPS    6
#define D_ENV_ARCH_TYPE_SPARC   7
#define D_ENV_ARCH_TYPE_S390    8
#define D_ENV_ARCH_TYPE_IA64    9
#define D_ENV_ARCH_TYPE_ALPHA   10
#define D_ENV_ARCH_TYPE_UNKNOWN 11

// endianness constants  
#define D_ENV_ARCH_ENDIAN_UNKNOWN 0
#define D_ENV_ARCH_ENDIAN_LITTLE  1
#define D_ENV_ARCH_ENDIAN_BIG     2

// architecture detection logic
#if (D_CFG_ENV_ARCH_ENABLED)

    // x86/x64 family
    #if ( defined(_M_X64)     ||  \
          defined(__x86_64__) ||  \
          defined(__x86_64)   ||  \
          defined(__amd64__)  ||  \
          defined(__amd64) )
        #define D_ENV_ARCH_X64      1
        #define D_ENV_ARCH_NAME     "x86-64"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_X64
        #define D_ENV_ARCH_BITS     64
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_LITTLE

    #elif ( defined(_M_IX86)  ||  \
            defined(__i386__) ||  \
            defined(__i386)   ||  \
            defined(i386) )
        #define D_ENV_ARCH_X86 1
        #define D_ENV_ARCH_NAME     "x86"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_X86
        #define D_ENV_ARCH_BITS     32
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_LITTLE

    // ARM family
    #elif ( defined(_M_ARM64) ||  \
            defined(__aarch64__) )
        #define D_ENV_ARCH_ARM64    1
        #define D_ENV_ARCH_NAME     "ARM64"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_ARM64
        #define D_ENV_ARCH_BITS     64
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_LITTLE

    #elif ( defined(_M_ARM)  ||  \
            defined(__arm__) ||  \
            defined(__thumb__) )
        #define D_ENV_ARCH_ARM      1
        #define D_ENV_ARCH_NAME     "ARM"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_ARM
        #define D_ENV_ARCH_BITS     32
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_LITTLE

    // RISC-V
    #elif defined(__riscv)
        #define D_ENV_ARCH_RISCV 1

        #if ( defined(__riscv_xlen) && \
              (__riscv_xlen == 64) )
            #define D_ENV_ARCH_RISCV64  1
            #define D_ENV_ARCH_NAME     "RISC-V 64"
            #define D_ENV_ARCH_BITS     64
        #else
            #define D_ENV_ARCH_RISCV32  1
            #define D_ENV_ARCH_NAME     "RISC-V 32"
            #define D_ENV_ARCH_BITS     32
        #endif
        #define D_ENV_ARCH_TYPE D_ENV_ARCH_TYPE_RISCV
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // PowerPC
    #elif ( defined(__powerpc__)   ||  \
            defined(__powerpc64__) ||  \
            defined(__PPC__)       ||  \
            defined(__PPC64__) )
        #define D_ENV_ARCH_POWERPC 1
        #if defined(__powerpc64__) || defined(__PPC64__)
            #define D_ENV_ARCH_POWERPC64 1
            #define D_ENV_ARCH_NAME      "PowerPC 64"
            #define D_ENV_ARCH_BITS      64
        #else
            #define D_ENV_ARCH_POWERPC32 1
            #define D_ENV_ARCH_NAME      "PowerPC 32"
            #define D_ENV_ARCH_BITS      32
        #endif
        #define D_ENV_ARCH_TYPE D_ENV_ARCH_TYPE_POWERPC
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_BIG

    // MIPS
    #elif ( defined(__mips__) ||  \
            defined(__mips)   ||  \
            defined(__MIPS__) )
        #define D_ENV_ARCH_MIPS 1
        #if ( defined(_MIPS_ARCH_MIPS64) ||  \
              defined(__mips64) )
            #define D_ENV_ARCH_MIPS64   1
            #define D_ENV_ARCH_NAME     "MIPS 64"
            #define D_ENV_ARCH_BITS     64
        #else
            #define D_ENV_ARCH_MIPS32   1
            #define D_ENV_ARCH_NAME     "MIPS 32"
            #define D_ENV_ARCH_BITS     32
        #endif
        #define D_ENV_ARCH_TYPE         D_ENV_ARCH_TYPE_MIPS
        #define D_ENV_ARCH_ENDIAN       D_ENV_ARCH_ENDIAN_BIG

    // SPARC
    #elif ( defined(__sparc__) ||  \
            defined(__sparc) )
        #define D_ENV_ARCH_SPARC 1

        #if (defined(__sparc64__) ||  \
             defined(__sparcv9) )
            #define D_ENV_ARCH_SPARC64  1
            #define D_ENV_ARCH_NAME     "sPARC 64"
            #define D_ENV_ARCH_BITS     64
        #else
            #define D_ENV_ARCH_SPARC32  1
            #define D_ENV_ARCH_NAME     "sPARC 32"
            #define D_ENV_ARCH_BITS     32
        #endif

        #define D_ENV_ARCH_TYPE         D_ENV_ARCH_TYPE_SPARC
        #define D_ENV_ARCH_ENDIAN       D_ENV_ARCH_ENDIAN_BIG

    // IBM System/390
    #elif ( defined(__s390__) ||  \
            defined(__s390x__) )
        #define D_ENV_ARCH_S390 1

        #if defined(__s390x__)
            #define D_ENV_ARCH_S390X    1
            #define D_ENV_ARCH_NAME     "system/390 64"
            #define D_ENV_ARCH_BITS     64
        #else
            #define D_ENV_ARCH_NAME     "system/390 32"
            #define D_ENV_ARCH_BITS     32
        #endif  // defined(__s390x__)

        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_S390
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_BIG
    // Itanium (IA-64)
    #elif ( defined(__ia64__) ||  \
            defined(_IA64)    ||  \
            defined(__IA64__) ||  \
            defined(_M_IA64) )
        #define D_ENV_ARCH_IA64     1
        #define D_ENV_ARCH_NAME     "Itanium"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_IA64
        #define D_ENV_ARCH_BITS     64
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_LITTLE
    // Alpha
    #elif ( defined(__alpha__) ||  \
            defined(__alpha)   ||  \
            defined(_M_ALPHA) )
        #define D_ENV_ARCH_ALPHA    1
        #define D_ENV_ARCH_NAME     "Alpha"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_ALPHA
        #define D_ENV_ARCH_BITS     64
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_LITTLE
    // Unknown architecture
    #else
        #define D_ENV_ARCH_UNKNOWN  1
        #define D_ENV_ARCH_NAME     "Unknown"
        #define D_ENV_ARCH_TYPE     D_ENV_ARCH_TYPE_UNKNOWN
        #define D_ENV_ARCH_BITS     0
        #define D_ENV_ARCH_ENDIAN   D_ENV_ARCH_ENDIAN_UNKNOWN
    #endif
#else
    // use pre-defined detection variables when architecture detection is disabled
    #ifdef D_ENV_DETECTED_ARCH_X64
        #define D_ENV_ARCH_X64     1
        #define D_ENV_ARCH_NAME    "x86-64"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_X64
        #define D_ENV_ARCH_BITS    64
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_X86)
        #define D_ENV_ARCH_X86     1
        #define D_ENV_ARCH_NAME    "x86"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_X86
        #define D_ENV_ARCH_BITS    32
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_ARM64)
        #define D_ENV_ARCH_ARM64   1
        #define D_ENV_ARCH_NAME    "ARM64"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_ARM64
        #define D_ENV_ARCH_BITS    64
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_ARM)
        #define D_ENV_ARCH_ARM     1
        #define D_ENV_ARCH_NAME    "ARM"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_ARM
        #define D_ENV_ARCH_BITS    32
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_RISCV)
        #define D_ENV_ARCH_RISCV   1
        #define D_ENV_ARCH_NAME    "RISC-V"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_RISCV
        #define D_ENV_ARCH_BITS    64  // default assumption
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_POWERPC)
        #define D_ENV_ARCH_POWERPC 1
        #define D_ENV_ARCH_NAME    "PowerPC"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_POWERPC
        #define D_ENV_ARCH_BITS    64  // default assumption
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG
    #elif defined(D_ENV_DETECTED_ARCH_MIPS)
        #define D_ENV_ARCH_MIPS    1
        #define D_ENV_ARCH_NAME    "MIPS"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_MIPS
        #define D_ENV_ARCH_BITS    64  // default assumption
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG
    #elif defined(D_ENV_DETECTED_ARCH_SPARC)
        #define D_ENV_ARCH_SPARC   1
        #define D_ENV_ARCH_NAME    "sPARC"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_SPARC
        #define D_ENV_ARCH_BITS    64  // default assumption
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG
    #elif defined(D_ENV_DETECTED_ARCH_S390)
        #define D_ENV_ARCH_S390    1
        #define D_ENV_ARCH_NAME    "system/390"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_S390
        #define D_ENV_ARCH_BITS    64  // default assumption
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG
    #elif defined(D_ENV_DETECTED_ARCH_IA64)
        #define D_ENV_ARCH_IA64    1
        #define D_ENV_ARCH_NAME    "Itanium"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_IA64
        #define D_ENV_ARCH_BITS    64
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_ALPHA)
        #define D_ENV_ARCH_ALPHA   1
        #define D_ENV_ARCH_NAME    "Alpha"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_ALPHA
        #define D_ENV_ARCH_BITS    64
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE
    #elif defined(D_ENV_DETECTED_ARCH_UNKNOWN)
        #define D_ENV_ARCH_UNKNOWN 1
        #define D_ENV_ARCH_NAME    "Unknown"
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_UNKNOWN
        #define D_ENV_ARCH_BITS    0
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_UNKNOWN
    #endif  
#endif  // D_CFG_ENV_ARCH_ENABLED

// architecture family macros
#if ( defined(D_ENV_ARCH_X86) ||  \
      defined(D_ENV_ARCH_X64) )
    #define D_ENV_ARCH_IS_X86_FAMILY 1
#else
    #define D_ENV_ARCH_IS_X86_FAMILY 0
#endif  // defined(D_ENV_ARCH_X86/64)

#if ( defined(D_ENV_ARCH_ARM) ||  \
      defined(D_ENV_ARCH_ARM64) )
    #define D_ENV_ARCH_IS_ARM_FAMILY 1
#else
    #define D_ENV_ARCH_IS_ARM_FAMILY 0
#endif  // defined(D_ENV_ARCH_ARM/ARM64)

#if (D_ENV_ARCH_BITS == 64)
    #define D_ENV_ARCH_IS_64BIT 1
#else
    #define D_ENV_ARCH_IS_64BIT 0
#endif  // (D_ENV_ARCH_BITS == 64)

#if (D_ENV_ARCH_BITS == 32)
    #define D_ENV_ARCH_IS_32BIT 1
#else
    #define D_ENV_ARCH_IS_32BIT 0
#endif  // (D_ENV_ARCH_BITS == 32)

#if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE)
    #define D_ENV_ARCH_IS_LITTLE_ENDIAN 1
#else
    #define D_ENV_ARCH_IS_LITTLE_ENDIAN 0
#endif  // (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE)

#if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)
    #define D_ENV_ARCH_IS_BIG_ENDIAN 1
#else
    #define D_ENV_ARCH_IS_BIG_ENDIAN 0
#endif  // (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)

// =============================================================================
// VI.  OPERATING SYSTEM DETECTION
// =============================================================================

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

// =============================================================================
// VII.  C/C++ FEATURES
// =============================================================================

// -----------------------------------------------------------------------------
// A.  Threading and Concurrency Support
// -----------------------------------------------------------------------------

#ifdef __STDC_HOSTED__

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
    #if D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(D_ENV_OS_ID)
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
//   feature: detect if stdalign.h is available (C11+; deprecated in C23
// where alignof/alignas are keywords, but header still exists)
#ifndef D_ENV_C_HAS_STDALIGN_H
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #define D_ENV_C_HAS_STDALIGN_H 1
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
    #if D_ENV_IS_OS_POSIX_LIKE_OR_ANDROID(D_ENV_OS_ID)
        #define D_ENV_C_HAS_UNISTD_H 1
    #else
        #define D_ENV_C_HAS_UNISTD_H 0
    #endif
#endif

// D_ENV_C_HAS_SYS_TYPES_H
//   feature: detect if sys/types.h is available
#ifndef D_ENV_C_HAS_SYS_TYPES_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_SYS_TYPES_H 1
    #else
        #define D_ENV_C_HAS_SYS_TYPES_H 0
    #endif
#endif

// D_ENV_C_HAS_SYS_STAT_H
//   feature: detect if sys/stat.h is available
#ifndef D_ENV_C_HAS_SYS_STAT_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_SYS_STAT_H 1
    #else
        #define D_ENV_C_HAS_SYS_STAT_H 0
    #endif
#endif

// D_ENV_C_HAS_DIRENT_H
//   feature: detect if dirent.h is available for directory operations
#ifndef D_ENV_C_HAS_DIRENT_H
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
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
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_STRTOK_R 1
    #else
        #define D_ENV_C_HAS_STRTOK_R 0
    #endif
#endif

// D_ENV_C_HAS_STRTOK_S
//   feature: detect if strtok_s (C11 Annex K / MSVC reentrant strtok)
// is available
#ifndef D_ENV_C_HAS_STRTOK_S
    #if ( D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)  ||  \
          defined(D_ENV_COMPILER_MSVC)       ||  \
          defined(__STDC_LIB_EXT1__) )
        #define D_ENV_C_HAS_STRTOK_S 1
    #else
        #define D_ENV_C_HAS_STRTOK_S 0
    #endif
#endif

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
#endif

// D_ENV_C_HAS_STRDUP
//   feature: detect if strdup is available (POSIX, standardized in C23)
#ifndef D_ENV_C_HAS_STRDUP
    #if ( D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID) ||  \
          D_ENV_LANG_IS_C23_OR_HIGHER )
        #define D_ENV_C_HAS_STRDUP 1
    #else
        #define D_ENV_C_HAS_STRDUP 0
    #endif
#endif

// D_ENV_C_HAS_STRNDUP
//   feature: detect if strndup is available (POSIX, standardized in C23)
#ifndef D_ENV_C_HAS_STRNDUP
    #if ( D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID) ||  \
          D_ENV_LANG_IS_C23_OR_HIGHER )
        #define D_ENV_C_HAS_STRNDUP 1
    #else
        #define D_ENV_C_HAS_STRNDUP 0
    #endif
#endif

// D_ENV_C_HAS_STRCASECMP
//   feature: detect if strcasecmp is available (POSIX)
#ifndef D_ENV_C_HAS_STRCASECMP
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
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
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_MEMCCPY 1
    #else
        #define D_ENV_C_HAS_MEMCCPY 0
    #endif
#endif


// -----------------------------------------------------------------------------
// E.  File System and I/O Features
// -----------------------------------------------------------------------------

// D_ENV_C_HAS_FLOCK
//   feature: detect if flock (file locking) is available
#ifndef D_ENV_C_HAS_FLOCK
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_FLOCK 1
    #else
        #define D_ENV_C_HAS_FLOCK 0
    #endif
#endif

// D_ENV_C_HAS_FOPEN_S
//   feature: detect if fopen_s is available (C11 Annex K / MSVC)
#ifndef D_ENV_C_HAS_FOPEN_S
    #if ( D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) ||                \
          (defined(__STDC_LIB_EXT1__)) )
        #define D_ENV_C_HAS_FOPEN_S 1
    #else
        #define D_ENV_C_HAS_FOPEN_S 0
    #endif
#endif

// D_ENV_C_HAS_FSYNC
//   feature: detect if fsync is available (POSIX)
#ifndef D_ENV_C_HAS_FSYNC
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_FSYNC 1
    #else
        #define D_ENV_C_HAS_FSYNC 0
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

// D_ENV_C_HAS_MMAP
//   feature: detect if mmap (memory-mapped files) is available
#ifndef D_ENV_C_HAS_MMAP
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_MMAP 1
    #else
        #define D_ENV_C_HAS_MMAP 0
    #endif
#endif

// D_ENV_C_HAS_SCANF_S
//   feature: detect if scanf_s is available (C11 Annex K / MSVC)
#ifndef D_ENV_C_HAS_SCANF_S
    #if ( defined(__STDC_LIB_EXT1__) ||  \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        #define D_ENV_C_HAS_SCANF_S 1
    #else
        #define D_ENV_C_HAS_SCANF_S 0
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
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_CLOCK_GETTIME 1
    #else
        #define D_ENV_C_HAS_CLOCK_GETTIME 0
    #endif
#endif

// D_ENV_C_HAS_GETTIMEOFDAY
//   feature: detect if gettimeofday is available (POSIX)
#ifndef D_ENV_C_HAS_GETTIMEOFDAY
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
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
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_BSD_SOCKETS 1
    #else
        #define D_ENV_C_HAS_BSD_SOCKETS 0
    #endif
#endif

// D_ENV_C_HAS_GETADDRINFO
//   feature: detect if getaddrinfo is available (modern socket API)
#ifndef D_ENV_C_HAS_GETADDRINFO
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
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
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_FORK 1
    #else
        #define D_ENV_C_HAS_FORK 0
    #endif
#endif

// D_ENV_C_HAS_EXECVE
//   feature: detect if execve() is available (POSIX)
#ifndef D_ENV_C_HAS_EXECVE
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
        #define D_ENV_C_HAS_EXECVE 1
    #else
        #define D_ENV_C_HAS_EXECVE 0
    #endif
#endif

// D_ENV_C_HAS_GETPID
//   feature: detect if getpid() is available
#ifndef D_ENV_C_HAS_GETPID
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_C_HAS_GETPID 1
    #else
        #define D_ENV_C_HAS_GETPID 0
    #endif
#endif

// D_ENV_C_HAS_SIGNAL_H
//   feature: detect if signal.h is available
#ifndef D_ENV_C_HAS_SIGNAL_H
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
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
#endif

// D_ENV_C_HAS_POSIX_MEMALIGN
//   feature: detect if posix_memalign is available (POSIX)
#ifndef D_ENV_C_HAS_POSIX_MEMALIGN
    #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
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
    #if D_ENV_IS_OS_POSIX_LIKE_OR_WINDOWS(D_ENV_OS_ID)
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
//   feature: detect if getentropy (secure random) is available.
// note: intentionally narrower than D_ENV_IS_OS_POSIX_LIKE — getentropy
// is relatively recent (glibc 2.25 / OpenBSD 5.6) and not available on
// all generic Unix systems, so the Unix block (0x1) is omitted.
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

// =============================================================================
// VIII.  BUILD CONFIGURATION
// =============================================================================

// build configuration detection logic
#if D_CFG_ENV_BUILD_IS_ENABLED
    // automatic detection
    // NOTE: !defined(NDEBUG) means builds that define neither DEBUG nor
    // NDEBUG will be classified as Debug. If this is too aggressive for
    // your build system, consider requiring an affirmative debug signal.
    #if ( defined(DEBUG)  ||  \
          defined(_DEBUG) ||  \
          (!defined(NDEBUG)) )
        #define D_ENV_BUILD_DEBUG   1
        #define D_ENV_BUILD_TYPE    "Debug"
    #else
        #define D_ENV_BUILD_RELEASE 1
        #define D_ENV_BUILD_TYPE    "Release"
    #endif
#else
    // manual detection
    #ifdef D_ENV_DETECTED_BUILD_DEBUG
        #define D_ENV_BUILD_DEBUG   1
        #define D_ENV_BUILD_TYPE    "Debug"
    #elif defined(D_ENV_DETECTED_BUILD_RELEASE)
        #define D_ENV_BUILD_RELEASE 1
        #define D_ENV_BUILD_TYPE    "Release"
    #endif
#endif  // D_CFG_ENV_BUILD_IS_ENABLED

// =============================================================================
// IX.  DEBUG UTILITIES
// =============================================================================

#ifdef D_DEBUG_
    #include <stdio.h>
    
    void print_compiler_info(void);
    
#endif  // D_DEBUG_


#endif  // DJINTERP_ENVIRONMENT_
