#include "../../../../../inc/djinterp/c/util/pdf/pdf_builtin_backend.h"

#include <stdio.h>
#include <time.h>


/*
append_helper
  Copies one byte into a caller buffer if it fits, and counts it either way.

  COUNTS UNCONDITIONALLY. Every emitter here returns the length it NEEDED, so a
caller can size with a null buffer and retry. That only works if the count
advances whether or not the byte was stored, which is snprintf's contract and
the reason this helper exists rather than a bounds test at each call site --
one forgotten test there turns a size query into a wrong answer.

Parameter(s):
  _out:          the caller buffer; may be NULL when only the length is wanted.
  _out_capacity: its size in bytes, including room for a terminator.
  _length:       how many bytes have been counted so far.
  _c:            the byte.
Return:
  none.
*/
static void
append_helper
(
    char*   _out,
    size_t  _out_capacity,
    size_t* _length,
    char    _c
)
{
    /*   The reserved slot is the terminator's. Writing at (capacity - 1) would
       fill the buffer and leave nowhere to terminate it. */
    if (_out && ((*_length + 1u) < _out_capacity))
    {
        _out[*_length] = _c;
    }

    *_length += 1u;

    return;
}

/*
terminate_helper
  NUL-terminates a caller buffer at the shorter of the written length and the
capacity.

Parameter(s):
  _out:          the caller buffer; may be NULL.
  _out_capacity: its size in bytes.
  _length:       the length that was needed, which may exceed the capacity.
Return:
  none.
*/
static void
terminate_helper
(
    char*  _out,
    size_t _out_capacity,
    size_t _length
)
{
    if ( (!_out) || (_out_capacity == 0u) )
    {
        return;
    }

    _out[(_length < (_out_capacity - 1u)) ? _length : (_out_capacity - 1u)] =
        '\0';

    return;
}


/* =========================================================================
   I.     escaping
   ========================================================================= */

/*
d_pdf_escape_text
  Escapes a run for a PDF literal string.

  THREE CLASSES, IN THIS ORDER: '(', ')' and '\\' take a backslash; printable
ASCII (0x20..0x7e) passes through; everything else becomes a three-digit octal
escape. The order matters -- '(' is printable, so testing the printable range
first would pass an unescaped parenthesis straight into the stream and end the
string early, which is a malformed PDF rather than a mangled one.

  OCTAL, NOT HEX, and three digits always. A PDF literal string's escape is
octal by specification, and a shorter escape followed by a digit would be
misread: "\\1" then '2' reads as "\\12", a different byte.

Parameter(s):
  _text:         the bytes; may be NULL only when _length is 0.
  _length:       how many.
  _out:          caller buffer; may be NULL to query the length.
  _out_capacity: its size, including the terminator.
Return:
  the length the escaped form needs, excluding the terminator. When that is
less than _out_capacity the buffer holds the whole result; otherwise it holds a
truncated, terminated prefix and the caller should retry with the returned
size plus one.
*/
size_t
d_pdf_escape_text
(
    const char* _text,
    size_t      _length,
    char*       _out,
    size_t      _out_capacity
)
{
    size_t _n = 0;
    size_t _i = 0;

    if ( (!_text) || (_length == 0u) )
    {
        terminate_helper(_out, _out_capacity, 0u);

        return 0u;
    }

    for (_i = 0; _i < _length; ++_i)
    {
        unsigned char _c = (unsigned char)_text[_i];

        if ( (_c == '(')  ||
             (_c == ')')  ||
             (_c == '\\') )
        {
            append_helper(_out, _out_capacity, &_n, '\\');
            append_helper(_out, _out_capacity, &_n, (char)_c);

            continue;
        }

        if ( (_c >= 0x20) &&
             (_c <= 0x7e) )
        {
            append_helper(_out, _out_capacity, &_n, (char)_c);

            continue;
        }

        append_helper(_out, _out_capacity, &_n, '\\');
        append_helper(_out, _out_capacity, &_n,
                      (char)('0' + ((_c >> 6) & 0x7)));
        append_helper(_out, _out_capacity, &_n,
                      (char)('0' + ((_c >> 3) & 0x7)));
        append_helper(_out, _out_capacity, &_n,
                      (char)('0' + (_c & 0x7)));
    }

    terminate_helper(_out, _out_capacity, _n);

    return _n;
}


/* =========================================================================
   II.    scalars
   ========================================================================= */

/*
d_pdf_num
  Formats a scalar for content-stream emission.

  "%g", MATCHING THE C++ SIDE, AND LOCALE-DEPENDENT BECAUSE OF IT. See the
header's note: under a locale whose LC_NUMERIC uses a comma, this emits "1,5"
and the resulting PDF is rejected by every reader. Emitting a locale-
independent form here alone would make the two tiers disagree precisely where
it matters and hide the disagreement from a differential run under the C
locale, so both stay wrong in the same detectable way until a ruling fixes
both.

Parameter(s):
  _value:        the scalar.
  _out:          caller buffer; may be NULL to query the length.
  _out_capacity: its size, including the terminator.
Return:
  the length needed, excluding the terminator.
*/
size_t
d_pdf_num
(
    double _value,
    char*  _out,
    size_t _out_capacity
)
{
    char _buf[D_PDF_NUM_MAX + 1];
    int  _written = 0;

    /*   Formatted into a local of known size first, then copied. snprintf
       straight into the caller's buffer would be shorter, but its return is
       the needed length only when the buffer is large enough on every libc a
       caller might be on -- and a NULL buffer with a non-zero size is
       undefined rather than a size query. */
    _written = snprintf(_buf, sizeof(_buf), "%g", _value);

    if (_written < 0)
    {
        terminate_helper(_out, _out_capacity, 0u);

        return 0u;
    }

    {
        size_t _n = (size_t)_written;
        size_t _i = 0;

        if (_n > D_PDF_NUM_MAX)
        {
            _n = D_PDF_NUM_MAX;     /* snprintf truncated into _buf as well */
        }

        for (_i = 0; _i < _n; ++_i)
        {
            if (_out && ((_i + 1u) < _out_capacity))
            {
                _out[_i] = _buf[_i];
            }
        }

        terminate_helper(_out, _out_capacity, _n);

        return _n;
    }
}


/* =========================================================================
   III.   colour operators
   ========================================================================= */

/*
append_num_helper
  Formats a scalar and appends it, counting whether or not it fit.

Parameter(s):
  _out:          the caller buffer; may be NULL.
  _out_capacity: its size.
  _length:       the running length.
  _value:        the scalar.
Return:
  none.
*/
static void
append_num_helper
(
    char*   _out,
    size_t  _out_capacity,
    size_t* _length,
    double  _value
)
{
    char   _buf[D_PDF_NUM_MAX + 1];
    size_t _n = d_pdf_num(_value, _buf, sizeof(_buf));
    size_t _i = 0;

    for (_i = 0; _i < _n; ++_i)
    {
        append_helper(_out, _out_capacity, _length, _buf[_i]);
    }

    return;
}

/*
append_literal_helper
  Appends a NUL-terminated literal, counting whether or not it fit.

Parameter(s):
  _out:          the caller buffer; may be NULL.
  _out_capacity: its size.
  _length:       the running length.
  _text:         the literal.
Return:
  none.
*/
static void
append_literal_helper
(
    char*       _out,
    size_t      _out_capacity,
    size_t*     _length,
    const char* _text
)
{
    size_t _i = 0;

    if (!_text)
    {
        return;
    }

    while (_text[_i] != '\0')
    {
        append_helper(_out, _out_capacity, _length, _text[_i]);
        ++_i;
    }

    return;
}

/*
d_pdf_set_color
  The content-stream operator selecting a colour as fill or stroke.

  THE OPERATOR'S CASE CARRIES THE MEANING: lower-case sets the fill colour,
upper-case the stroke. "g"/"G", "rg"/"RG", "k"/"K". They are otherwise
identical, so a transposed case produces a valid stream that paints the wrong
thing -- which renders, and is therefore not caught by anything that only asks
whether the output parses.

  EMITTED IN THE COLOUR'S OWN DEVICE SPACE, not converted to RGB. A cmyk colour
emits "k" with four channels; folding it to the rgb view would change the
document's colour model and, on a press, its output.

Parameter(s):
  _color:        the colour; NULL emits nothing and returns 0.
  _fill:         non-zero for the fill operator, zero for stroke.
  _out:          caller buffer; may be NULL to query the length.
  _out_capacity: its size, including the terminator.
Return:
  the length needed, excluding the terminator. The emitted run ends with a
newline, as the C++ side's does -- a content stream is line-oriented and the
caller concatenates without separators.
*/
size_t
d_pdf_set_color
(
    const struct d_pdf_color* _color,
    int32_t                   _fill,
    char*                     _out,
    size_t                    _out_capacity
)
{
    size_t _n = 0;

    if (!_color)
    {
        terminate_helper(_out, _out_capacity, 0u);

        return 0u;
    }

    switch (_color->space)
    {
        case D_PDF_COLOR_GRAY:
        {
            append_num_helper(_out, _out_capacity, &_n, _color->r);
            append_literal_helper(_out, _out_capacity, &_n,
                                  _fill ? " g\n" : " G\n");
            break;
        }

        case D_PDF_COLOR_CMYK:
        {
            append_num_helper(_out, _out_capacity, &_n, _color->c);
            append_literal_helper(_out, _out_capacity, &_n, " ");
            append_num_helper(_out, _out_capacity, &_n, _color->m);
            append_literal_helper(_out, _out_capacity, &_n, " ");
            append_num_helper(_out, _out_capacity, &_n, _color->y);
            append_literal_helper(_out, _out_capacity, &_n, " ");
            append_num_helper(_out, _out_capacity, &_n, _color->k);
            append_literal_helper(_out, _out_capacity, &_n,
                                  _fill ? " k\n" : " K\n");
            break;
        }

        case D_PDF_COLOR_RGB:
        default:
        {
            /*   RGB IS THE DEFAULT ARM, matching the C++ switch. An
               unrecognised space emits rgb rather than nothing, because a
               content stream missing a colour operator inherits whatever was
               set last -- a wrong colour that looks deliberate. */
            append_num_helper(_out, _out_capacity, &_n, _color->r);
            append_literal_helper(_out, _out_capacity, &_n, " ");
            append_num_helper(_out, _out_capacity, &_n, _color->g);
            append_literal_helper(_out, _out_capacity, &_n, " ");
            append_num_helper(_out, _out_capacity, &_n, _color->b);
            append_literal_helper(_out, _out_capacity, &_n,
                                  _fill ? " rg\n" : " RG\n");
            break;
        }
    }

    terminate_helper(_out, _out_capacity, _n);

    return _n;
}


/* =========================================================================
   IV.    dates
   ========================================================================= */

/*
d_pdf_format_date
  Formats a Unix timestamp as a PDF date string.

  SEPARATE FROM d_pdf_creation_date SO A BUILD CAN BE REPRODUCIBLE. Stamping
the wall clock makes two builds of one document differ, which defeats
byte-comparison of output; a caller that wants determinism passes its own
timestamp here.

  strftime IS NOT USED. Its %Y and %m are locale-sensitive in principle, and
the format here is fixed-width ASCII digits by specification -- so the fields
are written directly and the result cannot vary with LC_TIME.

Parameter(s):
  _unix_seconds:   seconds since the epoch.
  _use_local_time: non-zero for local time, zero for UTC.
  _out:            caller buffer; may be NULL to query the length.
  _out_capacity:   its size, including the terminator.
Return:
  D_PDF_DATE_MAX, always -- the form is fixed-width. On a timestamp the C
library cannot convert, the buffer is left empty and 0 is returned.
*/
size_t
d_pdf_format_date
(
    int64_t _unix_seconds,
    int32_t _use_local_time,
    char*   _out,
    size_t  _out_capacity
)
{
    time_t     _now = (time_t)_unix_seconds;
    struct tm* _tm  = 0;
    struct tm  _storage;
    size_t     _n   = 0;
    char       _buf[D_PDF_DATE_MAX + 1];
    int        _written = 0;

    (void)_storage;

    _tm = _use_local_time ? localtime(&_now) : gmtime(&_now);

    if (!_tm)
    {
        terminate_helper(_out, _out_capacity, 0u);

        return 0u;
    }

    _written = snprintf(_buf, sizeof(_buf), "D:%04d%02d%02d%02d%02d%02d",
                        _tm->tm_year + 1900,
                        _tm->tm_mon + 1,
                        _tm->tm_mday,
                        _tm->tm_hour,
                        _tm->tm_min,
                        _tm->tm_sec);

    if (_written < 0)
    {
        terminate_helper(_out, _out_capacity, 0u);

        return 0u;
    }

    for (_n = 0; (_n < (size_t)_written) && (_n < D_PDF_DATE_MAX); ++_n)
    {
        if (_out && ((_n + 1u) < _out_capacity))
        {
            _out[_n] = _buf[_n];
        }
    }

    terminate_helper(_out, _out_capacity, _n);

    return _n;
}

/*
d_pdf_creation_date
  The current local time as a PDF date string.

  LOCAL TIME AND THE WALL CLOCK, matching the C++ side. Both make the output
non-reproducible; d_pdf_format_date is the way out.

Parameter(s):
  _out:          caller buffer; may be NULL to query the length.
  _out_capacity: its size, including the terminator.
Return:
  the length written, excluding the terminator.
*/
size_t
d_pdf_creation_date
(
    char*  _out,
    size_t _out_capacity
)
{
    return d_pdf_format_date((int64_t)time(0), 1, _out, _out_capacity);
}


/* =========================================================================
   V.     object markers
   ========================================================================= */

/*
d_pdf_begin_object
  Records an object's byte offset and appends its "N 0 obj" marker.

  THE OFFSET IS THE POINT OF THIS FUNCTION. A PDF's cross-reference table is
byte offsets into the file, so recording the position BEFORE the marker is
written is what makes the table correct; recording it after points every entry
past its own object and produces a file that opens to a blank page in some
readers and an error in others.

  THE GENERATION NUMBER IS ALWAYS 0. The built-in writer never revises an
object, so every marker is "N 0 obj". A caller producing incremental updates
would need a generation parameter, and this function is the wrong shape for
that -- which is worth saying rather than leaving the 0 to look like an
oversight.

Parameter(s):
  _out:            the buffer being built; may be NULL to query the length.
  _out_capacity:   its size, including the terminator.
  _out_length:     how many bytes are already in it -- the offset to record.
  _offsets:        the cross-reference array; may be NULL to skip recording.
  _offset_count:   how many entries it holds.
  _object_number:  the object's number; also its index into _offsets.
Return:
  the new total length, excluding the terminator. Equal to _out_length when
_object_number is out of range for _offsets, in which case nothing is written
-- an out-of-range object number is a caller error that must not silently
corrupt the table by writing a marker with no offset behind it.
*/
size_t
d_pdf_begin_object
(
    char*   _out,
    size_t  _out_capacity,
    size_t  _out_length,
    size_t* _offsets,
    size_t  _offset_count,
    int32_t _object_number
)
{
    size_t _n = _out_length;
    char   _buf[32];
    int    _written = 0;
    size_t _i       = 0;

    if (_object_number < 0)
    {
        return _out_length;
    }

    if (_offsets)
    {
        if ((size_t)_object_number >= _offset_count)
        {
            return _out_length;
        }

        _offsets[(size_t)_object_number] = _out_length;
    }

    _written = snprintf(_buf, sizeof(_buf), "%d 0 obj\n", (int)_object_number);

    if (_written < 0)
    {
        return _out_length;
    }

    for (_i = 0; (_i < (size_t)_written) && (_i < (sizeof(_buf) - 1u)); ++_i)
    {
        append_helper(_out, _out_capacity, &_n, _buf[_i]);
    }

    terminate_helper(_out, _out_capacity, _n);

    return _n;
}
