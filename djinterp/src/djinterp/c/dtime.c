/*******************************************************************************
* djinterp [c]                                                           dtime.c
*
* Definitions for the declarations in `dtime.h`.
*   Each platform-dependent function is defined once per platform -- Windows,
* POSIX, and a portable fallback -- with the implementation selected at the
* function level rather than by conditionals inside one body.
*   Build it with -D_XOPEN_SOURCE=700 where the C library hides POSIX in a
* strict ISO mode (the C guide's rule). timegm, where D_TIME_HAS_TIMEGM
* selects it, needs _DEFAULT_SOURCE as well on glibc.
*
*
* path:      /src/djinterp/c/dtime.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.21
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "../../../inc/djinterp/c/dtime.h"      // corresponding header
// std
#include <assert.h>                               // assert
#include <errno.h>                                // errno, EINVAL
#include <stddef.h>                               // size_t, NULL
#include <string.h>                               // strlen
#include <time.h>                                 // localtime_r, strftime, ...
// djinterp
#include "../../../inc/djinterp/c/memory/dmemory.h"  // d_memcpy, d_memset
#include "../../../inc/djinterp/c/string_fn.h"       // d_strncasecmp
// re_std
#include "../../../inc/re_std/cstdint/dstdint.h"  // int64_t
// platform
#if defined(D_TIME_PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif  // WIN32_LEAN_AND_MEAN
    #include <windows.h>  // QueryPerformanceCounter, Sleep, FILETIME, ...
#endif

// D_INTERNAL_TIME_SLEEP_FALLBACK
//   macro: the d_nanosleep compiled where neither POSIX nanosleep nor Windows
// is available (decision 15 of the register): 1, C11's thrd_sleep, where the
// implementation has threads; 2, POSIX sleep(), whole seconds, on a POSIX
// platform without nanosleep; 3, none, failing with ENOSYS. 0 where the
// fallback is not compiled. sleep() used to be the fallback everywhere,
// including where POSIX is missing.
#if ( ( (D_TIME_HAS_NANOSLEEP) &&                                              \
        (defined(D_TIME_PLATFORM_POSIX)) ) ||                                  \
      (defined(D_TIME_PLATFORM_WINDOWS)) )
    #define D_INTERNAL_TIME_SLEEP_FALLBACK 0
#elif ( (D_ENV_LANG_IS_C11_OR_HIGHER) &&                                       \
        (!defined(__STDC_NO_THREADS__)) )
    #define D_INTERNAL_TIME_SLEEP_FALLBACK 1
    #include <threads.h>  // thrd_sleep
#elif defined(D_TIME_PLATFORM_POSIX)
    #define D_INTERNAL_TIME_SLEEP_FALLBACK 2
    #include <unistd.h>   // sleep
#else
    #define D_INTERNAL_TIME_SLEEP_FALLBACK 3
#endif

// D_INTERNAL_TIME_ENOSYS
//   constant: ENOSYS, POSIX's "not supported", where <errno.h> has it, and
// EINVAL elsewhere; C itself names neither.
#if defined(ENOSYS)
    #define D_INTERNAL_TIME_ENOSYS ENOSYS
#else
    #define D_INTERNAL_TIME_ENOSYS EINVAL
#endif


// thread-safe time conversion
#if defined(D_TIME_PLATFORM_WINDOWS)

/*
d_localtime
  Windows: localtime_s, whose parameters are the reverse of localtime_r's and
which reports failure as a nonzero errno_t rather than a NULL result.
*/
struct tm*
d_localtime(
    const time_t* _timer,
    struct tm*    _result
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_result) )
    {
        return NULL;
    }

    // Windows uses localtime_s with reversed parameter order
    if (localtime_s(_result,
                    _timer) != 0)
    {
        return NULL;
    }

    return _result;
}

#elif defined(D_TIME_PLATFORM_POSIX)

/*
d_localtime
  POSIX: delegates to localtime_r.
*/
struct tm*
d_localtime(
    const time_t* _timer,
    struct tm*    _result
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_result) )
    {
        return NULL;
    }

    // POSIX localtime_r
    return localtime_r(_timer,
                       _result);
}

#else

/*
d_localtime
  Fallback: copies out of localtime's shared static result. Not thread-safe:
another thread's localtime or gmtime can overwrite that buffer between the
call and the copy.
*/
struct tm*
d_localtime(
    const time_t* _timer,
    struct tm*    _result
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_result) )
    {
        return NULL;
    }

    // fallback: use non-thread-safe localtime and copy
    struct tm* temp = localtime(_timer);
    if (!temp)
    {
        return NULL;
    }

    d_memcpy(_result,
             temp,
             sizeof(struct tm));

    return _result;
}

#endif

#if defined(D_TIME_PLATFORM_WINDOWS)

/*
d_gmtime
  Windows: gmtime_s, whose parameters are the reverse of gmtime_r's and which
reports failure as a nonzero errno_t rather than a NULL result.
*/
struct tm*
d_gmtime(
    const time_t* _timer,
    struct tm*    _result
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_result) )
    {
        return NULL;
    }

    // Windows uses gmtime_s with reversed parameter order
    if (gmtime_s(_result,
                 _timer) != 0)
    {
        return NULL;
    }

    return _result;
}

#elif defined(D_TIME_PLATFORM_POSIX)

/*
d_gmtime
  POSIX: delegates to gmtime_r.
*/
struct tm*
d_gmtime(
    const time_t* _timer,
    struct tm*    _result
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_result) )
    {
        return NULL;
    }

    // POSIX gmtime_r
    return gmtime_r(_timer,
                    _result);
}

#else

/*
d_gmtime
  Fallback: copies out of gmtime's shared static result. Not thread-safe:
another thread's gmtime or localtime can overwrite that buffer between the
call and the copy.
*/
struct tm*
d_gmtime(
    const time_t* _timer,
    struct tm*    _result
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_result) )
    {
        return NULL;
    }

    // fallback: use non-thread-safe gmtime and copy
    struct tm* temp = gmtime(_timer);
    if (!temp)
    {
        return NULL;
    }

    d_memcpy(_result,
             temp,
             sizeof(struct tm));

    return _result;
}

#endif

#if defined(D_TIME_PLATFORM_WINDOWS)

/*
d_ctime
  Windows: ctime_s, given the fixed 26 bytes the ctime format needs.
*/
char*
d_ctime(
    const time_t* _timer,
    char*         _buf
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_buf) )
    {
        return NULL;
    }

    // Windows ctime_s requires buffer size (26 bytes for ctime format)
    if (ctime_s(_buf,
                26,
                _timer) != 0)
    {
        return NULL;
    }

    return _buf;
}

#elif defined(D_TIME_PLATFORM_POSIX)

/*
d_ctime
  POSIX: delegates to ctime_r.
*/
char*
d_ctime(
    const time_t* _timer,
    char*         _buf
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_buf) )
    {
        return NULL;
    }

    // POSIX ctime_r
    return ctime_r(_timer,
                   _buf);
}

#else

/*
d_ctime
  Fallback: copies 26 bytes out of ctime's shared static buffer. Not
thread-safe, and the fixed length holds only for four-digit years.
*/
char*
d_ctime(
    const time_t* _timer,
    char*         _buf
)
{
    // parameter validation
    if ( (!_timer) ||
         (!_buf) )
    {
        return NULL;
    }

    // fallback: use non-thread-safe ctime and copy
    const char* temp = ctime(_timer);
    if (!temp)
    {
        return NULL;
    }

    // ctime output is always 26 bytes including null terminator
    d_memcpy(_buf,
             temp,
             26);

    return _buf;
}

#endif

#if defined(D_TIME_PLATFORM_WINDOWS)

/*
d_asctime
  Windows: asctime_s, given the fixed 26 bytes the asctime format needs.
*/
char*
d_asctime(
    const struct tm* _tm,
    char*            _buf
)
{
    // parameter validation
    if ( (!_tm) ||
         (!_buf) )
    {
        return NULL;
    }

    // Windows asctime_s requires buffer size
    if (asctime_s(_buf,
                  26,
                  _tm) != 0)
    {
        return NULL;
    }

    return _buf;
}

#elif defined(D_TIME_PLATFORM_POSIX)

/*
d_asctime
  POSIX: delegates to asctime_r.
*/
char*
d_asctime(
    const struct tm* _tm,
    char*            _buf
)
{
    // parameter validation
    if ( (!_tm) ||
         (!_buf) )
    {
        return NULL;
    }

    // POSIX asctime_r
    return asctime_r(_tm,
                     _buf);
}

#else

/*
d_asctime
  Fallback: copies 26 bytes out of asctime's shared static buffer. Not
thread-safe, and the fixed length holds only for four-digit years.
*/
char*
d_asctime(
    const struct tm* _tm,
    char*            _buf
)
{
    // parameter validation
    if ( (!_tm) ||
         (!_buf) )
    {
        return NULL;
    }

    // fallback: use non-thread-safe asctime and copy
    const char* temp = asctime(_tm);
    if (!temp)
    {
        return NULL;
    }

    d_memcpy(_buf,
             temp,
             26);

    return _buf;
}

#endif

// high-resolution time
#if ( (D_TIME_HAS_CLOCK_GETTIME) &&                                           \
      defined(D_TIME_PLATFORM_POSIX) )

/*
d_clock_gettime
  POSIX: delegates to clock_gettime.
*/
int
d_clock_gettime(
    clockid_t        _clock_id,
    struct timespec* _tp
)
{
    // parameter validation
    if (!_tp)
    {
        errno = EINVAL;

        return -1;
    }

    // use native clock_gettime
    return clock_gettime(_clock_id,
                         _tp);
}

#elif defined(D_TIME_PLATFORM_WINDOWS)

/*
d_clock_gettime
  Windows: CLOCK_MONOTONIC scales QueryPerformanceCounter by its frequency;
CLOCK_REALTIME converts the system FILETIME (100 ns ticks since 1601) to the
Unix epoch, using the precise variant from Windows 8 on; the CPU-time clocks
add kernel and user time from GetProcessTimes or GetThreadTimes. Any other
clock fails with EINVAL.
*/
int
d_clock_gettime(
    clockid_t        _clock_id,
    struct timespec* _tp
)
{
    // parameter validation
    if (!_tp)
    {
        errno = EINVAL;

        return -1;
    }

    // Windows implementation using QueryPerformanceCounter
    // the counter's frequency is fixed at boot, so it is read once and kept
    // (decision 26 of the register: it was reset on every call)
    static LARGE_INTEGER frequency;
    static int           frequency_initialized;

    if (_clock_id == CLOCK_MONOTONIC)
    {
        // use QueryPerformanceCounter for monotonic time
        if (!frequency_initialized)
        {
            QueryPerformanceFrequency(&frequency);
            frequency_initialized = 1;
        }

        LARGE_INTEGER counter = {0};

        QueryPerformanceCounter(&counter);

        _tp->tv_sec  = (time_t)(counter.QuadPart / frequency.QuadPart);
        _tp->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) *
                               D_TIME_NSEC_PER_SEC) / frequency.QuadPart);

        return 0;
    }
    else if (_clock_id == CLOCK_REALTIME)
    {
        // use GetSystemTimePreciseAsFileTime for wall-clock time
        FILETIME ft = {0};

        #if (_WIN32_WINNT >= 0x0602)
            GetSystemTimePreciseAsFileTime(&ft);
        #else
            GetSystemTimeAsFileTime(&ft);
        #endif

        ULARGE_INTEGER uli = {0};

        uli.LowPart  = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;

        // convert from 100-nanosecond intervals since 1601 to Unix epoch
        // 11644473600 seconds between 1601 and 1970
        uli.QuadPart -= 116444736000000000ULL;

        _tp->tv_sec  = (time_t)(uli.QuadPart / 10000000ULL);
        _tp->tv_nsec = (long)((uli.QuadPart % 10000000ULL) * 100);

        return 0;
    }
    else if ( (_clock_id == CLOCK_PROCESS_CPUTIME_ID) ||
              (_clock_id == CLOCK_THREAD_CPUTIME_ID) )
    {
        FILETIME creation_time = {0};
        FILETIME exit_time     = {0};
        FILETIME kernel_time   = {0};
        FILETIME user_time     = {0};

        // get process or thread times
        const BOOL success = (_clock_id == CLOCK_PROCESS_CPUTIME_ID)
            ? GetProcessTimes(GetCurrentProcess(),
                              &creation_time,
                              &exit_time,
                              &kernel_time,
                              &user_time)
            : GetThreadTimes(GetCurrentThread(),
                             &creation_time,
                             &exit_time,
                             &kernel_time,
                             &user_time);

        if (!success)
        {
            errno = EINVAL;

            return -1;
        }

        // combine kernel and user time, as whole 64-bit counts: adding the
        // halves apart dropped the carry out of the low one (decision 26)
        ULARGE_INTEGER user   = {0};
        ULARGE_INTEGER kernel = {0};
        ULARGE_INTEGER uli    = {0};

        user.LowPart    = user_time.dwLowDateTime;
        user.HighPart   = user_time.dwHighDateTime;
        kernel.LowPart  = kernel_time.dwLowDateTime;
        kernel.HighPart = kernel_time.dwHighDateTime;
        uli.QuadPart    = user.QuadPart + kernel.QuadPart;

        _tp->tv_sec  = (time_t)(uli.QuadPart / 10000000ULL);
        _tp->tv_nsec = (long)((uli.QuadPart % 10000000ULL) * 100);

        return 0;
    }

    errno = EINVAL;

    return -1;
}

#else

/*
d_clock_gettime
  Fallback: only CLOCK_REALTIME, from time(), at one-second resolution; every
other clock fails with EINVAL.
*/
int
d_clock_gettime(
    clockid_t        _clock_id,
    struct timespec* _tp
)
{
    // parameter validation
    if (!_tp)
    {
        errno = EINVAL;

        return -1;
    }

    // minimal fallback using time()
    if (_clock_id == CLOCK_REALTIME)
    {
        _tp->tv_sec  = time(NULL);
        _tp->tv_nsec = 0;

        return 0;
    }

    errno = EINVAL;

    return -1;
}

#endif

#if ( (D_TIME_HAS_CLOCK_GETTIME) &&                                           \
      defined(D_TIME_PLATFORM_POSIX) )

/*
d_clock_getres
  POSIX: delegates to clock_getres.
*/
int
d_clock_getres(
    clockid_t        _clock_id,
    struct timespec* _res
)
{
    // parameter validation
    if (!_res)
    {
        errno = EINVAL;

        return -1;
    }

    // use native clock_getres
    return clock_getres(_clock_id,
                        _res);
}

#elif defined(D_TIME_PLATFORM_WINDOWS)

/*
d_clock_getres
  Windows: reports the tick of the source d_clock_gettime reads -- the
performance counter's period for CLOCK_MONOTONIC (at least 1 ns), and the 100
ns FILETIME tick for the realtime and CPU-time clocks.
*/
int
d_clock_getres(
    clockid_t        _clock_id,
    struct timespec* _res
)
{
    // parameter validation
    if (!_res)
    {
        errno = EINVAL;

        return -1;
    }

    // Windows implementation
    // the counter's frequency is fixed at boot, so it is read once and kept
    // (decision 26 of the register: it was reset on every call)
    static LARGE_INTEGER frequency;
    static int           frequency_initialized;

    if (_clock_id == CLOCK_MONOTONIC)
    {
        if (!frequency_initialized)
        {
            QueryPerformanceFrequency(&frequency);
            frequency_initialized = 1;
        }

        _res->tv_sec  = 0;
        _res->tv_nsec = (long)(D_TIME_NSEC_PER_SEC / frequency.QuadPart);

        // ensure minimum resolution of 1 nanosecond
        if (_res->tv_nsec == 0)
        {
            _res->tv_nsec = 1;
        }

        return 0;
    }
    else if (_clock_id == CLOCK_REALTIME)
    {
        // GetSystemTimePreciseAsFileTime has 100ns resolution
        _res->tv_sec  = 0;
        _res->tv_nsec = 100;

        return 0;
    }
    else if ( (_clock_id == CLOCK_PROCESS_CPUTIME_ID) ||
              (_clock_id == CLOCK_THREAD_CPUTIME_ID) )
    {
        // Process/thread times have 100ns resolution
        _res->tv_sec  = 0;
        _res->tv_nsec = 100;

        return 0;
    }

    errno = EINVAL;

    return -1;
}

#else

/*
d_clock_getres
  Fallback: time()'s one-second resolution, for CLOCK_REALTIME only, the one
clock the fallback d_clock_gettime supports.
*/
int
d_clock_getres(
    clockid_t        _clock_id,
    struct timespec* _res
)
{
    // parameter validation
    if (!_res)
    {
        errno = EINVAL;

        return -1;
    }

    // fallback: 1 second resolution
    if (_clock_id == CLOCK_REALTIME)
    {
        _res->tv_sec  = 1;
        _res->tv_nsec = 0;

        return 0;
    }

    errno = EINVAL;

    return -1;
}

#endif

#if D_TIME_HAS_TIMESPEC_GET

/*
d_timespec_get
  Delegates to C11 timespec_get.
*/
int
d_timespec_get(
    struct timespec* _ts,
    int              _base
)
{
    // parameter validation
    if (!_ts)
    {
        return 0;
    }

    // use native timespec_get
    return timespec_get(_ts,
                        _base);
}

#else

/*
d_timespec_get
  Fallback: TIME_UTC only, read through d_clock_gettime(CLOCK_REALTIME).
*/
int
d_timespec_get(
    struct timespec* _ts,
    int              _base
)
{
    // parameter validation
    if (!_ts)
    {
        return 0;
    }

    // fallback implementation
    if (_base == TIME_UTC)
    {
        if (d_clock_gettime(CLOCK_REALTIME,
                            _ts) == 0)
        {
            return _base;
        }
    }

    return 0;
}

#endif

// sleep
#if ( (D_TIME_HAS_NANOSLEEP) &&                                               \
      defined(D_TIME_PLATFORM_POSIX) )

/*
d_nanosleep
  POSIX: checks the nanoseconds itself, then delegates to nanosleep.
*/
int
d_nanosleep(
    const struct timespec* _req,
    struct timespec*       _rem
)
{
    // parameter validation
    if (!_req)
    {
        errno = EINVAL;

        return -1;
    }

    // validate timespec values
    if ( (_req->tv_nsec < 0)                  ||
         (_req->tv_nsec >= D_TIME_NSEC_PER_SEC) )
    {
        errno = EINVAL;

        return -1;
    }

    // use native nanosleep
    return nanosleep(_req,
                     _rem);
}

#elif defined(D_TIME_PLATFORM_WINDOWS)

/*
d_nanosleep
  Windows: Sleep has millisecond resolution, so the request is truncated to
whole milliseconds, with a nonzero sub-millisecond request raised to 1 ms.
When _rem is given, the performance counter measures the sleep, and the
remainder is whatever part of the request it fell short of -- usually nothing.
*/
int
d_nanosleep(
    const struct timespec* _req,
    struct timespec*       _rem
)
{
    // parameter validation
    if (!_req)
    {
        errno = EINVAL;

        return -1;
    }

    // validate timespec values
    if ( (_req->tv_nsec < 0)                  ||
         (_req->tv_nsec >= D_TIME_NSEC_PER_SEC) )
    {
        errno = EINVAL;

        return -1;
    }

    // Windows implementation using Sleep
    // convert to milliseconds (Sleep only has ms resolution)
    DWORD milliseconds = (DWORD)((_req->tv_sec * D_TIME_MSEC_PER_SEC) +
                                 (_req->tv_nsec / D_TIME_NSEC_PER_MSEC));

    // handle sub-millisecond requests
    if ( (milliseconds == 0) &&
         (_req->tv_sec == 0) &&
         (_req->tv_nsec > 0) )
    {
        milliseconds = 1;
    }

    LARGE_INTEGER frequency = {0};
    LARGE_INTEGER start     = {0};
    LARGE_INTEGER end       = {0};

    if (_rem)
    {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
    }

    Sleep(milliseconds);

    if (_rem)
    {
        QueryPerformanceCounter(&end);

        const int64_t elapsed_ns   = ((end.QuadPart - start.QuadPart) *
                                      D_TIME_NSEC_PER_SEC) /
                                     frequency.QuadPart;
        const int64_t requested_ns = (_req->tv_sec * D_TIME_NSEC_PER_SEC) +
                                     _req->tv_nsec;

        if (elapsed_ns < requested_ns)
        {
            const int64_t remaining = requested_ns - elapsed_ns;

            _rem->tv_sec  = (time_t)(remaining / D_TIME_NSEC_PER_SEC);
            _rem->tv_nsec = (long)(remaining % D_TIME_NSEC_PER_SEC);
        }
        else
        {
            _rem->tv_sec  = 0;
            _rem->tv_nsec = 0;
        }
    }

    return 0;
}

#elif (D_INTERNAL_TIME_SLEEP_FALLBACK == 1)

/*
d_nanosleep
  Fallback with C11 threads: thrd_sleep, which takes and reports the same
timespec. It returns 0, -1 when a signal cut the sleep short (the remainder is
in _rem), or another negative value on an error.
*/
int
d_nanosleep(
    const struct timespec* _req,
    struct timespec*       _rem
)
{
    // parameter validation
    if (!_req)
    {
        errno = EINVAL;

        return -1;
    }

    // validate timespec values
    if ( (_req->tv_nsec < 0)                  ||
         (_req->tv_nsec >= D_TIME_NSEC_PER_SEC) )
    {
        errno = EINVAL;

        return -1;
    }

    const int result = thrd_sleep(_req,
                                  _rem);

    // slept
    if (result == 0)
    {
        return 0;
    }

    errno = (result == -1) ? EINTR : EINVAL;

    return -1;
}

#elif (D_INTERNAL_TIME_SLEEP_FALLBACK == 2)

/*
d_nanosleep
  Fallback on a POSIX platform without nanosleep: sleep(), with any
fractional second rounded up to a whole one, and no remaining time reported.
*/
int
d_nanosleep(
    const struct timespec* _req,
    struct timespec*       _rem
)
{
    // parameter validation
    if (!_req)
    {
        errno = EINVAL;

        return -1;
    }

    // validate timespec values
    if ( (_req->tv_nsec < 0)                  ||
         (_req->tv_nsec >= D_TIME_NSEC_PER_SEC) )
    {
        errno = EINVAL;

        return -1;
    }

    unsigned int seconds = (unsigned int)_req->tv_sec;

    // round a fractional second up
    if (_req->tv_nsec > 0)
    {
        seconds += 1;
    }

    if (seconds > 0)
    {
        (void)sleep(seconds);
    }

    if (_rem)
    {
        _rem->tv_sec  = 0;
        _rem->tv_nsec = 0;
    }

    return 0;
}

#else

/*
d_nanosleep
  Fallback with no way to sleep: the arguments are checked as everywhere,
then it fails with ENOSYS (EINVAL where <errno.h> has no ENOSYS).
*/
int
d_nanosleep(
    const struct timespec* _req,
    struct timespec*       _rem
)
{
    // parameter validation
    if (!_req)
    {
        errno = EINVAL;

        return -1;
    }

    // validate timespec values
    if ( (_req->tv_nsec < 0)                  ||
         (_req->tv_nsec >= D_TIME_NSEC_PER_SEC) )
    {
        errno = EINVAL;

        return -1;
    }

    (void)_rem;
    errno = D_INTERNAL_TIME_ENOSYS;

    return -1;
}
#endif

/*
d_usleep
  Splits the microseconds into a timespec and delegates to d_nanosleep.
*/
int
d_usleep(
    unsigned int _usec
)
{
    const struct timespec ts =
    {
        .tv_sec  = _usec / D_TIME_USEC_PER_SEC,
        .tv_nsec = (_usec % D_TIME_USEC_PER_SEC) * D_TIME_NSEC_PER_USEC
    };

    return d_nanosleep(&ts,
                       NULL);
}

/*
d_sleep_ms
  Splits the milliseconds into a timespec and delegates to d_nanosleep.
*/
int
d_sleep_ms(
    unsigned long _milliseconds
)
{
    const struct timespec ts =
    {
        .tv_sec  = (time_t)(_milliseconds / D_TIME_MSEC_PER_SEC),
        .tv_nsec = (long)((_milliseconds % D_TIME_MSEC_PER_SEC) *
                          D_TIME_NSEC_PER_MSEC)
    };

    return d_nanosleep(&ts,
                       NULL);
}

// time zones
#if D_TIME_HAS_TIMEGM

/*
d_timegm
  Delegates to timegm.
*/
time_t
d_timegm(
    struct tm* _tm
)
{
    // parameter validation
    if (!_tm)
    {
        return (time_t)-1;
    }

    // use native timegm
    return timegm(_tm);
}

#elif defined(D_TIME_PLATFORM_WINDOWS)

/*
d_timegm
  Windows: delegates to _mkgmtime.
*/
time_t
d_timegm(
    struct tm* _tm
)
{
    // parameter validation
    if (!_tm)
    {
        return (time_t)-1;
    }

    // use Windows _mkgmtime
    return _mkgmtime(_tm);
}

#else

/*
d_timegm
  Portable fallback: counts whole days from the epoch -- year by year, then
month by month under the Gregorian leap rule -- and adds the time of day.
Months outside 0..11 are folded into the year first; days, hours, minutes, and
seconds are used as given, so out-of-range values carry naturally. Then, as
timegm does, it writes the normalized time back into _tm, tm_wday and
tm_yday included (decision 25 of the register; it used to leave _tm as
given).
*/
time_t
d_timegm(
    struct tm* _tm
)
{
    // parameter validation
    if (!_tm)
    {
        return (time_t)-1;
    }

    // portable fallback implementation
    // this algorithm is based on the public domain implementation
    // days in each month (non-leap year)
    static const int days_in_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    int       year  = _tm->tm_year + 1900;
    int       month = _tm->tm_mon;
    const int day   = _tm->tm_mday;

    // normalize month
    while (month < 0)
    {
        month += 12;
        year  -= 1;
    }

    while (month >= 12)
    {
        month -= 12;
        year  += 1;
    }

    // calculate days since Unix epoch
    time_t result = 0;

    // years since 1970
    for (int i = 1970; i < year; i++)
    {
        result += 365;
        // add leap day
        if ( ((i % 4 == 0) && (i % 100 != 0)) ||
             (i % 400 == 0) )
        {
            result += 1;
        }
    }

    // handle years before 1970
    for (int i = year; i < 1970; i++)
    {
        result -= 365;
        if ( ((i % 4 == 0) && (i % 100 != 0)) ||
             (i % 400 == 0) )
        {
            result -= 1;
        }
    }

    // add days for each month
    for (int i = 0; i < month; i++)
    {
        result += days_in_month[i];
        // add leap day for February
        if ( (i == 1) &&
             (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) )
        {
            result += 1;
        }
    }

    // add day of month
    result += day - 1;

    // convert to seconds and add time
    result *= 86400;
    result += _tm->tm_hour * 3600;
    result += _tm->tm_min * 60;
    result += _tm->tm_sec;

    // normalize _tm as timegm does: the broken-down form of the result
    struct tm normalized;

    if (d_gmtime(&result,
                 &normalized) != NULL)
    {
        *_tm = normalized;
    }

    return result;
}

#endif

/*
d_tzset
  Delegates to _tzset on Windows and to tzset elsewhere.
*/
void
d_tzset(
    void
)
{
#if defined(D_TIME_PLATFORM_WINDOWS)
    _tzset();
#else
    tzset();
#endif

    return;
}

// parsing and formatting
#if D_TIME_HAS_STRPTIME

/*
d_strptime
  Delegates to strptime.
*/
char*
d_strptime(
    const char* _s,
    const char* _format,
    struct tm*  _tm
)
{
    // parameter validation
    if ( (!_s)      ||
         (!_format) ||
         (!_tm) )
    {
        return NULL;
    }

    // use native strptime
    return strptime(_s,
                    _format,
                    _tm);
}

#else

/*
d_strptime
  Fallback parser. It zeroes _tm, then walks the format: a conversion reads at
most its field's digits, names match case-insensitively in either abbreviated
or full form, and %p adjusts the hour already parsed, so it must follow %I. A
space in the format skips any run of spaces in the input, and any other
character must match exactly. Parsing stops, without failing, when either
string runs out; an unmatched name or an unsupported conversion is skipped
rather than rejected.
*/
char*
d_strptime(
    const char* _s,
    const char* _format,
    struct tm*  _tm
)
{
    // parameter validation
    if ( (!_s)      ||
         (!_format) ||
         (!_tm) )
    {
        return NULL;
    }

    // Windows/fallback implementation
    // supports common format specifiers including month and weekday names
    // month names (abbreviated and full)
    static const char* month_abbrev[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static const char* month_full[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    // weekday names (abbreviated and full)
    static const char* weekday_abbrev[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char* weekday_full[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };

    const char* sp = _s;
    const char* fp = _format;

    // initialize tm to zeros
    d_memset(_tm,
             0,
             sizeof(struct tm));

    while ( (*fp) &&
            (*sp) )
    {
        if (*fp == '%')
        {
            fp++;
            if (!*fp)
            {
                break;
            }

            int value   = 0;
            int digits  = 0;
            int matched = 0;  // the name conversions' result: %b, %B, %a, %A

            switch (*fp)
            {
                case 'Y':  // 4-digit year
                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 4) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_year = value - 1900;
                    break;

                case 'y':  // 2-digit year
                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_year = (value < 69) ? value + 100 : value;
                    break;

                case 'm':  // month (01-12)
                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_mon = value - 1;
                    break;

                case 'b':  // abbreviated month name (Jan, Feb, ...)
                case 'h':  // same as %b
                    matched = 0;

                    for (int i = 0; i < 12; i++)
                    {
                        size_t len = strlen(month_abbrev[i]);

                        if (d_strncasecmp(sp,
                                          month_abbrev[i],
                                          len) == 0)
                        {
                            _tm->tm_mon = i;
                            sp += len;
                            matched = 1;
                            break;
                        }
                    }

                    if (!matched)
                    {
                        // try full month names as fallback
                        for (int i = 0; i < 12; i++)
                        {
                            size_t len = strlen(month_full[i]);

                            if (d_strncasecmp(sp,
                                              month_full[i],
                                              len) == 0)
                            {
                                _tm->tm_mon = i;
                                sp += len;
                                matched = 1;
                                break;
                            }
                        }
                    }

                    break;

                case 'B':  // full month name (January, February, ...)
                    matched = 0;

                    for (int i = 0; i < 12; i++)
                    {
                        size_t len = strlen(month_full[i]);

                        if (d_strncasecmp(sp,
                                          month_full[i],
                                          len) == 0)
                        {
                            _tm->tm_mon = i;
                            sp += len;
                            matched = 1;
                            break;
                        }
                    }

                    if (!matched)
                    {
                        // try abbreviated month names as fallback
                        for (int i = 0; i < 12; i++)
                        {
                            size_t len = strlen(month_abbrev[i]);

                            if (d_strncasecmp(sp,
                                              month_abbrev[i],
                                              len) == 0)
                            {
                                _tm->tm_mon = i;
                                sp += len;
                                matched = 1;
                                break;
                            }
                        }
                    }

                    break;

                case 'a':  // abbreviated weekday name (Sun, Mon, ...)
                    matched = 0;

                    for (int i = 0; i < 7; i++)
                    {
                        size_t len = strlen(weekday_abbrev[i]);

                        if (d_strncasecmp(sp,
                                          weekday_abbrev[i],
                                          len) == 0)
                        {
                            _tm->tm_wday = i;
                            sp += len;
                            matched = 1;
                            break;
                        }
                    }

                    if (!matched)
                    {
                        // try full weekday names as fallback
                        for (int i = 0; i < 7; i++)
                        {
                            size_t len = strlen(weekday_full[i]);

                            if (d_strncasecmp(sp,
                                              weekday_full[i],
                                              len) == 0)
                            {
                                _tm->tm_wday = i;
                                sp += len;
                                matched = 1;
                                break;
                            }
                        }
                    }

                    break;

                case 'A':  // full weekday name (Sunday, Monday, ...)
                    matched = 0;

                    for (int i = 0; i < 7; i++)
                    {
                        size_t len = strlen(weekday_full[i]);

                        if (d_strncasecmp(sp,
                                          weekday_full[i],
                                          len) == 0)
                        {
                            _tm->tm_wday = i;
                            sp += len;
                            matched = 1;
                            break;
                        }
                    }

                    if (!matched)
                    {
                        // try abbreviated weekday names as fallback
                        for (int i = 0; i < 7; i++)
                        {
                            size_t len = strlen(weekday_abbrev[i]);

                            if (d_strncasecmp(sp,
                                              weekday_abbrev[i],
                                              len) == 0)
                            {
                                _tm->tm_wday = i;
                                sp += len;
                                matched = 1;
                                break;
                            }
                        }
                    }

                    break;

                case 'd':  // day of month (01-31)
                case 'e':  // day of month (1-31, space padded)
                    // skip leading space for %e
                    if (*sp == ' ')
                    {
                        sp++;
                    }

                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_mday = value;
                    break;

                case 'H':  // hour (00-23)
                case 'k':  // hour (0-23, space padded)
                    if (*sp == ' ')
                    {
                        sp++;
                    }

                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_hour = value;
                    break;

                case 'I':  // hour (01-12)
                case 'l':  // hour (1-12, space padded)
                    if (*sp == ' ')
                    {
                        sp++;
                    }

                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_hour = value;  // will need AM/PM adjustment
                    break;

                case 'M':  // minute (00-59)
                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_min = value;
                    break;

                case 'S':  // second (00-60)
                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 2) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_sec = value;
                    break;

                case 'j':  // day of year (001-366)
                    while ( (*sp >= '0') &&
                            (*sp <= '9') &&
                            (digits < 3) )
                    {
                        value = value * 10 + (*sp - '0');
                        sp++;
                        digits++;
                    }

                    _tm->tm_yday = value - 1;
                    break;

                case 'w':  // weekday as decimal (0-6, Sunday = 0)
                    if ( (*sp >= '0') &&
                         (*sp <= '6') )
                    {
                        _tm->tm_wday = *sp - '0';
                        sp++;
                    }

                    break;

                case 'p':  // AM/PM
                case 'P':  // am/pm
                    if ( ((*sp == 'P') || (*sp == 'p')) &&
                         ((*(sp+1) == 'M') || (*(sp+1) == 'm')) )
                    {
                        if (_tm->tm_hour < 12)
                        {
                            _tm->tm_hour += 12;
                        }

                        sp += 2;
                    }
                    else if ( ((*sp == 'A') || (*sp == 'a')) &&
                              ((*(sp+1) == 'M') || (*(sp+1) == 'm')) )
                    {
                        if (_tm->tm_hour == 12)
                        {
                            _tm->tm_hour = 0;
                        }

                        sp += 2;
                    }

                    break;

                case '%':  // literal %
                    if (*sp == '%')
                    {
                        sp++;
                    }
                    else
                    {
                        return NULL;
                    }

                    break;

                case 'n':  // newline
                case 't':  // tab
                    // skip whitespace
                    while ( (*sp == ' ')  ||
                            (*sp == '\t') ||
                            (*sp == '\n') )
                    {
                        sp++;
                    }

                    break;

                default:
                    // unsupported format specifier, skip it
                    break;
            }

            fp++;
        }
        else if (*fp == ' ')
        {
            // skip whitespace in format and input
            while (*sp == ' ')
            {
                sp++;
            }

            fp++;
        }
        else
        {
            // literal character match
            if (*sp != *fp)
            {
                return NULL;
            }

            sp++;
            fp++;
        }
    }

    return (char*)sp;
}

#endif

/*
d_strftime_s
  On an invalid argument the buffer is emptied whenever it has room for a
terminator, so a caller that ignores the 0 still reads a terminated string.
It is strftime everywhere: the branch that called strftime_s, which no C
runtime has, behind D_ENV_CRT_MSVC, which only simulated-environment builds
define, is gone (decision 27 of the register). The size_t count is narrowed
to int.
*/
int
d_strftime_s(
    char*            _s,
    size_t           _maxsize,
    const char*      _format,
    const struct tm* _tm
)
{
    // parameter validation
    if ( (!_s)      ||
         (!_format) ||
         (!_tm)     ||
         (_maxsize == 0) )
    {
        if ( (_s) &&
             (_maxsize > 0) )
        {
            _s[0] = '\0';
        }

        return 0;
    }

    const size_t result = strftime(_s,
                                   _maxsize,
                                   _format,
                                   _tm);

    return (int)result;
}

// timespec arithmetic
/*
d_timespec_add
  Adds seconds and nanoseconds separately, then carries one second if the
nanoseconds reach a full second. A single carry repairs only normalized
operands.
*/
void
d_timespec_add(
    const struct timespec* _a,
    const struct timespec* _b,
    struct timespec*       _result
)
{
    // parameter validation
    if ( (!_a)      ||
         (!_b)      ||
         (!_result) )
    {
        return;
    }

    _result->tv_sec  = _a->tv_sec + _b->tv_sec;
    _result->tv_nsec = _a->tv_nsec + _b->tv_nsec;

    // handle nanosecond overflow
    if (_result->tv_nsec >= D_TIME_NSEC_PER_SEC)
    {
        _result->tv_sec  += 1;
        _result->tv_nsec -= D_TIME_NSEC_PER_SEC;
    }

    return;
}

/*
d_timespec_sub
  Subtracts seconds and nanoseconds separately, then borrows one second if the
nanoseconds went negative. A single borrow repairs only normalized operands.
*/
void
d_timespec_sub(
    const struct timespec* _a,
    const struct timespec* _b,
    struct timespec*       _result
)
{
    // parameter validation
    if ( (!_a)      ||
         (!_b)      ||
         (!_result) )
    {
        return;
    }

    _result->tv_sec  = _a->tv_sec - _b->tv_sec;
    _result->tv_nsec = _a->tv_nsec - _b->tv_nsec;

    // handle nanosecond underflow
    if (_result->tv_nsec < 0)
    {
        _result->tv_sec  -= 1;
        _result->tv_nsec += D_TIME_NSEC_PER_SEC;
    }

    return;
}

/*
d_timespec_cmp
  Compares seconds, then nanoseconds. NULL orders before every value, on
either side, so swapping the arguments negates the result (decision 28 of
the register: either one alone used to yield -1).
*/
int
d_timespec_cmp(
    const struct timespec* _a,
    const struct timespec* _b
)
{
    // NULL orders first
    if ( (!_a) &&
         (!_b) )
    {
        return 0;
    }

    if (!_a)
    {
        return -1;
    }

    if (!_b)
    {
        return 1;
    }

    // compare seconds first
    if (_a->tv_sec != _b->tv_sec)
    {
        return (_a->tv_sec < _b->tv_sec)
            ? -1
            : 1;
    }

    // seconds are equal, compare nanoseconds
    if (_a->tv_nsec != _b->tv_nsec)
    {
        return (_a->tv_nsec < _b->tv_nsec)
            ? -1
            : 1;
    }

    return 0;
}

/*
d_timespec_to_ms
  Widens the seconds to int64_t before scaling, so a 32-bit time_t cannot
overflow the multiply; the nanoseconds are truncated toward zero.
*/
int64_t
d_timespec_to_ms(
    const struct timespec* _ts
)
{
    if (!_ts)
    {
        return 0;
    }

    return ((int64_t)_ts->tv_sec * D_TIME_MSEC_PER_SEC) +
           (_ts->tv_nsec / D_TIME_NSEC_PER_MSEC);
}

/*
d_timespec_to_us
  Widens the seconds to int64_t before scaling, so a 32-bit time_t cannot
overflow the multiply; the nanoseconds are truncated toward zero.
*/
int64_t
d_timespec_to_us(
    const struct timespec* _ts
)
{
    if (!_ts)
    {
        return 0;
    }

    return ((int64_t)_ts->tv_sec * D_TIME_USEC_PER_SEC) +
           (_ts->tv_nsec / D_TIME_NSEC_PER_USEC);
}

/*
d_timespec_to_ns
  Widens the seconds to int64_t before scaling, so a 32-bit time_t cannot
overflow the multiply. The precondition, a seconds count whose nanosecond
total fits int64_t, is asserted in debug builds (decision 14 of the
register); with NDEBUG, breaking it is undefined, as the contract says.
*/
int64_t
d_timespec_to_ns(
    const struct timespec* _ts
)
{
    if (!_ts)
    {
        return 0;
    }

    assert( ( (int64_t)_ts->tv_sec >= (INT64_MIN / D_TIME_NSEC_PER_SEC) ) &&
            ( (int64_t)_ts->tv_sec <=
              ( (INT64_MAX - (D_TIME_NSEC_PER_SEC - 1)) /
                D_TIME_NSEC_PER_SEC ) ) );

    return ((int64_t)_ts->tv_sec * D_TIME_NSEC_PER_SEC) + _ts->tv_nsec;
}

/*
d_ms_to_timespec
  Splits with / and %, which truncate toward zero, so a negative input yields
a negative tv_nsec rather than a normalized timespec.
*/
void
d_ms_to_timespec(
    int64_t          _milliseconds,
    struct timespec* _ts
)
{
    if (!_ts)
    {
        return;
    }

    _ts->tv_sec  = (time_t)(_milliseconds / D_TIME_MSEC_PER_SEC);
    _ts->tv_nsec = (long)((_milliseconds % D_TIME_MSEC_PER_SEC) *
                          D_TIME_NSEC_PER_MSEC);

    return;
}

/*
d_us_to_timespec
  Splits with / and %, which truncate toward zero, so a negative input yields
a negative tv_nsec rather than a normalized timespec.
*/
void
d_us_to_timespec(
    int64_t          _microseconds,
    struct timespec* _ts
)
{
    if (!_ts)
    {
        return;
    }

    _ts->tv_sec  = (time_t)(_microseconds / D_TIME_USEC_PER_SEC);
    _ts->tv_nsec = (long)((_microseconds % D_TIME_USEC_PER_SEC) *
                          D_TIME_NSEC_PER_USEC);

    return;
}

/*
d_ns_to_timespec
  Splits with / and %, which truncate toward zero, so a negative input yields
a negative tv_nsec rather than a normalized timespec.
*/
void
d_ns_to_timespec(
    int64_t          _nanoseconds,
    struct timespec* _ts
)
{
    if (!_ts)
    {
        return;
    }

    _ts->tv_sec  = (time_t)(_nanoseconds / D_TIME_NSEC_PER_SEC);
    _ts->tv_nsec = (long)(_nanoseconds % D_TIME_NSEC_PER_SEC);

    return;
}

// monotonic time
/*
d_monotonic_time_ms
  Reads CLOCK_MONOTONIC, falling back to CLOCK_REALTIME -- which is not
monotonic -- when the monotonic clock cannot be read.
*/
int64_t
d_monotonic_time_ms(
    void
)
{
    struct timespec ts = {0};

    if (d_clock_gettime(CLOCK_MONOTONIC,
                        &ts) == 0)
    {
        return d_timespec_to_ms(&ts);
    }

    // fallback to realtime if monotonic not available
    if (d_clock_gettime(CLOCK_REALTIME,
                        &ts) == 0)
    {
        return d_timespec_to_ms(&ts);
    }

    return 0;
}

/*
d_monotonic_time_us
  Reads CLOCK_MONOTONIC, falling back to CLOCK_REALTIME -- which is not
monotonic -- when the monotonic clock cannot be read.
*/
int64_t
d_monotonic_time_us(
    void
)
{
    struct timespec ts = {0};

    if (d_clock_gettime(CLOCK_MONOTONIC,
                        &ts) == 0)
    {
        return d_timespec_to_us(&ts);
    }

    if (d_clock_gettime(CLOCK_REALTIME,
                        &ts) == 0)
    {
        return d_timespec_to_us(&ts);
    }

    return 0;
}

/*
d_monotonic_time_ns
  Reads CLOCK_MONOTONIC, falling back to CLOCK_REALTIME -- which is not
monotonic -- when the monotonic clock cannot be read.
*/
int64_t
d_monotonic_time_ns(
    void
)
{
    struct timespec ts = {0};

    if (d_clock_gettime(CLOCK_MONOTONIC,
                        &ts) == 0)
    {
        return d_timespec_to_ns(&ts);
    }

    if (d_clock_gettime(CLOCK_REALTIME,
                        &ts) == 0)
    {
        return d_timespec_to_ns(&ts);
    }

    return 0;
}

// normalization and validation
/*
d_timespec_normalize
  Moves whole seconds between tv_nsec and tv_sec in 64-bit arithmetic, since
tv_nsec may be a 32-bit long. Negative nanoseconds borrow with a ceiling
division, and a final range check catches its edge cases.
*/
void
d_timespec_normalize(
    struct timespec* _ts
)
{
    long long nsec_ll;
    long long sec_adj;

    if (!_ts)
    {
        return;
    }

    // use `long long` for all arithmetic to handle large values and overflow
    // this is necessary because tv_nsec on some platforms is a 32-bit long,
    // but the tests may assign values like 5000000000LL that exceed 32 bits
    nsec_ll = (long long)_ts->tv_nsec;

    // handle positive overflow (nsec >= 1 billion)
    if (nsec_ll >= D_TIME_NSEC_PER_SEC)
    {
        // calculate how many extra seconds we have
        sec_adj = nsec_ll / D_TIME_NSEC_PER_SEC;
        _ts->tv_sec += (time_t)sec_adj;
        _ts->tv_nsec = (long)(nsec_ll - sec_adj * D_TIME_NSEC_PER_SEC);
    }
    // handle negative nanoseconds
    else if (nsec_ll < 0)
    {
        // for negative values, we need to borrow from seconds
        // example: -300000000 ns -> borrow 1 sec, get 700000000 ns
        // example: -2500000000 ns -> borrow 3 sec, get 500000000 ns

        // calculate how many seconds to borrow (ceiling division for negatives)
        // we want sec_adj to be positive, representing seconds to subtract
        sec_adj = (-nsec_ll + D_TIME_NSEC_PER_SEC - 1) / D_TIME_NSEC_PER_SEC;

        _ts->tv_sec -= (time_t)sec_adj;
        _ts->tv_nsec = (long)(nsec_ll + sec_adj * D_TIME_NSEC_PER_SEC);

        // ensure tv_nsec is in valid range [0, 999999999]
        // this handles edge cases from the ceiling division
        if (_ts->tv_nsec < 0)
        {
            _ts->tv_sec--;
            _ts->tv_nsec += D_TIME_NSEC_PER_SEC;
        }
        else if (_ts->tv_nsec >= D_TIME_NSEC_PER_SEC)
        {
            _ts->tv_sec++;
            _ts->tv_nsec -= D_TIME_NSEC_PER_SEC;
        }
    }

    return;
}

/*
d_timespec_is_valid
  Checks tv_nsec only; any tv_sec, negative included, is accepted.
*/
int
d_timespec_is_valid(
    const struct timespec* _ts
)
{
    if (!_ts)
    {
        return 0;
    }

    // tv_nsec must be in range [0, 999999999]
    if ( (_ts->tv_nsec < 0) ||
         (_ts->tv_nsec >= D_TIME_NSEC_PER_SEC) )
    {
        return 0;
    }

    // for positive times, tv_sec should be non-negative
    // (negative tv_sec with positive tv_nsec is technically valid but unusual)

    return 1;
}
