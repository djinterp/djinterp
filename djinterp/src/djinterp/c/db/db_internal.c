/*******************************************************************************
* djinterp [c]                                                     db_internal.c
*
* Implementation of the buffers, list builders, statement formatting and
* number reading db_internal.h declares for the vendor layers.
*   A finished list is a single block: the array at its start, where malloc's
* alignment suits pointers and structs, and the text after it, where nothing
* needs aligning. Rows hold two arrays, the columns and then the values, the
* second placed where its own alignment requires. Offsets and records are
* read back with memcpy, which assumes nothing about the alignment of the
* buffers that hold them.
*   printf and strtod write and read a number's point as the C locale has it,
* which a program's setlocale can make a comma; SQL's is always a `.`. Each
* asks printf which point the locale uses, and translates.
*
*
* path:      /src/djinterp/c/db/db_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./db_internal.h"  // corresponding header
// std
#include <errno.h>   // errno
#include <math.h>    // isfinite, isinf
#include <stdarg.h>  // va_list, va_arg, va_copy, va_end
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free, strtod
#include <string.h>  // memcpy, memmove, strchr, strcmp, strlen, strstr
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, *_MAX


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_db_column_record
//   struct: a column while it is being built, its strings held as offsets into
// the builder's text.
struct d_internal_db_column_record
{
    size_t name;         // offset of the name
    size_t type;         // offset of the declared type
    size_t default_sql;  // offset of the DEFAULT expression, or NO_TEXT
    int    primary_key;  // 0, or the 1-based place in the primary key
    bool   not_null;     // whether the column refuses NULL
    bool   generated;    // whether the database computes it
};

/**
 * @brief Points each column of a packed list at its strings.
 *
 * @param[out] _block    the list's columns, `_count` of them.
 * @param[in]  _text     the list's copy of the builder's text.
 * @param[in]  _records  the builder's records, one per column.
 * @param[in]  _count    how many.
 *
 * Records are read with memcpy, for the reason d_internal_db_names_finish
 * reads offsets that way.
 */
static void
d_internal_db_columns_resolve(
    struct d_db_column_info* _block,
    char*                    _text,
    const char*              _records,
    size_t                   _count
)
{
    for (size_t i = 0u; i < _count; ++i)
    {
        struct d_internal_db_column_record record = { 0u, 0u, 0u, 0, false,
                                                      false };

        memcpy(&record, _records + (i * sizeof(record)), sizeof(record));

        _block[i].name        = _text + record.name;
        _block[i].type        = _text + record.type;
        _block[i].default_sql = (record.default_sql == D_INTERNAL_DB_NO_TEXT)
                                    ? NULL
                                    : _text + record.default_sql;
        _block[i].primary_key = record.primary_key;
        _block[i].not_null    = record.not_null;
        _block[i].generated   = record.generated;
    }

    return;
}

// d_internal_db_field_record
//   struct: a column of rows while they are being built, its strings held as
// offsets into the builder's text.
struct d_internal_db_field_record
{
    size_t         name;       // offset of the name
    size_t         type_name;  // offset of the vendor's type name
    enum d_db_type type;       // the column's type
};

// d_internal_db_value_record
//   struct: a value while rows are being built; text and bytes held as an
// offset into the builder's text.
struct d_internal_db_value_record
{
    enum d_db_value_type type;     // what the value is
    size_t               size;     // the bytes of text or a BLOB
    size_t               offset;   // where they start in the builder's text
    int64_t              integer;  // an INTEGER's value
    double               real;     // a REAL's value
};

// d_internal_db_value_alignment
//   struct: a value after one byte, so that the value's offset in it is the
// alignment a value needs, which C99 has no operator to ask for.
struct d_internal_db_value_alignment
{
    char              pad;    // one byte, which the value must follow
    struct d_db_value value;  // placed as its alignment requires
};

/**
 * @brief Adds a value's record to rows under construction.
 *
 * @param[in,out] _rows    the builder.
 * @param[in]     _record  the record.
 * @return `true`, or `false` when memory ran out, the builder unchanged.
 */
static bool
d_internal_db_rows_add(
    struct d_internal_db_rows*               _rows,
    const struct d_internal_db_value_record* _record
)
{
    // a value with no column to belong to
    if (_rows->column_count == 0u)
    {
        return false;
    }

    // store the record
    if (!d_internal_db_buffer_append(&_rows->values,
                                     _record,
                                     sizeof(*_record)))
    {
        return false;
    }

    ++_rows->value_count;

    return true;
}

/**
 * @brief Adds text or bytes, and their record, to rows under construction.
 *
 * @param[in,out] _rows   the builder.
 * @param[in]     _type   D_DB_VALUE_TEXT or D_DB_VALUE_BLOB.
 * @param[in]     _bytes  the bytes; may be `NULL` when `_size` is 0.
 * @param[in]     _size   how many.
 * @return `true`, or `false` when memory ran out, the builder unchanged.
 *
 * The bytes go in before the record. If the record then cannot be stored,
 * the bytes are trimmed back off, so a failure leaves the builder as it was.
 */
static bool
d_internal_db_rows_bytes(
    struct d_internal_db_rows* _rows,
    enum d_db_value_type       _type,
    const void*                _bytes,
    size_t                     _size
)
{
    const size_t                      length_before = _rows->text.length;
    const char                        terminator    = '\0';
    struct d_internal_db_value_record record        =
    {
        _type, _size, length_before, 0, 0.0
    };

    // the bytes, their terminator, and the record that finds them
    if ( (!d_internal_db_buffer_append(&_rows->text, _bytes, _size))   ||
         (!d_internal_db_buffer_append(&_rows->text, &terminator, 1u)) ||
         (!d_internal_db_rows_add(_rows, &record)) )
    {
        _rows->text.length = length_before;

        return false;
    }

    return true;
}

/**
 * @brief Points each column and value of packed rows at their strings.
 *
 * @param[out] _columns  the rows' columns.
 * @param[out] _values   the rows' values; may be `NULL` when there are none.
 * @param[in]  _text     the rows' copy of the builder's text.
 * @param[in]  _rows     the builder, whose records are read.
 *
 * Records are read with memcpy, for the reason d_internal_db_names_finish
 * reads offsets that way.
 */
static void
d_internal_db_rows_resolve(
    struct d_db_field*               _columns,
    struct d_db_value*               _values,
    const char*                      _text,
    const struct d_internal_db_rows* _rows
)
{
    // the columns, each pointed at its two strings
    for (size_t i = 0u; i < _rows->column_count; ++i)
    {
        struct d_internal_db_field_record record = { 0u, 0u, D_DB_TYPE_NONE };

        memcpy(&record,
               _rows->columns.data + (i * sizeof(record)),
               sizeof(record));

        _columns[i].name      = _text + record.name;
        _columns[i].type      = record.type;
        _columns[i].type_name = _text + record.type_name;
    }

    // the values, text and bytes pointed at their copies
    for (size_t i = 0u; i < _rows->value_count; ++i)
    {
        struct d_internal_db_value_record record =
        {
            D_DB_VALUE_NULL, 0u, 0u, 0, 0.0
        };

        memcpy(&record,
               _rows->values.data + (i * sizeof(record)),
               sizeof(record));

        _values[i].type       = record.type;
        _values[i].size       = 0u;
        _values[i].as.integer = 0;

        switch (record.type)
        {
            case D_DB_VALUE_INTEGER:
                _values[i].as.integer = record.integer;

                break;

            case D_DB_VALUE_REAL:
                _values[i].as.real = record.real;

                break;

            case D_DB_VALUE_TEXT:
                _values[i].size    = record.size;
                _values[i].as.text = _text + record.offset;

                break;

            case D_DB_VALUE_BLOB:
                _values[i].size    = record.size;
                _values[i].as.blob = _text + record.offset;

                break;

            case D_DB_VALUE_NULL:
            default:
                break;
        }
    }

    return;
}

// d_internal_db_length
//   enum: the length modifier a conversion carries.
enum d_internal_db_length
{
    D_INTERNAL_DB_LENGTH_NONE = 0,   // int, unsigned
    D_INTERNAL_DB_LENGTH_LONG,       // l: long, unsigned long
    D_INTERNAL_DB_LENGTH_LONG_LONG,  // ll: long long, unsigned long long
    D_INTERNAL_DB_LENGTH_SIZE        // z: size_t
};

// D_INTERNAL_DB_POINT_ROOM
//   constant: room for the C locale's decimal point and its terminator. The
// point is one character in most locales, and a few bytes of UTF-8 in some.
#define D_INTERNAL_DB_POINT_ROOM 8u

// D_INTERNAL_DB_REAL_ROOM
//   constant: room on the stack for a number read under a locale whose point
// is not `.`; a longer one is copied to the heap. No server writes one this
// long: DECIMAL's widest is 67 characters.
#define D_INTERNAL_DB_REAL_ROOM 128u

/*
d_internal_db_append_text
  A NUL-terminated string, terminator not included.
*/
static bool
d_internal_db_append_text(
    struct d_internal_db_buffer* _out,
    const char*                  _text
)
{
    return d_internal_db_buffer_append(_out, _text, strlen(_text));
}

/*
d_internal_db_append_integer
  Digits are produced from the right, into a buffer sized for the 20 digits
of 2^64 - 1 and a sign, so no printf dialect -- MinGW's msvcrt knows no %lld
-- is involved.
*/
static bool
d_internal_db_append_integer(
    struct d_internal_db_buffer* _out,
    unsigned long long           _magnitude,
    bool                         _negative
)
{
    char   digits[24] = "";
    size_t start      = sizeof(digits);

    // at least one digit, even for zero
    do
    {
        digits[--start] = (char)('0' + (int)(_magnitude % 10u));
        _magnitude     /= 10u;
    } while (_magnitude != 0u);

    // the sign, when there is one
    if (_negative)
    {
        digits[--start] = '-';
    }

    return d_internal_db_buffer_append(_out,
                                       digits + start,
                                       sizeof(digits) - start);
}

/**
 * @brief Finds the decimal point the C locale has printf write.
 *
 * @param[out] _point  receives the point, terminated, in
 *                     D_INTERNAL_DB_POINT_ROOM bytes.
 * @return the point's length in bytes, at least 1.
 *
 * printf is asked to write 0.5, and whatever stands between the 0 and the 5
 * is the point. Unlike localeconv, this is safe to call from any thread. A
 * point that would not fit, which no locale has, reads as `.`.
 */
static size_t
d_internal_db_locale_point(
    char* _point
)
{
    char      probe[2u * D_INTERNAL_DB_POINT_ROOM] = "";
    const int written = snprintf(probe, sizeof(probe), "%.1f", 0.5);

    // "0", the point, "5": any other shape keeps SQL's point
    if ( (written < 3)                                       ||
         ((size_t)written > (D_INTERNAL_DB_POINT_ROOM + 1u)) ||
         (probe[0] != '0')                                   ||
         (probe[written - 1] != '5') )
    {
        memcpy(_point, ".", 2u);

        return 1u;
    }

    const size_t length = (size_t)written - 2u;

    memcpy(_point, probe + 1, length);
    _point[length] = '\0';

    return length;
}

/**
 * @brief Appends a number printf wrote, with SQL's point in place of the
 *        C locale's.
 *
 * @param[in,out] _out     the buffer.
 * @param[in]     _text    the number, as printf wrote it.
 * @param[in]     _length  its length.
 * @return `true`, or `false` when memory ran out.
 */
static bool
d_internal_db_append_number(
    struct d_internal_db_buffer* _out,
    const char*                  _text,
    size_t                       _length
)
{
    char              point[D_INTERNAL_DB_POINT_ROOM] = "";
    const size_t      point_length = d_internal_db_locale_point(point);
    const char* const found        = strstr(_text, point);

    // SQL's point already, or no point at all: as written
    if ( (!found) ||
         (strcmp(point, ".") == 0) )
    {
        return d_internal_db_buffer_append(_out, _text, _length);
    }

    const size_t before = (size_t)(found - _text);

    // the digits before the point, SQL's point, and the rest
    return ( (d_internal_db_buffer_append(_out, _text, before)) &&
             (d_internal_db_buffer_append(_out, ".", 1u))      &&
             (d_internal_db_buffer_append(_out,
                                          found + point_length,
                                          _length - before - point_length)) );
}

/*
d_internal_db_read_unsigned
  The argument is read at the width the modifier names, as va_arg requires.
*/
static unsigned long long
d_internal_db_read_unsigned(
    enum d_internal_db_length _length,
    va_list*                  _args
)
{
    switch (_length)
    {
        case D_INTERNAL_DB_LENGTH_LONG:
            return va_arg(*_args, unsigned long);

        case D_INTERNAL_DB_LENGTH_LONG_LONG:
            return va_arg(*_args, unsigned long long);

        case D_INTERNAL_DB_LENGTH_SIZE:
            return va_arg(*_args, size_t);

        default:
            break;
    }

    return va_arg(*_args, unsigned);
}

/*
d_internal_db_read_signed
  As d_internal_db_read_unsigned, for the signed widths; size_t has none.
*/
static long long
d_internal_db_read_signed(
    enum d_internal_db_length _length,
    va_list*                  _args
)
{
    switch (_length)
    {
        case D_INTERNAL_DB_LENGTH_LONG:
            return va_arg(*_args, long);

        case D_INTERNAL_DB_LENGTH_LONG_LONG:
            return va_arg(*_args, long long);

        default:
            break;
    }

    return va_arg(*_args, int);
}

/*
d_internal_db_format_integer
  %zd and %zi are refused: size_t's signed twin has no name every platform
shares. A negative value's magnitude is taken in unsigned arithmetic, which
the most negative value survives.
*/
static enum d_internal_db_format_status
d_internal_db_format_integer(
    struct d_internal_db_buffer* _out,
    enum d_internal_db_length    _length,
    char                         _conversion,
    va_list*                     _args
)
{
    unsigned long long magnitude = 0u;
    bool               negative  = false;

    // a signed conversion of size_t's width
    if ( (_conversion != 'u') &&
         (_length == D_INTERNAL_DB_LENGTH_SIZE) )
    {
        return D_INTERNAL_DB_FORMAT_BAD;
    }

    // the value, at its width
    if (_conversion == 'u')
    {
        magnitude = d_internal_db_read_unsigned(_length, _args);
    }
    else
    {
        const long long value = d_internal_db_read_signed(_length, _args);

        negative  = (value < 0);
        magnitude = (negative) ? (0u - (unsigned long long)value)
                               : (unsigned long long)value;
    }

    return (d_internal_db_append_integer(_out, magnitude, negative))
               ? D_INTERNAL_DB_FORMAT_OK
               : D_INTERNAL_DB_FORMAT_NO_MEMORY;
}

/*
d_internal_db_format_real
  printf writes a double well enough in every dialect; its point is the C
locale's, which d_internal_db_append_number writes as SQL's. %g keeps 17
significant digits, so that the value read back is the value written. NaN
and the infinities have no SQL spelling, and are refused.
*/
static enum d_internal_db_format_status
d_internal_db_format_real(
    struct d_internal_db_buffer* _out,
    char                         _conversion,
    va_list*                     _args
)
{
    const double value     = va_arg(*_args, double);
    char         text[400] = "";

    // a value SQL cannot spell
    if (!isfinite(value))
    {
        return D_INTERNAL_DB_FORMAT_REFUSED;
    }

    const int written = (_conversion == 'f')
                            ? snprintf(text, sizeof(text), "%f", value)
                            : snprintf(text, sizeof(text), "%.17g", value);

    // DBL_MAX under %f is 317 characters, well inside the buffer
    if ( (written < 0) ||
         ((size_t)written >= sizeof(text)) )
    {
        return D_INTERNAL_DB_FORMAT_REFUSED;
    }

    return (d_internal_db_append_number(_out, text, (size_t)written))
               ? D_INTERNAL_DB_FORMAT_OK
               : D_INTERNAL_DB_FORMAT_NO_MEMORY;
}

/*
d_internal_db_format_text
  %%, %c and %s, written as they are. A NUL character would end the
statement early wherever the statement is read as a string, and is refused.
*/
static enum d_internal_db_format_status
d_internal_db_format_text(
    struct d_internal_db_buffer* _out,
    char                         _conversion,
    va_list*                     _args
)
{
    // a percent sign, which reads no argument
    if (_conversion == '%')
    {
        return (d_internal_db_append_text(_out, "%"))
                   ? D_INTERNAL_DB_FORMAT_OK
                   : D_INTERNAL_DB_FORMAT_NO_MEMORY;
    }

    // a character, which va_arg reads promoted to int
    if (_conversion == 'c')
    {
        const char character = (char)va_arg(*_args, int);

        // NUL cannot stand inside a statement
        if (character == '\0')
        {
            return D_INTERNAL_DB_FORMAT_REFUSED;
        }

        return (d_internal_db_buffer_append(_out, &character, 1u))
                   ? D_INTERNAL_DB_FORMAT_OK
                   : D_INTERNAL_DB_FORMAT_NO_MEMORY;
    }

    const char* const text = va_arg(*_args, const char*);

    return (d_internal_db_append_text(_out, (text) ? text : "NULL"))
               ? D_INTERNAL_DB_FORMAT_OK
               : D_INTERNAL_DB_FORMAT_NO_MEMORY;
}

/*
d_internal_db_format_quoted
  %Q, %I and %T, each through the vendor's quoting. A NULL value is the
keyword NULL; a NULL name cannot be quoted, and is refused. The vendor
leaves the buffer as it was when it fails, so failure here reads as either
memory or a refusal, and memory is the likelier.
*/
static enum d_internal_db_format_status
d_internal_db_format_quoted(
    struct d_internal_db_buffer*        _out,
    const struct d_internal_db_quoting* _quoting,
    char                                _conversion,
    va_list*                            _args
)
{
    const char* const first = va_arg(*_args, const char*);

    // a value: NULL is the keyword
    if (_conversion == 'Q')
    {
        return ( (first) ? _quoting->literal(_quoting->context, _out, first)
                         : d_internal_db_append_text(_out, "NULL") )
                   ? D_INTERNAL_DB_FORMAT_OK
                   : D_INTERNAL_DB_FORMAT_REFUSED;
    }

    const char* const name = (_conversion == 'T')
                                 ? va_arg(*_args, const char*)
                                 : first;
    const char* const qualifier = (_conversion == 'T') ? first : NULL;

    // a name must be there to be quoted
    if (!name)
    {
        return D_INTERNAL_DB_FORMAT_REFUSED;
    }

    // the qualifier and its dot, when there is one
    if ( (qualifier) &&
         ( (!_quoting->identifier(_quoting->context, _out, qualifier)) ||
           (!d_internal_db_append_text(_out, ".")) ) )
    {
        return D_INTERNAL_DB_FORMAT_REFUSED;
    }

    return (_quoting->identifier(_quoting->context, _out, name))
               ? D_INTERNAL_DB_FORMAT_OK
               : D_INTERNAL_DB_FORMAT_REFUSED;
}

/*
d_internal_db_read_length
  The length modifier at the cursor, if there is one; the cursor moves past
it.
*/
static enum d_internal_db_length
d_internal_db_read_length(
    const char** _cursor
)
{
    const char* const spec = *_cursor;

    // ll before l, which it begins with
    if ( (spec[0] == 'l') &&
         (spec[1] == 'l') )
    {
        *_cursor += 2;

        return D_INTERNAL_DB_LENGTH_LONG_LONG;
    }

    // l and z
    if ( (spec[0] == 'l') ||
         (spec[0] == 'z') )
    {
        ++*_cursor;

        return (spec[0] == 'l') ? D_INTERNAL_DB_LENGTH_LONG
                                : D_INTERNAL_DB_LENGTH_SIZE;
    }

    return D_INTERNAL_DB_LENGTH_NONE;
}

/*
d_internal_db_format_conversion
  One conversion: its length modifier, then its letter, which picks the
writer. The cursor moves past whatever was read, so the caller resumes
copying there.
*/
static enum d_internal_db_format_status
d_internal_db_format_conversion(
    struct d_internal_db_buffer*        _out,
    const struct d_internal_db_quoting* _quoting,
    const char**                        _cursor,
    va_list*                            _args
)
{
    const enum d_internal_db_length length = d_internal_db_read_length(_cursor);
    const char                      conversion = **_cursor;

    // past the letter, unless the format ended first
    if (conversion)
    {
        ++*_cursor;
    }

    // a modifier only the integers take
    if ( (length != D_INTERNAL_DB_LENGTH_NONE) &&
         (conversion != 'd')                   &&
         (conversion != 'i')                   &&
         (conversion != 'u') )
    {
        return D_INTERNAL_DB_FORMAT_BAD;
    }

    switch (conversion)
    {
        case 'd':
        case 'i':
        case 'u':
            return d_internal_db_format_integer(_out,
                                                length,
                                                conversion,
                                                _args);

        case 'f':
        case 'g':
            return d_internal_db_format_real(_out, conversion, _args);

        case '%':
        case 'c':
        case 's':
            return d_internal_db_format_text(_out, conversion, _args);

        case 'Q':
        case 'I':
        case 'T':
            return d_internal_db_format_quoted(_out,
                                               _quoting,
                                               conversion,
                                               _args);

        default:
            break;
    }

    return D_INTERNAL_DB_FORMAT_BAD;
}

/**
 * @brief Copies a format's text and writes its conversions, through the
 *        terminator.
 *
 * @param[in,out] _out      the buffer.
 * @param[in]     _quoting  the vendor's quoting.
 * @param[in]     _format   the format.
 * @param[in,out] _args     the arguments, advanced past each one read.
 * @return D_INTERNAL_DB_FORMAT_OK, or why not; the buffer is the caller's to
 *         release either way.
 *
 * The text between conversions is copied a run at a time, and the
 * terminator after the last run.
 */
static enum d_internal_db_format_status
d_internal_db_format_runs(
    struct d_internal_db_buffer*        _out,
    const struct d_internal_db_quoting* _quoting,
    const char*                         _format,
    va_list*                            _args
)
{
    const char*                      run    = _format;
    const char*                      cursor = _format;
    enum d_internal_db_format_status status = D_INTERNAL_DB_FORMAT_OK;

    // each run of text, then the conversion that ends it
    while ( (status == D_INTERNAL_DB_FORMAT_OK) &&
            (*cursor) )
    {
        // text, up to the next conversion
        if (*cursor != '%')
        {
            ++cursor;

            continue;
        }

        status = (d_internal_db_buffer_append(_out,
                                              run,
                                              (size_t)(cursor - run)))
                     ? D_INTERNAL_DB_FORMAT_OK
                     : D_INTERNAL_DB_FORMAT_NO_MEMORY;
        ++cursor;

        // the conversion, when the run went in
        if (status == D_INTERNAL_DB_FORMAT_OK)
        {
            status = d_internal_db_format_conversion(_out,
                                                     _quoting,
                                                     &cursor,
                                                     _args);
        }

        run = cursor;
    }

    // the last run and the terminator
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         ( (!d_internal_db_buffer_append(_out,
                                         run,
                                         (size_t)(cursor - run))) ||
           (!d_internal_db_buffer_append(_out, "", 1u)) ) )
    {
        status = D_INTERNAL_DB_FORMAT_NO_MEMORY;
    }

    return status;
}

/*
d_internal_db_digits
  The run of digits a text starts with, tested by value, which isdigit is
not guaranteed to do in every locale.
*/
static size_t
d_internal_db_digits(
    const char* _text
)
{
    size_t count = 0u;

    while ( (_text[count] >= '0') &&
            (_text[count] <= '9') )
    {
        ++count;
    }

    return count;
}

/**
 * @brief Whether a text is, all of it, a number in the shape a server writes
 *        one in.
 *
 * @param[in] _text  the text.
 * @param[in] _real  whether a fraction and an exponent may follow the digits.
 * @return `true` when it is.
 *
 * The shape is an optional sign; digits, for a real number with an optional
 * `.` and more digits after it, one digit at least; and, for a real number,
 * an optional exponent: `e` or `E`, an optional sign, digits.
 */
static bool
d_internal_db_is_number(
    const char* _text,
    bool        _real
)
{
    const char* at     = _text + ( ( (_text[0] == '-') ||
                                     (_text[0] == '+') ) ? 1 : 0 );
    size_t      digits = d_internal_db_digits(at);

    at += digits;

    // a fraction, for a real number
    if ( (_real) &&
         (*at == '.') )
    {
        const size_t fraction = d_internal_db_digits(at + 1);

        digits += fraction;
        at     += 1u + fraction;
    }

    // an exponent, for a real number with digits: a sign, then digits
    if ( (_real)        &&
         (digits != 0u) &&
         ( (*at == 'e') ||
           (*at == 'E') ) )
    {
        const char* const power = at + ( ( (at[1] == '-') ||
                                           (at[1] == '+') ) ? 2 : 1 );
        const size_t      count = d_internal_db_digits(power);

        at = (count) ? (power + count) : at;
    }

    return ( (digits != 0u) &&
             (*at == '\0') );
}

/*
d_internal_db_read_real
  strtod, on text already checked to be a number in the locale's spelling;
overflow is then the only failure, and strtod reports it with an infinity,
which no checked text spells. An underflow rounds toward zero, as a server's
own reading would. errno, which strtod may set, is left as it was.
*/
static bool
d_internal_db_read_real(
    const char* _text,
    double*     _value
)
{
    const int    saved = errno;
    char*        end   = NULL;
    const double value = strtod(_text, &end);

    errno = saved;

    // the whole text, read to a finite value
    if ( (*end != '\0') ||
         (isinf(value)) )
    {
        return false;
    }

    *_value = value;

    return true;
}

/**
 * @brief Reads a number whose point is SQL's under a locale whose point is
 *        another, through a copy that spells the point the locale's way.
 *
 * @param[in]  _text   the number, checked.
 * @param[in]  _dot    the offset of its `.`.
 * @param[in]  _point  the locale's point.
 * @param[out] _value  receives the value; written only on success.
 * @return `true`, or `false` for a number beyond double's range, or for one
 *         too long for the stack when memory ran out.
 *
 * The copy is on the stack up to D_INTERNAL_DB_REAL_ROOM bytes, which no
 * server's number reaches, and on the heap past it.
 */
static bool
d_internal_db_read_localized(
    const char* _text,
    size_t      _dot,
    const char* _point,
    double*     _value
)
{
    const size_t point_length                  = strlen(_point);
    const size_t length                        = strlen(_text) - 1u +
                                                 point_length;
    char         room[D_INTERNAL_DB_REAL_ROOM] = "";
    char* const  copy = (length < sizeof(room)) ? room
                                                : (char*)malloc(length + 1u);

    // the heap, for a number no server writes
    if (!copy)
    {
        return false;
    }

    // the digits before the point, the locale's point, and the rest
    memcpy(copy, _text, _dot);
    memcpy(copy + _dot, _point, point_length);
    memcpy(copy + _dot + point_length,
           _text + _dot + 1u,
           (length + 1u) - (_dot + point_length));

    const bool read = d_internal_db_read_real(copy, _value);

    // the heap's copy, when there was one
    if (copy != room)
    {
        free(copy);
    }

    return read;
}

//==============================================================================
// 1.  BUFFERS
//==============================================================================

// 1.2    Buffer operations
//------------------------------------------------------------------------------
/*
d_internal_db_buffer_reserve
  Capacity doubles from 64 bytes, so a list of n names costs O(log n)
reallocations. The arithmetic is checked before anything is allocated: a
request that cannot be represented fails rather than wrapping.
*/
bool
d_internal_db_buffer_reserve(
    struct d_internal_db_buffer* _buffer,
    size_t                       _count
)
{
    // refuse a length the arithmetic below could not represent
    if (_count > (SIZE_MAX - _buffer->length))
    {
        return false;
    }

    const size_t needed = _buffer->length + _count;

    // room enough already
    if (needed <= _buffer->capacity)
    {
        return true;
    }

    size_t capacity = (_buffer->capacity) ? _buffer->capacity : 64u;

    // double until the bytes fit, or until doubling would overflow
    while ( (capacity < needed) &&
            (capacity <= (SIZE_MAX / 2u)) )
    {
        capacity *= 2u;
    }

    // when doubling stopped short, ask for exactly what is needed
    if (capacity < needed)
    {
        capacity = needed;
    }

    char* const data = (char*)realloc(_buffer->data, capacity);

    // the old block is still the buffer's when realloc fails
    if (!data)
    {
        return false;
    }

    _buffer->data     = data;
    _buffer->capacity = capacity;

    return true;
}

/*
d_internal_db_buffer_append
  Room first, then the copy, so a failure leaves the buffer as it was.
*/
bool
d_internal_db_buffer_append(
    struct d_internal_db_buffer* _buffer,
    const void*                  _bytes,
    size_t                       _count
)
{
    // nothing to append is a success that touches nothing
    if (_count == 0u)
    {
        return true;
    }

    // room for the bytes
    if (!d_internal_db_buffer_reserve(_buffer, _count))
    {
        return false;
    }

    memcpy(_buffer->data + _buffer->length, _bytes, _count);
    _buffer->length += _count;

    return true;
}

/*
d_internal_db_buffer_add_text
  The offset is taken before the append, so it names the string's first byte
wherever the buffer later moves.
*/
bool
d_internal_db_buffer_add_text(
    struct d_internal_db_buffer* _buffer,
    const char*                  _text,
    size_t*                      _offset
)
{
    // a NULL string is recorded, not stored
    if (!_text)
    {
        *_offset = D_INTERNAL_DB_NO_TEXT;

        return true;
    }

    const size_t offset = _buffer->length;

    // the terminator is part of what is stored
    if (!d_internal_db_buffer_append(_buffer, _text, strlen(_text) + 1u))
    {
        return false;
    }

    *_offset = offset;

    return true;
}

/*
d_internal_db_buffer_release
  Leaves the buffer all zero, which is the empty state, so it can be reused.
*/
void
d_internal_db_buffer_release(
    struct d_internal_db_buffer* _buffer
)
{
    // accept NULL, as free does
    if (_buffer)
    {
        free(_buffer->data);

        _buffer->data     = NULL;
        _buffer->length   = 0u;
        _buffer->capacity = 0u;
    }

    return;
}

//==============================================================================
// 2.  BUILDERS
//==============================================================================

// 2.1    Name lists
//------------------------------------------------------------------------------
/*
d_internal_db_names_add
  The text goes in before the offset. If the offset then cannot be stored, the
text is trimmed back off, so a failure leaves the builder exactly as it was.
*/
bool
d_internal_db_names_add(
    struct d_internal_db_names* _names,
    const char*                 _name
)
{
    const size_t length_before = _names->text.length;
    size_t       offset        = 0u;

    // store the name's text
    if (!d_internal_db_buffer_add_text(&_names->text, _name, &offset))
    {
        return false;
    }

    // store where it starts, or take the text back out
    if (!d_internal_db_buffer_append(&_names->offsets,
                                     &offset,
                                     sizeof(offset)))
    {
        _names->text.length = length_before;

        return false;
    }

    ++_names->count;

    return true;
}

/*
d_internal_db_names_finish
  The block holds count + 1 pointers -- the last one the NULL that ends the
list -- followed by the text. Offsets are read with memcpy rather than through
a cast pointer, which says nothing about the buffer's alignment and so cannot
trip a strict-alignment target.
*/
bool
d_internal_db_names_finish(
    struct d_internal_db_names* _names,
    struct d_db_name_list*      _list
)
{
    const size_t count = _names->count;

    // refuse a list the size arithmetic could not represent
    if ( (count >= (SIZE_MAX / sizeof(char*))) ||
         (_names->text.length > (SIZE_MAX -
                                 ((count + 1u) * sizeof(char*)))) )
    {
        d_internal_db_names_release(_names);

        return false;
    }

    const size_t head  = (count + 1u) * sizeof(char*);
    char** const block = (char**)malloc(head + _names->text.length);

    // the builder's memory goes either way
    if (!block)
    {
        d_internal_db_names_release(_names);

        return false;
    }

    char* const text = (char*)block + head;

    // the text follows the array; an empty list has none
    if (_names->text.length)
    {
        memcpy(text, _names->text.data, _names->text.length);
    }

    // point each entry at its copy of the text
    for (size_t i = 0u; i < count; ++i)
    {
        size_t offset = 0u;

        memcpy(&offset,
               _names->offsets.data + (i * sizeof(offset)),
               sizeof(offset));
        block[i] = text + offset;
    }

    block[count] = NULL;

    _list->count = count;
    _list->names = block;

    d_internal_db_names_release(_names);

    return true;
}

/*
d_internal_db_names_release
  Both buffers go, and the count with them, so the builder is empty again.
*/
void
d_internal_db_names_release(
    struct d_internal_db_names* _names
)
{
    // accept NULL, as free does
    if (_names)
    {
        d_internal_db_buffer_release(&_names->text);
        d_internal_db_buffer_release(&_names->offsets);

        _names->count = 0u;
    }

    return;
}

// 2.2    Column lists
//------------------------------------------------------------------------------
/*
d_internal_db_columns_add
  Three strings and a record: any of the four can fail, so the text length is
remembered first and restored on failure, leaving the builder unchanged.
*/
bool
d_internal_db_columns_add(
    struct d_internal_db_columns*  _columns,
    const struct d_db_column_info* _column
)
{
    const size_t                       length_before = _columns->text.length;
    struct d_internal_db_column_record record        =
    {
        0u, 0u, D_INTERNAL_DB_NO_TEXT, _column->primary_key, _column->not_null,
        _column->generated
    };

    // store the three strings, then the record that finds them
    if ( (!d_internal_db_buffer_add_text(&_columns->text,
                                         _column->name,
                                         &record.name))                 ||
         (!d_internal_db_buffer_add_text(&_columns->text,
                                         (_column->type) ? _column->type
                                                         : "",
                                         &record.type))                 ||
         (!d_internal_db_buffer_add_text(&_columns->text,
                                         _column->default_sql,
                                         &record.default_sql))          ||
         (!d_internal_db_buffer_append(&_columns->records,
                                       &record,
                                       sizeof(record))) )
    {
        _columns->text.length = length_before;

        return false;
    }

    ++_columns->count;

    return true;
}

/*
d_internal_db_columns_finish
  As d_internal_db_names_finish, with column descriptions in place of names
and no terminating entry. An empty list allocates nothing.
*/
bool
d_internal_db_columns_finish(
    struct d_internal_db_columns* _columns,
    struct d_db_column_list*      _list
)
{
    const size_t count = _columns->count;

    // an empty list is NULL, as db_common.h promises
    if (count == 0u)
    {
        d_internal_db_columns_release(_columns);

        _list->count   = 0u;
        _list->columns = NULL;

        return true;
    }

    // refuse a list the size arithmetic could not represent
    if ( (count > (SIZE_MAX / sizeof(struct d_db_column_info))) ||
         (_columns->text.length >
          (SIZE_MAX - (count * sizeof(struct d_db_column_info)))) )
    {
        d_internal_db_columns_release(_columns);

        return false;
    }

    const size_t                   head  = count *
                                           sizeof(struct d_db_column_info);
    struct d_db_column_info* const block =
        (struct d_db_column_info*)malloc(head + _columns->text.length);

    // the builder's memory goes either way
    if (!block)
    {
        d_internal_db_columns_release(_columns);

        return false;
    }

    char* const text = (char*)block + head;

    memcpy(text, _columns->text.data, _columns->text.length);
    d_internal_db_columns_resolve(block, text, _columns->records.data, count);

    _list->count   = count;
    _list->columns = block;

    d_internal_db_columns_release(_columns);

    return true;
}

/*
d_internal_db_columns_release
  Both buffers go, and the count with them, so the builder is empty again.
*/
void
d_internal_db_columns_release(
    struct d_internal_db_columns* _columns
)
{
    // accept NULL, as free does
    if (_columns)
    {
        d_internal_db_buffer_release(&_columns->text);
        d_internal_db_buffer_release(&_columns->records);

        _columns->count = 0u;
    }

    return;
}

// 2.3    Rows
//------------------------------------------------------------------------------
/*
d_internal_db_rows_column
  Two strings and a record, any of which can fail, so the text length is
remembered first and restored on failure. A column described after a value
would shift every row, so it is refused.
*/
bool
d_internal_db_rows_column(
    struct d_internal_db_rows* _rows,
    const char*                _name,
    enum d_db_type             _type,
    const char*                _type_name
)
{
    const size_t                      length_before = _rows->text.length;
    struct d_internal_db_field_record record        = { 0u, 0u, _type };

    // the columns come first, all of them
    if (_rows->value_count != 0u)
    {
        return false;
    }

    // store the two strings, then the record that finds them
    if ( (!d_internal_db_buffer_add_text(&_rows->text,
                                         (_name) ? _name : "",
                                         &record.name))               ||
         (!d_internal_db_buffer_add_text(&_rows->text,
                                         (_type_name) ? _type_name : "",
                                         &record.type_name))          ||
         (!d_internal_db_buffer_append(&_rows->columns,
                                       &record,
                                       sizeof(record))) )
    {
        _rows->text.length = length_before;

        return false;
    }

    ++_rows->column_count;

    return true;
}

/*
d_internal_db_rows_null
  A record and nothing else.
*/
bool
d_internal_db_rows_null(
    struct d_internal_db_rows* _rows
)
{
    const struct d_internal_db_value_record record =
    {
        D_DB_VALUE_NULL, 0u, 0u, 0, 0.0
    };

    return d_internal_db_rows_add(_rows, &record);
}

/*
d_internal_db_rows_integer
  A record holding the value.
*/
bool
d_internal_db_rows_integer(
    struct d_internal_db_rows* _rows,
    int64_t                    _value
)
{
    const struct d_internal_db_value_record record =
    {
        D_DB_VALUE_INTEGER, 0u, 0u, _value, 0.0
    };

    return d_internal_db_rows_add(_rows, &record);
}

/*
d_internal_db_rows_real
  A record holding the value.
*/
bool
d_internal_db_rows_real(
    struct d_internal_db_rows* _rows,
    double                     _value
)
{
    const struct d_internal_db_value_record record =
    {
        D_DB_VALUE_REAL, 0u, 0u, 0, _value
    };

    return d_internal_db_rows_add(_rows, &record);
}

/*
d_internal_db_rows_text
  The text's bytes and a terminator, which the size does not count.
*/
bool
d_internal_db_rows_text(
    struct d_internal_db_rows* _rows,
    const char*                _text,
    size_t                     _size
)
{
    return d_internal_db_rows_bytes(_rows, D_DB_VALUE_TEXT, _text, _size);
}

/*
d_internal_db_rows_blob
  As d_internal_db_rows_text: the terminator costs one byte and lets bytes
that happen to be text read as a C string.
*/
bool
d_internal_db_rows_blob(
    struct d_internal_db_rows* _rows,
    const void*                _bytes,
    size_t                     _size
)
{
    return d_internal_db_rows_bytes(_rows, D_DB_VALUE_BLOB, _bytes, _size);
}

/*
d_internal_db_rows_finish
  The block holds the columns, then the values at the offset their alignment
requires -- which on a 32-bit target need not follow the columns directly --
and then the text. It starts with the columns, so d_db_rows_free releases it
through `columns`. Rows without columns allocate nothing.
*/
bool
d_internal_db_rows_finish(
    struct d_internal_db_rows* _rows,
    struct d_db_rows*          _out
)
{
    const size_t columns   = _rows->column_count;
    const size_t values    = _rows->value_count;
    const size_t alignment = offsetof(struct d_internal_db_value_alignment,
                                      value);

    // a last row cut short
    if ( (columns != 0u) &&
         ((values % columns) != 0u) )
    {
        d_internal_db_rows_release(_rows);

        return false;
    }

    // no columns: empty rows, as db_common.h promises
    if (columns == 0u)
    {
        d_internal_db_rows_release(_rows);

        _out->column_count = 0u;
        _out->row_count    = 0u;
        _out->columns      = NULL;
        _out->values       = NULL;

        return true;
    }

    // refuse rows the size arithmetic could not represent
    if ( (columns > ((SIZE_MAX / sizeof(struct d_db_field)) - alignment)) ||
         (values > (SIZE_MAX / sizeof(struct d_db_value))) )
    {
        d_internal_db_rows_release(_rows);

        return false;
    }

    const size_t field_bytes = columns * sizeof(struct d_db_field);
    const size_t value_at    = ((field_bytes + alignment - 1u) / alignment) *
                               alignment;
    const size_t value_bytes = values * sizeof(struct d_db_value);

    // and the sum of the three parts
    if ( (value_bytes > (SIZE_MAX - value_at)) ||
         (_rows->text.length > (SIZE_MAX - (value_at + value_bytes))) )
    {
        d_internal_db_rows_release(_rows);

        return false;
    }

    char* const block = (char*)malloc(value_at + value_bytes +
                                      _rows->text.length);

    // the builder's memory goes either way
    if (!block)
    {
        d_internal_db_rows_release(_rows);

        return false;
    }

    struct d_db_field* const field_block = (struct d_db_field*)block;
    struct d_db_value* const value_block =
        (values) ? (struct d_db_value*)(block + value_at) : NULL;
    char* const              text        = block + value_at + value_bytes;

    memcpy(text, _rows->text.data, _rows->text.length);
    d_internal_db_rows_resolve(field_block, value_block, text, _rows);

    _out->column_count = columns;
    _out->row_count    = values / columns;
    _out->columns      = field_block;
    _out->values       = value_block;

    d_internal_db_rows_release(_rows);

    return true;
}

/*
d_internal_db_rows_release
  The three buffers go, and the counts with them, so the builder is empty
again.
*/
void
d_internal_db_rows_release(
    struct d_internal_db_rows* _rows
)
{
    // accept NULL, as free does
    if (_rows)
    {
        d_internal_db_buffer_release(&_rows->text);
        d_internal_db_buffer_release(&_rows->columns);
        d_internal_db_buffer_release(&_rows->values);

        _rows->column_count = 0u;
        _rows->value_count  = 0u;
    }

    return;
}

//==============================================================================
// 3.  STATEMENT TEXT
//==============================================================================

// 3.2    Formatting
//------------------------------------------------------------------------------
/*
d_internal_db_vformat
  The arguments are copied first, since a va_list parameter's address is not
portably a va_list*, and the writers need one to advance it. The terminator
is written, then left out of the length.
*/
enum d_internal_db_format_status
d_internal_db_vformat(
    struct d_internal_db_buffer*        _out,
    const struct d_internal_db_quoting* _quoting,
    const char*                         _format,
    va_list                             _args
)
{
    va_list args;

    va_copy(args, _args);

    const enum d_internal_db_format_status status =
        d_internal_db_format_runs(_out, _quoting, _format, &args);

    va_end(args);

    // the text is the caller's only when it is whole
    if (status != D_INTERNAL_DB_FORMAT_OK)
    {
        d_internal_db_buffer_release(_out);

        return status;
    }

    --_out->length;

    return D_INTERNAL_DB_FORMAT_OK;
}

/*
d_internal_db_format
  The variadic face of d_internal_db_vformat.
*/
enum d_internal_db_format_status
d_internal_db_format(
    struct d_internal_db_buffer*        _out,
    const struct d_internal_db_quoting* _quoting,
    const char*                         _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const enum d_internal_db_format_status status =
        d_internal_db_vformat(_out, _quoting, _format, args);

    va_end(args);

    return status;
}

//==============================================================================
// 4.  VALUES
//==============================================================================

// 4.1    Numbers
//------------------------------------------------------------------------------
/*
d_internal_db_parse_integer
  Exact: the digits are accumulated in unsigned 64-bit arithmetic against
the limit the sign allows, and a digit that would carry past it fails before
anything wraps.
*/
bool
d_internal_db_parse_integer(
    const char* _text,
    int64_t*    _value
)
{
    // the shape: a sign, digits, and nothing else
    if ( (!_text) ||
         (!d_internal_db_is_number(_text, false)) )
    {
        return false;
    }

    const bool     negative  = (_text[0] == '-');
    const uint64_t limit     = (negative) ? ((uint64_t)INT64_MAX + 1u)
                                          : (uint64_t)INT64_MAX;
    const char*    digit     = _text + ( ( (negative) ||
                                           (_text[0] == '+') ) ? 1 : 0 );
    uint64_t       magnitude = 0u;

    // each digit, unless it would carry the value past the limit
    for (; *digit; ++digit)
    {
        const unsigned value = (unsigned)(*digit - '0');

        if (magnitude > ((limit - value) / 10u))
        {
            return false;
        }

        magnitude = (magnitude * 10u) + value;
    }

    // the most negative value has no positive twin: built from one less
    if ( (negative) &&
         (magnitude != 0u) )
    {
        *_value = -(int64_t)(magnitude - 1u) - 1;
    }
    else
    {
        *_value = (int64_t)magnitude;
    }

    return true;
}

/*
d_internal_db_parse_real
  The text is checked first, since strtod alone would also take leading
spaces, "inf" and C99's hexadecimal. strtod reads the C locale's point, so
under a locale whose point is not `.` a copy is read in which it is.
*/
bool
d_internal_db_parse_real(
    const char* _text,
    double*     _value
)
{
    // the shape a server writes, and nothing else
    if ( (!_text) ||
         (!d_internal_db_is_number(_text, true)) )
    {
        return false;
    }

    char              point[D_INTERNAL_DB_POINT_ROOM] = "";
    const char* const dot = strchr(_text, '.');

    d_internal_db_locale_point(point);

    // a point the locale spells another way
    if ( (dot) &&
         (strcmp(point, ".") != 0) )
    {
        return d_internal_db_read_localized(_text,
                                            (size_t)(dot - _text),
                                            point,
                                            _value);
    }

    return d_internal_db_read_real(_text, _value);
}

//==============================================================================
// 5.  COLUMN DEFINITIONS
//==============================================================================

// 5.1    Primary keys
//------------------------------------------------------------------------------
/*
d_internal_db_key_places_ok
  A place counted for each key column, and none for any other: with as many
places as key columns, none beyond the count and none repeated, the places
are 1 to the count. A repeat is found by comparing each place with those
before it, which a column set's size keeps cheap.
*/
bool
d_internal_db_key_places_ok(
    const struct d_db_column* _columns,
    size_t                    _count
)
{
    size_t keys   = 0u;
    size_t placed = 0u;

    // the key's columns, and the columns that have a place
    for (size_t i = 0u; i < _count; ++i)
    {
        const bool key = ((_columns[i].flags & D_DB_COLUMN_PRIMARY_KEY) != 0u);

        // a place off the key
        if ( (!key) &&
             (_columns[i].key_place != 0u) )
        {
            return false;
        }

        keys   += (key) ? 1u : 0u;
        placed += (_columns[i].key_place != 0u) ? 1u : 0u;
    }

    // no places at all: the columns' order
    if (placed == 0u)
    {
        return true;
    }

    // places for some key columns and not others
    if (placed != keys)
    {
        return false;
    }

    // each place within the count, and given once
    for (size_t i = 0u; i < _count; ++i)
    {
        const unsigned place = _columns[i].key_place;

        if ((size_t)place > keys)
        {
            return false;
        }

        for (size_t j = 0u; (place != 0u) && (j < i); ++j)
        {
            if (_columns[j].key_place == place)
            {
                return false;
            }
        }
    }

    return true;
}

/*
d_internal_db_key_column
  Whether the set numbers its key is asked first: then the place is the
number, and otherwise the count of key columns met so far.
*/
size_t
d_internal_db_key_column(
    const struct d_db_column* _columns,
    size_t                    _count,
    size_t                    _place
)
{
    size_t seen   = 0u;
    bool   placed = false;

    // whether the set numbers its key at all
    for (size_t i = 0u; (!placed) && (i < _count); ++i)
    {
        placed = (_columns[i].key_place != 0u);
    }

    for (size_t i = 0u; i < _count; ++i)
    {
        // the column numbered so, or the place-th key column in order
        if ( (placed) &&
             ((size_t)_columns[i].key_place == _place) )
        {
            return i;
        }

        if ( (!placed) &&
             ((_columns[i].flags & D_DB_COLUMN_PRIMARY_KEY) != 0u) &&
             (++seen == _place) )
        {
            return i;
        }
    }

    return _count;
}
