/*******************************************************************************
* djinterp [net]                                                      ftp_fact.c
*
* Implementation of the file facts declared in ftp_fact.h.
*   Calendar arithmetic follows Howard Hinnant's civil-date algorithms, so the
* conversions are exact across the whole four-digit-year range.
*
*
* path:      /src/djinterp/net/ftp/ftp_fact.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_fact.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t, uint8_t, uint16_t, uint64_t, UINT64_MAX
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SECONDS_PER_DAY
//   constant: seconds in a day, for the Unix-time conversions.
static const int64_t SECONDS_PER_DAY = 86400;

//==============================================================================
// 3.  TIMESTAMPS AND SIZES
//==============================================================================

/*
d_ftp_internal_is_leap_year
  File-local: the proleptic Gregorian leap-year rule.
*/
D_STATIC bool
d_ftp_internal_is_leap_year(
    unsigned _year
)
{
    return ( ( ((_year % 4u) == 0u) &&
               ((_year % 100u) != 0u) ) ||
             ((_year % 400u) == 0u) );
}

/*
d_ftp_internal_days_in_month
  File-local: the length of a month; `_month` must already be 1 through 12.
*/
D_STATIC unsigned
d_ftp_internal_days_in_month(
    unsigned _year,
    unsigned _month
)
{
    static const unsigned char DAYS[12] =
    {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    // February gains a day in leap years
    if ( (_month == 2u) &&
         (d_ftp_internal_is_leap_year(_year)) )
    {
        return 29u;
    }

    return DAYS[_month - 1u];
}

/*
d_ftp_time_is_valid
  The month is checked before the day, whose range depends on it.
*/
bool
d_ftp_time_is_valid(
    const struct d_ftp_time* _time
)
{
    // four-digit years and real months only
    if ( (!_time)              ||
         (_time->year == 0u)   ||
         (_time->year > 9999u) ||
         (_time->month == 0u)  ||
         (_time->month > 12u) )
    {
        return false;
    }

    const unsigned days = d_ftp_internal_days_in_month(_time->year,
                                                       _time->month);

    return ( (_time->day >= 1u)           &&
             (_time->day <= days)         &&
             (_time->hour <= 23u)         &&
             (_time->minute <= 59u)       &&
             (_time->second <= 60u)       &&
             (_time->millisecond <= 999u) );
}

/*
d_ftp_time_parse
  Fixed positions for the fourteen digits; a fraction is read to three
digits and scaled, so ".5" is 500 milliseconds and ".123456" is 123.
*/
enum d_ftp_error
d_ftp_time_parse(
    const char*        _text,
    size_t             _length,
    struct d_ftp_time* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_ftp_span text = d_ftp_internal_trim(_text,
                                                       _length);

    // fourteen characters at least
    if (text.length < 14u)
    {
        return D_FTP_ERROR_MALFORMED;
    }

    const struct d_ftp_span stamp = { text.data, 14u };

    // and those fourteen all digits
    if (!d_ftp_internal_is_number(stamp))
    {
        return D_FTP_ERROR_MALFORMED;
    }

    struct d_ftp_time time = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    time.year   = (uint16_t)d_ftp_internal_digits_value(text.data,
                                                        4u);
    time.month  = (uint8_t)d_ftp_internal_digits_value(text.data + 4u,
                                                       2u);
    time.day    = (uint8_t)d_ftp_internal_digits_value(text.data + 6u,
                                                       2u);
    time.hour   = (uint8_t)d_ftp_internal_digits_value(text.data + 8u,
                                                       2u);
    time.minute = (uint8_t)d_ftp_internal_digits_value(text.data + 10u,
                                                       2u);
    time.second = (uint8_t)d_ftp_internal_digits_value(text.data + 12u,
                                                       2u);

    // an optional fraction: a dot and at least one digit
    if (text.length > 14u)
    {
        const struct d_ftp_span fraction = { text.data + 15u,
                                             text.length - 15u };

        if ( (text.data[14] != '.') ||
             (!d_ftp_internal_is_number(fraction)) )
        {
            return D_FTP_ERROR_MALFORMED;
        }

        const size_t used  = (fraction.length < 3u) ? fraction.length : 3u;
        unsigned     value = d_ftp_internal_digits_value(fraction.data,
                                                         used);

        // scale a one- or two-digit fraction up to milliseconds
        for (size_t index = used; index < 3u; index++)
        {
            value *= 10u;
        }

        time.millisecond = (uint16_t)value;
    }

    // the calendar decides the rest
    if (!d_ftp_time_is_valid(&time))
    {
        return D_FTP_ERROR_MALFORMED;
    }

    *_out = time;

    return D_FTP_OK;
}

/*
d_ftp_time_format
  Zero-padded fields in scratch storage sized by D_FTP_TIME_SIZE.
*/
enum d_ftp_error
d_ftp_time_format(
    const struct d_ftp_time* _time,
    bool                     _with_millisecond,
    struct d_ftp_buffer*     _out
)
{
    // parameter validation
    if ( (!d_ftp_time_is_valid(_time)) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    static const size_t WIDTHS[6] = { 4u, 2u, 2u, 2u, 2u, 2u };
    const unsigned      fields[6] =
    {
        _time->year,
        _time->month,
        _time->day,
        _time->hour,
        _time->minute,
        _time->second
    };
    char                text[D_FTP_TIME_SIZE] = { 0 };
    struct d_ftp_buffer local = { text, sizeof(text), 0u };

    // YYYYMMDDHHMMSS
    for (size_t index = 0; index < 6u; index++)
    {
        d_ftp_internal_append_uint(&local,
                                   fields[index],
                                   WIDTHS[index]);
    }

    // then ".sss" on request
    if (_with_millisecond)
    {
        d_ftp_internal_append_char(&local,
                                   '.');
        d_ftp_internal_append_uint(&local,
                                   _time->millisecond,
                                   3u);
    }

    return d_ftp_internal_commit(_out,
                                 &local);
}

/*
d_ftp_time_to_unix
  Days by d_ftp_internal_days_from_civil(), then the time of day; a leap
second counts as the first second of the next minute.
*/
enum d_ftp_error
d_ftp_time_to_unix(
    const struct d_ftp_time* _time,
    int64_t*                 _out_seconds
)
{
    // parameter validation
    if ( (!d_ftp_time_is_valid(_time)) ||
         (!_out_seconds) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const int64_t days = d_ftp_internal_days_from_civil(
                             (int64_t)_time->year,
                             _time->month,
                             _time->day);

    *_out_seconds = (days * SECONDS_PER_DAY)          +
                    ((int64_t)_time->hour * 3600)     +
                    ((int64_t)_time->minute * 60)     +
                    (int64_t)_time->second;

    return D_FTP_OK;
}

/*
d_ftp_time_from_unix
  Floor division, so a moment before 1970 lands on the right day with a
non-negative time of day.
*/
enum d_ftp_error
d_ftp_time_from_unix(
    int64_t            _seconds,
    struct d_ftp_time* _out
)
{
    // parameter validation
    if (!_out)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    int64_t days = _seconds / SECONDS_PER_DAY;
    int64_t rest = _seconds % SECONDS_PER_DAY;

    // C truncates toward zero; floor instead
    if (rest < 0)
    {
        rest += SECONDS_PER_DAY;
        days--;
    }

    int64_t  year  = 0;
    unsigned month = 0;
    unsigned day   = 0;

    d_ftp_internal_civil_from_days(days,
                                   &year,
                                   &month,
                                   &day);

    // d_ftp_time holds four-digit years only
    if ( (year < 1) ||
         (year > 9999) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    _out->year        = (uint16_t)year;
    _out->month       = (uint8_t)month;
    _out->day         = (uint8_t)day;
    _out->hour        = (uint8_t)(rest / 3600);
    _out->minute      = (uint8_t)((rest % 3600) / 60);
    _out->second      = (uint8_t)(rest % 60);
    _out->millisecond = 0u;

    return D_FTP_OK;
}

/*
d_ftp_parse_size
  The full uint64_t range, with overflow refused rather than wrapped.
*/
enum d_ftp_error
d_ftp_parse_size(
    const char* _text,
    size_t      _length,
    uint64_t*   _out_size
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out_size) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_ftp_span text   = d_ftp_internal_trim(_text,
                                                         _length);
    uint64_t                value  = 0;
    const bool              parsed = d_ftp_internal_parse_uint(text.data,
                                                               text.length,
                                                               UINT64_MAX,
                                                               &value);

    // digits only, and in range
    if (!parsed)
    {
        return D_FTP_ERROR_MALFORMED;
    }

    *_out_size = value;

    return D_FTP_OK;
}
