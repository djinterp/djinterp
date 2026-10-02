/*******************************************************************************
* djinterp [net]                                                  ftp_internal.c
*
* Implementation of the private helpers declared in ftp_internal.h.
*   Bytes are classified locally rather than with <ctype.h>, whose answers
* depend on the locale and are undefined for negative `char` values; wire
* text needs exact ASCII.
*
*
* path:      /src/djinterp/net/ftp/ftp_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "./ftp_internal.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // int64_t, uint64_t
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span


//==============================================================================
// 2.  HELPERS
//==============================================================================

/*
d_ftp_internal_is_digit
  Reports whether `_c` is an ASCII
decimal digit.
*/
bool
d_ftp_internal_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

/*
d_ftp_internal_is_alpha
  Reports whether `_c` is an ASCII letter.
*/
bool
d_ftp_internal_is_alpha(
    char _c
)
{
    return ( ( (_c >= 'a') &&
               (_c <= 'z') ) ||
             ( (_c >= 'A') &&
               (_c <= 'Z') ) );
}

/*
d_ftp_internal_is_blank
  Reports whether `_c` is a space or a horizontal tab.
*/
bool
d_ftp_internal_is_blank(
    char _c
)
{
    return ( (_c == ' ') ||
             (_c == '\t') );
}

/*
d_ftp_internal_to_lower
  Folds an ASCII capital to lower case and returns any other byte
unchanged.
*/
char
d_ftp_internal_to_lower(
    char _c
)
{
    // only the 26 ASCII capitals fold
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return (char)((_c - 'A') + 'a');
    }

    return _c;
}

/*
d_ftp_internal_hex_value
  The value of an ASCII hexadecimal digit of either case, or -1
for any other byte.
*/
int
d_ftp_internal_hex_value(
    char _c
)
{
    const char lower = d_ftp_internal_to_lower(_c);

    // decimal digits first
    if (d_ftp_internal_is_digit(lower))
    {
        return lower - '0';
    }

    // then a through f
    if ( (lower >= 'a') &&
         (lower <= 'f') )
    {
        return (lower - 'a') + 10;
    }

    return -1;
}

/*
d_ftp_internal_equals_nocase
  Compares `_length` bytes at `_text` with the NUL-terminated
`_word`, folding ASCII case; true only when the lengths match too.
*/
bool
d_ftp_internal_equals_nocase(
    const char* _text,
    size_t      _length,
    const char* _word
)
{
    size_t index = 0;

    // walk both until the text ends or they differ
    while (index < _length)
    {
        // the word ended first, or a byte differs
        if ( (_word[index] == '\0') ||
             (d_ftp_internal_to_lower(_text[index]) !=
              d_ftp_internal_to_lower(_word[index])) )
        {
            return false;
        }

        index++;
    }

    return (_word[index] == '\0');
}

/*
d_ftp_internal_starts_nocase
  Reports whether the `_length` bytes at `_text` begin with the
NUL-terminated `_word`, folding ASCII case.
*/
bool
d_ftp_internal_starts_nocase(
    const char* _text,
    size_t      _length,
    const char* _word
)
{
    size_t index = 0;

    // every byte of the word must be matched
    while (_word[index] != '\0')
    {
        // the text ended first, or a byte differs
        if ( (index >= _length) ||
             (d_ftp_internal_to_lower(_text[index]) !=
              d_ftp_internal_to_lower(_word[index])) )
        {
            return false;
        }

        index++;
    }

    return true;
}

/*
d_ftp_internal_parse_uint
  Parses `_length` bytes that must all be decimal digits, at least
one, into a value no greater than `_max`. Overflow is refused before it can
happen, so any `_max` up to UINT64_MAX is safe.
*/
bool
d_ftp_internal_parse_uint(
    const char* _text,
    size_t      _length,
    uint64_t    _max,
    uint64_t*   _out_value
)
{
    // an empty field is not a number
    if (_length == 0u)
    {
        return false;
    }

    uint64_t value = 0;

    // accumulate digit by digit, stopping short of the maximum
    for (size_t index = 0; index < _length; index++)
    {
        // anything but a digit ends the number too early
        if (!d_ftp_internal_is_digit(_text[index]))
        {
            return false;
        }

        const uint64_t digit = (uint64_t)(_text[index] - '0');

        // the next step would pass the maximum
        if ( (digit > _max) ||
             (value > ((_max - digit) / 10u)) )
        {
            return false;
        }

        value = (value * 10u) + digit;
    }

    *_out_value = value;

    return true;
}

/*
d_ftp_internal_digits_value
  The value of `_count` bytes the caller has already checked are
digits. Every caller passes four or fewer, so the result cannot overflow.
*/
unsigned
d_ftp_internal_digits_value(
    const char* _text,
    size_t      _count
)
{
    unsigned value = 0;

    // most significant digit first
    for (size_t index = 0; index < _count; index++)
    {
        value = (value * 10u) + (unsigned)(_text[index] - '0');
    }

    return value;
}

/*
d_ftp_internal_is_number
  Reports whether a span is one or more decimal digits.
*/
bool
d_ftp_internal_is_number(
    struct d_ftp_span _span
)
{
    // an empty span is not a number
    if (_span.length == 0u)
    {
        return false;
    }

    // every byte a digit
    for (size_t index = 0; index < _span.length; index++)
    {
        if (!d_ftp_internal_is_digit(_span.data[index]))
        {
            return false;
        }
    }

    return true;
}

/*
d_ftp_internal_trim
  The span of `_length` bytes at `_text` without leading blanks,
or trailing blanks and line-ending bytes.
*/
struct d_ftp_span
d_ftp_internal_trim(
    const char* _text,
    size_t      _length
)
{
    size_t start = 0;
    size_t end   = _length;

    // leading blanks
    while ( (start < end) &&
            (d_ftp_internal_is_blank(_text[start])) )
    {
        start++;
    }

    // trailing blanks, CR, and LF
    while ( (end > start) &&
            ( (d_ftp_internal_is_blank(_text[end - 1u])) ||
              (_text[end - 1u] == '\r')                  ||
              (_text[end - 1u] == '\n') ) )
    {
        end--;
    }

    const struct d_ftp_span result = { _text + start, end - start };

    return result;
}

/*
d_ftp_internal_trim_eol
  The span of `_length` bytes at `_text` without trailing CR and
LF bytes. Blanks survive: in a listing they may belong to a file name.
*/
struct d_ftp_span
d_ftp_internal_trim_eol(
    const char* _text,
    size_t      _length
)
{
    size_t end = _length;

    // only line-ending bytes go
    while ( (end > 0u) &&
            ( (_text[end - 1u] == '\r') ||
              (_text[end - 1u] == '\n') ) )
    {
        end--;
    }

    const struct d_ftp_span result = { _text, end };

    return result;
}

/*
d_ftp_internal_has_line_break
  Reports whether any of `_length` bytes at `_text` is CR or LF,
the bytes that would end a command or reply line early. NULL has none.
*/
bool
d_ftp_internal_has_line_break(
    const char* _text,
    size_t      _length
)
{
    // nothing to scan
    if (!_text)
    {
        return false;
    }

    // any CR or LF ends a line
    for (size_t index = 0; index < _length; index++)
    {
        if ( (_text[index] == '\r') ||
             (_text[index] == '\n') )
        {
            return true;
        }
    }

    return false;
}

/*
d_ftp_internal_buffer_ok
  Reports whether `_buffer` meets the invariants every writer
relies on: storage present, and the length inside it with room left for the
terminator that is always kept.
*/
bool
d_ftp_internal_buffer_ok(
    const struct d_ftp_buffer* _buffer
)
{
    return ( (_buffer != NULL)                     &&
             (_buffer->data != NULL)               &&
             (_buffer->capacity > 0u)              &&
             (_buffer->length < _buffer->capacity) );
}

/*
d_ftp_internal_room
  The bytes a valid buffer can still take, its terminator
reserved.
*/
size_t
d_ftp_internal_room(
    const struct d_ftp_buffer* _buffer
)
{
    return (_buffer->capacity - _buffer->length) - 1u;
}

/*
d_ftp_internal_append
  Appends `_length` bytes and re-terminates, or appends nothing and
returns false when they and the terminator do not fit.
*/
bool
d_ftp_internal_append(
    struct d_ftp_buffer* _out,
    const char*          _data,
    size_t               _length
)
{
    // all or nothing
    if (_length > d_ftp_internal_room(_out))
    {
        return false;
    }

    // memcpy wants a valid source even for zero bytes
    if (_length > 0u)
    {
        memcpy(_out->data + _out->length,
               _data,
               _length);
    }

    _out->length             += _length;
    _out->data[_out->length]  = '\0';

    return true;
}

/*
d_ftp_internal_append_char
  Appends one byte, as d_ftp_internal_append() does.
*/
bool
d_ftp_internal_append_char(
    struct d_ftp_buffer* _out,
    char                 _c
)
{
    return d_ftp_internal_append(_out,
                                 &_c,
                                 1u);
}

/*
d_ftp_internal_append_text
  Appends a NUL-terminated string, as d_ftp_internal_append()
does.
*/
bool
d_ftp_internal_append_text(
    struct d_ftp_buffer* _out,
    const char*          _text
)
{
    return d_ftp_internal_append(_out,
                                 _text,
                                 strlen(_text));
}

/*
d_ftp_internal_append_uint
  Appends `_value` in decimal, zero-padded on the left to at least
`_width` digits, or nothing when the field does not fit. The digits are
produced backwards into a scratch array sized for UINT64_MAX (20 digits).
*/
bool
d_ftp_internal_append_uint(
    struct d_ftp_buffer* _out,
    uint64_t             _value,
    size_t               _width
)
{
    char     digits[20] = { 0 };
    size_t   count      = 0;
    uint64_t remaining  = _value;

    // least significant digit first; zero still yields one digit
    while ( (count == 0u) ||
            (remaining > 0u) )
    {
        digits[count] = (char)('0' + (int)(remaining % 10u));
        count++;
        remaining /= 10u;
    }

    const size_t padding = (_width > count) ? (_width - count) : 0u;

    // the whole field and the terminator must fit
    if ((padding + count) > d_ftp_internal_room(_out))
    {
        return false;
    }

    // the padding
    for (size_t index = 0; index < padding; index++)
    {
        _out->data[_out->length] = '0';
        _out->length++;
    }

    // then the digits, back in reading order
    while (count > 0u)
    {
        count--;
        _out->data[_out->length] = digits[count];
        _out->length++;
    }

    _out->data[_out->length] = '\0';

    return true;
}

/*
d_ftp_internal_rollback
  Truncates a buffer back to `_mark`, undoing a partial write so
that failed writers leave their output untouched.
*/
void
d_ftp_internal_rollback(
    struct d_ftp_buffer* _out,
    size_t               _mark
)
{
    _out->length      = _mark;
    _out->data[_mark] = '\0';

    return;
}

/*
d_ftp_internal_commit
  Appends text composed in a scratch buffer, all or nothing. The
fixed-width encodings are built in scratch storage sized for their longest
form, so only this final copy can fail.
*/
enum d_ftp_error
d_ftp_internal_commit(
    struct d_ftp_buffer*       _out,
    const struct d_ftp_buffer* _local
)
{
    // all of the composed text, or none of it
    if (!d_ftp_internal_append(_out,
                               _local->data,
                               _local->length))
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_telnet_accept
  Advances the Telnet filter by one byte and reports whether the
byte is data. IAC IAC yields one data byte 0xFF; IAC WILL, WONT, DO, or DONT
swallows the option byte after it; any other IAC command is two bytes long.
*/
bool
d_ftp_internal_telnet_accept(
    unsigned char* _state,
    unsigned char  _byte
)
{
    // after IAC: an escaped 0xFF, a negotiation, or a two-byte command
    if (*_state == (unsigned char)D_FTP_INTERNAL_TELNET_STATE_COMMAND)
    {
        const bool negotiation = ( (_byte >= D_FTP_INTERNAL_TELNET_WILL) &&
                                   (_byte <= D_FTP_INTERNAL_TELNET_DONT) );

        *_state = (negotiation)
                  ? (unsigned char)D_FTP_INTERNAL_TELNET_STATE_OPTION
                  : (unsigned char)D_FTP_INTERNAL_TELNET_STATE_DATA;

        return (_byte == D_FTP_INTERNAL_TELNET_IAC);
    }

    // a negotiation's option byte is not data
    if (*_state == (unsigned char)D_FTP_INTERNAL_TELNET_STATE_OPTION)
    {
        *_state = (unsigned char)D_FTP_INTERNAL_TELNET_STATE_DATA;

        return false;
    }

    // IAC opens a command sequence
    if (_byte == D_FTP_INTERNAL_TELNET_IAC)
    {
        *_state = (unsigned char)D_FTP_INTERNAL_TELNET_STATE_COMMAND;

        return false;
    }

    return true;
}

/*
d_ftp_internal_days_from_civil
  Days from 1970-01-01 to a proleptic Gregorian date, negative
before it. This is Howard Hinnant's days_from_civil: starting the year in
March puts the leap day last, which makes every 400-year era one closed
formula with no table and no loop.
*/
int64_t
d_ftp_internal_days_from_civil(
    int64_t  _year,
    unsigned _month,
    unsigned _day
)
{
    const int64_t year  = (_month <= 2u) ? (_year - 1) : _year;
    const int64_t era   = ((year >= 0) ? year : (year - 399)) / 400;
    const int64_t yoe   = year - (era * 400);
    const int64_t shift = (_month > 2u) ? ((int64_t)_month - 3)
                                        : ((int64_t)_month + 9);
    const int64_t doy   = (((153 * shift) + 2) / 5) + ((int64_t)_day - 1);
    const int64_t doe   = (yoe * 365) + (yoe / 4) - (yoe / 100) + doy;

    return (era * 146097) + doe - 719468;
}

/*
d_ftp_internal_civil_from_days
  The inverse of d_ftp_internal_days_from_civil(), by the same
March-based era arithmetic.
*/
void
d_ftp_internal_civil_from_days(
    int64_t   _days,
    int64_t*  _out_year,
    unsigned* _out_month,
    unsigned* _out_day
)
{
    const int64_t z     = _days + 719468;
    const int64_t era   = ((z >= 0) ? z : (z - 146096)) / 146097;
    const int64_t doe   = z - (era * 146097);
    const int64_t yoe   = (doe - (doe / 1460) + (doe / 36524) -
                           (doe / 146096)) / 365;
    const int64_t doy   = doe - ((365 * yoe) + (yoe / 4) - (yoe / 100));
    const int64_t shift = ((5 * doy) + 2) / 153;
    const int64_t month = (shift < 10) ? (shift + 3) : (shift - 9);

    *_out_year  = (yoe + (era * 400)) + ((month <= 2) ? 1 : 0);
    *_out_month = (unsigned)month;
    *_out_day   = (unsigned)((doy - (((153 * shift) + 2) / 5)) + 1);

    return;
}
