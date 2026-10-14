/*******************************************************************************
* djinterp [parse]                                              arg_completion.c
*
* Shell completion, declared in arg_completion.h and arg_internal.h (11.1 and
* 11.2.1 to 11.2.3).
*   The shells' names; the model -- the command tree as completion sees it,
* from which every shell's script is written: the commands help shows, in
* breadth-first order, each with its subcommands' names, its options and
* their names, in the order the parse searches them, and how each option's
* values complete; the text the writers share; d_arg_completion_script,
* which hands the model to the shell's writer -- arg_completion_sh.c for
* bash and zsh, arg_completion_fish.c, arg_completion_pwsh.c -- and the
* option that asks for a script.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_completion.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_completion.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // strlen, strstr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // type flags
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator
#include "./arg_internal.h"  // the model, failures, characters, growth
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SHELL_NAMES
//   constant: each shell's name, in d_arg_shell's order, then "pwsh", which
// names PowerShell too.
static const char* const SHELL_NAMES[] =
{
    "bash",
    "zsh",
    "fish",
    "powershell",
    "pwsh"
};

// SHELL_NAME_COUNT
//   constant: how many names SHELL_NAMES holds.
static const size_t SHELL_NAME_COUNT =
    sizeof(SHELL_NAMES) / sizeof(SHELL_NAMES[0]);

// SHELL_SET
//   constant: the names as a rejection lists them, as CLI11's IsMember
// writes its set.
static const char SHELL_SET[] = "{bash,zsh,fish,powershell,pwsh}";

// DEFAULT_DESCRIPTION
//   constant: the completion option's help text when none is given.
static const char DEFAULT_DESCRIPTION[] =
    "Print a completion script for a shell (bash, zsh, fish or powershell) "
    "and exit";

// HEX_DIGITS
//   constant: the digits an identifier writes a byte with.
static const char HEX_DIGITS[] = "0123456789abcdef";

// d_arg_internal_complete_listing
//   struct: a listing of an option's choices under way: the model they are
// copied into, and where the option's own begin.
struct d_arg_internal_complete_listing
{
    struct d_arg_internal_model* model;
    size_t                       first;
};

// d_arg_internal_complete_writer
//   type: a shell's writer, as arg_internal.h declares them.
typedef int (*d_arg_internal_complete_writer)(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _program,
    struct d_arg_buffer*               _out);

// WRITERS
//   constant: each shell's writer, in d_arg_shell's order.
static const d_arg_internal_complete_writer WRITERS[] =
{
    d_arg_internal_complete_bash,
    d_arg_internal_complete_zsh,
    d_arg_internal_complete_fish,
    d_arg_internal_complete_pwsh
};

//==============================================================================
// 1.  SHELLS
//==============================================================================

/*
d_arg_shell_from_name
  The shell a name names, ASCII case aside; "pwsh" is PowerShell. A span
without bytes names none.
*/
int
d_arg_shell_from_name(
    struct d_pack_text _name
)
{
    // validate the parameters
    if (!_name.data)
    {
        return -1;
    }

    // each name in turn
    for (size_t index = 0u; index < SHELL_NAME_COUNT; ++index)
    {
        if (d_arg_internal_equal_nocase_cstr(_name,
                                             SHELL_NAMES[index]))
        {
            return (index < (size_t)D_ARG_SHELL_COUNT)
                   ? (int)index
                   : (int)D_ARG_SHELL_POWERSHELL;
        }
    }

    return -1;
}

/*
d_arg_shell_name
  A shell's first name; "" for anything else.
*/
const char*
d_arg_shell_name(
    int _shell
)
{
    return ( (_shell >= 0) &&
             (_shell < (int)D_ARG_SHELL_COUNT) )
           ? SHELL_NAMES[_shell]
           : "";
}

//==============================================================================
// 2.  THE MODEL
//==============================================================================

/*
d_arg_internal_complete_room
  An array with room for one more element: the array, grown when full and
its capacity updated; NULL when it could not grow, the array then as it was.
*/
D_STATIC void*
d_arg_internal_complete_room(
    void*   _items,
    size_t  _count,
    size_t* _capacity,
    size_t  _size
)
{
    size_t capacity = 0u;
    void*  items    = NULL;

    // room already
    if (_count < *_capacity)
    {
        return _items;
    }

    items = d_arg_internal_grow(_items,
                                *_capacity,
                                _count + 1u,
                                _size,
                                &capacity);

    // the capacity it grew to
    if (items != NULL)
    {
        *_capacity = capacity;
    }

    return items;
}

/*
d_arg_internal_complete_flags
  A command's switches as the scripts spell them: `s` for subcommand
fallthrough, `f` for fallthrough, `w` for Windows-style options, `e` for
extra arguments kept -- by an extras mode that keeps them, or as a prefix
command keeps them -- and `-` alone for none.
*/
D_STATIC void
d_arg_internal_complete_flags(
    const struct d_arg_command* _command,
    char*                       _out
)
{
    const int extras = _command->allow_extras;
    size_t    length = 0u;

    // subcommand fallthrough
    if (d_arg_command_has_flag(_command,
                               D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH))
    {
        _out[length++] = 's';
    }

    // fallthrough
    if (d_arg_command_has_flag(_command,
                               D_ARG_COMMAND_FALLTHROUGH))
    {
        _out[length++] = 'f';
    }

    // Windows-style options
    if (d_arg_command_has_flag(_command,
                               D_ARG_COMMAND_WINDOWS_STYLE))
    {
        _out[length++] = 'w';
    }

    // extra arguments kept
    if ( ( (extras != (int)D_ARG_EXTRAS_ERROR)                &&
           (extras != (int)D_ARG_EXTRAS_ERROR_IMMEDIATELY) )   ||
         (_command->prefix_command != (int)D_ARG_PREFIX_OFF) )
    {
        _out[length++] = 'e';
    }

    // none at all
    if (length == 0u)
    {
        _out[length++] = '-';
    }

    _out[length] = '\0';

    return;
}

/*
d_arg_internal_complete_add_command
  A command at the end of the model's, under `_parent`; its lists are
filled in when its turn comes.
*/
D_STATIC int
d_arg_internal_complete_add_command(
    struct d_arg_internal_model* _model,
    const struct d_arg_command*  _command,
    size_t                       _parent
)
{
    void* const items = d_arg_internal_complete_room(
                            _model->commands,
                            _model->command_count,
                            &_model->command_capacity,
                            sizeof(*_model->commands));

    // check if memory allocation was successful
    if (!items)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    struct d_arg_internal_complete_command* const record =
        (struct d_arg_internal_complete_command*)items +
        _model->command_count;

    _model->commands    = (struct d_arg_internal_complete_command*)items;
    record->command     = _command;
    record->parent      = _parent;
    record->most        = _command->require_subcommand_max;
    record->first_name  = 0u;
    record->names       = 0u;
    record->first_entry = 0u;
    record->entries     = 0u;
    d_arg_internal_complete_flags(_command,
                                  record->flags);
    _model->command_count++;

    return D_ARG_OK;
}

/*
d_arg_internal_complete_add_name
  A subcommand's name or alias at the end of the model's.
*/
D_STATIC int
d_arg_internal_complete_add_name(
    struct d_arg_internal_model* _model,
    size_t                       _command,
    size_t                       _target,
    const char*                  _name,
    struct d_pack_text           _summary
)
{
    void* const items = d_arg_internal_complete_room(
                            _model->names,
                            _model->name_count,
                            &_model->name_capacity,
                            sizeof(*_model->names));

    // check if memory allocation was successful
    if (!items)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    struct d_arg_internal_complete_name* const record =
        (struct d_arg_internal_complete_name*)items + _model->name_count;

    _model->names   = (struct d_arg_internal_complete_name*)items;
    record->command = _command;
    record->target  = _target;
    record->name    = _name;
    record->summary = _summary;
    _model->name_count++;

    return D_ARG_OK;
}

/*
d_arg_internal_complete_add_entry
  An option's name at the end of the model's.
*/
D_STATIC int
d_arg_internal_complete_add_entry(
    struct d_arg_internal_model* _model,
    size_t                       _option,
    const char*                  _name,
    bool                         _long
)
{
    void* const items = d_arg_internal_complete_room(
                            _model->entries,
                            _model->entry_count,
                            &_model->entry_capacity,
                            sizeof(*_model->entries));

    // check if memory allocation was successful
    if (!items)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    struct d_arg_internal_complete_entry* const record =
        (struct d_arg_internal_complete_entry*)items + _model->entry_count;

    _model->entries   = (struct d_arg_internal_complete_entry*)items;
    record->option    = _option;
    record->name      = _name;
    record->long_name = _long;
    _model->entry_count++;

    return D_ARG_OK;
}

/*
d_arg_internal_complete_choice
  A value a validator lists, copied into the model's choices, unless the
option's choices so far hold it already.
*/
D_STATIC int
d_arg_internal_complete_choice(
    struct d_pack_text _choice,
    void*              _context
)
{
    struct d_arg_internal_complete_listing* const listing =
        (struct d_arg_internal_complete_listing*)_context;
    struct d_arg_string_list* const               choices =
        &listing->model->choices;

    // the option's choices so far
    for (size_t index = listing->first; index < choices->count; ++index)
    {
        if (d_arg_internal_equal(_choice,
                                 d_arg_internal_text_of(
                                     choices->items[index])))
        {
            return D_ARG_OK;
        }
    }

    return d_arg_string_list_append(choices,
                                    _choice);
}

/*
d_arg_internal_complete_applies
  Whether a validator speaks for an option's values in completion: one
that is active, was made, and applies to every value or to the first.
*/
D_STATIC bool
d_arg_internal_complete_applies(
    const struct d_arg_validator* _validator
)
{
    return ( ((_validator->flags & D_ARG_VALIDATOR_ACTIVE) != 0u) &&
             (_validator->status == D_ARG_OK)                     &&
             (_validator->application_index < 1) );
}

/*
d_arg_internal_complete_listed
  The choices an option's validators list, appended to the model's: those
of each validator that speaks for the values.
*/
D_STATIC int
d_arg_internal_complete_listed(
    struct d_arg_internal_model* _model,
    const struct d_arg_option*   _option
)
{
    struct d_arg_internal_complete_listing listing = { NULL, 0u };
    int                                    status  = D_ARG_OK;

    listing.model = _model;
    listing.first = _model->choices.count;

    // each validator that speaks for the values
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->validators.count);
         ++index)
    {
        const struct d_arg_validator* const validator =
            &_option->validators.items[index];

        if (d_arg_internal_complete_applies(validator))
        {
            status = d_arg_validator_each_choice(
                         validator,
                         d_arg_internal_complete_choice,
                         &listing);
        }
    }

    return status;
}

/*
d_arg_internal_complete_value_kind
  How values complete that no choices list: as nothing when a validator
takes numbers; else as directories or files when one takes paths; else as
nothing for a bound number, truth value, character or enumeration; and as
files for anything else.
*/
D_STATIC int
d_arg_internal_complete_value_kind(
    const struct d_arg_option* _option
)
{
    const uint32_t numeric = D_ARG_TYPE_FLAG_INTEGER |
                             D_ARG_TYPE_FLAG_FLOAT   |
                             D_ARG_TYPE_FLAG_BOOL    |
                             D_ARG_TYPE_FLAG_CHAR    |
                             D_ARG_TYPE_FLAG_ENUM;
    uint32_t       kinds   = 0u;

    // what the validators that speak for the values take
    for (size_t index = 0u; index < _option->validators.count; ++index)
    {
        if (d_arg_internal_complete_applies(
                &_option->validators.items[index]))
        {
            kinds |= _option->validators.items[index].flags;
        }
    }

    // numbers
    if ((kinds & D_ARG_VALIDATOR_NUMBER) != 0u)
    {
        return (int)D_ARG_INTERNAL_COMPLETE_NOTHING;
    }

    // directories, or files
    if ((kinds & D_ARG_VALIDATOR_PATH) != 0u)
    {
        return ((kinds & D_ARG_VALIDATOR_DIRECTORY) != 0u)
               ? (int)D_ARG_INTERNAL_COMPLETE_DIRECTORIES
               : (int)D_ARG_INTERNAL_COMPLETE_FILES;
    }

    return ( (_option->type != NULL) &&
             ((_option->type->flags & numeric) != 0u) )
           ? (int)D_ARG_INTERNAL_COMPLETE_NOTHING
           : (int)D_ARG_INTERNAL_COMPLETE_FILES;
}

/*
d_arg_internal_complete_count
  A count of values as the scripts write it: unlimited for an option that
takes extra arguments, or for a count at CLI11's unlimited one.
*/
D_STATIC int
d_arg_internal_complete_count(
    const struct d_arg_option* _option,
    int                        _count
)
{
    return ( (d_arg_option_has_flag(_option,
                                    D_ARG_OPTION_ALLOW_EXTRA_ARGS)) ||
             (_count >= (int)D_ARG_EXPECTED_UNLIMITED) )
           ? D_INTERNAL_ARG_COMPLETE_UNLIMITED
           : _count;
}

/*
d_arg_internal_complete_values
  An option record's counts and kind: the values a name of it takes --
CLI11's min_num and max_num -- and as a positional; then, for one that
takes values, how they complete: as its choices, if its validators list
any, or as d_arg_internal_complete_value_kind says -- or, for one help
hides, as nothing.
*/
D_STATIC int
d_arg_internal_complete_values(
    struct d_arg_internal_model*           _model,
    struct d_arg_internal_complete_option* _record,
    bool                                   _shown
)
{
    const struct d_arg_option* const option = _record->option;
    const int                        least  = d_arg_internal_parse_min(
                                                  option);
    const int                        most   = d_arg_internal_parse_max(
                                                  option);
    int                              status = D_ARG_OK;

    _record->least        = (least > 0) ? least : 0;
    _record->most         = d_arg_internal_complete_count(option,
                                                          most);
    _record->slots        = d_arg_option_is_positional(option)
                            ? d_arg_internal_complete_count(
                                  option,
                                  d_arg_option_items_expected_max(option))
                            : 0;
    _record->first_choice = _model->choices.count;
    _record->choices      = 0u;
    _record->kind         = (int)D_ARG_INTERNAL_COMPLETE_NOTHING;

    // no values at all
    if ( (_record->most == 0) &&
         (_record->slots == 0) )
    {
        _record->kind = (int)D_ARG_INTERNAL_COMPLETE_NONE;

        return D_ARG_OK;
    }

    // an option help hides completes as nothing
    if (!_shown)
    {
        return D_ARG_OK;
    }

    status           = d_arg_internal_complete_listed(_model,
                                                      option);
    _record->choices = _model->choices.count - _record->first_choice;
    _record->kind    = (_record->choices > 0u)
                       ? (int)D_ARG_INTERNAL_COMPLETE_CHOICES
                       : d_arg_internal_complete_value_kind(option);

    return status;
}

/*
d_arg_internal_complete_names
  An option's names, each at the end of the model's: the short ones, then
the long ones.
*/
D_STATIC int
d_arg_internal_complete_names(
    struct d_arg_internal_model* _model,
    size_t                       _index,
    const struct d_arg_option*   _option
)
{
    int status = D_ARG_OK;

    // each short name
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->short_names.count);
         ++index)
    {
        status = d_arg_internal_complete_add_entry(
                     _model,
                     _index,
                     _option->short_names.items[index],
                     false);
    }

    // each long name
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->long_names.count);
         ++index)
    {
        status = d_arg_internal_complete_add_entry(
                     _model,
                     _index,
                     _option->long_names.items[index],
                     true);
    }

    return status;
}

/*
d_arg_internal_complete_add_option
  An option of the command at `_command`, if completion knows it: a
positional always -- one help hides completing as nothing -- and a named
option help shows, with its names.
*/
D_STATIC int
d_arg_internal_complete_add_option(
    struct d_arg_internal_model* _model,
    size_t                       _command,
    const struct d_arg_option*   _option
)
{
    const bool shown = (d_arg_option_group(_option)[0] != '\0');
    const bool named = ( (shown) &&
                         (d_arg_option_is_nonpositional(_option)) );

    // an option completion does not know
    if ( (!named) &&
         (!d_arg_option_is_positional(_option)) )
    {
        return D_ARG_OK;
    }

    void* const items = d_arg_internal_complete_room(
                            _model->options,
                            _model->option_count,
                            &_model->option_capacity,
                            sizeof(*_model->options));

    // check if memory allocation was successful
    if (!items)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    struct d_arg_internal_complete_option* const record =
        (struct d_arg_internal_complete_option*)items + _model->option_count;
    int                                          status = D_ARG_OK;

    _model->options = (struct d_arg_internal_complete_option*)items;
    record->option  = _option;
    record->command = _command;
    record->summary = d_arg_internal_complete_line_text(
                          _option->description);
    _model->option_count++;
    status = d_arg_internal_complete_values(_model,
                                            record,
                                            shown);

    return ( (status == D_ARG_OK) &&
             (named) )
           ? d_arg_internal_complete_names(_model,
                                           _model->option_count - 1u,
                                           _option)
           : status;
}

/*
d_arg_internal_complete_shown
  Whether help shows a subcommand or an option group: one enabled, outside
the empty group.
*/
D_STATIC bool
d_arg_internal_complete_shown(
    const struct d_arg_command* _command
)
{
    return ( (!d_arg_command_has_flag(_command,
                                      D_ARG_COMMAND_DISABLED)) &&
             (d_arg_command_group(_command)[0] != '\0') );
}

/*
d_arg_internal_complete_options
  A command's options, then those of each option group it holds that help
shows, in turn -- the order the parse searches them in -- recorded under
the command at `_index`.
*/
D_STATIC int
d_arg_internal_complete_options(
    struct d_arg_internal_model* _model,
    size_t                       _index,
    const struct d_arg_command*  _command
)
{
    int status = D_ARG_OK;

    // each option
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        status = d_arg_internal_complete_add_option(
                     _model,
                     _index,
                     _command->options.items[index]);
    }

    // each option group shown
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const group =
            _command->subcommands.items[index];

        if ( (d_arg_command_is_option_group(group)) &&
             (d_arg_internal_complete_shown(group)) )
        {
            status = d_arg_internal_complete_options(_model,
                                                     _index,
                                                     group);
        }
    }

    return status;
}

/*
d_arg_internal_complete_subcommand
  A subcommand added to the model's commands, under the command at
`_index`, with its name and its aliases.
*/
D_STATIC int
d_arg_internal_complete_subcommand(
    struct d_arg_internal_model* _model,
    size_t                       _index,
    const struct d_arg_command*  _sub
)
{
    const size_t             target  = _model->command_count;
    const struct d_pack_text summary =
        d_arg_internal_complete_line_text(_sub->description);
    int                      status  = d_arg_internal_complete_add_command(
                                           _model,
                                           _sub,
                                           _index);

    status = (status == D_ARG_OK)
             ? d_arg_internal_complete_add_name(_model,
                                                _index,
                                                target,
                                                d_arg_command_name(_sub),
                                                summary)
             : status;

    // each alias
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _sub->aliases.count);
         ++index)
    {
        status = d_arg_internal_complete_add_name(_model,
                                                  _index,
                                                  target,
                                                  _sub->aliases.items[index],
                                                  summary);
    }

    return status;
}

/*
d_arg_internal_complete_subcommands
  The subcommands help shows of a command, and of each option group it
holds that help shows -- where the parse finds them too -- added under the
command at `_index`.
*/
D_STATIC int
d_arg_internal_complete_subcommands(
    struct d_arg_internal_model* _model,
    size_t                       _index,
    const struct d_arg_command*  _command
)
{
    int status = D_ARG_OK;

    // each subcommand or option group shown
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const sub =
            _command->subcommands.items[index];

        if (!d_arg_internal_complete_shown(sub))
        {
            continue;
        }

        status = d_arg_command_is_option_group(sub)
                 ? d_arg_internal_complete_subcommands(_model,
                                                       _index,
                                                       sub)
                 : d_arg_internal_complete_subcommand(_model,
                                                      _index,
                                                      sub);
    }

    return status;
}

/*
d_arg_internal_complete_build
  The root first; then each command in turn, breadth first: its
subcommands, which join the commands to come, and its options, each
command's records together.
*/
int
d_arg_internal_complete_build(
    struct d_arg_internal_model* _model,
    const struct d_arg_command*  _root
)
{
    const struct d_arg_internal_model empty  = D_INTERNAL_ARG_MODEL_INIT;
    int                               status = D_ARG_OK;

    *_model = empty;
    status  = d_arg_internal_complete_add_command(_model,
                                                  _root,
                                                  0u);

    // each command in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _model->command_count);
         ++index)
    {
        const struct d_arg_command* const command =
            _model->commands[index].command;

        _model->commands[index].first_name = _model->name_count;
        status = d_arg_internal_complete_subcommands(_model,
                                                     index,
                                                     command);
        _model->commands[index].names =
            _model->name_count - _model->commands[index].first_name;
        _model->commands[index].first_entry = _model->entry_count;
        status = (status == D_ARG_OK)
                 ? d_arg_internal_complete_options(_model,
                                                   index,
                                                   command)
                 : status;
        _model->commands[index].entries =
            _model->entry_count - _model->commands[index].first_entry;
    }

    // a model half made is no model
    if (status != D_ARG_OK)
    {
        d_arg_internal_complete_release(_model);
    }

    return status;
}

/*
d_arg_internal_complete_release
  The arrays and the copied choices freed; the model left empty.
*/
void
d_arg_internal_complete_release(
    struct d_arg_internal_model* _model
)
{
    const struct d_arg_internal_model empty = D_INTERNAL_ARG_MODEL_INIT;

    // nothing to release
    if (!_model)
    {
        return;
    }

    free(_model->commands);
    free(_model->names);
    free(_model->options);
    free(_model->entries);
    d_arg_string_list_release(&_model->choices);
    *_model = empty;

    return;
}

//==============================================================================
// 3.  WRITING
//==============================================================================

/*
d_arg_internal_complete_ident
  ASCII letters and digits as they are, `_` doubled, and any other byte `_`
and its two hexadecimal digits.
*/
int
d_arg_internal_complete_ident(
    struct d_pack_text   _program,
    struct d_arg_buffer* _out
)
{
    // each byte
    for (size_t index = 0u; index < _program.length; ++index)
    {
        const char          c    = _program.data[index];
        const unsigned char byte = (unsigned char)c;

        if ( (d_arg_internal_is_alpha(c)) ||
             (d_arg_internal_is_digit(c)) )
        {
            (void)d_arg_buffer_append_char(_out,
                                           c);
        }
        else if (c == '_')
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "__");
        }
        else
        {
            (void)d_arg_buffer_append_char(_out,
                                           '_');
            (void)d_arg_buffer_append_char(_out,
                                           HEX_DIGITS[byte >> 4]);
            (void)d_arg_buffer_append_char(_out,
                                           HEX_DIGITS[byte & 0x0Fu]);
        }
    }

    return (_out->failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/*
d_arg_internal_complete_lines
  Each line with its `@@`s replaced, and a newline after it.
*/
int
d_arg_internal_complete_lines(
    const char* const*   _lines,
    size_t               _count,
    struct d_pack_text   _ident,
    struct d_arg_buffer* _out
)
{
    // each line
    for (size_t index = 0u; index < _count; ++index)
    {
        const char* rest = _lines[index];
        const char* mark = strstr(rest,
                                  "@@");

        // each mark in the line
        while (mark != NULL)
        {
            (void)d_arg_buffer_append(_out,
                                      rest,
                                      (size_t)(mark - rest));
            (void)d_arg_buffer_append_text(_out,
                                           _ident);
            rest = mark + 2;
            mark = strstr(rest,
                          "@@");
        }

        (void)d_arg_buffer_append_cstr(_out,
                                       rest);
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    return (_out->failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/*
d_arg_internal_complete_line_text
  The text up to its first line break, trimmed.
*/
struct d_pack_text
d_arg_internal_complete_line_text(
    const char* _text
)
{
    const struct d_pack_text text = d_arg_internal_text_of(_text);
    size_t                   end  = 0u;

    // to the first line break
    while ( (end < text.length) &&
            (text.data[end] != '\n') &&
            (text.data[end] != '\r') )
    {
        ++end;
    }

    return d_arg_internal_trim(d_arg_internal_span(text.data,
                                                   end));
}

/*
d_arg_internal_complete_base
  The name a path runs a program by: its last component, after its last
`/` or `\`, less a ".exe" in any case.
*/
D_STATIC struct d_pack_text
d_arg_internal_complete_base(
    struct d_pack_text _path
)
{
    size_t start = 0u;
    size_t end   = _path.length;

    // after the last separator
    for (size_t index = 0u; index < end; ++index)
    {
        if ( (_path.data[index] == '/') ||
             (_path.data[index] == '\\') )
        {
            start = index + 1u;
        }
    }

    // less ".exe"
    if ( (end - start > 4u) &&
         (d_arg_internal_equal_nocase_cstr(
              d_arg_internal_span(_path.data + end - 4u,
                                  4u),
              ".exe")) )
    {
        end -= 4u;
    }

    return d_arg_internal_span(_path.data + start,
                               end - start);
}

/*
d_arg_internal_complete_script
  The script for a shell, appended to a buffer: the model built, then
written by the shell's writer, for the program given -- or named by the
command's name, as d_arg_internal_complete_base reads it.
*/
D_STATIC int
d_arg_internal_complete_script(
    const struct d_arg_command* _command,
    int                         _shell,
    struct d_pack_text          _program,
    struct d_arg_buffer*        _out
)
{
    struct d_arg_internal_model model  = D_INTERNAL_ARG_MODEL_INIT;
    const struct d_pack_text    name   =
        (_program.length > 0u)
        ? _program
        : d_arg_internal_complete_base(
              d_arg_internal_text_of(d_arg_command_name(_command)));
    int                         status = D_ARG_OK;

    // a shell, and a name to complete
    if ( (_shell < 0)                         ||
         (_shell >= (int)D_ARG_SHELL_COUNT)   ||
         (name.length == 0u) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    status = d_arg_internal_complete_build(&model,
                                           _command);
    status = (status == D_ARG_OK)
             ? WRITERS[_shell](&model,
                               name,
                               _out)
             : status;
    d_arg_internal_complete_release(&model);

    return status;
}

/*
d_arg_completion_script
  The script built in a buffer, then written to the sink at once.
*/
int
d_arg_completion_script(
    const struct d_arg_command* _command,
    int                         _shell,
    struct d_pack_text          _program,
    struct d_pack_sink          _sink
)
{
    // validate the parameters
    if ( (!_command) ||
         ( (_program.length > 0u) &&
           (!_program.data) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer script = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_complete_script(_command,
                                                                _shell,
                                                                _program,
                                                                &script);

    status = (status == D_ARG_OK)
             ? d_arg_sink_write(_sink,
                                script.data,
                                script.length)
             : status;
    d_arg_buffer_release(&script);

    return status;
}

//==============================================================================
// 4.  THE COMPLETION FLAG
//==============================================================================

/*
d_arg_internal_complete_shell_check
  A shell's name passes; anything else is rejected as CLI11's IsMember
rejects a value outside its set.
*/
D_STATIC int
d_arg_internal_complete_shell_check(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    // a shell's name
    if (d_arg_shell_from_name(_validation->value) >= 0)
    {
        return D_ARG_OK;
    }

    return d_arg_validation_rejectf(
               _validation,
               "%.*s not in %s",
               d_arg_internal_print_length(_validation->value),
               _validation->value.data,
               SHELL_SET);
}

/*
d_arg_internal_complete_shell_choices
  The shells' names, each listed.
*/
D_STATIC int
d_arg_internal_complete_shell_choices(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    int status = D_ARG_OK;

    (void)_validator;

    // each name
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < SHELL_NAME_COUNT);
         ++index)
    {
        status = _each(d_arg_internal_text_of(SHELL_NAMES[index]),
                       _context);
    }

    return status;
}

/*
d_arg_internal_complete_requested
  The completion option's callback: the script for the shell named last,
for the program as the parse was given its name -- `argv[0]`, as
d_arg_internal_complete_base reads it -- or else as the root is named,
becomes the parse's report, its storage handed over, which ends the parse
with D_ARG_CALL_FOR_COMPLETION.
*/
D_STATIC int
d_arg_internal_complete_requested(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    struct d_arg_parser* const parser = d_arg_internal_parser_of(_option);
    struct d_arg_buffer        script = D_ARG_BUFFER_INIT;
    int                        status = D_ARG_OK;

    (void)_context;

    // a parser to report to, and a value
    if ( (!parser) ||
         (_count == 0u) )
    {
        return D_ARG_OK;
    }

    status = d_arg_internal_complete_script(
                 &parser->command,
                 d_arg_shell_from_name(_results[_count - 1u]),
                 d_arg_internal_complete_base(
                     d_arg_internal_text_of(parser->program)),
                 &script);
    status = (status == D_ARG_OK)
             ? d_arg_internal_fail_adopt(&parser->command,
                                         _option,
                                         D_ARG_CALL_FOR_COMPLETION,
                                         d_arg_buffer_detach(&script))
             : d_arg_internal_fail(&parser->command,
                                   _option,
                                   status,
                                   NULL);
    d_arg_buffer_release(&script);

    return status;
}

/*
d_arg_internal_complete_set_up
  What makes an option the completion option: a shell's name for a value,
named SHELL in help and listed for completion; not configurable; called
back first, as the help flag is, with the script.
*/
D_STATIC void
d_arg_internal_complete_set_up(
    struct d_arg_option* _option
)
{
    struct d_arg_validator check = d_arg_validator_custom(
                                       d_arg_internal_complete_shell_check,
                                       NULL,
                                       NULL);

    d_arg_validator_set_choices(&check,
                                d_arg_internal_complete_shell_choices);
    (void)d_arg_option_set_type_name(_option,
                                     "SHELL");
    (void)d_arg_option_check(_option,
                             check,
                             NULL);
    d_arg_option_set_configurable(_option,
                                  false);
    d_arg_option_set_callback_priority(_option,
                                       (int)D_ARG_PRIORITY_FIRST);
    d_arg_option_set_callback(_option,
                              d_arg_internal_complete_requested,
                              NULL,
                              NULL);

    return;
}

/*
d_arg_parser_set_completion_flag
  The option made before removed, wherever it was moved; then, for names,
a new one on the root, set up as the completion option.
*/
struct d_arg_option*
d_arg_parser_set_completion_flag(
    struct d_arg_parser* _parser,
    const char*          _names,
    const char*          _description
)
{
    // validate the parameters
    if (!_parser)
    {
        return NULL;
    }

    // the option made before
    if (_parser->completion != NULL)
    {
        (void)d_arg_command_remove_option(_parser->completion->parent,
                                          _parser->completion);
        _parser->completion = NULL;
    }

    // no names, no option
    if ( (!_names) ||
         (_names[0] == '\0') )
    {
        return NULL;
    }

    struct d_arg_option* const option = d_arg_command_add_option(
                                            &_parser->command,
                                            _names,
                                            (_description != NULL)
                                            ? _description
                                            : DEFAULT_DESCRIPTION);

    // an option made is set up
    if (option != NULL)
    {
        d_arg_internal_complete_set_up(option);
        _parser->completion = option;
    }

    return option;
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION
