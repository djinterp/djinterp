/*******************************************************************************
* djinterp [net]                                              ftp_listing_unix.c
*
* Implementation of the Unix listing dialect: "ls -l" lines.
*   Fields are collected until one begins a date, so the variable middle
* columns never need counting; everything after the date is the name, spaces
* and all.
*
*
* path:      /src/djinterp/net/ftp/ftp_listing_unix.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // int64_t, uint8_t, uint16_t, uint32_t, uint64_t
#include <string.h>   // memchr, memcmp, strchr
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "../../../../inc/djinterp/net/ftp/ftp_fact.h"    // d_ftp_time
#include "./ftp_internal.h"                               // text helpers
#include "./ftp_listing_internal.h"                       // listing helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_ftp_internal_date
//   struct: a date read from a Unix listing: the time, what it carried, and
// where its last token ended.
struct d_ftp_internal_date
{
    struct d_ftp_time time;       // what was read
    bool              has_year;   // a year was given
    bool              has_clock;  // a time of day was given
    size_t            end;        // just past the date's last token
};

// d_ftp_internal_columns
//   struct: the fields between a Unix line's permissions and its date --
// links, owner, group, and a device's two numbers, six at most -- and
// where the date ended.
struct d_ftp_internal_columns
{
    struct d_ftp_span fields[6];  // the fields, in order
    size_t            count;      // fields used
    size_t            date_end;   // just past the date
};

//==============================================================================
// 2.  DIRECTORY LISTINGS
//==============================================================================

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
d_ftp_internal_unix_type
  File-local: reads the file-type character opening a permission field.
Block and character devices, pipes, sockets, and Solaris doors are OTHER.
*/
D_STATIC bool
d_ftp_internal_unix_type(
    char                   _character,
    enum d_ftp_entry_type* _out_type
)
{
    switch (_character)
    {
        case '-':
            *_out_type = D_FTP_ENTRY_FILE;

            return true;

        case 'd':
            *_out_type = D_FTP_ENTRY_DIRECTORY;

            return true;

        case 'l':
            *_out_type = D_FTP_ENTRY_SYMLINK;

            return true;

        case 'b':
        case 'c':
        case 'p':
        case 's':
        case 'D':
            *_out_type = D_FTP_ENTRY_OTHER;

            return true;

        default:
            break;
    }

    return false;
}

/*
d_ftp_internal_unix_bits
  File-local: reads the nine permission characters, three per class, into
mode bits. Each position has its own alphabet: r, w, x, s, and t grant the
permission itself, and s, S, t, and T set their class's special bit --
setuid, setgid, or sticky -- as does l, setgid as mandatory locking.
*/
D_STATIC bool
d_ftp_internal_unix_bits(
    const char* _characters,
    uint32_t*   _out_mode
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
    uint32_t              mode       = 0u;

    // nine permission characters, three per class
    for (size_t index = 0u; index < 9u; index++)
    {
        const char c = _characters[index];

        // each position's alphabet; strchr() would also find the NUL
        if ( (c == '\0') ||
             (!strchr(ALLOWED[index],
                      c)) )
        {
            return false;
        }

        // the permission itself
        if (strchr("rwxst",
                   c))
        {
            mode |= PERMISSION[index];
        }

        // the class's special bit
        if (strchr("sStTl",
                   c))
        {
            mode |= SPECIAL[index / 3u];
        }
    }

    *_out_mode = mode;

    return true;
}

/*
d_ftp_internal_unix_mode
  Reads an "ls -l" permission field -- "drwxr-sr-x" and kin, with an
optional eleventh '+', '.', or '@' for ACLs, security contexts, or extended
attributes -- into an entry type and mode bits.
*/
bool
d_ftp_internal_unix_mode(
    struct d_ftp_span      _field,
    enum d_ftp_entry_type* _out_type,
    uint32_t*              _out_mode
)
{
    enum d_ftp_entry_type type = D_FTP_ENTRY_OTHER;
    uint32_t              mode = 0u;

    // ten characters, or eleven with a marker
    if ( (_field.length < 10u) ||
         (_field.length > 11u) )
    {
        return false;
    }

    // the only markers
    if ( (_field.length == 11u)   &&
         (_field.data[10] != '+') &&
         (_field.data[10] != '.') &&
         (_field.data[10] != '@') )
    {
        return false;
    }

    // the file-type character, then the nine permissions
    if ( (!d_ftp_internal_unix_type(_field.data[0],
                                    &type)) ||
         (!d_ftp_internal_unix_bits(_field.data + 1,
                                    &mode)) )
    {
        return false;
    }

    *_out_type = type;
    *_out_mode = mode;

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
d_ftp_internal_unix_calendar
  File-local: reads "Mon DD" from the tokens `_first` and `_next`, then the
token after `_second_end`: a four-digit year, or a time of day, which leaves
the year for d_ftp_internal_place_year().
*/
D_STATIC bool
d_ftp_internal_unix_calendar(
    struct d_ftp_span           _line,
    struct d_ftp_span           _first,
    struct d_ftp_span           _next,
    size_t                      _second_end,
    struct d_ftp_internal_date* _out
)
{
    const struct d_ftp_time empty     = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    const unsigned          month     = d_ftp_internal_month(_first);
    const size_t            third     = d_ftp_internal_skip_blanks(
                                            _line,
                                            _second_end);
    const size_t            third_end = d_ftp_internal_token_end(_line,
                                                                 third);
    const struct d_ftp_span last      = d_ftp_internal_span_at(_line,
                                                               third,
                                                               third_end);
    uint64_t                day       = 0u;

    // a month name, then a day of the month
    if ( (month == 0u)                              ||
         (_next.length > 2u)                        ||
         (!d_ftp_internal_parse_uint(_next.data,
                                     _next.length,
                                     31u,
                                     &day))         ||
         (day == 0u) )
    {
        return false;
    }

    _out->time       = empty;
    _out->time.month = (uint8_t)month;
    _out->time.day   = (uint8_t)day;
    _out->end        = third_end;

    // a four-digit year
    if ( (last.length == 4u) &&
         (d_ftp_internal_is_number(last)) )
    {
        _out->time.year = (uint16_t)d_ftp_internal_digits_value(last.data,
                                                                4u);
        _out->has_clock = false;

        return true;
    }

    _out->has_year = false;

    return d_ftp_internal_read_clock(last,
                                     &_out->time);
}

/*
d_ftp_internal_place_year
  File-local: gives a year-less date the year of `_now`, or the year before
when that would put it more than a day ahead: listings drop the year only
for recent dates, and a day of slack absorbs clocks and time zones.
*/
D_STATIC void
d_ftp_internal_place_year(
    const struct d_ftp_time* _now,
    struct d_ftp_time*       _time
)
{
    const int64_t today = d_ftp_internal_days_from_civil(
                              (int64_t)_now->year,
                              _now->month,
                              _now->day);
    const int64_t then  = d_ftp_internal_days_from_civil(
                              (int64_t)_now->year,
                              _time->month,
                              _time->day);

    _time->year = (uint16_t)((then > (today + 1)) ? (_now->year - 1u)
                                                  : _now->year);

    return;
}

/*
d_ftp_internal_unix_date
  File-local: tries to read a date at `_position`: "YYYY-MM-DD HH:MM", "Mon DD
HH:MM", or "Mon DD YYYY". A year-less date takes its year from `_now`; with
no valid `_now` the date is consumed but left unknown. `_out` changes only
when a date is recorded.
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
    const size_t               first_end  =
        d_ftp_internal_token_end(_line,
                                 _position);
    const size_t               second     =
        d_ftp_internal_skip_blanks(_line,
                                   first_end);
    const size_t               second_end =
        d_ftp_internal_token_end(_line,
                                 second);
    const struct d_ftp_span    first      =
        d_ftp_internal_span_at(_line,
                               _position,
                               first_end);
    const struct d_ftp_span    next       =
        d_ftp_internal_span_at(_line,
                               second,
                               second_end);
    struct d_ftp_internal_date date       =
        { { 0u, 0u, 0u, 0u, 0u, 0u, 0u }, true, true, second_end };

    // ISO, or else "Mon DD" and a year or a time of day
    if ( ( (!d_ftp_internal_read_iso_date(first,
                                          &date.time))  ||
           (!d_ftp_internal_read_clock(next,
                                       &date.time)) )   &&
         (!d_ftp_internal_unix_calendar(_line,
                                        first,
                                        next,
                                        second_end,
                                        &date)) )
    {
        return false;
    }

    *_out_end = date.end;

    // a year-less date is placed relative to now, or left unknown
    if (!date.has_year)
    {
        if (!d_ftp_time_is_valid(_now))
        {
            return true;
        }

        d_ftp_internal_place_year(_now,
                                  &date.time);
    }

    // a date that cannot exist is not a date
    if (!d_ftp_time_is_valid(&date.time))
    {
        return false;
    }

    _out->modified  = date.time;
    _out->known    |= D_FTP_ENTRY_KNOWN_MODIFIED;
    _out->known    |= (date.has_clock) ? D_FTP_ENTRY_KNOWN_TIME : 0u;

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
d_ftp_internal_unix_columns
  File-local: collects fields from `_position` until one begins a date,
which is recorded in `_out`, so the variable middle columns never need
counting. Fails when the line runs out first, or when more than six fields
come before a date.
*/
D_STATIC bool
d_ftp_internal_unix_columns(
    struct d_ftp_span              _line,
    size_t                         _position,
    const struct d_ftp_time*       _now,
    struct d_ftp_entry*            _out,
    struct d_ftp_internal_columns* _columns
)
{
    // collect fields until one begins a date
    for (size_t position = _position; ; )
    {
        position = d_ftp_internal_skip_blanks(_line,
                                              position);

        // the line ran out before any date, or a date here ends the fields
        if ( (position >= _line.length)                          ||
             (d_ftp_internal_unix_date(_line,
                                       position,
                                       _now,
                                       _out,
                                       &_columns->date_end)) )
        {
            return (position < _line.length);
        }

        // links, owner, group, and a device's two numbers: six at most
        if (_columns->count == 6u)
        {
            return false;
        }

        const size_t end = d_ftp_internal_token_end(_line,
                                                    position);

        _columns->fields[_columns->count] = d_ftp_internal_span_at(_line,
                                                                   position,
                                                                   end);
        _columns->count++;
        position = end;
    }
}

/*
d_ftp_internal_parse_unix
  Reads an "ls -l" line: the permission field, the columns up to and through
the date, and then the name -- everything after the date, spaces and all.
*/
enum d_ftp_line_result
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

    const size_t                  mode_end   =
        d_ftp_internal_token_end(_line,
                                 0u);
    const struct d_ftp_span       mode_field = { _line.data, mode_end };
    enum d_ftp_entry_type         type       = D_FTP_ENTRY_UNKNOWN;
    uint32_t                      mode       = 0u;
    struct d_ftp_internal_columns columns    = { { { NULL, 0u } }, 0u, 0u };

    // the permission field, then the columns through the date
    if ( (!d_ftp_internal_unix_mode(mode_field,
                                    &type,
                                    &mode))            ||
         (!d_ftp_internal_unix_columns(_line,
                                       mode_end,
                                       _now,
                                       _out,
                                       &columns)) )
    {
        return D_FTP_LINE_MALFORMED;
    }

    const size_t name_start = d_ftp_internal_skip_blanks(_line,
                                                         columns.date_end);

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

    d_ftp_internal_unix_fields(columns.fields,
                               columns.count,
                               _out);

    return D_FTP_LINE_ENTRY;
}
