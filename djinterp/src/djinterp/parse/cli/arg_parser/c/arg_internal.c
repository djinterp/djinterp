/*******************************************************************************
* djinterp [parse]                                                arg_internal.c
*
* Implementation of the private helpers declared in arg_internal.h.
*   ASCII spans, heap copies, integer and floating-point text in both
* directions with `.` as the decimal point in every locale, the arithmetic
* values of a type descriptor -- converted as CLI11's lexical_cast converts
* them, and written as the type writes them -- scratch storage, and array
* growth.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <float.h>   // FLT_MANT_DIG, DBL_MANT_DIG, LDBL_MANT_DIG, FLT_MAX,
                     // DBL_MAX
#include <limits.h>  // INT_MAX, INT_MIN
#include <locale.h>  // localeconv, struct lconv
#include <math.h>    // isnan, isinf
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, strtof, strtod, strtold
#include <string.h>  // memcmp, memcpy, memmove, memset, strchr, strlen,
                     // strstr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_PARSER_LONG_DOUBLE
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type, flags
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intN_t, uintN_t, intmax_t, uintmax_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_ARG_SNPRINTF_LONG_DOUBLE
//   macro: the function that formats a `long double` correctly on this
// runtime: MinGW-w64's own where the Microsoft runtime would misread `%Lg`
// (env_arg_parser.h, 3.1), snprintf elsewhere.
#if (D_ENV_ARG_PARSER_LONG_DOUBLE == D_ENV_ARG_PARSER_LONG_DOUBLE_MINGW)
    #define D_INTERNAL_ARG_SNPRINTF_LONG_DOUBLE __mingw_snprintf
#else
    #define D_INTERNAL_ARG_SNPRINTF_LONG_DOUBLE snprintf
#endif

/*
d_arg_internal_next_significant
  Advances `*_index` to the next byte of `_text` that is not a digit
separator, `_` or `'`, and reads it; false at the end. CLI11 removes every
separator before converting, so skipping them wherever they stand -- before
a sign, inside a prefix -- reads exactly what it reads.
*/
D_STATIC bool
d_arg_internal_next_significant(
    struct d_pack_text _text,
    size_t*            _index,
    char*              _out
)
{
    // skip separators until a significant byte or the end
    while (*_index < _text.length)
    {
        const char c = _text.data[*_index];

        // a separator is skipped
        if ( (c == '_') ||
             (c == '\'') )
        {
            (*_index)++;

            continue;
        }

        *_out = c;

        return true;
    }

    return false;
}

/*
d_arg_internal_integer_base
  Reads an integer's sign and base prefix, leaving `*_index` at its first
digit. A leading 0 followed by `x`, `o` or `b` names the base and is
consumed; under the C rule a leading 0 followed by anything else makes the
number octal and stays, being a valid octal digit itself.
*/
D_STATIC int
d_arg_internal_integer_base(
    struct d_pack_text _text,
    int                _rule,
    size_t*            _index,
    bool*              _negative
)
{
    char c = '\0';

    // a sign comes first, if any
    if ( (d_arg_internal_next_significant(_text,
                                          _index,
                                          &c)) &&
         ( (c == '+') ||
           (c == '-') ) )
    {
        *_negative = (c == '-');
        (*_index)++;
    }

    // only a leading 0 followed by something can carry a prefix
    if ( (!d_arg_internal_next_significant(_text,
                                           _index,
                                           &c)) ||
         (c != '0') )
    {
        return 10;
    }

    size_t after = *_index + 1u;
    char   next  = '\0';

    // a lone 0 is decimal under either rule
    if (!d_arg_internal_next_significant(_text,
                                         &after,
                                         &next))
    {
        return 10;
    }

    const char lower = d_arg_internal_to_lower(next);
    const int  base  = (lower == 'x') ? 16 :
                       (lower == 'o') ? 8  :
                       (lower == 'b') ? 2  : 0;

    // a named base consumes its prefix
    if (base != 0)
    {
        *_index = after + 1u;

        return base;
    }

    return (_rule == (int)D_ARG_INTEGER_C) ? 8 : 10;
}

/*
d_arg_internal_decimal_point
  The current locale's decimal point as localeconv reports it, or "." when it
reports nothing usable.
*/
D_STATIC const char*
d_arg_internal_decimal_point(
    void
)
{
    const struct lconv* const conventions = localeconv();

    // a missing or empty point is the C locale's
    if ( (!conventions)                       ||
         (!conventions->decimal_point)        ||
         (conventions->decimal_point[0] == '\0') )
    {
        return ".";
    }

    return conventions->decimal_point;
}

/*
d_arg_internal_real_input
  Writes the text strto* is to read into `_out`: the separators dropped and
each `.` replaced by the locale's decimal point, NUL-terminated. Returns the
length written, or (size_t)-1 when the text holds the locale's own decimal
point, which this module refuses so that one spelling means one number.
*/
D_STATIC size_t
d_arg_internal_real_input(
    struct d_pack_text _text,
    const char*        _point,
    char*              _out
)
{
    const size_t point_length = strlen(_point);
    const bool   is_dot       = ( (point_length == 1u) &&
                                  (_point[0] == '.') );
    size_t       length       = 0u;

    // copy each byte, dropping separators and translating the point
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const char c = _text.data[index];

        // the locale's point, where it is not `.`, is not this module's
        if ( (!is_dot) &&
             (c == _point[0]) &&
             (point_length <= (_text.length - index)) &&
             (memcmp(_text.data + index,
                     _point,
                     point_length) == 0) )
        {
            return (size_t)-1;
        }

        // separators are dropped
        if ( (c == '_') ||
             (c == '\'') )
        {
            continue;
        }

        // `.` becomes the locale's point
        if (c == '.')
        {
            memcpy(_out + length,
                   _point,
                   point_length);
            length += point_length;

            continue;
        }

        _out[length++] = c;
    }

    _out[length] = '\0';

    return length;
}

/*
d_arg_internal_real_parse
  Calls the strto* function of the type, so that the number is rounded once,
directly to the type, and reports whether it read the whole input.
*/
D_STATIC bool
d_arg_internal_real_parse(
    const char* _input,
    size_t      _length,
    int         _real,
    void*       _out
)
{
    char* end = NULL;

    // each type through its own conversion
    if (_real == (int)D_ARG_INTERNAL_REAL_FLOAT)
    {
        const float value = strtof(_input,
                                   &end);

        memcpy(_out,
               &value,
               sizeof(value));
    }
    else if (_real == (int)D_ARG_INTERNAL_REAL_DOUBLE)
    {
        const double value = strtod(_input,
                                    &end);

        memcpy(_out,
               &value,
               sizeof(value));
    }
    else
    {
        const long double value = strtold(_input,
                                          &end);

        memcpy(_out,
               &value,
               sizeof(value));
    }

    return ( (_length > 0u) &&
             (end == _input + _length) );
}

/*
d_arg_internal_decimal_digits
  The significant decimal digits that always distinguish the values of a
type with `_mantissa` binary digits, 1 + ceil(_mantissa * log10(2)) -- C11's
*_DECIMAL_DIG, which C99 lacks: 9, 17, 21 and 36 for the IEEE formats. The
logarithm is taken as 30103/100000, exact to the digits that matter here.
*/
D_STATIC int
d_arg_internal_decimal_digits(
    int _mantissa
)
{
    return 1 + (((_mantissa * 30103) + 99999) / 100000);
}

/*
d_arg_internal_real_digits
  The most significant digits a shortest round-trip search tries for a
type: past them every value round-trips.
*/
D_STATIC int
d_arg_internal_real_digits(
    int _real
)
{
    // the float and double formats, then whatever long double is
    if (_real == (int)D_ARG_INTERNAL_REAL_FLOAT)
    {
        return d_arg_internal_decimal_digits(FLT_MANT_DIG);
    }

    if (_real == (int)D_ARG_INTERNAL_REAL_DOUBLE)
    {
        return d_arg_internal_decimal_digits(DBL_MANT_DIG);
    }

    return d_arg_internal_decimal_digits(LDBL_MANT_DIG);
}

/*
d_arg_internal_real_as_double
  A float or a double read as a double; a float widens exactly.
*/
D_STATIC double
d_arg_internal_real_as_double(
    int         _real,
    const void* _value
)
{
    // a float is widened exactly
    if (_real == (int)D_ARG_INTERNAL_REAL_FLOAT)
    {
        float narrow = 0.0f;

        memcpy(&narrow,
               _value,
               sizeof(narrow));

        return (double)narrow;
    }

    double value = 0.0;

    memcpy(&value,
           _value,
           sizeof(value));

    return value;
}

/*
d_arg_internal_real_print
  snprintf with `%.<digits>g` for the type; a float is promoted to double,
which printf would do anyway. Returns false when the runtime fails or the
text does not fit, which no `%g` of these precisions can do.
*/
D_STATIC bool
d_arg_internal_real_print(
    struct d_arg_internal_number_text* _out,
    int                                _real,
    const void*                        _value,
    int                                _digits
)
{
    int written = -1;

    // each type through the formatter that reads it correctly
    if (_real == (int)D_ARG_INTERNAL_REAL_LONG_DOUBLE)
    {
        long double value = 0.0L;

        memcpy(&value,
               _value,
               sizeof(value));
        written = D_INTERNAL_ARG_SNPRINTF_LONG_DOUBLE(_out->data,
                                                      sizeof(_out->data),
                                                      "%.*Lg",
                                                      _digits,
                                                      value);
    }
    else
    {
        written = snprintf(_out->data,
                           sizeof(_out->data),
                           "%.*g",
                           _digits,
                           d_arg_internal_real_as_double(_real,
                                                         _value));
    }

    // a failed or truncated format leaves nothing
    if ( (written < 0) ||
         ((size_t)written >= sizeof(_out->data)) )
    {
        _out->data[0] = '\0';
        _out->length  = 0u;

        return false;
    }

    _out->length = (size_t)written;

    return true;
}

/*
d_arg_internal_real_round_trips
  Whether the text printf just wrote -- with the locale's decimal point, so
read back with the same locale -- names exactly the value printed.
*/
D_STATIC bool
d_arg_internal_real_round_trips(
    const struct d_arg_internal_number_text* _text,
    int                                      _real,
    const void*                              _value
)
{
    union d_arg_internal_scratch back = { 0.0L };

    memset(&back,
           0,
           sizeof(back));

    // the text must read back as a number at all
    if (!d_arg_internal_real_parse(_text->data,
                                   _text->length,
                                   _real,
                                   &back))
    {
        return false;
    }

    // compared as the type, so -0 and 0 both round-trip
    if (_real == (int)D_ARG_INTERNAL_REAL_FLOAT)
    {
        float original = 0.0f;
        float read     = 0.0f;

        memcpy(&original,
               _value,
               sizeof(original));
        memcpy(&read,
               &back,
               sizeof(read));

        return (read == original);
    }

    if (_real == (int)D_ARG_INTERNAL_REAL_DOUBLE)
    {
        double original = 0.0;

        memcpy(&original,
               _value,
               sizeof(original));

        return (back.d == original);
    }

    long double original = 0.0L;

    memcpy(&original,
           _value,
           sizeof(original));

    return (back.r == original);
}

/*
d_arg_internal_real_normalize
  Rewrites what printf wrote into the module's one spelling: the locale's
decimal point becomes `.`, and an exponent of three or more digits with
leading zeros -- the old Microsoft runtime's `1e+006` -- loses them down to
C99's two.
*/
D_STATIC void
d_arg_internal_real_normalize(
    struct d_arg_internal_number_text* _text
)
{
    const char* const point        = d_arg_internal_decimal_point();
    const size_t      point_length = strlen(point);
    char* const       found        = strstr(_text->data,
                                            point);

    // the locale's point, if any, becomes `.`
    if ( (found != NULL) &&
         ( (point_length != 1u) ||
           (point[0] != '.') ) )
    {
        found[0] = '.';
        memmove(found + 1,
                found + point_length,
                strlen(found + point_length) + 1u);
        _text->length -= (point_length - 1u);
    }

    char* const exponent = strchr(_text->data,
                                  'e');

    // only an exponent written with a sign has digits to trim
    if ( (!exponent) ||
         ( (exponent[1] != '+') &&
           (exponent[1] != '-') ) )
    {
        return;
    }

    char* const digits = exponent + 2;

    // drop leading zeros while more than two digits remain
    while ( (digits[0] == '0') &&
            (strlen(digits) > 2u) )
    {
        memmove(digits,
                digits + 1,
                strlen(digits));
        _text->length--;
    }

    return;
}

/*
d_arg_internal_real_exponent
  The decimal exponent printf wrote after `e`, or 0 when it wrote none, the
number being in fixed notation already.
*/
D_STATIC int
d_arg_internal_real_exponent(
    const struct d_arg_internal_number_text* _text
)
{
    const char* const mark     = strchr(_text->data,
                                        'e');
    int               exponent = 0;

    // fixed notation has no exponent
    if (!mark)
    {
        return 0;
    }

    const bool  negative = (mark[1] == '-');
    const char* digit    = mark + (( (mark[1] == '-') ||
                                     (mark[1] == '+') ) ? 2 : 1);

    // accumulate the exponent's digits, which are few
    while (d_arg_internal_is_digit(*digit))
    {
        exponent = (exponent * 10) + (*digit - '0');
        digit++;
    }

    return negative ? -exponent : exponent;
}

/*
d_arg_internal_text_word
  Writes a short constant word, such as "nan", as a number's text.
*/
D_STATIC void
d_arg_internal_text_word(
    struct d_arg_internal_number_text* _out,
    const char*                        _word
)
{
    const size_t length = strlen(_word);

    memcpy(_out->data,
           _word,
           length + 1u);
    _out->length = length;

    return;
}

/*
d_arg_internal_real_widen
  A floating-point object of the type, widened to long double for the tests
that apply to every type alike.
*/
D_STATIC long double
d_arg_internal_real_widen(
    int         _real,
    const void* _value
)
{
    // each type is read as itself, then widened exactly
    if (_real == (int)D_ARG_INTERNAL_REAL_FLOAT)
    {
        float value = 0.0f;

        memcpy(&value,
               _value,
               sizeof(value));

        return (long double)value;
    }

    if (_real == (int)D_ARG_INTERNAL_REAL_DOUBLE)
    {
        double value = 0.0;

        memcpy(&value,
               _value,
               sizeof(value));

        return (long double)value;
    }

    long double value = 0.0L;

    memcpy(&value,
           _value,
           sizeof(value));

    return value;
}

/*
d_arg_internal_write_signed
  Writes a signed value already known to fit into an object of `_size`
bytes, each width through its own type so that the bytes are the value's;
any other size is intmax_t's own.
*/
D_STATIC void
d_arg_internal_write_signed(
    intmax_t _value,
    size_t   _size,
    void*    _object
)
{
    // the width chooses the type
    if (_size == sizeof(int8_t))
    {
        const int8_t narrow = (int8_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(int16_t))
    {
        const int16_t narrow = (int16_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(int32_t))
    {
        const int32_t narrow = (int32_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(int64_t))
    {
        const int64_t narrow = (int64_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else
    {
        memcpy(_object,
               &_value,
               sizeof(_value));
    }

    return;
}

/*
d_arg_internal_store_signed
  Narrows a signed value into an object of `_size` bytes, refusing a value
the object cannot hold; the range is checked before the conversion, from
the exact-width limits.
*/
D_STATIC bool
d_arg_internal_store_signed(
    intmax_t _value,
    size_t   _size,
    void*    _object
)
{
    const intmax_t low  = (_size == sizeof(int8_t))  ? INT8_MIN  :
                          (_size == sizeof(int16_t)) ? INT16_MIN :
                          (_size == sizeof(int32_t)) ? INT32_MIN :
                          (_size == sizeof(int64_t)) ? INT64_MIN :
                                                       INTMAX_MIN;
    const intmax_t high = (_size == sizeof(int8_t))  ? INT8_MAX  :
                          (_size == sizeof(int16_t)) ? INT16_MAX :
                          (_size == sizeof(int32_t)) ? INT32_MAX :
                          (_size == sizeof(int64_t)) ? INT64_MAX :
                                                       INTMAX_MAX;

    // the value must fit the width
    if ( (_value < low) ||
         (_value > high) )
    {
        return false;
    }

    d_arg_internal_write_signed(_value,
                                _size,
                                _object);

    return true;
}

/*
d_arg_internal_write_unsigned
  As d_arg_internal_write_signed, for an unsigned value.
*/
D_STATIC void
d_arg_internal_write_unsigned(
    uintmax_t _value,
    size_t    _size,
    void*     _object
)
{
    // the width chooses the type
    if (_size == sizeof(uint8_t))
    {
        const uint8_t narrow = (uint8_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(uint16_t))
    {
        const uint16_t narrow = (uint16_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(uint32_t))
    {
        const uint32_t narrow = (uint32_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(uint64_t))
    {
        const uint64_t narrow = (uint64_t)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else
    {
        memcpy(_object,
               &_value,
               sizeof(_value));
    }

    return;
}

/*
d_arg_internal_store_unsigned
  Narrows an unsigned value into an object of `_size` bytes, refusing a
value the object cannot hold.
*/
D_STATIC bool
d_arg_internal_store_unsigned(
    uintmax_t _value,
    size_t    _size,
    void*     _object
)
{
    const uintmax_t limit = (_size == sizeof(uint8_t))  ? UINT8_MAX  :
                            (_size == sizeof(uint16_t)) ? UINT16_MAX :
                            (_size == sizeof(uint32_t)) ? UINT32_MAX :
                            (_size == sizeof(uint64_t)) ? UINT64_MAX :
                                                          UINTMAX_MAX;

    // the value must fit the width
    if (_value > limit)
    {
        return false;
    }

    d_arg_internal_write_unsigned(_value,
                                  _size,
                                  _object);

    return true;
}

/*
d_arg_internal_store_real
  Narrows a long double into a float, double or long double, refusing a
finite value beyond the type's range -- converting one is undefined in C, so
the range is checked first -- while infinities and NaNs pass, being
representable in every format.
*/
D_STATIC bool
d_arg_internal_store_real(
    long double _value,
    size_t      _size,
    void*       _object
)
{
    const bool        finite = ( (!isnan(_value)) &&
                                 (!isinf(_value)) );
    const long double limit  = (_size == sizeof(float))
                               ? (long double)FLT_MAX
                               : (long double)DBL_MAX;

    // a finite value must lie within a narrower type's range
    if ( (finite)                         &&
         (_size != sizeof(long double))   &&
         ( (_value > limit) ||
           (_value < -limit) ) )
    {
        return false;
    }

    // the narrowest type first, since sizes may coincide
    if (_size == sizeof(float))
    {
        const float narrow = (float)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else if (_size == sizeof(double))
    {
        const double narrow = (double)_value;

        memcpy(_object,
               &narrow,
               sizeof(narrow));
    }
    else
    {
        memcpy(_object,
               &_value,
               sizeof(_value));
    }

    return true;
}

/*
d_arg_internal_load_signed
  Reads a signed object of `_size` bytes through its exact-width type, by
copying it into one, which reads it without aliasing it.
*/
D_STATIC intmax_t
d_arg_internal_load_signed(
    const void* _object,
    size_t      _size
)
{
    // each width through its own type
    if (_size == sizeof(int8_t))
    {
        int8_t value = 0;

        memcpy(&value,
               _object,
               sizeof(value));

        return (intmax_t)value;
    }

    if (_size == sizeof(int16_t))
    {
        int16_t value = 0;

        memcpy(&value,
               _object,
               sizeof(value));

        return (intmax_t)value;
    }

    if (_size == sizeof(int32_t))
    {
        int32_t value = 0;

        memcpy(&value,
               _object,
               sizeof(value));

        return (intmax_t)value;
    }

    if (_size == sizeof(int64_t))
    {
        int64_t value = 0;

        memcpy(&value,
               _object,
               sizeof(value));

        return (intmax_t)value;
    }

    intmax_t value = 0;

    memcpy(&value,
           _object,
           sizeof(value));

    return value;
}

/*
d_arg_internal_load_unsigned
  As d_arg_internal_load_signed, for an unsigned object.
*/
D_STATIC uintmax_t
d_arg_internal_load_unsigned(
    const void* _object,
    size_t      _size
)
{
    // each width through its own type
    if (_size == sizeof(uint8_t))
    {
        uint8_t value = 0u;

        memcpy(&value,
               _object,
               sizeof(value));

        return (uintmax_t)value;
    }

    if (_size == sizeof(uint16_t))
    {
        uint16_t value = 0u;

        memcpy(&value,
               _object,
               sizeof(value));

        return (uintmax_t)value;
    }

    if (_size == sizeof(uint32_t))
    {
        uint32_t value = 0u;

        memcpy(&value,
               _object,
               sizeof(value));

        return (uintmax_t)value;
    }

    if (_size == sizeof(uint64_t))
    {
        uint64_t value = 0u;

        memcpy(&value,
               _object,
               sizeof(value));

        return (uintmax_t)value;
    }

    uintmax_t value = 0u;

    memcpy(&value,
           _object,
           sizeof(value));

    return value;
}

//==============================================================================
// 2.  TEXT
//==============================================================================

/*
d_arg_internal_trim
  Narrows the span from both ends; a NULL span is returned as it is.
*/
struct d_pack_text
d_arg_internal_trim(
    struct d_pack_text _text
)
{
    // a NULL span has nothing to trim
    if (!_text.data)
    {
        _text.length = 0u;

        return _text;
    }

    // leading space
    while ( (_text.length > 0u) &&
            (d_arg_internal_is_space(_text.data[0])) )
    {
        _text.data++;
        _text.length--;
    }

    // trailing space
    while ( (_text.length > 0u) &&
            (d_arg_internal_is_space(_text.data[_text.length - 1u])) )
    {
        _text.length--;
    }

    return _text;
}

/*
d_arg_internal_equal_cstr
  Length first, then the bytes, so a span holding a NUL never matches early.
*/
bool
d_arg_internal_equal_cstr(
    struct d_pack_text _text,
    const char*        _word
)
{
    // nothing equals a missing word
    if (!_word)
    {
        return false;
    }

    const size_t length = strlen(_word);

    // the lengths must agree before the bytes are compared
    if (length != _text.length)
    {
        return false;
    }

    return ( (length == 0u) ||
             (memcmp(_text.data,
                     _word,
                     length) == 0) );
}

/*
d_arg_internal_equal_nocase_cstr
  As d_arg_internal_equal_cstr, folding ASCII case in both.
*/
bool
d_arg_internal_equal_nocase_cstr(
    struct d_pack_text _text,
    const char*        _word
)
{
    // nothing equals a missing word, and the lengths must agree
    if ( (!_word) ||
         (strlen(_word) != _text.length) )
    {
        return false;
    }

    // every byte must agree once folded
    for (size_t index = 0u; index < _text.length; ++index)
    {
        if (d_arg_internal_to_lower(_text.data[index]) !=
            d_arg_internal_to_lower(_word[index]))
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_is_separator
  CLI11's is_separator: the values that part a container's items, the empty
one and `%%`.
*/
bool
d_arg_internal_is_separator(
    struct d_pack_text _text
)
{
    return ( (_text.length == 0u) ||
             (d_arg_internal_equal_cstr(_text,
                                        "%%")) );
}

/*
d_arg_internal_equal
  Lengths first, then the bytes; two empty spans are equal whatever their
pointers.
*/
bool
d_arg_internal_equal(
    struct d_pack_text _a,
    struct d_pack_text _b
)
{
    // the lengths must agree
    if (_a.length != _b.length)
    {
        return false;
    }

    return ( (_a.length == 0u) ||
             (memcmp(_a.data,
                     _b.data,
                     _a.length) == 0) );
}

/*
d_arg_internal_names_match
  Walks both names at once, stepping over underscores on either side when
they are ignored, so neither name is copied; both must run out together.
*/
bool
d_arg_internal_names_match(
    struct d_pack_text _a,
    struct d_pack_text _b,
    bool               _case,
    bool               _underscore
)
{
    size_t index_a = 0u;
    size_t index_b = 0u;

    // compare character by character, skipping what is ignored
    for (;;)
    {
        // step over ignored underscores on each side
        while ( (_underscore)                 &&
                (index_a < _a.length)         &&
                (_a.data[index_a] == '_') )
        {
            ++index_a;
        }

        while ( (_underscore)                 &&
                (index_b < _b.length)         &&
                (_b.data[index_b] == '_') )
        {
            ++index_b;
        }

        // both must end together
        if ( (index_a == _a.length) ||
             (index_b == _b.length) )
        {
            return ( (index_a == _a.length) &&
                     (index_b == _b.length) );
        }

        const char char_a = _case
                            ? d_arg_internal_to_lower(_a.data[index_a])
                            : _a.data[index_a];
        const char char_b = _case
                            ? d_arg_internal_to_lower(_b.data[index_b])
                            : _b.data[index_b];

        // a difference ends the match
        if (char_a != char_b)
        {
            return false;
        }

        ++index_a;
        ++index_b;
    }
}

/*
d_arg_internal_valid_name
  The first character by one rule and the rest by the other.
*/
bool
d_arg_internal_valid_name(
    struct d_pack_text _name
)
{
    // a name needs a valid first character
    if ( (_name.length == 0u) ||
         (!d_arg_internal_valid_first_char(_name.data[0])) )
    {
        return false;
    }

    // and nothing invalid after it
    for (size_t index = 1u; index < _name.length; ++index)
    {
        if (!d_arg_internal_valid_later_char(_name.data[index]))
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_print_length
  A size_t narrowed for printf's precision, saturating rather than wrapping.
*/
int
d_arg_internal_print_length(
    struct d_pack_text _text
)
{
    return (_text.length > (size_t)INT_MAX)
           ? INT_MAX
           : (int)_text.length;
}

/*
d_arg_internal_checked_multiply
  CLI11's integral checked_multiply and its overflowCheck, for `int`: the
trivial factors first, INT_MIN refused, then the quotient test in the
product's sign.
*/
bool
d_arg_internal_checked_multiply(
    int* _a,
    int  _b
)
{
    const int a = *_a;

    // a zero or a one never overflows
    if ( (a == 0) ||
         (_b == 0) ||
         (a == 1) ||
         (_b == 1) )
    {
        *_a = a * _b;

        return true;
    }

    // INT_MIN has no magnitude to divide by
    if ( (a == INT_MIN) ||
         (_b == INT_MIN) )
    {
        return false;
    }

    const int magnitude_a = (a < 0) ? -a : a;
    const int magnitude_b = (_b < 0) ? -_b : _b;

    // the quotient test, by the product's sign
    if ( (a > 0) == (_b > 0) )
    {
        if ((INT_MAX / magnitude_a) < magnitude_b)
        {
            return false;
        }
    }
    else if ((INT_MIN / magnitude_a) > -magnitude_b)
    {
        return false;
    }

    *_a = a * _b;

    return true;
}

/*
d_arg_internal_copy_text
  memcpy rather than a string copy, so NUL bytes inside the span survive.
*/
char*
d_arg_internal_copy_text(
    struct d_pack_text _text
)
{
    const size_t length = (_text.data != NULL) ? _text.length : 0u;

    // the terminator must fit a size_t
    if (length == (size_t)-1)
    {
        return NULL;
    }

    char* const copy = (char*)malloc(length + 1u);

    // check if memory allocation was successful
    if (!copy)
    {
        return NULL;
    }

    // memcpy wants a valid source even for zero bytes
    if (length > 0u)
    {
        memcpy(copy,
               _text.data,
               length);
    }

    copy[length] = '\0';

    return copy;
}

/*
d_arg_internal_copy_cstr
  d_arg_internal_text_of and d_arg_internal_copy_text; NULL is the empty
string.
*/
char*
d_arg_internal_copy_cstr(
    const char* _text
)
{
    return d_arg_internal_copy_text(d_arg_internal_text_of(_text));
}

//==============================================================================
// 3.  NUMBERS
//==============================================================================

/*
d_arg_internal_read_integer
  Trims, reads the sign and base, then accumulates digits with a check before
each multiply-add, so the magnitude never wraps: once it would, `overflow`
is set and the remaining digits are only validated. At least one digit must
follow the prefix.
*/
int
d_arg_internal_read_integer(
    struct d_pack_text             _text,
    int                            _rule,
    struct d_arg_internal_integer* _out
)
{
    const struct d_pack_text text     = d_arg_internal_trim(_text);
    size_t                   index    = 0u;
    bool                     negative = false;
    const int                base     = d_arg_internal_integer_base(text,
                                                                   _rule,
                                                                   &index,
                                                                   &negative);
    const uintmax_t          ubase    = (uintmax_t)base;
    uintmax_t                value    = 0u;
    bool                     overflow = false;
    size_t                   digits   = 0u;
    char                     c        = '\0';

    // every significant byte left must be a digit of the base
    while (d_arg_internal_next_significant(text,
                                           &index,
                                           &c))
    {
        const int digit = d_arg_internal_digit_value(c);

        // anything else ends the number unread
        if ( (digit < 0) ||
             (digit >= base) )
        {
            return D_ARG_ERROR_CONVERSION;
        }

        // past uintmax_t the digits are still checked
        if (value > ((UINTMAX_MAX - (uintmax_t)digit) / ubase))
        {
            overflow = true;
        }

        value = overflow ? UINTMAX_MAX : (value * ubase) + (uintmax_t)digit;
        digits++;
        index++;
    }

    // a sign or a prefix alone is not a number
    if (digits == 0u)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    _out->magnitude = value;
    _out->negative  = negative;
    _out->overflow  = overflow;

    return D_ARG_OK;
}

/*
d_arg_internal_real_read_into
  Writes strto*'s input for `_text` into `_input`, which has room for it,
and has strto* read all of it into `_value`.
*/
D_STATIC bool
d_arg_internal_real_read_into(
    struct d_pack_text            _text,
    const char*                   _point,
    char*                         _input,
    int                           _real,
    union d_arg_internal_scratch* _value
)
{
    const size_t length = d_arg_internal_real_input(_text,
                                                    _point,
                                                    _input);

    return ( (length != (size_t)-1) &&
             (d_arg_internal_real_parse(_input,
                                        length,
                                        _real,
                                        _value)) );
}

/*
d_arg_internal_read_real
  Builds strto*'s input on the stack when it fits, on the heap otherwise --
the only allocation, for a number written with hundreds of digits -- and
requires strto* to read all of it. The number lands in scratch storage
first, so a failed conversion leaves `_out` untouched.
*/
int
d_arg_internal_read_real(
    struct d_pack_text _text,
    int                _real,
    void*              _out
)
{
    const struct d_pack_text text   = d_arg_internal_trim(_text);
    const char* const        point  = d_arg_internal_decimal_point();
    const size_t             widest = strlen(point);

    // nothing to read, or too much to measure
    if ( (text.length == 0u) ||
         (text.length > (((size_t)-1 - 1u) / widest)) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    const size_t needed = (text.length * widest) + 1u;
    char         local[D_INTERNAL_ARG_NUMBER_TEXT_MAX] = { '\0' };
    char* const  input  = (needed <= sizeof(local))
                          ? local
                          : (char*)malloc(needed);

    // check if memory allocation was successful
    if (!input)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    union d_arg_internal_scratch value = { 0.0L };
    const bool                   read  = d_arg_internal_real_read_into(text,
                                                                       point,
                                                                       input,
                                                                       _real,
                                                                       &value);

    // a heap copy is freed whatever the outcome
    if (input != local)
    {
        free(input);
    }

    // the output is written only with a number read whole
    if (!read)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    memcpy(_out,
           &value,
           (_real == (int)D_ARG_INTERNAL_REAL_FLOAT)  ? sizeof(float)  :
           (_real == (int)D_ARG_INTERNAL_REAL_DOUBLE) ? sizeof(double) :
                                                        sizeof(long double));

    return D_ARG_OK;
}

/*
d_arg_internal_text_unsigned
  Digits from the right into a local array, then copied to the front.
*/
void
d_arg_internal_text_unsigned(
    struct d_arg_internal_number_text* _out,
    uintmax_t                          _value
)
{
    char      digits[D_INTERNAL_ARG_NUMBER_TEXT_MAX] = { '\0' };
    size_t    first = sizeof(digits);
    uintmax_t value = _value;

    // at least one digit, for 0
    do
    {
        digits[--first] = (char)('0' + (int)(value % 10u));
        value          /= 10u;
    } while (value != 0u);

    _out->length = sizeof(digits) - first;
    memcpy(_out->data,
           digits + first,
           _out->length);
    _out->data[_out->length] = '\0';

    return;
}

/*
d_arg_internal_text_signed
  The magnitude is taken in uintmax_t arithmetic, where negating INTMAX_MIN
is defined, and written after the sign.
*/
void
d_arg_internal_text_signed(
    struct d_arg_internal_number_text* _out,
    intmax_t                           _value
)
{
    const bool      negative  = (_value < 0);
    const uintmax_t magnitude = negative
                                ? ((uintmax_t)0 - (uintmax_t)_value)
                                : (uintmax_t)_value;

    d_arg_internal_text_unsigned(_out,
                                 magnitude);

    // the sign goes in front of the digits
    if (negative)
    {
        memmove(_out->data + 1,
                _out->data,
                _out->length + 1u);
        _out->data[0] = '-';
        _out->length++;
    }

    return;
}

/*
d_arg_internal_text_special
  Spells NaN and the infinities by hand, since runtimes disagree
("-nan(ind)" on Microsoft's); false for a finite value, left to the caller.
*/
D_STATIC bool
d_arg_internal_text_special(
    struct d_arg_internal_number_text* _out,
    long double                        _wide
)
{
    // NaN in one spelling everywhere
    if (isnan(_wide))
    {
        d_arg_internal_text_word(_out,
                                 "nan");

        return true;
    }

    // the infinities likewise
    if (isinf(_wide))
    {
        d_arg_internal_text_word(_out,
                                 (_wide < 0.0L) ? "-inf" : "inf");

        return true;
    }

    return false;
}

/*
d_arg_internal_real_shortest
  Prints with 1, 2, ... significant digits until the text reads back as the
value, which `_last` digits -- the type's decimal digits -- always do; a
positive `_precision` prints once, with that many. Returns the digits the
search stopped at, or 0 when printing failed.
*/
D_STATIC int
d_arg_internal_real_shortest(
    struct d_arg_internal_number_text* _out,
    int                                _real,
    const void*                        _value,
    int                                _precision,
    int                                _last
)
{
    int digits = (_precision > 0) ? _precision : 1;

    // the fewest digits that read back as the value
    for (; digits <= _last; ++digits)
    {
        // a failed print ends the search
        if (!d_arg_internal_real_print(_out,
                                       _real,
                                       _value,
                                       digits))
        {
            return 0;
        }

        // a fixed precision prints once
        if ( (_precision > 0) ||
             (d_arg_internal_real_round_trips(_out,
                                              _real,
                                              _value)) )
        {
            break;
        }
    }

    return digits;
}

/*
d_arg_internal_text_real
  Non-finite values are spelled by d_arg_internal_text_special, finite ones
by the shortest search; then a number `%g` put in scientific notation only
because the digits were few is written again with as many digits as its
integer part has, which `%g` writes in fixed notation and which still reads
back.
*/
bool
d_arg_internal_text_real(
    struct d_arg_internal_number_text* _out,
    int                                _real,
    const void*                        _value,
    int                                _precision
)
{
    // NaN and the infinities in one spelling everywhere
    if (d_arg_internal_text_special(_out,
                                    d_arg_internal_real_widen(_real,
                                                              _value)))
    {
        return true;
    }

    const int last   = (_precision > 0)
                       ? _precision
                       : d_arg_internal_real_digits(_real);
    const int digits = d_arg_internal_real_shortest(_out,
                                                    _real,
                                                    _value,
                                                    _precision,
                                                    last);

    // the search could not print
    if (digits == 0)
    {
        return false;
    }

    const int exponent = d_arg_internal_real_exponent(_out);

    // a shortest number within the type's digits reads better fixed: 100,
    // not 1e+02, as CLI11's to_string writes it
    if ( (_precision == 0)        &&
         (exponent >= digits)     &&
         (exponent < last)        &&
         (!d_arg_internal_real_print(_out,
                                     _real,
                                     _value,
                                     exponent + 1)) )
    {
        return false;
    }

    d_arg_internal_real_normalize(_out);

    return true;
}

/*
d_arg_internal_number_of
  Floating point first, since a float type may carry SIGNED; then integers
and characters, by the SIGNED flag. A size no C type of the kind has makes
the type unusable as a number rather than misread.
*/
int
d_arg_internal_number_of(
    const struct d_arg_type* _type
)
{
    // no descriptor, no number
    if (!_type)
    {
        return D_ARG_INTERNAL_NUMBER_NONE;
    }

    const size_t   size  = _type->size;
    const uint32_t flags = _type->flags;

    // floating point by its size
    if ((flags & D_ARG_TYPE_FLAG_FLOAT) != 0u)
    {
        return ( (size == sizeof(float))  ||
                 (size == sizeof(double)) ||
                 (size == sizeof(long double)) )
               ? D_ARG_INTERNAL_NUMBER_REAL
               : D_ARG_INTERNAL_NUMBER_NONE;
    }

    // integers and characters of a width the exact types cover
    if ( ((flags & (D_ARG_TYPE_FLAG_INTEGER | D_ARG_TYPE_FLAG_CHAR)) == 0u) ||
         ( (size != 1u) &&
           (size != 2u) &&
           (size != 4u) &&
           (size != 8u) &&
           (size != sizeof(intmax_t)) ) )
    {
        return D_ARG_INTERNAL_NUMBER_NONE;
    }

    return ((flags & D_ARG_TYPE_FLAG_SIGNED) != 0u)
           ? D_ARG_INTERNAL_NUMBER_SIGNED
           : D_ARG_INTERNAL_NUMBER_UNSIGNED;
}

/*
d_arg_internal_real_kind
  The floating-point type of a descriptor's size, the narrowest first, since
sizes may coincide.
*/
D_STATIC int
d_arg_internal_real_kind(
    size_t _size
)
{
    return (_size == sizeof(float))
           ? (int)D_ARG_INTERNAL_REAL_FLOAT
           : (_size == sizeof(double))
             ? (int)D_ARG_INTERNAL_REAL_DOUBLE
             : (int)D_ARG_INTERNAL_REAL_LONG_DOUBLE;
}

/*
d_arg_internal_number_load
  Reads the object through the exact-width type of its size, so that the
widening is a C conversion rather than a reinterpretation.
*/
bool
d_arg_internal_number_load(
    const struct d_arg_type*      _type,
    const void*                   _object,
    struct d_arg_internal_number* _out
)
{
    const int kind = d_arg_internal_number_of(_type);

    memset(_out,
           0,
           sizeof(*_out));
    _out->kind = kind;

    // floating point through the shared widening
    if (kind == D_ARG_INTERNAL_NUMBER_REAL)
    {
        _out->r = d_arg_internal_real_widen(
                      d_arg_internal_real_kind(_type->size),
                      _object);

        return true;
    }

    // integers through their exact-width types
    if (kind == D_ARG_INTERNAL_NUMBER_SIGNED)
    {
        _out->s = d_arg_internal_load_signed(_object,
                                             _type->size);

        return true;
    }

    if (kind == D_ARG_INTERNAL_NUMBER_UNSIGNED)
    {
        _out->u = d_arg_internal_load_unsigned(_object,
                                               _type->size);

        return true;
    }

    return false;
}

/*
d_arg_internal_number_store
  Dispatches by kind to the narrowing that checks the type's range.
*/
bool
d_arg_internal_number_store(
    const struct d_arg_type*            _type,
    const struct d_arg_internal_number* _number,
    void*                               _object
)
{
    const int kind = d_arg_internal_number_of(_type);

    // the number must be of the type's own kind
    if ( (kind == D_ARG_INTERNAL_NUMBER_NONE) ||
         (kind != _number->kind) )
    {
        return false;
    }

    // each kind through its narrowing
    if (kind == D_ARG_INTERNAL_NUMBER_SIGNED)
    {
        return d_arg_internal_store_signed(_number->s,
                                           _type->size,
                                           _object);
    }

    if (kind == D_ARG_INTERNAL_NUMBER_UNSIGNED)
    {
        return d_arg_internal_store_unsigned(_number->u,
                                             _type->size,
                                             _object);
    }

    return d_arg_internal_store_real(_number->r,
                                     _type->size,
                                     _object);
}

/*
d_arg_internal_number_compare
  Compares the member of the kind; NaN is tested first, since every ordered
comparison with it is false.
*/
int
d_arg_internal_number_compare(
    const struct d_arg_internal_number* _a,
    const struct d_arg_internal_number* _b
)
{
    // integers compare exactly
    if (_a->kind == D_ARG_INTERNAL_NUMBER_SIGNED)
    {
        return (_a->s < _b->s) ? -1 : (_a->s > _b->s) ? 1 : 0;
    }

    if (_a->kind == D_ARG_INTERNAL_NUMBER_UNSIGNED)
    {
        return (_a->u < _b->u) ? -1 : (_a->u > _b->u) ? 1 : 0;
    }

    // floating point is unordered with a NaN
    if ( (isnan(_a->r)) ||
         (isnan(_b->r)) )
    {
        return D_INTERNAL_ARG_UNORDERED;
    }

    return (_a->r < _b->r) ? -1 : (_a->r > _b->r) ? 1 : 0;
}

/*
d_arg_internal_number_stream
  `operator<<` with a default stream: a char as itself, integers in decimal,
floating point as `%g`. A float or double is printed as double, which is
what the stream does with either.
*/
void
d_arg_internal_number_stream(
    struct d_arg_internal_number_text*  _out,
    const struct d_arg_type*            _type,
    const struct d_arg_internal_number* _number
)
{
    const bool is_char = ((_type->flags & D_ARG_TYPE_FLAG_CHAR) != 0u);

    // a character type streams as the character
    if ( (is_char) &&
         (_number->kind != D_ARG_INTERNAL_NUMBER_REAL) )
    {
        _out->data[0] = (_number->kind == D_ARG_INTERNAL_NUMBER_SIGNED)
                        ? (char)_number->s
                        : (char)_number->u;
        _out->data[1] = '\0';
        _out->length  = 1u;

        return;
    }

    // integers in full
    if (_number->kind == D_ARG_INTERNAL_NUMBER_SIGNED)
    {
        d_arg_internal_text_signed(_out,
                                   _number->s);

        return;
    }

    if (_number->kind == D_ARG_INTERNAL_NUMBER_UNSIGNED)
    {
        d_arg_internal_text_unsigned(_out,
                                     _number->u);

        return;
    }

    // floating point to six digits, in the precision of its type
    if (_type->size == sizeof(long double))
    {
        (void)d_arg_internal_text_real(_out,
                                       (int)D_ARG_INTERNAL_REAL_LONG_DOUBLE,
                                       &_number->r,
                                       6);

        return;
    }

    const double value = (double)_number->r;

    (void)d_arg_internal_text_real(_out,
                                   (int)D_ARG_INTERNAL_REAL_DOUBLE,
                                   &value,
                                   6);

    return;
}

/*
d_arg_internal_lexical_cast
  d_arg_convert with one value, but for the empty text of an arithmetic type:
CLI11's lexical_cast refuses it, where its lexical_assign -- what an option
converts with -- value-initializes the variable.
*/
int
d_arg_internal_lexical_cast(
    const struct d_arg_type*            _type,
    struct d_pack_text                  _text,
    const struct d_arg_convert_context* _context,
    void*                               _object
)
{
    // the empty text is no number
    if ( (_text.length == 0u) &&
         (d_arg_internal_number_of(_type) !=
          (int)D_ARG_INTERNAL_NUMBER_NONE) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    return d_arg_convert(_type,
                         &_text,
                         1u,
                         _object,
                         _context);
}

/*
d_arg_internal_number_write_plain
  An object of a type without a format function, written as the built-in
formats write theirs: floating point to the digits that read back, anything
else as it streams. Only an arithmetic type has such a form.
*/
D_STATIC int
d_arg_internal_number_write_plain(
    const struct d_arg_type* _type,
    const void*              _object,
    struct d_arg_buffer*     _out
)
{
    struct d_arg_internal_number      number = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number_text text   = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    // only an arithmetic type has a number to write
    if (!d_arg_internal_number_load(_type,
                                    _object,
                                    &number))
    {
        return D_ARG_ERROR_UNSUPPORTED;
    }

    // floating point to the digits that read back; the rest as streamed
    if (number.kind == (int)D_ARG_INTERNAL_NUMBER_REAL)
    {
        // a runtime that cannot print the number leaves nothing to write
        if (!d_arg_internal_text_real(&text,
                                      d_arg_internal_real_kind(_type->size),
                                      _object,
                                      0))
        {
            return D_ARG_ERROR_UNSUPPORTED;
        }
    }
    else
    {
        d_arg_internal_number_stream(&text,
                                     _type,
                                     &number);
    }

    return d_arg_buffer_append(_out,
                               text.data,
                               text.length);
}

/*
d_arg_internal_number_write
  The type's own format, through the buffer's sink, whose refusal is the
buffer's failure; without one, the form the built-in formats write.
*/
int
d_arg_internal_number_write(
    const struct d_arg_type* _type,
    const void*              _object,
    struct d_arg_buffer*     _out
)
{
    // validate the parameters
    if ( (!_type)   ||
         (!_object) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a type without a format function
    if (!_type->format)
    {
        return d_arg_internal_number_write_plain(_type,
                                                 _object,
                                                 _out);
    }

    const int status = _type->format(_type,
                                     _object,
                                     d_arg_buffer_sink(_out));

    return (_out->failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

//==============================================================================
// 4.  STORAGE
//==============================================================================

/*
d_arg_internal_scratch_acquire
  The union is zeroed whole, so a conversion that writes fewer bytes than the
type's size still leaves the rest defined.
*/
void*
d_arg_internal_scratch_acquire(
    union d_arg_internal_scratch* _scratch,
    size_t                        _size
)
{
    // the stack storage serves every type it can hold
    if (_size <= sizeof(*_scratch))
    {
        memset(_scratch,
               0,
               sizeof(*_scratch));

        return _scratch;
    }

    return calloc(1u,
                  _size);
}

/*
d_arg_internal_scratch_release
  Frees only what acquire took from the heap.
*/
void
d_arg_internal_scratch_release(
    union d_arg_internal_scratch* _scratch,
    void*                         _storage
)
{
    // the stack storage is not freed
    if (_storage != (void*)_scratch)
    {
        free(_storage);
    }

    return;
}

/*
d_arg_internal_grow
  Doubles from a small start until the request fits, falling back to the
exact request where doubling would overflow; the byte count is checked
before realloc sees it.
*/
void*
d_arg_internal_grow(
    void*   _items,
    size_t  _capacity,
    size_t  _needed,
    size_t  _size,
    size_t* _new_capacity
)
{
    size_t capacity = (_capacity > 0u) ? _capacity : 4u;

    // double until the request fits, stopping short of overflow
    while (capacity < _needed)
    {
        if (capacity > ((size_t)-1 / 2u))
        {
            capacity = _needed;

            break;
        }

        capacity *= 2u;
    }

    // the byte count must fit a size_t
    if ( (_size == 0u) ||
         (capacity > ((size_t)-1 / _size)) )
    {
        return NULL;
    }

    void* const items = realloc(_items,
                                capacity * _size);

    // check if memory allocation was successful
    if (!items)
    {
        return NULL;
    }

    *_new_capacity = capacity;

    return items;
}
