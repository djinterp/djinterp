#include "..\..\inc\c\dtime.h"


///////////////////////////////////////////////////////////////////////////////
///             III.  THREAD-SAFE TIME CONVERSION                           ///
///////////////////////////////////////////////////////////////////////////////

/*
d_localtime
  Thread-safe local time conversion. Converts a time_t value to a broken-down
local time representation.

Parameter(s):
  _timer:  pointer to the time_t value to convert.
  _result: pointer to struct tm to store the result.
Return:
  A pointer to _result on success, or NULL on failure.
*/
struct tm*
d_localtime
(
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

#if defined(D_TIME_PLATFORM_WINDOWS)
    // Windows uses localtime_s with reversed parameter order
    if (localtime_s(_result, _timer) != 0)
    {
        return NULL;
    }
    
    return _result;

#elif defined(D_TIME_PLATFORM_POSIX)
    // POSIX localtime_r
    return localtime_r(_timer, _result);

#else
    // fallback: use non-thread-safe localtime and copy
    struct tm* temp;
    
    temp = localtime(_timer);
    if (!temp)
    {
        return NULL;
    }

    d_memcpy(_result, temp, sizeof(struct tm));
    
    return _result;
#endif
}


/*
d_gmtime
  Thread-safe UTC time conversion. Converts a time_t value to a broken-down
UTC time representation.

Parameter(s):
  _timer:  pointer to the time_t value to convert.
  _result: pointer to struct tm to store the result.
Return:
  A pointer to _result on success, or NULL on failure.
*/
struct tm*
d_gmtime
(
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

#if defined(D_TIME_PLATFORM_WINDOWS)
    // Windows uses gmtime_s with reversed parameter order
    if (gmtime_s(_result, _timer) != 0)
    {
        return NULL;
    }
    
    return _result;

#elif defined(D_TIME_PLATFORM_POSIX)
    // POSIX gmtime_r
    return gmtime_r(_timer, _result);

#else
    // fallback: use non-thread-safe gmtime and copy
    struct tm* temp;
    
    temp = gmtime(_timer);
    if (!temp)
    {
        return NULL;
    }

    d_memcpy(_result, temp, sizeof(struct tm));
    
    return _result;
#endif
}


/*
d_ctime
  Thread-safe time-to-string conversion. Converts time_t to a string
representation of local time.

Parameter(s):
  _timer: pointer to the time_t value to convert.
  _buf:   buffer to store the result (must be at least 26 bytes).
Return:
  A pointer to _buf on success, or NULL on failure.
*/
char*
d_ctime
(
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

#if defined(D_TIME_PLATFORM_WINDOWS)
    // Windows ctime_s requires buffer size (26 bytes for ctime format)
    if (ctime_s(_buf, 26, _timer) != 0)
    {
        return NULL;
    }
    
    return _buf;

#elif defined(D_TIME_PLATFORM_POSIX)
    // POSIX ctime_r
    return ctime_r(_timer, _buf);

#else
    // fallback: use non-thread-safe ctime and copy
    const char* temp;
    
    temp = ctime(_timer);
    if (!temp)
    {
        return NULL;
    }
    
    // ctime output is always 26 bytes including null terminator
    d_memcpy(_buf, temp, 26);
    
    return _buf;
#endif
}


/*
d_asctime
  Thread-safe tm-to-string conversion. Converts struct tm to a string
representation.

Parameter(s):
  _tm:  pointer to the struct tm to convert.
  _buf: buffer to store the result (must be at least 26 bytes).
Return:
  A pointer to _buf on success, or NULL on failure.
*/
char*
d_asctime
(
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

#if defined(D_TIME_PLATFORM_WINDOWS)
    // Windows asctime_s requires buffer size
    if (asctime_s(_buf, 26, _tm) != 0)
    {
        return NULL;
    }
    
    return _buf;

#elif defined(D_TIME_PLATFORM_POSIX)
    // POSIX asctime_r
    return asctime_r(_tm, _buf);

#else
    // fallback: use non-thread-safe asctime and copy
    const char* temp;
    
    temp = asctime(_tm);
    if (!temp)
    {
        return NULL;
    }
    
    d_memcpy(_buf, temp, 26);
    
    return _buf;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   HIGH-RESOLUTION TIME                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_clock_gettime
  Get high-resolution time from the specified clock.

Parameter(s):
  _clock_id: the clock to query (CLOCK_REALTIME, CLOCK_MONOTONIC, etc.).
  _tp:       pointer to struct timespec to store the result.
Return:
  0 on success, -1 on failure (with errno set).
*/
int
d_clock_gettime
(
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

#if D_TIME_HAS_CLOCK_GETTIME && defined(D_TIME_PLATFORM_POSIX)
    // use native clock_gettime
    return clock_gettime(_clock_id, _tp);

#elif defined(D_TIME_PLATFORM_WINDOWS)
    // Windows implementation using QueryPerformanceCounter
    static LARGE_INTEGER frequency;
    static int           frequency_initialized;
    LARGE_INTEGER        counter;
    FILETIME             ft;
    ULARGE_INTEGER       uli;

    frequency_initialized = 0;

    if (_clock_id == CLOCK_MONOTONIC)
    {
        // use QueryPerformanceCounter for monotonic time
        if (!frequency_initialized)
        {
            QueryPerformanceFrequency(&frequency);
            frequency_initialized = 1;
        }

        QueryPerformanceCounter(&counter);
        
        _tp->tv_sec  = (time_t)(counter.QuadPart / frequency.QuadPart);
        _tp->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) * 
                               D_TIME_NSEC_PER_SEC) / frequency.QuadPart);

        return 0;
    }
    else if (_clock_id == CLOCK_REALTIME)
    {
        // use GetSystemTimePreciseAsFileTime for wall-clock time
        #if (_WIN32_WINNT >= 0x0602)
            GetSystemTimePreciseAsFileTime(&ft);
        #else
            GetSystemTimeAsFileTime(&ft);
        #endif

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
        FILETIME creation_time;
        FILETIME exit_time;
        FILETIME kernel_time;
        FILETIME user_time;
        BOOL     success;

        // get process or thread times
        if (_clock_id == CLOCK_PROCESS_CPUTIME_ID)
        {
            success = GetProcessTimes(GetCurrentProcess(),
                                       &creation_time,
                                       &exit_time, 
                                       &kernel_time,
                                       &user_time);
        }
        else
        {
            success = GetThreadTimes(GetCurrentThread(),
                                      &creation_time,
                                      &exit_time, 
                                      &kernel_time,
                                      &user_time);
        }

        if (!success)
        {
            errno = EINVAL;

            return -1;
        }

        // combine kernel and user time
        uli.LowPart  = user_time.dwLowDateTime + kernel_time.dwLowDateTime;
        uli.HighPart = user_time.dwHighDateTime + kernel_time.dwHighDateTime;

        _tp->tv_sec  = (time_t)(uli.QuadPart / 10000000ULL);
        _tp->tv_nsec = (long)((uli.QuadPart % 10000000ULL) * 100);

        return 0;
    }

    errno = EINVAL;

    return -1;

#else
    // minimal fallback using time()
    if (_clock_id == CLOCK_REALTIME)
    {
        _tp->tv_sec  = time(NULL);
        _tp->tv_nsec = 0;

        return 0;
    }

    errno = EINVAL;

    return -1;
#endif
}


/*
d_clock_getres
  Get the resolution of the specified clock.

Parameter(s):
  _clock_id: the clock to query.
  _res:      pointer to struct timespec to store the resolution.
Return:
  0 on success, -1 on failure (with errno set).
*/
int
d_clock_getres
(
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

#if D_TIME_HAS_CLOCK_GETTIME && defined(D_TIME_PLATFORM_POSIX)
    // use native clock_getres
    return clock_getres(_clock_id, _res);

#elif defined(D_TIME_PLATFORM_WINDOWS)
    // Windows implementation
    static LARGE_INTEGER frequency;
    static int           frequency_initialized;

    frequency_initialized = 0;

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

#else
    // fallback: 1 second resolution
    if (_clock_id == CLOCK_REALTIME)
    {
        _res->tv_sec  = 1;
        _res->tv_nsec = 0;

        return 0;
    }

    errno = EINVAL;

    return -1;
#endif
}


/*
d_timespec_get
  C11-style timespec retrieval.

Parameter(s):
  _ts:   pointer to struct timespec to store the result.
  _base: time base (TIME_UTC is the only required base).
Return:
  The base value on success, 0 on failure.
*/
int
d_timespec_get
(
    struct timespec* _ts, 
    int              _base
)
{
    // parameter validation
    if (!_ts)
    {
        return 0;
    }

#if D_TIME_HAS_TIMESPEC_GET
    // use native timespec_get
    return timespec_get(_ts, _base);

#else
    // fallback implementation
    if (_base == TIME_UTC)
    {
        if (d_clock_gettime(CLOCK_REALTIME, _ts) == 0)
        {
            return _base;
        }
    }

    return 0;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             V.    SLEEP FUNCTIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_nanosleep
  High-resolution sleep function.

Parameter(s):
  _req: pointer to struct timespec specifying the sleep duration.
  _rem: pointer to struct timespec to store remaining time if interrupted
        (may be NULL).
Return:
  0 on success, -1 on failure or interruption (with errno set).
*/
int
d_nanosleep
(
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

#if D_TIME_HAS_NANOSLEEP && defined(D_TIME_PLATFORM_POSIX)
    // use native nanosleep
    return nanosleep(_req, _rem);

#elif defined(D_TIME_PLATFORM_WINDOWS)
    // Windows implementation using Sleep
    DWORD           milliseconds;
    LARGE_INTEGER   start;
    LARGE_INTEGER   end;
    LARGE_INTEGER   frequency;
    int64_t         elapsed_ns;
    int64_t         requested_ns;

    // convert to milliseconds (Sleep only has ms resolution)
    milliseconds = (DWORD)((_req->tv_sec * D_TIME_MSEC_PER_SEC) + 
                           (_req->tv_nsec / D_TIME_NSEC_PER_MSEC));

    // handle sub-millisecond requests
    if ( (milliseconds == 0) && 
         (_req->tv_sec == 0) && 
         (_req->tv_nsec > 0) )
    {
        milliseconds = 1;
    }

    if (_rem)
    {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
    }

    Sleep(milliseconds);

    if (_rem)
    {
        QueryPerformanceCounter(&end);
        
        elapsed_ns = ((end.QuadPart - start.QuadPart) * D_TIME_NSEC_PER_SEC) / 
                     frequency.QuadPart;
        requested_ns = (_req->tv_sec * D_TIME_NSEC_PER_SEC) + _req->tv_nsec;
        
        if (elapsed_ns < requested_ns)
        {
            int64_t remaining = requested_ns - elapsed_ns;

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

#else
    // minimal fallback using sleep()
    unsigned int seconds;

    seconds = (unsigned int)_req->tv_sec;
    if (_req->tv_nsec > 0)
    {
        seconds += 1;  // round up
    }

    if (seconds > 0)
    {
        sleep(seconds);
    }

    if (_rem)
    {
        _rem->tv_sec  = 0;
        _rem->tv_nsec = 0;
    }

    return 0;
#endif
}


/*
d_usleep
  Microsecond sleep function.

Parameter(s):
  _usec: microseconds to sleep.
Return:
  0 on success, -1 on failure.
*/
int
d_usleep
(
    unsigned int _usec
)
{
    struct timespec ts;

    ts.tv_sec  = _usec / D_TIME_USEC_PER_SEC;
    ts.tv_nsec = (_usec % D_TIME_USEC_PER_SEC) * D_TIME_NSEC_PER_USEC;

    return d_nanosleep(&ts, NULL);
}


/*
d_sleep_ms
  Millisecond sleep function (convenience wrapper).

Parameter(s):
  _milliseconds: milliseconds to sleep.
Return:
  0 on success, -1 on failure.
*/
int
d_sleep_ms
(
    unsigned long _milliseconds
)
{
    struct timespec ts;

    ts.tv_sec  = _milliseconds / D_TIME_MSEC_PER_SEC;
    ts.tv_nsec = (_milliseconds % D_TIME_MSEC_PER_SEC) * D_TIME_NSEC_PER_MSEC;

    return d_nanosleep(&ts, NULL);
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   TIMEZONE UTILITIES                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
d_timegm
  Inverse of gmtime: convert struct tm (interpreted as UTC) to time_t.

Parameter(s):
  _tm: pointer to struct tm in UTC.
Return:
  The corresponding time_t value, or -1 on failure.
*/
time_t
d_timegm
(
    struct tm* _tm
)
{
    // parameter validation
    if (!_tm)
    {
        return (time_t)-1;
    }

#if D_TIME_HAS_TIMEGM
    // use native timegm
    return timegm(_tm);

#elif defined(D_TIME_PLATFORM_WINDOWS)
    // use Windows _mkgmtime
    return _mkgmtime(_tm);

#else
    // portable fallback implementation
    // this algorithm is based on the public domain implementation
    time_t result;
    int    year;
    int    month;
    int    day;
    int    i;

    // days in each month (non-leap year)
    static const int days_in_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    year  = _tm->tm_year + 1900;
    month = _tm->tm_mon;
    day   = _tm->tm_mday;

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
    result = 0;
    
    // years since 1970
    for (i = 1970; i < year; i++)
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
    for (i = year; i < 1970; i++)
    {
        result -= 365;
        if ( ((i % 4 == 0) && (i % 100 != 0)) || 
             (i % 400 == 0) )
        {
            result -= 1;
        }
    }

    // add days for each month
    for (i = 0; i < month; i++)
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

    return result;
#endif
}


/*
d_tzset
  Initialize timezone information from environment.

Parameter(s):
  (none)
Return:
  (none)
*/
void
d_tzset
(
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


///////////////////////////////////////////////////////////////////////////////
///             VII.  STRING PARSING AND FORMATTING                         ///
///////////////////////////////////////////////////////////////////////////////

/*
d_strptime
  Parse a time string according to a format string.

Parameter(s):
  _s:      the string to parse.
  _format: format string (similar to strftime format).
  _tm:     pointer to struct tm to store the result.
Return:
  Pointer to the first character not processed, or NULL on error.
*/
char*
d_strptime
(
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

#if D_TIME_HAS_STRPTIME
    // use native strptime
    return strptime(_s, _format, _tm);

#else
    // Windows/fallback implementation
    // supports common format specifiers including month and weekday names
    const char* sp;
    const char* fp;
    int         value;
    int         digits;
    int         i;
    int         matched;

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

    sp = _s;
    fp = _format;

    // initialize tm to zeros
    d_memset(_tm, 0, sizeof(struct tm));

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

            value  = 0;
            digits = 0;

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

                    for (i = 0; i < 12; i++)
                    {
                        size_t len = strlen(month_abbrev[i]);

                        if (d_strncasecmp(sp, month_abbrev[i], len) == 0)
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
                        for (i = 0; i < 12; i++)
                        {
                            size_t len = strlen(month_full[i]);

                            if (d_strncasecmp(sp, month_full[i], len) == 0)
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

                    for (i = 0; i < 12; i++)
                    {
                        size_t len = strlen(month_full[i]);

                        if (d_strncasecmp(sp, month_full[i], len) == 0)
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
                        for (i = 0; i < 12; i++)
                        {
                            size_t len = strlen(month_abbrev[i]);

                            if (d_strncasecmp(sp, month_abbrev[i], len) == 0)
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

                    for (i = 0; i < 7; i++)
                    {
                        size_t len = strlen(weekday_abbrev[i]);

                        if (d_strncasecmp(sp, weekday_abbrev[i], len) == 0)
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
                        for (i = 0; i < 7; i++)
                        {
                            size_t len = strlen(weekday_full[i]);

                            if (d_strncasecmp(sp, weekday_full[i], len) == 0)
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

                    for (i = 0; i < 7; i++)
                    {
                        size_t len = strlen(weekday_full[i]);

                        if (d_strncasecmp(sp, weekday_full[i], len) == 0)
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
                        for (i = 0; i < 7; i++)
                        {
                            size_t len = strlen(weekday_abbrev[i]);

                            if (d_strncasecmp(sp, weekday_abbrev[i], len) == 0)
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
#endif
}


/*
d_strftime_s
  Safe time formatting with bounds checking.

Parameter(s):
  _s:       buffer to store the formatted string.
  _maxsize: size of the buffer.
  _format:  format string.
  _tm:      pointer to struct tm with time values.
Return:
  The number of characters written (excluding null terminator), or 0 on error.
*/
int
d_strftime_s
(
    char*            _s, 
    size_t           _maxsize, 
    const char*      _format, 
    const struct tm* _tm
)
{
    size_t result;

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

#if D_ENV_CRT_MSVC
    result = strftime_s(_s, _maxsize, _format, _tm);
#else
    result = strftime(_s, _maxsize, _format, _tm);
#endif

    return (int)result;
}


///////////////////////////////////////////////////////////////////////////////
///             VIII. TIME ARITHMETIC                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_timespec_add
  Add two timespec values.

Parameter(s):
  _a:      first timespec.
  _b:      second timespec.
  _result: pointer to store the sum.
Return:
  (none)
*/
void
d_timespec_add
(
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
  Subtract two timespec values (_a - _b).

Parameter(s):
  _a:      first timespec (minuend).
  _b:      second timespec (subtrahend).
  _result: pointer to store the difference.
Return:
  (none)
*/
void
d_timespec_sub
(
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
  Compare two timespec values.

Parameter(s):
  _a: first timespec.
  _b: second timespec.
Return:
  Negative value if _a < _b, zero if _a == _b, positive if _a > _b.
*/
int
d_timespec_cmp
(
    const struct timespec* _a, 
    const struct timespec* _b
)
{
    // parameter validation - treat NULL as zero
    if ( (!_a) && 
         (!_b) )
    {
        return 0;
    }

    if ( (!_a) ||
         (!_b) )
    {
        return -1;
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
  Convert timespec to milliseconds.

Parameter(s):
  _ts: pointer to timespec.
Return:
  Total time in milliseconds.
*/
int64_t
d_timespec_to_ms
(
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
  Convert timespec to microseconds.

Parameter(s):
  _ts: pointer to timespec.
Return:
  Total time in microseconds.
*/
int64_t
d_timespec_to_us
(
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
  Convert timespec to nanoseconds.

Parameter(s):
  _ts: pointer to timespec.
Return:
  Total time in nanoseconds.
*/
int64_t
d_timespec_to_ns
(
    const struct timespec* _ts
)
{
    if (!_ts)
    {
        return 0;
    }

    return ((int64_t)_ts->tv_sec * D_TIME_NSEC_PER_SEC) + _ts->tv_nsec;
}

/*
d_ms_to_timespec
  Convert milliseconds to timespec.

Parameter(s):
  _milliseconds: time in milliseconds.
  _ts:           pointer to timespec to store the result.
Return:
  (none)
*/
void
d_ms_to_timespec
(
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
  Convert microseconds to timespec.

Parameter(s):
  _microseconds: time in microseconds.
  _ts:           pointer to timespec to store the result.
Return:
  (none)
*/
void
d_us_to_timespec
(
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
  Convert nanoseconds to timespec.

Parameter(s):
  _nanoseconds: time in nanoseconds.
  _ts:          pointer to timespec to store the result.
Return:
  (none)
*/
void
d_ns_to_timespec
(
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

///////////////////////////////////////////////////////////////////////////////
///             IX.   MONOTONIC TIME UTILITIES                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_monotonic_time_ms
  Get monotonic (non-decreasing) time in milliseconds.

Parameter(s):
  (none)
Return:
  Monotonic time in milliseconds.
*/
int64_t
d_monotonic_time_ms
(
    void
)
{
    struct timespec ts;

    if (d_clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        return d_timespec_to_ms(&ts);
    }

    // fallback to realtime if monotonic not available
    if (d_clock_gettime(CLOCK_REALTIME, &ts) == 0)
    {
        return d_timespec_to_ms(&ts);
    }

    return 0;
}


/*
d_monotonic_time_us
  Get monotonic time in microseconds.

Parameter(s):
  (none)
Return:
  Monotonic time in microseconds.
*/
int64_t
d_monotonic_time_us
(
    void
)
{
    struct timespec ts;

    if (d_clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        return d_timespec_to_us(&ts);
    }

    if (d_clock_gettime(CLOCK_REALTIME, &ts) == 0)
    {
        return d_timespec_to_us(&ts);
    }

    return 0;
}


/*
d_monotonic_time_ns
  Get monotonic time in nanoseconds.

Parameter(s):
  (none)
Return:
  Monotonic time in nanoseconds.
*/
int64_t
d_monotonic_time_ns
(
    void
)
{
    struct timespec ts;

    if (d_clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        return d_timespec_to_ns(&ts);
    }

    if (d_clock_gettime(CLOCK_REALTIME, &ts) == 0)
    {
        return d_timespec_to_ns(&ts);
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///             X.    TIMESPEC NORMALIZATION                                ///
///////////////////////////////////////////////////////////////////////////////

/*
d_timespec_normalize
  Normalize a timespec so that tv_nsec is in [0, 999999999].

Parameter(s):
  _ts: pointer to timespec to normalize (modified in place).
Return:
  (none)
*/
void
d_timespec_normalize
(
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
  Check if a timespec has valid values.

Parameter(s):
  _ts: pointer to timespec to check.
Return:
  Non-zero if valid, zero if invalid.
*/
int
d_timespec_is_valid
(
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

