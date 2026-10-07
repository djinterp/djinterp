/*******************************************************************************
* djinterp [c]                                                           dtime.h
*
* Cross-platform variants of certain `time.h` functions.
*   This header provides portable implementations of time-related functions
* that are not consistently available across all platforms. It includes
* thread-safe time conversion, high-resolution timing, sleep functions,
* timezone utilities, and string parsing/formatting.
*   Native implementations are used whenever available, with fallback
* implementations for platforms that lack them.
*
*
* path:      /inc/djinterp/c/dtime.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.21
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PLATFORM DETECTION
    ------------------
    1.  Platform selection
         1.  D_TIME_PLATFORM_WINDOWS / D_TIME_PLATFORM_POSIX
    2.  Feature detection
         1.  D_TIME_HAS_CLOCK_GETTIME
         2.  D_TIME_HAS_NANOSLEEP
         3.  D_TIME_HAS_TIMESPEC_GET
         4.  D_TIME_HAS_STRPTIME
         5.  D_TIME_HAS_TIMEGM
2.  TYPES AND CONSTANTS
    -------------------
    1.  Portable types
         1.  clockid_t
    2.  Clock identifiers
         1.  POSIX clock identifiers
         2.  TIME_UTC
    3.  Unit conversions
         1.  D_TIME_NSEC_PER_SEC
         2.  D_TIME_USEC_PER_SEC
         3.  D_TIME_MSEC_PER_SEC
         4.  D_TIME_NSEC_PER_MSEC
         5.  D_TIME_NSEC_PER_USEC
         6.  D_TIME_USEC_PER_MSEC
3.  THREAD-SAFE TIME CONVERSION
    ---------------------------
    1.  Conversion
4.  HIGH-RESOLUTION TIME
    --------------------
    1.  Clocks
5.  SLEEP
    -----
    1.  Sleep
6.  TIME ZONES
    ----------
    1.  Time zones
7.  PARSING AND FORMATTING
    ----------------------
    1.  Parsing and formatting
8.  TIMESPEC ARITHMETIC
    -------------------
    1.  Arithmetic and comparison
    2.  Unit conversion
    3.  Normalization and validation
9.  MONOTONIC TIME
    --------------
    1.  Monotonic clock
*/

#ifndef DJINTERP_C_DTIME_H
#define DJINTERP_C_DTIME_H 1

// std
#include <stddef.h>          // size_t
#include <time.h>            // time_t, struct tm, struct timespec, clockid_t
// djinterp
#include "./djinterp.h"      // framework root
#include "../env/env.h"      // D_ENV_PLATFORM_*, D_ENV_POSIX_*, D_ENV_LANG_*
// re_std
#include "../../re_std/cstdint/dstdint.h"  // int64_t


//==============================================================================
// 1.  PLATFORM DETECTION
//==============================================================================


// 1.1    Platform selection
//------------------------------------------------------------------------------
// 1.1.1
// D_TIME_PLATFORM_WINDOWS / D_TIME_PLATFORM_POSIX
//   constant: the implementation family dtime.c compiles, defined to 1 from the
// env layer's platform flags: Windows, or POSIX (Linux, macOS, Unix, Android).
// With neither defined, the portable fallbacks are used.
#if defined(D_ENV_PLATFORM_WINDOWS)
    #ifndef D_TIME_PLATFORM_WINDOWS
        #define D_TIME_PLATFORM_WINDOWS 1
    #endif  // D_TIME_PLATFORM_WINDOWS
#endif

#if ( defined(D_ENV_PLATFORM_LINUX) ||                                        \
      defined(D_ENV_PLATFORM_MACOS) ||                                        \
      defined(D_ENV_PLATFORM_UNIX)  ||                                        \
      defined(D_ENV_PLATFORM_ANDROID) )
    #ifndef D_TIME_PLATFORM_POSIX
        #define D_TIME_PLATFORM_POSIX 1
    #endif  // D_TIME_PLATFORM_POSIX
#endif

// 1.2    Feature detection
//------------------------------------------------------------------------------
// 1.2.1
// D_TIME_HAS_CLOCK_GETTIME
//   feature: detect if clock_gettime is available.
#ifndef D_TIME_HAS_CLOCK_GETTIME
    #if ( defined(D_TIME_PLATFORM_POSIX)                        &&            \
          ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1993) ||              \
            (D_ENV_POSIX_FEATURE_REALTIME) ) )
        #define D_TIME_HAS_CLOCK_GETTIME 1
    #else
        #define D_TIME_HAS_CLOCK_GETTIME 0
    #endif
#endif

// 1.2.2
// D_TIME_HAS_NANOSLEEP
//   feature: detect if nanosleep is available.
#ifndef D_TIME_HAS_NANOSLEEP
    #if ( defined(D_TIME_PLATFORM_POSIX)                        &&            \
          ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1993) ||              \
            (D_ENV_POSIX_FEATURE_REALTIME) ) )
        #define D_TIME_HAS_NANOSLEEP 1
    #else
        #define D_TIME_HAS_NANOSLEEP 0
    #endif
#endif

// 1.2.3
// D_TIME_HAS_TIMESPEC_GET
//   feature: 1 where the C library has C11's timespec_get, which it says by
// defining TIME_UTC in <time.h> (C11 7.27.1): glibc and musl in C11 mode,
// Microsoft's UCRT in any. Read here, before 2.2.2 supplies TIME_UTC where it
// is missing. It replaces a C11-or-MSVC-2015 test (decision 27 of the
// register): MinGW over msvcrt passes the first and has no timespec_get, and
// the second read D_ENV_MSC_VER, which nothing defines, so MSVC never had it.
#ifndef D_TIME_HAS_TIMESPEC_GET
    #if defined(TIME_UTC)
        #define D_TIME_HAS_TIMESPEC_GET 1
    #else
        #define D_TIME_HAS_TIMESPEC_GET 0
    #endif
#endif

// 1.2.4
// D_TIME_HAS_STRPTIME
//   feature: detect strptime availability (POSIX but not Windows).
#ifndef D_TIME_HAS_STRPTIME
    #if defined(D_TIME_PLATFORM_POSIX)
        #define D_TIME_HAS_STRPTIME 1
    #else
        #define D_TIME_HAS_STRPTIME 0
    #endif
#endif

// 1.2.5
// D_TIME_HAS_TIMEGM
//   feature: 1 where <time.h> declares timegm, a BSD and GNU extension, under
// the build's feature-test macros (decision 25 of the register). glibc and
// musl declare it for _DEFAULT_SOURCE, _GNU_SOURCE or _BSD_SOURCE, which they
// define themselves where the build asks for no standard, but not for
// _XOPEN_SOURCE alone; macOS and the BSDs declare it unless the build asks
// for a standard with _POSIX_C_SOURCE or _XOPEN_SOURCE. Elsewhere d_timegm
// has a portable fallback, and on Windows _mkgmtime. The old test answered 0
// on Linux and macOS, and 1 on FreeBSD even where timegm was hidden.
#ifndef D_TIME_HAS_TIMEGM
    #if ( (defined(D_TIME_PLATFORM_POSIX))   &&                                \
          ( (defined(_DEFAULT_SOURCE))     ||                                  \
            (defined(_GNU_SOURCE))         ||                                  \
            (defined(_BSD_SOURCE))         ||                                  \
            ( ( (defined(D_ENV_PLATFORM_MACOS)) ||                             \
                (defined(D_ENV_PLATFORM_UNIX)) )  &&                           \
              (!defined(_POSIX_C_SOURCE))         &&                           \
              (!defined(_XOPEN_SOURCE)) ) ) )
        #define D_TIME_HAS_TIMEGM 1
    #else
        #define D_TIME_HAS_TIMEGM 0
    #endif
#endif


//==============================================================================
// 2.  TYPES AND CONSTANTS
//==============================================================================


// 2.1    Portable types
//------------------------------------------------------------------------------
// 2.1.1
// clockid_t
//   type: clock identifier type, defined here on Windows, which has none.
#if defined(D_TIME_PLATFORM_WINDOWS)
    #ifndef clockid_t
        typedef int clockid_t;
    #endif
#endif

// 2.2    Clock identifiers
//------------------------------------------------------------------------------
// 2.2.1
// POSIX clock identifiers
//   constant: CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, and
// CLOCK_THREAD_CPUTIME_ID, defined only where the platform has none, for
// d_clock_gettime() and d_clock_getres().
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

// 2.2.2
// TIME_UTC
//   constant: C11's calendar-time base for d_timespec_get(), defined only where
// <time.h> lacks it.
#ifndef TIME_UTC
    #define TIME_UTC 1
#endif

// 2.3    Unit conversions
//------------------------------------------------------------------------------
// 2.3.1
// D_TIME_NSEC_PER_SEC
//   constant: nanoseconds per second.
#define D_TIME_NSEC_PER_SEC  1000000000L

// 2.3.2
// D_TIME_USEC_PER_SEC
//   constant: microseconds per second.
#define D_TIME_USEC_PER_SEC  1000000L

// 2.3.3
// D_TIME_MSEC_PER_SEC
//   constant: milliseconds per second.
#define D_TIME_MSEC_PER_SEC  1000L

// 2.3.4
// D_TIME_NSEC_PER_MSEC
//   constant: nanoseconds per millisecond.
#define D_TIME_NSEC_PER_MSEC 1000000L

// 2.3.5
// D_TIME_NSEC_PER_USEC
//   constant: nanoseconds per microsecond.
#define D_TIME_NSEC_PER_USEC 1000L

// 2.3.6
// D_TIME_USEC_PER_MSEC
//   constant: microseconds per millisecond.
#define D_TIME_USEC_PER_MSEC 1000L


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN


//==============================================================================
// 3.  THREAD-SAFE TIME CONVERSION
//==============================================================================


// 3.1    Conversion
//------------------------------------------------------------------------------
/**
 * @brief Converts a time to broken-down local time, thread-safely (POSIX
 *        localtime_r / C11 localtime_s equivalent).
 *
 * @note the fallback for platforms with neither is not thread-safe.
 *
 * @param[in]  _timer   the time to convert.
 * @param[out] _result  receives the broken-down local time.
 * @return `_result` on success, or `NULL` if either argument is `NULL` or the
 *         conversion fails.
 */
struct tm*       d_localtime(const time_t* _timer,
                             struct tm*    _result);
/**
 * @brief Converts a time to broken-down UTC, thread-safely (POSIX gmtime_r /
 *        C11 gmtime_s equivalent).
 *
 * @note the fallback for platforms with neither is not thread-safe.
 *
 * @param[in]  _timer   the time to convert.
 * @param[out] _result  receives the broken-down UTC.
 * @return `_result` on success, or `NULL` if either argument is `NULL` or the
 *         conversion fails.
 */
struct tm*       d_gmtime(const time_t* _timer,
                          struct tm*    _result);
/**
 * @brief Formats a time in the ctime layout, thread-safely (POSIX ctime_r / C11
 *        ctime_s equivalent).
 *
 * @note the fallback for platforms with neither is not thread-safe.
 *
 * @param[in]  _timer  the time to convert, as local time.
 * @param[out] _buf    receives the string; at least 26 bytes.
 * @return `_buf` on success, or `NULL` if either argument is `NULL` or the
 *         conversion fails.
 */
char*            d_ctime(const time_t* _timer,
                         char*         _buf);
/**
 * @brief Formats a time in the ctime layout, thread-safely (POSIX asctime_r /
 *        C11 asctime_s equivalent).
 *
 * @note the fallback for platforms with neither is not thread-safe.
 *
 * @param[in]  _tm   the broken-down time to convert.
 * @param[out] _buf  receives the string; at least 26 bytes.
 * @return `_buf` on success, or `NULL` if either argument is `NULL` or the
 *         conversion fails.
 */
char*            d_asctime(const struct tm* _tm,
                           char*            _buf);


//==============================================================================
// 4.  HIGH-RESOLUTION TIME
//==============================================================================


// 4.1    Clocks
//------------------------------------------------------------------------------
/**
 * @brief Reads a clock (POSIX clock_gettime equivalent).
 *
 * @note the portable fallback supports CLOCK_REALTIME only, at one-second
 *       resolution.
 *
 * @param[in]  _clock_id  the clock: CLOCK_REALTIME, CLOCK_MONOTONIC,
 *                        CLOCK_PROCESS_CPUTIME_ID, or CLOCK_THREAD_CPUTIME_ID.
 * @param[out] _tp        receives the time.
 * @return `0` on success, or `-1` with errno set: EINVAL for a `NULL` `_tp` or
 *         an unsupported clock.
 */
int              d_clock_gettime(clockid_t        _clock_id,
                                 struct timespec* _tp);
/**
 * @brief Reads a clock's resolution (POSIX clock_getres equivalent).
 *
 * @param[in]  _clock_id  the clock, as for d_clock_gettime().
 * @param[out] _res       receives the resolution.
 * @return `0` on success, or `-1` with errno set: EINVAL for a `NULL` `_res` or
 *         an unsupported clock.
 */
int              d_clock_getres(clockid_t        _clock_id,
                                struct timespec* _res);
/**
 * @brief Reads the current calendar time (C11 timespec_get equivalent).
 *
 * @param[out] _ts    receives the time.
 * @param[in]  _base  the time base; TIME_UTC is the only one required.
 * @return `_base` on success, or `0` on failure, including a `NULL` `_ts`.
 */
int              d_timespec_get(struct timespec* _ts,
                                int              _base);


//==============================================================================
// 5.  SLEEP
//==============================================================================


// 5.1    Sleep
//------------------------------------------------------------------------------
/**
 * @brief Sleeps for a high-resolution interval (POSIX nanosleep equivalent).
 *
 * @note Windows sleeps at millisecond resolution, raising a nonzero sub-
 *       millisecond request to 1 ms. Where neither POSIX nanosleep nor
 *       Windows is available, it uses C11's thrd_sleep where threads exist;
 *       else POSIX sleep(), whole seconds rounded up, on a POSIX platform;
 *       else it fails with ENOSYS.
 *
 * @param[in]  _req  the interval; tv_nsec must be in [0, 999999999].
 * @param[out] _rem  receives the unslept time if the sleep is cut short; may be
 *                   `NULL`.
 * @return `0` on success, or `-1` with errno set: EINVAL for a `NULL` or out-
 *         of-range `_req`; EINTR when interrupted (nanosleep, thrd_sleep);
 *         ENOSYS where the platform cannot sleep.
 */
int              d_nanosleep(const struct timespec* _req,
                             struct timespec*       _rem);
/**
 * @brief Sleeps for a number of microseconds (POSIX usleep equivalent).
 *
 * @param[in] _usec  the interval in microseconds.
 * @return d_nanosleep()'s result.
 */
int              d_usleep(unsigned int _usec);
/**
 * @brief Sleeps for a number of milliseconds.
 *
 * @param[in] _milliseconds  the interval in milliseconds.
 * @return d_nanosleep()'s result.
 */
int              d_sleep_ms(unsigned long _milliseconds);


//==============================================================================
// 6.  TIME ZONES
//==============================================================================


// 6.1    Time zones
//------------------------------------------------------------------------------
/**
 * @brief Converts a broken-down UTC time to a time_t, the inverse of gmtime
 *        (timegm equivalent).
 *
 * @param[in,out] _tm  the UTC time, its fields normalized on return, as timegm
 *                     does: the portable fallback normalizes them too.
 * @return the time, or `(time_t)-1` on failure, including a `NULL` `_tm`.
 */
time_t           d_timegm(struct tm* _tm);
/**
 * @brief Initializes time-zone information from the environment (POSIX tzset /
 *        Windows _tzset equivalent).
 */
void             d_tzset(void);


//==============================================================================
// 7.  PARSING AND FORMATTING
//==============================================================================


// 7.1    Parsing and formatting
//------------------------------------------------------------------------------
/**
 * @brief Parses a time string according to a format (POSIX strptime
 *        equivalent).
 *
 * @note where the platform has no strptime (e.g. Windows) a fallback parser
 *       handles %Y %y %m %b %h %B %a %A %d %e %H %k %I %l %M %S %j %w %p %P %n
 *       %t and %%. It zeroes `_tm` first, skips an unsupported conversion or an
 *       unmatched name rather than failing, and stops without failing when the
 *       input runs out.
 *
 * @param[in]  _s       the string to parse.
 * @param[in]  _format  the format, with strftime-style conversions.
 * @param[out] _tm      receives the parsed fields.
 * @return a pointer to the first character of `_s` not parsed, or `NULL` on a
 *         mismatch or a `NULL` argument.
 */
char*            d_strptime(const char* _s,
                            const char* _format,
                            struct tm*  _tm);
/**
 * @brief Formats a time with strftime, rejecting invalid arguments.
 *
 * @param[out] _s        receives the formatted string.
 * @param[in]  _maxsize  the size of `_s` in bytes.
 * @param[in]  _format   the strftime format.
 * @param[in]  _tm       the time to format.
 * @return the number of characters written, excluding the terminator; `0` if an
 *         argument is invalid (then `_s` is emptied whenever it has room for a
 *         terminator) or the result does not fit (then its contents are
 *         unspecified).
 */
int              d_strftime_s(char*            _s,
                              size_t           _maxsize,
                              const char*      _format,
                              const struct tm* _tm);


//==============================================================================
// 8.  TIMESPEC ARITHMETIC
//==============================================================================


// 8.1    Arithmetic and comparison
//------------------------------------------------------------------------------
/**
 * @brief Adds two timespec values.
 *
 * @param[in]  _a       the first operand.
 * @param[in]  _b       the second operand.
 * @param[out] _result  receives the sum; may alias an operand.
 * @pre    both operands are normalized: a single carry or borrow repairs only
 *         tv_nsec values in [0, 999999999].
 * @post   nothing is written if any argument is `NULL`.
 */
void             d_timespec_add(const struct timespec* _a,
                                const struct timespec* _b,
                                struct timespec*       _result);
/**
 * @brief Subtracts two timespec values (`_a` - `_b`).
 *
 * @param[in]  _a       the first operand.
 * @param[in]  _b       the second operand.
 * @param[out] _result  receives the difference; may alias an operand.
 * @pre    both operands are normalized: a single carry or borrow repairs only
 *         tv_nsec values in [0, 999999999].
 * @post   nothing is written if any argument is `NULL`.
 */
void             d_timespec_sub(const struct timespec* _a,
                                const struct timespec* _b,
                                struct timespec*       _result);
/**
 * @brief Compares two timespec values.
 *
 * @param[in] _a  the first value.
 * @param[in] _b  the second value.
 * @return a value less than, equal to, or greater than zero as `_a` is earlier
 *         than, equal to, or later than `_b`. `NULL` orders before every
 *         value: `0` if both are `NULL`, `-1` if `_a` alone is, `1` if `_b`
 *         alone is.
 */
int              d_timespec_cmp(const struct timespec* _a,
                                const struct timespec* _b);

// 8.2    Unit conversion
//------------------------------------------------------------------------------
/**
 * @brief Converts a timespec to milliseconds.
 *
 * @param[in] _ts  the value to convert.
 * @return the total in milliseconds, truncated toward zero; `0` for a `NULL`
 *         `_ts`.
 */
int64_t          d_timespec_to_ms(const struct timespec* _ts);
/**
 * @brief Converts a timespec to microseconds.
 *
 * @param[in] _ts  the value to convert.
 * @return the total in microseconds, truncated toward zero; `0` for a `NULL`
 *         `_ts`.
 */
int64_t          d_timespec_to_us(const struct timespec* _ts);
/**
 * @brief Converts a timespec to nanoseconds.
 *
 * @param[in] _ts  the value to convert.
 * @pre    |tv_sec| is below about 9.2e9 (292 years); beyond that the nanosecond
 *         total overflows int64_t, which is undefined. Debug builds assert
 *         it; with NDEBUG nothing checks.
 * @return the total in nanoseconds; `0` for a `NULL` `_ts`.
 */
int64_t          d_timespec_to_ns(const struct timespec* _ts);
/**
 * @brief Converts milliseconds to a timespec.
 *
 * @note a negative input yields a negative tv_nsec; normalize with
 *       d_timespec_normalize() where that matters.
 *
 * @param[in]  _milliseconds  the value in milliseconds.
 * @param[out] _ts            receives the timespec; nothing is written if it is
 *                            `NULL`.
 */
void             d_ms_to_timespec(int64_t          _milliseconds,
                                  struct timespec* _ts);
/**
 * @brief Converts microseconds to a timespec.
 *
 * @note a negative input yields a negative tv_nsec; normalize with
 *       d_timespec_normalize() where that matters.
 *
 * @param[in]  _microseconds  the value in microseconds.
 * @param[out] _ts            receives the timespec; nothing is written if it is
 *                            `NULL`.
 */
void             d_us_to_timespec(int64_t          _microseconds,
                                  struct timespec* _ts);
/**
 * @brief Converts nanoseconds to a timespec.
 *
 * @note a negative input yields a negative tv_nsec; normalize with
 *       d_timespec_normalize() where that matters.
 *
 * @param[in]  _nanoseconds  the value in nanoseconds.
 * @param[out] _ts           receives the timespec; nothing is written if it is
 *                           `NULL`.
 */
void             d_ns_to_timespec(int64_t          _nanoseconds,
                                  struct timespec* _ts);

// 8.3    Normalization and validation
//------------------------------------------------------------------------------
/**
 * @brief Normalizes a timespec so tv_nsec lies in [0, 999999999], preserving
 *        its total.
 *
 * @param[in,out] _ts  the value to normalize; may be `NULL`, which does
 *                     nothing.
 */
void             d_timespec_normalize(struct timespec* _ts);
/**
 * @brief Tests whether a timespec's tv_nsec lies in [0, 999999999].
 *
 * @note tv_sec is not checked; any value, negative included, passes.
 *
 * @param[in] _ts  the value to check.
 * @return `1` if valid, `0` otherwise or for a `NULL` `_ts`.
 */
int              d_timespec_is_valid(const struct timespec* _ts);


//==============================================================================
// 9.  MONOTONIC TIME
//==============================================================================


// 9.1    Monotonic clock
//------------------------------------------------------------------------------
/**
 * @brief Reads monotonic time in milliseconds.
 *
 * @note falls back to CLOCK_REALTIME, which is not monotonic, when
 *       CLOCK_MONOTONIC cannot be read.
 *
 * @return the time in milliseconds, or `0` if no clock can be read.
 */
int64_t          d_monotonic_time_ms(void);
/**
 * @brief Reads monotonic time in microseconds.
 *
 * @note falls back to CLOCK_REALTIME, which is not monotonic, when
 *       CLOCK_MONOTONIC cannot be read.
 *
 * @return the time in microseconds, or `0` if no clock can be read.
 */
int64_t          d_monotonic_time_us(void);
/**
 * @brief Reads monotonic time in nanoseconds.
 *
 * @note falls back to CLOCK_REALTIME, which is not monotonic, when
 *       CLOCK_MONOTONIC cannot be read.
 *
 * @return the time in nanoseconds, or `0` if no clock can be read.
 */
int64_t          d_monotonic_time_ns(void);


D_EXTERN_C_END


#endif  // DJINTERP_C_DTIME_H
