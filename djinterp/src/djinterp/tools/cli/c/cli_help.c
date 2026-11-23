/*******************************************************************************
* djinterp [c][tools]                                                 cli_help.c
*
* A program's description as JSON, declared in cli_help.h: the writer, then
* the description, from the program's parser and its registry.
*   The writer appends to a buffer, so that a description is written to the
* program's output whole or not at all, as help is. A command is described
* as help shows it: the options and subcommands help hides are left out, and
* an option group's options are its command's, under the group's heading.
*
*
* path:      /src/djinterp/tools/cli/c/cli_help.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/cli/c/cli_help.h"  // corresponding header
// std
#include <math.h>    // isfinite
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <stdlib.h>   // strtod
#include <string.h>   // memcpy, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // dj_arg_parser
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_validator.h"  // choices
#include "./cli_internal.h"  // d_cli_internal_exit_meanings
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64

#if RE_STD_HAS_INT64
#include "../../../../../inc/djinterp/c/option/option_env.h"           // rows
#include "../../../../../inc/djinterp/c/option/option_env_registry.h"  // frames
#endif


//==============================================================================
// 1.  THE WRITER
//==============================================================================

/*
d_internal_cli_json_put
  Bytes appended, while the writer is well.
*/
static void
d_internal_cli_json_put(
    struct dj_cli_json* _json,
    const char*         _text,
    size_t              _size
)
{
    if (_json->status == D_ARG_OK)
    {
        _json->status = d_arg_buffer_append(_json->out, _text, _size);
    }

    return;
}

/*
d_internal_cli_json_line
  A new line, indented two spaces for each container open.
*/
static void
d_internal_cli_json_line(
    struct dj_cli_json* _json
)
{
    d_internal_cli_json_put(_json, "\n", 1u);

    for (size_t level = 0u; level < _json->depth; ++level)
    {
        d_internal_cli_json_put(_json, "  ", 2u);
    }

    return;
}

/*
d_internal_cli_json_ready
  Whether the writer may write: one given, and well.
*/
static bool
d_internal_cli_json_ready(
    const struct dj_cli_json* _json
)
{
    return ( (_json != NULL) &&
             (_json->status == D_ARG_OK) );
}

/*
d_internal_cli_json_value
  Readies the next value: after a member's name, on its line; as an array's
element, after a comma and on a line of its own; alone, once, at depth 0.
Anything else -- a value where an object wants a name -- fails the writer.
*/
static bool
d_internal_cli_json_value(
    struct dj_cli_json* _json
)
{
    if (!d_internal_cli_json_ready(_json))
    {
        return false;
    }

    if (_json->named)
    {
        _json->named = false;

        return true;
    }

    if ( ( (_json->depth == 0u) &&
           (_json->count[0] > 0u) ) ||
         ( (_json->depth > 0u) &&
           (_json->open[_json->depth] != '[') ) )
    {
        _json->status = D_ARG_ERROR_BAD_ARGUMENT;

        return false;
    }

    if ( (_json->depth > 0u) &&
         (_json->count[_json->depth] > 0u) )
    {
        d_internal_cli_json_put(_json, ",", 1u);
    }

    if (_json->depth > 0u)
    {
        d_internal_cli_json_line(_json);
    }

    ++_json->count[_json->depth];

    return d_internal_cli_json_ready(_json);
}

/*
d_internal_cli_json_utf8
  The length of the well-formed UTF-8 sequence `_text` begins with, of at
most `_left` bytes (RFC 3629: no overlong form, no surrogate, nothing past
U+10FFFF); 0 for a byte that begins none.
*/
static size_t
d_internal_cli_json_utf8(
    const unsigned char* _text,
    size_t               _left
)
{
    const unsigned char lead = _text[0];
    size_t              size;
    unsigned char       low  = 0x80u;
    unsigned char       high = 0xBFu;

    if (lead < 0x80u)
    {
        return 1u;
    }

    size = (lead >= 0xC2u && lead <= 0xDFu) ? 2u
         : (lead >= 0xE0u && lead <= 0xEFu) ? 3u
         : (lead >= 0xF0u && lead <= 0xF4u) ? 4u
         : 0u;
    low  = (lead == 0xE0u) ? 0xA0u : (lead == 0xF0u) ? 0x90u : low;
    high = (lead == 0xEDu) ? 0x9Fu : (lead == 0xF4u) ? 0x8Fu : high;

    if ( (size == 0u) ||
         (size > _left) ||
         (_text[1] < low) ||
         (_text[1] > high) )
    {
        return 0u;
    }

    for (size_t at = 2u; at < size; ++at)
    {
        if ( (_text[at] < 0x80u) ||
             (_text[at] > 0xBFu) )
        {
            return 0u;
        }
    }

    return size;
}

/*
d_internal_cli_json_escaped
  Text as a JSON string: quoted, a quote and a backslash escaped, a control
character as its short escape or \u00XX, UTF-8 as it is, and each byte that
begins no UTF-8 sequence as �.
*/
static void
d_internal_cli_json_escaped(
    struct dj_cli_json* _json,
    const char*         _text,
    size_t              _size
)
{
    const unsigned char* const text = (const unsigned char*)_text;
    size_t                     at   = 0u;
    char                       code[8];

    d_internal_cli_json_put(_json, "\"", 1u);

    while ( (at < _size) &&
            (_json->status == D_ARG_OK) )
    {
        const unsigned char each = text[at];
        const size_t        size = d_internal_cli_json_utf8(text + at,
                                                            _size - at);
        const char* const   short_form =
            (each == '"')  ? "\\\"" : (each == '\\') ? "\\\\"
          : (each == '\b') ? "\\b"  : (each == '\f') ? "\\f"
          : (each == '\n') ? "\\n"  : (each == '\r') ? "\\r"
          : (each == '\t') ? "\\t"  : NULL;

        if (short_form != NULL)
        {
            d_internal_cli_json_put(_json, short_form, 2u);
        }
        else if (each < 0x20u)
        {
            (void)snprintf(code, sizeof(code), "\\u%04x", (unsigned)each);
            d_internal_cli_json_put(_json, code, 6u);
        }
        else if (size == 0u)
        {
            d_internal_cli_json_put(_json, "\\ufffd", 6u);
        }
        else
        {
            d_internal_cli_json_put(_json, _text + at, size);
        }

        at += (size == 0u) ? 1u : size;
    }

    d_internal_cli_json_put(_json, "\"", 1u);

    return;
}

/*
d_cli_json_init
  Nothing open, nothing written.
*/
void d_cli_json_init(
    struct dj_cli_json*   _json,
    struct dj_arg_buffer* _out
)
{
    if (_json == NULL)
    {
        return;
    }

    memset(_json, 0, sizeof(*_json));
    _json->out    = _out;
    _json->status = (_out != NULL) ? D_ARG_OK : D_ARG_ERROR_BAD_ARGUMENT;

    return;
}

/*
d_internal_cli_json_open
  A container opened as the next value, as deep as the writer allows.
*/
static void
d_internal_cli_json_open(
    struct dj_cli_json* _json,
    char                _bracket
)
{
    if (!d_internal_cli_json_value(_json))
    {
        return;
    }

    if (_json->depth >= (size_t)D_CLI_JSON_DEPTH)
    {
        _json->status = D_ARG_ERROR_BAD_ARGUMENT;

        return;
    }

    d_internal_cli_json_put(_json, &_bracket, 1u);
    ++_json->depth;
    _json->open[_json->depth]  = _bracket;
    _json->count[_json->depth] = 0u;

    return;
}

/*
d_cli_json_object, d_cli_json_array
  An object, an array.
*/
void d_cli_json_object(
    struct dj_cli_json* _json
)
{
    d_internal_cli_json_open(_json, '{');

    return;
}

void d_cli_json_array(
    struct dj_cli_json* _json
)
{
    d_internal_cli_json_open(_json, '[');

    return;
}

/*
d_cli_json_end
  The innermost container closed: on a line of its own after its values,
on its opening's line when it holds none.
*/
void d_cli_json_end(
    struct dj_cli_json* _json
)
{
    if (!d_internal_cli_json_ready(_json))
    {
        return;
    }

    if ( (_json->depth == 0u) ||
         (_json->named) )
    {
        _json->status = D_ARG_ERROR_BAD_ARGUMENT;

        return;
    }

    {
        const char   close = (_json->open[_json->depth] == '{') ? '}' : ']';
        const size_t held  = _json->count[_json->depth];

        --_json->depth;

        if (held > 0u)
        {
            d_internal_cli_json_line(_json);
        }

        d_internal_cli_json_put(_json, &close, 1u);
    }

    return;
}

/*
d_cli_json_member
  A member's name, on a line of its own after a comma where one came
before; its value follows.
*/
void d_cli_json_member(
    struct dj_cli_json* _json,
    const char*         _name
)
{
    if (!d_internal_cli_json_ready(_json))
    {
        return;
    }

    if ( (_json->depth == 0u) ||
         (_json->open[_json->depth] != '{') ||
         (_json->named) )
    {
        _json->status = D_ARG_ERROR_BAD_ARGUMENT;

        return;
    }

    if (_json->count[_json->depth] > 0u)
    {
        d_internal_cli_json_put(_json, ",", 1u);
    }

    d_internal_cli_json_line(_json);
    d_internal_cli_json_escaped(_json, (_name != NULL) ? _name : "",
                                (_name != NULL) ? strlen(_name) : 0u);
    d_internal_cli_json_put(_json, ": ", 2u);
    ++_json->count[_json->depth];
    _json->named = true;

    return;
}

/*
d_cli_json_string, d_cli_json_text
  Text escaped; null for none.
*/
void d_cli_json_string(
    struct dj_cli_json* _json,
    const char*         _text
)
{
    d_cli_json_text(_json, _text, (_text != NULL) ? strlen(_text) : 0u);

    return;
}

void d_cli_json_text(
    struct dj_cli_json* _json,
    const char*         _text,
    size_t              _size
)
{
    if (!d_internal_cli_json_value(_json))
    {
        return;
    }

    if (_text == NULL)
    {
        d_internal_cli_json_put(_json, "null", 4u);

        return;
    }

    d_internal_cli_json_escaped(_json, _text, _size);

    return;
}

/*
d_cli_json_integer
  In decimal.
*/
void d_cli_json_integer(
    struct dj_cli_json* _json,
    int64_t             _value
)
{
    char       text[32];
    const int  size = snprintf(text, sizeof(text), "%lld",
                               (long long)_value);

    if (d_internal_cli_json_value(_json))
    {
        d_internal_cli_json_put(_json, text, (size > 0) ? (size_t)size : 0u);
    }

    return;
}

/*
d_cli_json_real
  The fewest significant digits that read back as the value, with '.' for
the decimal point whatever the locale says; null for what JSON has no
number for.
*/
void d_cli_json_real(
    struct dj_cli_json* _json,
    double              _value
)
{
    char text[40];
    int  size = 0;

    if (!d_internal_cli_json_value(_json))
    {
        return;
    }

    if (!isfinite(_value))
    {
        d_internal_cli_json_put(_json, "null", 4u);

        return;
    }

    for (int digits = 1; digits <= 17; ++digits)
    {
        size = snprintf(text, sizeof(text), "%.*g", digits, _value);

        if (strtod(text, NULL) == _value)
        {
            break;
        }
    }

    for (int at = 0; at < size; ++at)
    {
        text[at] = (text[at] == ',') ? '.' : text[at];
    }

    d_internal_cli_json_put(_json, text, (size > 0) ? (size_t)size : 0u);

    return;
}

/*
d_cli_json_bool, d_cli_json_null
  A truth value; null.
*/
void d_cli_json_bool(
    struct dj_cli_json* _json,
    bool                _value
)
{
    if (d_internal_cli_json_value(_json))
    {
        d_internal_cli_json_put(_json, (_value) ? "true" : "false",
                                (_value) ? 4u : 5u);
    }

    return;
}

void d_cli_json_null(
    struct dj_cli_json* _json
)
{
    if (d_internal_cli_json_value(_json))
    {
        d_internal_cli_json_put(_json, "null", 4u);
    }

    return;
}

/*
d_cli_json_finish
  The final newline, once every container is closed and a value written.
*/
int d_cli_json_finish(
    struct dj_cli_json* _json
)
{
    if (_json == NULL)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    if ( (_json->status == D_ARG_OK) &&
         ( (_json->depth != 0u) ||
           (_json->named) ||
           (_json->count[0] == 0u) ) )
    {
        _json->status = D_ARG_ERROR_BAD_ARGUMENT;
    }

    d_internal_cli_json_put(_json, "\n", 1u);

    return _json->status;
}


//==============================================================================
// 2.  A COMMAND
//==============================================================================

/*
d_internal_cli_help_text
  A member and its text, left out where there is none.
*/
static void
d_internal_cli_help_text(
    struct dj_cli_json* _json,
    const char*         _name,
    const char*         _text
)
{
    if ( (_text != NULL) &&
         (_text[0] != '\0') )
    {
        d_cli_json_member(_json, _name);
        d_cli_json_string(_json, _text);
    }

    return;
}

/*
d_internal_cli_help_shown_option, _shown_command
  What help shows: an option not in the group named ""; a named subcommand
not hidden there, nor disabled.
*/
static bool
d_internal_cli_help_shown_option(
    const struct dj_arg_option* _option
)
{
    return ( (_option != NULL) &&
             ( (_option->group == NULL) ||
               (_option->group[0] != '\0') ) );
}

static bool
d_internal_cli_help_shown_command(
    const struct dj_arg_command* _command
)
{
    return ( (!d_arg_command_is_option_group(_command)) &&
             ( (_command->group == NULL) ||
               (_command->group[0] != '\0') ) &&
             (!d_arg_command_has_flag(_command, D_ARG_COMMAND_DISABLED)) );
}

/*
d_internal_cli_help_counted, _listed
  Choice visits: one that counts, one that writes each as a string.
*/
static int
d_internal_cli_help_counted(
    struct dj_pack_text _choice,
    void*               _context
)
{
    (void)_choice;
    ++*(size_t*)_context;

    return D_ARG_OK;
}

static int
d_internal_cli_help_listed(
    struct dj_pack_text _choice,
    void*               _context
)
{
    d_cli_json_text((struct dj_cli_json*)_context, _choice.data,
                    _choice.length);

    return D_ARG_OK;
}

/*
d_internal_cli_help_choices
  `choices`: the values an option's validators list, where they list any.
*/
static void
d_internal_cli_help_choices(
    struct dj_cli_json*         _json,
    const struct dj_arg_option* _option
)
{
    size_t count = 0u;

    for (size_t i = 0u; i < _option->validators.count; ++i)
    {
        (void)d_arg_validator_each_choice(&_option->validators.items[i],
                                          d_internal_cli_help_counted,
                                          &count);
    }

    if (count == 0u)
    {
        return;
    }

    d_cli_json_member(_json, "choices");
    d_cli_json_array(_json);

    for (size_t i = 0u; i < _option->validators.count; ++i)
    {
        (void)d_arg_validator_each_choice(&_option->validators.items[i],
                                          d_internal_cli_help_listed,
                                          _json);
    }

    d_cli_json_end(_json);

    return;
}

/*
d_internal_cli_help_value
  The value an option takes, as help shows it: its placeholder, `value`;
and `optional` where a flag's long name may take one after `=`.
*/
static void
d_internal_cli_help_value(
    struct dj_cli_json*         _json,
    const struct dj_arg_option* _option
)
{
    const char* const placeholder = ( (_option->type_name != NULL) &&
                                      (_option->type_name[0] != '\0') )
                                    ? _option->type_name
                                    : "VALUE";
    bool              optional    = false;

    for (size_t i = 0u;
         (_option->expected_max == 0) &&
         (_option->type_name != NULL) &&
         (_option->type_name[0] != '\0') &&
         (i < _option->flag_values.count);
         ++i)
    {
        optional = ( (optional) ||
                     (strcmp(_option->flag_values.items[i], "false") != 0) );
    }

    if ( (_option->expected_max != 0) ||
         (optional) )
    {
        d_cli_json_member(_json, "value");
        d_cli_json_string(_json, placeholder);
    }

    if (optional)
    {
        d_cli_json_member(_json, "optional");
        d_cli_json_bool(_json, true);
    }

    return;
}

/*
d_internal_cli_help_standard
  Whether an option is one every program on the kit has (GRAMMAR.md,
section 6): a command's help, the version, -q, -v, --color.
*/
static bool
d_internal_cli_help_standard(
    const struct dj_cli_program* _program,
    const struct dj_arg_option*  _option
)
{
    const struct dj_arg_command* const holder = _option->parent;

    return ( ( (holder != NULL) &&
               ( (_option == holder->help) ||
                 (_option == holder->help_all) ||
                 (_option == holder->version) ) ) ||
             (_option == _program->quiet) ||
             (_option == _program->verbose) ||
             (_option == _program->colour_option) ||
             (_option == _program->colour_alias) );
}

/*
d_internal_cli_help_option
  An option's object: its names, as typed; its value; what help says of
it; its heading, default, variable and choices, where it has them; whether
it is required, and whether it is one every program has.
*/
static void
d_internal_cli_help_option(
    struct dj_cli_json*          _json,
    const struct dj_cli_program* _program,
    const struct dj_arg_option*  _option,
    const char*                  _group
)
{
    char name[128];

    d_cli_json_object(_json);
    d_cli_json_member(_json, "names");
    d_cli_json_array(_json);

    for (size_t i = 0u; i < _option->short_names.count; ++i)
    {
        (void)snprintf(name, sizeof(name), "-%s",
                       _option->short_names.items[i]);
        d_cli_json_string(_json, name);
    }

    for (size_t i = 0u; i < _option->long_names.count; ++i)
    {
        (void)snprintf(name, sizeof(name), "--%s",
                       _option->long_names.items[i]);
        d_cli_json_string(_json, name);
    }

    d_cli_json_end(_json);
    d_internal_cli_help_value(_json, _option);
    d_internal_cli_help_text(_json, "summary", _option->description);
    d_internal_cli_help_text(_json, "group",
                             (_option->group != NULL) ? _option->group
                                                      : _group);
    d_internal_cli_help_text(_json, "default", _option->default_text);
    d_internal_cli_help_text(_json, "env", _option->env_name);
    d_internal_cli_help_choices(_json, _option);
    d_cli_json_member(_json, "required");
    d_cli_json_bool(_json, ((_option->flags & D_ARG_OPTION_REQUIRED) != 0u));
    d_cli_json_member(_json, "standard");
    d_cli_json_bool(_json, d_internal_cli_help_standard(_program, _option));
    d_cli_json_end(_json);

    return;
}

/*
d_internal_cli_help_options_of
  Each option of a command's list that has a name and that help shows.
*/
static void
d_internal_cli_help_options_of(
    struct dj_cli_json*          _json,
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const char*                  _group
)
{
    for (size_t i = 0u; i < _command->options.count; ++i)
    {
        const struct dj_arg_option* const option = _command->options.items[i];

        if ( (d_arg_option_is_nonpositional(option)) &&
             (d_internal_cli_help_shown_option(option)) )
        {
            d_internal_cli_help_option(_json, _program, option, _group);
        }
    }

    return;
}

/*
d_internal_cli_help_options
  `options`: a command's own, then each option group's, under its name.
*/
static void
d_internal_cli_help_options(
    struct dj_cli_json*          _json,
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command
)
{
    d_cli_json_member(_json, "options");
    d_cli_json_array(_json);
    d_internal_cli_help_options_of(_json, _program, _command, NULL);

    for (size_t i = 0u; i < _command->subcommands.count; ++i)
    {
        const struct dj_arg_command* const group =
            _command->subcommands.items[i];

        // an option group, enabled, its options under its heading
        if ( (d_arg_command_is_option_group(group)) &&
             (!d_arg_command_has_flag(group, D_ARG_COMMAND_DISABLED)) )
        {
            d_internal_cli_help_options_of(_json, _program, group,
                                           d_arg_command_group(group));
        }
    }

    d_cli_json_end(_json);

    return;
}

/*
d_internal_cli_help_arguments
  `arguments`: the positionals, in order, each its name, what help says of
it, how many values it takes -- `max` null for no limit -- whether it is
required, its default and its choices.
*/
static void
d_internal_cli_help_arguments(
    struct dj_cli_json*          _json,
    const struct dj_arg_command* _command
)
{
    d_cli_json_member(_json, "arguments");
    d_cli_json_array(_json);

    for (size_t i = 0u; i < _command->options.count; ++i)
    {
        const struct dj_arg_option* const option = _command->options.items[i];
        const int                         most   =
            d_arg_option_items_expected_max(option);

        if ( (d_arg_option_is_nonpositional(option)) ||
             (!d_arg_option_is_positional(option)) ||
             (!d_internal_cli_help_shown_option(option)) )
        {
            continue;
        }

        d_cli_json_object(_json);
        d_internal_cli_help_text(_json, "name", option->positional_name);
        d_internal_cli_help_text(_json, "summary", option->description);
        d_cli_json_member(_json, "min");
        d_cli_json_integer(_json, d_arg_option_items_expected_min(option));
        d_cli_json_member(_json, "max");

        if (most >= (int)D_ARG_EXPECTED_UNLIMITED)
        {
            d_cli_json_null(_json);
        }
        else
        {
            d_cli_json_integer(_json, most);
        }

        d_cli_json_member(_json, "required");
        d_cli_json_bool(_json,
                        ((option->flags & D_ARG_OPTION_REQUIRED) != 0u));
        d_internal_cli_help_text(_json, "default", option->default_text);
        d_internal_cli_help_choices(_json, option);
        d_cli_json_end(_json);
    }

    d_cli_json_end(_json);

    return;
}

/*
d_internal_cli_help_command
  A command's object: its name, its path from the program, what help says
of it, its aliases; its options, arguments and subcommands, these
described the same way, down.
*/
static void
d_internal_cli_help_command(
    struct dj_cli_json*          _json,
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command
)
{
    struct dj_arg_buffer path = D_ARG_BUFFER_INIT;

    d_cli_json_object(_json);
    d_internal_cli_help_text(_json, "name", _command->name);

    if (d_cli_command_path(_command, &path) == D_ARG_OK)
    {
        d_internal_cli_help_text(_json, "path", path.data);
    }
    else if (_json->status == D_ARG_OK)
    {
        _json->status = D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_arg_buffer_release(&path);
    d_internal_cli_help_text(_json, "summary", _command->description);
    d_internal_cli_help_text(_json, "usage", _command->usage);
    d_internal_cli_help_text(_json, "footer", _command->footer);

    if (_command->aliases.count > 0u)
    {
        d_cli_json_member(_json, "aliases");
        d_cli_json_array(_json);

        for (size_t i = 0u; i < _command->aliases.count; ++i)
        {
            d_cli_json_string(_json, _command->aliases.items[i]);
        }

        d_cli_json_end(_json);
    }

    d_internal_cli_help_options(_json, _program, _command);
    d_internal_cli_help_arguments(_json, _command);
    d_cli_json_member(_json, "commands");
    d_cli_json_array(_json);

    for (size_t i = 0u; i < _command->subcommands.count; ++i)
    {
        if (d_internal_cli_help_shown_command(_command->subcommands.items[i]))
        {
            d_internal_cli_help_command(_json, _program,
                                        _command->subcommands.items[i]);
        }
    }

    d_cli_json_end(_json);
    d_cli_json_end(_json);

    return;
}


//==============================================================================
// 3.  THE SETTINGS AND THE ENVIRONMENT
//==============================================================================

// D_INTERNAL_CLI_HELP_ROOM
//   constant: the room for a prefix or a variable's name; a longer one is
// cut short.
#define D_INTERNAL_CLI_HELP_ROOM 256u

/*
d_internal_cli_help_prefix
  The program's own prefix, as the kit reads the environment under it
(cli_settings.c): its name upper-cased, each '-' an '_', and an '_' after
it; empty when it does not fit, or is the shared one.
*/
static void
d_internal_cli_help_prefix(
    const char* _name,
    char*       _buffer,
    size_t      _capacity
)
{
    size_t at = 0u;

    for (const char* each = (_name != NULL) ? _name : "";
         (*each != '\0') && ((at + 2u) < _capacity);
         ++each)
    {
        _buffer[at++] = ( (*each >= 'a') &&
                          (*each <= 'z') )
                        ? (char)(*each - 'a' + 'A')
                        : (*each == '-') ? '_' : *each;
    }

    _buffer[at++] = '_';
    _buffer[at]   = '\0';

    if ( (_name == NULL) ||
         (at != strlen(_name) + 1u) ||
         (strcmp(_buffer, "DJINTERP_") == 0) )
    {
        _buffer[0] = '\0';
    }

    return;
}

#if RE_STD_HAS_INT64

/*
d_internal_cli_help_variable
  A setting's variable under a prefix: the prefix, then the setting's full
name upper-cased, each '.' an '_', as the registry's import reads it.
*/
static void
d_internal_cli_help_variable(
    const char* _prefix,
    const char* _djin,
    const char* _name,
    char*       _buffer,
    size_t      _capacity
)
{
    const int size = (_djin != NULL)
                     ? snprintf(_buffer, _capacity, "%s%s_%s", _prefix,
                                _djin, _name)
                     : snprintf(_buffer, _capacity, "%s%s", _prefix, _name);
    const size_t end  = ( (size > 0) &&
                          ((size_t)size < _capacity) )
                        ? (size_t)size
                        : 0u;

    for (size_t at = strlen(_prefix); at < end; ++at)
    {
        _buffer[at] = ( (_buffer[at] >= 'a') &&
                        (_buffer[at] <= 'z') )
                      ? (char)(_buffer[at] - 'a' + 'A')
                      : (_buffer[at] == '.') ? '_' : _buffer[at];
    }

    _buffer[end] = '\0';

    return;
}

/*
d_internal_cli_help_name_of
  The name a value has among a row's names; NULL for none.
*/
static const char*
d_internal_cli_help_name_of(
    const struct dj_option_env_var* _row,
    int64_t                         _value
)
{
    for (uint32_t i = 0u; i < _row->name_count; ++i)
    {
        if (_row->names[i].value == _value)
        {
            return _row->names[i].name;
        }
    }

    return NULL;
}

/*
d_internal_cli_help_flags
  Flags as names: the one name of the whole value, where it has one --
`all`, `none` -- and otherwise the names of each single bit it sets.
*/
static void
d_internal_cli_help_flags(
    struct dj_cli_json*             _json,
    const struct dj_option_env_var* _row,
    const char*                     _whole
)
{
    d_cli_json_array(_json);

    if (_whole != NULL)
    {
        d_cli_json_string(_json, _whole);
    }

    for (uint32_t i = 0u; (_whole == NULL) && (i < _row->name_count); ++i)
    {
        const int64_t bit = _row->names[i].value;

        if ( (bit > 0) &&
             ((bit & (bit - 1)) == 0) &&
             ((_row->integer & bit) == bit) )
        {
            d_cli_json_string(_json, _row->names[i].name);
        }
    }

    d_cli_json_end(_json);

    return;
}

/*
d_internal_cli_help_default
  `default`, as the kind holds it: a truth value, a number, text or null; a
choice by its name; flags by their names.
*/
static void
d_internal_cli_help_default(
    struct dj_cli_json*             _json,
    const struct dj_option_env_var* _row
)
{
    const char* const name = d_internal_cli_help_name_of(_row,
                                                         _row->integer);

    d_cli_json_member(_json, "default");

    switch (_row->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            d_cli_json_bool(_json, (_row->integer != 0));
            break;
        case D_OPTION_ENV_KIND_REAL:
            d_cli_json_real(_json, _row->real);
            break;
        case D_OPTION_ENV_KIND_TEXT:
            d_cli_json_string(_json, _row->text);
            break;
        case D_OPTION_ENV_KIND_CHOICE:
            if (name != NULL)
            {
                d_cli_json_string(_json, name);
            }
            else
            {
                d_cli_json_integer(_json, _row->integer);
            }
            break;
        case D_OPTION_ENV_KIND_FLAGS:
            d_internal_cli_help_flags(_json, _row, name);
            break;
        default:
            d_cli_json_integer(_json, _row->integer);
            break;
    }

    return;
}

/*
d_internal_cli_help_names
  `choices`, a choice's or flags' names; `names`, an int's named values,
each a name and a value; `unbuilt`, the names ruled but not built.
*/
static void
d_internal_cli_help_names(
    struct dj_cli_json*             _json,
    const struct dj_option_env_var* _row
)
{
    const bool listed = ( (_row->kind == D_OPTION_ENV_KIND_CHOICE) ||
                          (_row->kind == D_OPTION_ENV_KIND_FLAGS) );

    if (_row->name_count > 0u)
    {
        d_cli_json_member(_json, (listed) ? "choices" : "names");
        d_cli_json_array(_json);

        for (uint32_t i = 0u; i < _row->name_count; ++i)
        {
            if (listed)
            {
                d_cli_json_string(_json, _row->names[i].name);
                continue;
            }

            d_cli_json_object(_json);
            d_cli_json_member(_json, "name");
            d_cli_json_string(_json, _row->names[i].name);
            d_cli_json_member(_json, "value");
            d_cli_json_integer(_json, _row->names[i].value);
            d_cli_json_end(_json);
        }

        d_cli_json_end(_json);
    }

    if (_row->unbuilt_count > 0u)
    {
        d_cli_json_member(_json, "unbuilt");
        d_cli_json_array(_json);

        for (uint32_t i = 0u; i < _row->unbuilt_count; ++i)
        {
            d_cli_json_string(_json, _row->unbuilt[i].name);
        }

        d_cli_json_end(_json);
    }

    return;
}

/*
d_internal_cli_help_sources
  `refuses`, the sources a setting refuses, by name; and `env`, the
variables it is read from -- the shared prefix's, then the program's --
unless it refuses the environment.
*/
static void
d_internal_cli_help_sources(
    struct dj_cli_json*             _json,
    const struct dj_option_env_var* _row,
    const char*                     _djin,
    const char*                     _own
)
{
    char variable[D_INTERNAL_CLI_HELP_ROOM];

    if (_row->refused != 0u)
    {
        d_cli_json_member(_json, "refuses");
        d_cli_json_array(_json);

        for (int32_t source = (int32_t)D_OPTION_ENV_SOURCE_DEFAULT;
             source <= (int32_t)D_OPTION_ENV_SOURCE_OVERRIDE;
             ++source)
        {
            if ((_row->refused & D_OPTION_ENV_REFUSES(source)) != 0u)
            {
                d_cli_json_string(_json, d_option_env_source_name(source));
            }
        }

        d_cli_json_end(_json);
    }

    if ((_row->refused &
         D_OPTION_ENV_REFUSES(D_OPTION_ENV_SOURCE_ENVIRONMENT)) != 0u)
    {
        return;
    }

    d_cli_json_member(_json, "env");
    d_cli_json_array(_json);
    d_internal_cli_help_variable("DJINTERP_", _djin, _row->name, variable,
                                 sizeof(variable));
    d_cli_json_string(_json, variable);

    if (_own[0] != '\0')
    {
        d_internal_cli_help_variable(_own, _djin, _row->name, variable,
                                     sizeof(variable));
        d_cli_json_string(_json, variable);
    }

    d_cli_json_end(_json);

    return;
}

/*
d_internal_cli_help_setting
  A setting's object: its full name, its djin (none on the root), its kind,
default and knob, what it is for, its older name; its names, range and
sources.
*/
static void
d_internal_cli_help_setting(
    struct dj_cli_json*             _json,
    const struct dj_option_env_var* _row,
    const char*                     _djin,
    const char*                     _own
)
{
    char name[D_INTERNAL_CLI_HELP_ROOM];

    (void)snprintf(name, sizeof(name), (_djin != NULL) ? "%s.%s" : "%s%s",
                   (_djin != NULL) ? _djin : "", _row->name);
    d_cli_json_object(_json);
    d_cli_json_member(_json, "name");
    d_cli_json_string(_json, name);
    d_internal_cli_help_text(_json, "djin", _djin);
    d_cli_json_member(_json, "kind");
    d_cli_json_string(_json, d_option_env_kind_name(_row->kind));
    d_internal_cli_help_default(_json, _row);
    d_internal_cli_help_text(_json, "knob", _row->knob);
    d_internal_cli_help_text(_json, "summary", _row->help);
    d_internal_cli_help_text(_json, "alias", _row->alias);
    d_internal_cli_help_names(_json, _row);

    if ( (_row->kind == D_OPTION_ENV_KIND_INT) &&
         (_row->minimum != INT64_MIN) )
    {
        d_cli_json_member(_json, "min");
        d_cli_json_integer(_json, _row->minimum);
    }

    if ( (_row->kind == D_OPTION_ENV_KIND_INT) &&
         (_row->maximum != INT64_MAX) )
    {
        d_cli_json_member(_json, "max");
        d_cli_json_integer(_json, _row->maximum);
    }

    d_internal_cli_help_sources(_json, _row, _djin, _own);
    d_cli_json_end(_json);

    return;
}

#endif  // RE_STD_HAS_INT64

/*
d_internal_cli_help_settings
  `settings`: every one the program reads, the root's first, then each
djin's, in the order the registry lists them; none without the option core.
*/
static void
d_internal_cli_help_settings(
    struct dj_cli_json* _json,
    const char*         _own
)
{
    d_cli_json_member(_json, "settings");
    d_cli_json_array(_json);

#if RE_STD_HAS_INT64
    {
        struct dj_option_env_registry* const registry =
            d_option_env_registry();
        const struct dj_option_env* const    root     =
            d_option_env_registry_frame(registry);
        const uint32_t                       count    =
            d_option_env_registry_count(registry);

        for (uint32_t k = 0u; (root != NULL) && (k < root->var_count); ++k)
        {
            d_internal_cli_help_setting(_json, root->vars[k], NULL, _own);
        }

        for (uint32_t i = 0u; i < count; ++i)
        {
            const struct dj_option_env* const frame =
                d_option_env_registry_at(registry, i);

            for (uint32_t k = 0u;
                 (frame != NULL) && (frame->name != NULL) &&
                 (k < frame->var_count);
                 ++k)
            {
                d_internal_cli_help_setting(_json, frame->vars[k],
                                            frame->name, _own);
            }
        }
    }
#else
    (void)_own;
#endif

    d_cli_json_end(_json);

    return;
}

// d_internal_cli_help_read
//   constant: the variables the kit reads besides the settings', each with
// the system it is read on, NULL for every one, and what it does.
static const struct
{
    const char* name;
    const char* system;
    const char* summary;
} d_internal_cli_help_read[] =
{
    { "NO_COLOR", NULL,
      "set and not empty: no colour, unless --color=always asks for it" },
    { "TERM", NULL,
      "dumb: no colour, unless --color=always asks for it" },
    { "COLUMNS", NULL,
      "the width help is wrapped to where the terminal does not say, from "
      "1 to 10000" },
    { "XDG_CONFIG_HOME", "posix",
      "an absolute path: the settings file is djinterp/settings under it" },
    { "HOME", "posix",
      "where XDG_CONFIG_HOME is not one: the settings file is "
      ".config/djinterp/settings under it" },
    { "APPDATA", "windows",
      "the settings file is djinterp\\settings under it" },
    { "PATH", "posix",
      "where the program's own file is looked for when it was run by name, "
      "so that its modules are found beside it" }
};

/*
d_internal_cli_help_variables
  `variables`: each setting's, with the setting it sets, and the kit's own,
with what each does.
*/
static void
d_internal_cli_help_variables(
    struct dj_cli_json* _json
)
{
    d_cli_json_member(_json, "variables");
    d_cli_json_array(_json);

    for (size_t i = 0u;
         i < sizeof(d_internal_cli_help_read) /
             sizeof(d_internal_cli_help_read[0]);
         ++i)
    {
        d_cli_json_object(_json);
        d_cli_json_member(_json, "name");
        d_cli_json_string(_json, d_internal_cli_help_read[i].name);
        d_internal_cli_help_text(_json, "system",
                                 d_internal_cli_help_read[i].system);
        d_cli_json_member(_json, "summary");
        d_cli_json_string(_json, d_internal_cli_help_read[i].summary);
        d_cli_json_end(_json);
    }

    d_cli_json_end(_json);

    return;
}


//==============================================================================
// 4.  THE DESCRIPTION
//==============================================================================

/*
d_internal_cli_help_tail
  `prefixes`, the shared one and the program's own; `exit_statuses`, what
each status means, as the root's help says it.
*/
static void
d_internal_cli_help_tail(
    struct dj_cli_json* _json,
    const char*         _own
)
{
    d_cli_json_member(_json, "prefixes");
    d_cli_json_array(_json);
    d_cli_json_string(_json, "DJINTERP_");

    if (_own[0] != '\0')
    {
        d_cli_json_string(_json, _own);
    }

    d_cli_json_end(_json);
    d_cli_json_member(_json, "exit_statuses");
    d_cli_json_array(_json);

    for (size_t i = 0u; i < D_CLI_INTERNAL_EXIT_MEANINGS; ++i)
    {
        d_cli_json_object(_json);
        d_cli_json_member(_json, "status");
        d_cli_json_integer(_json, (int64_t)i);
        d_cli_json_member(_json, "meaning");
        d_cli_json_string(_json, d_cli_internal_exit_meanings[i]);
        d_cli_json_end(_json);
    }

    d_cli_json_end(_json);

    return;
}

/*
d_cli_help_json
  The kit's members, then the program's, from its hook.
*/
int d_cli_help_json(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
)
{
    struct dj_cli_json json;
    char               own[D_INTERNAL_CLI_HELP_ROOM];

    if ( (_program == NULL) ||
         (_program->parser == NULL) ||
         (_out == NULL) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    d_internal_cli_help_prefix(_program->name, own, sizeof(own));
    d_cli_json_init(&json, _out);
    d_cli_json_object(&json);
    d_cli_json_member(&json, "format");
    d_cli_json_string(&json, D_CLI_HELP_SHAPE);
    d_cli_json_member(&json, "program");
    d_cli_json_string(&json, _program->name);
    d_cli_json_member(&json, "version");
    d_cli_json_string(&json, _program->version);
    d_cli_json_member(&json, "command");
    d_internal_cli_help_command(&json, _program,
                                (_command != NULL)
                                ? _command
                                : &_program->parser->command);
    d_internal_cli_help_settings(&json, own);
    d_internal_cli_help_variables(&json);
    d_internal_cli_help_tail(&json, own);

    // the program's own members, a failure of its ending the description
    if ( (_program->help_json != NULL) &&
         (json.status == D_ARG_OK) )
    {
        const int status = _program->help_json(_program->help_json_context,
                                               _program, &json);

        json.status = ( (status != D_ARG_OK) &&
                        (json.status == D_ARG_OK) ) ? status : json.status;
    }

    d_cli_json_end(&json);

    return d_cli_json_finish(&json);
}

/*
d_cli_program_set_help_json
  Kept on the program, for its next description.
*/
void d_cli_program_set_help_json(
    struct dj_cli_program* _program,
    dj_cli_help_json_fn    _hook,
    void*                  _context
)
{
    if (_program == NULL)
    {
        return;
    }

    _program->help_json         = _hook;
    _program->help_json_context = (_hook != NULL) ? _context : NULL;

    return;
}
