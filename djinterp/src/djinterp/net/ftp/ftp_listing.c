/*******************************************************************************
* djinterp [net]                                                   ftp_listing.c
*
* Implementation of the listing parsers declared in ftp_listing.h.
*   Unix lines are read by collecting fields until one begins a date, so the
* variable middle columns never need counting; everything after the date is
* the name, spaces and all.
*
*
* path:      /src/djinterp/net/ftp/ftp_listing.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // int64_t, uint8_t, uint16_t, uint32_t, uint64_t
#include <string.h>   // memchr, memcmp, memset, strchr
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "../../../../inc/djinterp/net/ftp/ftp_fact.h"    // d_ftp_time
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// 2.  DIRECTORY LISTINGS
//==============================================================================

/*
d_ftp_internal_skip_blanks
  File-local: the first position at or after `_position` holding no blank.
*/
D_STATIC size_t
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
  File-local: the first blank position at or after `_position`, or the end.
*/
D_STATIC size_t
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
  File-local: the span from `_start` to `_end` within a line.
*/
D_STATIC struct d_ftp_span
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
d_ftp_internal_month
  File-local: the month (1-12) a three-letter English abbreviation names,
case-insensitively, or 0.
*/
D_STATIC unsigned
d_ftp_internal_month(
    struct d_ftp_span _token
)
{
    static const char* const MONTHS[12] =
    {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };

    // each abbreviation in turn
    for (size_t index = 0; index < 12u; index++)
    {
        if (d_ftp_internal_equals_nocase(_token.data,
                                         _token.length,
                                         MONTHS[index]))
        {
            return (unsigned)(index + 1u);
        }
    }

    return 0u;
}

/*
d_ftp_internal_unix_mode
  File-local: reads an "ls -l" permission field -- "drwxr-sr-x" and kin, with
an optional eleventh '+', '.', or '@' for ACLs, security contexts, or extended
attributes -- into an entry type and mode bits. The execute positions also
carry setuid and setgid ('s', or 'S' without execute; 'l' is setgid as
mandatory locking) and sticky ('t', or 'T' without execute).
*/
D_STATIC bool
d_ftp_internal_unix_mode(
    struct d_ftp_span      _field,
    enum d_ftp_entry_type* _out_type,
    uint32_t*              _out_mode
)
{
    static const char* const ALLOWED[9] =
    {
        "r-", "w-", "xsS-",
        "r-", "w-", "xsSl-",
        "r-", "w-", "xtT-"
    };
    static const uint32_t PERMISSION[9] =
    {
        0400u, 0200u, 0100u,
        0040u, 0020u, 0010u,
        0004u, 0002u, 0001u
    };
    static const uint32_t SPECIAL[3] = { 04000u, 02000u, 01000u };

    // ten characters, or eleven with a marker
    if ( (_field.length < 10u) ||
         (_field.length > 11u) )
    {
        return false;
    }

    // the only markers
    if ( (_field.length == 11u)     &&
         (_field.data[10] != '+')   &&
         (_field.data[10] != '.')   &&
         (_field.data[10] != '@') )
    {
        return false;
    }

    enum d_ftp_entry_type type = D_FTP_ENTRY_OTHER;

    // the file-type character
    switch (_field.data[0])
    {
        case '-':
            type = D_FTP_ENTRY_FILE;

            break;

        case 'd':
            type = D_FTP_ENTRY_DIRECTORY;

            break;

        case 'l':
            type = D_FTP_ENTRY_SYMLINK;

            break;

        case 'b':
        case 'c':
        case 'p':
        case 's':
        case 'D':
            type = D_FTP_ENTRY_OTHER;

            break;

        default:
            return false;
    }

    uint32_t mode = 0;

    // nine permission characters, three per class
    for (size_t index = 0; index < 9u; index++)
    {
        const char        c       = _field.data[index + 1u];
        const char* const allowed = strchr(ALLOWED[index],
                                           c);

        // each position has its own alphabet; strchr() also finds the NUL
        if ( (c == '\0') ||
             (!allowed) )
        {
            return false;
        }

        // r, w, x, s, and t grant the permission itself
        if ( (c == 'r') ||
             (c == 'w') ||
             (c == 'x') ||
             (c == 's') ||
             (c == 't') )
        {
            mode |= PERMISSION[index];
        }

        // s, S, t, T, and l set their class's special bit
        if ( (c == 's') ||
             (c == 'S') ||
             (c == 't') ||
             (c == 'T') ||
             (c == 'l') )
        {
            mode |= SPECIAL[index / 3u];
        }
    }

    *_out_type = type;
    *_out_mode = mode;

    return true;
}

/*
d_ftp_internal_read_clock
  File-local: reads "H:MM" or "HH:MM" into the time of day; writes nothing on
failure.
*/
D_STATIC bool
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
d_ftp_internal_read_iso_date
  File-local: reads "YYYY-MM-DD" into the date; writes nothing on failure.
*/
D_STATIC bool
d_ftp_internal_read_iso_date(
    struct d_ftp_span  _token,
    struct d_ftp_time* _time
)
{
    // ten characters with dashes at 4 and 7
    if ( (_token.length != 10u)   ||
         (_token.data[4] != '-')  ||
         (_token.data[7] != '-') )
    {
        return false;
    }

    const struct d_ftp_span year  = { _token.data, 4u };
    const struct d_ftp_span month = { _token.data + 5u, 2u };
    const struct d_ftp_span day   = { _token.data + 8u, 2u };

    // digits between the dashes
    if ( (!d_ftp_internal_is_number(year))  ||
         (!d_ftp_internal_is_number(month)) ||
         (!d_ftp_internal_is_number(day)) )
    {
        return false;
    }

    _time->year  = (uint16_t)d_ftp_internal_digits_value(year.data,
                                                         4u);
    _time->month = (uint8_t)d_ftp_internal_digits_value(month.data,
                                                        2u);
    _time->day   = (uint8_t)d_ftp_internal_digits_value(day.data,
                                                        2u);

    return true;
}

/*
d_ftp_internal_unix_date
  File-local: tries to read a date at `_position`: "YYYY-MM-DD HH:MM", "Mon DD
HH:MM", or "Mon DD YYYY". A year-less date takes the year of `_now`, or the
year before when that would put it more than a day ahead; with no valid
`_now` the date is consumed but left unknown. `_out` changes only when a
date is recorded.
*/
D_STATIC bool
d_ftp_internal_unix_date(
    struct d_ftp_span        _line,
    size_t                   _position,
    const struct d_ftp_time* _now,
    struct d_ftp_entry*      _out,
    size_t*                  _out_end
)
{
    const size_t            first_end  = d_ftp_internal_token_end(_line,
                                                                  _position);
    const size_t            second     = d_ftp_internal_skip_blanks(_line,
                                                                    first_end);
    const size_t            second_end = d_ftp_internal_token_end(_line,
                                                                  second);
    const struct d_ftp_span first      = d_ftp_internal_span_at(_line,
                                                                _position,
                                                                first_end);
    const struct d_ftp_span next       = d_ftp_internal_span_at(_line,
                                                                second,
                                                                second_end);
    struct d_ftp_time       time       = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    bool                    has_year   = true;
    bool                    has_clock  = true;
    size_t                  end        = second_end;
    const bool              iso_date   = d_ftp_internal_read_iso_date(first,
                                                                      &time);
    const bool              iso_clock  = ( (iso_date) &&
                                           (d_ftp_internal_read_clock(next,
                                                                      &time)) );

    // not ISO: then "Mon DD" and a year or a time of day
    if (!iso_clock)
    {
        const struct d_ftp_time empty     = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
        const unsigned          month     = d_ftp_internal_month(first);
        const size_t            third     = d_ftp_internal_skip_blanks(
                                                _line,
                                                second_end);
        const size_t            third_end = d_ftp_internal_token_end(_line,
                                                                     third);
        const struct d_ftp_span last      = d_ftp_internal_span_at(_line,
                                                                   third,
                                                                   third_end);
        uint64_t                day       = 0;
        const bool              day_ok    = ( (next.length <= 2u) &&
                                              (d_ftp_internal_parse_uint(
                                                   next.data,
                                                   next.length,
                                                   31u,
                                                   &day)) );

        // a month name, then a day of the month
        if ( (month == 0u) ||
             (!day_ok)     ||
             (day == 0u) )
        {
            return false;
        }

        time       = empty;
        time.month = (uint8_t)month;
        time.day   = (uint8_t)day;
        end        = third_end;

        // a four-digit year, or else a time of day
        if ( (last.length == 4u) &&
             (d_ftp_internal_is_number(last)) )
        {
            time.year = (uint16_t)d_ftp_internal_digits_value(last.data,
                                                              4u);
            has_clock = false;
        }
        else if (d_ftp_internal_read_clock(last,
                                           &time))
        {
            has_year = false;
        }
        else
        {
            return false;
        }
    }

    *_out_end = end;

    // a year-less date is placed relative to now
    if (!has_year)
    {
        // no usable clock: the date is consumed but unknown
        if (!d_ftp_time_is_valid(_now))
        {
            return true;
        }

        const int64_t today = d_ftp_internal_days_from_civil(
                                  (int64_t)_now->year,
                                  _now->month,
                                  _now->day);
        const int64_t then  = d_ftp_internal_days_from_civil(
                                  (int64_t)_now->year,
                                  time.month,
                                  time.day);

        // more than a day ahead means last year
        time.year = (uint16_t)((then > (today + 1))
                               ? (_now->year - 1u)
                               : _now->year);
    }

    // a date that cannot exist is not a date
    if (!d_ftp_time_is_valid(&time))
    {
        return false;
    }

    _out->modified  = time;
    _out->known    |= D_FTP_ENTRY_KNOWN_MODIFIED;

    // an explicit time of day is known too
    if (has_clock)
    {
        _out->known |= D_FTP_ENTRY_KNOWN_TIME;
    }

    return true;
}

/*
d_ftp_internal_split_link
  File-local: splits a symbolic link's name at the first " -> ", moving what
follows into the link target.
*/
D_STATIC void
d_ftp_internal_split_link(
    struct d_ftp_entry* _entry
)
{
    const struct d_ftp_span name = _entry->name;

    // the first arrow wins
    for (size_t index = 0; (index + 4u) <= name.length; index++)
    {
        if (memcmp(name.data + index,
                   " -> ",
                   4u) == 0)
        {
            _entry->name.length         = index;
            _entry->link_target.data    = name.data + index + 4u;
            _entry->link_target.length  = name.length - index - 4u;
            _entry->known              |= D_FTP_ENTRY_KNOWN_LINK_TARGET;

            return;
        }
    }

    return;
}

/*
d_ftp_internal_unix_fields
  File-local: reads the fields between the permissions and the date, from the
right: a size (or a device's "major, minor", which is no size), then an owner
and a group after an optional link count. Servers omit the link count or the
group often enough that position from the left cannot be trusted.
*/
D_STATIC void
d_ftp_internal_unix_fields(
    const struct d_ftp_span _fields[],
    size_t                  _count,
    struct d_ftp_entry*     _out
)
{
    size_t   end  = _count;
    uint64_t size = 0;

    // a device's "major," "minor" pair stands where a size would
    if ( (end >= 2u)                                             &&
         (_fields[end - 2u].length > 0u)                         &&
         (_fields[end - 2u].data[_fields[end - 2u].length - 1u] ==
          ',') )
    {
        end -= 2u;
    }
    else if ( (end >= 1u) &&
              (d_ftp_internal_is_number(_fields[end - 1u])) )
    {
        // a size too large for 64 bits is left unknown
        if (d_ftp_internal_parse_uint(_fields[end - 1u].data,
                                      _fields[end - 1u].length,
                                      UINT64_MAX,
                                      &size))
        {
            _out->size   = size;
            _out->known |= D_FTP_ENTRY_KNOWN_SIZE;
        }

        end -= 1u;
    }

    size_t start = 0;

    // a leading number is the link count
    if ( (start < end) &&
         (d_ftp_internal_is_number(_fields[0])) )
    {
        start = 1u;
    }

    // the owner, then the group when there is one
    if (start < end)
    {
        _out->owner  = _fields[start];
        _out->known |= D_FTP_ENTRY_KNOWN_OWNER;
    }

    if ((start + 1u) < end)
    {
        _out->group  = _fields[start + 1u];
        _out->known |= D_FTP_ENTRY_KNOWN_GROUP;
    }

    return;
}

/*
d_ftp_internal_parse_unix
  File-local: fields are collected until one begins a date, so the variable
middle (link count, owner, group, size) never has to be counted in advance;
everything after the date is the name, spaces and all.
*/
D_STATIC enum d_ftp_line_result
d_ftp_internal_parse_unix(
    struct d_ftp_span        _line,
    const struct d_ftp_time* _now,
    struct d_ftp_entry*      _out
)
{
    // "total N" summarizes the listing and names nothing
    if (d_ftp_internal_starts_nocase(_line.data,
                                     _line.length,
                                     "total"))
    {
        return D_FTP_LINE_SKIP;
    }

    const size_t            mode_end   = d_ftp_internal_token_end(_line,
                                                                  0u);
    const struct d_ftp_span mode_field = { _line.data, mode_end };
    enum d_ftp_entry_type   type       = D_FTP_ENTRY_UNKNOWN;
    uint32_t                mode       = 0;

    // the line opens with the permission field
    if (!d_ftp_internal_unix_mode(mode_field,
                                  &type,
                                  &mode))
    {
        return D_FTP_LINE_MALFORMED;
    }

    struct d_ftp_span fields[6] = { { NULL, 0u } };
    size_t            count     = 0;
    size_t            position  = mode_end;
    size_t            date_end  = 0;

    // collect fields until one begins a date
    for (;;)
    {
        position = d_ftp_internal_skip_blanks(_line,
                                              position);

        // the line ran out before any date
        if (position >= _line.length)
        {
            return D_FTP_LINE_MALFORMED;
        }

        // a date here ends the fields
        if (d_ftp_internal_unix_date(_line,
                                     position,
                                     _now,
                                     _out,
                                     &date_end))
        {
            break;
        }

        // links, owner, group, and a device's two numbers: six at most
        if (count == 6u)
        {
            return D_FTP_LINE_MALFORMED;
        }

        const size_t end = d_ftp_internal_token_end(_line,
                                                    position);

        fields[count] = d_ftp_internal_span_at(_line,
                                               position,
                                               end);
        count++;
        position = end;
    }

    const size_t name_start = d_ftp_internal_skip_blanks(_line,
                                                         date_end);

    // an entry needs a name
    if (name_start >= _line.length)
    {
        return D_FTP_LINE_MALFORMED;
    }

    _out->type   = type;
    _out->mode   = mode;
    _out->known |= D_FTP_ENTRY_KNOWN_MODE;
    _out->name   = d_ftp_internal_span_at(_line,
                                          name_start,
                                          _line.length);

    // a symbolic link names its target after " -> "
    if (type == D_FTP_ENTRY_SYMLINK)
    {
        d_ftp_internal_split_link(_out);
    }

    d_ftp_internal_unix_fields(fields,
                               count,
                               _out);

    return D_FTP_LINE_ENTRY;
}

/*
d_ftp_internal_is_dos_date
  File-local: reports whether a token is "MM-DD-YY" or "MM-DD-YYYY".
*/
D_STATIC bool
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
d_ftp_internal_parse_dos
  File-local: date, time, "<DIR>" or a size, then the name. Two-digit years
pivot at 1970, as IIS writes them.
*/
D_STATIC enum d_ftp_line_result
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

    const unsigned    year = d_ftp_internal_digits_value(date.data + 6u,
                                                         date.length - 6u);
    struct d_ftp_time time = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    time.month = (uint8_t)d_ftp_internal_digits_value(date.data,
                                                      2u);
    time.day   = (uint8_t)d_ftp_internal_digits_value(date.data + 3u,
                                                      2u);
    time.year  = (uint16_t)((date.length == 10u) ? year
                            : (year < 70u)       ? (2000u + year)
                                                 : (1900u + year));

    const size_t            clock_start = d_ftp_internal_skip_blanks(_line,
                                                                     date_end);
    const size_t            clock_end   = d_ftp_internal_token_end(_line,
                                                                   clock_start);
    const struct d_ftp_span clock       = d_ftp_internal_span_at(_line,
                                                                 clock_start,
                                                                 clock_end);

    // then the time of day
    if (!d_ftp_internal_read_dos_clock(clock,
                                       &time))
    {
        return D_FTP_LINE_MALFORMED;
    }

    const size_t            kind_start = d_ftp_internal_skip_blanks(_line,
                                                                    clock_end);
    const size_t            kind_end   = d_ftp_internal_token_end(_line,
                                                                  kind_start);
    const struct d_ftp_span kind       = d_ftp_internal_span_at(_line,
                                                                kind_start,
                                                                kind_end);
    const size_t            name_start = d_ftp_internal_skip_blanks(_line,
                                                                    kind_end);
    uint64_t                size       = 0;
    const bool              directory  = d_ftp_internal_equals_nocase(
                                             kind.data,
                                             kind.length,
                                             "<DIR>");
    const bool              bracketed  = ( (kind.length > 0u) &&
                                           (kind.data[0] == '<') );
    const bool              sized      = d_ftp_internal_read_grouped(kind,
                                                                     &size);

    // "<DIR>", another bracketed kind such as "<JUNCTION>", or a size
    if (directory)
    {
        _out->type = D_FTP_ENTRY_DIRECTORY;
    }
    else if (bracketed)
    {
        _out->type = D_FTP_ENTRY_OTHER;
    }
    else if (sized)
    {
        _out->type   = D_FTP_ENTRY_FILE;
        _out->size   = size;
        _out->known |= D_FTP_ENTRY_KNOWN_SIZE;
    }
    else
    {
        return D_FTP_LINE_MALFORMED;
    }

    // a name, and a date that exists
    if ( (name_start >= _line.length) ||
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

/*
d_ftp_internal_mlsx_split
  File-local: splits an MLSx line into its facts and its name. The facts run
to the first space and end in ';' (RFC 3659 7.2); an MLST line carries one
extra leading space, which is skipped.
*/
D_STATIC bool
d_ftp_internal_mlsx_split(
    struct d_ftp_span  _line,
    struct d_ftp_span* _out_facts,
    struct d_ftp_span* _out_name
)
{
    const size_t      start = ( (_line.length > 0u) &&
                                (_line.data[0] == ' ') ) ? 1u : 0u;
    const char* const space = memchr(_line.data + start,
                                     ' ',
                                     _line.length - start);

    // facts end at the first space
    if (!space)
    {
        return false;
    }

    const size_t      length = (size_t)(space - (_line.data + start));
    const char* const equals = memchr(_line.data + start,
                                      '=',
                                      length);

    // at least one fact, and the last one terminated
    if ( (length == 0u)                                  ||
         (!equals)                                       ||
         (_line.data[start + length - 1u] != ';') )
    {
        return false;
    }

    _out_facts->data   = _line.data + start;
    _out_facts->length = length;
    _out_name->data    = space + 1;
    _out_name->length  = _line.length - start - length - 1u;

    return true;
}

/*
d_ftp_internal_parse_octal
  File-local: reads one to six octal digits, at most 07777, as mode bits.
*/
D_STATIC bool
d_ftp_internal_parse_octal(
    struct d_ftp_span _text,
    uint32_t*         _out_value
)
{
    // one to six digits
    if ( (_text.length == 0u) ||
         (_text.length > 6u) )
    {
        return false;
    }

    uint32_t value = 0;

    // octal digits only
    for (size_t index = 0; index < _text.length; index++)
    {
        const char c = _text.data[index];

        if ( (c < '0') ||
             (c > '7') )
        {
            return false;
        }

        value = (value * 8u) + (uint32_t)(c - '0');
    }

    // permission and special bits only
    if (value > 07777u)
    {
        return false;
    }

    *_out_value = value;

    return true;
}

/*
d_ftp_internal_mlsx_type
  File-local: reads the "type" fact, including the OS.unix symbolic-link forms
("OS.unix=slink:target" and "OS.unix=symlink") that servers use.
*/
D_STATIC void
d_ftp_internal_mlsx_type(
    struct d_ftp_span   _value,
    struct d_ftp_entry* _out
)
{
    static const char* const NAMES[4] = { "file", "dir", "cdir", "pdir" };
    static const enum d_ftp_entry_type TYPES[4] =
    {
        D_FTP_ENTRY_FILE,
        D_FTP_ENTRY_DIRECTORY,
        D_FTP_ENTRY_CURRENT_DIRECTORY,
        D_FTP_ENTRY_PARENT_DIRECTORY
    };

    // the four standard types
    for (size_t index = 0; index < 4u; index++)
    {
        if (d_ftp_internal_equals_nocase(_value.data,
                                         _value.length,
                                         NAMES[index]))
        {
            _out->type = TYPES[index];

            return;
        }
    }

    const bool slink   = d_ftp_internal_starts_nocase(_value.data,
                                                      _value.length,
                                                      "os.unix=slink");
    const bool symlink = d_ftp_internal_starts_nocase(_value.data,
                                                      _value.length,
                                                      "os.unix=symlink");

    // anything else is some other kind of object
    if ( (!slink) &&
         (!symlink) )
    {
        _out->type = D_FTP_ENTRY_OTHER;

        return;
    }

    const char* const colon = memchr(_value.data,
                                     ':',
                                     _value.length);

    _out->type = D_FTP_ENTRY_SYMLINK;

    // "slink:target" names the target
    if (colon)
    {
        _out->link_target.data    = colon + 1;
        _out->link_target.length  = _value.length -
                                    (size_t)(colon - _value.data) - 1u;
        _out->known              |= D_FTP_ENTRY_KNOWN_LINK_TARGET;
    }

    return;
}

/*
d_ftp_internal_mlsx_fact
  File-local: records one fact. Unknown facts are ignored, as RFC 3659 7.5
requires, and a malformed value leaves its field unknown rather than
discarding the entry.
*/
D_STATIC void
d_ftp_internal_mlsx_fact(
    struct d_ftp_span   _name,
    struct d_ftp_span   _value,
    struct d_ftp_entry* _out
)
{
    uint64_t          number = 0;
    uint32_t          mode   = 0;
    struct d_ftp_time time   = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    if (d_ftp_internal_equals_nocase(_name.data,
                                     _name.length,
                                     "type"))
    {
        d_ftp_internal_mlsx_type(_value,
                                 _out);
    }
    else if ( (d_ftp_internal_equals_nocase(_name.data,
                                            _name.length,
                                            "size")) ||
              (d_ftp_internal_equals_nocase(_name.data,
                                            _name.length,
                                            "sizd")) )
    {
        // "size" for files, "sizd" for directories
        if (d_ftp_internal_parse_uint(_value.data,
                                      _value.length,
                                      UINT64_MAX,
                                      &number))
        {
            _out->size   = number;
            _out->known |= D_FTP_ENTRY_KNOWN_SIZE;
        }
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "modify"))
    {
        // MLSx times are UTC
        if (d_ftp_time_parse(_value.data,
                             _value.length,
                             &time) == D_FTP_OK)
        {
            _out->modified  = time;
            _out->known    |= D_FTP_ENTRY_KNOWN_MODIFIED;
            _out->known    |= D_FTP_ENTRY_KNOWN_TIME;
            _out->known    |= D_FTP_ENTRY_KNOWN_UTC;
        }
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "unique"))
    {
        _out->unique  = _value;
        _out->known  |= D_FTP_ENTRY_KNOWN_UNIQUE;
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "perm"))
    {
        _out->permissions  = _value;
        _out->known       |= D_FTP_ENTRY_KNOWN_PERMISSIONS;
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "unix.mode"))
    {
        // octal mode bits
        if (d_ftp_internal_parse_octal(_value,
                                       &mode))
        {
            _out->mode   = mode;
            _out->known |= D_FTP_ENTRY_KNOWN_MODE;
        }
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "unix.owner"))
    {
        _out->owner  = _value;
        _out->known |= D_FTP_ENTRY_KNOWN_OWNER;
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "unix.group"))
    {
        _out->group  = _value;
        _out->known |= D_FTP_ENTRY_KNOWN_GROUP;
    }

    return;
}

/*
d_ftp_internal_parse_mlsx
  File-local: each fact is "name=value;", split at its first '=' so values
such as "OS.unix=slink:x" keep their own '='. A line without facts is taken
as a bare name.
*/
D_STATIC enum d_ftp_line_result
d_ftp_internal_parse_mlsx(
    struct d_ftp_span   _line,
    struct d_ftp_entry* _out
)
{
    struct d_ftp_span facts = { NULL, 0u };
    struct d_ftp_span name  = { NULL, 0u };

    // an entry without facts is only its name
    if (!d_ftp_internal_mlsx_split(_line,
                                   &facts,
                                   &name))
    {
        const size_t start = ( (_line.length > 0u) &&
                               (_line.data[0] == ' ') ) ? 1u : 0u;

        name = d_ftp_internal_span_at(_line,
                                      start,
                                      _line.length);
    }

    // the name is what the line exists to carry
    if (name.length == 0u)
    {
        return D_FTP_LINE_MALFORMED;
    }

    size_t position = 0;

    // fact by fact
    while (position < facts.length)
    {
        const char* const semicolon = memchr(facts.data + position,
                                             ';',
                                             facts.length - position);
        const size_t      end       = (semicolon)
                                      ? (size_t)(semicolon - facts.data)
                                      : facts.length;
        const struct d_ftp_span fact = d_ftp_internal_span_at(facts,
                                                              position,
                                                              end);

        position = end + 1u;

        // ";;" leaves an empty fact, which says nothing
        if (fact.length == 0u)
        {
            continue;
        }

        const char* const equals = memchr(fact.data,
                                          '=',
                                          fact.length);

        // every fact names itself
        if (!equals)
        {
            return D_FTP_LINE_MALFORMED;
        }

        const size_t name_length = (size_t)(equals - fact.data);

        d_ftp_internal_mlsx_fact(d_ftp_internal_span_at(fact,
                                                        0u,
                                                        name_length),
                                 d_ftp_internal_span_at(fact,
                                                        name_length + 1u,
                                                        fact.length),
                                 _out);
    }

    _out->name = name;

    return D_FTP_LINE_ENTRY;
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
