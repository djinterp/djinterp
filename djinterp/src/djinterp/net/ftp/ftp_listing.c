/*******************************************************************************
* djinterp [net]                                                   ftp_listing.c
*
* Implementation of the listing parsers declared in ftp_listing.h: detection,
* the line dispatcher, and the token scanning every dialect shares.
*   Each dialect has its own file -- ftp_listing_unix.c, ftp_listing_dos.c,
* and ftp_listing_mlsx.c -- reached through ftp_listing_internal.h.
*
*
* path:      /src/djinterp/net/ftp/ftp_listing.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memset
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
d_ftp_internal_skip_blanks
  The first position at or after `_position` holding no blank.
*/
size_t
d_ftp_internal_skip_blanks(
    struct d_ftp_span _line,
    size_t            _position
)
{
    size_t position = _position;

    // past the run of blanks
    while ( (position < _line.length) &&
            (d_ftp_internal_is_blank(_line.data[position])) )
    {
        position++;
    }

    return position;
}

/*
d_ftp_internal_token_end
  The first blank position at or after `_position`, or the end.
*/
size_t
d_ftp_internal_token_end(
    struct d_ftp_span _line,
    size_t            _position
)
{
    size_t position = _position;

    // past the run of non-blanks
    while ( (position < _line.length) &&
            (!d_ftp_internal_is_blank(_line.data[position])) )
    {
        position++;
    }

    return position;
}

/*
d_ftp_internal_span_at
  The span from `_start` to `_end` within a line.
*/
struct d_ftp_span
d_ftp_internal_span_at(
    struct d_ftp_span _line,
    size_t            _start,
    size_t            _end
)
{
    const struct d_ftp_span result = { _line.data + _start, _end - _start };

    return result;
}

/*
d_ftp_internal_read_clock
  Reads "H:MM" or "HH:MM" into the time of day; writes nothing on
failure.
*/
bool
d_ftp_internal_read_clock(
    struct d_ftp_span  _token,
    struct d_ftp_time* _time
)
{
    // four or five characters
    if ( (_token.length < 4u) ||
         (_token.length > 5u) )
    {
        return false;
    }

    const size_t            colon   = _token.length - 3u;
    const struct d_ftp_span hours   = { _token.data, colon };
    const struct d_ftp_span minutes = { _token.data + colon + 1u, 2u };

    // digits on either side of the colon
    if ( (_token.data[colon] != ':')           ||
         (!d_ftp_internal_is_number(hours))    ||
         (!d_ftp_internal_is_number(minutes)) )
    {
        return false;
    }

    const unsigned hour   = d_ftp_internal_digits_value(hours.data,
                                                        colon);
    const unsigned minute = d_ftp_internal_digits_value(minutes.data,
                                                        2u);

    // a real time of day
    if ( (hour > 23u) ||
         (minute > 59u) )
    {
        return false;
    }

    _time->hour   = (uint8_t)hour;
    _time->minute = (uint8_t)minute;

    return true;
}

/*
d_ftp_listing_detect
  Cheapest shape first: "total", a permission field, a DOS date, then facts.
*/
enum d_ftp_listing_format
d_ftp_listing_detect(
    const char* _line,
    size_t      _length
)
{
    // parameter validation
    if (!_line)
    {
        return D_FTP_LISTING_AUTO;
    }

    const struct d_ftp_span line  = d_ftp_internal_trim_eol(_line,
                                                            _length);
    const struct d_ftp_span first = d_ftp_internal_span_at(
                                        line,
                                        0u,
                                        d_ftp_internal_token_end(line,
                                                                 0u));
    enum d_ftp_entry_type   type  = D_FTP_ENTRY_UNKNOWN;
    uint32_t                mode  = 0;
    struct d_ftp_span       facts = { NULL, 0u };
    struct d_ftp_span       name  = { NULL, 0u };

    // "total N" heads a Unix listing
    if (d_ftp_internal_starts_nocase(line.data,
                                     line.length,
                                     "total "))
    {
        return D_FTP_LISTING_UNIX;
    }

    // a permission field opens a Unix line
    if (d_ftp_internal_unix_mode(first,
                                 &type,
                                 &mode))
    {
        return D_FTP_LISTING_UNIX;
    }

    // an MM-DD-YY date opens a DOS line
    if (d_ftp_internal_is_dos_date(first))
    {
        return D_FTP_LISTING_DOS;
    }

    // facts before the first space mark an MLSx line
    if (d_ftp_internal_mlsx_split(line,
                                  &facts,
                                  &name))
    {
        return D_FTP_LISTING_MLSX;
    }

    return D_FTP_LISTING_AUTO;
}

/*
d_ftp_listing_parse
  Clears the entry, trims only the line ending, and dispatches on the
dialect, detecting it first when asked to.
*/
enum d_ftp_line_result
d_ftp_listing_parse(
    const char*               _line,
    size_t                    _length,
    enum d_ftp_listing_format _format,
    const struct d_ftp_time*  _now,
    struct d_ftp_entry*       _out
)
{
    // parameter validation
    if ( (!_line) ||
         (!_out) )
    {
        return D_FTP_LINE_MALFORMED;
    }

    memset(_out,
           0,
           sizeof(*_out));

    const struct d_ftp_span line = d_ftp_internal_trim_eol(_line,
                                                           _length);

    // a blank line names nothing
    if (line.length == 0u)
    {
        return D_FTP_LINE_SKIP;
    }

    const enum d_ftp_listing_format format =
        (_format == D_FTP_LISTING_AUTO)
        ? d_ftp_listing_detect(line.data,
                               line.length)
        : _format;

    switch (format)
    {
        case D_FTP_LISTING_UNIX:
            return d_ftp_internal_parse_unix(line,
                                             _now,
                                             _out);

        case D_FTP_LISTING_DOS:
            return d_ftp_internal_parse_dos(line,
                                            _out);

        case D_FTP_LISTING_MLSX:
            return d_ftp_internal_parse_mlsx(line,
                                             _out);

        case D_FTP_LISTING_NAMES:
            _out->name = line;

            return D_FTP_LINE_ENTRY;

        case D_FTP_LISTING_AUTO:
        default:
            break;
    }

    return D_FTP_LINE_MALFORMED;
}
