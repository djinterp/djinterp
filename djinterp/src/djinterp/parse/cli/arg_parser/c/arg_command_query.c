/*******************************************************************************
* djinterp [parse]                                           arg_command_query.c
*
* What can be asked of a command, declared in arg_command.h (section 6).
*   Its fields, as CLI11's getters read them; what the last parse did with
* it -- how often it and its subcommands were used, which subcommands it
* met; its display name; the options and groups it lists; the arguments it
* left unused; and the usage, footer and version it writes on demand.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_command_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text,
                                                                // d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, sinks
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "./arg_internal.h"  // lists, spans, commands


//==============================================================================
// 1.  FIELDS
//==============================================================================

/*
d_arg_command_name
  The name, or "".
*/
const char*
d_arg_command_name(
    const struct d_arg_command* _command
)
{
    return ( (_command != NULL) &&
             (_command->name != NULL) )
           ? _command->name
           : "";
}

/*
d_arg_command_description
  The description, or "".
*/
const char*
d_arg_command_description(
    const struct d_arg_command* _command
)
{
    return ( (_command != NULL) &&
             (_command->description != NULL) )
           ? _command->description
           : "";
}

/*
d_arg_command_group
  The group, or CLI11's default, "SUBCOMMANDS".
*/
const char*
d_arg_command_group(
    const struct d_arg_command* _command
)
{
    return ( (_command != NULL) &&
             (_command->group != NULL) )
           ? _command->group
           : "SUBCOMMANDS";
}

//==============================================================================
// 2.  PARSE STATE
//==============================================================================

/*
d_arg_command_count
  CLI11's count(): the uses the last parse counted.
*/
size_t
d_arg_command_count(
    const struct d_arg_command* _command
)
{
    return (_command != NULL)
           ? _command->parsed
           : 0u;
}

/*
d_arg_command_count_all
  CLI11's count_all(): the values of the command's options, then those of
its subcommands in turn, then -- for a command with a name -- its own
uses.
*/
size_t
d_arg_command_count_all(
    const struct d_arg_command* _command
)
{
    size_t total = 0u;

    // nothing counted without a command
    if (!_command)
    {
        return 0u;
    }

    // its options' values
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        total += d_arg_option_count(_command->options.items[index]);
    }

    // its subcommands', in turn
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        total += d_arg_command_count_all(_command->subcommands.items[index]);
    }

    return (d_arg_command_name(_command)[0] != '\0')
           ? (total + _command->parsed)
           : total;
}

/*
d_arg_command_parsed_subcommand_count
  How many subcommands the last parse met.
*/
size_t
d_arg_command_parsed_subcommand_count(
    const struct d_arg_command* _command
)
{
    return (_command != NULL)
           ? _command->parsed_subcommands.count
           : 0u;
}

/*
d_arg_command_parsed_subcommand_at
  The subcommand the last parse met at an index, NULL past the end.
*/
struct d_arg_command*
d_arg_command_parsed_subcommand_at(
    const struct d_arg_command* _command,
    size_t                      _index
)
{
    return ( (_command != NULL) &&
             (_index < _command->parsed_subcommands.count) )
           ? _command->parsed_subcommands.items[_index]
           : NULL;
}

/*
d_arg_command_got_subcommand
  CLI11's got_subcommand(App*): only one of the command's own subcommands
counts, and only when the last parse used it.
*/
bool
d_arg_command_got_subcommand(
    const struct d_arg_command* _command,
    const struct d_arg_command* _subcommand
)
{
    return ( (_command != NULL)                                    &&
             (_subcommand != NULL)                                 &&
             (d_arg_internal_command_has(&_command->subcommands,
                                         _subcommand))             &&
             (_subcommand->parsed > 0u) );
}

/*
d_arg_command_got_subcommand_name
  CLI11's got_subcommand(name): the subcommand the name selects, used by
the last parse.
*/
bool
d_arg_command_got_subcommand_name(
    const struct d_arg_command* _command,
    const char*                 _name
)
{
    const struct d_arg_command* const subcommand =
        d_arg_command_subcommand(_command,
                                 _name);

    return ( (subcommand != NULL) &&
             (subcommand->parsed > 0u) );
}

//==============================================================================
// 3.  NAMES
//==============================================================================

/*
d_arg_command_display_name
  CLI11's get_display_name: a command without a name -- an option group,
or a root still unnamed -- by its group; otherwise its name, then each
alias after a comma when asked.
*/
int
d_arg_command_display_name(
    const struct d_arg_command* _command,
    bool                        _with_aliases,
    struct d_arg_buffer*        _out
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // no name: the group stands in for it
    if (d_arg_command_name(_command)[0] == '\0')
    {
        return d_arg_buffer_printf(_out,
                                   "[Option Group: %s]",
                                   d_arg_command_group(_command));
    }

    int status = d_arg_buffer_append_cstr(_out,
                                          _command->name);

    // the aliases, when asked for
    for (size_t index = 0u;
         (_with_aliases) &&
         (status == D_ARG_OK) &&
         (index < _command->aliases.count);
         ++index)
    {
        status = d_arg_buffer_printf(_out,
                                     ", %s",
                                     _command->aliases.items[index]);
    }

    return status;
}

//==============================================================================
// 4.  TRAVERSAL
//==============================================================================

/*
d_arg_command_each_option
  CLI11's get_options() collected first, then visited, so that the visit
sees the list as it stood.
*/
int
d_arg_command_each_option(
    struct d_arg_command* _command,
    d_arg_option_visit_fn _visit,
    void*                 _context
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_visit) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_option_list list   = D_ARG_LIST_INIT;
    int                      status = d_arg_internal_command_options(_command,
                                                                     false,
                                                                     &list);

    // each option, until a visit stops
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < list.count);
         ++index)
    {
        status = _visit(list.items[index],
                        _context);
    }

    d_arg_internal_option_list_release(&list);

    return status;
}

/*
d_arg_internal_command_group_seen
  Whether an option before `_index` in the command's list is in the same
group as the option at it.
*/
D_STATIC bool
d_arg_internal_command_group_seen(
    const struct d_arg_command* _command,
    size_t                      _index
)
{
    const char* const group = d_arg_option_group(
                                  _command->options.items[_index]);

    // each option before it
    for (size_t index = 0u; index < _index; ++index)
    {
        if (strcmp(d_arg_option_group(_command->options.items[index]),
                   group) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_command_each_group
  CLI11's get_groups(): each group among the command's own options, once,
in the order first met.
*/
int
d_arg_command_each_group(
    const struct d_arg_command* _command,
    d_arg_group_visit_fn        _visit,
    void*                       _context
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_visit) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = D_ARG_OK;

    // each group the first time it is met
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        if (!d_arg_internal_command_group_seen(_command,
                                               index))
        {
            status = _visit(d_arg_internal_text_of(
                                d_arg_option_group(
                                    _command->options.items[index])),
                            _context);
        }
    }

    return status;
}

//==============================================================================
// 5.  LEFTOVERS
//==============================================================================

/*
d_arg_internal_command_visit_extras
  Each argument of one list, until a visit stops.
*/
D_STATIC int
d_arg_internal_command_visit_extras(
    const struct d_arg_extra_list* _list,
    d_arg_extra_visit_fn           _visit,
    void*                          _context
)
{
    int status = D_ARG_OK;

    // each argument, in order
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _list->count);
         ++index)
    {
        status = _visit(&_list->items[index],
                        _context);
    }

    return status;
}

/*
d_arg_internal_command_visit_groups
  The part of CLI11's remaining(true) for option groups: when the command's
own extras are errors or ignored, those its nameless subcommands kept.
*/
D_STATIC int
d_arg_internal_command_visit_groups(
    const struct d_arg_command* _command,
    d_arg_extra_visit_fn        _visit,
    void*                       _context
)
{
    int status = D_ARG_OK;

    // only where the command itself keeps none
    if ( (_command->allow_extras != (int)D_ARG_EXTRAS_ERROR) &&
         (_command->allow_extras != (int)D_ARG_EXTRAS_IGNORE) )
    {
        return D_ARG_OK;
    }

    // each nameless subcommand's
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const group =
            _command->subcommands.items[index];

        if (d_arg_command_name(group)[0] == '\0')
        {
            status = d_arg_internal_command_visit_extras(&group->missing,
                                                         _visit,
                                                         _context);
        }
    }

    return status;
}

/*
d_arg_command_each_remaining
  CLI11's remaining(): the command's own extras; with `_recurse`, then its
option groups' and, in turn, those of each subcommand the last parse met.
*/
int
d_arg_command_each_remaining(
    const struct d_arg_command* _command,
    bool                        _recurse,
    d_arg_extra_visit_fn        _visit,
    void*                       _context
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_visit) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_internal_command_visit_extras(&_command->missing,
                                                     _visit,
                                                     _context);

    // the option groups', then the parsed subcommands'
    if ( (status == D_ARG_OK) &&
         (_recurse) )
    {
        status = d_arg_internal_command_visit_groups(_command,
                                                     _visit,
                                                     _context);
    }

    for (size_t index = 0u;
         (_recurse) &&
         (status == D_ARG_OK) &&
         (index < _command->parsed_subcommands.count);
         ++index)
    {
        status = d_arg_command_each_remaining(
                     _command->parsed_subcommands.items[index],
                     true,
                     _visit,
                     _context);
    }

    return status;
}

/*
d_arg_command_remaining_size
  CLI11's remaining_size(): the extras that are not the `--` ending
options, and, with `_recurse`, those of every subcommand.
*/
size_t
d_arg_command_remaining_size(
    const struct d_arg_command* _command,
    bool                        _recurse
)
{
    size_t total = 0u;

    // nothing remains of no command
    if (!_command)
    {
        return 0u;
    }

    // the command's own
    for (size_t index = 0u; index < _command->missing.count; ++index)
    {
        if (_command->missing.items[index].classifier !=
            (int)D_ARG_CLASS_POSITIONAL_MARK)
        {
            total++;
        }
    }

    // every subcommand's, parsed or not, as CLI11 counts them
    for (size_t index = 0u;
         (_recurse) && (index < _command->subcommands.count);
         ++index)
    {
        total += d_arg_command_remaining_size(
                     _command->subcommands.items[index],
                     true);
    }

    return total;
}

//==============================================================================
// 6.  TEXT ON DEMAND
//==============================================================================

/*
d_arg_internal_command_write_text
  CLI11's get_usage and get_footer: the function's text and a newline, when
there is a function, then the fixed text.
*/
D_STATIC int
d_arg_internal_command_write_text(
    const struct d_arg_command* _command,
    d_arg_command_text_fn       _write,
    void*                       _context,
    const char*                 _text,
    struct d_pack_sink          _sink
)
{
    int status = D_ARG_OK;

    // the function's text first
    if (_write != NULL)
    {
        status = _write(_command,
                        _sink,
                        _context);
        status = (status == D_ARG_OK)
                 ? d_arg_sink_write(_sink,
                                    "\n",
                                    1u)
                 : status;
    }

    return (status == D_ARG_OK)
           ? d_arg_sink_cstr(_sink,
                             _text)
           : status;
}

/*
d_arg_command_usage
  CLI11's get_usage().
*/
int
d_arg_command_usage(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink
)
{
    return (_command != NULL)
           ? d_arg_internal_command_write_text(_command,
                                               _command->usage_fn,
                                               _command->usage_context,
                                               _command->usage,
                                               _sink)
           : D_ARG_ERROR_BAD_ARGUMENT;
}

/*
d_arg_command_footer
  CLI11's get_footer().
*/
int
d_arg_command_footer(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink
)
{
    return (_command != NULL)
           ? d_arg_internal_command_write_text(_command,
                                               _command->footer_fn,
                                               _command->footer_context,
                                               _command->footer,
                                               _sink)
           : D_ARG_ERROR_BAD_ARGUMENT;
}

/*
d_arg_command_version
  CLI11's version(): nothing without a version flag; otherwise what the
function writes, or the text.
*/
int
d_arg_command_version(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // no flag, no version
    if (!_command->version)
    {
        return D_ARG_OK;
    }

    return (_command->version_fn != NULL)
           ? _command->version_fn(_command,
                                  _sink,
                                  _command->version_context)
           : d_arg_sink_cstr(_sink,
                             _command->version_text);
}
