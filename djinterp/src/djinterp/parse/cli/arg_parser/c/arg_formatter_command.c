/*******************************************************************************
* djinterp [parse]                                       arg_formatter_command.c
*
* The default steps for a command's help, declared in arg_formatter.h (4.2
* and 4.3).
*   CLI11's Formatter, one C function for each of its make_* functions that
* lays out a command and for each of their parts: the description, with the
* option requirements CLI11 adds to it; the usage line; the footer; the
* positionals and the groups of named options; the subcommands, grouped,
* listed one per line or, for help-all, each with its own help; a subcommand
* or an option group expanded in place; and the whole help. Each calls the
* others through the dispatchers in arg_formatter.c, as CLI11's virtual calls
* go through `this`, so a replaced step is used wherever CLI11 would use it.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_formatter_command.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr, strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "./arg_internal.h"  // spans, lists, help text


//==============================================================================
// 1.  TEXT OF A COMMAND
//==============================================================================

/*
d_arg_internal_help_written
  A command's usage or footer -- CLI11's get_usage() or get_footer(), what
d_arg_command_usage or d_arg_command_footer writes -- caught in a buffer;
a buffer that ran out of memory turns the sink's refusal into what it was.
*/
D_STATIC int
d_arg_internal_help_written(
    const struct d_arg_command* _command,
    bool                        _footer,
    struct d_arg_buffer*        _text
)
{
    const struct d_pack_sink sink   = d_arg_buffer_sink(_text);
    const int                status = (_footer)
                                      ? d_arg_command_footer(_command,
                                                             sink)
                                      : d_arg_command_usage(_command,
                                                            sink);

    return (_text->failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

/*
d_arg_internal_help_requirement
  The sentence CLI11's make_description adds for a command's option
requirements: at most, exactly, between or at least so many.
*/
D_STATIC int
d_arg_internal_help_requirement(
    const struct d_arg_command* _command,
    struct d_arg_buffer*        _out
)
{
    const size_t low    = _command->require_option_min;
    const size_t high   = _command->require_option_max;
    const bool   range  = ( (low > 0u)   &&
                            (high > 0u)  &&
                            (high != low) );
    int          status = D_ARG_OK;

    // no requirement, no sentence
    if ( (low == 0u) &&
         (high == 0u) )
    {
        return D_ARG_OK;
    }

    status = d_arg_buffer_append_cstr(_out,
                                      (low == 0u)
                                      ? " \n[At most "
                                      : (high == low)
                                        ? " \n[Exactly "
                                        : (range)
                                          ? " \n[Between "
                                          : " \n[At least ");
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_unsigned(_out,
                                            (low == 0u) ? high : low)
             : status;
    status = ( (status == D_ARG_OK) &&
               (range) )
             ? d_arg_buffer_append_cstr(_out,
                                        " and ")
             : status;
    status = ( (status == D_ARG_OK) &&
               (range) )
             ? d_arg_internal_help_unsigned(_out,
                                            high)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_out,
                                      (low == 0u)
                                      ? " of the following options are "
                                        "allowed]"
                                      : " of the following options are "
                                        "required]")
           : status;
}

/*
d_arg_format_description_default
  CLI11's make_description: the description, " REQUIRED " for a required
command, the requirement sentence, and a blank line after whatever was
written -- nothing at all when that was nothing.
*/
int
d_arg_format_description_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t start  = _out->length;
    int          status = d_arg_buffer_append_cstr(
                              _out,
                              d_arg_command_description(_command));

    // a required command says so, between spaces
    if ( (status == D_ARG_OK) &&
         (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_REQUIRED)) )
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_label(_formatter,
                                             "REQUIRED",
                                             _out)
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_out,
                                            ' ')
                 : status;
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_requirement(_command,
                                               _out)
             : status;

    return ( (status == D_ARG_OK) &&
             (_out->length > start) )
           ? d_arg_buffer_append_cstr(_out,
                                      "\n\n")
           : status;
}

/*
d_arg_internal_help_named
  The filter make_usage's badge collects with: an option with a name --
hidden ones and the help flag among them.
*/
D_STATIC bool
d_arg_internal_help_named(
    const struct d_arg_option* _option,
    const void*                _context
)
{
    (void)_context;

    return d_arg_option_is_nonpositional(_option);
}

/*
d_arg_internal_help_shown_positional
  The filter make_usage and make_positionals collect positionals with: a
positional name, and a group, so not hidden.
*/
D_STATIC bool
d_arg_internal_help_shown_positional(
    const struct d_arg_option* _option,
    const void*                _context
)
{
    (void)_context;

    return ( (d_arg_option_group(_option)[0] != '\0') &&
             (d_arg_option_is_positional(_option)) );
}

/*
d_arg_internal_help_badge
  " [OPTIONS]" when the command lists any option with a name.
*/
D_STATIC int
d_arg_internal_help_badge(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_option_list named  = D_ARG_LIST_INIT;
    int                      status = d_arg_internal_command_options_where(
                                          _command,
                                          true,
                                          d_arg_internal_help_named,
                                          NULL,
                                          &named);

    // a name, a badge
    if ( (status == D_ARG_OK) &&
         (named.count > 0u) )
    {
        status = d_arg_buffer_append_cstr(_out,
                                          " [");
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_label(_formatter,
                                             "OPTIONS",
                                             _out)
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_out,
                                            ']')
                 : status;
    }

    d_arg_internal_option_list_release(&named);

    return status;
}

/*
d_arg_internal_help_usage_options
  The usage line's middle: the options badge, then each positional the
command lists that is not hidden, as its usage step writes it, after a
space.
*/
D_STATIC int
d_arg_internal_help_usage_options(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_option_list positionals = D_ARG_LIST_INIT;
    int                      status      = d_arg_internal_help_badge(
                                               _formatter,
                                               _command,
                                               _out);

    status = (status == D_ARG_OK)
             ? d_arg_internal_command_options_where(
                   _command,
                   true,
                   d_arg_internal_help_shown_positional,
                   NULL,
                   &positionals)
             : status;

    // each after a space
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < positionals.count);
         ++index)
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
        status = (status == D_ARG_OK)
                 ? d_arg_format_option_usage(_formatter,
                                             positionals.items[index],
                                             _out)
                 : status;
    }

    d_arg_internal_option_list_release(&positionals);

    return status;
}

/*
d_arg_internal_help_usage_marker
  The usage line's end: "[SUBCOMMANDS]" when the command has an enabled
subcommand with a name, without the brackets when one is required, and in
the singular when at most one may be given.
*/
D_STATIC int
d_arg_internal_help_usage_marker(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    const bool optional = (_command->require_subcommand_min == 0u);
    bool       any      = false;
    int        status   = D_ARG_OK;

    // whether a subcommand can be given
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* sub = _command->subcommands.items[index];

        any = ( (any) ||
                ( (!d_arg_command_has_flag(sub,
                                           D_ARG_COMMAND_DISABLED)) &&
                  (d_arg_command_name(sub)[0] != '\0') ) );
    }

    // nothing to mark
    if (!any)
    {
        return D_ARG_OK;
    }

    status = d_arg_buffer_append_cstr(_out,
                                      (optional) ? " [" : " ");
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_label(
                   _formatter,
                   (_command->require_subcommand_max == 1u)
                   ? "SUBCOMMAND"
                   : "SUBCOMMANDS",
                   _out)
             : status;

    return ( (status == D_ARG_OK) &&
             (optional) )
           ? d_arg_buffer_append_char(_out,
                                      ']')
           : status;
}

/*
d_arg_internal_help_usage_line
  The usage line make_usage composes: a line feed, the name -- or "Usage:"
for a nameless command -- the options badge, the positionals and the
subcommand marker, then a blank line.
*/
D_STATIC int
d_arg_internal_help_usage_line(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    struct d_arg_buffer*          _out
)
{
    int status = d_arg_buffer_append_char(_out,
                                          '\n');

    status = (status != D_ARG_OK)
             ? status
             : (_name.length > 0u)
               ? d_arg_buffer_append_text(_out,
                                          _name)
               : d_arg_internal_help_label(_formatter,
                                           "Usage",
                                           _out);
    status = ( (status == D_ARG_OK) &&
               (_name.length == 0u) )
             ? d_arg_buffer_append_char(_out,
                                        ':')
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_usage_options(_formatter,
                                                 _command,
                                                 _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_usage_marker(_formatter,
                                                _command,
                                                _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_out,
                                      "\n\n")
           : status;
}

/*
d_arg_format_usage_default
  CLI11's make_usage: the command's own usage, when it has any, followed by
a blank line; otherwise the line composed.
*/
int
d_arg_format_usage_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer usage  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_help_written(_command,
                                                             false,
                                                             &usage);
    const bool          own    = ( (status != D_ARG_OK) ||
                                   (usage.length > 0u) );

    // the command's own usage stands for the line
    if (own)
    {
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_text(_out,
                                            d_arg_buffer_text(&usage))
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_cstr(_out,
                                            "\n\n")
                 : status;
    }

    d_arg_buffer_release(&usage);

    return (own)
           ? status
           : d_arg_internal_help_usage_line(_formatter,
                                            _command,
                                            _name,
                                            _out);
}

/*
d_arg_format_footer_default
  CLI11's make_footer: the footer between line feeds, or nothing.
*/
int
d_arg_format_footer_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer footer = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_help_written(_command,
                                                             true,
                                                             &footer);

    // a footer goes between line feeds
    if ( (status == D_ARG_OK) &&
         (footer.length > 0u) )
    {
        status = d_arg_buffer_append_char(_out,
                                          '\n');
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_text(_out,
                                            d_arg_buffer_text(&footer))
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_out,
                                            '\n')
                 : status;
    }

    d_arg_buffer_release(&footer);

    return status;
}

//==============================================================================
// 2.  OPTIONS OF A COMMAND
//==============================================================================

/*
d_arg_format_positionals_default
  CLI11's make_positionals: the positionals the command lists that are not
hidden, as one group under the POSITIONALS label; nothing when there are
none.
*/
int
d_arg_format_positionals_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_option_list chosen = D_ARG_LIST_INIT;
    int                      status = d_arg_internal_command_options_where(
                                          _command,
                                          true,
                                          d_arg_internal_help_shown_positional,
                                          NULL,
                                          &chosen);

    // a group only when there are some
    if ( (status == D_ARG_OK) &&
         (chosen.count > 0u) )
    {
        status = d_arg_format_group(
                     _formatter,
                     d_arg_formatter_label(
                         _formatter,
                         d_arg_internal_text_of("POSITIONALS")),
                     true,
                     &chosen,
                     _out);
    }

    d_arg_internal_option_list_release(&chosen);

    return status;
}

// d_arg_internal_help_groups
//   struct: what make_groups hands each group it visits and the filter it
// collects with: the formatter, the command, the mode, the text, and the
// group being collected.
struct d_arg_internal_help_groups
{
    const struct d_arg_formatter* formatter;
    const struct d_arg_command*   command;
    int                           mode;
    struct d_arg_buffer*          out;
    struct d_pack_text            group;
};

/*
d_arg_internal_help_in_group
  The filter of make_groups: an option of the group, with a name, but, for
an expanded subcommand, not the command's own help flags.
*/
D_STATIC bool
d_arg_internal_help_in_group(
    const struct d_arg_option* _option,
    const void*                _context
)
{
    const struct d_arg_internal_help_groups* const groups =
        (const struct d_arg_internal_help_groups*)_context;

    return ( (d_arg_internal_equal_cstr(groups->group,
                                        d_arg_option_group(_option))) &&
             (d_arg_option_is_nonpositional(_option))                 &&
             ( (groups->mode != (int)D_ARG_HELP_SUB)  ||
               ( (_option != groups->command->help) &&
                 (_option != groups->command->help_all) ) ) );
}

/*
d_arg_internal_help_group_visit
  One group of make_groups: the options the command lists in it, collected
through the filter; nothing for the hidden group or a group left empty.
*/
D_STATIC int
d_arg_internal_help_group_visit(
    struct d_pack_text _group,
    void*              _context
)
{
    struct d_arg_internal_help_groups* const groups =
        (struct d_arg_internal_help_groups*)_context;

    struct d_arg_option_list chosen = D_ARG_LIST_INIT;
    int                      status = D_ARG_OK;

    groups->group = _group;
    status        = d_arg_internal_command_options_where(
                        groups->command,
                        true,
                        d_arg_internal_help_in_group,
                        groups,
                        &chosen);

    // shown when named and not empty
    if ( (status == D_ARG_OK) &&
         (_group.length > 0u) &&
         (chosen.count > 0u) )
    {
        status = d_arg_format_group(groups->formatter,
                                    _group,
                                    false,
                                    &chosen,
                                    groups->out);
    }

    d_arg_internal_option_list_release(&chosen);

    return status;
}

/*
d_arg_format_groups
  CLI11's make_groups: the groups of the command's own options, in the order
first met, each with the options the command lists in it -- its option
groups' merged ones and its fallthrough parent's among them.
*/
int
d_arg_format_groups(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_internal_help_groups groups =
    {
        _formatter,                       // formatter
        _command,                         // command
        _mode,                            // mode
        _out,                             // out
        d_arg_internal_span("",
                            0u)           // group
    };

    return d_arg_command_each_group(_command,
                                    d_arg_internal_help_group_visit,
                                    &groups);
}

//==============================================================================
// 3.  SUBCOMMANDS
//==============================================================================

/*
d_arg_format_subcommand_default
  CLI11's make_subcommand: "  name, aliases", with the REQUIRED label for a
required one, padded to the left column, then the description in the right
column -- on the next line when the name fills the left.
*/
int
d_arg_format_subcommand_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t start  = _out->length;
    int          status = d_arg_buffer_append_cstr(_out,
                                                   "  ");

    status = (status == D_ARG_OK)
             ? d_arg_command_display_name(_command,
                                          true,
                                          _out)
             : status;

    // a required subcommand says so
    if ( (status == D_ARG_OK) &&
         (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_REQUIRED)) )
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_label(_formatter,
                                             "REQUIRED",
                                             _out)
                 : status;
    }

    const size_t named = _out->length - start;

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_pad(_out,
                                       start,
                                       d_arg_internal_help_int(
                                           _formatter->column_width))
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_column(
                   _formatter,
                   d_arg_internal_text_of(d_arg_command_description(_command)),
                   (named >= _formatter->column_width),
                   _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/*
d_arg_internal_help_seen
  Whether a group is among those already listed, compared with ASCII case
folded, as CLI11 compares them through to_lower.
*/
D_STATIC bool
d_arg_internal_help_seen(
    const struct d_arg_text_list* _seen,
    struct d_pack_text            _group
)
{
    // each group listed so far
    for (size_t index = 0u; index < _seen->count; ++index)
    {
        // the same group, whatever its case
        if (d_arg_internal_names_match(_seen->items[index],
                                       _group,
                                       true,
                                       false))
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_help_subgroup
  One group of make_subcommands: its heading, then each named subcommand in
it -- whatever the case of its group -- on a line of its own or, for
help-all, with its own expanded help, built by its own formatter within the
room this one leaves, and a line feed.
*/
D_STATIC int
d_arg_internal_help_subgroup(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _group,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    int status = d_arg_buffer_append_char(_out,
                                          '\n');

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_text(_out,
                                        _group)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        ":\n")
             : status;

    // each named subcommand of the group
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* sub  = _command->subcommands.items[index];
        const char*                 name = d_arg_command_name(sub);

        // another group's, or an option group
        if ( (!d_arg_internal_names_match(
                  d_arg_internal_text_of(d_arg_command_group(sub)),
                  _group,
                  true,
                  false)) ||
             (name[0] == '\0') )
        {
            continue;
        }

        status = (_mode != (int)D_ARG_HELP_ALL)
                 ? d_arg_format_subcommand(_formatter,
                                           sub,
                                           _out)
                 : d_arg_internal_help_make(sub,
                                            d_arg_internal_text_of(name),
                                            (int)D_ARG_HELP_SUB,
                                            _formatter->line_width,
                                            _out);
        status = ( (status == D_ARG_OK) &&
                   (_mode == (int)D_ARG_HELP_ALL) )
                 ? d_arg_buffer_append_char(_out,
                                            '\n')
                 : status;
    }

    return status;
}

/*
d_arg_internal_help_gather
  The first pass of make_subcommands: each option group expanded where it
stands -- but the hidden ones and those whose group starts with `+` -- and
the groups of the named subcommands noted in the order first met.
*/
D_STATIC int
d_arg_internal_help_gather(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_text_list*       _seen,
    struct d_arg_buffer*          _out
)
{
    int status = D_ARG_OK;

    // each subcommand: an option group expanded, a group noted
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* sub   = _command->subcommands.items[index];
        const struct d_pack_text    group = d_arg_internal_text_of(
                                                d_arg_command_group(sub));

        // an option group in place; a named one's group, if new
        if (d_arg_command_name(sub)[0] == '\0')
        {
            status = ( (group.length > 0u) &&
                       (group.data[0] != '+') )
                     ? d_arg_format_expanded(_formatter,
                                             sub,
                                             _mode,
                                             _out)
                     : D_ARG_OK;
        }
        else if ( (group.length > 0u) &&
                  (!d_arg_internal_help_seen(_seen,
                                             group)) )
        {
            status = d_arg_internal_text_push(_seen,
                                              group);
        }
    }

    return status;
}

/*
d_arg_format_subcommands_default
  CLI11's make_subcommands: the option groups and the groups gathered, then
each group's subcommands under its name.
*/
int
d_arg_format_subcommands_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_text_list seen   = D_ARG_LIST_INIT;
    int                    status = d_arg_internal_help_gather(_formatter,
                                                               _command,
                                                               _mode,
                                                               &seen,
                                                               _out);

    // each group's subcommands
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < seen.count);
         ++index)
    {
        status = d_arg_internal_help_subgroup(_formatter,
                                              _command,
                                              seen.items[index],
                                              _mode,
                                              _out);
    }

    d_arg_text_list_release(&seen);

    return status;
}

//==============================================================================
// 4.  WHOLE HELP
//==============================================================================

/*
d_arg_internal_help_indent
  CLI11's indent_block: the indent before each line that is not empty.
*/
D_STATIC int
d_arg_internal_help_indent(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text,
    size_t               _indent
)
{
    size_t at     = 0u;
    int    status = D_ARG_OK;

    // each line, with its line feed
    while ( (status == D_ARG_OK) &&
            (at < _text.length) )
    {
        const char*  feed = (const char*)memchr(_text.data + at,
                                                '\n',
                                                _text.length - at);
        const size_t end  = (feed != NULL)
                            ? (size_t)(feed - _text.data) + 1u
                            : _text.length;

        status = (_text.data[at] != '\n')
                 ? d_arg_internal_help_spaces(_out,
                                              _indent)
                 : D_ARG_OK;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append(_out,
                                       _text.data + at,
                                       end - at)
                 : status;
        at     = end;
    }

    return status;
}

/*
d_arg_internal_help_aliases
  CLI11's format_aliases: "     aliases: " right-aligned in the width --
setw without std::left -- then the aliases joined by ", ", each with
fourteen spaces after every carriage return or line feed in it, as
fix_newlines inserts them, and a line feed.
*/
D_STATIC int
d_arg_internal_help_aliases(
    struct d_arg_buffer*            _out,
    const struct d_arg_string_list* _aliases,
    size_t                          _width
)
{
    static const char LABEL[]  = "     aliases: ";
    const size_t      labelled = sizeof(LABEL) - 1u;
    int               status   = (_width > labelled)
                                 ? d_arg_internal_help_spaces(
                                       _out,
                                       _width - labelled)
                                 : D_ARG_OK;

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        LABEL)
             : status;

    // each alias, its line breaks indented
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _aliases->count);
         ++index)
    {
        const char* alias = _aliases->items[index];

        status = (index > 0u)
                 ? d_arg_buffer_append_cstr(_out,
                                            ", ")
                 : D_ARG_OK;

        // each character, a line break followed by the indent
        for (size_t at = 0u; (status == D_ARG_OK) && (alias[at] != '\0'); ++at)
        {
            status = d_arg_buffer_append_char(_out,
                                              alias[at]);
            status = ( (status == D_ARG_OK) &&
                       ( (alias[at] == '\r') ||
                         (alias[at] == '\n') ) )
                     ? d_arg_internal_help_spaces(_out,
                                                  labelled)
                     : status;
        }
    }

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/*
d_arg_internal_help_described
  The description as make_help or make_expanded writes it: wrapped as a
paragraph behind the indent when paragraphs are on; otherwise as it is --
indented, and followed by a line feed, when expanded.
*/
D_STATIC int
d_arg_internal_help_described(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    size_t                        _indent,
    bool                          _expanded,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_format_description(_formatter,
                                                          _command,
                                                          &text);

    // paragraphs, or the text as it is
    if ((_formatter->flags & D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS) != 0u)
    {
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_paragraph(
                       _out,
                       d_arg_buffer_text(&text),
                       d_arg_internal_help_fitted(
                           _formatter,
                           _formatter->description_paragraph_width,
                           0u),
                       _indent,
                       false)
                 : status;
    }
    else
    {
        status = (status == D_ARG_OK)
                 ? d_arg_internal_help_indent(_out,
                                              d_arg_buffer_text(&text),
                                              _indent)
                 : status;
        status = ( (status == D_ARG_OK) &&
                   (_expanded) )
                 ? d_arg_buffer_append_char(_out,
                                            '\n')
                 : status;
    }

    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_internal_help_footed
  A footer as make_help and make_expanded write it: wrapped as a paragraph
when footer paragraphs are on, as it is otherwise.
*/
D_STATIC int
d_arg_internal_help_footed(
    const struct d_arg_formatter* _formatter,
    struct d_pack_text            _footer,
    struct d_arg_buffer*          _out
)
{
    return ((_formatter->flags & D_ARG_FORMAT_FOOTER_PARAGRAPHS) != 0u)
           ? d_arg_internal_help_paragraph(
                 _out,
                 _footer,
                 d_arg_internal_help_fitted(
                     _formatter,
                     _formatter->footer_paragraph_width,
                     0u),
                 0u,
                 false)
           : d_arg_buffer_append_text(_out,
                                      _footer);
}

/*
d_arg_internal_help_body
  The positionals, groups and subcommands of an expanded command, each made
whole and then indented.
*/
D_STATIC int
d_arg_internal_help_body(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    size_t                        _indent,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer part   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_format_positionals(_formatter,
                                                          _command,
                                                          &part);

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_indent(_out,
                                          d_arg_buffer_text(&part),
                                          _indent)
             : status;
    d_arg_buffer_clear(&part);
    status = (status == D_ARG_OK)
             ? d_arg_format_groups(_formatter,
                                   _command,
                                   _mode,
                                   &part)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_indent(_out,
                                          d_arg_buffer_text(&part),
                                          _indent)
             : status;
    d_arg_buffer_clear(&part);
    status = (status == D_ARG_OK)
             ? d_arg_format_subcommands(_formatter,
                                        _command,
                                        _mode,
                                        &part)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_indent(_out,
                                          d_arg_buffer_text(&part),
                                          _indent)
             : status;
    d_arg_buffer_release(&part);

    return status;
}

/*
d_arg_internal_help_expanded_footer
  An expanded command's footer, but, inside an expanded help, not one its
parent shows as well -- the copy a subcommand inherits.
*/
D_STATIC int
d_arg_internal_help_expanded_footer(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer footer = D_ARG_BUFFER_INIT;
    struct d_arg_buffer parent = D_ARG_BUFFER_INIT;
    int                 status = d_arg_format_footer(_formatter,
                                                     _command,
                                                     &footer);

    // the parent's footer, to compare
    if ( (status == D_ARG_OK)              &&
         (_mode == (int)D_ARG_HELP_SUB)    &&
         (footer.length > 0u)              &&
         (_command->parent != NULL) )
    {
        status = d_arg_format_footer(_formatter,
                                     _command->parent,
                                     &parent);

        // the same footer is left to the parent
        if ( (status == D_ARG_OK) &&
             (d_arg_internal_equal(d_arg_buffer_text(&footer),
                                   d_arg_buffer_text(&parent))) )
        {
            d_arg_buffer_clear(&footer);
        }
    }

    status = ( (status == D_ARG_OK) &&
               (footer.length > 0u) )
             ? d_arg_internal_help_footed(_formatter,
                                          d_arg_buffer_text(&footer),
                                          _out)
             : status;
    d_arg_buffer_release(&parent);
    d_arg_buffer_release(&footer);

    return status;
}

/*
d_arg_internal_help_heading
  The head of an expanded command: its display name on a line of its own,
its description, and an option group's aliases.
*/
D_STATIC int
d_arg_internal_help_heading(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    size_t                        _indent,
    struct d_arg_buffer*          _out
)
{
    int status = d_arg_command_display_name(_command,
                                            true,
                                            _out);

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(_out,
                                        '\n')
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_described(_formatter,
                                             _command,
                                             _indent,
                                             true,
                                             _out)
             : status;

    return ( (status == D_ARG_OK)                           &&
             (d_arg_command_name(_command)[0] == '\0')      &&
             (_command->aliases.count > 0u) )
           ? d_arg_internal_help_aliases(_out,
                                         &_command->aliases,
                                         _formatter->column_width + 2u)
           : status;
}

/*
d_arg_format_expanded_default
  CLI11's make_expanded: the head, the body and the footer. An option
group's description and body are indented two spaces; with a line width,
they are laid out by a copy of the formatter whose line width is two less,
so that the indented lines keep within the original's.
*/
int
d_arg_format_expanded_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const size_t           indent = (d_arg_command_name(_command)[0] == '\0')
                                    ? 2u
                                    : 0u;
    struct d_arg_formatter nested = *_formatter;

    // the nested text keeps within what the indent leaves
    if (_formatter->line_width > 0u)
    {
        nested.line_width = (_formatter->line_width > indent)
                            ? (_formatter->line_width - indent)
                            : 1u;
    }

    int status = d_arg_internal_help_heading(&nested,
                                             _command,
                                             indent,
                                             _out);

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_body(&nested,
                                        _command,
                                        _mode,
                                        indent,
                                        _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_help_expanded_footer(_formatter,
                                                 _command,
                                                 _mode,
                                                 _out)
           : status;
}

/*
d_arg_internal_help_rest
  The end of make_help: the positionals, the groups, the subcommands, and
the footer, in turn.
*/
D_STATIC int
d_arg_internal_help_rest(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer footer = D_ARG_BUFFER_INIT;
    int                 status = d_arg_format_positionals(_formatter,
                                                          _command,
                                                          _out);

    status = (status == D_ARG_OK)
             ? d_arg_format_groups(_formatter,
                                   _command,
                                   _mode,
                                   _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_format_subcommands(_formatter,
                                        _command,
                                        _mode,
                                        _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_format_footer(_formatter,
                                   _command,
                                   &footer)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_help_footed(_formatter,
                                          d_arg_buffer_text(&footer),
                                          _out)
             : status;
    d_arg_buffer_release(&footer);

    return status;
}

/*
d_arg_internal_help_group_name
  What make_help writes first for an option group asked for directly: its
group and a colon, unless the group is "SUBCOMMANDS".
*/
D_STATIC int
d_arg_internal_help_group_name(
    const struct d_arg_command* _command,
    struct d_arg_buffer*        _out
)
{
    const char* group  = d_arg_command_group(_command);
    int         status = D_ARG_OK;

    // only a nameless command with a parent
    if ( (d_arg_command_name(_command)[0] != '\0') ||
         (_command->parent == NULL)                ||
         (strcmp(group,
                 "SUBCOMMANDS") == 0) )
    {
        return D_ARG_OK;
    }

    status = d_arg_buffer_append_cstr(_out,
                                      group);

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      ':')
           : status;
}

/*
d_arg_format_help_default
  CLI11's Formatter::make_help: in SUB mode the expanded form alone;
otherwise an option group's name, the description, the usage, then the
positionals, the groups, the subcommands and the footer.
*/
int
d_arg_format_help_default(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter)                      ||
         (!_command)                        ||
         (!_out)                            ||
         (!d_arg_internal_help_mode(_mode)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // an expanded subcommand
    if (_mode == (int)D_ARG_HELP_SUB)
    {
        return d_arg_format_expanded(_formatter,
                                     _command,
                                     _mode,
                                     _out);
    }

    int status = d_arg_internal_help_group_name(_command,
                                                _out);

    status = (status == D_ARG_OK)
             ? d_arg_internal_help_described(_formatter,
                                             _command,
                                             0u,
                                             false,
                                             _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_format_usage(_formatter,
                                  _command,
                                  _name,
                                  _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_help_rest(_formatter,
                                      _command,
                                      _mode,
                                      _out)
           : status;
}
