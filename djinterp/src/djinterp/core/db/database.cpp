/*******************************************************************************
* djinterp [core]                                                   database.cpp
*
* Definitions for database.hpp: its exceptions and configurations, and the
* utilities -- connection strings, quoting and escaping, values as text, and
* names.
*   Text is read and written without the C locale: numbers go through the C
* layers' readers (db_common.h's conversions), which read `.` as the point
* under any locale, and out through a stream imbued with the classic locale;
* letters are compared as ASCII. Points in time are converted with the
* proleptic Gregorian calendar's day arithmetic, in UTC, with neither the C
* library's gmtime, which is not thread-safe, nor its timegm, which is not
* standard.
*
*
* path:      /src/djinterp/core/db/database.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/core/db/database.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>     // std::chrono::system_clock, duration_cast, floor
#include <cmath>      // std::isnan, std::isinf
#include <cstddef>    // std::size_t
#include <iomanip>    // std::setprecision
#include <limits>     // std::numeric_limits
#include <locale>     // std::locale::classic
#include <map>        // std::map
#include <optional>   // std::optional
#include <sstream>    // std::ostringstream
#include <string>     // std::string, std::to_string
#include <utility>    // std::pair
#include <variant>    // std::visit, std::monostate
#include <vector>     // std::vector
// djinterp
#include "../../../../inc/djinterp/c/db/db_common.h"  // d_db_value_*,
                                                       // d_db_type_name
// re_std
#include "../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int32_t,
                                                       // int64_t, uint8_t


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================
// ASCII, numbers, calendar arithmetic, and the parts of the connection-string
// readers.

namespace
{

/**
 * @brief Whether a character is ASCII white space.
 *
 * @param[in] _c  the character.
 * @return `true` for a space, tab, line feed, carriage return, vertical tab
 *         or form feed.
 */
bool
ascii_space(
    char _c
) noexcept
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\n') ||
             (_c == '\r') ||
             (_c == '\v') ||
             (_c == '\f') );
}

/**
 * @brief Whether a character is an ASCII digit.
 *
 * @param[in] _c  the character.
 * @return `true` for `0` to `9`.
 */
bool
ascii_digit(
    char _c
) noexcept
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

/**
 * @brief Folds a string's ASCII letters to lower case.
 *
 * @param[in] _text  the string.
 * @return the folded copy; every other byte is kept.
 */
std::string
ascii_lower(
    const std::string& _text
)
{
    std::string folded(_text);

    // each upper-case ASCII letter to its lower case
    for (char& c : folded)
    {
        if ( (c >= 'A') &&
             (c <= 'Z') )
        {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }

    return folded;
}

/**
 * @brief Trims ASCII white space from both ends of a string.
 *
 * @param[in] _text  the string.
 * @return the trimmed copy.
 */
std::string
ascii_trim(
    const std::string& _text
)
{
    std::size_t begin = 0u;
    std::size_t end   = _text.size();

    // white space at the front, then at the back
    while ( (begin < end) &&
            (ascii_space(_text[begin])) )
    {
        ++begin;
    }

    while ( (end > begin) &&
            (ascii_space(_text[end - 1u])) )
    {
        --end;
    }

    return _text.substr(begin, end - begin);
}

/**
 * @brief The value of a hexadecimal digit.
 *
 * @param[in] _c  the character.
 * @return 0 to 15, or -1 for a character that is no hexadecimal digit.
 */
int
hex_value(
    char _c
) noexcept
{
    // digits, then letters of either case
    if (ascii_digit(_c))
    {
        return _c - '0';
    }

    if ( (_c >= 'a') &&
         (_c <= 'f') )
    {
        return _c - 'a' + 10;
    }

    if ( (_c >= 'A') &&
         (_c <= 'F') )
    {
        return _c - 'A' + 10;
    }

    return -1;
}

/**
 * @brief Throws the exception for a connection string that cannot be read.
 *
 * @param[in] _why  what is wrong with it.
 * @throws exception with D_DB_INVALID_ARGUMENT, always.
 */
[[noreturn]] void
refuse_string(
    const std::string& _why
)
{
    throw exception("parse_connection_string: " + _why,
                    D_DB_INVALID_ARGUMENT);
}

/**
 * @brief Throws the exception for text that is not a value of a type.
 *
 * @param[in] _text  the text.
 * @param[in] _type  the type's name.
 * @throws type_conversion_exception with D_DB_MISMATCH, always.
 */
[[noreturn]] void
refuse_value(
    const std::string& _text,
    const char*        _type
)
{
    // a long text is named by its start, so that the message stays a line
    const std::string shown = (_text.size() > 64u)
                                  ? _text.substr(0u, 64u) + "..."
                                  : _text;

    throw type_conversion_exception("string_to_value: '" + shown +
                                    "' is not " + _type,
                                    D_DB_MISMATCH);
}

/**
 * @brief Describes text as the C layers' conversions take a value.
 *
 * @param[in] _text  the text; it must outlive the value.
 * @return a TEXT value over the text.
 */
struct d_db_value
text_value(
    const std::string& _text
) noexcept
{
    struct d_db_value described = {};

    described.type    = D_DB_VALUE_TEXT;
    described.size    = _text.size();
    described.as.text = _text.c_str();

    return described;
}

/**
 * @brief Reads text as a 64-bit integer, as the C layers read one.
 *
 * @param[in]  _text   the text.
 * @param[out] _value  receives the integer; written only on success.
 * @return `true` for an optional sign and digits that int64_t holds, and
 *         nothing else; a NUL inside, which would end the C reader's text
 *         early, is never one.
 */
bool
read_int64(
    const std::string& _text,
    re_std::int64_t&   _value
) noexcept
{
    const struct d_db_value described = text_value(_text);

    return ( (_text.find('\0') == std::string::npos) &&
             (d_db_value_int64(&described, &_value) == 0) );
}

/**
 * @brief Reads text as a double, as the C layers read one.
 *
 * @param[in]  _text   the text.
 * @param[out] _value  receives the double; written only on success.
 * @return `true` for a number, its point a `.`; NaN and the infinities are
 *         read by the caller.
 */
bool
read_double(
    const std::string& _text,
    double&            _value
) noexcept
{
    const struct d_db_value described = text_value(_text);

    return ( (_text.find('\0') == std::string::npos) &&
             (d_db_value_double(&described, &_value) == 0) );
}

/**
 * @brief Writes a double in the fewest significant digits that read back as
 *        the same double, with `.` as the point.
 *
 * @param[in] _value  the double.
 * @return the text; NaN and the infinities as PostgreSQL spells them.
 *
 * Fifteen significant digits suffice for most doubles, and %g's form drops
 * the trailing zeros, so the first that reads back is the shortest: no
 * double needs more than seventeen.
 */
std::string
format_double(
    double _value
)
{
    // the values with no digits
    if (std::isnan(_value))
    {
        return "NaN";
    }

    if (std::isinf(_value))
    {
        return (_value > 0.0) ? "Infinity"
                              : "-Infinity";
    }

    std::string written;

    // fifteen, sixteen, then seventeen digits, the first that reads back
    for (int precision = 15; precision <= 17; ++precision)
    {
        std::ostringstream out;
        double             read = 0.0;

        out.imbue(std::locale::classic());
        out << std::setprecision(precision) << _value;
        written = out.str();

        if ( (read_double(written, read)) &&
             (read == _value) )
        {
            break;
        }
    }

    return written;
}

/**
 * @brief The days from 1970-01-01 to a date of the proleptic Gregorian
 *        calendar.
 *
 * @param[in] _year   the year.
 * @param[in] _month  the month, 1 to 12.
 * @param[in] _day    the day, 1 to 31.
 * @return the days; negative before 1970.
 *
 * Howard Hinnant's days_from_civil: years counted from March, so that the
 * leap day ends a year, in 400-year eras of 146097 days.
 */
re_std::int64_t
days_from_civil(
    re_std::int64_t _year,
    unsigned        _month,
    unsigned        _day
) noexcept
{
    const re_std::int64_t year = _year - ((_month <= 2u) ? 1 : 0);
    const re_std::int64_t era  = ((year >= 0) ? year : (year - 399)) / 400;
    const unsigned        yoe  = static_cast<unsigned>(year - (era * 400));
    const unsigned        doy  = ((153u * ((_month > 2u) ? (_month - 3u)
                                                          : (_month + 9u))) +
                                  2u) / 5u + _day - 1u;
    const unsigned        doe  = (yoe * 365u) + (yoe / 4u) - (yoe / 100u) +
                                 doy;

    return (era * 146097) + static_cast<re_std::int64_t>(doe) - 719468;
}

// civil_date
//   struct: a date of the proleptic Gregorian calendar.
struct civil_date
{
    re_std::int64_t year;
    unsigned        month;
    unsigned        day;
};

/**
 * @brief The date a count of days from 1970-01-01 falls on.
 *
 * @param[in] _days  the days; negative before 1970.
 * @return the date.
 *
 * Howard Hinnant's civil_from_days, days_from_civil's inverse.
 */
civil_date
civil_from_days(
    re_std::int64_t _days
) noexcept
{
    const re_std::int64_t days = _days + 719468;
    const re_std::int64_t era  = ((days >= 0) ? days : (days - 146096)) /
                                 146097;
    const unsigned        doe  = static_cast<unsigned>(days - (era * 146097));
    const unsigned        yoe  = (doe - (doe / 1460u) + (doe / 36524u) -
                                  (doe / 146096u)) / 365u;
    const unsigned        doy  = doe - ((365u * yoe) + (yoe / 4u) -
                                        (yoe / 100u));
    const unsigned        mp   = ((5u * doy) + 2u) / 153u;
    const unsigned        day  = doy - (((153u * mp) + 2u) / 5u) + 1u;
    const unsigned        mon  = (mp < 10u) ? (mp + 3u)
                                            : (mp - 9u);
    civil_date            date = {};

    date.year  = static_cast<re_std::int64_t>(yoe) + (era * 400) +
                 ((mon <= 2u) ? 1 : 0);
    date.month = mon;
    date.day   = day;

    return date;
}

/**
 * @brief The days in a month of the proleptic Gregorian calendar.
 *
 * @param[in] _year   the year.
 * @param[in] _month  the month, 1 to 12.
 * @return 28 to 31.
 */
unsigned
days_in_month(
    re_std::int64_t _year,
    unsigned        _month
) noexcept
{
    static const unsigned DAYS[12] = { 31u, 28u, 31u, 30u, 31u, 30u,
                                       31u, 31u, 30u, 31u, 30u, 31u };
    const bool            leap     = ( ( (_year % 4) == 0 ) &&
                                       ( ( (_year % 100) != 0 ) ||
                                         ( (_year % 400) == 0 ) ) );

    return ( (_month == 2u) &&
             (leap) ) ? 29u
                      : DAYS[_month - 1u];
}

/**
 * @brief Writes an unsigned number in decimal, zero-padded to a width.
 *
 * @param[in,out] _out    the text to append to.
 * @param[in]     _value  the number.
 * @param[in]     _width  the fewest digits to write.
 */
void
append_padded(
    std::string&       _out,
    unsigned long long _value,
    std::size_t        _width
)
{
    const std::string digits = std::to_string(_value);

    // zeros, then the digits
    if (digits.size() < _width)
    {
        _out.append(_width - digits.size(), '0');
    }

    _out += digits;

    return;
}

/**
 * @brief Writes a point in time as its UTC date and time.
 *
 * @param[in] _point  the point.
 * @return `YYYY-MM-DD HH:MM:SS`, with a fraction of a second where there is
 *         one, to nine digits at most and without trailing zeros.
 */
std::string
format_time_point(
    std::chrono::system_clock::time_point _point
)
{
    using day_duration = std::chrono::duration<re_std::int64_t,
                                               std::ratio<86400>>;

    const std::chrono::system_clock::duration since = _point.time_since_epoch();
    const day_duration days = std::chrono::floor<day_duration>(since);
    const std::chrono::system_clock::duration within = since - days;
    const std::chrono::seconds seconds =
        std::chrono::floor<std::chrono::seconds>(within);
    const std::chrono::nanoseconds fraction =
        std::chrono::duration_cast<std::chrono::nanoseconds>(within - seconds);
    const civil_date date = civil_from_days(days.count());
    const long long  of_day = static_cast<long long>(seconds.count());
    std::string      text;

    // the date; a year before 1 keeps its sign
    if (date.year < 0)
    {
        text += '-';
        append_padded(text,
                      static_cast<unsigned long long>(-date.year),
                      4u);
    }
    else
    {
        append_padded(text,
                      static_cast<unsigned long long>(date.year),
                      4u);
    }

    text += '-';
    append_padded(text, date.month, 2u);
    text += '-';
    append_padded(text, date.day, 2u);
    text += ' ';
    append_padded(text, static_cast<unsigned long long>(of_day / 3600), 2u);
    text += ':';
    append_padded(text,
                  static_cast<unsigned long long>((of_day / 60) % 60),
                  2u);
    text += ':';
    append_padded(text, static_cast<unsigned long long>(of_day % 60), 2u);

    // a fraction, to the clock's precision, without trailing zeros
    if (fraction.count() != 0)
    {
        std::string digits;

        append_padded(digits,
                      static_cast<unsigned long long>(fraction.count()),
                      9u);

        while (digits.back() == '0')
        {
            digits.pop_back();
        }

        text += '.';
        text += digits;
    }

    return text;
}

// time_reader
//   struct: reads a point in time from text, position by position.
struct time_reader
{
    const std::string& text;
    std::size_t        at;

    // number -- exactly `_count` digits, as an unsigned number, or false
    bool
    number(
        std::size_t _count,
        unsigned&   _value
    ) noexcept
    {
        unsigned read = 0u;

        // too few characters left for the digits
        if ((text.size() - at) < _count)
        {
            return false;
        }

        // each digit in turn
        for (std::size_t i = 0u; i < _count; ++i)
        {
            if (!ascii_digit(text[at + i]))
            {
                return false;
            }

            read = (read * 10u) + static_cast<unsigned>(text[at + i] - '0');
        }

        at     += _count;
        _value  = read;

        return true;
    }

    // skip -- one given character, or false
    bool
    skip(
        char _c
    ) noexcept
    {
        // only the character asked for
        if ( (at >= text.size()) ||
             (text[at] != _c) )
        {
            return false;
        }

        ++at;

        return true;
    }

    // done -- whether every character was read
    bool
    done() const noexcept
    {
        return (at == text.size());
    }
};

/**
 * @brief Reads the time of day and the offset after a date.
 *
 * @param[in,out] _reader   the reader, after the date.
 * @param[out]    _seconds  receives the seconds since midnight, the offset
 *                          taken off.
 * @param[out]    _nanos    receives the fraction of a second, in
 *                          nanoseconds.
 * @return `true` when the rest of the text is a time and an offset, or
 *         nothing.
 */
bool
read_time_of_day(
    time_reader&     _reader,
    re_std::int64_t& _seconds,
    re_std::int64_t& _nanos
) noexcept
{
    unsigned hour   = 0u;
    unsigned minute = 0u;
    unsigned second = 0u;

    _seconds = 0;
    _nanos   = 0;

    // a date alone is its midnight
    if (_reader.done())
    {
        return true;
    }

    // HH:MM, with :SS and a fraction optional
    if ( ( (!_reader.skip(' ')) &&
           (!_reader.skip('T')) ) ||
         (!_reader.number(2u, hour))   ||
         (!_reader.skip(':'))          ||
         (!_reader.number(2u, minute)) ||
         (hour > 23u)                  ||
         (minute > 59u) )
    {
        return false;
    }

    // seconds, and a fraction of one
    if (_reader.skip(':'))
    {
        if ( (!_reader.number(2u, second)) ||
             (second > 59u) )
        {
            return false;
        }

        if (_reader.skip('.'))
        {
            re_std::int64_t scale  = 100000000;
            std::size_t     digits = 0u;

            // digits past the ninth are below a nanosecond, and dropped
            while ( (!_reader.done()) &&
                    (ascii_digit(_reader.text[_reader.at])) )
            {
                _nanos += scale * (_reader.text[_reader.at] - '0');
                scale  /= 10;
                ++_reader.at;
                ++digits;
            }

            if (digits == 0u)
            {
                return false;
            }
        }
    }

    _seconds = (static_cast<re_std::int64_t>(hour) * 3600) +
               (static_cast<re_std::int64_t>(minute) * 60) +
               static_cast<re_std::int64_t>(second);

    // no offset: UTC
    if (_reader.done())
    {
        return true;
    }

    // Z, or a sign, hours, and minutes with or without a colon
    if (_reader.skip('Z'))
    {
        return _reader.done();
    }

    const bool west = ( (!_reader.done()) &&
                        (_reader.text[_reader.at] == '-') );
    unsigned   offset_hours   = 0u;
    unsigned   offset_minutes = 0u;

    if ( ( (!_reader.skip('+')) &&
           (!_reader.skip('-')) ) ||
         (!_reader.number(2u, offset_hours)) ||
         (offset_hours > 23u) )
    {
        return false;
    }

    // minutes, after a colon or not
    if (!_reader.done())
    {
        (void)_reader.skip(':');

        if ( (!_reader.number(2u, offset_minutes)) ||
             (offset_minutes > 59u) )
        {
            return false;
        }
    }

    const re_std::int64_t offset =
        (static_cast<re_std::int64_t>(offset_hours) * 3600) +
        (static_cast<re_std::int64_t>(offset_minutes) * 60);

    _seconds += (west) ? offset
                       : -offset;

    return _reader.done();
}

/**
 * @brief Reads text as a point in time.
 *
 * @param[in]  _text   the text, as string_to_value describes it.
 * @param[out] _point  receives the point; written only on success.
 * @return `true` for a date of the calendar, a time, and an offset that the
 *         clock can hold.
 */
bool
read_time_point(
    const std::string&                     _text,
    std::chrono::system_clock::time_point& _point
) noexcept
{
    time_reader     reader = { _text, 0u };
    unsigned        year   = 0u;
    unsigned        month  = 0u;
    unsigned        day    = 0u;
    re_std::int64_t within = 0;
    re_std::int64_t nanos  = 0;

    // YYYY-MM-DD, a day the month has
    if ( (!reader.number(4u, year))  ||
         (!reader.skip('-'))         ||
         (!reader.number(2u, month)) ||
         (!reader.skip('-'))         ||
         (!reader.number(2u, day))   ||
         (month < 1u)                ||
         (month > 12u)               ||
         (day < 1u)                  ||
         (day > days_in_month(year, month)) )
    {
        return false;
    }

    // the time, the offset, and nothing after them
    if (!read_time_of_day(reader, within, nanos))
    {
        return false;
    }

    using clock_duration = std::chrono::system_clock::duration;

    const re_std::int64_t seconds =
        (days_from_civil(year, month, day) * 86400) + within;
    const re_std::int64_t limit =
        std::chrono::duration_cast<std::chrono::seconds>(
            clock_duration::max()).count() - 1;

    // a point the clock cannot hold
    if ( (seconds > limit) ||
         (seconds < -limit) )
    {
        return false;
    }

    _point = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<clock_duration>(
            std::chrono::seconds(seconds)) +
        std::chrono::duration_cast<clock_duration>(
            std::chrono::nanoseconds(nanos)));

    return true;
}

/**
 * @brief Reads hexadecimal as bytes.
 *
 * @param[in]  _text   the text, optionally after `\x` or `0x`.
 * @param[out] _bytes  receives the bytes; written only on success.
 * @return `true` for pairs of hexadecimal digits, and nothing else.
 */
bool
read_hex(
    const std::string&            _text,
    std::vector<re_std::uint8_t>& _bytes
)
{
    std::size_t at = 0u;

    // the prefixes PostgreSQL and C write
    if ( (_text.size() >= 2u) &&
         ( (_text.compare(0u, 2u, "\\x") == 0) ||
           (_text.compare(0u, 2u, "0x") == 0)  ||
           (_text.compare(0u, 2u, "0X") == 0) ) )
    {
        at = 2u;
    }

    // whole pairs only
    if (((_text.size() - at) % 2u) != 0u)
    {
        return false;
    }

    std::vector<re_std::uint8_t> bytes;

    bytes.reserve((_text.size() - at) / 2u);

    // each pair to its byte
    for (; at < _text.size(); at += 2u)
    {
        const int high = hex_value(_text[at]);
        const int low  = hex_value(_text[at + 1u]);

        if ( (high < 0) ||
             (low < 0) )
        {
            return false;
        }

        bytes.push_back(static_cast<re_std::uint8_t>((high * 16) + low));
    }

    _bytes.swap(bytes);

    return true;
}

/**
 * @brief Percent-decodes a part of a URI.
 *
 * @param[in] _part  the part.
 * @return the decoded part.
 * @throws exception with D_DB_INVALID_ARGUMENT for a `%` not followed by two
 *         hexadecimal digits.
 */
std::string
percent_decode(
    const std::string& _part
)
{
    std::string decoded;

    decoded.reserve(_part.size());

    // each escape to its byte, everything else as it is
    for (std::size_t i = 0u; i < _part.size(); ++i)
    {
        if (_part[i] != '%')
        {
            decoded += _part[i];

            continue;
        }

        // two hexadecimal digits must follow
        if ( (i + 2u >= _part.size()) ||
             (hex_value(_part[i + 1u]) < 0) ||
             (hex_value(_part[i + 2u]) < 0) )
        {
            refuse_string("a '%' in the URI is not followed by two "
                          "hexadecimal digits");
        }

        decoded += static_cast<char>((hex_value(_part[i + 1u]) * 16) +
                                     hex_value(_part[i + 2u]));
        i       += 2u;
    }

    return decoded;
}

/**
 * @brief Reads a port number.
 *
 * @param[in] _text  the text.
 * @return the port.
 * @throws exception with D_DB_INVALID_ARGUMENT for anything but a number
 *         from 1 to 65535.
 */
re_std::uint16_t
read_port(
    const std::string& _text
)
{
    re_std::int64_t port = 0;

    // digits only, a port's range
    if ( (_text.empty())                       ||
         (_text[0] == '+')                     ||
         (_text[0] == '-')                     ||
         (!read_int64(_text, port))            ||
         (port < 1)                            ||
         (port > 65535) )
    {
        refuse_string("'" + _text + "' is not a port from 1 to 65535");
    }

    return static_cast<re_std::uint16_t>(port);
}

/**
 * @brief Applies one key and its value to a configuration.
 *
 * @param[in,out] _config  the configuration.
 * @param[in]     _key     the key, as written.
 * @param[in]     _value   the value, unquoted.
 * @throws exception with D_DB_INVALID_ARGUMENT for a value its key cannot
 *         take: a port, an sslmode, a timeout.
 */
void
apply_keyword(
    connection_config& _config,
    const std::string& _key,
    const std::string& _value
)
{
    const std::string key = ascii_lower(ascii_trim(_key));

    // each well-known key to its field, any other into custom_options
    if ( (key == "host")        ||
         (key == "server")      ||
         (key == "hostaddr")    ||
         (key == "data source") ||
         (key == "address") )
    {
        _config.host = _value;
    }
    else if (key == "port")
    {
        _config.port = read_port(_value);
    }
    else if ( (key == "dbname")          ||
              (key == "database")        ||
              (key == "initial catalog") )
    {
        _config.database = _value;
    }
    else if ( (key == "user")     ||
              (key == "username") ||
              (key == "uid")      ||
              (key == "user id") )
    {
        _config.username = _value;
    }
    else if ( (key == "password") ||
              (key == "pwd") )
    {
        _config.password = _value;
    }
    else if ( (key == "charset") ||
              (key == "client_encoding") )
    {
        _config.charset = _value;
    }
    else if ( (key == "schema") ||
              (key == "search_path") )
    {
        _config.schema = _value;
    }
    else if (key == "sslmode")
    {
        const std::string mode = ascii_lower(_value);

        // the modes that do not ask for TLS, then those that do
        if ( (mode == "disable") ||
             (mode == "allow")   ||
             (mode == "prefer") )
        {
            _config.enable_ssl = false;
        }
        else if (mode == "require")
        {
            _config.enable_ssl = true;
            _config.verify_ssl = false;
        }
        else if ( (mode == "verify-ca") ||
                  (mode == "verify-full") )
        {
            _config.enable_ssl = true;
            _config.verify_ssl = true;
        }
        else
        {
            refuse_string("'" + _value + "' is not an sslmode");
        }
    }
    else if (key == "sslcert")
    {
        _config.ssl_cert = _value;
    }
    else if (key == "sslkey")
    {
        _config.ssl_key = _value;
    }
    else if ( (key == "sslrootcert") ||
              (key == "sslca") )
    {
        _config.ssl_ca = _value;
    }
    else if (key == "connect_timeout")
    {
        re_std::int64_t seconds = 0;

        // whole seconds, none of them negative
        if ( (_value.empty())               ||
             (_value[0] == '+')             ||
             (_value[0] == '-')             ||
             (!read_int64(_value, seconds)) ||
             (seconds > 86400) )
        {
            refuse_string("'" + _value + "' is not a connect_timeout in "
                          "seconds");
        }

        _config.connect_timeout = std::chrono::seconds(seconds);
    }
    else
    {
        _config.custom_options[ascii_trim(_key)] = _value;
    }

    return;
}

/**
 * @brief Whether a string holds a `;` outside single quotes, which makes it
 *        ODBC's keyword form rather than libpq's.
 *
 * @param[in] _text  the string.
 * @return `true` when it does.
 */
bool
has_bare_semicolon(
    const std::string& _text
) noexcept
{
    bool quoted = false;

    // quotes toggle, a backslash inside them escapes the next character
    for (std::size_t i = 0u; i < _text.size(); ++i)
    {
        if ( (quoted) &&
             (_text[i] == '\\') )
        {
            ++i;
        }
        else if (_text[i] == '\'')
        {
            quoted = !quoted;
        }
        else if ( (!quoted) &&
                  (_text[i] == ';') )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Reads libpq's keyword form: `key=value` pairs separated by white
 *        space, a value in single quotes where it holds spaces, `\` escaping
 *        a quote or a backslash.
 *
 * @param[in]     _text    the string.
 * @param[in,out] _config  the configuration each pair applies to.
 * @throws exception with D_DB_INVALID_ARGUMENT for a pair without `=` or an
 *         unterminated quote; and as apply_keyword.
 */
void
read_libpq_keywords(
    const std::string& _text,
    connection_config& _config
)
{
    std::size_t at = 0u;

    // pair after pair, until only white space is left
    while (true)
    {
        while ( (at < _text.size()) &&
                (ascii_space(_text[at])) )
        {
            ++at;
        }

        if (at >= _text.size())
        {
            break;
        }

        const std::size_t key_start = at;

        // the key, up to `=` or white space
        while ( (at < _text.size()) &&
                (_text[at] != '=')  &&
                (!ascii_space(_text[at])) )
        {
            ++at;
        }

        const std::string key = _text.substr(key_start, at - key_start);

        while ( (at < _text.size()) &&
                (ascii_space(_text[at])) )
        {
            ++at;
        }

        // every key has its `=`
        if ( (key.empty())            ||
             (at >= _text.size())     ||
             (_text[at] != '=') )
        {
            refuse_string("'" + key + "' has no '=' and value");
        }

        ++at;

        while ( (at < _text.size()) &&
                (ascii_space(_text[at])) )
        {
            ++at;
        }

        std::string value;

        // a quoted value, or one up to white space
        if ( (at < _text.size()) &&
             (_text[at] == '\'') )
        {
            bool closed = false;

            for (++at; at < _text.size(); ++at)
            {
                if ( (_text[at] == '\\') &&
                     (at + 1u < _text.size()) )
                {
                    value += _text[++at];
                }
                else if (_text[at] == '\'')
                {
                    closed = true;
                    ++at;

                    break;
                }
                else
                {
                    value += _text[at];
                }
            }

            if (!closed)
            {
                refuse_string("the value of '" + key + "' has no closing "
                              "quote");
            }
        }
        else
        {
            for (; ( (at < _text.size()) &&
                     (!ascii_space(_text[at])) ); ++at)
            {
                if ( (_text[at] == '\\') &&
                     (at + 1u < _text.size()) )
                {
                    ++at;
                }

                value += _text[at];
            }
        }

        apply_keyword(_config, key, value);
    }

    return;
}

/**
 * @brief Reads ODBC's keyword form: `key=value` pairs separated by `;`, a
 *        value in braces where it holds a `;`, `}}` standing for `}`.
 *
 * @param[in]     _text    the string.
 * @param[in,out] _config  the configuration each pair applies to.
 * @throws exception with D_DB_INVALID_ARGUMENT for a pair without `=` or an
 *         unterminated brace; and as apply_keyword.
 */
void
read_odbc_keywords(
    const std::string& _text,
    connection_config& _config
)
{
    std::size_t at = 0u;

    // pair after pair; empty ones, such as a trailing `;` leaves, are skipped
    while (at < _text.size())
    {
        const std::size_t equals = _text.find('=', at);
        const std::size_t ends   = _text.find(';', at);

        // a segment with no `=` is empty, or wrong
        if ( (equals == std::string::npos) ||
             ( (ends != std::string::npos) &&
               (ends < equals) ) )
        {
            const std::size_t stop = (ends == std::string::npos)
                                         ? _text.size()
                                         : ends;

            if (!ascii_trim(_text.substr(at, stop - at)).empty())
            {
                refuse_string("'" + ascii_trim(_text.substr(at, stop - at)) +
                              "' has no '=' and value");
            }

            at = stop + 1u;

            continue;
        }

        const std::string key = ascii_trim(_text.substr(at, equals - at));
        std::size_t       pos = equals + 1u;

        while ( (pos < _text.size()) &&
                (ascii_space(_text[pos])) )
        {
            ++pos;
        }

        std::string value;

        // a braced value, or one up to the next `;`
        if ( (pos < _text.size()) &&
             (_text[pos] == '{') )
        {
            bool closed = false;

            for (++pos; pos < _text.size(); ++pos)
            {
                if (_text[pos] == '}')
                {
                    if ( (pos + 1u < _text.size()) &&
                         (_text[pos + 1u] == '}') )
                    {
                        value += '}';
                        ++pos;

                        continue;
                    }

                    closed = true;
                    ++pos;

                    break;
                }

                value += _text[pos];
            }

            if (!closed)
            {
                refuse_string("the value of '" + key + "' has no closing "
                              "brace");
            }

            const std::size_t next = _text.find(';', pos);

            at = (next == std::string::npos) ? _text.size()
                                             : next + 1u;
        }
        else
        {
            const std::size_t next = _text.find(';', pos);

            value = ascii_trim(_text.substr(pos,
                                            (next == std::string::npos)
                                                ? std::string::npos
                                                : next - pos));
            at    = (next == std::string::npos) ? _text.size()
                                                : next + 1u;
        }

        // a key, at least
        if (key.empty())
        {
            refuse_string("a value has no key");
        }

        apply_keyword(_config, key, value);
    }

    return;
}

/**
 * @brief Reads the URI form.
 *
 * @param[in]     _text    the string, `scheme:` and the rest.
 * @param[in]     _colon   where the scheme's `:` is.
 * @param[in,out] _config  the configuration.
 * @throws exception with D_DB_INVALID_ARGUMENT for a malformed URI; and as
 *         apply_keyword.
 */
void
read_uri(
    const std::string& _text,
    std::size_t        _colon,
    connection_config& _config
)
{
    const std::string scheme = ascii_lower(_text.substr(0u, _colon));
    std::string       rest   = _text.substr(_colon + 1u);
    std::string       query;

    _config.custom_options["scheme"] = scheme;

    // the fragment means nothing to a connection, the query is read last
    rest = rest.substr(0u, rest.find('#'));

    if (rest.find('?') != std::string::npos)
    {
        query = rest.substr(rest.find('?') + 1u);
        rest  = rest.substr(0u, rest.find('?'));
    }

    // SQLite's: everything after the scheme, or after `//`, is the file
    if (scheme == "sqlite")
    {
        _config.database =
            percent_decode((rest.compare(0u, 2u, "//") == 0) ? rest.substr(2u)
                                                             : rest);
    }
    else
    {
        // a server's: //[user[:password]@]host[:port][/database]
        if (rest.compare(0u, 2u, "//") != 0)
        {
            refuse_string("'" + _text + "' has no '//' after its scheme");
        }

        rest = rest.substr(2u);

        const std::size_t slash     = rest.find('/');
        std::string       authority = rest.substr(0u, slash);

        // the database, after the authority
        if (slash != std::string::npos)
        {
            _config.database = percent_decode(rest.substr(slash + 1u));
        }

        // user and password, before the last `@`
        if (authority.rfind('@') != std::string::npos)
        {
            const std::size_t at       = authority.rfind('@');
            const std::string userinfo = authority.substr(0u, at);
            const std::size_t colon    = userinfo.find(':');

            _config.username = percent_decode(userinfo.substr(0u, colon));

            if (colon != std::string::npos)
            {
                _config.password = percent_decode(userinfo.substr(colon + 1u));
            }

            authority = authority.substr(at + 1u);
        }

        // the host -- an IPv6 address in brackets -- and the port
        if ( (!authority.empty()) &&
             (authority[0] == '[') )
        {
            const std::size_t close = authority.find(']');

            if (close == std::string::npos)
            {
                refuse_string("an IPv6 host in '" + _text + "' has no ']'");
            }

            _config.host = authority.substr(1u, close - 1u);

            if (close + 1u < authority.size())
            {
                if (authority[close + 1u] != ':')
                {
                    refuse_string("'" + authority + "' is not a host and "
                                  "port");
                }

                _config.port = read_port(authority.substr(close + 2u));
            }
        }
        else if (authority.find(',') != std::string::npos)
        {
            _config.host = percent_decode(authority);
        }
        else
        {
            const std::size_t colon = authority.find(':');

            _config.host = percent_decode(authority.substr(0u, colon));

            if (colon != std::string::npos)
            {
                _config.port = read_port(authority.substr(colon + 1u));
            }
        }
    }

    // the query's pairs, as keywords
    std::size_t at = 0u;

    while (at < query.size())
    {
        const std::size_t amp   = query.find('&', at);
        const std::string pair  = query.substr(at,
                                               (amp == std::string::npos)
                                                   ? std::string::npos
                                                   : amp - at);
        const std::size_t equal = pair.find('=');

        at = (amp == std::string::npos) ? query.size()
                                        : amp + 1u;

        // an empty pair, as `&&` leaves, is nothing
        if (pair.empty())
        {
            continue;
        }

        if (equal == std::string::npos)
        {
            refuse_string("'" + pair + "' in the URI's query has no '='");
        }

        apply_keyword(_config,
                      percent_decode(pair.substr(0u, equal)),
                      percent_decode(pair.substr(equal + 1u)));
    }

    return;
}

/**
 * @brief Whether a string is the URI form: a scheme and `://`, or SQLite's
 *        scheme and anything.
 *
 * @param[in]  _text   the string.
 * @param[out] _colon  receives where the scheme's `:` is; written only when
 *                     it is the URI form.
 * @return `true` when it is.
 */
bool
is_uri(
    const std::string& _text,
    std::size_t&       _colon
) noexcept
{
    std::size_t at = 0u;

    // a scheme: a letter, then letters, digits, `+`, `-` and `.`
    if ( (_text.empty()) ||
         (!( ( (_text[0] >= 'a') && (_text[0] <= 'z') ) ||
             ( (_text[0] >= 'A') && (_text[0] <= 'Z') ) )) )
    {
        return false;
    }

    while ( (at < _text.size()) &&
            ( ( (_text[at] >= 'a') && (_text[at] <= 'z') ) ||
              ( (_text[at] >= 'A') && (_text[at] <= 'Z') ) ||
              (ascii_digit(_text[at]))                    ||
              (_text[at] == '+')                          ||
              (_text[at] == '-')                          ||
              (_text[at] == '.') ) )
    {
        ++at;
    }

    // then `:`, and `//` or SQLite's scheme
    if ( (at >= _text.size()) ||
         (_text[at] != ':') )
    {
        return false;
    }

    if ( (_text.compare(at + 1u, 2u, "//") == 0) ||
         (ascii_lower(_text.substr(0u, at)) == "sqlite") )
    {
        _colon = at;

        return true;
    }

    return false;
}

/**
 * @brief Appends one `key=value` pair to a keyword string, the value quoted
 *        where it has to be.
 *
 * @param[in,out] _out    the string.
 * @param[in]     _key    the key.
 * @param[in]     _value  the value.
 * @throws exception with D_DB_INVALID_ARGUMENT for a key the keyword form
 *         cannot write: empty, or holding white space, `=`, `'`, `;` or `\`.
 */
void
append_keyword(
    std::string&       _out,
    const std::string& _key,
    const std::string& _value
)
{
    bool quote = _value.empty();

    // a key is written bare, so it must need no quoting
    if (_key.empty())
    {
        throw exception("build_connection_string: an option has an empty "
                        "key",
                        D_DB_INVALID_ARGUMENT);
    }

    for (const char c : _key)
    {
        if ( (ascii_space(c)) ||
             (c == '=')       ||
             (c == '\'')      ||
             (c == ';')       ||
             (c == '\\') )
        {
            throw exception("build_connection_string: the key '" + _key +
                            "' cannot be written in the keyword form",
                            D_DB_INVALID_ARGUMENT);
        }
    }

    // a value with space, a quote, a backslash or a `;` is quoted
    for (const char c : _value)
    {
        if ( (ascii_space(c)) ||
             (c == '\'')      ||
             (c == '\\')      ||
             (c == ';') )
        {
            quote = true;
        }
    }

    if (!_out.empty())
    {
        _out += ' ';
    }

    _out += _key;
    _out += '=';

    if (!quote)
    {
        _out += _value;

        return;
    }

    _out += '\'';

    for (const char c : _value)
    {
        if ( (c == '\'') ||
             (c == '\\') )
        {
            _out += '\\';
        }

        _out += c;
    }

    _out += '\'';

    return;
}

/**
 * @brief The C layers' type for a field type, where the two share one.
 *
 * @param[in]  _type  the field type.
 * @param[out] _out   receives the C type; written only when there is one.
 * @return `true` for boolean through uuid.
 */
bool
to_layer_type(
    field_type      _type,
    enum d_db_type& _out
) noexcept
{
    // the vocabulary the C layers share with field_type, member for member
    switch (_type)
    {
        case field_type::boolean:        _out = D_DB_TYPE_BOOLEAN;        break;
        case field_type::integer:        _out = D_DB_TYPE_INTEGER;        break;
        case field_type::big_integer:    _out = D_DB_TYPE_BIG_INTEGER;    break;
        case field_type::floating_point: _out = D_DB_TYPE_FLOATING_POINT; break;
        case field_type::decimal:        _out = D_DB_TYPE_DECIMAL;        break;
        case field_type::string:         _out = D_DB_TYPE_STRING;         break;
        case field_type::binary:         _out = D_DB_TYPE_BINARY;         break;
        case field_type::date:           _out = D_DB_TYPE_DATE;           break;
        case field_type::time:           _out = D_DB_TYPE_TIME;           break;
        case field_type::datetime:       _out = D_DB_TYPE_DATETIME;       break;
        case field_type::timestamp:      _out = D_DB_TYPE_TIMESTAMP;      break;
        case field_type::json:           _out = D_DB_TYPE_JSON;           break;
        case field_type::xml:            _out = D_DB_TYPE_XML;            break;
        case field_type::uuid:           _out = D_DB_TYPE_UUID;           break;
        case field_type::null:
        case field_type::array:
        case field_type::custom:
        default:
            return false;
    }

    return true;
}

}  // namespace

//==============================================================================
// 2.  EXCEPTIONS
//==============================================================================

// 2.1    Exceptions
//------------------------------------------------------------------------------
exception::exception(
    const std::string& _message,
    int                _error_code
)
    : std::runtime_error(_message),
      m_error_code(_error_code)
{}

exception::exception(
    const char* _message,
    int         _error_code
)
    : std::runtime_error((_message) ? _message
                                    : ""),
      m_error_code(_error_code)
{}

int
exception::error_code() const noexcept
{
    return m_error_code;
}

//==============================================================================
// 3.  CONFIGURATION
//==============================================================================

// 3.1    Connections
//------------------------------------------------------------------------------
connection_config::connection_config()
    : host("localhost"),
      port(0),
      connect_timeout(std::chrono::seconds(30)),
      read_timeout(0),
      write_timeout(0),
      auto_reconnect(false),
      enable_ssl(false),
      verify_ssl(true),
      max_packet_size(16u * 1024u * 1024u)
{}

pool_config::pool_config()
    : min_connections(1),
      max_connections(10),
      connection_queue_size(100),
      connection_timeout(std::chrono::seconds(30)),
      idle_timeout(std::chrono::minutes(10)),
      max_lifetime(std::chrono::hours(1)),
      validate_on_acquire(false),
      validate_on_return(false)
{}

//==============================================================================
// 6.  UTILITIES
//==============================================================================

// 6.1    Connection strings
//------------------------------------------------------------------------------
/*
parse_connection_string
  The form is decided once, by the start: a scheme and `://` (or SQLite's
scheme) is a URI; otherwise a `;` outside quotes is ODBC's keyword form, and
anything else libpq's. Every field the string names is applied in order, so a
key named twice keeps its last value, as libpq keeps it.
*/
connection_config
parse_connection_string(
    const std::string& _connection_string
)
{
    const std::string text   = ascii_trim(_connection_string);
    connection_config config;
    std::size_t       colon  = 0u;

    // the URI form, then the two keyword forms
    if (is_uri(text, colon))
    {
        read_uri(text, colon, config);
    }
    else if (has_bare_semicolon(text))
    {
        read_odbc_keywords(text, config);
    }
    else
    {
        read_libpq_keywords(text, config);
    }

    return config;
}

/*
build_connection_string
  Each field is written only where it differs from connection_config()'s
default, except the ones a connection always needs, so that the string
says what was set; parse_connection_string reads back the same connection.
*/
std::string
build_connection_string(
    const connection_config& _config
)
{
    const connection_config defaults;
    std::string             text;

    // where and as whom
    if (!_config.host.empty())
    {
        append_keyword(text, "host", _config.host);
    }

    if (_config.port != 0u)
    {
        append_keyword(text, "port", std::to_string(_config.port));
    }

    if (!_config.database.empty())
    {
        append_keyword(text, "dbname", _config.database);
    }

    if (!_config.username.empty())
    {
        append_keyword(text, "user", _config.username);
    }

    if (!_config.password.empty())
    {
        append_keyword(text, "password", _config.password);
    }

    // the session's settings, the character set by libpq's name for it
    if (_config.charset)
    {
        append_keyword(text, "client_encoding", *_config.charset);
    }

    if (_config.schema)
    {
        append_keyword(text, "schema", *_config.schema);
    }

    // TLS: a mode only when it is asked for, then the files
    if (_config.enable_ssl)
    {
        append_keyword(text,
                       "sslmode",
                       (_config.verify_ssl) ? "verify-full"
                                            : "require");
    }

    if (_config.ssl_cert)
    {
        append_keyword(text, "sslcert", *_config.ssl_cert);
    }

    if (_config.ssl_key)
    {
        append_keyword(text, "sslkey", *_config.ssl_key);
    }

    if (_config.ssl_ca)
    {
        append_keyword(text, "sslrootcert", *_config.ssl_ca);
    }

    // a timeout of its own, in whole seconds, rounded up
    if (_config.connect_timeout != defaults.connect_timeout)
    {
        const std::chrono::seconds seconds =
            std::chrono::ceil<std::chrono::seconds>(_config.connect_timeout);

        append_keyword(text,
                       "connect_timeout",
                       std::to_string((seconds.count() < 0) ? 0
                                                            : seconds.count()));
    }

    // everything else, as given
    for (const std::pair<const std::string, std::string>& option :
         _config.custom_options)
    {
        append_keyword(text, option.first, option.second);
    }

    return text;
}

// 6.2    Quoting and escaping
//------------------------------------------------------------------------------
std::string
escape_string(
    const std::string& _input
)
{
    std::string escaped;

    // no SQL string literal can hold a NUL
    if (_input.find('\0') != std::string::npos)
    {
        throw exception("escape_string: the text holds a NUL character",
                        D_DB_INVALID_ARGUMENT);
    }

    escaped.reserve(_input.size() + 2u);

    // each quote doubled
    for (const char c : _input)
    {
        if (c == '\'')
        {
            escaped += '\'';
        }

        escaped += c;
    }

    return escaped;
}

/*
quote_identifier
  The closing quote is the one doubled inside: a backtick for backticks, a
double quote for double quotes, and `]` -- not `[` -- for brackets.
*/
std::string
quote_identifier(
    const std::string& _identifier,
    database_type      _database_type
)
{
    char opening = '"';
    char closing = '"';

    // no name can hold a NUL
    if (_identifier.find('\0') != std::string::npos)
    {
        throw exception("quote_identifier: the name holds a NUL character",
                        D_DB_INVALID_ARGUMENT);
    }

    // each vendor's quote; double quotes, the standard's, for the rest
    switch (_database_type)
    {
        case database_type::mariadb:
        case database_type::mysql:
        case database_type::arangodb:
        case database_type::neo4j:
            opening = '`';
            closing = '`';
            break;

        case database_type::mssql:
            opening = '[';
            closing = ']';
            break;

        default:
            break;
    }

    std::string quoted;

    quoted.reserve(_identifier.size() + 2u);
    quoted += opening;

    // the closing quote doubled wherever it appears
    for (const char c : _identifier)
    {
        if (c == closing)
        {
            quoted += closing;
        }

        quoted += c;
    }

    quoted += closing;

    return quoted;
}

// 6.3    Values as text
//------------------------------------------------------------------------------
std::string
value_to_string(
    const value& _value
)
{
    return std::visit(
        [](const auto& _held) -> std::string
        {
            using held_type = std::decay_t<decltype(_held)>;

            // each alternative in the form string_to_value reads back
            if constexpr (std::is_same_v<held_type, std::monostate>)
            {
                return "NULL";
            }
            else if constexpr (std::is_same_v<held_type, bool>)
            {
                return (_held) ? "true"
                               : "false";
            }
            else if constexpr ( (std::is_same_v<held_type,
                                                re_std::int32_t>) ||
                                (std::is_same_v<held_type,
                                                re_std::int64_t>) )
            {
                return std::to_string(static_cast<long long>(_held));
            }
            else if constexpr (std::is_same_v<held_type, double>)
            {
                return format_double(_held);
            }
            else if constexpr (std::is_same_v<held_type, std::string>)
            {
                return _held;
            }
            else if constexpr (std::is_same_v<held_type,
                                              std::vector<re_std::uint8_t>>)
            {
                static const char DIGITS[] = "0123456789abcdef";
                std::string       hex;

                hex.reserve(_held.size() * 2u);

                for (const re_std::uint8_t byte : _held)
                {
                    hex += DIGITS[byte >> 4];
                    hex += DIGITS[byte & 0x0Fu];
                }

                return hex;
            }
            else
            {
                return format_time_point(_held);
            }
        },
        _value);
}

value
string_to_value(
    const std::string& _str,
    field_type         _type
)
{
    // each type read as its documentation says
    switch (_type)
    {
        case field_type::null:
            return std::monostate();

        case field_type::boolean:
        {
            const struct d_db_value described = text_value(_str);
            bool                    read      = false;

            if ( (_str.find('\0') != std::string::npos) ||
                 (d_db_value_bool(&described, &read) != 0) )
            {
                refuse_value(_str, "a boolean");
            }

            return read;
        }

        case field_type::integer:
        {
            re_std::int64_t read = 0;

            if ( (!read_int64(_str, read))                                 ||
                 (read < std::numeric_limits<re_std::int32_t>::min()) ||
                 (read > std::numeric_limits<re_std::int32_t>::max()) )
            {
                refuse_value(_str, "a 32-bit integer");
            }

            return static_cast<re_std::int32_t>(read);
        }

        case field_type::big_integer:
        {
            re_std::int64_t read = 0;

            if (!read_int64(_str, read))
            {
                refuse_value(_str, "a 64-bit integer");
            }

            return read;
        }

        case field_type::floating_point:
        {
            const std::string word = ascii_lower(_str);
            double            read = 0.0;

            // the words PostgreSQL writes for the values with no digits
            if (word == "nan")
            {
                return std::numeric_limits<double>::quiet_NaN();
            }

            if ( (word == "infinity") ||
                 (word == "+infinity") )
            {
                return std::numeric_limits<double>::infinity();
            }

            if (word == "-infinity")
            {
                return -std::numeric_limits<double>::infinity();
            }

            if (!read_double(_str, read))
            {
                refuse_value(_str, "a number");
            }

            return read;
        }

        case field_type::binary:
        {
            std::vector<re_std::uint8_t> bytes;

            if (!read_hex(_str, bytes))
            {
                refuse_value(_str, "hexadecimal");
            }

            return bytes;
        }

        case field_type::date:
        case field_type::datetime:
        case field_type::timestamp:
        {
            std::chrono::system_clock::time_point point;

            if (!read_time_point(_str, point))
            {
                refuse_value(_str, "a date and time");
            }

            return point;
        }

        case field_type::decimal:
        case field_type::string:
        case field_type::time:
        case field_type::json:
        case field_type::xml:
        case field_type::uuid:
        case field_type::array:
        case field_type::custom:
        default:
            return _str;
    }
}

// 6.4    Names
//------------------------------------------------------------------------------
std::string
get_type_name(
    database_type _type
)
{
    // the names vendor_traits gives, and the reserved vendors'
    switch (_type)
    {
        case database_type::mariadb:    return "MariaDB";
        case database_type::mysql:      return "MySQL";
        case database_type::postgresql: return "PostgreSQL";
        case database_type::sqlite:     return "SQLite";
        case database_type::mongodb:    return "MongoDB";
        case database_type::redis:      return "Redis";
        case database_type::arangodb:   return "ArangoDB";
        case database_type::oracle:     return "Oracle Database";
        case database_type::mssql:      return "Microsoft SQL Server";
        case database_type::db2:        return "Db2";
        case database_type::firebase:   return "Firebase";
        case database_type::cassandra:  return "Cassandra";
        case database_type::couchdb:    return "CouchDB";
        case database_type::neo4j:      return "Neo4j";
        case database_type::dynamodb:   return "DynamoDB";
        case database_type::unknown:
        default:
            return "unknown";
    }
}

std::string
get_field_type_name(
    field_type _type
)
{
    enum d_db_type shared = D_DB_TYPE_NONE;

    // the C layers name what the two vocabularies share
    if (to_layer_type(_type, shared))
    {
        return d_db_type_name(shared);
    }

    // and field_type alone has these
    switch (_type)
    {
        case field_type::null:   return "null";
        case field_type::array:  return "array";
        case field_type::custom: return "custom";
        default:                 return "unknown";
    }
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
