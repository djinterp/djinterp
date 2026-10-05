/*******************************************************************************
* djinterp [parse]                                                 lex_literal.c
*
* Definitions for lex_literal.h.
*   A numeric literal is read left to right in one pass: prefix, digits,
* fraction, exponent, and whatever is left is the suffix.  Every question the
* standards answer differently -- binary literals, separators, hexadecimal
* floats, which suffixes exist -- is put to the dialect, never to a language
* test, so a level that gains a form gains it here by its feature bit.
*   A quoted literal is read as its prefix, its body and its suffix, the body
* one c-char at a time.  Each c-char is measured in code units of the
* literal's encoding, since that is what the standards constrain: a character
* literal holds one code unit, not one character.
*
*
* path:      /src/djinterp/parse/lex/lex_literal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/djinterp/parse/lex/lex_literal.h"  // corresponding header
// std
#include <limits.h>   // CHAR_BIT
#include <locale.h>   // localeconv
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free, strtod
#include <string.h>   // memset, memcmp, strcmp
#include <wchar.h>    // WCHAR_MAX
// djinterp
#include "../../../../inc/djinterp/parse/lex/lex_unicode.h"      // UTF-8
#include "../../../../inc/djinterp/parse/source/source_reader.h" // decoding
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uint16_t,
                                                     // uint64_t, UINT64_MAX,
                                                     // ...


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Decoding state
//------------------------------------------------------------------------------
// 1.1.1
// D_INTERNAL_FLOAT_MAX
//   constant: the longest floating literal whose value is computed.  Past
// this the literal is still checked, but its value is not, and says so.
#define D_INTERNAL_FLOAT_MAX 256

// 1.1.2
// D_INTERNAL_UNKNOWN
//   constant: the value of a named character, which is not resolved.
#define D_INTERNAL_UNKNOWN 0xFFFFFFFFu

// 1.1.3
// d_internal_decode
//   struct: one decode in progress.  `exact` says the spelling is the token's
//   byte range, so an index into it is a source offset and a diagnostic can
//   point at the character rather than at the token.
struct d_internal_decode
{
    const struct d_lex_dialect* dialect;
    const struct d_lex_target*  target;
    const struct d_token*       token;
    const char*                 text;
    size_t                      length;
    struct d_parse_diag_sink*   sink;
    struct d_lex_literal*       out;
    bool                        exact;
};

// 1.1.4
// d_internal_char
//   struct: one decoded c-char: its value, the code units it occupies in the
//   literal's encoding, and whether it was an escape that names a code unit
//   directly rather than a character.
struct d_internal_char
{
    uint32_t value;
    uint32_t units;
    bool     unit_escape;
};


// 1.1.5
// d_internal_float_suffix
//   struct: one standard floating suffix: its lower-case spelling, the width
//   it names, its D_LEX_SUFFIX_* bit, the feature a dialect needs for it, and
//   the languages it exists in -- 1 for C, 2 for C++, 3 for both.
struct d_internal_float_suffix
{
    const char* text;
    unsigned    width;
    unsigned    bit;
    unsigned    feature;
    unsigned    languages;
};

// 1.1.6
// d_internal_float_suffixes
//   constant: every standard floating suffix in either language.
static const struct d_internal_float_suffix d_internal_float_suffixes[] =
{
    { "f",     0u,   D_LEX_SUFFIX_FLOAT,
      0u,                            3u },
    { "l",     0u,   D_LEX_SUFFIX_LONG_DOUBLE,
      0u,                            3u },
    { "df",    32u,  D_LEX_SUFFIX_DECIMAL,
      D_LEX_FEATURE_DECIMAL_FLOAT,   1u },
    { "dd",    64u,  D_LEX_SUFFIX_DECIMAL,
      D_LEX_FEATURE_DECIMAL_FLOAT,   1u },
    { "dl",    128u, D_LEX_SUFFIX_DECIMAL,
      D_LEX_FEATURE_DECIMAL_FLOAT,   1u },
    { "d32",   32u,  D_LEX_SUFFIX_DECIMAL,
      D_LEX_FEATURE_DECIMAL_FLOAT,   1u },
    { "d64",   64u,  D_LEX_SUFFIX_DECIMAL,
      D_LEX_FEATURE_DECIMAL_FLOAT,   1u },
    { "d128",  128u, D_LEX_SUFFIX_DECIMAL,
      D_LEX_FEATURE_DECIMAL_FLOAT,   1u },
    { "f16",   16u,  D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  3u },
    { "f32",   32u,  D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  3u },
    { "f64",   64u,  D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  3u },
    { "f128",  128u, D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  3u },
    { "f32x",  32u,  D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  1u },
    { "f64x",  64u,  D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  1u },
    { "f128x", 128u, D_LEX_SUFFIX_EXTENDED,
      D_LEX_FEATURE_EXTENDED_FLOAT,  1u },
    { "bf16",  16u,  D_LEX_SUFFIX_BFLOAT,
      D_LEX_FEATURE_EXTENDED_FLOAT,  2u }
};


//==============================================================================
// 2.  HELPERS
//==============================================================================


/**
 * @brief Reports one condition, on the characters it concerns when the
 *        spelling is the source.
 *
 * @param[in,out] _d       the decode.
 * @param[in]     _code    a d_lex_error.
 * @param[in]     _at      where the condition begins in the spelling.
 * @param[in]     _length  how many bytes it covers.
 */
static void
d_internal_raise(
    struct d_internal_decode* _d,
    int                       _code,
    size_t                    _at,
    size_t                    _length
)
{
    const struct d_parse_span* const whole  = &_d->token->span;
    struct d_parse_span              span   = *whole;
    const bool                       single =
        (_d->token->end_line == whole->line);

    // count errors whether or not anyone is listening
    if (d_lex_error_severity(_code) >= D_PARSE_SEVERITY_ERROR)
    {
        ++_d->out->errors;
    }

    // a sub-range needs the bytes to be the source's and one line to count on
    if ( _d->exact &&
         single )
    {
        uint32_t column = whole->column;

        // a column counts code points, so continuation bytes do not advance it
        for (size_t at = 0u; at < _at; ++at)
        {
            if ((((unsigned char)_d->text[at]) & 0xC0u) != 0x80u)
            {
                ++column;
            }
        }

        span.offset = whole->offset + (uint32_t)_at;
        span.length = (uint32_t)_length;
        span.column = column;
    }

    (void)d_parse_diag_emit(_d->sink,
                            d_lex_error_severity(_code),
                            (uint16_t)D_PARSE_DIAG_DOMAIN_LEX,
                            (uint16_t)_code,
                            span,
                            d_lex_error_text(_code));

    return;
}


/**
 * @brief Reports a character's value as a digit in base 16, or -1.
 *
 * @param[in] _character  the character.
 * @return 0 to 15, or -1 when it is not a hexadecimal digit.
 */
static int
d_internal_digit_value(
    int _character
)
{
    if ( (_character >= '0') &&
         (_character <= '9') )
    {
        return _character - '0';
    }

    if ( (_character >= 'a') &&
         (_character <= 'f') )
    {
        return _character - 'a' + 10;
    }

    if ( (_character >= 'A') &&
         (_character <= 'F') )
    {
        return _character - 'A' + 10;
    }

    return -1;
}


/**
 * @brief Reports whether a character is a digit of a run in a base.
 *
 * @note A run of a binary or octal literal accepts every decimal digit, so
 *       that a bad digit is found and reported rather than taken for the
 *       start of a suffix.
 *
 * @param[in] _character  the character.
 * @param[in] _base       16, or anything else for decimal digits.
 * @return `true` if the character belongs to the run.
 */
static bool
d_internal_is_run_digit(
    int      _character,
    unsigned _base
)
{
    const int value = d_internal_digit_value(_character);

    return ( (value >= 0) &&
             ( (_base == 16u) ||
               (value < 10) ) );
}


/**
 * @brief Consumes a digit run, digit separators included, from a position.
 *
 * @note A separator is part of the run only between two digits of it; one
 *       anywhere else ends the run, and the caller finds it in what follows.
 *
 * @param[in,out] _d     the decode; the separator flag is set on it.
 * @param[in]     _at    where the run begins.
 * @param[in]     _base  16, or anything else for decimal digits.
 * @return the position just past the run.
 */
static size_t
d_internal_run(
    struct d_internal_decode* _d,
    size_t                    _at,
    unsigned                  _base
)
{
    size_t at = _at;

    for (;;)
    {
        // an ordinary digit
        if ( (at < _d->length) &&
             d_internal_is_run_digit(_d->text[at], _base) )
        {
            ++at;
            continue;
        }

        // a separator, standing between two digits of the run
        if ( (at > _at)                  &&
             ((at + 1u) < _d->length)    &&
             (_d->text[at] == '\'')      &&
             d_lex_dialect_has(_d->dialect,
                               D_LEX_FEATURE_DIGIT_SEPARATOR) &&
             d_internal_is_run_digit(_d->text[at + 1u], _base) )
        {
            _d->out->flags |= D_LEX_LITERAL_FLAG_SEPARATOR;
            at             += 2u;
            continue;
        }

        return at;
    }
}


/**
 * @brief Reports whether a suffix is one the standard library defines, and
 *        so is not reserved to the implementation in the user's hands.
 *
 * @param[in] _suffix  the suffix.
 * @param[in] _length  its length.
 * @param[in] _quoted  whether it follows a character or string literal.
 * @return `true` for `s`, `sv`, and the `<chrono>` and `<complex>` set.
 */
static bool
d_internal_library_suffix(
    const char* _suffix,
    size_t      _length,
    bool        _quoted
)
{
    static const char* const numeric[] =
    {
        "h", "min", "s", "ms", "us", "ns", "d", "y", "i", "il", "if", NULL
    };
    static const char* const quoted[] =
    {
        "s", "sv", NULL
    };

    const char* const* const list = _quoted ? quoted : numeric;

    for (size_t at = 0u; list[at]; ++at)
    {
        if ( (strlen(list[at]) == _length) &&
             (memcmp(list[at], _suffix, _length) == 0) )
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Accepts a suffix as user-defined where the dialect has them.
 *
 * @param[in,out] _d       the decode.
 * @param[in]     _at      where the suffix begins.
 * @param[in]     _quoted  whether it follows a character or string literal.
 * @return `true` if it was accepted, `false` if it is simply invalid.
 */
static bool
d_internal_user_suffix(
    struct d_internal_decode* _d,
    size_t                    _at,
    bool                      _quoted
)
{
    const char* const suffix = _d->text + _at;
    const size_t      length = _d->length - _at;
    const int         first  = (unsigned char)suffix[0];

    // a ud-suffix is an identifier, and exists only where the dialect says
    if ( (!d_lex_dialect_has(_d->dialect, D_LEX_FEATURE_USER_SUFFIX)) ||
         (!( ( (first >= 'a') && (first <= 'z') ) ||
             ( (first >= 'A') && (first <= 'Z') ) ||
             (first == '_')                        ||
             (first == '\\')                       ||
             (first >= 0x80) )) )
    {
        return false;
    }

    _d->out->suffix |= D_LEX_SUFFIX_USER;

    // a suffix without an underscore is the implementation's to declare;
    // using one the standard library declares is ordinary
    if ( (first != '_') &&
         (!d_internal_library_suffix(suffix, length, _quoted)) )
    {
        d_internal_raise(_d,
                         D_LEX_ERR_RESERVED_SUFFIX,
                         _at,
                         length);
    }

    return true;
}


//==============================================================================
// 3.  NUMBERS
//==============================================================================


/**
 * @brief Recognizes one integer length suffix at a position.
 *
 * @param[in]  _dialect   the dialect, for the lengths it has.
 * @param[in]  _suffix    the suffix.
 * @param[in]  _length    its length.
 * @param[in]  _at        where to look.
 * @param[out] _out_size  receives the length's D_LEX_SUFFIX_* bit.
 * @return how many characters it spans, or zero if none stands there.
 */
static size_t
d_internal_length_suffix(
    const struct d_lex_dialect* _dialect,
    const char*                 _suffix,
    size_t                      _length,
    size_t                      _at,
    unsigned*                   _out_size
)
{
    const char c    = _suffix[_at];
    const char next = ((_at + 1u) < _length) ? _suffix[_at + 1u] : '\0';

    // `ll` must be one case: `lL` is not a suffix
    if ( ( (c == 'l') || (c == 'L') ) &&
         (next == c) &&
         d_lex_dialect_has(_dialect, D_LEX_FEATURE_LONG_LONG) )
    {
        *_out_size = D_LEX_SUFFIX_LONG_LONG;

        return 2u;
    }

    if ( (c == 'l') ||
         (c == 'L') )
    {
        *_out_size = D_LEX_SUFFIX_LONG;

        return 1u;
    }

    if ( ( (c == 'z') || (c == 'Z') ) &&
         d_lex_dialect_has(_dialect, D_LEX_FEATURE_SIZE_SUFFIX) )
    {
        *_out_size = D_LEX_SUFFIX_SIZE;

        return 1u;
    }

    if ( ( ( (c == 'w') && (next == 'b') ) ||
           ( (c == 'W') && (next == 'B') ) ) &&
         d_lex_dialect_has(_dialect, D_LEX_FEATURE_BIT_PRECISE) )
    {
        *_out_size = D_LEX_SUFFIX_BIT_PRECISE;

        return 2u;
    }

    return 0u;
}


/**
 * @brief Classifies an integer literal's suffix.
 *
 * @note Any order of `u` with at most one length -- `l`, `ll`, `z` or `wb`
 *       -- each where the dialect has it.
 *
 * @param[in]  _dialect  the dialect.
 * @param[in]  _suffix   the suffix.
 * @param[in]  _length   its length.
 * @param[out] _out_bits receives the D_LEX_SUFFIX_* bits.
 * @return `true` if the suffix is a standard integer suffix.
 */
static bool
d_internal_integer_suffix(
    const struct d_lex_dialect* _dialect,
    const char*                 _suffix,
    size_t                      _length,
    unsigned*                   _out_bits
)
{
    unsigned bits = D_LEX_SUFFIX_NONE;
    unsigned size = D_LEX_SUFFIX_NONE;
    size_t   at   = 0u;

    while (at < _length)
    {
        const char c = _suffix[at];

        // `u`, once
        if ( ( (c == 'u') || (c == 'U') ) &&
             ((bits & D_LEX_SUFFIX_UNSIGNED) == 0u) )
        {
            bits |= D_LEX_SUFFIX_UNSIGNED;
            ++at;
            continue;
        }

        // a length, once
        const size_t width = (size == D_LEX_SUFFIX_NONE)
              ? d_internal_length_suffix(_dialect,
                                         _suffix,
                                         _length,
                                         at,
                                         &size)
              : 0u;

        if (width == 0u)
        {
            return false;
        }

        at += width;
    }

    *_out_bits = bits | size;

    return true;
}


/**
 * @brief Classifies a floating literal's suffix.
 *
 * @param[in]  _dialect    the dialect.
 * @param[in]  _suffix     the suffix.
 * @param[in]  _length     its length.
 * @param[out] _out_bits   receives the D_LEX_SUFFIX_* bit.
 * @param[out] _out_width  receives the width of an extended suffix.
 * @return `true` if the suffix is a standard floating suffix.
 */
static bool
d_internal_floating_suffix(
    const struct d_lex_dialect* _dialect,
    const char*                 _suffix,
    size_t                      _length,
    unsigned*                   _out_bits,
    unsigned*                   _out_width
)
{
    const unsigned language = (strcmp(_dialect->language, "c") == 0) ? 1u
                                                                      : 2u;

    const size_t rows = sizeof(d_internal_float_suffixes) /
                        sizeof(d_internal_float_suffixes[0]);

    for (size_t row = 0u; row < rows; ++row)
    {
        const struct d_internal_float_suffix* const entry =
            &d_internal_float_suffixes[row];
        const size_t width = strlen(entry->text);
        const bool   upper = ( (_suffix[0] >= 'A') &&
                               (_suffix[0] <= 'Z') );
        bool         match = (width == _length);

        // a suffix is all lower case or all upper case, `x` staying lower
        for (size_t at = 0u; match && (at < width); ++at)
        {
            const char want = entry->text[at];

            match = ( _suffix[at] ==
                      ( ( upper &&
                          (want >= 'a') && (want <= 'z') &&
                          (want != 'x') )
                        ? (char)(want - 'a' + 'A')
                        : want ) );
        }

        if ( match &&
             ((entry->languages & language) != 0u) &&
             ( (entry->feature == 0u) ||
               d_lex_dialect_has(_dialect, entry->feature) ) )
        {
            *_out_bits  = entry->bit;
            *_out_width = entry->width;

            return true;
        }
    }

    return false;
}


/**
 * @brief Computes an integer literal's value and checks its digits.
 *
 * @param[in,out] _d      the decode.
 * @param[in]     _first  where the digits begin, past any prefix.
 * @param[in]     _end    where they end.
 */
static void
d_internal_integer_value(
    struct d_internal_decode* _d,
    size_t                    _first,
    size_t                    _end
)
{
    const unsigned base  = _d->out->base;
    uint64_t       value = 0u;

    for (size_t at = _first; at < _end; ++at)
    {
        const int digit = d_internal_digit_value(_d->text[at]);

        // a separator carries no value
        if (_d->text[at] == '\'')
        {
            continue;
        }

        // a digit the base does not have: `9` in octal, `2` in binary
        if ( (digit < 0) ||
             ((unsigned)digit >= base) )
        {
            d_internal_raise(_d,
                             D_LEX_ERR_NUMBER_DIGIT,
                             at,
                             1u);

            return;
        }

        // past 64 bits the value is not kept, only the fact
        if (value > ((UINT64_MAX - (uint64_t)digit) / base))
        {
            _d->out->flags |= D_LEX_LITERAL_FLAG_OVERFLOW;
        }

        value = (value * base) + (uint64_t)digit;
    }

    _d->out->integer = value;

    return;
}


/**
 * @brief Computes a floating literal's value.
 *
 * @note strtod does the rounding, which is the correctly rounded conversion
 *       a hand-written one would have to reproduce.  Its decimal point is the
 *       locale's, so the literal's `.` is rewritten to it; separators are
 *       dropped.
 *
 * @param[in,out] _d    the decode.
 * @param[in]     _end  where the literal's number ends, before its suffix.
 */
static void
d_internal_floating_value(
    struct d_internal_decode* _d,
    size_t                    _end
)
{
    char         buffer[D_INTERNAL_FLOAT_MAX];
    const char   point = localeconv()->decimal_point[0];
    size_t       used  = 0u;

    // too long to convert here; the literal is still checked
    if (_end >= (size_t)D_INTERNAL_FLOAT_MAX)
    {
        _d->out->flags |= D_LEX_LITERAL_FLAG_NO_VALUE;

        return;
    }

    for (size_t at = 0u; at < _end; ++at)
    {
        if (_d->text[at] != '\'')
        {
            buffer[used] = (_d->text[at] == '.') ? point : _d->text[at];
            ++used;
        }
    }

    buffer[used]       = '\0';
    _d->out->floating  = strtod(buffer, NULL);

    return;
}


/**
 * @brief Reads a number's prefix and settles its base.
 *
 * @param[in,out] _d  the decode; `base` is set.
 * @return the position of the first digit.
 */
static size_t
d_internal_number_prefix(
    struct d_internal_decode* _d
)
{
    const char second = (_d->length > 1u) ? _d->text[1] : '\0';

    _d->out->base = 10u;

    if (_d->text[0] != '0')
    {
        return 0u;
    }

    if ( (second == 'x') ||
         (second == 'X') )
    {
        _d->out->base = 16u;

        return 2u;
    }

    if ( ( (second == 'b') ||
           (second == 'B') ) &&
         d_lex_dialect_has(_d->dialect, D_LEX_FEATURE_BINARY_LITERAL) )
    {
        _d->out->base = 2u;

        return 2u;
    }

    return 0u;
}


/**
 * @brief Reads the fraction and exponent a floating literal may have.
 *
 * @param[in,out] _d         the decode.
 * @param[in]     _at        the position after the integer digits.
 * @param[out]    _out_float receives whether either was present.
 * @param[out]    _out_exp   receives whether an exponent was present.
 * @return the position after them, or (size_t)-1 when an exponent has no
 *         digits, which has been reported.
 */
static size_t
d_internal_number_tail(
    struct d_internal_decode* _d,
    size_t                    _at,
    bool*                     _out_float,
    bool*                     _out_exp
)
{
    const bool hex    = (_d->out->base == 16u);
    const char lower  = hex ? 'p' : 'e';
    size_t     at     = _at;

    *_out_float = false;
    *_out_exp   = false;

    // a fraction, only in bases that have one
    if ( (at < _d->length) &&
         (_d->text[at] == '.') &&
         (_d->out->base != 2u) )
    {
        *_out_float = true;
        at          = d_internal_run(_d, at + 1u, _d->out->base);
    }

    // an exponent, `e` for decimal and `p` for hexadecimal
    if ( (at < _d->length) &&
         ( (_d->text[at] == lower) ||
           (_d->text[at] == (char)(lower - 'a' + 'A')) ) &&
         (_d->out->base != 2u) )
    {
        const size_t sign = at + 1u;
        const size_t from = ( (sign < _d->length) &&
                              ( (_d->text[sign] == '+') ||
                                (_d->text[sign] == '-') ) )
                          ? sign + 1u
                          : sign;
        const size_t to   = d_internal_run(_d, from, 10u);

        *_out_float = true;
        *_out_exp   = true;

        if (to == from)
        {
            d_internal_raise(_d, D_LEX_ERR_NUMBER_FORM, at, to - at);

            return (size_t)-1;
        }

        return to;
    }

    return at;
}


/**
 * @brief Checks a floating literal's form and suffix, and computes its value.
 *
 * @param[in,out] _d          the decode.
 * @param[in]     _end        where the number ends and the suffix begins.
 * @param[in]     _exponent   whether a `p` exponent was read.
 */
static void
d_internal_floating(
    struct d_internal_decode* _d,
    size_t                    _end,
    bool                      _exponent
)
{
    _d->out->category = D_LEX_LITERAL_FLOATING;

    // a hexadecimal floating literal needs its feature and its exponent
    if ( (_d->out->base == 16u) &&
         ( (!_exponent) ||
           (!d_lex_dialect_has(_d->dialect, D_LEX_FEATURE_HEX_FLOAT)) ) )
    {
        d_internal_raise(_d, D_LEX_ERR_NUMBER_FORM, 0u, _end);

        return;
    }

    d_internal_floating_value(_d, _end);

    // nothing to classify
    if (_end == _d->length)
    {
        return;
    }

    if ( (!d_internal_floating_suffix(_d->dialect,
                                      _d->text + _end,
                                      _d->length - _end,
                                      &_d->out->suffix,
                                      &_d->out->float_width)) &&
         (!d_internal_user_suffix(_d, _end, false)) )
    {
        d_internal_raise(_d,
                         D_LEX_ERR_NUMBER_SUFFIX,
                         _end,
                         _d->length - _end);
    }

    return;
}


/**
 * @brief Reports whether an integer literal's value fits no integer type the
 *        target has for it.
 *
 * @note A decimal literal without `u` has only signed types, so its limit is
 *       the target's `long long` maximum; every other literal may also be
 *       unsigned, and its limit is `unsigned long long`'s.
 *
 * @param[in] _d  the decode, its value and suffix already read.
 * @return `true` if the value is too large.
 */
static bool
d_internal_out_of_range(
    const struct d_internal_decode* _d
)
{
    const unsigned bits   = _d->target->long_long_bits;
    const uint64_t smax   = (bits >= 64u)
                          ? (uint64_t)INT64_MAX
                          : ((UINT64_C(1) << (bits - 1u)) - 1u);
    const uint64_t umax   = (bits >= 64u)
                          ? UINT64_MAX
                          : ((UINT64_C(1) << bits) - 1u);
    const bool     signed_only =
        ( (_d->out->base == 10u) &&
          ((_d->out->suffix & D_LEX_SUFFIX_UNSIGNED) == 0u) );

    // past 64 bits the value was not kept; it fits nothing
    if ((_d->out->flags & D_LEX_LITERAL_FLAG_OVERFLOW) != 0u)
    {
        return true;
    }

    return (_d->out->integer > (signed_only ? smax : umax));
}


/**
 * @brief Checks an integer literal's digits and suffix, and computes its
 *        value.
 *
 * @param[in,out] _d      the decode.
 * @param[in]     _first  where the digits begin.
 * @param[in]     _end    where the suffix begins.
 */
static void
d_internal_integer(
    struct d_internal_decode* _d,
    size_t                    _first,
    size_t                    _end
)
{
    _d->out->category = D_LEX_LITERAL_INTEGER;

    // a leading zero followed by more digits makes it octal
    if ( (_d->out->base == 10u) &&
         (_d->text[0] == '0') &&
         (_end > 1u) )
    {
        _d->out->base = 8u;
    }

    d_internal_integer_value(_d, _first, _end);

    const bool standard = (_end == _d->length) ||
        d_internal_integer_suffix(_d->dialect,
                                  _d->text + _end,
                                  _d->length - _end,
                                  &_d->out->suffix);

    if ( (!standard) &&
         (!d_internal_user_suffix(_d, _end, false)) )
    {
        d_internal_raise(_d,
                         D_LEX_ERR_NUMBER_SUFFIX,
                         _end,
                         _d->length - _end);

        return;
    }

    // too wide for every integer type on the target -- and a decimal
    // literal without `u` has only signed types -- unless a bit-precise type
    // or a literal operator takes it
    if ( d_internal_out_of_range(_d) &&
         ((_d->out->suffix & ( D_LEX_SUFFIX_BIT_PRECISE |
                               D_LEX_SUFFIX_USER )) == 0u) )
    {
        d_internal_raise(_d,
                         D_LEX_ERR_NUMBER_RANGE,
                         0u,
                         _end);
    }

    return;
}


/**
 * @brief Decodes a pp-number as a numeric literal.
 *
 * @param[in,out] _d  the decode.
 */
static void
d_internal_number(
    struct d_internal_decode* _d
)
{
    const size_t first     = d_internal_number_prefix(_d);
    const size_t digits    = d_internal_run(_d, first, _d->out->base);
    bool         floating  = false;
    bool         exponent  = false;
    const size_t end       = d_internal_number_tail(_d,
                                                    digits,
                                                    &floating,
                                                    &exponent);

    // an exponent with no digits, already reported
    if (end == (size_t)-1)
    {
        _d->out->category = D_LEX_LITERAL_FLOATING;

        return;
    }

    _d->out->suffix_offset = (uint32_t)end;
    _d->out->suffix_length = (uint32_t)(_d->length - end);

    // a prefix needs digits of its own: `0x` and `0b` alone are not numbers,
    // nor is a separator where a digit should be
    if ( ( (first > 0u) &&
           (digits == first) &&
           (!( floating &&
               (first < _d->length) &&
               (_d->text[first] == '.') &&
               d_internal_is_run_digit(_d->text[first + 1u], 16u) )) ) ||
         ( (end < _d->length) &&
           (_d->text[end] == '\'') ) )
    {
        _d->out->category = floating ? D_LEX_LITERAL_FLOATING
                                     : D_LEX_LITERAL_INTEGER;
        d_internal_raise(_d, D_LEX_ERR_NUMBER_FORM, 0u, _d->length);

        return;
    }

    if (floating)
    {
        d_internal_floating(_d,
                            end,
                            exponent);

        return;
    }

    d_internal_integer(_d, first, end);

    return;
}


//==============================================================================
// 4.  CHARACTERS AND STRINGS
//==============================================================================


/**
 * @brief Reports the width in bits of one code unit of an encoding on the
 *        target.
 *
 * @param[in] _d         the decode, for its target.
 * @param[in] _encoding  a D_LEX_ENCODING_* value.
 * @return the width, at most 32.
 */
static unsigned
d_internal_unit_bits(
    const struct d_internal_decode* _d,
    int                             _encoding
)
{
    const unsigned bits = (_encoding == D_LEX_ENCODING_UTF16) ? 16u
                        : (_encoding == D_LEX_ENCODING_UTF32) ? 32u
                        : (_encoding == D_LEX_ENCODING_WIDE)
                          ? _d->target->wchar_bits
                          : _d->target->char_bits;

    return (bits > 32u) ? 32u : bits;
}


/**
 * @brief Reports the largest value one code unit of an encoding holds.
 *
 * @param[in] _d         the decode, for its target.
 * @param[in] _encoding  a D_LEX_ENCODING_* value.
 * @return the maximum.
 */
static uint32_t
d_internal_unit_max(
    const struct d_internal_decode* _d,
    int                             _encoding
)
{
    const unsigned bits = d_internal_unit_bits(_d, _encoding);

    return (bits >= 32u) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
}


/**
 * @brief Reports how many code units a code point occupies in an encoding.
 *
 * @note A unit of 16 bits or more holds UTF-16 or UTF-32 by its width; one
 *       narrower holds UTF-8, the execution character set assumed for
 *       ordinary and `u8` literals.
 *
 * @param[in] _d         the decode, for its target.
 * @param[in] _encoding  a D_LEX_ENCODING_* value.
 * @param[in] _code      the code point.
 * @return the count; one for an unresolved named character.
 */
static uint32_t
d_internal_units_of(
    const struct d_internal_decode* _d,
    int                             _encoding,
    uint32_t                        _code
)
{
    const unsigned bits = d_internal_unit_bits(_d, _encoding);

    if (_code == D_INTERNAL_UNKNOWN)
    {
        return 1u;
    }

    if (bits >= 32u)
    {
        return 1u;
    }

    if (bits >= 16u)
    {
        return (_code > 0xFFFFu) ? 2u : 1u;
    }

    return (uint32_t)d_lex_utf8_length(_code);
}


/**
 * @brief Reads a braced digit sequence, the body of `\x{}`, `\o{}` or
 *        `\u{}`.
 *
 * @param[in]  _d          the decode.
 * @param[in]  _at         the position of the opening brace.
 * @param[in]  _end        the end of the body.
 * @param[in]  _base       8 or 16.
 * @param[out] _out_value  receives the value, saturated past 32 bits.
 * @return the position after the closing brace, or zero if there is no digit
 *         or no brace.
 */
static size_t
d_internal_braced(
    const struct d_internal_decode* _d,
    size_t                          _at,
    size_t                          _end,
    unsigned                        _base,
    uint64_t*                       _out_value
)
{
    size_t   at    = _at + 1u;
    uint64_t value = 0u;

    while (at < _end)
    {
        const int digit = d_internal_digit_value(_d->text[at]);

        if ( (digit < 0) ||
             ((unsigned)digit >= _base) )
        {
            break;
        }

        value = (value > 0xFFFFFFFFu) ? value : ((value * _base) +
                                                 (uint64_t)digit);
        ++at;
    }

    *_out_value = value;

    return ( (at > (_at + 1u)) &&
             (at < _end) &&
             (_d->text[at] == '}') )
           ? (at + 1u)
           : 0u;
}


/**
 * @brief Reads a run of up to `_most` digits in a base.
 *
 * @note Bounded by the spelling, not the body: a closing quote is never a
 *       digit, so it ends the run where the body does.
 *
 * @param[in]  _d          the decode.
 * @param[in]  _at         where the digits begin.
 * @param[in]  _base       8 or 16.
 * @param[in]  _most       the most digits to take.
 * @param[out] _out_value  receives the value, saturated past 32 bits.
 * @return the position after the digits; `_at` when there are none.
 */
static size_t
d_internal_digits(
    const struct d_internal_decode* _d,
    size_t                          _at,
    unsigned                        _base,
    size_t                          _most,
    uint64_t*                       _out_value
)
{
    size_t   at    = _at;
    uint64_t value = 0u;

    while ( (at < _d->length) &&
            ((at - _at) < _most) )
    {
        const int digit = d_internal_digit_value(_d->text[at]);

        if ( (digit < 0) ||
             ((unsigned)digit >= _base) )
        {
            break;
        }

        value = (value > 0xFFFFFFFFu) ? value : ((value * _base) +
                                                 (uint64_t)digit);
        ++at;
    }

    *_out_value = value;

    return at;
}


/**
 * @brief Finds the end of `\N{NAME}` whose brace stands at a position.
 *
 * @param[in] _d      the decode.
 * @param[in] _brace  where the opening brace stands.
 * @param[in] _end    the end of the body.
 * @return the position after the closing brace, or zero if the name is
 *         empty or unclosed.
 */
static size_t
d_internal_named_end(
    const struct d_internal_decode* _d,
    size_t                          _brace,
    size_t                          _end
)
{
    size_t at = _brace + 1u;

    while ( (at < _end) &&
            (_d->text[at] != '}') )
    {
        ++at;
    }

    return ( (at > (_brace + 1u)) &&
             (at < _end) )
           ? (at + 1u)
           : 0u;
}


/**
 * @brief Reports whether a UCN in a literal names a value it may.
 *
 * @note It must name a scalar value, and in C no basic or control character
 *       but `$`, `@` and `` ` ``; C++ allows those inside a literal.
 *
 * @param[in] _d     the decode.
 * @param[in] _code  the value, or D_INTERNAL_UNKNOWN for a named character.
 * @return `true` if the value is allowed.
 */
static bool
d_internal_ucn_allowed(
    const struct d_internal_decode* _d,
    uint32_t                        _code
)
{
    if (_code == D_INTERNAL_UNKNOWN)
    {
        return true;
    }

    if (!d_lex_unicode_is_scalar(_code))
    {
        return false;
    }

    return ( (_code >= 0xA0u) ||
             (_code == 0x24u) ||
             (_code == 0x40u) ||
             (_code == 0x60u) ||
             d_lex_dialect_has(_d->dialect,
                               D_LEX_FEATURE_UCN_ANY_LITERAL) );
}


/**
 * @brief Reads a universal character name inside a literal and checks the
 *        value it names.
 *
 * @param[in,out] _d      the decode.
 * @param[in]     _at     the position of the backslash.
 * @param[in]     _end    the end of the body.
 * @param[out]    _out    receives the character.
 * @return the position after the UCN.
 */
static size_t
d_internal_ucn(
    struct d_internal_decode* _d,
    size_t                    _at,
    size_t                    _end,
    struct d_internal_char*   _out
)
{
    const char     marker = _d->text[_at + 1u];
    const bool     braced = ( ((_at + 2u) < _end) &&
                              (_d->text[_at + 2u] == '{') );
    const size_t   fixed  = (marker == 'u') ? 4u : 8u;
    uint64_t       value  = D_INTERNAL_UNKNOWN;
    size_t         next   = 0u;

    _d->out->flags |= D_LEX_LITERAL_FLAG_UCN;

    // `\N{NAME}` is shape-checked only; `\u{}` where the dialect has it;
    // otherwise exactly four or eight digits
    if (marker == 'N')
    {
        next = braced ? d_internal_named_end(_d, _at + 2u, _end) : 0u;
    }
    else if ( (marker == 'u') &&
              braced &&
              d_lex_dialect_has(_d->dialect, D_LEX_FEATURE_DELIMITED_ESCAPE) )
    {
        next = d_internal_braced(_d, _at + 2u, _end, 16u, &value);
    }
    else
    {
        next = d_internal_digits(_d, _at + 2u, 16u, fixed, &value);
        next = ((next - (_at + 2u)) == fixed) ? next : 0u;
    }

    // not well-formed: take the backslash and marker, and report them
    if (next == 0u)
    {
        d_internal_raise(_d, D_LEX_ERR_UNIVERSAL_NAME, _at, 2u);
        _out->value = '?';
        _out->units = 1u;

        return _at + 2u;
    }

    _out->value = (value == D_INTERNAL_UNKNOWN)
                ? D_INTERNAL_UNKNOWN
                : (uint32_t)((value > 0x110000u) ? 0x110000u : value);
    _out->units = d_internal_units_of(_d, _d->out->encoding, _out->value);

    if (!d_internal_ucn_allowed(_d, _out->value))
    {
        d_internal_raise(_d, D_LEX_ERR_UNIVERSAL_NAME, _at, next - _at);
    }

    return next;
}


/**
 * @brief Reads a numeric escape -- octal or hexadecimal, braced or not --
 *        and checks that it fits one code unit.
 *
 * @param[in,out] _d    the decode.
 * @param[in]     _at   the position of the backslash.
 * @param[in]     _end  the end of the body.
 * @param[out]    _out  receives the character.
 * @return the position after the escape.
 */
static size_t
d_internal_numeric_escape(
    struct d_internal_decode* _d,
    size_t                    _at,
    size_t                    _end,
    struct d_internal_char*   _out
)
{
    const char     marker    = _d->text[_at + 1u];
    const bool     hex       = (marker == 'x');
    const bool     braced    = ( ((_at + 2u) < _end) &&
                                 (_d->text[_at + 2u] == '{') &&
                                 (hex || (marker == 'o')) &&
                                 d_lex_dialect_has(
                                     _d->dialect,
                                     D_LEX_FEATURE_DELIMITED_ESCAPE) );
    const unsigned base      = hex ? 16u : 8u;
    const size_t   from      = (hex || braced) ? (_at + 2u) : (_at + 1u);
    uint64_t       value     = 0u;
    size_t         next      = braced
        ? d_internal_braced(_d, from, _end, base, &value)
        : d_internal_digits(_d, from, base,
                            hex ? (size_t)-1 : 3u, &value);

    _d->out->flags     |= hex ? D_LEX_LITERAL_FLAG_HEX_ESC
                              : D_LEX_LITERAL_FLAG_OCTAL_ESC;
    _out->unit_escape   = true;
    _out->units         = 1u;

    // `\x` with no digit, or a brace left open
    if ( (next == 0u) ||
         (next == from) )
    {
        next = (next == 0u) ? (_at + 2u) : next;
        d_internal_raise(_d, D_LEX_ERR_ESCAPE_FORM, _at, next - _at);
        _out->value = 0u;

        return next;
    }

    if (value > d_internal_unit_max(_d, _d->out->encoding))
    {
        d_internal_raise(_d, D_LEX_ERR_ESCAPE_RANGE, _at, next - _at);
    }

    _out->value = (uint32_t)value;

    return next;
}


/**
 * @brief Reads one escape sequence.
 *
 * @param[in,out] _d    the decode.
 * @param[in]     _at   the position of the backslash.
 * @param[in]     _end  the end of the body.
 * @param[out]    _out  receives the character.
 * @return the position after the escape.
 */
static size_t
d_internal_escape(
    struct d_internal_decode* _d,
    size_t                    _at,
    size_t                    _end,
    struct d_internal_char*   _out
)
{
    static const char simple[]  = "'\"?\\abfnrtv";
    static const char values[]  = "'\"?\\\a\b\f\n\r\t\v";
    const char        marker    = _d->text[_at + 1u];
    const char* const found     = (marker != '\0') ? strchr(simple, marker)
                                                   : NULL;

    _out->units       = 1u;
    _out->unit_escape = false;

    if (found)
    {
        _out->value = (uint32_t)(unsigned char)values[found - simple];

        return _at + 2u;
    }

    if ( (marker == 'x') ||
         ( (marker >= '0') && (marker <= '7') ) ||
         ( (marker == 'o') &&
           d_lex_dialect_has(_d->dialect, D_LEX_FEATURE_DELIMITED_ESCAPE) ) )
    {
        return d_internal_numeric_escape(_d, _at, _end, _out);
    }

    if ( (marker == 'u') ||
         (marker == 'U') ||
         ( (marker == 'N') &&
           d_lex_dialect_has(_d->dialect, D_LEX_FEATURE_NAMED_ESCAPE) ) )
    {
        return d_internal_ucn(_d, _at, _end, _out);
    }

    // anything else is conditionally supported; it stands for itself,
    // however many bytes it is
    uint32_t     code  = (uint32_t)(unsigned char)marker;
    const size_t width = d_source_utf8_decode(_d->text + _at + 1u,
                                              _end - (_at + 1u),
                                              &code);

    d_internal_raise(_d, D_LEX_ERR_UNKNOWN_ESCAPE, _at, 2u);
    _out->value = code;
    _out->units = d_internal_units_of(_d, _d->out->encoding, code);

    return _at + 1u + ((width > 0u) ? width : 1u);
}


/**
 * @brief Reads one c-char or s-char: an escape, or a source character.
 *
 * @param[in,out] _d    the decode.
 * @param[in]     _at   where it begins.
 * @param[in]     _end  the end of the body.
 * @param[in]     _raw  whether the literal is raw, so has no escapes.
 * @param[out]    _out  receives the character.
 * @return the position after it.
 */
static size_t
d_internal_c_char(
    struct d_internal_decode* _d,
    size_t                    _at,
    size_t                    _end,
    bool                      _raw,
    struct d_internal_char*   _out
)
{
    if ( (!_raw) &&
         (_d->text[_at] == '\\') &&
         ((_at + 1u) < _end) )
    {
        return d_internal_escape(_d, _at, _end, _out);
    }

    uint32_t     code  = (uint32_t)(unsigned char)_d->text[_at];
    const size_t width = d_source_utf8_decode(_d->text + _at,
                                              _end - _at,
                                              &code);

    // ill-formed UTF-8 was reported by the scanner; count it a byte
    _out->value       = code;
    _out->units       = (width > 0u)
                      ? d_internal_units_of(_d, _d->out->encoding, code)
                      : 1u;
    _out->unit_escape = (width == 0u);

    return _at + ((width > 0u) ? width : 1u);
}


/**
 * @brief Reads a quoted literal's encoding prefix.
 *
 * @param[in,out] _d  the decode; `encoding` is set.
 * @return the prefix's length, `R` excluded.
 */
static size_t
d_internal_encoding_prefix(
    struct d_internal_decode* _d
)
{
    if ( (_d->length > 2u) &&
         (_d->text[0] == 'u') &&
         (_d->text[1] == '8') )
    {
        _d->out->encoding = D_LEX_ENCODING_UTF8;

        return 2u;
    }

    switch (_d->text[0])
    {
        case 'u':
            _d->out->encoding = D_LEX_ENCODING_UTF16;
            return 1u;
        case 'U':
            _d->out->encoding = D_LEX_ENCODING_UTF32;
            return 1u;
        case 'L':
            _d->out->encoding = D_LEX_ENCODING_WIDE;
            return 1u;
        default:
            return 0u;
    }
}


/**
 * @brief Narrows a raw string's bounds to its body.
 *
 * @note The body lies between `delimiter(` and `)delimiter"`; the scanner
 *       has already matched the two delimiters, so only their length is
 *       needed here.
 *
 * @param[in]     _d          the decode.
 * @param[in]     _quote      the position of the opening quote.
 * @param[in]     _close      the position just past the closing quote.
 * @param[in,out] _out_first  receives where the body begins.
 * @param[in,out] _out_end    receives where it ends.
 */
static void
d_internal_raw_body(
    const struct d_internal_decode* _d,
    size_t                          _quote,
    size_t                          _close,
    size_t*                         _out_first,
    size_t*                         _out_end
)
{
    const char* const open = memchr(_d->text + _quote,
                                    '(',
                                    _close - _quote);

    // the scanner rejects a raw string with no parenthesis
    if (!open)
    {
        return;
    }

    const size_t paren = (size_t)(open - _d->text);
    const size_t delim = paren - (_quote + 1u);

    *_out_first = paren + 1u;
    *_out_end   = (*_out_end >= (*_out_first + delim + 1u))
                ? (*_out_end - delim - 1u)
                : *_out_first;

    return;
}


/**
 * @brief Reads a quoted literal's prefix and finds its body and suffix.
 *
 * @param[in,out] _d          the decode; `encoding` and `category` are set.
 * @param[out]    _out_first  receives where the body begins.
 * @param[out]    _out_end    receives where it ends.
 * @param[out]    _out_raw    receives whether the literal is raw.
 * @return the position of the suffix, the spelling's end when none.
 */
static size_t
d_internal_quoted_bounds(
    struct d_internal_decode* _d,
    size_t*                   _out_first,
    size_t*                   _out_end,
    bool*                     _out_raw
)
{
    size_t at = d_internal_encoding_prefix(_d);

    *_out_raw = (_d->token->kind == D_TOKEN_RAW_STRING);
    at       += *_out_raw ? 1u : 0u;

    const char quote = _d->text[at];
    size_t     close = _d->length;

    _d->out->category = (quote == '\'') ? D_LEX_LITERAL_CHARACTER
                                         : D_LEX_LITERAL_STRING;

    // a suffix holds no quote, so the last quote closes the literal
    while ( (close > (at + 1u)) &&
            (_d->text[close - 1u] != quote) )
    {
        --close;
    }

    *_out_first = at + 1u;
    *_out_end   = (close > *_out_first) ? (close - 1u) : *_out_first;

    if (*_out_raw)
    {
        _d->out->flags |= D_LEX_LITERAL_FLAG_RAW;
        d_internal_raw_body(_d, at, close, _out_first, _out_end);
    }

    return close;
}


/**
 * @brief Checks a character literal once its c-chars are counted.
 *
 * @note A character literal holds one code unit.  What happens when it holds
 *       more depends on the prefix and the language:
 *         u8, and u or U in C++   ill-formed
 *         ordinary, L in C++      conditionally supported before C++23,
 *                                 ill-formed from it (P1854, P2362); an
 *                                 ordinary literal of several characters
 *                                 stays conditionally supported
 *         ordinary in C           implementation-defined
 *         L, u, U in C            implementation-defined
 *
 * @param[in,out] _d      the decode.
 * @param[in]     _units  the code units the c-chars occupy together.
 */
static void
d_internal_character_checks(
    struct d_internal_decode* _d,
    uint32_t                  _units
)
{
    const int  encoding = _d->out->encoding;
    const bool cpp      = (strcmp(_d->dialect->language, "c++") == 0);
    const bool several  = (_d->out->characters > 1u);
    const bool strict   = ( cpp &&
                            (_d->dialect->level >= D_LEX_CPP23) );
    int        code     = D_LEX_ERR_MULTICHARACTER;

    if (_units <= 1u)
    {
        return;
    }

    _d->out->flags |= D_LEX_LITERAL_FLAG_MULTICHAR;

    if ( (encoding == D_LEX_ENCODING_UTF8) ||
         ( cpp &&
           ( (encoding == D_LEX_ENCODING_UTF16) ||
             (encoding == D_LEX_ENCODING_UTF32) ) ) )
    {
        code = D_LEX_ERR_CHARACTER_WIDTH;
    }
    else if (cpp)
    {
        code = ( strict &&
                 ( (encoding == D_LEX_ENCODING_WIDE) ||
                   (!several) ) )
             ? D_LEX_ERR_CHARACTER_WIDTH
             : D_LEX_ERR_MULTICHARACTER;
    }
    else if ( (encoding != D_LEX_ENCODING_ORDINARY) &&
              (!several) )
    {
        code = D_LEX_ERR_WIDE_CHARACTER;
    }

    d_internal_raise(_d, code, 0u, _d->length);

    return;
}


/**
 * @brief Folds one character into a character literal's value.
 *
 * @note For an ordinary literal this is the value gcc and clang give: each
 *       code unit shifted in from the right, a UTF-8 character one byte at a
 *       time, so `'ab'` is 0x6162 and `'\u00e9'` is 0xC3A9.  A prefixed
 *       literal holds one code unit, and its value is the character's.
 *
 * @param[in] _encoding  the literal's encoding.
 * @param[in] _value     the value so far.
 * @param[in] _char      the character.
 * @return the new value.
 */
static uint64_t
d_internal_accumulate(
    int                           _encoding,
    uint64_t                      _value,
    const struct d_internal_char* _char
)
{
    unsigned char bytes[4] = { 0u, 0u, 0u, 0u };
    size_t        count    = 1u;

    // a prefixed literal is its character's value, except that a
    // character needing a surrogate pair keeps the last unit, as gcc and
    // clang do; the caller counts units, so two units mean a pair
    if (_encoding != D_LEX_ENCODING_ORDINARY)
    {
        return ( (_char->units == 2u) &&
                 (!_char->unit_escape) &&
                 (_char->value > 0xFFFFu) &&
                 (_char->value != D_INTERNAL_UNKNOWN) )
               ? (0xDC00u | ((_char->value - 0x10000u) & 0x3FFu))
               : _char->value;
    }

    bytes[0] = (unsigned char)(_char->value & 0xFFu);

    // a character, rather than a unit escape, contributes its UTF-8 bytes
    if ( (!_char->unit_escape) &&
         (_char->units > 1u) &&
         (_char->value != D_INTERNAL_UNKNOWN) )
    {
        const uint32_t code = _char->value;

        count    = _char->units;
        bytes[0] = (unsigned char)((count == 2u) ? (0xC0u | (code >> 6))
                                 : (count == 3u) ? (0xE0u | (code >> 12))
                                                 : (0xF0u | (code >> 18)));

        for (size_t at = 1u; at < count; ++at)
        {
            bytes[at] = (unsigned char)
                (0x80u | ((code >> (6u * (count - 1u - at))) & 0x3Fu));
        }
    }

    for (size_t at = 0u; at < count; ++at)
    {
        _value = (_value << 8) | bytes[at];
    }

    return _value;
}


/**
 * @brief Decodes a character or string literal.
 *
 * @param[in,out] _d  the decode.
 */
static void
d_internal_quoted(
    struct d_internal_decode* _d
)
{
    size_t       first  = 0u;
    size_t       end    = 0u;
    bool         raw    = false;
    const size_t suffix = d_internal_quoted_bounds(_d, &first, &end, &raw);
    uint32_t     units  = 0u;
    uint64_t     value  = 0u;

    for (size_t at = first; at < end; )
    {
        struct d_internal_char c = { 0u, 1u, false };

        at = d_internal_c_char(_d, at, end, raw, &c);

        ++_d->out->characters;
        units += c.units;
        value  = d_internal_accumulate(_d->out->encoding,
                                       value,
                                       &c);
    }

    _d->out->integer       = value & 0xFFFFFFFFu;
    _d->out->suffix_offset = (uint32_t)suffix;
    _d->out->suffix_length = (uint32_t)(_d->length - suffix);

    if (_d->out->category == D_LEX_LITERAL_CHARACTER)
    {
        d_internal_character_checks(_d, units);
    }

    // a quoted literal has no standard suffix, only a user-defined one,
    // which the scanner already confined to dialects that have them
    if (suffix < _d->length)
    {
        (void)d_internal_user_suffix(_d, suffix, true);
    }

    return;
}


//==============================================================================
// 5.  PUBLIC INTERFACE
//==============================================================================


/*
d_lex_target_preset
  The host preset is read from the compiler's own limits, so it is right for
the machine the decoder runs on without being configured.
*/
const struct d_lex_target*
d_lex_target_preset(
    int _which
)
{
    static const struct d_lex_target host =
    {
        (unsigned)CHAR_BIT,
        (WCHAR_MAX > 0xFFFF) ? 32u : 16u,
        (unsigned)(sizeof(long long) * CHAR_BIT)
    };
    static const struct d_lex_target unix_like = { 8u, 32u, 64u };
    static const struct d_lex_target windows   = { 8u, 16u, 64u };

    switch (_which)
    {
        case D_LEX_TARGET_HOST:
            return &host;
        case D_LEX_TARGET_UNIX:
            return &unix_like;
        case D_LEX_TARGET_WINDOWS:
            return &windows;
        default:
            return NULL;
    }
}


/*
d_lex_literal_decode
  The token's kind chooses the path; everything else is read from the
spelling, so the decoder never needs the scanner that produced the token.
*/
bool
d_lex_literal_decode(
    const struct d_lex_literal_context* _context,
    const struct d_token*               _token,
    const char*                         _spelling,
    size_t                              _length,
    struct d_lex_literal*               _out
)
{
    // parameter validation first
    if ( (!_context)          ||
         (!_context->dialect) ||
         (!_token)            ||
         (!_spelling)         ||
         (!_out)              ||
         (_length == 0u) )
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));

    struct d_internal_decode d =
    {
        _context->dialect,
        _context->target ? _context->target
                         : d_lex_target_preset(D_LEX_TARGET_HOST),
        _token, _spelling, _length, _context->sink, _out,
        ((_token->flags & (D_TOKEN_FLAG_SPLICED |
                           D_TOKEN_FLAG_TRIGRAPH)) == 0u)
    };

    switch (_token->kind)
    {
        case D_TOKEN_NUMBER:
            d_internal_number(&d);
            break;
        case D_TOKEN_CHARACTER:
        case D_TOKEN_STRING:
        case D_TOKEN_RAW_STRING:
            d_internal_quoted(&d);
            break;
        default:
            return false;
    }

    return (_out->errors == 0u);
}


/*
d_lex_literal_decode_token
  The common case -- nothing rewritten -- decodes the source bytes where they
lie and allocates nothing.
*/
bool
d_lex_literal_decode_token(
    const struct d_lexer*      _lexer,
    const struct d_lex_target* _target,
    const struct d_token*      _token,
    struct d_parse_diag_sink*  _sink,
    struct d_lex_literal*      _out
)
{
    const struct d_source* const       source  = d_lex_source_of(_lexer);
    const struct d_lex_literal_context context =
    {
        d_lex_dialect_of(_lexer),
        _target,
        _sink
    };

    // parameter validation first
    if ( (!source) ||
         (!_token) )
    {
        return false;
    }

    // a token whose bytes are its spelling needs no copy
    if ((_token->flags & (D_TOKEN_FLAG_SPLICED | D_TOKEN_FLAG_TRIGRAPH)) == 0u)
    {
        return d_lex_literal_decode(&context,
                                    _token,
                                    source->text + _token->span.offset,
                                    _token->span.length,
                                    _out);
    }

    const size_t needed   = d_token_spell(_lexer, _token, NULL, 0u);
    char* const  spelling = malloc(needed + 1u);

    // check if memory allocation was successful
    if (!spelling)
    {
        return false;
    }

    (void)d_token_spell(_lexer, _token, spelling, needed + 1u);

    const bool decoded = d_lex_literal_decode(&context,
                                              _token,
                                              spelling,
                                              needed,
                                              _out);

    free(spelling);

    return decoded;
}
