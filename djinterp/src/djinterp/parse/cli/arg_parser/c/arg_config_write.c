/*******************************************************************************
* djinterp [parse]                                            arg_config_write.c
*
* Writing configuration text, declared in arg_config.h (5.2.1).
*   CLI11's ConfigBase::to_config, a C function for each of its helpers and
* steps. A command's own configurable options are written group by group --
* OPTIONS first, with the options of the empty group, then each other group
* in the order its options first name it -- each with the values its policy
* keeps, or, when defaults are asked for and it has none, its default; the
* values joined and quoted as convert_arg_for_ini and ini_join write them, a
* flag's as one of its names reads them back, a name quoted when it needs
* it, a description as a comment. The nameless subcommands follow, option
* groups among them, each written in turn under the command's prefix, then
* the named ones: a section of their own when configurable and used -- or
* when every default is asked for -- else under a dotted prefix. With
* descriptions, the command's own heads whatever it wrote.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_config_write.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr, strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type
#include "./arg_internal.h"  // spans, escapes, reduced values, watches


#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 1.  VALUES
//==============================================================================

// LONG_VALUE
//   constant: the length past which CLI11 writes a value holding escapable
// characters as a `'''` literal over lines, outside an array.
static const size_t LONG_VALUE = 100u;

/*
d_arg_internal_ini_printable
  C's isprint in the "C" locale, as CLI11 calls it: a byte from the space to
the tilde.
*/
D_STATIC bool
d_arg_internal_ini_printable(
    char _c
)
{
    return ( ((unsigned char)_c >= 0x20u) &&
             ((unsigned char)_c <= 0x7Eu) );
}

/*
d_arg_internal_ini_word
  Whether a value is one of the words CLI11 writes bare: `true`, `false`,
`nan` and `inf`.
*/
D_STATIC bool
d_arg_internal_ini_word(
    struct d_pack_text _value
)
{
    return ( (d_arg_internal_equal_cstr(_value,
                                        "true"))  ||
             (d_arg_internal_equal_cstr(_value,
                                        "false")) ||
             (d_arg_internal_equal_cstr(_value,
                                        "nan"))   ||
             (d_arg_internal_equal_cstr(_value,
                                        "inf")) );
}

/*
d_arg_internal_ini_digits
  How many decimal digits run from `_at`.
*/
D_STATIC size_t
d_arg_internal_ini_digits(
    struct d_pack_text _value,
    size_t             _at
)
{
    size_t count = 0u;

    // each digit in turn
    while ( ((_at + count) < _value.length) &&
            (d_arg_internal_is_digit(_value.data[_at + count])) )
    {
        count++;
    }

    return count;
}

/*
d_arg_internal_ini_signed
  Whether a byte of a number is its sign.
*/
D_STATIC bool
d_arg_internal_ini_signed(
    struct d_pack_text _value,
    size_t             _at
)
{
    return ( (_at < _value.length)       &&
             ( (_value.data[_at] == '+') ||
               (_value.data[_at] == '-') ) );
}

/*
d_arg_internal_ini_exponent
  How much of the value an exponent takes from `_at`: `e` or `E`, a sign,
and at least one digit -- none at all when they are not there, as strtold
then stops before the `e`.
*/
D_STATIC size_t
d_arg_internal_ini_exponent(
    struct d_pack_text _value,
    size_t             _at
)
{
    size_t sign   = 0u;
    size_t digits = 0u;

    // not an exponent
    if ( (_at >= _value.length)        ||
         ( (_value.data[_at] != 'e') &&
           (_value.data[_at] != 'E') ) )
    {
        return 0u;
    }

    sign   = (d_arg_internal_ini_signed(_value,
                                        _at + 1u)) ? 1u : 0u;
    digits = d_arg_internal_ini_digits(_value,
                                       _at + 1u + sign);

    return (digits > 0u)
           ? (1u + sign + digits)
           : 0u;
}

/*
d_arg_internal_ini_number
  Whether CLI11 writes a value bare as a number: one strtold reads whole that
holds only digits, `.`, `+`, `-`, `e` and `E`, so a decimal floating-point
literal -- an optional sign, digits with an optional point and at least one
digit, and an optional exponent. Such a value never starts with `0x`, which
CLI11 checks first.
*/
D_STATIC bool
d_arg_internal_ini_number(
    struct d_pack_text _value
)
{
    size_t at       = (d_arg_internal_ini_signed(_value,
                                                 0u)) ? 1u : 0u;
    size_t mantissa = d_arg_internal_ini_digits(_value,
                                                at);

    at += mantissa;

    // a fraction
    if ( (at < _value.length) &&
         (_value.data[at] == '.') )
    {
        const size_t fraction = d_arg_internal_ini_digits(_value,
                                                          at + 1u);

        mantissa += fraction;
        at       += 1u + fraction;
    }

    return ( (mantissa > 0u) &&
             ((at + d_arg_internal_ini_exponent(_value,
                                                at)) == _value.length) );
}

/*
d_arg_internal_ini_radix_digit
  Whether a byte is a digit of the base a prefix letter names: `x`
hexadecimal, `o` octal, `b` binary.
*/
D_STATIC bool
d_arg_internal_ini_radix_digit(
    char _base,
    char _c
)
{
    // the base's digits
    if (_base == 'x')
    {
        return (d_arg_internal_digit_value(_c) >= 0);
    }

    return (_base == 'o')
           ? ( (_c >= '0') &&
               (_c <= '7') )
           : ( (_base == 'b') &&
               ( (_c == '0') ||
                 (_c == '1') ) );
}

/*
d_arg_internal_ini_radix
  Whether CLI11 writes a value bare as a hexadecimal, octal or binary
number: `0x`, `0o` or `0b`, then nothing but that base's digits, if
anything.
*/
D_STATIC bool
d_arg_internal_ini_radix(
    struct d_pack_text _value
)
{
    // `0` and a base letter
    if ( (_value.length < 2u)       ||
         (_value.data[0] != '0')    ||
         ( (_value.data[1] != 'x') &&
           (_value.data[1] != 'o') &&
           (_value.data[1] != 'b') ) )
    {
        return false;
    }

    // the base's digits after them
    for (size_t at = 2u; at < _value.length; ++at)
    {
        if (!d_arg_internal_ini_radix_digit(_value.data[1],
                                            _value.data[at]))
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_ini_prints
  CLI11's is_printable: every byte prints, or is a line feed or a tab.
*/
D_STATIC bool
d_arg_internal_ini_prints(
    struct d_pack_text _value
)
{
    // each byte in turn
    for (size_t at = 0u; at < _value.length; ++at)
    {
        const char c = _value.data[at];

        if ( (!d_arg_internal_ini_printable(c)) &&
             (c != '\n')                         &&
             (c != '\t') )
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_ini_triple
  Whether a value holds `'''`, which a multiline literal could not hold.
*/
D_STATIC bool
d_arg_internal_ini_triple(
    struct d_pack_text _value
)
{
    // each place three bytes start
    for (size_t at = 0u; (at + 3u) <= _value.length; ++at)
    {
        if ( (_value.data[at] == '\'')       &&
             (_value.data[at + 1u] == '\'')  &&
             (_value.data[at + 2u] == '\'') )
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_ini_quoted
  A value between two of a quote character, with its escapes added or not.
*/
D_STATIC int
d_arg_internal_ini_quoted(
    struct d_pack_text   _value,
    char                 _quote,
    bool                 _escaped,
    struct d_arg_buffer* _out
)
{
    (void)d_arg_buffer_append_char(_out,
                                   _quote);

    // the value, escaped as asked
    if (_escaped)
    {
        (void)d_arg_internal_escape(_value,
                                    _out);
    }
    else
    {
        (void)d_arg_buffer_append_text(_out,
                                       _value);
    }

    return d_arg_buffer_append_char(_out,
                                    _quote);
}

/*
d_arg_internal_ini_character
  A value of one byte as CLI11 writes it: one that does not print as a
binary string; a single quote between string quotes; any other between
literal quotes.
*/
D_STATIC int
d_arg_internal_ini_character(
    struct d_pack_text                _value,
    const struct d_arg_config_format* _format,
    struct d_arg_buffer*              _out
)
{
    // a byte that does not print
    if (!d_arg_internal_ini_printable(_value.data[0]))
    {
        return d_arg_internal_binary_escape(_value,
                                            false,
                                            _out);
    }

    return d_arg_internal_ini_quoted(_value,
                                     (_value.data[0] == '\'')
                                     ? _format->string_quote
                                     : _format->literal_quote,
                                     false,
                                     _out);
}

/*
d_arg_internal_ini_escaped
  A value holding characters CLI11 escapes: a long one outside an array as a
`'''` literal over lines -- a newline at either end doubled, so that the
reader's dropping it keeps one -- or, holding `'''` itself, as a binary
string; any other between string quotes with its escapes.
*/
D_STATIC int
d_arg_internal_ini_escaped(
    struct d_pack_text                _value,
    const struct d_arg_config_format* _format,
    bool                              _single_line,
    struct d_arg_buffer*              _out
)
{
    // short, or inside an array: a string with escapes
    if ( (_value.length <= LONG_VALUE) ||
         (_single_line) )
    {
        return d_arg_internal_ini_quoted(_value,
                                         _format->string_quote,
                                         true,
                                         _out);
    }

    // holding `'''`: a binary string, wrapped whatever it holds
    if (d_arg_internal_ini_triple(_value))
    {
        return d_arg_internal_binary_escape(_value,
                                            true,
                                            _out);
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   (_value.data[0] == '\n') ? "'''\n" : "'''");
    (void)d_arg_buffer_append_text(_out,
                                   _value);

    return d_arg_buffer_append_cstr(_out,
                                    (_value.data[_value.length - 1u] == '\n')
                                    ? "\n'''"
                                    : "'''");
}

/*
d_arg_internal_ini_value
  CLI11's convert_arg_for_ini: an empty value as two string quotes; a word
or number TOML reads as itself bare; a single byte as
d_arg_internal_ini_character writes it; a hexadecimal, octal or binary
number bare; bytes that do not print as a binary string; escapable
characters as d_arg_internal_ini_escaped writes them; any other value
between string quotes.
*/
D_STATIC int
d_arg_internal_ini_value(
    struct d_pack_text                _value,
    const struct d_arg_config_format* _format,
    bool                              _single_line,
    struct d_arg_buffer*              _out
)
{
    // empty
    if (_value.length == 0u)
    {
        (void)d_arg_buffer_append_char(_out,
                                       _format->string_quote);

        return d_arg_buffer_append_char(_out,
                                        _format->string_quote);
    }

    // read back as itself, bare
    if ( (d_arg_internal_ini_word(_value))   ||
         (d_arg_internal_ini_number(_value)) )
    {
        return d_arg_buffer_append_text(_out,
                                        _value);
    }

    // a single byte
    if (_value.length == 1u)
    {
        return d_arg_internal_ini_character(_value,
                                            _format,
                                            _out);
    }

    // a number in another base, bare; or bytes that do not print
    if (d_arg_internal_ini_radix(_value))
    {
        return d_arg_buffer_append_text(_out,
                                        _value);
    }

    if (!d_arg_internal_ini_prints(_value))
    {
        return d_arg_internal_binary_escape(_value,
                                            false,
                                            _out);
    }

    return (d_arg_internal_escapable(_value))
           ? d_arg_internal_ini_escaped(_value,
                                        _format,
                                        _single_line,
                                        _out)
           : d_arg_internal_ini_quoted(_value,
                                       _format->string_quote,
                                       false,
                                       _out);
}

/*
d_arg_internal_ini_join
  CLI11's ini_join: the values as d_arg_internal_ini_value writes them --
several between the array's brackets, where the format has them, each then
on one line -- separated by the array separator and, unless it is
whitespace, a space.
*/
D_STATIC int
d_arg_internal_ini_join(
    const struct d_arg_text_list*     _values,
    const struct d_arg_config_format* _format,
    struct d_arg_buffer*              _out
)
{
    const bool several = (_values->count > 1u);
    const bool array   = ( (several) &&
                           (_format->array_start != '\0') );
    int        status  = D_ARG_OK;

    // the array's opening
    if (array)
    {
        (void)d_arg_buffer_append_char(_out,
                                       _format->array_start);
    }

    // each value, after a separator
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _values->count);
         ++index)
    {
        if (index > 0u)
        {
            (void)d_arg_buffer_append_char(_out,
                                           _format->array_separator);
            (void)d_arg_buffer_append_cstr(
                      _out,
                      (d_arg_internal_is_space(_format->array_separator))
                      ? ""
                      : " ");
        }

        status = d_arg_internal_ini_value(_values->items[index],
                                          _format,
                                          array,
                                          _out);
    }

    // the array's closing
    if ( (several) &&
         (_format->array_end != '\0') )
    {
        (void)d_arg_buffer_append_char(_out,
                                       _format->array_end);
    }

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

//==============================================================================
// 2.  NAMES AND COMMENTS
//==============================================================================

// d_arg_internal_ini_writer
//   struct: one command's part of CLI11's to_config: the format and the
// command, the output mode, whether descriptions are written, the prefix
// each name takes, CLI11's key characters -- those that make a name need
// quotes -- and the text written so far.
struct d_arg_internal_ini_writer
{
    const struct d_arg_config_format* format;
    const struct d_arg_command*       command;
    int                               mode;
    bool                              describe;
    struct d_pack_text                prefix;
    char                              keys[10];
    struct d_arg_buffer*              out;
};

/*
d_arg_internal_ini_has_any
  find_first_of over a set of bytes, NUL among them when one is: whether the
text holds any of them.
*/
D_STATIC bool
d_arg_internal_ini_has_any(
    struct d_pack_text _text,
    const char*        _set,
    size_t             _count
)
{
    // each byte against each of the set
    for (size_t at = 0u; at < _text.length; ++at)
    {
        if (memchr(_set,
                   _text.data[at],
                   _count) != NULL)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_ini_clean
  CLI11's clean_name_string, the name appended: one holding a key
character, a quote or a backslash, or in brackets, between single quotes --
or, holding a single quote, between double quotes, escaped when it needs
it; any other as it is.
*/
D_STATIC int
d_arg_internal_ini_clean(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_pack_text                      _name,
    struct d_arg_buffer*                    _out
)
{
    const bool bracketed = ( (_name.length > 0u)                     &&
                             (_name.data[0] == '[')                  &&
                             (_name.data[_name.length - 1u] == ']') );

    // a name that reads as it is
    if ( (!d_arg_internal_ini_has_any(_name,
                                      _writer->keys,
                                      sizeof _writer->keys))  &&
         (!bracketed)                                          &&
         (!d_arg_internal_ini_has_any(_name,
                                      "'`\"\\",
                                      4u)) )
    {
        return d_arg_buffer_append_text(_out,
                                        _name);
    }

    return (memchr(_name.data,
                   '\'',
                   _name.length) == NULL)
           ? d_arg_internal_ini_quoted(_name,
                                       '\'',
                                       false,
                                       _out)
           : d_arg_internal_ini_quoted(_name,
                                       '"',
                                       d_arg_internal_escapable(_name),
                                       _out);
}

/*
d_arg_internal_ini_newlines
  CLI11's fix_newlines: text with a leader written after each carriage
return and each line feed in it.
*/
D_STATIC int
d_arg_internal_ini_newlines(
    struct d_pack_text   _leader,
    struct d_pack_text   _text,
    struct d_arg_buffer* _out
)
{
    // each byte, a line's end followed by the leader
    for (size_t at = 0u; at < _text.length; ++at)
    {
        (void)d_arg_buffer_append_char(_out,
                                       _text.data[at]);

        if ( (_text.data[at] == '\r') ||
             (_text.data[at] == '\n') )
        {
            (void)d_arg_buffer_append_text(_out,
                                           _leader);
        }
    }

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/*
d_arg_internal_ini_comment
  A description as a comment: `_lead` -- the comment character and a
space, the comment character doubled for a command's own -- then the
text, its lines each led the same way, and a line feed.
*/
D_STATIC int
d_arg_internal_ini_comment(
    struct d_pack_text   _lead,
    const char*          _description,
    struct d_arg_buffer* _out
)
{
    (void)d_arg_buffer_append_text(_out,
                                   _lead);
    (void)d_arg_internal_ini_newlines(_lead,
                                      d_arg_internal_text_of(_description),
                                      _out);

    return d_arg_buffer_append_char(_out,
                                    '\n');
}

/*
d_arg_internal_ini_header
  A group's header: a blank line, then, as a comment, the group's name and
" Options".
*/
D_STATIC int
d_arg_internal_ini_header(
    const struct d_arg_internal_ini_writer* _writer,
    const char*                             _group
)
{
    const char comment = _writer->format->comment;

    (void)d_arg_buffer_append_char(_writer->out,
                                   '\n');
    (void)d_arg_buffer_append_char(_writer->out,
                                   comment);
    (void)d_arg_buffer_append_char(_writer->out,
                                   comment);
    (void)d_arg_buffer_append_char(_writer->out,
                                   ' ');
    (void)d_arg_buffer_append_cstr(_writer->out,
                                   _group);

    return d_arg_buffer_append_cstr(_writer->out,
                                    " Options\n");
}

//==============================================================================
// 3.  AN OPTION'S VALUE
//==============================================================================

/*
d_arg_internal_ini_results_convert
  CLI11's conversion of an option's values into a vector of strings, as its
results(T&) makes it for to_config: none, or `{}` alone, is no value; `{}`
and a separator is `{}` alone; any others are themselves, viewed.
*/
D_STATIC int
d_arg_internal_ini_results_convert(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    struct d_arg_text_list* const list   = (struct d_arg_text_list*)_target;
    const bool                    braces = ( (_count > 0u) &&
                                             (d_arg_internal_equal_cstr(
                                                  _values[0],
                                                  "{}")) );
    size_t                        keep   = _count;
    int                           status = D_ARG_OK;

    (void)_type;
    (void)_context;
    list->count = 0u;

    // `{}` alone is no value; `{}` and a separator is `{}`
    if ( (braces) &&
         (_count <= 2u) )
    {
        keep = ( (_count == 2u)                          &&
                 ( (_values[1].length == 0u) ||
                   (d_arg_internal_equal_cstr(_values[1],
                                              "%%")) ) )
               ? 1u
               : ((_count == 1u) ? 0u : _count);
    }

    // the values kept
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < keep);
         ++index)
    {
        status = d_arg_internal_text_push(list,
                                          _values[index]);
    }

    return status;
}

// RESULTS_TYPE
//   constant: a vector of strings as a type an option's values convert to,
// CLI11's results_t, which to_config reads a default's values into.
static const struct d_arg_type RESULTS_TYPE =
{
    "TEXT",                                  // name
    d_arg_internal_ini_results_convert,      // convert
    NULL,                                    // format
    NULL,                                    // data
    sizeof(struct d_arg_text_list),          // size
    1,                                       // type_size_min
    1,                                       // type_size_max
    D_ARG_EXPECTED_UNLIMITED,                // expected
    D_ARG_TYPE_FLAG_STRING |
    D_ARG_TYPE_FLAG_CONTAINER                // flags
};

/*
d_arg_internal_ini_given
  The option's values as given, CLI11's results(), in place of those in the
list.
*/
D_STATIC int
d_arg_internal_ini_given(
    const struct d_arg_option* _option,
    struct d_arg_text_list*    _values
)
{
    _values->count = 0u;

    return d_arg_internal_text_append(_values,
                                      &_option->results,
                                      0u,
                                      _option->results.count);
}

/*
d_arg_internal_ini_sum
  CLI11's check of a summed value: validated as the first value; refused,
the values as given are written instead, and passed, what the validators
made of it.
*/
D_STATIC int
d_arg_internal_ini_sum(
    const struct d_arg_option* _option,
    struct d_arg_text_list*    _values,
    struct d_arg_buffer*       _scratch
)
{
    struct d_arg_buffer message = D_ARG_BUFFER_INIT;
    int                 status  = d_arg_internal_option_judge(
                                      _option,
                                      _values->items[0],
                                      0,
                                      &message,
                                      _scratch);

    // refused: the values as given
    if ( (status == D_ARG_OK) &&
         (message.length > 0u) )
    {
        status = d_arg_internal_ini_given(_option,
                                          _values);
    }
    else if (status == D_ARG_OK)
    {
        _values->items[0] = d_arg_buffer_text(_scratch);
    }

    d_arg_buffer_release(&message);

    return status;
}

/*
d_arg_internal_ini_joined
  CLI11's check of a joined value: without a delimiter, or when the delimiter
cannot split the value back -- it ends the value, is doubled in it, or is
no rarer than the values given -- the values as given are written instead.
*/
D_STATIC int
d_arg_internal_ini_joined(
    const struct d_arg_option* _option,
    struct d_arg_text_list*    _values
)
{
    const char delimiter = _option->delimiter;
    size_t     count     = 0u;
    bool       doubled   = false;

    // no delimiter, or no value to check
    if ( (delimiter == '\0') ||
         (_values->count == 0u) )
    {
        return (delimiter == '\0')
               ? d_arg_internal_ini_given(_option,
                                          _values)
               : D_ARG_OK;
    }

    const struct d_pack_text joined = _values->items[0];

    // the delimiters, and whether two meet
    for (size_t at = 0u; at < joined.length; ++at)
    {
        count   += (joined.data[at] == delimiter) ? 1u : 0u;
        doubled  = ( (doubled) ||
                     ( (at > 0u)                           &&
                       (joined.data[at] == delimiter)      &&
                       (joined.data[at - 1u] == delimiter) ) );
    }

    return ( ( (joined.length > 0u) &&
               (joined.data[joined.length - 1u] == delimiter) ) ||
             (d_arg_option_count(_option) <= count)              ||
             (doubled) )
           ? d_arg_internal_ini_given(_option,
                                      _values)
           : D_ARG_OK;
}

/*
d_arg_internal_ini_results
  The values CLI11's to_config writes for an option: those its policy keeps,
reduced on a copy as reduced_results reduces them; reversed under Reverse;
under Sum and Join, the values as given where the kept one would not read
back.
*/
D_STATIC int
d_arg_internal_ini_results(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _values,
    struct d_arg_buffer*    _scratch
)
{
    const int    policy = _option->multi_option_policy;
    const size_t given  = d_arg_option_count(_option);
    int          status = d_arg_internal_option_reduced(_option,
                                                        _values);

    // the policies whose kept values may not read back as they are
    if ( (status == D_ARG_OK)                         &&
         (policy == (int)D_ARG_MULTI_REVERSE)         &&
         (_values->count > 1u) )
    {
        // reversed in place
        for (size_t low = 0u; low < (_values->count / 2u); ++low)
        {
            const size_t             high = _values->count - 1u - low;
            const struct d_pack_text kept = _values->items[low];

            _values->items[low]  = _values->items[high];
            _values->items[high] = kept;
        }
    }
    else if ( (status == D_ARG_OK)              &&
              (policy == (int)D_ARG_MULTI_SUM)  &&
              (given >= 1u)                     &&
              (_values->count == 1u) )
    {
        status = d_arg_internal_ini_sum(_option,
                                        _values,
                                        _scratch);
    }
    else if ( (status == D_ARG_OK)               &&
              (policy == (int)D_ARG_MULTI_JOIN)  &&
              (given > 1u) )
    {
        status = d_arg_internal_ini_joined(_option,
                                           _values);
    }

    return status;
}

/*
d_arg_internal_ini_given_value
  The value CLI11 writes for what an option was given: `{}` given once with
its separator as a quoted `{}`, so that it reads back as itself; any other
values joined.
*/
D_STATIC int
d_arg_internal_ini_given_value(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_arg_option*                    _option,
    struct d_arg_buffer*                    _value
)
{
    struct d_arg_text_list values  = D_ARG_LIST_INIT;
    struct d_arg_buffer    scratch = D_ARG_BUFFER_INIT;
    int                    status  = d_arg_internal_ini_results(_option,
                                                                &values,
                                                                &scratch);

    // `{}` with its separator, or the values joined
    if ( (status == D_ARG_OK)                                &&
         (d_arg_option_count(_option) == 1u)                 &&
         (values.count == 2u)                                &&
         (d_arg_internal_equal_cstr(values.items[0],
                                    "{}"))                   &&
         (d_arg_internal_equal_cstr(values.items[1],
                                    "%%")) )
    {
        status = d_arg_buffer_append_cstr(_value,
                                          "\"{}\"");
    }
    else if (status == D_ARG_OK)
    {
        status = d_arg_internal_ini_join(&values,
                                         _writer->format,
                                         _value);
    }

    d_arg_text_list_release(&values);
    d_arg_buffer_release(&scratch);

    return status;
}

/*
d_arg_internal_ini_default
  What CLI11 writes for an option without values when defaults are asked
for: its default text's values, as results(T&) reads them; for an option
taking no values, `false`; for one not required, or whose callback runs for
its default, `""`; else `"<REQUIRED>"`.
*/
D_STATIC int
d_arg_internal_ini_default(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_arg_option*                    _option,
    struct d_arg_buffer*                    _value
)
{
    // the default text's values
    if (d_arg_option_default_text(_option)[0] != '\0')
    {
        struct d_arg_text_list values = D_ARG_LIST_INIT;
        int                    status = d_arg_option_as(_option,
                                                        &RESULTS_TYPE,
                                                        &values);

        status = (status == D_ARG_OK)
                 ? d_arg_internal_ini_join(&values,
                                           _writer->format,
                                           _value)
                 : status;
        d_arg_text_list_release(&values);

        return status;
    }

    // a word for the option's kind
    if (_option->expected_min == 0)
    {
        return d_arg_buffer_append_cstr(_value,
                                        "false");
    }

    return d_arg_buffer_append_cstr(
               _value,
               ( (d_arg_option_has_flag(
                      _option,
                      D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT)) ||
                 (!d_arg_option_has_flag(_option,
                                         D_ARG_OPTION_REQUIRED)) )
               ? "\"\""
               : "\"<REQUIRED>\"");
}

//==============================================================================
// 4.  AN OPTION'S LINE
//==============================================================================

/*
d_arg_internal_ini_flag_as
  The value as a flag name reads it, CLI11's get_flag_value, replacing the
value when the name accepts it; a refusal -- the ArgumentMismatch CLI11
catches -- leaves the value and the parser's report as they were.
*/
D_STATIC int
d_arg_internal_ini_flag_as(
    struct d_arg_option* _option,
    struct d_pack_text   _name,
    struct d_arg_buffer* _value,
    bool*                _accepted
)
{
    struct d_arg_internal_watch watch  = { NULL,
                                           D_ARG_ERROR_INIT };
    struct d_arg_buffer         read   = D_ARG_BUFFER_INIT;
    int                         status = D_ARG_OK;

    d_arg_internal_watch_begin(&watch,
                               d_arg_internal_parser_of(_option));
    status = d_arg_option_flag_value(_option,
                                     _name,
                                     d_arg_buffer_text(_value),
                                     &read);
    (void)d_arg_internal_watch_end(&watch,
                                   (status == D_ARG_ERROR_ARGUMENT_MISMATCH)
                                   ? D_ARG_OK
                                   : status);
    *_accepted = (status == D_ARG_OK);

    // the name's reading replaces the value
    if (status == D_ARG_OK)
    {
        d_arg_buffer_clear(_value);
        status = d_arg_buffer_append_text(_value,
                                          d_arg_buffer_text(&read));
    }

    d_arg_buffer_release(&read);

    return (status == D_ARG_ERROR_ARGUMENT_MISMATCH)
           ? D_ARG_OK
           : status;
}

/*
d_arg_internal_ini_flag_names
  CLI11's second look at a flag's value, once its single name refused it:
every flag name in turn -- none stopping the others -- each reading the
value as the one before left it, the last that accepted it naming the line.
*/
D_STATIC int
d_arg_internal_ini_flag_names(
    struct d_arg_option* _option,
    struct d_pack_text*  _name,
    struct d_arg_buffer* _value,
    bool*                _valid
)
{
    int status = D_ARG_OK;

    // every flag name in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->flag_names.count);
         ++index)
    {
        const struct d_pack_text name = d_arg_internal_text_of(
                                            _option->flag_names.items[index]);
        bool                     took = false;

        status = d_arg_internal_ini_flag_as(_option,
                                            name,
                                            _value,
                                            &took);

        // the last name that accepted
        if (took)
        {
            *_name  = name;
            *_valid = true;
        }
    }

    return status;
}

/*
d_arg_internal_ini_flag
  CLI11's reading of a written flag's value by its names: by the single name;
refused, by every flag name as d_arg_internal_ini_flag_names reads it;
refused by all, the values as given.
*/
D_STATIC int
d_arg_internal_ini_flag(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_arg_option*                    _option,
    struct d_pack_text*                     _name,
    struct d_arg_buffer*                    _value
)
{
    bool valid  = false;
    int  status = d_arg_internal_ini_flag_as(_option,
                                             *_name,
                                             _value,
                                             &valid);

    // refused by the single name: every flag name
    if ( (status == D_ARG_OK) &&
         (!valid) )
    {
        status = d_arg_internal_ini_flag_names(_option,
                                               _name,
                                               _value,
                                               &valid);
    }

    // refused by every name: the values as given
    if ( (status == D_ARG_OK) &&
         (!valid) )
    {
        d_arg_buffer_clear(_value);
        status = d_arg_internal_ini_join(&_option->results,
                                         _writer->format,
                                         _value);
    }

    return status;
}

/*
d_arg_internal_ini_line
  An option's line: a flag's value as its names read it back; the
description first, as a comment, when asked for -- after a blank line,
unless it is the first text written; then the name, prefixed and cleaned --
commented out, a default, when defaults are -- the value delimiter, the
value and a line feed.
*/
D_STATIC int
d_arg_internal_ini_line(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_arg_option*                    _option,
    struct d_arg_buffer*                    _value,
    bool                                    _default
)
{
    const char               lead[2] = { _writer->format->comment, ' ' };
    struct d_arg_buffer*     out     = _writer->out;
    struct d_pack_text       name    = d_arg_internal_text_of(
                                           d_arg_option_single_name(_option));
    int                      status  = (_option->flag_names.count > 0u)
                                       ? d_arg_internal_ini_flag(_writer,
                                                                 _option,
                                                                 &name,
                                                                 _value)
                                       : D_ARG_OK;

    // the description, as a comment
    if ( (status == D_ARG_OK)                                &&
         (_writer->describe)                                 &&
         (d_arg_option_description(_option)[0] != '\0') )
    {
        (void)d_arg_buffer_append_cstr(out,
                                       (out->length > 0u) ? "\n" : "");
        status = d_arg_internal_ini_comment(d_arg_internal_span(lead,
                                                                2u),
                                            d_arg_option_description(_option),
                                            out);
    }

    // a default commented out
    if ( (_default) &&
         ((_writer->format->flags & D_ARG_CONFIG_COMMENT_DEFAULTS) != 0u) )
    {
        (void)d_arg_buffer_append_char(out,
                                       _writer->format->comment);
    }

    (void)d_arg_buffer_append_text(out,
                                   _writer->prefix);
    (void)d_arg_internal_ini_clean(_writer,
                                   name,
                                   out);
    (void)d_arg_buffer_append_char(out,
                                   _writer->format->value_delimiter);
    (void)d_arg_buffer_append_text(out,
                                   d_arg_buffer_text(_value));
    (void)d_arg_buffer_append_char(out,
                                   '\n');

    return ( (status == D_ARG_OK) &&
             (out->failed) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

/*
d_arg_internal_ini_option
  One option of a group's pass: its value -- what it was given, or, when
defaults are asked for and that is nothing, its default -- and, unless the
value is still empty, its line.
*/
D_STATIC int
d_arg_internal_ini_option(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_arg_option*                    _option
)
{
    struct d_arg_buffer value      = D_ARG_BUFFER_INIT;
    bool                is_default = false;
    int                 status     = d_arg_internal_ini_given_value(_writer,
                                                                    _option,
                                                                    &value);

    // nothing given: the default, when asked for
    if ( (status == D_ARG_OK)                                &&
         (value.length == 0u)                                &&
         (_writer->mode != (int)D_ARG_CONFIG_OUTPUT_ACTIVE) )
    {
        status     = d_arg_internal_ini_default(_writer,
                                                _option,
                                                &value);
        is_default = true;
    }

    status = ( (status == D_ARG_OK) &&
               (value.length > 0u) )
             ? d_arg_internal_ini_line(_writer,
                                       _option,
                                       &value,
                                       is_default)
             : status;
    d_arg_buffer_release(&value);

    return status;
}

//==============================================================================
// 5.  GROUPS
//==============================================================================

/*
d_arg_internal_ini_matches
  Whether a group's pass writes an option: configurable, named, and of the
group -- the OPTIONS pass taking the empty group's too.
*/
D_STATIC bool
d_arg_internal_ini_matches(
    const struct d_arg_option* _option,
    const char*                _group
)
{
    const char* const group = d_arg_option_group(_option);

    // not configurable, or with no name to write
    if ( (!d_arg_option_has_flag(_option,
                                 D_ARG_OPTION_CONFIGURABLE))    ||
         (d_arg_option_single_name(_option)[0] == '\0') )
    {
        return false;
    }

    return ( (strcmp(group,
                     _group) == 0)        ||
             ( (strcmp(_group,
                       "OPTIONS") == 0) &&
               (group[0] == '\0') ) );
}

/*
d_arg_internal_ini_group
  One pass of CLI11's to_config over the command's own options: a named
group's header, when descriptions are written, then each option the pass
writes.
*/
D_STATIC int
d_arg_internal_ini_group(
    const struct d_arg_internal_ini_writer* _writer,
    const char*                             _group
)
{
    const struct d_arg_command* const command = _writer->command;
    int                               status  = D_ARG_OK;

    // the group's header
    if ( (_writer->describe)                &&
         (strcmp(_group,
                 "OPTIONS") != 0)           &&
         (_group[0] != '\0') )
    {
        status = d_arg_internal_ini_header(_writer,
                                           _group);
    }

    // each option of the group
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < command->options.count);
         ++index)
    {
        struct d_arg_option* const option = command->options.items[index];

        if (d_arg_internal_ini_matches(option,
                                       _group))
        {
            status = d_arg_internal_ini_option(_writer,
                                               option);
        }
    }

    return status;
}

/*
d_arg_internal_ini_first_seen
  Whether an option is the first of the command's to name its group, the
order CLI11's get_groups lists the groups in.
*/
D_STATIC bool
d_arg_internal_ini_first_seen(
    const struct d_arg_command* _command,
    size_t                      _index
)
{
    const char* const group = d_arg_option_group(
                                  _command->options.items[_index]);

    // an earlier option of the group
    for (size_t earlier = 0u; earlier < _index; ++earlier)
    {
        if (strcmp(d_arg_option_group(_command->options.items[earlier]),
                   group) == 0)
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_ini_options
  CLI11's passes over the command's groups: OPTIONS first, then each other
group once, in the order the command's options first name them.
*/
D_STATIC int
d_arg_internal_ini_options(
    const struct d_arg_internal_ini_writer* _writer
)
{
    const struct d_arg_command* const command = _writer->command;
    int                               status  = d_arg_internal_ini_group(
                                                    _writer,
                                                    "OPTIONS");

    // each other group, once
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < command->options.count);
         ++index)
    {
        const char* const group = d_arg_option_group(
                                      command->options.items[index]);

        if ( (strcmp(group,
                     "OPTIONS") != 0)                          &&
             (group[0] != '\0')                                &&
             (d_arg_internal_ini_first_seen(command,
                                            index)) )
        {
            status = d_arg_internal_ini_group(_writer,
                                              group);
        }
    }

    return status;
}

//==============================================================================
// 6.  SUBCOMMANDS
//==============================================================================

/*
d_arg_internal_ini_nameless
  CLI11's pass over the nameless subcommands, option groups among them:
each with a value -- or each, when defaults are asked for -- written in
turn with the command's prefix, after its group's header when descriptions
are written and the group is named, not with `+`.
*/
D_STATIC int
d_arg_internal_ini_nameless(
    const struct d_arg_internal_ini_writer* _writer
)
{
    const struct d_arg_command* const command = _writer->command;
    int                               status  = D_ARG_OK;

    // each nameless subcommand in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const sub   =
            command->subcommands.items[index];
        const char* const                 group = d_arg_command_group(sub);

        // named, or unused while only values are written
        if ( (d_arg_command_name(sub)[0] != '\0')                       ||
             ( (_writer->mode == (int)D_ARG_CONFIG_OUTPUT_ACTIVE) &&
               (d_arg_command_count_all(sub) == 0u) ) )
        {
            continue;
        }

        // the group's header
        if ( (_writer->describe) &&
             (group[0] != '\0')  &&
             (group[0] != '+') )
        {
            status = d_arg_internal_ini_header(_writer,
                                               group);
        }

        status = (status == D_ARG_OK)
                 ? d_arg_config_to_text(_writer->format,
                                        sub,
                                        _writer->mode,
                                        _writer->describe,
                                        _writer->prefix,
                                        _writer->out)
                 : status;
    }

    return status;
}

/*
d_arg_internal_ini_ancestors
  The names of a command and its ancestors below the root, outermost first,
each cleaned and followed by the section separator.
*/
D_STATIC void
d_arg_internal_ini_ancestors(
    const struct d_arg_internal_ini_writer* _writer,
    const struct d_arg_command*             _command
)
{
    // the root contributes nothing
    if (!_command->parent)
    {
        return;
    }

    d_arg_internal_ini_ancestors(_writer,
                                 _command->parent);
    (void)d_arg_internal_ini_clean(_writer,
                                   d_arg_internal_text_of(
                                       d_arg_command_name(_command)),
                                   _writer->out);
    (void)d_arg_buffer_append_char(_writer->out,
                                   _writer->format->parent_separator);

    return;
}

/*
d_arg_internal_ini_section
  A subcommand's section header: the prefix -- or, below the top without
one, the names of the command and its ancestors -- then the subcommand's
name, cleaned, in brackets.
*/
D_STATIC int
d_arg_internal_ini_section(
    const struct d_arg_internal_ini_writer* _writer,
    const struct d_arg_command*             _sub
)
{
    struct d_arg_buffer* const out = _writer->out;

    (void)d_arg_buffer_append_char(out,
                                   '[');

    // the prefix, or the command's path
    if ( (_writer->prefix.length > 0u) ||
         (!_writer->command->parent) )
    {
        (void)d_arg_buffer_append_text(out,
                                       _writer->prefix);
    }
    else
    {
        d_arg_internal_ini_ancestors(_writer,
                                     _writer->command);
    }

    (void)d_arg_internal_ini_clean(_writer,
                                   d_arg_internal_text_of(
                                       d_arg_command_name(_sub)),
                                   out);

    return d_arg_buffer_append_cstr(out,
                                    "]\n");
}

/*
d_arg_internal_ini_subcommand
  A named subcommand written: as a section of its own, when it is
configurable and was used, or when every default is asked for; otherwise
with the prefix, its name and the section separator before each of its
names.
*/
D_STATIC int
d_arg_internal_ini_subcommand(
    const struct d_arg_internal_ini_writer* _writer,
    const struct d_arg_command*             _sub
)
{
    struct d_arg_buffer prefix  = D_ARG_BUFFER_INIT;
    const bool          section =
        ( (d_arg_command_has_flag(_sub,
                                  D_ARG_COMMAND_CONFIGURABLE))          &&
          ( (d_arg_command_got_subcommand(_writer->command,
                                          _sub))                   ||
            (_writer->mode == (int)D_ARG_CONFIG_OUTPUT_ALL_DEFAULTS) ) );
    int                 status  = D_ARG_OK;

    // a section of its own, or a longer prefix
    if (section)
    {
        status = d_arg_internal_ini_section(_writer,
                                            _sub);
    }
    else
    {
        (void)d_arg_buffer_append_text(&prefix,
                                       _writer->prefix);
        (void)d_arg_internal_ini_clean(_writer,
                                       d_arg_internal_text_of(
                                           d_arg_command_name(_sub)),
                                       &prefix);
        status = d_arg_buffer_append_char(&prefix,
                                          _writer->format->parent_separator);
    }

    status = (status == D_ARG_OK)
             ? d_arg_config_to_text(_writer->format,
                                    _sub,
                                    _writer->mode,
                                    _writer->describe,
                                    d_arg_buffer_text(&prefix),
                                    _writer->out)
             : status;
    d_arg_buffer_release(&prefix);

    return status;
}

/*
d_arg_internal_ini_named
  CLI11's pass over the named subcommands: each with a value -- or each,
when defaults are asked for, but for the defaults of the subcommands used
alone, each used -- written as d_arg_internal_ini_subcommand writes it.
*/
D_STATIC int
d_arg_internal_ini_named(
    const struct d_arg_internal_ini_writer* _writer
)
{
    const struct d_arg_command* const command = _writer->command;
    int                               status  = D_ARG_OK;

    // each named subcommand in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const sub =
            command->subcommands.items[index];

        // nameless, unused, or not used when only those used are wanted
        if ( (d_arg_command_name(sub)[0] == '\0')                          ||
             ( (_writer->mode == (int)D_ARG_CONFIG_OUTPUT_ACTIVE) &&
               (d_arg_command_count_all(sub) == 0u) )                      ||
             ( (_writer->mode ==
                (int)D_ARG_CONFIG_OUTPUT_ACTIVE_SUBCOMMAND_DEFAULTS) &&
               (!d_arg_command_got_subcommand(command,
                                              sub)) ) )
        {
            continue;
        }

        status = d_arg_internal_ini_subcommand(_writer,
                                               sub);
    }

    return status;
}

//==============================================================================
// 7.  WRITING
//==============================================================================

/*
d_arg_internal_ini_mode
  Whether a value is one of d_arg_config_output_mode's.
*/
D_STATIC bool
d_arg_internal_ini_mode(
    int _mode
)
{
    return ( (_mode == (int)D_ARG_CONFIG_OUTPUT_ACTIVE)                     ||
             (_mode == (int)D_ARG_CONFIG_OUTPUT_ALL_DEFAULTS)               ||
             (_mode == (int)D_ARG_CONFIG_OUTPUT_ACTIVE_SUBCOMMAND_DEFAULTS) );
}

/*
d_arg_internal_ini_write
  A command's text, in CLI11's order: its own options group by group, its
nameless subcommands, its named ones.
*/
D_STATIC int
d_arg_internal_ini_write(
    const struct d_arg_internal_ini_writer* _writer
)
{
    int status = d_arg_internal_ini_options(_writer);

    status = (status == D_ARG_OK)
             ? d_arg_internal_ini_nameless(_writer)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_ini_named(_writer)
           : status;
}

/*
d_arg_internal_ini_emit
  The command's text written apart, then appended -- headed by the
command's description as a comment, when descriptions are written and
there is any text.
*/
D_STATIC int
d_arg_internal_ini_emit(
    const struct d_arg_internal_ini_writer* _writer,
    struct d_arg_buffer*                    _out
)
{
    const char lead[3] = { _writer->format->comment,
                           _writer->format->comment,
                           ' ' };
    int        status  = d_arg_internal_ini_write(_writer);

    // the command's description heads its text
    if ( (status == D_ARG_OK)          &&
         (_writer->describe)           &&
         (_writer->out->length > 0u) )
    {
        status = d_arg_internal_ini_comment(
                     d_arg_internal_span(lead,
                                         3u),
                     d_arg_command_description(_writer->command),
                     _out);
    }

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_text(_out,
                                      d_arg_buffer_text(_writer->out))
           : status;
}

/*
d_arg_config_to_text_default
  CLI11's ConfigBase::to_config, its six parameters its own four with the
format it is a member of and the text it returns: the command's part
written and emitted.
*/
int
d_arg_config_to_text_default(
    const struct d_arg_config_format* _format,
    const struct d_arg_command*       _command,
    int                               _mode,
    bool                              _write_description,
    struct d_pack_text                _prefix,
    struct d_arg_buffer*              _out
)
{
    // validate the parameters
    if ( (!_format)                         ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_ini_mode(_mode))  ||
         ( (!_prefix.data) &&
           (_prefix.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer                    text   = D_ARG_BUFFER_INIT;
    const struct d_arg_internal_ini_writer writer =
    {
        _format,                                          // format
        _command,                                         // command
        _mode,                                            // mode
        _write_description,                               // describe
        (_prefix.data != NULL)
        ? _prefix
        : d_arg_internal_span("",
                              0u),                        // prefix
        { '#', ';', _format->comment, _format->parent_separator,
          _format->literal_quote, _format->string_quote,
          _format->array_start, _format->array_end,
          _format->value_delimiter, _format->array_separator }, // keys
        &text                                             // out
    };
    const int                              status = d_arg_internal_ini_emit(
                                                        &writer,
                                                        _out);

    d_arg_buffer_release(&text);

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
