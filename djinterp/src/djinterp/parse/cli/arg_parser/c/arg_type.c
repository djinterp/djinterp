/*******************************************************************************
* djinterp [parse]                                                    arg_type.c
*
* Implementation of the value types declared in arg_type.h.
*   The built-in descriptors, CLI11's conversions -- integers under either
* rule, floating point with `.` in every locale, truth values and flag
* values -- the conversion and formatting entry points, and the arithmetic
* of the sum policy.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_type.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // corresponding header
// std
#include <limits.h>  // CHAR_MIN, LONG_MIN, LONG_MAX, ULONG_MAX, LLONG_MIN,
                     // LLONG_MAX
#include <stddef.h>  // size_t, ptrdiff_t, NULL
#include <string.h>  // memchr, memcpy, memset
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text,
                                                                // d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer, sinks
#include "./arg_internal.h"  // numbers, spans
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intN_t, uintN_t, intmax_t, uintmax_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// TRUE_WORDS / FALSE_WORDS
//   constant: the words CLI11's to_flag_value reads as true and as false,
// matched without case.
static const char* const TRUE_WORDS[]  = { "true", "on", "yes", "enable" };
static const char* const FALSE_WORDS[] = { "false", "off", "no", "disable" };

// D_INTERNAL_ARG_WORD_COUNT
//   constant: how many words each of TRUE_WORDS and FALSE_WORDS holds.
#define D_INTERNAL_ARG_WORD_COUNT 4u

D_STATIC_ASSERT( ( (sizeof(TRUE_WORDS) / sizeof(TRUE_WORDS[0])) ==
                   D_INTERNAL_ARG_WORD_COUNT ) &&
                 ( (sizeof(FALSE_WORDS) / sizeof(FALSE_WORDS[0])) ==
                   D_INTERNAL_ARG_WORD_COUNT ),
                 "the truth words come in pairs" );

/*
d_arg_internal_first_value
  The value a scalar converts: the first, as CLI11's lexical_conversion takes
the first string, or the empty text when there is none.
*/
D_STATIC struct d_pack_text
d_arg_internal_first_value(
    const struct d_pack_text* _values,
    size_t                    _count
)
{
    struct d_pack_text first = d_arg_internal_span("",
                                                   0u);

    // a missing or NULL value reads as empty
    if ( (_values != NULL)         &&
         (_count > 0u)             &&
         (_values[0].data != NULL) )
    {
        first = _values[0];
    }

    return first;
}

/*
d_arg_internal_rule_of
  A conversion context's integer rule, or the configured default without
one.
*/
D_STATIC int
d_arg_internal_rule_of(
    const struct d_arg_convert_context* _context
)
{
    return (_context != NULL)
           ? _context->integer_rule
           : (int)D_INTERNAL_ARG_PARSER_INTEGER_RULE;
}

/*
d_arg_internal_signed_value
  CLI11's signed integral_conversion: the exact word `true` is 1, then an
integer that must fit intmax_t. The magnitude of INTMAX_MIN is one more than
INTMAX_MAX, so the negative bound is checked in uintmax_t.
*/
D_STATIC int
d_arg_internal_signed_value(
    struct d_pack_text _text,
    int                _rule,
    intmax_t*          _out
)
{
    const struct d_pack_text      text = d_arg_internal_trim(_text);
    struct d_arg_internal_integer integer = D_INTERNAL_ARG_INTEGER_INIT;

    // CLI11 reads `true` as 1 for every signed type
    if (d_arg_internal_equal_cstr(text,
                                  "true"))
    {
        *_out = 1;

        return D_ARG_OK;
    }

    // the text must be an integer within intmax_t
    if ( (d_arg_internal_read_integer(text,
                                      _rule,
                                      &integer) != D_ARG_OK) ||
         (integer.overflow) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    const uintmax_t limit = integer.negative
                            ? ((uintmax_t)INTMAX_MAX + 1u)
                            : (uintmax_t)INTMAX_MAX;

    // beyond intmax_t either way
    if (integer.magnitude > limit)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    // negation in uintmax_t, so INTMAX_MIN's magnitude does not overflow
    *_out = integer.negative
            ? (intmax_t)(-(intmax_t)(integer.magnitude - 1u) - 1)
            : (intmax_t)integer.magnitude;

    return D_ARG_OK;
}

/*
d_arg_internal_unsigned_value
  CLI11's unsigned integral_conversion: any minus sign is refused, even on
zero, since strtoull would silently wrap it; `true` is not a number here.
*/
D_STATIC int
d_arg_internal_unsigned_value(
    struct d_pack_text _text,
    int                _rule,
    uintmax_t*         _out
)
{
    struct d_arg_internal_integer integer = D_INTERNAL_ARG_INTEGER_INIT;

    // an unsigned integer is never negative, nor too wide
    if ( (d_arg_internal_read_integer(_text,
                                      _rule,
                                      &integer) != D_ARG_OK) ||
         (integer.negative)                                  ||
         (integer.overflow) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    *_out = integer.magnitude;

    return D_ARG_OK;
}

/*
d_arg_internal_flag_character
  CLI11's to_flag_value for a single character, already folded to lower
case: a digit from 1 to 9 is its value; 0, f, n and - are -1; t, y and +
are 1; anything else is not a flag value.
*/
D_STATIC bool
d_arg_internal_flag_character(
    char  _c,
    long* _out
)
{
    // a digit from 1 to 9 is its own value
    if ( (_c >= '1') &&
         (_c <= '9') )
    {
        *_out = (long)(_c - '0');

        return true;
    }

    // the single-character spellings of false and true
    switch (_c)
    {
        case '0':
        case 'f':
        case 'n':
        case '-':
            *_out = -1;

            return true;

        case 't':
        case 'y':
        case '+':
            *_out = 1;

            return true;

        default:
            break;
    }

    return false;
}

/*
d_arg_internal_flag_word
  CLI11's to_flag_value for everything but numbers: a single character by
its own table, then the truth words, both with case folded, as CLI11 folds
it.
*/
D_STATIC bool
d_arg_internal_flag_word(
    struct d_pack_text _text,
    long*              _out
)
{
    // a single character has its own table
    if (_text.length == 1u)
    {
        return d_arg_internal_flag_character(
                   d_arg_internal_to_lower(_text.data[0]),
                   _out);
    }

    // then the words, either way
    for (size_t index = 0u; index < D_INTERNAL_ARG_WORD_COUNT; ++index)
    {
        if (d_arg_internal_equal_nocase_cstr(_text,
                                             TRUE_WORDS[index]))
        {
            *_out = 1;

            return true;
        }

        if (d_arg_internal_equal_nocase_cstr(_text,
                                             FALSE_WORDS[index]))
        {
            *_out = -1;

            return true;
        }
    }

    return false;
}

/*
d_arg_internal_flag_shaped
  Whether a number has the shape C23's strtoll reads, which CLI11's
to_flag_value reads numbers with: no digit separator, at which strtoll stops
short, and, past the space and the sign, no `0o` prefix, which no strtoll
knows. A `0b` prefix is read, as C23 reads it -- and the C libraries that
follow it, glibc's among them, where CLI11 is built most.
*/
D_STATIC bool
d_arg_internal_flag_shaped(
    struct d_pack_text _text
)
{
    size_t index = 0u;

    // a separator anywhere stops strtoll short of the end
    if ( (memchr(_text.data,
                 '_',
                 _text.length) != NULL) ||
         (memchr(_text.data,
                 '\'',
                 _text.length) != NULL) )
    {
        return false;
    }

    // past the space before the number
    while ( (index < _text.length) &&
            (d_arg_internal_is_space(_text.data[index])) )
    {
        ++index;
    }

    // and past its sign
    if ( (index < _text.length)      &&
         ( (_text.data[index] == '+') ||
           (_text.data[index] == '-') ) )
    {
        ++index;
    }

    return !( ((_text.length - index) >= 2u)    &&
              (_text.data[index] == '0')        &&
              ( (_text.data[index + 1u] == 'o') ||
                (_text.data[index + 1u] == 'O') ) );
}

/*
d_arg_internal_convert_bool
  d_arg_parse_bool into a `bool`, under the context's integer rule; the
empty text is `false`, as CLI11's lexical_assign value-initializes every
type but text from an empty value.
*/
D_STATIC int
d_arg_internal_convert_bool(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    // an empty value is the type's zero
    if (text.length == 0u)
    {
        *(bool*)_target = false;

        return D_ARG_OK;
    }

    bool      value  = false;
    const int status = d_arg_parse_bool(text,
                                        d_arg_internal_rule_of(_context),
                                        &value);

    // the target is written only with a truth value read
    if (status == D_ARG_OK)
    {
        *(bool*)_target = value;
    }

    return status;
}

/*
d_arg_internal_convert_char
  CLI11's char conversion: one character is itself -- untrimmed, so a space
is a space -- and anything longer is a signed 8-bit integer, the range CLI11
reads whatever the signedness of `char`; the empty text is NUL.
*/
D_STATIC int
d_arg_internal_convert_char(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    // an empty value is the type's zero
    if (text.length == 0u)
    {
        *(char*)_target = '\0';

        return D_ARG_OK;
    }

    // a single character is itself
    if (text.length == 1u)
    {
        *(char*)_target = text.data[0];

        return D_ARG_OK;
    }

    intmax_t value = 0;

    // otherwise a small integer
    if ( (d_arg_internal_signed_value(text,
                                      d_arg_internal_rule_of(_context),
                                      &value) != D_ARG_OK) ||
         (value < INT8_MIN)                                ||
         (value > INT8_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    const int8_t narrow = (int8_t)value;

    memcpy(_target,
           &narrow,
           sizeof(narrow));

    return D_ARG_OK;
}

/*
d_arg_internal_convert_integer
  One converter for every integer descriptor, built-in or not: the kind and
width come from the descriptor's flags and size, and the range check from
d_arg_internal_number_store, so an enumeration's descriptor can reuse it.
The empty text stores the number 0 of the descriptor's kind.
*/
D_STATIC int
d_arg_internal_convert_integer(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    const struct d_pack_text     text = d_arg_internal_first_value(_values,
                                                                   _count);
    const int                    rule = d_arg_internal_rule_of(_context);
    struct d_arg_internal_number number = D_INTERNAL_ARG_NUMBER_INIT;

    memset(&number,
           0,
           sizeof(number));
    number.kind = d_arg_internal_number_of(_type);

    int status = D_ARG_ERROR_CONVERSION;

    // an empty value is the type's zero
    if (text.length == 0u)
    {
        status = D_ARG_OK;
    }
    // signed and unsigned each by their own rules
    else if (number.kind == D_ARG_INTERNAL_NUMBER_SIGNED)
    {
        status = d_arg_internal_signed_value(text,
                                             rule,
                                             &number.s);
    }
    else if (number.kind == D_ARG_INTERNAL_NUMBER_UNSIGNED)
    {
        status = d_arg_internal_unsigned_value(text,
                                               rule,
                                               &number.u);
    }

    // the value must also fit the type
    if ( (status != D_ARG_OK) ||
         (!d_arg_internal_number_store(_type,
                                       &number,
                                       _target)) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    return D_ARG_OK;
}

/*
d_arg_internal_convert_float
  Through strtof, so a value is rounded once, to the type, where CLI11 reads
every type as long double and rounds again. The empty text is zero.
*/
D_STATIC int
d_arg_internal_convert_float(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;
    (void)_context;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    // an empty value is the type's zero
    if (text.length == 0u)
    {
        *(float*)_target = 0.0f;

        return D_ARG_OK;
    }

    return d_arg_internal_read_real(text,
                                    (int)D_ARG_INTERNAL_REAL_FLOAT,
                                    _target);
}

/*
d_arg_internal_convert_double
  As d_arg_internal_convert_float, through strtod.
*/
D_STATIC int
d_arg_internal_convert_double(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;
    (void)_context;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    // an empty value is the type's zero
    if (text.length == 0u)
    {
        *(double*)_target = 0.0;

        return D_ARG_OK;
    }

    return d_arg_internal_read_real(text,
                                    (int)D_ARG_INTERNAL_REAL_DOUBLE,
                                    _target);
}

/*
d_arg_internal_convert_long_double
  As d_arg_internal_convert_float, through strtold.
*/
D_STATIC int
d_arg_internal_convert_long_double(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;
    (void)_context;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    // an empty value is the type's zero
    if (text.length == 0u)
    {
        *(long double*)_target = 0.0L;

        return D_ARG_OK;
    }

    return d_arg_internal_read_real(text,
                                    (int)D_ARG_INTERNAL_REAL_LONG_DOUBLE,
                                    _target);
}

/*
d_arg_internal_convert_string
  Points a `const char*` at the value itself, which every value a parse
produces keeps NUL-terminated for as long as the parse's results live.
*/
D_STATIC int
d_arg_internal_convert_string(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;
    (void)_context;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    memcpy(_target,
           &text.data,
           sizeof(text.data));

    return D_ARG_OK;
}

/*
d_arg_internal_convert_text
  Copies the span itself, with the same lifetime as a string's.
*/
D_STATIC int
d_arg_internal_convert_text(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    (void)_type;
    (void)_context;

    const struct d_pack_text text = d_arg_internal_first_value(_values,
                                                               _count);

    memcpy(_target,
           &text,
           sizeof(text));

    return D_ARG_OK;
}

/*
d_arg_internal_format_bool
  "true" or "false", the words d_arg_parse_bool reads first.
*/
D_STATIC int
d_arg_internal_format_bool(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    return d_arg_sink_cstr(_sink,
                           (*(const bool*)_value) ? "true" : "false");
}

/*
d_arg_internal_format_char
  The character itself, which its converter reads back as itself.
*/
D_STATIC int
d_arg_internal_format_char(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    return d_arg_sink_write(_sink,
                            (const char*)_value,
                            1u);
}

/*
d_arg_internal_format_integer
  Decimal, through the widened value, for any integer descriptor.
*/
D_STATIC int
d_arg_internal_format_integer(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    struct d_arg_internal_number      number = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number_text text   = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    // only an integer descriptor has an integer to write
    if (!d_arg_internal_number_load(_type,
                                    _value,
                                    &number))
    {
        return D_ARG_ERROR_UNSUPPORTED;
    }

    // signed and unsigned through their own digits
    if (number.kind == D_ARG_INTERNAL_NUMBER_SIGNED)
    {
        d_arg_internal_text_signed(&text,
                                   number.s);
    }
    else
    {
        d_arg_internal_text_unsigned(&text,
                                     number.u);
    }

    return d_arg_sink_write(_sink,
                            text.data,
                            text.length);
}

/*
d_arg_internal_format_real
  The shortest text that reads back as the value, for the real type given;
shared by the three floating-point descriptors.
*/
D_STATIC int
d_arg_internal_format_real(
    int                _real,
    const void*        _value,
    struct d_pack_sink _sink
)
{
    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    // a runtime that cannot print the number leaves nothing to write
    if (!d_arg_internal_text_real(&text,
                                  _real,
                                  _value,
                                  0))
    {
        return D_ARG_ERROR_UNSUPPORTED;
    }

    return d_arg_sink_write(_sink,
                            text.data,
                            text.length);
}

/*
d_arg_internal_format_float
  d_arg_internal_format_real for `float`.
*/
D_STATIC int
d_arg_internal_format_float(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    return d_arg_internal_format_real((int)D_ARG_INTERNAL_REAL_FLOAT,
                                      _value,
                                      _sink);
}

/*
d_arg_internal_format_double
  d_arg_internal_format_real for `double`.
*/
D_STATIC int
d_arg_internal_format_double(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    return d_arg_internal_format_real((int)D_ARG_INTERNAL_REAL_DOUBLE,
                                      _value,
                                      _sink);
}

/*
d_arg_internal_format_long_double
  d_arg_internal_format_real for `long double`.
*/
D_STATIC int
d_arg_internal_format_long_double(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    return d_arg_internal_format_real((int)D_ARG_INTERNAL_REAL_LONG_DOUBLE,
                                      _value,
                                      _sink);
}

/*
d_arg_internal_format_string
  The string itself; NULL writes nothing.
*/
D_STATIC int
d_arg_internal_format_string(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    const char* text = NULL;

    memcpy(&text,
           _value,
           sizeof(text));

    return d_arg_sink_cstr(_sink,
                           text);
}

/*
d_arg_internal_format_text
  The span itself.
*/
D_STATIC int
d_arg_internal_format_text(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    (void)_type;

    struct d_pack_text text = d_arg_internal_span("",
                                                  0u);

    memcpy(&text,
           _value,
           sizeof(text));

    return d_arg_sink_write(_sink,
                            text.data,
                            (text.data != NULL) ? text.length : 0u);
}

// D_INTERNAL_ARG_SIGNED
//   macro: the flags of a signed integer descriptor.
#define D_INTERNAL_ARG_SIGNED   (D_ARG_TYPE_FLAG_INTEGER |                     \
                                 D_ARG_TYPE_FLAG_SIGNED)

// D_INTERNAL_ARG_UNSIGNED
//   macro: the flags of an unsigned integer descriptor.
#define D_INTERNAL_ARG_UNSIGNED (D_ARG_TYPE_FLAG_INTEGER)

// D_INTERNAL_ARG_REAL
//   macro: the flags of a floating-point descriptor, which holds negative
// values, so a negated flag bound to one is -1.
#define D_INTERNAL_ARG_REAL     (D_ARG_TYPE_FLAG_FLOAT | D_ARG_TYPE_FLAG_SIGNED)

// D_INTERNAL_ARG_CHAR
//   macro: the flags of `char`, signed where the platform's is.
#define D_INTERNAL_ARG_CHAR     (D_ARG_TYPE_FLAG_CHAR |                        \
                                 ((CHAR_MIN < 0) ? D_ARG_TYPE_FLAG_SIGNED : 0u))

// BUILTINS
//   constant: the built-in descriptors, one row per d_arg_builtin in the
// enumeration's order. Every one is a scalar taking one value: name,
// convert, format, data, size, type sizes, expected items, flags.
static const struct d_arg_type BUILTINS[] =
{
    { "BOOLEAN", d_arg_internal_convert_bool, d_arg_internal_format_bool,
      NULL, sizeof(bool), 1, 1, 1, D_ARG_TYPE_FLAG_BOOL },
    { "CHAR", d_arg_internal_convert_char, d_arg_internal_format_char,
      NULL, sizeof(char), 1, 1, 1, D_INTERNAL_ARG_CHAR },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(signed char), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(unsigned char), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(short), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(unsigned short), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(int), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(unsigned int), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(long), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(unsigned long), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(long long), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(unsigned long long), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(size_t), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(ptrdiff_t), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(int8_t), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(int16_t), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(int32_t), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(int64_t), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(uint8_t), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(uint16_t), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(uint32_t), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(uint64_t), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "INT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(intmax_t), 1, 1, 1, D_INTERNAL_ARG_SIGNED },
    { "UINT", d_arg_internal_convert_integer, d_arg_internal_format_integer,
      NULL, sizeof(uintmax_t), 1, 1, 1, D_INTERNAL_ARG_UNSIGNED },
    { "FLOAT", d_arg_internal_convert_float, d_arg_internal_format_float,
      NULL, sizeof(float), 1, 1, 1, D_INTERNAL_ARG_REAL },
    { "FLOAT", d_arg_internal_convert_double, d_arg_internal_format_double,
      NULL, sizeof(double), 1, 1, 1, D_INTERNAL_ARG_REAL },
    { "FLOAT", d_arg_internal_convert_long_double,
      d_arg_internal_format_long_double,
      NULL, sizeof(long double), 1, 1, 1, D_INTERNAL_ARG_REAL },
    { "TEXT", d_arg_internal_convert_string, d_arg_internal_format_string,
      NULL, sizeof(const char*), 1, 1, 1, D_ARG_TYPE_FLAG_STRING },
    { "TEXT", d_arg_internal_convert_text, d_arg_internal_format_text,
      NULL, sizeof(struct d_pack_text), 1, 1, 1, D_ARG_TYPE_FLAG_STRING }
};

D_STATIC_ASSERT( (sizeof(BUILTINS) / sizeof(BUILTINS[0])) ==
                 (size_t)D_ARG_BUILTIN_COUNT,
                 "every d_arg_builtin needs a descriptor" );

/*
d_arg_internal_sum_integers
  The exact half of CLI11's sum_string_vector: every value a 64-bit integer
-- `true` counting as 1, as CLI11's signed conversion reads it -- and no
partial sum overflowing. False when either fails, so the caller falls back
to floating point.
*/
D_STATIC bool
d_arg_internal_sum_integers(
    const struct d_pack_text* _values,
    size_t                    _count,
    int                       _rule,
    int64_t*                  _sum
)
{
    int64_t sum = 0;

    // each value, checked against overflow before it is added
    for (size_t index = 0u; index < _count; ++index)
    {
        intmax_t value = 0;

        if ( (d_arg_internal_signed_value(_values[index],
                                          _rule,
                                          &value) != D_ARG_OK) ||
             (value < INT64_MIN)                               ||
             (value > INT64_MAX)                               ||
             ( (value > 0) &&
               (sum > (INT64_MAX - (int64_t)value)) )          ||
             ( (value < 0) &&
               (sum < (INT64_MIN - (int64_t)value)) ) )
        {
            return false;
        }

        sum += (int64_t)value;
    }

    *_sum = sum;

    return true;
}

/*
d_arg_internal_sum_reals
  The floating half: each value as a double, or else as its flag value; a
value that is neither ends the sum, and the caller concatenates instead.
*/
D_STATIC bool
d_arg_internal_sum_reals(
    const struct d_pack_text* _values,
    size_t                    _count,
    int                       _rule,
    double*                   _sum
)
{
    double sum = 0.0;

    // each value as a number, or a flag word's number
    for (size_t index = 0u; index < _count; ++index)
    {
        double   value = 0.0;
        intmax_t flag  = 0;

        if (d_arg_parse_double(_values[index],
                               &value) == D_ARG_OK)
        {
            sum += value;
        }
        else if (d_arg_flag_value_intmax(_values[index],
                                         _rule,
                                         &flag) == D_ARG_OK)
        {
            sum += (double)flag;
        }
        else
        {
            return false;
        }
    }

    *_sum = sum;

    return true;
}

//==============================================================================
// 1.  TYPE DESCRIPTORS
//==============================================================================

/*
d_arg_type_builtin
  A table lookup; see BUILTINS.
*/
const struct d_arg_type*
d_arg_type_builtin(
    int _builtin
)
{
    // outside the enumeration there is no descriptor
    if ( (_builtin < 0) ||
         (_builtin >= (int)D_ARG_BUILTIN_COUNT) )
    {
        return NULL;
    }

    return &BUILTINS[_builtin];
}

//==============================================================================
// 2.  CONVERSIONS
//==============================================================================

/*
d_arg_parse_long
  The signed reading into intmax_t, then `long`'s range.
*/
int
d_arg_parse_long(
    struct d_pack_text _text,
    int                _rule,
    long*              _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    intmax_t value = 0;

    // an integer within `long`
    if ( (d_arg_internal_signed_value(_text,
                                      _rule,
                                      &value) != D_ARG_OK) ||
         (value < LONG_MIN)                                ||
         (value > LONG_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    *_out = (long)value;

    return D_ARG_OK;
}

/*
d_arg_parse_ulong
  The unsigned reading into uintmax_t, then `unsigned long`'s range.
*/
int
d_arg_parse_ulong(
    struct d_pack_text _text,
    int                _rule,
    unsigned long*     _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    uintmax_t value = 0u;

    // a non-negative integer within `unsigned long`
    if ( (d_arg_internal_unsigned_value(_text,
                                        _rule,
                                        &value) != D_ARG_OK) ||
         (value > ULONG_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    *_out = (unsigned long)value;

    return D_ARG_OK;
}

/*
d_arg_parse_intmax
  The signed reading, which is already intmax_t's.
*/
int
d_arg_parse_intmax(
    struct d_pack_text _text,
    int                _rule,
    intmax_t*          _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    intmax_t  value  = 0;
    const int status = d_arg_internal_signed_value(_text,
                                                   _rule,
                                                   &value);

    // the output is written only with a value read
    if (status == D_ARG_OK)
    {
        *_out = value;
    }

    return status;
}

/*
d_arg_parse_uintmax
  The unsigned reading, which is already uintmax_t's.
*/
int
d_arg_parse_uintmax(
    struct d_pack_text _text,
    int                _rule,
    uintmax_t*         _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    uintmax_t value  = 0u;
    const int status = d_arg_internal_unsigned_value(_text,
                                                     _rule,
                                                     &value);

    // the output is written only with a value read
    if (status == D_ARG_OK)
    {
        *_out = value;
    }

    return status;
}

/*
d_arg_parse_double
  d_arg_internal_read_real for a double.
*/
int
d_arg_parse_double(
    struct d_pack_text _text,
    double*            _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_read_real(_text,
                                    (int)D_ARG_INTERNAL_REAL_DOUBLE,
                                    _out);
}

/*
d_arg_parse_long_double
  d_arg_internal_read_real for a long double.
*/
int
d_arg_parse_long_double(
    struct d_pack_text _text,
    long double*       _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_read_real(_text,
                                    (int)D_ARG_INTERNAL_REAL_LONG_DOUBLE,
                                    _out);
}

/*
d_arg_parse_bool
  The flag words decide first; otherwise the text must be an integer under
the rule, true above 0. An integer too wide for any type is still read by
its sign, as CLI11 reads strtoll's ERANGE.
*/
int
d_arg_parse_bool(
    struct d_pack_text _text,
    int                _rule,
    bool*              _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    long word = 0;

    // nothing is 0, as CLI11's strtoll reads it, so false
    if (_text.length == 0u)
    {
        *_out = false;

        return D_ARG_OK;
    }

    // a word or a single character decides by its flag value
    if (d_arg_internal_flag_word(_text,
                                 &word))
    {
        *_out = (word > 0);

        return D_ARG_OK;
    }

    struct d_arg_internal_integer integer = D_INTERNAL_ARG_INTEGER_INIT;

    // otherwise an integer, true above zero, which whitespace may precede
    // but not follow, in the shape strtoll reads
    if ( (d_arg_internal_is_space(_text.data[_text.length - 1u])) ||
         (!d_arg_internal_flag_shaped(_text))                     ||
         (d_arg_internal_read_integer(_text,
                                      _rule,
                                      &integer) != D_ARG_OK) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    *_out = ( (!integer.negative) &&
              (integer.magnitude > 0u) );

    return D_ARG_OK;
}

/*
d_arg_flag_value
  d_arg_flag_value_intmax's number, which must also fit `long`.
*/
int
d_arg_flag_value(
    struct d_pack_text _text,
    int                _rule,
    long*              _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    intmax_t value = 0;

    // a flag value within `long`
    if ( (d_arg_flag_value_intmax(_text,
                                  _rule,
                                  &value) != D_ARG_OK) ||
         (value < LONG_MIN)                            ||
         (value > LONG_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    *_out = (long)value;

    return D_ARG_OK;
}

/*
d_arg_flag_value_intmax
  The flag words, then an integer under the rule, which must fit `long
long`: CLI11's strtoll reads it into one, and a number one cannot hold is
no flag value there.
*/
int
d_arg_flag_value_intmax(
    struct d_pack_text _text,
    int                _rule,
    intmax_t*          _out
)
{
    // somewhere to put the value
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    long word = 0;

    // nothing is 0, as CLI11's strtoll reads it
    if (_text.length == 0u)
    {
        *_out = 0;

        return D_ARG_OK;
    }

    // a word or a single character has its own value
    if (d_arg_internal_flag_word(_text,
                                 &word))
    {
        *_out = (intmax_t)word;

        return D_ARG_OK;
    }

    intmax_t value = 0;

    // otherwise an integer within `long long`, which whitespace may precede
    // but not follow, in the shape strtoll reads
    if ( (d_arg_internal_is_space(_text.data[_text.length - 1u])) ||
         (!d_arg_internal_flag_shaped(_text))                     ||
         (d_arg_internal_signed_value(_text,
                                      _rule,
                                      &value) != D_ARG_OK)        ||
         (value < LLONG_MIN)                                      ||
         (value > LLONG_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    *_out = value;

    return D_ARG_OK;
}

/*
d_arg_convert
  Supplies the default context when none is given, so a descriptor's convert
function always receives one.
*/
int
d_arg_convert(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    // a descriptor that converts, a target, and values for a count
    if ( (!_type)          ||
         (!_type->convert) ||
         (!_target)        ||
         ( (!_values) &&
           (_count > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const struct d_arg_convert_context defaults =
    {
        (int)D_INTERNAL_ARG_PARSER_INTEGER_RULE,
        NULL,
        NULL
    };

    return _type->convert(_type,
                          _values,
                          _count,
                          _target,
                          (_context != NULL) ? _context : &defaults);
}

/*
d_arg_format
  A descriptor without a format function has no text form.
*/
int
d_arg_format(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    // a descriptor and a value
    if ( (!_type) ||
         (!_value) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a type with no text form
    if (!_type->format)
    {
        return D_ARG_ERROR_UNSUPPORTED;
    }

    return _type->format(_type,
                         _value,
                         _sink);
}

/*
d_arg_internal_sum_numbers
  The first two tiers of CLI11's sum_string_vector: an exact integer sum
when every value is an integer, else a `double` sum written to sixteen
significant digits when every value is a number or a flag word; false when
neither applies.
*/
D_STATIC bool
d_arg_internal_sum_numbers(
    const struct d_pack_text*          _values,
    size_t                             _count,
    int                                _rule,
    struct d_arg_internal_number_text* _text
)
{
    int64_t integer = 0;
    double  real    = 0.0;

    // every value an integer
    if (d_arg_internal_sum_integers(_values,
                                    _count,
                                    _rule,
                                    &integer))
    {
        d_arg_internal_text_signed(_text,
                                   (intmax_t)integer);

        return true;
    }

    return ( (d_arg_internal_sum_reals(_values,
                                       _count,
                                       _rule,
                                       &real)) &&
             (d_arg_internal_text_real(_text,
                                       (int)D_ARG_INTERNAL_REAL_DOUBLE,
                                       &real,
                                       16)) );
}

/*
d_arg_sum_values
  CLI11's sum_string_vector in its three tiers: exact integers, then doubles
written to sixteen significant digits, then the values side by side.
*/
int
d_arg_sum_values(
    const struct d_pack_text* _values,
    size_t                    _count,
    int                       _rule,
    struct d_arg_buffer*      _out
)
{
    // a buffer, and values for a count
    if ( (!_out) ||
         ( (!_values) &&
           (_count > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    // an exact integer sum, else a floating one, when the values allow
    if (d_arg_internal_sum_numbers(_values,
                                   _count,
                                   _rule,
                                   &text))
    {
        return d_arg_buffer_append(_out,
                                   text.data,
                                   text.length);
    }

    int status = D_ARG_OK;

    // otherwise the values themselves, one after another
    for (size_t index = 0u; ( (index < _count) &&
                              (status == D_ARG_OK) ); ++index)
    {
        status = d_arg_buffer_append_text(_out,
                                          _values[index]);
    }

    return status;
}
