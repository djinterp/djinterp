/******************************************************************************
* djinterp [core]                                                      dtime.h
*
* Cross-platform variants of certain `time.h` functions.
*   This header provides portable implementations of time-related functions
* that are not consistently available across all platforms. It includes
* thread-safe time conversion, high-resolution timing, sleep functions,
* timezone utilities, and string parsing/formatting.
*   Native implementations are used whenever available, with fallback
* implementations for platforms that lack them.
*
* path:      \inc\dtime.h                                  
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PLATFORM DETECTION AND INCLUDES
      --------------------------------
      1.  Standard headers
      2.  Platform-specific headers
      3.  Feature detection macros

II.   TYPE DEFINITIONS
      -----------------
      1.  struct timespec (for platforms lacking it)
      2.  clockid_t (for platforms lacking it)
      3.  Clock ID constants

III.  THREAD-SAFE TIME CONVERSION
      ---------------------------
      1.  d_localtime    (POSIX localtime_r / C11 localtime_s equivalent)
      2.  d_gmtime       (POSIX gmtime_r / C11 gmtime_s equivalent)
      3.  d_ctime        (POSIX ctime_r / C11 ctime_s equivalent)
      4.  d_asctime      (POSIX asctime_r / C11 asctime_s equivalent)

IV.   HIGH-RESOLUTION TIME
      --------------------
      1.  d_clock_gettime  (POSIX clock_gettime equivalent)
      2.  d_clock_getres   (POSIX clock_getres equivalent)
      3.  d_timespec_get   (C11 timespec_get equivalent)

V.    SLEEP FUNCTIONS
      ----------------
      1.  d_nanosleep      (POSIX nanosleep equivalent)
      2.  d_usleep         (POSIX usleep equivalent)
      3.  d_sleep_ms       (convenience: sleep in milliseconds)

VI.   TIMEZONE UTILITIES
      -------------------
      1.  d_timegm         (inverse of gmtime - non-standard but common)
      2.  d_tzset          (POSIX tzset / Windows _tzset equivalent)

VII.  STRING PARSING AND FORMATTING
      -----------------------------
      1.  d_strptime       (POSIX strptime equivalent)
      2.  d_strftime_s     (C11 Annex K strftime_s equivalent)

VIII. TIME ARITHMETIC
      ----------------
      1.  d_timespec_add   (add two timespec values)
      2.  d_timespec_sub   (subtract two timespec values)
      3.  d_timespec_cmp   (compare two timespec values)
      4.  d_timespec_to_ms (convert timespec to milliseconds)
      5.  d_ms_to_timespec (convert milliseconds to timespec)

IX.   MONOTONIC TIME UTILITIES
      ------------------------
      1.  d_monotonic_time_ms  (get monotonic time in milliseconds)
      2.  d_monotonic_time_us  (get monotonic time in microseconds)
      3.  d_monotonic_time_ns  (get monotonic time in nanoseconds)
*/

#ifndef DJINTERP_TIME_
#define DJINTERP_TIME_ 1


///////////////////////////////////////////////////////////////////////////////
///             I.    PLATFORM DETECTION AND INCLUDES                       ///
///////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include ".\djinterp.h"
#include ".\dmemory.h"


// platform-specific includes
#if defined(D_ENV_PLATFORM_WINDOWS)
    #ifndef D_TIME_PLATFORM_WINDOWS
        #define D_TIME_PLATFORM_WINDOWS 1
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#if defined(D_ENV_PLATFORM_LINUX) ||  \
    defined(D_ENV_PLATFORM_MACOS) ||  \
    defined(D_ENV_PLATFORM_UNIX)  ||  \
    defined(D_ENV_PLATFORM_ANDROID)
    #ifndef D_TIME_PLATFORM_POSIX
        #define D_TIME_PLATFORM_POSIX 1
    #endif
    #include <unistd.h>
    #include <sys/time.h>
#endif


// D_TIME_HAS_CLOCK_GETTIME
//   feature: detect if clock_gettime is available.
#ifndef D_TIME_HAS_CLOCK_GETTIME
    #if ( defined(D_TIME_PLATFORM_POSIX) &&                       \
          ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1993) ||  \
          D_ENV_POSIX_FEATURE_REALTIME) )
        #define D_TIME_HAS_CLOCK_GETTIME 1
    #else
        #define D_TIME_HAS_CLOCK_GETTIME 0
    #endif
#endif

// D_TIME_HAS_NANOSLEEP
//   feature: detect if nanosleep is available.
#ifndef D_TIME_HAS_NANOSLEEP
    #if ( defined(D_TIME_PLATFORM_POSIX) &&                     \
        ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1993) ||  \
          (D_ENV_POSIX_FEATURE_REALTIME) ) )
        #define D_TIME_HAS_NANOSLEEP 1
    #else
        #define D_TIME_HAS_NANOSLEEP 0
    #endif
#endif

// D_TIME_HAS_TIMESPEC_GET
//   feature: detect C11 timespec_get availability.
#ifndef D_TIME_HAS_TIMESPEC_GET
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        #define D_TIME_HAS_TIMESPEC_GET 1
    #elif D_ENV_CRT_MSVC && (D_ENV_MSC_VER >= 1900)
        #define D_TIME_HAS_TIMESPEC_GET 1
    #else
        #define D_TIME_HAS_TIMESPEC_GET 0
    #endif
#endif

// D_TIME_HAS_STRPTIME
//   feature: detect strptime availability (POSIX but not Windows).
#ifndef D_TIME_HAS_STRPTIME
    #if defined(D_TIME_PLATFORM_POSIX)
        #define D_TIME_HAS_STRPTIME 1
    #else
        #define D_TIME_HAS_STRPTIME 0
    #endif
#endif

// D_TIME_HAS_TIMEGM
//   feature: detect timegm availability (GNU/BSD extension).
#ifndef D_TIME_HAS_TIMEGM
    #if ( ( defined(D_ENV_PLATFORM_LINUX) ||  \
            defined(D_ENV_PLATFORM_MACOS) ||  \
            defined(D_ENV_OS_ID) ) &&          \
            D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4) )
        #define D_TIME_HAS_TIMEGM 1
    #else
        #define D_TIME_HAS_TIMEGM 0
    #endif
#endif

///////////////////////////////////////////////////////////////////////////////
///             II.   TYPE DEFINITIONS                                      ///
///////////////////////////////////////////////////////////////////////////////

// struct timespec
//   type: high-resolution time structure.
// Define struct timespec if not already defined (Windows prior to VS2015).
#if defined(D_TIME_PLATFORM_WINDOWS)
    #if !defined(_TIMESPEC_DEFINED) && !defined(HAVE_STRUCT_TIMESPEC)
        #ifndef _CRT_NO_TIME_T
            #if (_MSC_VER < 1900)
                #define _TIMESPEC_DEFINED 1
                #define HAVE_STRUCT_TIMESPEC 1
                struct timespec
                {
                    time_t tv_sec;      // seconds
                    long   tv_nsec;     // nanoseconds
                };
            #endif  // _MSC_VER < 1900
        #endif  // _CRT_NO_TIME_T
    #endif  // !_TIMESPEC_DEFINED
#endif  // D_TIME_PLATFORM_WINDOWS

// clockid_t
//   type: clock identifier type.
#if defined(D_TIME_PLATFORM_WINDOWS)
    #ifndef clockid_t
        typedef int clockid_t;
    #endif
#endif

// clock ID constants
#ifndef CLOCK_REALTIME
    #define CLOCK_REALTIME           0
#endif

#ifndef CLOCK_MONOTONIC
    #define CLOCK_MONOTONIC          1
#endif

#ifndef CLOCK_PROCESS_CPUTIME_ID
    #define CLOCK_PROCESS_CPUTIME_ID 2
#endif

#ifndef CLOCK_THREAD_CPUTIME_ID
    #define CLOCK_THREAD_CPUTIME_ID  3
#endif

// C11 TIME_UTC constant
#ifndef TIME_UTC
    #define TIME_UTC 1
#endif

// D_TIME_NSEC_PER_SEC
//   constant: nanoseconds per second.
#define D_TIME_NSEC_PER_SEC  1000000000L

// D_TIME_USEC_PER_SEC
//   constant: microseconds per second.
#define D_TIME_USEC_PER_SEC  1000000L

// D_TIME_MSEC_PER_SEC
//   constant: milliseconds per second.
#define D_TIME_MSEC_PER_SEC  1000L

// D_TIME_NSEC_PER_MSEC
//   constant: nanoseconds per millisecond.
#define D_TIME_NSEC_PER_MSEC 1000000L

// D_TIME_NSEC_PER_USEC
//   constant: nanoseconds per microsecond.
#define D_TIME_NSEC_PER_USEC 1000L

// D_TIME_USEC_PER_MSEC
//   constant: microseconds per millisecond.
#define D_TIME_USEC_PER_MSEC 1000L


///////////////////////////////////////////////////////////////////////////////
///             III.  THREAD-SAFE TIME CONVERSION                           ///
///////////////////////////////////////////////////////////////////////////////

struct tm* d_localtime(const time_t* _timer, struct tm* _result);
struct tm* d_gmtime(const time_t* _timer, struct tm* _result);
char*      d_ctime(const time_t* _timer, char* _buf);
char*      d_asctime(const struct tm* _tm, char* _buf);

// IV.   High-resolution time
int        d_clock_gettime(clockid_t _clock_id, struct timespec* _tp);
int        d_clock_getres(clockid_t _clock_id, struct timespec* _res);
int        d_timespec_get(struct timespec* _ts, int _base);


///////////////////////////////////////////////////////////////////////////////
///             V.    SLEEP FUNCTIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

// d_nanosleep
//   function: high-resolution sleep.
// Uses nanosleep on POSIX, Sleep on Windows.
int        d_nanosleep(const struct timespec* _req, struct timespec* _rem);

// d_usleep
//   function: microsecond sleep.
// Uses usleep on POSIX (if available), otherwise falls back to nanosleep
// or Windows Sleep.
int        d_usleep(unsigned int _usec);

// d_sleep_ms
//   function: millisecond sleep (convenience function).
// Portable sleep function with millisecond granularity.
int        d_sleep_ms(unsigned long _milliseconds);


///////////////////////////////////////////////////////////////////////////////
///             VI.   TIMEZONE UTILITIES                                    ///
///////////////////////////////////////////////////////////////////////////////

// d_timegm
//   function: inverse of gmtime - convert struct tm (UTC) to time_t.
// Uses timegm on systems that have it, _mkgmtime on Windows, or a portable
// fallback implementation.
time_t     d_timegm(struct tm* _tm);

// d_tzset
//   function: initialize timezone information.
// Uses tzset on POSIX, _tzset on Windows.
void       d_tzset(void);


///////////////////////////////////////////////////////////////////////////////
///             VII.  STRING PARSING AND FORMATTING                         ///
///////////////////////////////////////////////////////////////////////////////

// d_strptime
//   function: parse time string according to format.
// Uses native strptime on POSIX, provides minimal implementation on Windows.
char*      d_strptime(const char* _s, const char* _format, struct tm* _tm);

// d_strftime_s
//   function: safe string formatting of time.
// Uses strftime_s on Windows, strftime with bounds checking elsewhere.
int        d_strftime_s(char* _s, size_t _maxsize, const char* _format, 
                        const struct tm* _tm);


///////////////////////////////////////////////////////////////////////////////
///             VIII. TIME ARITHMETIC                                       ///
///////////////////////////////////////////////////////////////////////////////

// d_timespec_add
//   function: add two timespec values.
// Result is stored in _result. Handles nanosecond overflow correctly.
void       d_timespec_add(const struct timespec* _a, 
                          const struct timespec* _b,
                          struct timespec*       _result);

// d_timespec_sub
//   function: subtract _b from _a.
// Result is stored in _result. Handles nanosecond underflow correctly.
void       d_timespec_sub(const struct timespec* _a, 
                          const struct timespec* _b,
                          struct timespec*       _result);

// d_timespec_cmp
//   function: compare two timespec values.
// Returns negative if _a < _b, zero if _a == _b, positive if _a > _b.
int        d_timespec_cmp(const struct timespec* _a, 
                          const struct timespec* _b);

// d_timespec_to_ms
//   function: convert timespec to milliseconds.
// Returns the total time in milliseconds.
int64_t    d_timespec_to_ms(const struct timespec* _ts);

// d_timespec_to_us
//   function: convert timespec to microseconds.
// Returns the total time in microseconds.
int64_t    d_timespec_to_us(const struct timespec* _ts);

// d_timespec_to_ns
//   function: convert timespec to nanoseconds.
// Returns the total time in nanoseconds.
int64_t    d_timespec_to_ns(const struct timespec* _ts);

// d_ms_to_timespec
//   function: convert milliseconds to timespec.
// Stores the result in _ts.
void       d_ms_to_timespec(int64_t _milliseconds, struct timespec* _ts);

// d_us_to_timespec
//   function: convert microseconds to timespec.
// Stores the result in _ts.
void       d_us_to_timespec(int64_t _microseconds, struct timespec* _ts);

// d_ns_to_timespec
//   function: convert nanoseconds to timespec.
// Stores the result in _ts.
void       d_ns_to_timespec(int64_t _nanoseconds, struct timespec* _ts);


// IX.   monotonic time utilites
int64_t    d_monotonic_time_ms(void);
int64_t    d_monotonic_time_us(void);
int64_t    d_monotonic_time_ns(void);


///////////////////////////////////////////////////////////////////////////////
///             X.    TIMESPEC NORMALIZATION                                ///
///////////////////////////////////////////////////////////////////////////////

// d_timespec_normalize
//   function: normalize a timespec so that tv_nsec is in [0, 999999999].
// Modifies _ts in place.
void       d_timespec_normalize(struct timespec* _ts);

// d_timespec_is_valid
//   function: check if a timespec has valid values.
// Returns non-zero if valid, zero if invalid.
int        d_timespec_is_valid(const struct timespec* _ts);


#endif	// DJINTERP_TIME_
