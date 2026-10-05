/*******************************************************************************
* djinterp [net]                                               ftp_listing_dos.c
*
* Implementation of the DOS listing dialect: the lines IIS writes.
*   Date, time, "<DIR>" or a size, then the name; two-digit years pivot at
* 1970, as IIS writes them.
*
*
* path:      /src/djinterp/net/ftp/ftp_listing_dos.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint8_t, uint16_t, uint64_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "../../../../inc/djinterp/net/ftp/ftp_fact.h"    // d_ftp_time
#include "./ftp_internal.h"                               // text helpers
#include "./ftp_listing_internal.h"                       // listing helpers


//==============================================================================
// 2.  DIRECTORY LISTINGS
//==============================================================================

/*
d_ftp_internal_is_dos_date
  Reports whether a token is "MM-DD-YY" or "MM-DD-YYYY".
*/
bool
d_ftp_internal_is_dos_date(
    struct d_ftp_span _token
)
{
    // eight or ten characters
    if ( (_token.length != 8u) &&
         (_token.length != 10u) )
    {
        return false;
    }

    // dashes at 2 and 5, digits elsewhere
    for (size_t index = 0; index < _token.length; index++)
    {
        const bool dash = ( (index == 2u) ||
                            (index == 5u) );

        if ( ( (dash) &&
               (_token.data[index] != '-') ) ||
             ( (!dash) &&
               (!d_ftp_internal_is_digit(_token.data[index])) ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_ftp_internal_read_dos_clock
  File-local: reads "HH:MM" with an optional "AM" or "PM" suffix, converting
a 12-hour time (12, 1, ..., 11) to 24 hours.
*/
D_STATIC bool
d_ftp_internal_read_dos_clock(
    struct d_ftp_span  _token,
    struct d_ftp_time* _time
)
{
    struct d_ftp_span clock     = _token;
    bool              afternoon = false;
    bool              meridiem  = false;

    // a trailing AM or PM marks a 12-hour clock
    if (clock.length > 2u)
    {
        const char* const suffix  = clock.data + clock.length - 2u;
        const bool        morning = d_ftp_internal_equals_nocase(suffix,
                                                                 2u,
                                                                 "AM");

        afternoon = d_ftp_internal_equals_nocase(suffix,
                                                 2u,
                                                 "PM");
        meridiem  = ( (morning) ||
                      (afternoon) );

        // the suffix is not part of the clock
        if (meridiem)
        {
            clock.length -= 2u;
        }
    }

    // "HH:MM" underneath
    if (!d_ftp_internal_read_clock(clock,
                                   _time))
    {
        return false;
    }

    // a 12-hour clock runs 12, 1, ..., 11
    if (meridiem)
    {
        if ( (_time->hour < 1u) ||
             (_time->hour > 12u) )
        {
            return false;
        }

        _time->hour = (uint8_t)((_time->hour % 12u) +
                                ((afternoon) ? 12u : 0u));
    }

    return true;
}

/*
d_ftp_internal_read_grouped
  File-local: reads a size that may carry ',' digit grouping, as IIS writes
it, with overflow refused.
*/
D_STATIC bool
d_ftp_internal_read_grouped(
    struct d_ftp_span _token,
    uint64_t*         _out_value
)
{
    uint64_t value  = 0;
    size_t   digits = 0;

    // digits, with commas allowed between them
    for (size_t index = 0; index < _token.length; index++)
    {
        const char c = _token.data[index];

        // grouping commas carry no value
        if ( (c == ',') &&
             (digits > 0u) )
        {
            continue;
        }

        if (!d_ftp_internal_is_digit(c))
        {
            return false;
        }

        const uint64_t digit = (uint64_t)(c - '0');

        // the next step would overflow
        if (value > ((UINT64_MAX - digit) / 10u))
        {
            return false;
        }

        value = (value * 10u) + digit;
        digits++;
    }

    // at least one digit
    if (digits == 0u)
    {
        return false;
    }

    *_out_value = value;

    return true;
}

/*
d_ftp_internal_dos_date
  File-local: reads a date token d_ftp_internal_is_dos_date() accepted,
"MM-DD-YY" or "MM-DD-YYYY". Two-digit years pivot at 1970, as IIS writes
them.
*/
D_STATIC struct d_ftp_time
d_ftp_internal_dos_date(
    struct d_ftp_span _date
)
{
    const unsigned    year = d_ftp_internal_digits_value(_date.data + 6u,
                                                         _date.length - 6u);
    struct d_ftp_time time = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    time.month = (uint8_t)d_ftp_internal_digits_value(_date.data,
                                                      2u);
    time.day   = (uint8_t)d_ftp_internal_digits_value(_date.data + 3u,
                                                      2u);
    time.year  = (uint16_t)((_date.length == 10u) ? year
                            : (year < 70u)        ? (2000u + year)
                                                  : (1900u + year));

    return time;
}

/*
d_ftp_internal_dos_kind
  File-local: reads the column between the time and the name: "<DIR>",
another bracketed kind such as "<JUNCTION>", or a size, digit-grouped or
not.
*/
D_STATIC bool
d_ftp_internal_dos_kind(
    struct d_ftp_span   _kind,
    struct d_ftp_entry* _out
)
{
    uint64_t size = 0u;

    // a directory
    if (d_ftp_internal_equals_nocase(_kind.data,
                                     _kind.length,
                                     "<DIR>"))
    {
        _out->type = D_FTP_ENTRY_DIRECTORY;

        return true;
    }

    // another bracketed kind
    if ( (_kind.length > 0u) &&
         (_kind.data[0] == '<') )
    {
        _out->type = D_FTP_ENTRY_OTHER;

        return true;
    }

    // or else a file's size
    if (!d_ftp_internal_read_grouped(_kind,
                                     &size))
    {
        return false;
    }

    _out->type   = D_FTP_ENTRY_FILE;
    _out->size   = size;
    _out->known |= D_FTP_ENTRY_KNOWN_SIZE;

    return true;
}

/*
d_ftp_internal_parse_dos
  Reads an IIS line: date, time, "<DIR>" or a size, then the name, spaces
and all.
*/
enum d_ftp_line_result
d_ftp_internal_parse_dos(
    struct d_ftp_span   _line,
    struct d_ftp_entry* _out
)
{
    const size_t            date_end = d_ftp_internal_token_end(_line,
                                                                0u);
    const struct d_ftp_span date     = { _line.data, date_end };

    // the line opens with its date
    if (!d_ftp_internal_is_dos_date(date))
    {
        return D_FTP_LINE_MALFORMED;
    }

    struct d_ftp_time time        = d_ftp_internal_dos_date(date);
    const size_t      clock_start = d_ftp_internal_skip_blanks(_line,
                                                               date_end);
    const size_t      clock_end   = d_ftp_internal_token_end(_line,
                                                             clock_start);
    const size_t      kind_start  = d_ftp_internal_skip_blanks(_line,
                                                               clock_end);
    const size_t      kind_end    = d_ftp_internal_token_end(_line,
                                                             kind_start);
    const size_t      name_start  = d_ftp_internal_skip_blanks(_line,
                                                               kind_end);

    // the time of day, the kind, a name, and a date that exists
    if ( (!d_ftp_internal_read_dos_clock(
              d_ftp_internal_span_at(_line,
                                     clock_start,
                                     clock_end),
              &time))                                      ||
         (!d_ftp_internal_dos_kind(
              d_ftp_internal_span_at(_line,
                                     kind_start,
                                     kind_end),
              _out))                                       ||
         (name_start >= _line.length)                      ||
         (!d_ftp_time_is_valid(&time)) )
    {
        return D_FTP_LINE_MALFORMED;
    }

    _out->name      = d_ftp_internal_span_at(_line,
                                             name_start,
                                             _line.length);
    _out->modified  = time;
    _out->known    |= D_FTP_ENTRY_KNOWN_MODIFIED;
    _out->known    |= D_FTP_ENTRY_KNOWN_TIME;

    return D_FTP_LINE_ENTRY;
}
