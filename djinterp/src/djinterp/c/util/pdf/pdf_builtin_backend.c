/*******************************************************************************
* djinterp [c]                                             pdf_builtin_backend.c
*
* The built-in writer's serialization primitives.
*   Defines what pdf_builtin_backend.h declares, byte for byte with the C++
* helpers in pdf_builtin_backend.hpp. Every emitter follows snprintf: it
* writes at most `_out_capacity - 1` bytes and a terminator when there is any
* room, and returns the length the whole result needs, so a NULL buffer
* measures. Numbers are written by d_ostream_write_double
* (c/text/ostream_format.h), as "%g" writes them in the C locale whatever the
* program's locale (the decimal-point ruling, stream 1), onto a stream over
* the caller's buffer (d_ostream_init_fixed), whose contract is snprintf's.
*
*
* path:      /src/djinterp/c/util/pdf/pdf_builtin_backend.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/pdf/pdf_builtin_backend.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // snprintf
#include <string.h>  // memcpy
#include <time.h>    // time, localtime, gmtime, strftime
// djinterp
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
                                    // d_ostream_init_fixed, d_ostream_write
#include "../../../../../inc/djinterp/c/text/ostream_format.h"
                                    // d_ostream_write_double
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t, int64_t


/*
d_internal_pdf_put
  Appends one byte at *_length if it fits below the terminator, and counts it
whether or not it did.
*/
static
void d_internal_pdf_put(
    char*   _out,
    size_t  _out_capacity,
    size_t* _length,
    char    _c
)
{
    if ( (_out != NULL) &&
         ((*_length + 1u) < _out_capacity) )
    {
        _out[*_length] = _c;
    }

    ++*_length;

    return;
}

/*
d_internal_pdf_terminate
  Terminates what was written: at the full length when it fit, at the last
byte of the buffer when it did not.
*/
static
void d_internal_pdf_terminate(
    char*  _out,
    size_t _out_capacity,
    size_t _length
)
{
    if ( (_out != NULL) &&
         (_out_capacity > 0u) )
    {
        _out[(_length < _out_capacity) ? _length : (_out_capacity - 1u)] = '\0';
    }

    return;
}

/*
d_pdf_escape_text
  The C++ escape: a backslash before ( ) and \, printable ASCII as itself,
and every other byte as a three-digit octal escape.
*/
size_t d_pdf_escape_text(
    const char* _text,
    size_t      _length,
    char*       _out,
    size_t      _out_capacity
)
{
    size_t length = 0u;

    // escape each byte
    for (size_t i = 0u; (_text != NULL) && (i < _length); ++i)
    {
        const unsigned char c = (unsigned char)_text[i];

        if ( (c == '(') ||
             (c == ')') ||
             (c == '\\') )
        {
            d_internal_pdf_put(_out, _out_capacity, &length, '\\');
            d_internal_pdf_put(_out, _out_capacity, &length, (char)c);
        }
        else if ( (c >= 0x20u) &&
                  (c <= 0x7Eu) )
        {
            d_internal_pdf_put(_out, _out_capacity, &length, (char)c);
        }
        else
        {
            d_internal_pdf_put(_out, _out_capacity, &length, '\\');
            d_internal_pdf_put(_out, _out_capacity, &length,
                               (char)('0' + ((c >> 6) & 0x3u)));
            d_internal_pdf_put(_out, _out_capacity, &length,
                               (char)('0' + ((c >> 3) & 0x7u)));
            d_internal_pdf_put(_out, _out_capacity, &length,
                               (char)('0' + (c & 0x7u)));
        }
    }

    d_internal_pdf_terminate(_out, _out_capacity, length);

    return length;
}

/*
d_internal_pdf_print_length
  An snprintf result as a length, with an encoding error read as nothing.
*/
static
size_t d_internal_pdf_print_length(
    int _written
)
{
    return (_written > 0) ? (size_t)_written : 0u;
}

/*
d_internal_pdf_fixed
  Opens a stream over the caller's buffer with snprintf's contract: a NULL
buffer, or no room, only counts.
*/
static
void d_internal_pdf_fixed(
    struct dj_ostream*       _os,
    struct dj_ostream_fixed* _fixed,
    char*                    _out,
    size_t                   _out_capacity
)
{
    (void)d_ostream_init_fixed(_os,
                               _fixed,
                               _out,
                               (_out != NULL) ? _out_capacity : 0u,
                               1);

    return;
}

/*
d_internal_pdf_finish
  Closes a stream opened by d_internal_pdf_fixed and returns the length the
whole output needed, terminator excluded, which a fixed stream counts past
its array.
*/
static
size_t d_internal_pdf_finish(
    struct dj_ostream* _os
)
{
    const size_t needed = d_ostream_written(_os);

    (void)d_ostream_close(_os);

    return needed;
}

/*
d_pdf_num
  "%g"'s bytes in the C locale, through d_ostream_write_double: 'g' with
printf's default precision, and the decimal point always '.'.
*/
size_t d_pdf_num(
    double _value,
    char*  _out,
    size_t _out_capacity
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    d_internal_pdf_fixed(&os, &fixed, _out, _out_capacity);
    (void)d_ostream_write_double(&os, _value, 'g', -1);

    return d_internal_pdf_finish(&os);
}

/*
d_internal_pdf_numbers
  Writes _count numbers as d_pdf_num writes each, a space after every one.
*/
static
void d_internal_pdf_numbers(
    struct dj_ostream*  _os,
    const double* const _values,
    size_t              _count
)
{
    for (size_t i = 0u; i < _count; ++i)
    {
        (void)d_ostream_write_double(_os, _values[i], 'g', -1);
        (void)d_ostream_write_char(_os, ' ');
    }

    return;
}

/*
d_pdf_set_color
  The C++ operators: "v g" or "v G" for gray (its r channel), "c m y k k" or
"K" for cmyk, and "r g b rg" or "RG" for rgb and any other space. A missing
colour writes nothing.
*/
size_t d_pdf_set_color(
    const struct dj_pdf_color* _color,
    int32_t                    _fill,
    char*                      _out,
    size_t                     _out_capacity
)
{
    // no colour, no operator
    if (!_color)
    {
        d_internal_pdf_terminate(_out, _out_capacity, 0u);

        return 0u;
    }

    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    d_internal_pdf_fixed(&os, &fixed, _out, _out_capacity);

    switch (_color->space)
    {
        case D_PDF_COLOR_GRAY:
        {
            const double v[1] = { _color->r };

            d_internal_pdf_numbers(&os, v, 1u);
            (void)d_ostream_write_cstr(&os, _fill ? "g\n" : "G\n");

            break;
        }

        case D_PDF_COLOR_CMYK:
        {
            const double v[4] = { _color->c, _color->m, _color->y, _color->k };

            d_internal_pdf_numbers(&os, v, 4u);
            (void)d_ostream_write_cstr(&os, _fill ? "k\n" : "K\n");

            break;
        }

        default:
        {
            const double v[3] = { _color->r, _color->g, _color->b };

            d_internal_pdf_numbers(&os, v, 3u);
            (void)d_ostream_write_cstr(&os, _fill ? "rg\n" : "RG\n");

            break;
        }
    }

    return d_internal_pdf_finish(&os);
}

/*
d_pdf_format_date
  "D:YYYYMMDDHHmmSS" for the given instant, in local time or UTC. A time that
does not fit the platform's time_t, or that the C library cannot break
down, yields nothing. localtime and gmtime share static storage in C99, so
this is not safe to call from two threads at once.
*/
size_t d_pdf_format_date(
    int64_t _unix_seconds,
    int32_t _use_local_time,
    char*   _out,
    size_t  _out_capacity
)
{
    const time_t seconds = (time_t)_unix_seconds;
    char         stamp[D_PDF_DATE_MAX + 1];

    d_internal_pdf_terminate(_out, _out_capacity, 0u);

    // the instant must survive the conversion to time_t
    if ((int64_t)seconds != _unix_seconds)
    {
        return 0u;
    }

    const struct tm* const parts = _use_local_time ? localtime(&seconds)
                                                   : gmtime(&seconds);

    if ( (!parts) ||
         (strftime(stamp, sizeof(stamp), "D:%Y%m%d%H%M%S", parts) !=
          D_PDF_DATE_MAX) )
    {
        return 0u;
    }

    // copy what fits, and terminate
    if ( (_out != NULL) &&
         (_out_capacity > 0u) )
    {
        const size_t copy = (D_PDF_DATE_MAX < _out_capacity)
                                ? (size_t)D_PDF_DATE_MAX
                                : (_out_capacity - 1u);

        memcpy(_out, stamp, copy);
        _out[copy] = '\0';
    }

    return (size_t)D_PDF_DATE_MAX;
}

size_t d_pdf_creation_date(
    char*  _out,
    size_t _out_capacity
)
{
    return d_pdf_format_date((int64_t)time(NULL), 1, _out, _out_capacity);
}

/*
d_pdf_begin_object
  Records the object's byte offset -- `_out_length`, where its marker starts
-- in `_offsets[_object_number]` when that slot exists, and writes
"N 0 obj\n" at `_out + _out_length` under the snprintf rule for the room
left there. Returns the marker's length; a negative object number is no
object, and returns 0.
*/
size_t d_pdf_begin_object(
    char*   _out,
    size_t  _out_capacity,
    size_t  _out_length,
    size_t* _offsets,
    size_t  _offset_count,
    int32_t _object_number
)
{
    // there is no object below 0
    if (_object_number < 0)
    {
        return 0u;
    }

    // record where the object starts
    if ( (_offsets != NULL) &&
         ((size_t)_object_number < _offset_count) )
    {
        _offsets[_object_number] = _out_length;
    }

    const int has_room = ( (_out != NULL) &&
                           (_out_length < _out_capacity) );

    return d_internal_pdf_print_length(
        snprintf(has_room ? (_out + _out_length) : NULL,
                 has_room ? (_out_capacity - _out_length) : 0u,
                 "%d 0 obj\n",
                 (int)_object_number));
}
