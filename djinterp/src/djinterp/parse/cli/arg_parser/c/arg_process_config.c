/*******************************************************************************
* djinterp [parse]                                          arg_process_config.c
*
* Configuration in the parse, declared in arg_internal.h (8.2.4 to 8.2.6).
*   CLI11's _add_flag_like_result, in every build, as a positional that
* expects no values takes its argument through it as a configuration item
* does; then, with configuration files built in, its _parse_config and
* _parse_single_config, each item handed to the option it names in the
* command its sections name; and the configuration option's processing --
* the files it names read in the order its policy leaves them, an
* environment variable's when none is given, its default when forced.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_process_config.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, lists
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // items, formats
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_flag_value_intmax
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, intmax_t


//==============================================================================
// 1.  FLAG-LIKE RESULTS
//==============================================================================

/*
d_arg_internal_process_flag_choice
  The text CLI11's _add_flag_like_result hands an option for an item with
one input or none, read by the format as a flag: for an option refusing
flag overrides, given what reads as 1, the item name's own value; else the
value the name gives the input -- but a `{}` stays as it is for an option
that takes several values.
*/
D_STATIC int
d_arg_internal_process_flag_choice(
    const struct d_arg_option* _option,
    struct d_pack_text         _name,
    struct d_pack_text         _read,
    struct d_arg_buffer*       _out
)
{
    intmax_t flag = 0;

    // the name's own value, for an override refused but meaning yes
    if ( (d_arg_option_has_flag(_option,
                                D_ARG_OPTION_DISABLE_FLAG_OVERRIDE)) &&
         (d_arg_flag_value_intmax(_read,
                                  _option->integer_rule,
                                  &flag) == D_ARG_OK)               &&
         (flag == 1) )
    {
        return d_arg_option_flag_value(_option,
                                       _name,
                                       d_arg_internal_span("{}",
                                                           2u),
                                       _out);
    }

    // `{}` kept for an option taking several values
    if ( (d_arg_internal_equal_cstr(_read,
                                    "{}")) &&
         (_option->expected_max > 1) )
    {
        return d_arg_buffer_append_text(_out,
                                        _read);
    }

    return d_arg_option_flag_value(_option,
                                   _name,
                                   _read,
                                   _out);
}

/*
d_arg_internal_process_flag_one
  CLI11's _add_flag_like_result for an item with one input or none: the
item read as a flag through the command's format, the text chosen for it
added as the option's value, which then owns it.
*/
D_STATIC int
d_arg_internal_process_flag_one(
    const struct d_arg_command*     _command,
    struct d_arg_option*            _option,
    const struct d_arg_config_item* _item
)
{
    struct d_arg_buffer read   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer value  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_config_to_flag(
                                     d_arg_command_config_format(_command),
                                     _item,
                                     &read);

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_flag_choice(_option,
                                                  _item->name,
                                                  d_arg_buffer_text(&read),
                                                  &value)
             : status;

    // the value, or a failure no one recorded
    if (status == D_ARG_OK)
    {
        status         = d_arg_internal_option_add_built(_option,
                                                         &_option->results,
                                                         &value,
                                                         NULL);
        _option->state = (int)D_ARG_OPTION_STATE_PARSING;
    }
    else if (status != D_ARG_ERROR_ARGUMENT_MISMATCH)
    {
        status = d_arg_internal_fail(_command,
                                     _option,
                                     status,
                                     NULL);
    }

    d_arg_buffer_release(&read);
    d_arg_buffer_release(&value);

    return status;
}

/*
d_arg_internal_process_flag_listed
  Whether an input is one an option refusing flag overrides takes from a
list: one of its flag names' values, or, without any, one of the four
words CLI11 allows.
*/
D_STATIC bool
d_arg_internal_process_flag_listed(
    const struct d_arg_option* _option,
    struct d_pack_text         _input
)
{
    // the four words, for an option without flag values
    if (_option->flag_values.count == 0u)
    {
        return ( (d_arg_internal_equal_cstr(_input,
                                            "true"))  ||
                 (d_arg_internal_equal_cstr(_input,
                                            "false")) ||
                 (d_arg_internal_equal_cstr(_input,
                                            "1"))     ||
                 (d_arg_internal_equal_cstr(_input,
                                            "0")) );
    }

    // one of its flag values
    for (size_t index = 0u; index < _option->flag_values.count; ++index)
    {
        if (d_arg_internal_equal_cstr(_input,
                                      _option->flag_values.items[index]))
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_process_flag_refusal
  CLI11's errors for an item with more inputs than a flag-like option
takes, naming the item: an AtMost mismatch for an option taking several,
or a TooManyInputsFlag conversion error for one that allows overrides.
*/
D_STATIC int
d_arg_internal_process_flag_refusal(
    const struct d_arg_option*      _option,
    const struct d_arg_config_item* _item,
    size_t                          _count
)
{
    const int           maximum = d_arg_option_items_expected_max(_option);
    struct d_arg_buffer name    = D_ARG_BUFFER_INIT;
    int                 status  = d_arg_config_item_fullname(_item,
                                                             &name);
    const char* const   text    = (name.data != NULL) ? name.data : "";

    // the item's name, then the error
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_fail(NULL,
                                     _option,
                                     status,
                                     NULL);
    }
    else if (maximum > 1)
    {
        status = d_arg_internal_fail(NULL,
                                     _option,
                                     D_ARG_ERROR_ARGUMENT_MISMATCH,
                                     "%s: At most %d required but received "
                                     "%zu",
                                     text,
                                     maximum,
                                     _count);
    }
    else
    {
        status = d_arg_internal_fail(NULL,
                                     _option,
                                     D_ARG_ERROR_CONVERSION,
                                     "%s: too many inputs for a flag",
                                     text);
    }

    d_arg_buffer_release(&name);

    return status;
}

/*
d_arg_internal_process_flag_several
  CLI11's _add_flag_like_result for an item with several inputs: within
what the option takes -- or under a policy that takes or joins them all --
they are left for the caller; beyond it they are refused, unless the
option refuses flag overrides, when each is added that is one of its
values, and any other is CLI11's InvalidError -- in its words, which its
one-argument constructor, meant for a name, ends with a sentence about
positionals that do not come into it.
*/
D_STATIC int
d_arg_internal_process_flag_several(
    struct d_arg_option*            _option,
    const struct d_arg_config_item* _item,
    const struct d_arg_text_list*   _inputs,
    bool*                           _handled
)
{
    const int policy = _option->multi_option_policy;
    int       status = D_ARG_OK;

    // within what the option takes
    if ( ( (int)_inputs->count <=
           d_arg_option_items_expected_max(_option) ) ||
         (policy == (int)D_ARG_MULTI_TAKE_ALL)        ||
         (policy == (int)D_ARG_MULTI_JOIN) )
    {
        *_handled = false;

        return D_ARG_OK;
    }

    *_handled = true;

    // too many, for an option taking several or allowing overrides
    if ( (d_arg_option_items_expected_max(_option) > 1)                 ||
         (!d_arg_option_has_flag(_option,
                                 D_ARG_OPTION_DISABLE_FLAG_OVERRIDE)) )
    {
        return d_arg_internal_process_flag_refusal(_option,
                                                   _item,
                                                   _inputs->count);
    }

    // each input, which must be one of its values
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _inputs->count);
         ++index)
    {
        const struct d_pack_text input = _inputs->items[index];

        status = d_arg_internal_process_flag_listed(_option,
                                                    input)
                 ? d_arg_option_add_result(_option,
                                           input,
                                           NULL)
                 : d_arg_internal_fail(NULL,
                                       _option,
                                       D_ARG_ERROR_INVALID,
                                       "invalid flag argument given: Too "
                                       "many positional arguments with "
                                       "unlimited expected args");
    }

    return status;
}

/*
d_arg_internal_add_flag_like_result
  CLI11's _add_flag_like_result: an item with one input or none is a
flag's -- counted by the item's own inputs, whatever was filtered out of
`_inputs` -- and one with several is checked against what the option
takes.
*/
int
d_arg_internal_add_flag_like_result(
    const struct d_arg_command*     _command,
    struct d_arg_option*            _option,
    const struct d_arg_config_item* _item,
    const struct d_arg_text_list*   _inputs,
    bool*                           _handled
)
{
    // one input or none: a flag
    if (_item->inputs.count <= 1u)
    {
        *_handled = true;

        return d_arg_internal_process_flag_one(_command,
                                               _option,
                                               _item);
    }

    return d_arg_internal_process_flag_several(_option,
                                               _item,
                                               _inputs,
                                               _handled);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 2.  CONFIGURATION ITEMS
//==============================================================================

/*
d_arg_internal_process_lookup
  CLI11's get_option_no_throw of `_prefix` and an item's name: the option
answering to `--name`, `-n` or `name`, in the command, its option groups
and, with fallthrough, its parent.
*/
D_STATIC int
d_arg_internal_process_lookup(
    const struct d_arg_command* _command,
    const char*                 _prefix,
    struct d_pack_text          _name,
    struct d_arg_option**       _option
)
{
    struct d_arg_buffer text = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&text,
                                   _prefix);
    (void)d_arg_buffer_append_text(&text,
                                   _name);

    *_option = (text.failed == 0)
               ? d_arg_internal_find_option(_command,
                                            d_arg_buffer_text(&text))
               : NULL;

    const int status = (text.failed == 0)
                       ? D_ARG_OK
                       : d_arg_internal_parse_stored(
                             _command,
                             D_ARG_ERROR_OUT_OF_MEMORY);

    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_internal_process_configurable
  The first option CLI11's get_options(filter) lists that is configurable
and answers to the name as a positional name, a long name or a short one:
the command's own, then -- in turn -- those of its nameless subcommands
and of those whose group starts with `+`, then, for a named command that
falls through, its fallthrough parent's.
*/
D_STATIC struct d_arg_option*
d_arg_internal_process_configurable(
    const struct d_arg_command* _command,
    struct d_pack_text          _name
)
{
    // the command's own
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        struct d_arg_option* const option = _command->options.items[index];

        if ( (d_arg_option_has_flag(option,
                                    D_ARG_OPTION_CONFIGURABLE))   &&
             ( (d_arg_option_check_name(option,
                                        _name))                  ||
               (d_arg_option_check_long_name(option,
                                             _name))             ||
               (d_arg_option_check_short_name(option,
                                              _name)) ) )
        {
            return option;
        }
    }

    // the nameless subcommands, and those grouped with `+`
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* const sub =
            _command->subcommands.items[index];
        struct d_arg_option* const        found =
            ( (d_arg_command_name(sub)[0] == '\0') ||
              (d_arg_command_group(sub)[0] == '+') )
            ? d_arg_internal_process_configurable(sub,
                                                  _name)
            : NULL;

        if (found != NULL)
        {
            return found;
        }
    }

    return ( (d_arg_command_has_flag(_command,
                                     D_ARG_COMMAND_FALLTHROUGH)) &&
             (_command->parent != NULL)                          &&
             (d_arg_command_name(_command)[0] != '\0') )
           ? d_arg_internal_process_configurable(
                 d_arg_internal_fallthrough_parent(_command),
                 _name)
           : NULL;
}

/*
d_arg_internal_process_short_first
  CLI11's second look for an item with a one-character name: `-n`, when
`--n` found nothing, or found one that is not configurable and a
configurable `-n` exists.
*/
D_STATIC int
d_arg_internal_process_short_first(
    const struct d_arg_command*     _command,
    const struct d_arg_config_item* _item,
    struct d_arg_option**           _option
)
{
    struct d_arg_option* found  = NULL;
    const bool           single = (_item->name.length == 1u);
    int                  status = (single)
                                  ? d_arg_internal_process_lookup(
                                        _command,
                                        "-",
                                        _item->name,
                                        &found)
                                  : D_ARG_OK;

    // nothing yet: the short name's, else the bare name's
    if ( (status == D_ARG_OK) &&
         (!(*_option)) )
    {
        *_option = found;
        status   = (!found)
                   ? d_arg_internal_process_lookup(_command,
                                                   "",
                                                   _item->name,
                                                   _option)
                   : D_ARG_OK;
    }
    else if ( (found != NULL)                                     &&
              (d_arg_option_has_flag(found,
                                     D_ARG_OPTION_CONFIGURABLE)) )
    {
        *_option = found;
    }

    return status;
}

/*
d_arg_internal_process_item_option
  CLI11's search in _parse_single_config for the option an item names:
`--name`, then the short and bare names as d_arg_internal_process_
short_first looks for them; and, when that finds nothing configurable, the
first configurable option answering to the name in any form.
*/
D_STATIC int
d_arg_internal_process_item_option(
    const struct d_arg_command*     _command,
    const struct d_arg_config_item* _item,
    struct d_arg_option**           _option
)
{
    int status = d_arg_internal_process_lookup(_command,
                                               "--",
                                               _item->name,
                                               _option);

    // a long option not configurable gives way to a short one
    if ( (status == D_ARG_OK) &&
         ( (!(*_option)) ||
           (!d_arg_option_has_flag(*_option,
                                   D_ARG_OPTION_CONFIGURABLE)) ) )
    {
        status = d_arg_internal_process_short_first(_command,
                                                    _item,
                                                    _option);
    }

    // nothing configurable yet: any configurable option of the name
    if ( (status == D_ARG_OK)                                       &&
         ( (!(*_option)) ||
           (!d_arg_option_has_flag(*_option,
                                   D_ARG_OPTION_CONFIGURABLE)) ) )
    {
        struct d_arg_option* const any = d_arg_internal_process_configurable(
                                             _command,
                                             _item->name);

        *_option = (any != NULL) ? any : *_option;
    }

    return status;
}

/*
d_arg_internal_process_keep_copy
  A copy of text the parser's pool keeps until the next parse, kept among
a command's unused arguments as a positional.
*/
D_STATIC int
d_arg_internal_process_keep_copy(
    struct d_arg_command* _command,
    struct d_pack_text    _text
)
{
    const char* copy   = NULL;
    int         status = d_arg_internal_string_copy(&_command->parser->pool,
                                                    _text,
                                                    &copy);

    status = (status == D_ARG_OK)
             ? d_arg_internal_extra_push(&_command->missing,
                                         d_arg_internal_span(copy,
                                                             _text.length),
                                         (int)D_ARG_CLASS_NONE)
             : status;

    return d_arg_internal_parse_stored(_command,
                                       status);
}

/*
d_arg_internal_process_capture_item
  An item no option takes, kept among the command's unused arguments when
configuration extras are captured: its full name, then each input, as
positionals.
*/
D_STATIC int
d_arg_internal_process_capture_item(
    struct d_arg_command*           _command,
    const struct d_arg_config_item* _item
)
{
    struct d_arg_buffer name   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_config_item_fullname(_item,
                                                            &name);

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_keep_copy(_command,
                                                d_arg_buffer_text(&name))
             : d_arg_internal_parse_stored(_command,
                                           status);
    d_arg_buffer_release(&name);

    // each input after it
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _item->inputs.count);
         ++index)
    {
        status = d_arg_internal_process_keep_copy(_command,
                                                  _item->inputs.items[index]);
    }

    return status;
}

/*
d_arg_internal_process_item_error
  CLI11's ConfigError for an item, naming it: its Extras, "INI was not
able to parse <name>", or its NotConfigurable, "<name>: This option is not
allowed in a configuration file".
*/
D_STATIC int
d_arg_internal_process_item_error(
    const struct d_arg_command*     _command,
    const struct d_arg_config_item* _item,
    bool                            _configurable
)
{
    struct d_arg_buffer name   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_config_item_fullname(_item,
                                                            &name);
    const char* const   text   = (name.data != NULL) ? name.data : "";

    // the item's name, then the error
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_parse_stored(_command,
                                             status);
    }
    else
    {
        status = (_configurable)
                 ? d_arg_internal_fail(_command,
                                       NULL,
                                       D_ARG_ERROR_CONFIG,
                                       "INI was not able to parse %s",
                                       text)
                 : d_arg_internal_fail(_command,
                                       NULL,
                                       D_ARG_ERROR_CONFIG,
                                       "%s: This option is not allowed in "
                                       "a configuration file",
                                       text);
    }

    d_arg_buffer_release(&name);

    return status;
}

/*
d_arg_internal_process_section
  CLI11's section marks in configuration, for a configurable command: `++`
opens its section -- a use counted, its pre-parse step with 2 left, listed
among its parent's parsed subcommands -- and `--` closes it, running its
completion callbacks, without the help flags, when it has a parse-complete
callback.
*/
D_STATIC int
d_arg_internal_process_section(
    struct d_arg_command* _command,
    bool                  _open
)
{
    int status = D_ARG_OK;

    // a command that is not configurable takes no sections
    if (!d_arg_command_has_flag(_command,
                                D_ARG_COMMAND_CONFIGURABLE))
    {
        return D_ARG_OK;
    }

    // opened: counted, pre-parsed, listed
    if (_open)
    {
        d_arg_internal_increment_parsed(_command);
        status = d_arg_internal_trigger_pre_parse(_command,
                                                  2u);

        return ( (status == D_ARG_OK) &&
                 (_command->parent != NULL) )
               ? d_arg_internal_parse_stored(
                     _command,
                     d_arg_internal_command_push(
                         &_command->parent->parsed_subcommands,
                         _command))
               : status;
    }

    return (_command->parse_complete_fn != NULL)
           ? d_arg_internal_process_completion_callbacks(_command,
                                                         false)
           : D_ARG_OK;
}

/*
d_arg_internal_process_filtered
  A multiline item's inputs without its `%%` separators, viewed in `_out`,
for an option that does not inject separators of its own.
*/
D_STATIC int
d_arg_internal_process_filtered(
    const struct d_arg_config_item* _item,
    struct d_arg_text_list*         _out
)
{
    int status = D_ARG_OK;

    // each input but the separators
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _item->inputs.count);
         ++index)
    {
        const struct d_pack_text input = _item->inputs.items[index];

        status = (!d_arg_internal_equal_cstr(input,
                                             "%%"))
                 ? d_arg_internal_text_push(_out,
                                            input)
                 : D_ARG_OK;
    }

    return status;
}

/*
d_arg_internal_process_item_inputs
  The inputs an item gives an option that has no value yet: taken as a
flag's by an option that expects no values; else the option, PARSING, given
each as CLI11's add_result(vector) gives them, and its callback run.
*/
D_STATIC int
d_arg_internal_process_item_inputs(
    const struct d_arg_command*     _command,
    struct d_arg_option*            _option,
    const struct d_arg_config_item* _item,
    const struct d_arg_text_list*   _inputs
)
{
    bool handled = false;
    int  status  = D_ARG_OK;

    // an option expecting no values takes them as a flag's
    if (_option->expected_min == 0)
    {
        status = d_arg_internal_add_flag_like_result(_command,
                                                     _option,
                                                     _item,
                                                     _inputs,
                                                     &handled);
    }

    // a flag's result, or its failure, ends the step
    if ( (status != D_ARG_OK) ||
         (handled) )
    {
        return status;
    }

    _option->state = (int)D_ARG_OPTION_STATE_PARSING;

    // each added as given
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _inputs->count);
         ++index)
    {
        status = d_arg_option_add_result(_option,
                                         _inputs->items[index],
                                         NULL);
    }

    return (status == D_ARG_OK)
           ? d_arg_option_run_callback(_option)
           : status;
}

/*
d_arg_internal_process_item_values
  CLI11's step for an option an item names that has no value yet: the
item's inputs, a multiline item's without its separators unless the option
injects its own.
*/
D_STATIC int
d_arg_internal_process_item_values(
    const struct d_arg_command*     _command,
    struct d_arg_option*            _option,
    const struct d_arg_config_item* _item
)
{
    // the inputs as they are
    if ( (!_item->multiline) ||
         (d_arg_option_has_flag(_option,
                                D_ARG_OPTION_INJECT_SEPARATOR)) )
    {
        return d_arg_internal_process_item_inputs(_command,
                                                  _option,
                                                  _item,
                                                  &_item->inputs);
    }

    struct d_arg_text_list filtered = D_ARG_LIST_INIT;
    int                    status   =
        d_arg_internal_parse_stored(
            _command,
            d_arg_internal_process_filtered(_item,
                                            &filtered));

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_item_inputs(_command,
                                                  _option,
                                                  _item,
                                                  &filtered)
             : status;
    d_arg_text_list_release(&filtered);

    return status;
}

/*
d_arg_internal_process_item_to
  The end of CLI11's _parse_single_config: the option an item names takes
it -- kept unused when none does and configuration extras are captured;
refused when the option is not configurable, unless every unknown is
ignored; and taken only by an option the command line left without a
value. `_handled` receives CLI11's result.
*/
D_STATIC int
d_arg_internal_process_item_to(
    struct d_arg_command*           _command,
    const struct d_arg_config_item* _item,
    bool*                           _handled
)
{
    struct d_arg_option* option = NULL;
    const int            mode   = _command->allow_config_extras;
    const int            status = d_arg_internal_process_item_option(_command,
                                                                     _item,
                                                                     &option);

    *_handled = ( (option != NULL)                                     &&
                  (d_arg_option_has_flag(option,
                                         D_ARG_OPTION_CONFIGURABLE)) );

    // the search failed
    if (status != D_ARG_OK)
    {
        return status;
    }

    // no option: kept unused, where captured
    if (!option)
    {
        return (mode == (int)D_ARG_CONFIG_EXTRAS_CAPTURE)
               ? d_arg_internal_process_capture_item(_command,
                                                     _item)
               : D_ARG_OK;
    }

    // an option the file may not set
    if (!(*_handled))
    {
        return (mode == (int)D_ARG_CONFIG_EXTRAS_IGNORE_ALL)
               ? D_ARG_OK
               : d_arg_internal_process_item_error(_command,
                                                   _item,
                                                   false);
    }

    return (option->results.count == 0u)
           ? d_arg_internal_process_item_values(_command,
                                                option,
                                                _item)
           : D_ARG_OK;
}

/*
d_arg_internal_process_item
  CLI11's _parse_single_config: an item in a section is handed down to the
subcommand its next section names -- not handled when none does; the
section marks open and close a configurable command's section; any other
item goes to the option it names.
*/
D_STATIC int
d_arg_internal_process_item(
    struct d_arg_command*           _command,
    const struct d_arg_config_item* _item,
    size_t                          _level,
    bool*                           _handled
)
{
    const struct d_pack_text name = _item->name;

    // an item in a section, for the subcommand the section names
    if (_level < _item->parents.count)
    {
        struct d_arg_command* const sub = d_arg_internal_find_subcommand(
                                              _command,
                                              _item->parents.items[_level],
                                              false,
                                              false);

        *_handled = false;

        return (sub != NULL)
               ? d_arg_internal_process_item(sub,
                                             _item,
                                             _level + 1u,
                                             _handled)
               : D_ARG_OK;
    }

    // a section opened, or closed
    if ( (d_arg_internal_equal_cstr(name,
                                    "++")) ||
         (d_arg_internal_equal_cstr(name,
                                    "--")) )
    {
        *_handled = true;

        return d_arg_internal_process_section(
                   _command,
                   d_arg_internal_equal_cstr(name,
                                             "++"));
    }

    return d_arg_internal_process_item_to(_command,
                                          _item,
                                          _handled);
}

/*
d_arg_internal_parse_config
  CLI11's _parse_config: each item in turn, one that nothing took refused
-- CLI11's ConfigError::Extras -- when configuration extras are errors.
*/
int
d_arg_internal_parse_config(
    struct d_arg_command*                _command,
    const struct d_arg_config_item_list* _items
)
{
    int status = D_ARG_OK;

    // each item in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _items->count);
         ++index)
    {
        bool handled = false;

        status = d_arg_internal_process_item(_command,
                                             &_items->items[index],
                                             0u,
                                             &handled);

        if ( (status == D_ARG_OK)                                       &&
             (!handled)                                                 &&
             (_command->allow_config_extras ==
              (int)D_ARG_CONFIG_EXTRAS_ERROR) )
        {
            status = d_arg_internal_process_item_error(
                         _command,
                         &_items->items[index],
                         true);
        }
    }

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 3.  CONFIGURATION FILES
//==============================================================================

/*
d_arg_internal_process_file_missing
  CLI11's FileError::Missing: "<path> was not readable (missing?)".
*/
D_STATIC int
d_arg_internal_process_file_missing(
    const struct d_arg_command* _command,
    struct d_pack_text          _path
)
{
    return d_arg_internal_fail(_command,
                               NULL,
                               D_ARG_ERROR_FILE,
                               "%.*s was not readable (missing?)",
                               d_arg_internal_print_length(_path),
                               _path.data);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_arg_internal_process_config_file
  CLI11's _process_config_file(file, throw_error): a file that is there is
read through the command's format and its items parsed, `_used` then set;
one that is not, or cannot be opened, is an error when it must be read and
is skipped otherwise; any other failure of the reading stands.
*/
D_STATIC int
d_arg_internal_process_config_file(
    struct d_arg_command* _command,
    struct d_pack_text    _path,
    bool                  _must,
    bool*                 _used
)
{
    // not a file
    if (d_arg_internal_path_kind(_path.data) != 1)
    {
        return (_must)
               ? d_arg_internal_process_file_missing(_command,
                                                     _path)
               : D_ARG_OK;
    }

    struct d_arg_config_item_list items  = D_ARG_LIST_INIT;
    struct d_arg_buffer           error  = D_ARG_BUFFER_INIT;
    int                           status = d_arg_config_from_file(
                                               d_arg_command_config_format(
                                                   _command),
                                               _path.data,
                                               &items,
                                               &error);

    // read, then parsed into the options; or skipped, or refused
    if (status == D_ARG_OK)
    {
        *_used = true;
        status = d_arg_internal_parse_config(_command,
                                             &items);
    }
    else if ( (status == D_ARG_ERROR_FILE) &&
              (!_must) )
    {
        status = D_ARG_OK;
    }
    else
    {
        status = d_arg_internal_fail(_command,
                                     NULL,
                                     status,
                                     (error.length > 0u) ? "%s" : NULL,
                                     error.data);
    }

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&error);

    return status;
}

#else

/*
d_arg_internal_process_config_file
  Without configuration files built in, a file that is there is refused
with D_ARG_ERROR_CONFIG; one that is not is missing, or skipped, as CLI11
has it.
*/
D_STATIC int
d_arg_internal_process_config_file(
    struct d_arg_command* _command,
    struct d_pack_text    _path,
    bool                  _must,
    bool*                 _used
)
{
    (void)_used;

    // not a file
    if (d_arg_internal_path_kind(_path.data) != 1)
    {
        return (_must)
               ? d_arg_internal_process_file_missing(_command,
                                                     _path)
               : D_ARG_OK;
    }

    return d_arg_internal_fail(_command,
                               NULL,
                               D_ARG_ERROR_CONFIG,
                               "%.*s: configuration files are not built in",
                               d_arg_internal_print_length(_path),
                               _path.data);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

/*
d_arg_internal_process_config_env
  CLI11's fallback for a configuration option not given: its environment
variable's value, when the option has one and it is set, added as given --
without the validation an ordinary option's value from the environment
goes through.
*/
D_STATIC int
d_arg_internal_process_config_env(
    struct d_arg_command* _command,
    struct d_arg_option*  _config
)
{
    // without a variable
    if ( (!_config->env_name) ||
         (_config->env_name[0] == '\0') )
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer value  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_parse_stored(
                                     _command,
                                     d_arg_internal_getenv(_config->env_name,
                                                           &value));

    status = ( (status == D_ARG_OK) &&
               (value.length > 0u) )
             ? d_arg_option_add_result(_config,
                                       d_arg_buffer_text(&value),
                                       NULL)
             : status;
    d_arg_buffer_release(&value);

    return status;
}

#else

/*
d_arg_internal_process_config_env
  Without the environment built in, nothing to read.
*/
D_STATIC int
d_arg_internal_process_config_env(
    struct d_arg_command* _command,
    struct d_arg_option*  _config
)
{
    (void)_command;
    (void)_config;

    return D_ARG_OK;
}

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

/*
d_arg_internal_process_config_count
  How many files CLI11's as<std::vector<std::string>>() finds in the
configuration option once called back: its values, reduced or as given --
none for a lone `{}`, and only the `{}` of `{}` and a separator.
*/
D_STATIC size_t
d_arg_internal_process_config_count(
    const struct d_arg_option* _option,
    const struct d_pack_text** _files
)
{
    size_t count = 0u;

    *_files = d_arg_option_values(_option,
                                  &count);

    // a lone `{}` names nothing
    if ( (count == 1u)                              &&
         (d_arg_internal_equal_cstr((*_files)[0],
                                    "{}")) )
    {
        return 0u;
    }

    return ( (count == 2u)                                     &&
             (d_arg_internal_equal_cstr((*_files)[0],
                                        "{}"))                 &&
             (d_arg_internal_is_separator((*_files)[1])) )
           ? 1u
           : count;
}

/*
d_arg_internal_process_config_unused
  CLI11's end for a configuration option whose files were none read: its
values dropped, so its count reads 0, and its callback run again as if it
were not forced.
*/
D_STATIC int
d_arg_internal_process_config_unused(
    struct d_arg_option* _config
)
{
    const uint32_t forced = (_config->flags & D_ARG_OPTION_FORCE_CALLBACK);

    d_arg_option_clear(_config);
    _config->flags &= ~D_ARG_OPTION_FORCE_CALLBACK;

    const int status = d_arg_option_run_callback(_config);

    _config->flags |= forced;

    return status;
}

/*
d_arg_internal_process_config_read
  The files the configuration option names, each read in turn -- each must
be read when the option is required or was given; none named is an error
when one is required, and none read leaves the option as if unused.
*/
D_STATIC int
d_arg_internal_process_config_read(
    struct d_arg_command* _command,
    struct d_arg_option*  _config,
    bool                  _required,
    bool                  _given
)
{
    const struct d_pack_text* files  = NULL;
    const size_t              count  = d_arg_internal_process_config_count(
                                           _config,
                                           &files);
    bool                      used   = _given;
    int                       status = D_ARG_OK;

    // no file named
    if ( (count == 0u) ||
         (files[0].length == 0u) )
    {
        return (_required)
               ? d_arg_internal_fail(_command,
                                     _config,
                                     D_ARG_ERROR_FILE,
                                     "config file is required but none was "
                                     "given")
               : D_ARG_OK;
    }

    // each file in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < count);
         ++index)
    {
        status = d_arg_internal_process_config_file(
                     _command,
                     files[index],
                     ( (_required) ||
                       (_given) ),
                     &used);
    }

    return ( (status == D_ARG_OK) &&
             (!used) )
           ? d_arg_internal_process_config_unused(_config)
           : status;
}

/*
d_arg_internal_process_config
  CLI11's _process_config_file: the configuration option, given nothing,
takes its environment variable's value; is called back, a forced default
taking a value's place; then the files it names are read.
*/
int
d_arg_internal_process_config(
    struct d_arg_command* _command
)
{
    struct d_arg_option* const config = _command->config;

    // no configuration option
    if (!config)
    {
        return D_ARG_OK;
    }

    const bool required = d_arg_option_has_flag(config,
                                                D_ARG_OPTION_REQUIRED);
    const bool given    = (config->results.count > 0u);
    int        status   = (given)
                          ? D_ARG_OK
                          : d_arg_internal_process_config_env(_command,
                                                              config);

    status = (status == D_ARG_OK)
             ? d_arg_option_run_callback(config)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_process_config_read(_command,
                                                config,
                                                required,
                                                given)
           : status;
}
