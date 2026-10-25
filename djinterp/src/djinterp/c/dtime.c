/*******************************************************************************
* djinterp [c]                                                           dtime.c
*
* Definitions for the declarations in `dtime.h`.
*   Each platform-dependent function is defined once per platform -- Windows,
* POSIX, and a portable fallback -- with the implementation selected at the
* function level rather than by conditionals inside one body.
*   Build it with -D_XOPEN_SOURCE=700 where the C library hides POSIX in a
* strict ISO mode (the C guide's rule). timegm, where D_TIME_HAS_TIMEGM
* selects it, needs _DEFAULT_SOURCE as well on glibc. d_strptime parses for
* itself on every platform and never calls the system's strptime.
*
*
* path:      /src/djinterp/c/dtime.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.21
*                                                            revised: 2026.10.18
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
        .tv_sec  = (time_t)(_usec / D_TIME_USEC_PER_SEC),
        .tv_nsec = (long)((_usec % D_TIME_USEC_PER_SEC) *
                          D_TIME_NSEC_PER_USEC)
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

// d_time_internal_parse
//   struct: a parse in progress: the input left, the struct it fills, and
// what the conversions so far have read that d_strptime's last step must
// settle -- the century and year in it, the week, the 12-hour clock, and
// which of the date's fields the input gave.
struct d_time_internal_parse
{
    const char* input;
    struct tm*  tm;
    int         century;     // %C's, or -1
    int         short_year;  // %y's, or -1
    int         week;        // %U's or %W's, or -1
    int         hour12;      // %I's or %l's, or -1
    int         pm;          // %p's: 1 PM, 0 AM, -1 unread
    bool        monday;      // the week is %W's, counted from Monday
    bool        year;        // the year is known
    bool        month;       // %b, %B, %h or %m read
    bool        mday;        // %d or %e read
    bool        yday;        // %j read, or the day of the year settled
    bool        wday;        // %a, %A, %u or %w read
};

/*
DAY_NAMES, MONTH_NAMES
  The C locale's names, whatever the process's locale; each abbreviation is
its name's first three letters.
*/
static const char* const DAY_NAMES[7] =
{
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
    "Saturday"
};
static const char* const MONTH_NAMES[12] =
{
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};

/*
YEAR_DAYS
  The days before each month, and in the year, for a common year and for a
leap year.
*/
static const int YEAR_DAYS[2][13] =
{
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

/**
 * @brief Whether a character is white space in the C locale.
 *
 * @param[in] _c  the character.
 * @return true for a space, tab, newline, vertical tab, form feed or return.
 */
static bool
d_time_internal_is_space(
    char _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\n') ||
             (_c == '\v') ||
             (_c == '\f') ||
             (_c == '\r') );
}

/**
 * @brief Steps over any white space at the parse's position.
 *
 * @param[in,out] _parse  the parse.
 */
static void
d_time_internal_skip_space(
    struct d_time_internal_parse* _parse
)
{
    // each white-space character
    while (d_time_internal_is_space(*_parse->input))
    {
        ++_parse->input;
    }

    return;
}

/**
 * @brief Whether a proleptic Gregorian year is a leap year.
 *
 * @param[in] _year  the year, as written.
 * @return 1 for a leap year, 0 otherwise, to index YEAR_DAYS.
 */
static int
d_time_internal_leap(
    int _year
)
{
    return ( ( ((_year % 4) == 0) &&
               ((_year % 100) != 0) ) ||
             ((_year % 400) == 0) ) ? 1 : 0;
}

/**
 * @brief The day of the week of a date, 0 for Sunday.
 *
 * @param[in] _year   the year, as written; any year.
 * @param[in] _month  the month, 0 to 11.
 * @param[in] _mday   the day of the month.
 * @return the day of the week, 0 to 6.
 *
 * Counts days from 1970-01-01, a Thursday, with Howard Hinnant's
 * days-from-civil, which starts each year at March so that February's
 * length is the year's last term.
 */
static int
d_time_internal_weekday(
    int _year,
    int _month,
    int _mday
)
{
    const long year  = (long)_year - ((_month < 2) ? 1L : 0L);
    const long era   = ((year >= 0) ? year : (year - 399L)) / 400L;
    const long yoe   = year - (era * 400L);
    const long month = (long)_month + ((_month < 2) ? 10L : -2L);
    const long doy   = (((153L * month) + 2L) / 5L) + (long)_mday - 1L;
    const long doe   = (yoe * 365L) + (yoe / 4L) - (yoe / 100L) + doy;
    const long days  = (era * 146097L) + doe - 719468L;
    const long wday  = (days + 4L) % 7L;

    return (int)((wday < 0L) ? (wday + 7L) : wday);
}

/**
 * @brief Reads a number of at most `_digits` digits in [_low, _high].
 *
 * @param[in,out] _parse   the parse; its position passes the digits.
 * @param[in]     _low     the least value accepted.
 * @param[in]     _high    the greatest value accepted.
 * @param[in]     _digits  the most digits read.
 * @param[out]    _value   receives the number.
 * @return true when a number in range was read.
 *
 * White space before the number is skipped, as %e's padding needs. A digit
 * is read while the field has room and the value so far, times ten, does
 * not pass `_high`, as the C libraries read: "123" as a month is 12,
 * leaving "3".
 */
static bool
d_time_internal_number(
    struct d_time_internal_parse* _parse,
    int                           _low,
    int                           _high,
    int                           _digits,
    int*                          _value
)
{
    int value = 0;
    int count = 0;

    d_time_internal_skip_space(_parse);

    // digits, while the field has room and the value can still grow
    while ( (count < _digits)        &&
            (*_parse->input >= '0')  &&
            (*_parse->input <= '9')  &&
            ( (count == 0) ||
              ((value * 10) <= _high) ) )
    {
        value = (value * 10) + (*_parse->input - '0');
        ++_parse->input;
        ++count;
    }

    *_value = value;

    return ( (count > 0)      &&
             (value >= _low)  &&
             (value <= _high) );
}

/**
 * @brief Reads exactly `_digits` digits, with nothing skipped before them.
 *
 * @param[in,out] _parse   the parse; its position passes the digits.
 * @param[in]     _digits  how many digits.
 * @param[out]    _value   receives the number.
 * @return true when that many digits were there.
 */
static bool
d_time_internal_digits(
    struct d_time_internal_parse* _parse,
    int                           _digits,
    int*                          _value
)
{
    int value = 0;

    // each digit in turn
    for (int i = 0; i < _digits; ++i)
    {
        const char digit = _parse->input[i];

        // anything else ends the field too soon
        if ( (digit < '0') ||
             (digit > '9') )
        {
            return false;
        }

        value = (value * 10) + (digit - '0');
    }

    _parse->input += _digits;
    *_value        = value;

    return true;
}

/**
 * @brief Reads a day's or a month's name, full or abbreviated, in any case.
 *
 * @param[in,out] _parse  the parse; its position passes the name.
 * @param[in]     _names  the full names, in order.
 * @param[in]     _count  how many names there are.
 * @param[out]    _index  receives the index of the name read.
 * @return true when a name was read.
 *
 * Each name is tried in full and then by its first three letters before the
 * next name is tried, as the C libraries try them in the C locale.
 */
static bool
d_time_internal_name(
    struct d_time_internal_parse* _parse,
    const char* const*            _names,
    int                           _count,
    int*                          _index
)
{
    // each name, in full and then abbreviated
    for (int i = 0; i < _count; ++i)
    {
        const size_t full  = strlen(_names[i]);
        const size_t taken = (d_strncasecmp(_parse->input,
                                            _names[i],
                                            full) == 0)
                                 ? full
                                 : (d_strncasecmp(_parse->input,
                                                  _names[i],
                                                  3u) == 0)
                                       ? 3u
                                       : 0u;

        // the first name to match is the one read
        if (taken > 0u)
        {
            _parse->input += taken;
            *_index        = i;

            return true;
        }
    }

    return false;
}

/**
 * @brief Reads %p's "AM" or "PM", in any case.
 *
 * @param[in,out] _parse  the parse; its position passes the marker.
 * @return true when either was read.
 */
static bool
d_time_internal_meridiem(
    struct d_time_internal_parse* _parse
)
{
    const bool am = (d_strncasecmp(_parse->input,
                                   "AM",
                                   2u) == 0);
    const bool pm = (d_strncasecmp(_parse->input,
                                   "PM",
                                   2u) == 0);

    // neither marker
    if ( (!am) &&
         (!pm) )
    {
        return false;
    }

    _parse->pm     = (pm) ? 1 : 0;
    _parse->input += 2;

    return true;
}

/**
 * @brief Reads %z's offset: "Z", or a sign and hh, hhmm or hh:mm.
 *
 * @param[in,out] _parse  the parse; its position passes the offset.
 * @return true when an offset of at most 23:59 was read. It is checked and
 *         not stored, struct tm having no standard field for it.
 */
static bool
d_time_internal_offset(
    struct d_time_internal_parse* _parse
)
{
    int hours   = 0;
    int minutes = 0;

    d_time_internal_skip_space(_parse);

    // Z alone is UTC
    if (*_parse->input == 'Z')
    {
        ++_parse->input;

        return true;
    }

    // a sign
    if ( (*_parse->input != '+') &&
         (*_parse->input != '-') )
    {
        return false;
    }

    ++_parse->input;

    // the hours' two digits
    if (!d_time_internal_digits(_parse,
                                2,
                                &hours))
    {
        return false;
    }

    // a colon before the minutes is skipped where minutes follow it
    if ( (_parse->input[0] == ':') &&
         (_parse->input[1] >= '0') &&
         (_parse->input[1] <= '9') )
    {
        ++_parse->input;
    }

    // minutes, if any digit follows, must be two
    if ( (*_parse->input >= '0') &&
         (*_parse->input <= '9') &&
         (!d_time_internal_digits(_parse,
                                  2,
                                  &minutes)) )
    {
        return false;
    }

    return ( (hours <= 23) &&
             (minutes <= 59) );
}

/**
 * @brief Reads a date conversion: a name, a number or a week.
 *
 * @param[in,out] _parse       the parse.
 * @param[in]     _conversion  the conversion's character.
 * @return true when it read; false for a mismatch or a character that is
 *         no date conversion.
 */
static bool
d_time_internal_date(
    struct d_time_internal_parse* _parse,
    char                          _conversion
)
{
    struct tm* const tm    = _parse->tm;
    int              value = 0;

    switch (_conversion)
    {
        case 'a':
        case 'A':
            _parse->wday = d_time_internal_name(_parse,
                                           DAY_NAMES,
                                           7,
                                           &tm->tm_wday);

            return _parse->wday;
        case 'b':
        case 'B':
        case 'h':
            _parse->month = d_time_internal_name(_parse,
                                           MONTH_NAMES,
                                           12,
                                           &tm->tm_mon);

            return _parse->month;
        case 'C':
            return d_time_internal_number(_parse,
                                          0,
                                          99,
                                          2,
                                          &_parse->century);
        case 'd':
        case 'e':
            _parse->mday = d_time_internal_number(_parse,
                                                  1,
                                                  31,
                                                  2,
                                                  &tm->tm_mday);

            return _parse->mday;
        case 'j':
            _parse->yday = d_time_internal_number(_parse,
                                                  1,
                                                  366,
                                                  3,
                                                  &value);
            tm->tm_yday  = value - 1;

            return _parse->yday;
        case 'm':
            _parse->month = d_time_internal_number(_parse,
                                                   1,
                                                   12,
                                                   2,
                                                   &value);
            tm->tm_mon    = value - 1;

            return _parse->month;
        case 'u':
            _parse->wday = d_time_internal_number(_parse,
                                                  1,
                                                  7,
                                                  1,
                                                  &value);
            tm->tm_wday  = value % 7;

            return _parse->wday;
        case 'U':
        case 'W':
            _parse->monday = (_conversion == 'W');

            return d_time_internal_number(_parse,
                                          0,
                                          53,
                                          2,
                                          &_parse->week);
        case 'w':
            _parse->wday = d_time_internal_number(_parse,
                                                  0,
                                                  6,
                                                  1,
                                                  &tm->tm_wday);

            return _parse->wday;
        case 'y':
            return d_time_internal_number(_parse,
                                          0,
                                          99,
                                          2,
                                          &_parse->short_year);
        case 'Y':
            _parse->year = d_time_internal_number(_parse,
                                                  0,
                                                  9999,
                                                  4,
                                                  &value);
            tm->tm_year  = value - 1900;

            return _parse->year;
        default:
            return false;
    }
}

/**
 * @brief Reads a time conversion: an hour, a minute, a second, a meridiem
 *        or a zone.
 *
 * @param[in,out] _parse       the parse.
 * @param[in]     _conversion  the conversion's character.
 * @return true when it read; false for a mismatch or a character that is
 *         no time conversion.
 */
static bool
d_time_internal_time(
    struct d_time_internal_parse* _parse,
    char                          _conversion
)
{
    struct tm* const tm = _parse->tm;

    switch (_conversion)
    {
        case 'H':
        case 'k':
            _parse->hour12 = -1;

            return d_time_internal_number(_parse,
                                          0,
                                          23,
                                          2,
                                          &tm->tm_hour);
        case 'I':
        case 'l':
            return d_time_internal_number(_parse,
                                          1,
                                          12,
                                          2,
                                          &_parse->hour12);
        case 'M':
            return d_time_internal_number(_parse,
                                          0,
                                          59,
                                          2,
                                          &tm->tm_min);
        case 'S':
            return d_time_internal_number(_parse,
                                          0,
                                          60,
                                          2,
                                          &tm->tm_sec);
        case 'p':
        case 'P':
            return d_time_internal_meridiem(_parse);
        case 'z':
            return d_time_internal_offset(_parse);
        case 'Z':
            // a zone's name: skipped, to the next white space
            while ( (*_parse->input != '\0') &&
                    (!d_time_internal_is_space(*_parse->input)) )
            {
                ++_parse->input;
            }

            return true;
        default:
            return false;
    }
}

static bool d_time_internal_walk(struct d_time_internal_parse* _parse,
                                 const char*                   _format);

/**
 * @brief Reads one conversion, a composite one through its C-locale form.
 *
 * @param[in,out] _parse       the parse.
 * @param[in]     _conversion  the conversion's character.
 * @return true when it read; false for a mismatch or an unknown conversion.
 */
static bool
d_time_internal_convert(
    struct d_time_internal_parse* _parse,
    char                          _conversion
)
{
    switch (_conversion)
    {
        case 'c':
            return d_time_internal_walk(_parse,
                                        "%a %b %e %H:%M:%S %Y");
        case 'D':
        case 'x':
            return d_time_internal_walk(_parse,
                                        "%m/%d/%y");
        case 'r':
            return d_time_internal_walk(_parse,
                                        "%I:%M:%S %p");
        case 'R':
            return d_time_internal_walk(_parse,
                                        "%H:%M");
        case 'T':
        case 'X':
            return d_time_internal_walk(_parse,
                                        "%H:%M:%S");
        case 'n':
        case 't':
            d_time_internal_skip_space(_parse);

            return true;
        case '%':
            // a percent sign matches only itself
            if (*_parse->input != '%')
            {
                return false;
            }

            ++_parse->input;

            return true;
        default:
            return ( (d_time_internal_date(_parse,
                                           _conversion)) ||
                     (d_time_internal_time(_parse,
                                           _conversion)) );
    }
}

/**
 * @brief Whether POSIX allows an E or O modifier before a conversion.
 *
 * @param[in] _modifier    'E' or 'O'.
 * @param[in] _conversion  the conversion's character.
 * @return true where POSIX defines the pair; the C locale reads it as the
 *         plain conversion.
 */
static bool
d_time_internal_modifies(
    char _modifier,
    char _conversion
)
{
    const char* const allowed = (_modifier == 'E') ? "cCxXyY"
                                                   : "deHImMSuUVwWy";

    return ( (_conversion != '\0') &&
             (strchr(allowed,
                     _conversion) != NULL) );
}

/**
 * @brief Walks a format over the parse's input.
 *
 * @param[in,out] _parse   the parse.
 * @param[in]     _format  the format.
 * @return true when the whole format was matched.
 *
 * White space in the format matches any run of white space in the input, or
 * none; any other character outside a conversion matches only itself.
 */
static bool
d_time_internal_walk(
    struct d_time_internal_parse* _parse,
    const char*                   _format
)
{
    const char* format = _format;

    // each element of the format, in order
    while (*format != '\0')
    {
        // white space, then an ordinary character, then a conversion
        if (d_time_internal_is_space(*format))
        {
            d_time_internal_skip_space(_parse);
        }
        else if (*format != '%')
        {
            // an ordinary character matches only itself
            if (*_parse->input != *format)
            {
                return false;
            }

            ++_parse->input;
        }
        else
        {
            ++format;

            // an E or O modifier POSIX allows reads as the plain conversion
            if ( ( (*format == 'E') ||
                   (*format == 'O') ) &&
                 (d_time_internal_modifies(format[0],
                                           format[1])) )
            {
                ++format;
            }

            // a conversion that does not read ends the parse
            if ( (*format == '\0') ||
                 (!d_time_internal_convert(_parse,
                                           *format)) )
            {
                return false;
            }
        }

        ++format;
    }

    return true;
}

/**
 * @brief Settles the day of the year from %U or %W and a weekday.
 *
 * @param[in,out] _parse  the parse, whose year, week and weekday are known.
 * @return false for a day the year does not have.
 *
 * Week 1 starts on the year's first Sunday (%U) or Monday (%W); the days
 * before it are week 0.
 */
static bool
d_time_internal_from_week(
    struct d_time_internal_parse* _parse
)
{
    struct tm* const tm      = _parse->tm;
    const int        year    = tm->tm_year + 1900;
    const int        january = d_time_internal_weekday(year,
                                                       0,
                                                       1);
    const int        first   = (_parse->monday) ? ((8 - january) % 7)
                                                : ((7 - january) % 7);
    const int        offset  = (_parse->monday) ? ((tm->tm_wday + 6) % 7)
                                                : tm->tm_wday;
    const int        yday    = first + ((_parse->week - 1) * 7) + offset;

    // a day outside the year
    if ( (yday < 0) ||
         (yday >= YEAR_DAYS[d_time_internal_leap(year)][12]) )
    {
        return false;
    }

    tm->tm_yday  = yday;
    _parse->yday = true;

    return true;
}

/**
 * @brief Settles the month and the day of the month from the day of the
 *        year.
 *
 * @param[in,out] _parse  the parse, whose year and day of the year are known.
 * @return false for a day the year does not have, or a month or day the
 *         input gave that the day of the year contradicts.
 */
static bool
d_time_internal_from_yday(
    struct d_time_internal_parse* _parse
)
{
    struct tm* const tm    = _parse->tm;
    const int        leap  = d_time_internal_leap(tm->tm_year + 1900);
    int              month = 0;

    // a day the year does not have
    if (tm->tm_yday >= YEAR_DAYS[leap][12])
    {
        return false;
    }

    // the month whose days hold it
    while (YEAR_DAYS[leap][month + 1] <= tm->tm_yday)
    {
        ++month;
    }

    const int mday = tm->tm_yday - YEAR_DAYS[leap][month] + 1;

    // what the input gave must agree
    if ( ( (_parse->month) &&
           (tm->tm_mon != month) ) ||
         ( (_parse->mday) &&
           (tm->tm_mday != mday) ) )
    {
        return false;
    }

    tm->tm_mon    = month;
    tm->tm_mday   = mday;
    _parse->month = true;
    _parse->mday  = true;

    return true;
}

/**
 * @brief Settles what the conversions read into the fields they imply.
 *
 * @param[in,out] _parse  the parse, its format walked.
 * @return false where the fields read contradict each other or name a day
 *         the year does not have.
 *
 * The year first: %Y's, else %C's century with %y's year in it, %y alone
 * the 2000s below 69 and the 1900s from it, %C alone its first year. Then
 * the hour of %I or %l, AM unless %p said PM. With the year known, a date
 * the input did not give whole comes from %j, or from %U or %W with a
 * weekday; and once the year, the month and the day are known, the day of
 * the year and of the week follow, where the input gave neither.
 */
static bool
d_time_internal_settle(
    struct d_time_internal_parse* _parse
)
{
    struct tm* const tm = _parse->tm;

    // a year from %C and %y, where %Y gave none
    if ( (!_parse->year) &&
         ( (_parse->century >= 0) ||
           (_parse->short_year >= 0) ) )
    {
        const int short_year = (_parse->short_year >= 0) ? _parse->short_year
                                                         : 0;
        const int century    = (_parse->century >= 0)
                                   ? _parse->century
                                   : ((short_year < 69) ? 20 : 19);

        tm->tm_year  = (century * 100) + short_year - 1900;
        _parse->year = true;
    }

    // the 12-hour clock, AM unless %p said PM
    if (_parse->hour12 >= 0)
    {
        tm->tm_hour = (_parse->hour12 % 12) + ((_parse->pm == 1) ? 12 : 0);
    }

    // with the year known, a week and a weekday give a date not given
    // whole its day of the year, and the day of the year gives the month
    // and the day, or must agree with those the input gave
    if ( (_parse->year) &&
         ( ( (!_parse->yday)                  &&
             (_parse->week >= 0)              &&
             (_parse->wday)                   &&
             ( (!_parse->month) ||
               (!_parse->mday) )              &&
             (!d_time_internal_from_week(_parse)) ) ||
           ( (_parse->yday) &&
             (!d_time_internal_from_yday(_parse)) ) ) )
    {
        return false;
    }

    // a whole date gives the day of the year and of the week
    if ( (_parse->year)  &&
         (_parse->month) &&
         (_parse->mday) )
    {
        const int year = tm->tm_year + 1900;

        tm->tm_yday = (_parse->yday)
                          ? tm->tm_yday
                          : (YEAR_DAYS[d_time_internal_leap(year)][tm->tm_mon] +
                             tm->tm_mday - 1);
        tm->tm_wday = (_parse->wday)
                          ? tm->tm_wday
                          : d_time_internal_weekday(year,
                                                    tm->tm_mon,
                                                    tm->tm_mday);
    }

    return true;
}

/*
d_strptime
  dtime's own parser, the same on every platform and C library (decision
120 of the register, as the owner ruled it on 2026.10.18): it never calls
the system's strptime. The format is walked once, each conversion reading
into _tm or, where its meaning waits on others, into the parse; a last
step settles the year, the hour, the date, and the days of the year and of
the week.
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

    struct d_time_internal_parse parse = { .input      = _s,
                                           .tm         = _tm,
                                           .century    = -1,
                                           .short_year = -1,
                                           .week       = -1,
                                           .hour12     = -1,
                                           .pm         = -1 };

    // the format, then what it leaves to settle
    if ( (!d_time_internal_walk(&parse,
                                _format)) ||
         (!d_time_internal_settle(&parse)) )
    {
        return NULL;
    }

    return (char*)parse.input;
}

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
