/*******************************************************************************
* djinterp [parse]                                                  arg_option.c
*
* Options' lifetime and names, declared in arg_option.h (sections 2 and 3)
* and arg_internal.h (section 7.2): CLI11's Option in C, in four sources.
*   This one makes and destroys options and reads, writes and matches their
* names as CLI11 does. The settings are in arg_option_set.c, the relations,
* validators and callbacks in arg_option_link.c, and the values in
* arg_option_value.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_option.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free, malloc
#include <string.h>  // memchr, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text,
                                                                // d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator
#include "./arg_internal.h"  // lists, spans, failures
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// OPTION_EMPTY
//   constant: an option with every field zero, every pointer null and every
// list empty, which a new option starts as.
static const struct d_arg_option OPTION_EMPTY;

/*
d_arg_internal_option_replace_text
  Replaces an owned text field with a copy, or clears it for NULL; the old
text is freed only once the copy exists, so a refused copy changes nothing
but the parser's definition error.
*/
int
d_arg_internal_option_replace_text(
    struct d_arg_option* _option,
    char**               _field,
    const char*          _text
)
{
    char* const copy = (_text != NULL)
                       ? d_arg_internal_copy_cstr(_text)
                       : NULL;

    // check if memory allocation was successful
    if ( (_text != NULL) &&
         (!copy) )
    {
        return d_arg_internal_fail_definition(NULL,
                                              _option,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    free(*_field);
    *_field = copy;

    return D_ARG_OK;
}

//==============================================================================
// NAMES
//==============================================================================

/*
d_arg_internal_option_take_short
  The single-dash branch of CLI11's get_names: one valid character is a
short name; one invalid character is CLI11's OneCharName; several are
MissingDash unless the command allows non-standard names, when they are a
short name if valid and BadLongName -- naming them without the dash, as
CLI11 does -- if not.
*/
D_STATIC int
d_arg_internal_option_take_short(
    struct d_arg_option* _option,
    struct d_pack_text   _name,
    bool                 _non_standard
)
{
    const struct d_pack_text bare = d_arg_internal_span(_name.data + 1,
                                                        _name.length - 1u);

    // one valid character
    if ( (_name.length == 2u) &&
         (d_arg_internal_valid_first_char(_name.data[1])) )
    {
        return d_arg_internal_string_copy(&_option->short_names,
                                          bare,
                                          NULL);
    }

    // a single invalid character
    if (_name.length == 2u)
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Invalid one char name: %.*s",
                   d_arg_internal_print_length(_name),
                   _name.data);
    }

    // several characters after one dash
    if (!_non_standard)
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Long names strings require 2 dashes %.*s",
                   d_arg_internal_print_length(_name),
                   _name.data);
    }

    // a non-standard short name must still be a valid name
    if (!d_arg_internal_valid_name(bare))
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Bad long name: %.*s",
                   d_arg_internal_print_length(bare),
                   bare.data);
    }

    return d_arg_internal_string_copy(&_option->short_names,
                                      bare,
                                      NULL);
}

/*
d_arg_internal_option_take_long
  The double-dash branch of CLI11's get_names: the rest must be a valid
name, and is named without its dashes when it is not.
*/
D_STATIC int
d_arg_internal_option_take_long(
    struct d_arg_option* _option,
    struct d_pack_text   _name
)
{
    const struct d_pack_text bare = d_arg_internal_span(_name.data + 2,
                                                        _name.length - 2u);

    // the name after the dashes must be valid
    if (!d_arg_internal_valid_name(bare))
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Bad long name: %.*s",
                   d_arg_internal_print_length(bare),
                   bare.data);
    }

    return d_arg_internal_string_copy(&_option->long_names,
                                      bare,
                                      NULL);
}

/*
d_arg_internal_option_take_positional
  The last branches of CLI11's get_names: the three reserved words are
refused, then a second positional name, then an invalid one.
*/
D_STATIC int
d_arg_internal_option_take_positional(
    struct d_arg_option* _option,
    struct d_pack_text   _name
)
{
    // the reserved words
    if ( (d_arg_internal_equal_cstr(_name,
                                    "-"))  ||
         (d_arg_internal_equal_cstr(_name,
                                    "--")) ||
         (d_arg_internal_equal_cstr(_name,
                                    "++")) )
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Names '-','--','++' are reserved and not allowed as "
                   "option names %.*s",
                   d_arg_internal_print_length(_name),
                   _name.data);
    }

    // there is one positional name at most
    if (_option->positional_name != NULL)
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Only one positional name allowed, remove: %.*s",
                   d_arg_internal_print_length(_name),
                   _name.data);
    }

    // and it must be a valid name
    if (!d_arg_internal_valid_name(_name))
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_BAD_NAME,
                   "Invalid positional Name: %.*s",
                   d_arg_internal_print_length(_name),
                   _name.data);
    }

    _option->positional_name = d_arg_internal_copy_text(_name);

    return (_option->positional_name != NULL)
           ? D_ARG_OK
           : D_ARG_ERROR_OUT_OF_MEMORY;
}

/*
d_arg_internal_option_take_name
  One piece of CLI11's get_names, dispatched in CLI11's order of tests:
`-x`, then `--name`, then everything else.
*/
D_STATIC int
d_arg_internal_option_take_name(
    struct d_arg_option* _option,
    struct d_pack_text   _name,
    bool                 _non_standard
)
{
    // a single dash and no second one
    if ( (_name.length > 1u)      &&
         (_name.data[0] == '-')   &&
         (_name.data[1] != '-') )
    {
        return d_arg_internal_option_take_short(_option,
                                                _name,
                                                _non_standard);
    }

    // two dashes and more
    if ( (_name.length > 2u)      &&
         (_name.data[0] == '-')   &&
         (_name.data[1] == '-') )
    {
        return d_arg_internal_option_take_long(_option,
                                               _name);
    }

    return d_arg_internal_option_take_positional(_option,
                                                 _name);
}

/*
d_arg_internal_option_take_names
  CLI11's split_names: the text is cut at every comma, each piece trimmed,
and an empty piece -- between two commas, or the whole of an empty text --
names nothing.
*/
D_STATIC int
d_arg_internal_option_take_names(
    struct d_arg_option* _option,
    struct d_pack_text   _names,
    bool                 _non_standard
)
{
    size_t start = 0u;

    // each piece up to a comma or the end
    while (start <= _names.length)
    {
        const char* const        comma = (start < _names.length)
                                         ? (const char*)memchr(
                                               _names.data + start,
                                               ',',
                                               _names.length - start)
                                         : NULL;
        const size_t             end   = (comma != NULL)
                                         ? (size_t)(comma - _names.data)
                                         : _names.length;
        const struct d_pack_text piece = d_arg_internal_trim(
                                             d_arg_internal_span(
                                                 _names.data + start,
                                                 end - start));

        // an empty piece names nothing; the first bad one fails the option
        if (piece.length > 0u)
        {
            const int status = d_arg_internal_option_take_name(_option,
                                                               piece,
                                                               _non_standard);

            if (status != D_ARG_OK)
            {
                return status;
            }
        }

        start = end + 1u;
    }

    return D_ARG_OK;
}

/*
d_arg_internal_option_defaults
  CLI11's Option member defaults: one value per item, one item expected,
policy THROW, NORMAL priority, the configured integer rule, configurable.
*/
D_STATIC void
d_arg_internal_option_defaults(
    struct d_arg_option*  _option,
    struct d_arg_command* _command
)
{
    *_option                     = OPTION_EMPTY;
    _option->parent              = _command;
    _option->type_size_min       = 1;
    _option->type_size_max       = 1;
    _option->expected_min        = 1;
    _option->expected_max        = 1;
    _option->multi_option_policy = (int)D_ARG_MULTI_THROW;
    _option->callback_priority   = (int)D_ARG_PRIORITY_NORMAL;
    _option->integer_rule        = (int)D_INTERNAL_ARG_PARSER_INTEGER_RULE;
    _option->state               = (int)D_ARG_OPTION_STATE_PARSING;
    _option->flags               = D_ARG_OPTION_CONFIGURABLE;

    return;
}

/*
d_arg_internal_option_create
  CLI11's Option constructor, before App::add_option looks for conflicts:
the record with CLI11's member defaults, its names, its description. A bad
name has recorded its own BadNameString; any other failure is recorded
here, and a failed option is not kept.
*/
struct d_arg_option*
d_arg_internal_option_create(
    struct d_arg_command* _command,
    struct d_pack_text    _names,
    const char*           _description
)
{
    struct d_arg_option* const option = malloc(sizeof(*option));

    // check if memory allocation was successful
    if (!option)
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             D_ARG_ERROR_OUT_OF_MEMORY,
                                             NULL);

        return NULL;
    }

    d_arg_internal_option_defaults(option,
                                   _command);

    int status = d_arg_internal_option_take_names(
                     option,
                     _names,
                     d_arg_command_has_flag(_command,
                                            D_ARG_COMMAND_NON_STANDARD_NAMES));

    // the description, once the names are good
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_replace_text(option,
                                                    &option->description,
                                                    _description);
    }

    // a failure other than a bad name is recorded here
    if ( (status != D_ARG_OK) &&
         (status != D_ARG_ERROR_BAD_NAME) )
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             status,
                                             NULL);
    }

    // a failed option is not kept
    if (status != D_ARG_OK)
    {
        d_arg_internal_option_destroy(option);

        return NULL;
    }

    return option;
}

/*
d_arg_internal_option_destroy
  The parser's error reports forget the option; then its values -- their
copies live in `owned` -- the definition, the three contexts through their
release functions, and the record.
*/
void
d_arg_internal_option_destroy(
    struct d_arg_option* _option
)
{
    // nothing to free without an option
    if (!_option)
    {
        return;
    }

    d_arg_internal_unreport(d_arg_internal_parser_of(_option),
                            NULL,
                            _option);
    d_arg_option_clear(_option);
    d_arg_text_list_release(&_option->results);
    d_arg_text_list_release(&_option->processed);
    d_arg_string_list_release(&_option->owned);
    d_arg_string_list_release(&_option->short_names);
    d_arg_string_list_release(&_option->long_names);
    d_arg_string_list_release(&_option->flag_names);
    d_arg_string_list_release(&_option->flag_values);
    free(_option->positional_name);
    free(_option->env_name);
    free(_option->description);
    free(_option->default_text);
    free(_option->option_text);
    free(_option->group);
    free(_option->type_name);
    d_arg_internal_context_release(&_option->type_name_context,
                                   &_option->type_name_release);
    d_arg_internal_context_release(&_option->default_context,
                                   &_option->default_release);
    d_arg_internal_context_release(&_option->callback_context,
                                   &_option->callback_release);
    d_arg_internal_validator_list_release(&_option->validators);
    d_arg_internal_option_list_release(&_option->needs);
    d_arg_internal_option_list_release(&_option->excludes);
    free(_option);

    return;
}

// d_arg_internal_name_writer
//   struct: where d_arg_option_name's list form writes: the buffer, whether
// nothing is written yet -- so no comma is due -- and whether flag names
// carry their values.
struct d_arg_internal_name_writer
{
    struct d_arg_buffer* out;
    bool                 first;
    bool                 flag_values;
};

/*
d_arg_internal_option_append_names
  The names of one list, each with its dashes and, when asked, the value a
flag name stands for in braces, each after a comma but the first written.
*/
D_STATIC int
d_arg_internal_option_append_names(
    const struct d_arg_option*         _option,
    const struct d_arg_string_list*    _names,
    const char*                        _dashes,
    struct d_arg_internal_name_writer* _writer
)
{
    // each name of the list
    for (size_t index = 0u; index < _names->count; ++index)
    {
        const struct d_pack_text name = d_arg_internal_text_of(
                                            _names->items[index]);

        // a comma before every name but the first
        if (!_writer->first)
        {
            (void)d_arg_buffer_append_char(_writer->out,
                                           ',');
        }

        _writer->first = false;
        (void)d_arg_buffer_append_cstr(_writer->out,
                                       _dashes);
        (void)d_arg_buffer_append_text(_writer->out,
                                       name);

        // a flag name's value, in braces
        if ( (_writer->flag_values) &&
             (d_arg_option_check_flag_name(_option,
                                           name)) )
        {
            (void)d_arg_buffer_append_char(_writer->out,
                                           '{');

            const int status = d_arg_option_flag_value(
                                   _option,
                                   name,
                                   d_arg_internal_span("",
                                                       0u),
                                   _writer->out);

            // without an input a flag value cannot fail; a buffer can
            if (status != D_ARG_OK)
            {
                return status;
            }

            (void)d_arg_buffer_append_char(_writer->out,
                                           '}');
        }
    }

    return _writer->out->failed ? D_ARG_ERROR_OUT_OF_MEMORY : D_ARG_OK;
}

/*
d_arg_internal_option_name_all
  The list form of CLI11's get_name: the positional name only when asked
for or when it is the only name, and flag values only for an option that
expects no values and has flag names at all.
*/
D_STATIC int
d_arg_internal_option_name_all(
    const struct d_arg_option* _option,
    uint32_t                   _form,
    struct d_arg_buffer*       _out
)
{
    const bool                        flag_values =
        ( (d_arg_option_items_expected_min(_option) == 0) &&
          (_option->flag_names.count > 0u)                &&
          ((_form & D_ARG_NAME_NO_FLAG_VALUES) == 0u) );
    struct d_arg_internal_name_writer writer      = { _out,
                                                      true,
                                                      flag_values };

    // the positional name when asked for, or when it is the only one
    if ( ( ((_form & D_ARG_NAME_POSITIONAL) != 0u) &&
           (_option->positional_name != NULL) )        ||
         ( (_option->short_names.count == 0u) &&
           (_option->long_names.count == 0u) ) )
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       _option->positional_name);
        writer.first = false;
    }

    const int status = d_arg_internal_option_append_names(
                           _option,
                           &_option->short_names,
                           "-",
                           &writer);

    return (status == D_ARG_OK)
           ? d_arg_internal_option_append_names(_option,
                                                &_option->long_names,
                                                "--",
                                                &writer)
           : status;
}

/*
d_arg_option_name
  CLI11's get_name: hidden options have no name; the list form is its own
function; the single forms prefer the positional name when asked, then a
long name, then a short one, then the positional name again.
*/
int
d_arg_option_name(
    const struct d_arg_option* _option,
    uint32_t                   _form,
    struct d_arg_buffer*       _out
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // an option in the empty group is hidden, and has no name
    if ( (_option->group != NULL) &&
         (_option->group[0] == '\0') )
    {
        return D_ARG_OK;
    }

    // every name, comma-separated
    if ((_form & D_ARG_NAME_ALL) != 0u)
    {
        return d_arg_internal_option_name_all(_option,
                                              _form,
                                              _out);
    }

    // the positional name when asked; else long, short, positional
    if ((_form & D_ARG_NAME_POSITIONAL) != 0u)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       _option->positional_name);
    }
    else if (_option->long_names.count > 0u)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "--");
        (void)d_arg_buffer_append_cstr(_out,
                                       _option->long_names.items[0]);
    }
    else if (_option->short_names.count > 0u)
    {
        (void)d_arg_buffer_append_char(_out,
                                       '-');
        (void)d_arg_buffer_append_cstr(_out,
                                       _option->short_names.items[0]);
    }
    else
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       _option->positional_name);
    }

    return _out->failed ? D_ARG_ERROR_OUT_OF_MEMORY : D_ARG_OK;
}

/*
d_arg_option_single_name
  The first of the four kinds of name the option has.
*/
const char*
d_arg_option_single_name(
    const struct d_arg_option* _option
)
{
    // no option has no name
    if (!_option)
    {
        return "";
    }

    // long, short, positional, environment, in that order
    if (_option->long_names.count > 0u)
    {
        return _option->long_names.items[0];
    }

    if (_option->short_names.count > 0u)
    {
        return _option->short_names.items[0];
    }

    if (_option->positional_name != NULL)
    {
        return _option->positional_name;
    }

    return (_option->env_name != NULL) ? _option->env_name : "";
}

/*
d_arg_option_check_name
  CLI11's check_name: the dashes choose the list; a bare name is compared
with the positional name under both switches, and then, exactly, with the
environment variable's name -- which is asked only when the positional
name did not match.
*/
bool
d_arg_option_check_name(
    const struct d_arg_option* _option,
    struct d_pack_text         _name
)
{
    // no option matches anything
    if (!_option)
    {
        return false;
    }

    // `--name`
    if ( (_name.length > 2u)     &&
         (_name.data[0] == '-')  &&
         (_name.data[1] == '-') )
    {
        return d_arg_option_check_long_name(
                   _option,
                   d_arg_internal_span(_name.data + 2,
                                       _name.length - 2u));
    }

    // `-x`
    if ( (_name.length > 1u) &&
         (_name.data[0] == '-') )
    {
        return d_arg_option_check_short_name(
                   _option,
                   d_arg_internal_span(_name.data + 1,
                                       _name.length - 1u));
    }

    // the positional name, under the option's switches
    if ( (_option->positional_name != NULL) &&
         (d_arg_internal_names_match(
              _name,
              d_arg_internal_text_of(_option->positional_name),
              d_arg_option_has_flag(_option,
                                    D_ARG_OPTION_IGNORE_CASE),
              d_arg_option_has_flag(_option,
                                    D_ARG_OPTION_IGNORE_UNDERSCORE))) )
    {
        return true;
    }

    // the environment variable's name, exactly
    return ( (_option->env_name != NULL) &&
             (d_arg_internal_equal(_name,
                                   d_arg_internal_text_of(
                                       _option->env_name))) );
}

/*
d_arg_option_check_short_name
  CLI11's check_sname, which ignores case when asked but never underscores.
*/
bool
d_arg_option_check_short_name(
    const struct d_arg_option* _option,
    struct d_pack_text         _name
)
{
    return ( (_option != NULL) &&
             (d_arg_internal_find_member(
                  _name,
                  &_option->short_names,
                  d_arg_option_has_flag(_option,
                                        D_ARG_OPTION_IGNORE_CASE),
                  false) >= 0L) );
}

/*
d_arg_option_check_long_name
  CLI11's check_lname, under both switches.
*/
bool
d_arg_option_check_long_name(
    const struct d_arg_option* _option,
    struct d_pack_text         _name
)
{
    return ( (_option != NULL) &&
             (d_arg_internal_find_member(
                  _name,
                  &_option->long_names,
                  d_arg_option_has_flag(_option,
                                        D_ARG_OPTION_IGNORE_CASE),
                  d_arg_option_has_flag(_option,
                                        D_ARG_OPTION_IGNORE_UNDERSCORE))
              >= 0L) );
}

/*
d_arg_option_check_flag_name
  CLI11's check_fname, under both switches.
*/
bool
d_arg_option_check_flag_name(
    const struct d_arg_option* _option,
    struct d_pack_text         _name
)
{
    return ( (_option != NULL) &&
             (d_arg_internal_find_member(
                  _name,
                  &_option->flag_names,
                  d_arg_option_has_flag(_option,
                                        D_ARG_OPTION_IGNORE_CASE),
                  d_arg_option_has_flag(_option,
                                        D_ARG_OPTION_IGNORE_UNDERSCORE))
              >= 0L) );
}

/*
d_arg_internal_option_matching_dashed
  The first two loops of CLI11's matching_name: this option's short names
against the other's short names -- and long names, when both are
configurable -- then its long names against the other's long names, and a
one-character long name against its short names.
*/
D_STATIC const char*
d_arg_internal_option_matching_dashed(
    const struct d_arg_option* _option,
    const struct d_arg_option* _other,
    bool                       _both_configurable
)
{
    // short names
    for (size_t index = 0u; index < _option->short_names.count; ++index)
    {
        const char* const        name = _option->short_names.items[index];
        const struct d_pack_text text = d_arg_internal_text_of(name);

        if ( (d_arg_option_check_short_name(_other,
                                            text)) ||
             ( (_both_configurable) &&
               (d_arg_option_check_long_name(_other,
                                             text)) ) )
        {
            return name;
        }
    }

    // long names
    for (size_t index = 0u; index < _option->long_names.count; ++index)
    {
        const char* const        name = _option->long_names.items[index];
        const struct d_pack_text text = d_arg_internal_text_of(name);

        if ( (d_arg_option_check_long_name(_other,
                                           text)) ||
             ( (text.length == 1u)      &&
               (_both_configurable)     &&
               (d_arg_option_check_short_name(_other,
                                              text)) ) )
        {
            return name;
        }
    }

    return NULL;
}

/*
d_arg_internal_option_matching_positional
  CLI11's two positional tests: a configurable option known only by its
positional name collides with any name of the other's, and the other's
positional name -- when it has no short or flag names, as CLI11 tests --
with any of this option's.
*/
D_STATIC const char*
d_arg_internal_option_matching_positional(
    const struct d_arg_option* _option,
    const struct d_arg_option* _other
)
{
    const struct d_pack_text mine   = d_arg_internal_text_of(
                                          _option->positional_name);
    const struct d_pack_text theirs = d_arg_internal_text_of(
                                          _other->positional_name);

    // this option has only its positional name
    if ( (_option->short_names.count == 0u) &&
         (_option->long_names.count == 0u)  &&
         (_option->positional_name != NULL) )
    {
        if ( (d_arg_option_check_short_name(_other,
                                            mine)) ||
             (d_arg_option_check_long_name(_other,
                                           mine))  ||
             (d_arg_internal_equal(mine,
                                   theirs)) )
        {
            return _option->positional_name;
        }
    }

    // the other has only its positional name, as CLI11 judges it
    if ( (_other->short_names.count == 0u) &&
         (_other->flag_names.count == 0u)  &&
         (_other->positional_name != NULL) )
    {
        if ( (d_arg_option_check_short_name(_option,
                                            theirs)) ||
             (d_arg_option_check_long_name(_option,
                                           theirs))  ||
             (d_arg_internal_equal(mine,
                                   theirs)) )
        {
            return _other->positional_name;
        }
    }

    return NULL;
}

/*
d_arg_option_matching_name
  CLI11's matching_name, test for test, and its last loop: an option that
ignores case or underscores also looks for the other's names among its
own, since the other's tests would not fold them.
*/
const char*
d_arg_option_matching_name(
    const struct d_arg_option* _option,
    const struct d_arg_option* _other
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_other) )
    {
        return NULL;
    }

    const bool  both  = ( (d_arg_option_has_flag(_option,
                                                 D_ARG_OPTION_CONFIGURABLE)) &&
                          (d_arg_option_has_flag(_other,
                                                 D_ARG_OPTION_CONFIGURABLE)) );
    const char* match = d_arg_internal_option_matching_dashed(_option,
                                                              _other,
                                                              both);

    // then the positional names, when both are configurable
    if ( (match == NULL) &&
         (both) )
    {
        match = d_arg_internal_option_matching_positional(_option,
                                                          _other);
    }

    // the reverse search, when this option folds its names
    if ( (match == NULL) &&
         ( (d_arg_option_has_flag(_option,
                                  D_ARG_OPTION_IGNORE_CASE)) ||
           (d_arg_option_has_flag(_option,
                                  D_ARG_OPTION_IGNORE_UNDERSCORE)) ) )
    {
        // the other's short names, then its long names
        for (size_t index = 0u; index < _other->short_names.count; ++index)
        {
            if (d_arg_option_check_short_name(
                    _option,
                    d_arg_internal_text_of(_other->short_names.items[index])))
            {
                return _other->short_names.items[index];
            }
        }

        for (size_t index = 0u; index < _other->long_names.count; ++index)
        {
            if (d_arg_option_check_long_name(
                    _option,
                    d_arg_internal_text_of(_other->long_names.items[index])))
            {
                return _other->long_names.items[index];
            }
        }
    }

    return match;
}
