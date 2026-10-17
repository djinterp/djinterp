/*******************************************************************************
* djinterp [parse]                                                  arg_escape.c
*
* CLI11's string escapes, declared in arg_internal.h (section 5).
*   Backslash escapes resolved as CLI11's remove_escaped_characters resolves
* them, code points written as UTF-8 with CLI11's checks, and CLI11's binary
* strings. The EscapedString validator and the configuration-file reader
* share them, so a value means the same in either place.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_escape.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t
#include <string.h>  // memchr, memcmp
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// ESCAPE_CODES / ESCAPE_CHARACTERS
//   constant: CLI11's escapedCharsCode and escapedChars: the letter after a
// backslash, and the character it stands for, position for position.
static const char ESCAPE_CODES[]      = "btnfr\"\\";
static const char ESCAPE_CHARACTERS[] = "\b\t\n\f\r\"\\";

D_STATIC_ASSERT( sizeof(ESCAPE_CODES) == sizeof(ESCAPE_CHARACTERS),
                 "every escape code needs its character" );

// HEX_DIGITS
//   constant: the digits of a `\xHH` escape, lowercase, as CLI11's std::hex
// writes them.
static const char HEX_DIGITS[] = "0123456789abcdef";

/*
d_arg_internal_escape_fail
  Records CLI11's message for a malformed escape -- a prefix and the whole
text -- and returns the validation status the caller passes up.
*/
D_STATIC int
d_arg_internal_escape_fail(
    struct d_arg_buffer* _error,
    const char*          _prefix,
    struct d_pack_text   _text
)
{
    const int status = d_arg_buffer_printf(_error,
                                           "%s%.*s",
                                           _prefix,
                                           (int)_text.length,
                                           _text.data);

    return (status == D_ARG_OK) ? D_ARG_ERROR_VALIDATION : status;
}

/*
d_arg_internal_codepoint_fail
  Records one of CLI11's two code-point messages, which name no text.
*/
D_STATIC int
d_arg_internal_codepoint_fail(
    struct d_arg_buffer* _error,
    const char*          _message
)
{
    const int status = d_arg_buffer_append_cstr(_error,
                                                _message);

    return (status == D_ARG_OK) ? D_ARG_ERROR_VALIDATION : status;
}

/*
d_arg_internal_append_codepoint
  CLI11's append_codepoint: one to four bytes of UTF-8, refusing the
surrogates, which are not characters, and anything past U+10FFFF.
*/
D_STATIC int
d_arg_internal_append_codepoint(
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error,
    uint32_t             _code
)
{
    char   bytes[4] = { '\0', '\0', '\0', '\0' };
    size_t length   = 0u;

    // the surrogates are not code points
    if ( (_code >= 0xD800u) &&
         (_code <= 0xDFFFu) )
    {
        return d_arg_internal_codepoint_fail(_error,
                   "[0xD800, 0xDFFF] are not valid code points.");
    }

    // nor is anything past Unicode
    if (_code >= 0x110000u)
    {
        return d_arg_internal_codepoint_fail(_error,
                   "values above 0x10FFFF are not valid code points.");
    }

    // one byte for ASCII, then two, three or four
    if (_code < 0x80u)
    {
        bytes[length++] = (char)_code;
    }
    else if (_code < 0x800u)
    {
        bytes[length++] = (char)(unsigned char)(0xC0u | (_code >> 6));
        bytes[length++] = (char)(unsigned char)(0x80u | (_code & 0x3Fu));
    }
    else if (_code < 0x10000u)
    {
        bytes[length++] = (char)(unsigned char)(0xE0u | (_code >> 12));
        bytes[length++] = (char)(unsigned char)(0x80u | ((_code >> 6) &
                                                         0x3Fu));
        bytes[length++] = (char)(unsigned char)(0x80u | (_code & 0x3Fu));
    }
    else
    {
        bytes[length++] = (char)(unsigned char)(0xF0u | (_code >> 18));
        bytes[length++] = (char)(unsigned char)(0x80u | ((_code >> 12) &
                                                         0x3Fu));
        bytes[length++] = (char)(unsigned char)(0x80u | ((_code >> 6) &
                                                         0x3Fu));
        bytes[length++] = (char)(unsigned char)(0x80u | (_code & 0x3Fu));
    }

    return d_arg_buffer_append(_out,
                               bytes,
                               length);
}

/*
d_arg_internal_unicode_escape
  Reads the `_digits` hexadecimal digits after `\u` or `\U` at `_index` and
appends their code point; CLI11 reports a short or non-hexadecimal sequence
with one message per form.
*/
D_STATIC int
d_arg_internal_unicode_escape(
    struct d_pack_text   _text,
    size_t               _index,
    size_t               _digits,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error
)
{
    const char* const message = (_digits == 4u)
        ? "unicode sequence must have 4 hex codes "
        : "unicode sequence must have 8 hex codes ";
    uint32_t          code    = 0u;

    // the backslash, the letter and every digit must be there
    if ((_text.length - _index) < (_digits + 2u))
    {
        return d_arg_internal_escape_fail(_error,
                                          message,
                                          _text);
    }

    // each digit must be hexadecimal
    for (size_t offset = 0u; offset < _digits; ++offset)
    {
        const int digit = d_arg_internal_digit_value(
                              _text.data[_index + 2u + offset]);

        if (digit < 0)
        {
            return d_arg_internal_escape_fail(_error,
                                              message,
                                              _text);
        }

        code = (code * 16u) + (uint32_t)digit;
    }

    return d_arg_internal_append_codepoint(_out,
                                           _error,
                                           code);
}

/*
d_arg_internal_escape_code
  The character a one-letter escape stands for, written to `*_out`; false
when the letter is not one of CLI11's codes. NUL is never a code, though
memchr would find the table's terminator.
*/
D_STATIC bool
d_arg_internal_escape_code(
    char  _code,
    char* _out
)
{
    const char* const found = (_code != '\0')
                              ? (const char*)memchr(ESCAPE_CODES,
                                                    _code,
                                                    sizeof(ESCAPE_CODES) -
                                                    1u)
                              : NULL;

    // a letter outside the table is not a code
    if (!found)
    {
        return false;
    }

    *_out = ESCAPE_CHARACTERS[found - ESCAPE_CODES];

    return true;
}

/*
d_arg_internal_escape_index
  The position of a character among those CLI11 escapes, or -1; NUL is not
one of them, though memchr would find the table's terminator.
*/
D_STATIC int
d_arg_internal_escape_index(
    char _c
)
{
    const char* const found = (_c != '\0')
                              ? (const char*)memchr(ESCAPE_CHARACTERS,
                                                    _c,
                                                    sizeof(ESCAPE_CHARACTERS) -
                                                    1u)
                              : NULL;

    return (found != NULL)
           ? (int)(found - ESCAPE_CHARACTERS)
           : -1;
}

/*
d_arg_internal_printable
  C's isprint in the "C" locale, which CLI11's binary strings rely on: a
byte from the space to the tilde.
*/
D_STATIC bool
d_arg_internal_printable(
    char _c
)
{
    return ( ((unsigned char)_c >= 0x20u) &&
             ((unsigned char)_c <= 0x7Eu) );
}

/*
d_arg_internal_binary_byte
  A byte as a binary string's escape, `\x` and two lowercase hexadecimal
digits.
*/
D_STATIC void
d_arg_internal_binary_byte(
    unsigned char        _byte,
    struct d_arg_buffer* _out
)
{
    (void)d_arg_buffer_append(_out,
                              "\\x",
                              2u);
    (void)d_arg_buffer_append_char(_out,
                                   HEX_DIGITS[_byte >> 4]);
    (void)d_arg_buffer_append_char(_out,
                                   HEX_DIGITS[_byte & 0x0Fu]);

    return;
}

/*
d_arg_internal_binary_bounds
  The lengths of a binary string's opening and closing delimiters, or false
when the text is not one. The sizes keep the two delimiters from
overlapping, so `B"()"` is the shortest.
*/
D_STATIC bool
d_arg_internal_binary_bounds(
    struct d_pack_text _text,
    size_t*            _start,
    size_t*            _tail
)
{
    const char* const data   = _text.data;
    const size_t      length = _text.length;

    // B"( ... )"
    if ( (length >= 5u)                    &&
         (memcmp(data,
                 "B\"(",
                 3u) == 0)                 &&
         (memcmp(data + length - 2u,
                 ")\"",
                 2u) == 0) )
    {
        *_start = 3u;
        *_tail  = 2u;

        return true;
    }

    // 'B"( ... )"'
    if ( (length >= 7u)                    &&
         (memcmp(data,
                 "'B\"(",
                 4u) == 0)                 &&
         (memcmp(data + length - 3u,
                 ")\"'",
                 3u) == 0) )
    {
        *_start = 4u;
        *_tail  = 3u;

        return true;
    }

    return false;
}

//==============================================================================
// 5.  ESCAPES
//==============================================================================

/*
d_arg_internal_unescape_one
  The escape at `_index`, a backslash with a byte after it: a code letter,
a NUL, a Unicode escape -- delegated -- or CLI11's "unrecognized escape
sequence". `*_consumed` receives how many bytes it spans.
*/
D_STATIC int
d_arg_internal_unescape_one(
    struct d_pack_text   _text,
    size_t               _index,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error,
    size_t*              _consumed
)
{
    const char code      = _text.data[_index + 1u];
    char       character = '\0';

    *_consumed = 2u;

    // a single-letter escape
    if (d_arg_internal_escape_code(code,
                                   &character))
    {
        return d_arg_buffer_append_char(_out,
                                        character);
    }

    // a NUL
    if (code == '0')
    {
        return d_arg_buffer_append_char(_out,
                                        '\0');
    }

    // a code point in four or eight hexadecimal digits
    if ( (code == 'u') ||
         (code == 'U') )
    {
        const size_t digits = (code == 'u') ? 4u : 8u;

        *_consumed = digits + 2u;

        return d_arg_internal_unicode_escape(_text,
                                             _index,
                                             digits,
                                             _out,
                                             _error);
    }

    const int written = d_arg_buffer_printf(_error,
                                            "unrecognized escape sequence "
                                            "\\%c in %.*s",
                                            code,
                                            d_arg_internal_print_length(_text),
                                            _text.data);

    return (written == D_ARG_OK) ? D_ARG_ERROR_VALIDATION : written;
}

/*
d_arg_internal_unescape
  One pass, as remove_escaped_characters makes it: a byte other than a
backslash is copied, and a backslash with a byte after it is an escape;
one at the very end is malformed.
*/
int
d_arg_internal_unescape(
    struct d_pack_text   _text,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error
)
{
    size_t index  = 0u;
    int    status = D_ARG_OK;

    // each byte, or each escape, in turn
    while ( (index < _text.length) &&
            (status == D_ARG_OK) )
    {
        size_t consumed = 1u;

        // an ordinary byte is itself
        if (_text.data[index] != '\\')
        {
            status = d_arg_buffer_append_char(_out,
                                              _text.data[index]);
        }
        // a backslash needs a byte after it
        else if ((index + 1u) >= _text.length)
        {
            return d_arg_internal_escape_fail(_error,
                                              "invalid escape sequence ",
                                              _text);
        }
        else
        {
            status = d_arg_internal_unescape_one(_text,
                                                 index,
                                                 _out,
                                                 _error,
                                                 &consumed);
        }

        index += consumed;
    }

    return status;
}

/*
d_arg_internal_escapable
  has_escapable_character: any byte of CLI11's escape table.
*/
bool
d_arg_internal_escapable(
    struct d_pack_text _text
)
{
    // each byte in turn
    for (size_t index = 0u; index < _text.length; ++index)
    {
        if (d_arg_internal_escape_index(_text.data[index]) >= 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_escape
  add_escaped_characters: each byte of the escape table as a backslash and
its code letter, every other byte as it is.
*/
int
d_arg_internal_escape(
    struct d_pack_text   _text,
    struct d_arg_buffer* _out
)
{
    // each byte in turn
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const int at = d_arg_internal_escape_index(_text.data[index]);

        if (at >= 0)
        {
            (void)d_arg_buffer_append_char(_out,
                                           '\\');
            (void)d_arg_buffer_append_char(_out,
                                           ESCAPE_CODES[at]);
        }
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           _text.data[index]);
        }
    }

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/*
d_arg_internal_is_binary_string
  d_arg_internal_binary_bounds, without the bounds.
*/
bool
d_arg_internal_is_binary_string(
    struct d_pack_text _text
)
{
    size_t start = 0u;
    size_t tail  = 0u;

    return ( (_text.data != NULL) &&
             (d_arg_internal_binary_bounds(_text,
                                           &start,
                                           &tail)) );
}

/*
d_arg_internal_extract_binary
  extract_binary_string: between the delimiters, `\x` or `\X` and two
hexadecimal digits are a byte, and every other byte -- a backslash not
followed so included -- is itself.
*/
int
d_arg_internal_extract_binary(
    struct d_pack_text   _text,
    struct d_arg_buffer* _out
)
{
    size_t start = 0u;
    size_t tail  = 0u;

    // anything but a binary string is kept as it is
    if ( (!_text.data) ||
         (!d_arg_internal_binary_bounds(_text,
                                        &start,
                                        &tail)) )
    {
        return d_arg_buffer_append_text(_out,
                                        _text);
    }

    const size_t end    = _text.length - tail;
    size_t       index  = start;
    int          status = D_ARG_OK;

    // each byte, or each hexadecimal escape, in turn
    while ( (index < end) &&
            (status == D_ARG_OK) )
    {
        const char* const at   = _text.data + index;
        const int         high = ((index + 3u) < end)
                                 ? d_arg_internal_digit_value(at[2])
                                 : -1;
        const int         low  = ((index + 3u) < end)
                                 ? d_arg_internal_digit_value(at[3])
                                 : -1;

        // \xHH is one byte
        if ( (at[0] == '\\')    &&
             ( (at[1] == 'x') ||
               (at[1] == 'X') ) &&
             (high >= 0)        &&
             (low >= 0) )
        {
            status = d_arg_buffer_append_char(_out,
                                              (char)(unsigned char)
                                              ((high * 16) + low));
            index += 4u;

            continue;
        }

        status = d_arg_buffer_append_char(_out,
                                          at[0]);
        index++;
    }

    return status;
}

/*
d_arg_internal_binary_escaped
  The loop of CLI11's binary_escape_string: a byte that does not print as
`\xHH`, an `x` or `X` after a backslash as its own escape, any other byte as
it is.
*/
D_STATIC void
d_arg_internal_binary_escaped(
    struct d_pack_text   _text,
    struct d_arg_buffer* _out
)
{
    // each byte in turn
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const char c     = _text.data[index];
        const bool after = ( (_out->length > 0u) &&
                             (_out->data[_out->length - 1u] == '\\') );

        if (!d_arg_internal_printable(c))
        {
            d_arg_internal_binary_byte((unsigned char)c,
                                       _out);
        }
        else if ( ( (c == 'x') ||
                    (c == 'X') ) &&
                  (after) )
        {
            d_arg_internal_binary_byte((unsigned char)c,
                                       _out);
        }
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           c);
        }
    }

    return;
}

/*
d_arg_internal_binary_escape
  binary_escape_string: the bytes escaped as needed; then, when that changed
the text or wrapping is forced, each single quote as `\x27` and the whole
between `'B"(` and `)"'`.
*/
int
d_arg_internal_binary_escape(
    struct d_pack_text   _text,
    bool                 _force,
    struct d_arg_buffer* _out
)
{
    struct d_arg_buffer escaped = D_ARG_BUFFER_INIT;

    d_arg_internal_binary_escaped(_text,
                                  &escaped);

    const bool changed = ( (escaped.length != _text.length) ||
                           ( (escaped.length > 0u) &&
                             (memcmp(escaped.data,
                                     _text.data,
                                     _text.length) != 0) ) );

    // unchanged and not forced: the text as it is
    if ( (!changed) &&
         (!_force) )
    {
        (void)d_arg_buffer_append_text(_out,
                                       d_arg_buffer_text(&escaped));
    }
    else
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "'B\"(");

        // each single quote as its escape
        for (size_t index = 0u; index < escaped.length; ++index)
        {
            if (escaped.data[index] == '\'')
            {
                (void)d_arg_buffer_append_cstr(_out,
                                               "\\x27");
            }
            else
            {
                (void)d_arg_buffer_append_char(_out,
                                               escaped.data[index]);
            }
        }

        (void)d_arg_buffer_append_cstr(_out,
                                       ")\"'");
    }

    const int status = ( (escaped.failed) ||
                         (_out->failed) )
                       ? D_ARG_ERROR_OUT_OF_MEMORY
                       : D_ARG_OK;

    d_arg_buffer_release(&escaped);

    return status;
}
