/*******************************************************************************
* djinterp [parse]                                            arg_command_tree.c
*
* A command's special options and its place in the tree, declared in
* arg_command.h (3.3 and 4).
*   The help, help-all and version flags and the configuration option, each
* replacing the one before as CLI11's set_help_flag, set_version_flag and
* set_config replace it; then subcommands and option groups, made, moved,
* found and removed as CLI11's add_subcommand, add_option_group,
* Option_group's add_option and add_subcommand, get_subcommand and
* remove_subcommand do it, with every pointer into a removed subtree
* dropped.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_command_tree.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // strchr, strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "./arg_internal.h"  // failures, lists, commands, characters


//==============================================================================
// 1.  SPECIAL OPTIONS
//==============================================================================

/*
d_arg_internal_command_names_given
  Whether a special option is named at all: NULL and "" name nothing, and
remove it.
*/
D_STATIC bool
d_arg_internal_command_names_given(
    const char* _names
)
{
    return ( (_names != NULL) &&
             (_names[0] != '\0') );
}

/*
d_arg_internal_command_drop_special
  The first half of CLI11's set_help_flag and its kin: the option a slot
points at removed -- when the command still owns it, for a version flag
moved into an option group stays there, as in CLI11 -- and the slot
emptied.
*/
D_STATIC void
d_arg_internal_command_drop_special(
    struct d_arg_command* _command,
    struct d_arg_option** _slot
)
{
    // the option the slot points at
    if (*_slot != NULL)
    {
        (void)d_arg_command_remove_option(_command,
                                          *_slot);
    }

    *_slot = NULL;

    return;
}

/*
d_arg_internal_command_mark_special
  The second half: a special option that was made is not configurable and
runs first.
*/
D_STATIC struct d_arg_option*
d_arg_internal_command_mark_special(
    struct d_arg_option* _option
)
{
    // only an option that was made
    if (_option != NULL)
    {
        d_arg_option_set_configurable(_option,
                                      false);
        d_arg_option_set_callback_priority(_option,
                                           (int)D_ARG_PRIORITY_FIRST);
    }

    return _option;
}

/*
d_arg_command_set_help_flag
  CLI11's set_help_flag: the old flag removed, then, for a name, a new one.
*/
struct d_arg_option*
d_arg_command_set_help_flag(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description
)
{
    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    d_arg_internal_command_drop_special(_command,
                                        &_command->help);

    // a name, for a new flag
    if (d_arg_internal_command_names_given(_names))
    {
        _command->help = d_arg_internal_command_mark_special(
                             d_arg_command_add_flag(_command,
                                                    _names,
                                                    _description));
    }

    return _command->help;
}

/*
d_arg_command_set_help_all_flag
  CLI11's set_help_all_flag, as set_help_flag.
*/
struct d_arg_option*
d_arg_command_set_help_all_flag(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description
)
{
    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    d_arg_internal_command_drop_special(_command,
                                        &_command->help_all);

    // a name, for a new flag
    if (d_arg_internal_command_names_given(_names))
    {
        _command->help_all = d_arg_internal_command_mark_special(
                                 d_arg_command_add_flag(_command,
                                                        _names,
                                                        _description));
    }

    return _command->help_all;
}

/*
d_arg_internal_command_drop_version
  The version flag removed, with its text and its function.
*/
D_STATIC void
d_arg_internal_command_drop_version(
    struct d_arg_command* _command
)
{
    d_arg_internal_command_drop_special(_command,
                                        &_command->version);
    free(_command->version_text);
    _command->version_text = NULL;
    _command->version_fn   = NULL;
    d_arg_internal_context_release(&_command->version_context,
                                   &_command->version_release);

    return;
}

/*
d_arg_internal_command_version_noop
  CLI11's empty lambda behind the version flag: the flag's use is what the
parse acts on, and nothing is left for its callback to do.
*/
D_STATIC int
d_arg_internal_command_version_noop(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;
    (void)_context;

    return D_ARG_OK;
}

/*
d_arg_internal_command_add_version
  The flag CLI11's set_version_flag adds once the version is in place, with
CLI11's description unless one is given; a refused flag takes the version
with it, which CLI11's version() would no longer show.
*/
D_STATIC struct d_arg_option*
d_arg_internal_command_add_version(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description
)
{
    struct d_arg_option* const flag = d_arg_command_add_flag_callback(
        _command,
        _names,
        d_arg_internal_command_version_noop,
        NULL,
        NULL,
        (_description != NULL)
        ? _description
        : "Display program version information and exit");

    _command->version = d_arg_internal_command_mark_special(flag);

    // a refused flag leaves no version behind
    if (!flag)
    {
        d_arg_internal_command_drop_version(_command);
    }

    return flag;
}

/*
d_arg_command_set_version_flag
  CLI11's set_version_flag(name, version, help): the old flag removed with
its version, then, for a name, the version copied and the new flag added.
*/
struct d_arg_option*
d_arg_command_set_version_flag(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _version,
    const char*           _description
)
{
    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    d_arg_internal_command_drop_version(_command);

    // no name, no flag
    if (!d_arg_internal_command_names_given(_names))
    {
        return NULL;
    }

    _command->version_text = (_version != NULL)
                             ? d_arg_internal_copy_cstr(_version)
                             : NULL;

    // check if memory allocation was successful
    if ( (_version != NULL) &&
         (!_command->version_text) )
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             D_ARG_ERROR_OUT_OF_MEMORY,
                                             NULL);

        return NULL;
    }

    return d_arg_internal_command_add_version(_command,
                                              _names,
                                              _description);
}

/*
d_arg_command_set_version_fn
  CLI11's set_version_flag(name, function, help): as set_version_flag, with
the function kept in place of the text. The old flag's context is freed
unless it is the one given again; a context nothing keeps is freed at once.
*/
struct d_arg_option*
d_arg_command_set_version_fn(
    struct d_arg_command* _command,
    const char*           _names,
    d_arg_command_text_fn _write,
    void*                 _context,
    d_arg_release_fn      _release,
    const char*           _description
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // validate the parameters; the context goes either way
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return NULL;
    }

    // the same context given again is not freed with the old flag
    if (_command->version_context == _context)
    {
        _command->version_context = NULL;
        _command->version_release = NULL;
    }

    d_arg_internal_command_drop_version(_command);

    // no name, no flag, and nothing to keep the context for
    if (!d_arg_internal_command_names_given(_names))
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return NULL;
    }

    _command->version_fn      = _write;
    _command->version_context = _context;
    _command->version_release = _release;

    return d_arg_internal_command_add_version(_command,
                                              _names,
                                              _description);
}

/*
d_arg_internal_command_config_default
  The default file CLI11's set_config gives its option: the default text,
and the callback forced, so that the file is read when none is given.
*/
D_STATIC int
d_arg_internal_command_config_default(
    struct d_arg_option* _option,
    const char*          _default_file
)
{
    // no default file, nothing to set
    if ( (!_default_file) ||
         (_default_file[0] == '\0') )
    {
        return D_ARG_OK;
    }

    const int status = d_arg_option_set_default_text(_option,
                                                     _default_file);

    // the callback is forced once the default is in place
    if (status == D_ARG_OK)
    {
        d_arg_option_set_force_callback(_option,
                                        true);
    }

    return status;
}

/*
d_arg_command_set_config
  CLI11's set_config: the old option removed; for a name, a new one --
required only when asked, as CLI11 only ever sets it, with its default
file, not configurable, and taking its files last first. An option whose
default could not be kept is removed again.
*/
struct d_arg_option*
d_arg_command_set_config(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _default_file,
    const char*           _description,
    bool                  _required
)
{
    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    d_arg_internal_command_drop_special(_command,
                                        &_command->config);

    // no name, no option
    if (!d_arg_internal_command_names_given(_names))
    {
        return NULL;
    }

    struct d_arg_option* const option = d_arg_command_add_option(
        _command,
        _names,
        (_description != NULL)
        ? _description
        : "Read an ini file");

    // refused, recorded; or without the default it was given
    if ( (!option) ||
         (d_arg_internal_command_config_default(option,
                                                _default_file) != D_ARG_OK) )
    {
        (void)d_arg_command_remove_option(_command,
                                          option);

        return NULL;
    }

    // CLI11 only ever sets it
    if (_required)
    {
        d_arg_option_set_required(option,
                                  true);
    }

    d_arg_option_set_configurable(option,
                                  false);
    d_arg_option_set_multi_option_policy(option,
                                         (int)D_ARG_MULTI_REVERSE);
    _command->config = option;

    return option;
}

//==============================================================================
// 2.  MAKING SUBCOMMANDS AND GROUPS
//==============================================================================

/*
d_arg_internal_command_check_subcommand_name
  CLI11's checks in add_subcommand: a name that is not valid is refused for
its first character when that cannot start a name, and otherwise for the
first character -- the first itself included -- that cannot follow in one.
*/
D_STATIC int
d_arg_internal_command_check_subcommand_name(
    const struct d_arg_command* _command,
    const char*                 _name
)
{
    const struct d_pack_text name = d_arg_internal_text_of(_name);
    size_t                   at   = 0u;

    // no name, or a valid one
    if ( (name.length == 0u) ||
         (d_arg_internal_valid_name(name)) )
    {
        return D_ARG_OK;
    }

    // a first character that cannot start a name
    if (!d_arg_internal_valid_first_char(name.data[0]))
    {
        return d_arg_internal_fail_definition(
                   _command,
                   NULL,
                   D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                   "Subcommand name starts with invalid character, '!' "
                   "and '-' and control characters");
    }

    // the first character that cannot follow, which an invalid name has
    while ( (at < name.length) &&
            (d_arg_internal_valid_later_char(name.data[at])) )
    {
        at++;
    }

    return d_arg_internal_fail_definition(
               _command,
               NULL,
               D_ARG_ERROR_INCORRECT_CONSTRUCTION,
               "Subcommand name contains invalid character ('%c'), all "
               "characters are allowed except'=',':','{','}', ' ', and "
               "control characters",
               name.data[at]);
}

/*
d_arg_internal_command_discard
  A subcommand that never joined the tree, or has left it: released, then
freed.
*/
D_STATIC void
d_arg_internal_command_discard(
    struct d_arg_command* _subcommand
)
{
    d_arg_internal_command_release(_subcommand);
    free(_subcommand);

    return;
}

/*
d_arg_internal_command_make
  CLI11's App constructor for a subcommand: the record allocated and made
under `_command`; a refused one has recorded its failure, and is
discarded.
*/
D_STATIC struct d_arg_command*
d_arg_internal_command_make(
    struct d_arg_command* _command,
    const char*           _name,
    const char*           _description
)
{
    struct d_arg_command* const subcommand =
        (struct d_arg_command*)malloc(sizeof(*subcommand));

    // check if memory allocation was successful
    if (!subcommand)
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             D_ARG_ERROR_OUT_OF_MEMORY,
                                             NULL);

        return NULL;
    }

    // made, or recorded and discarded
    if (d_arg_internal_command_init(subcommand,
                                    _command->parser,
                                    _command,
                                    _name,
                                    _description) != D_ARG_OK)
    {
        d_arg_internal_command_discard(subcommand);

        return NULL;
    }

    return subcommand;
}

/*
d_arg_internal_command_adopt
  CLI11's add_subcommand(App_p): the new subcommand's names checked against
the subcommands of the command -- or, for an option group, of its
fallthrough parent -- then the subcommand kept; refused, it is discarded.
*/
D_STATIC struct d_arg_command*
d_arg_internal_command_adopt(
    struct d_arg_command* _command,
    struct d_arg_command* _subcommand
)
{
    const char* const clash = d_arg_internal_compare_subcommand_names(
                                  _subcommand,
                                  (d_arg_command_is_option_group(_command))
                                  ? d_arg_internal_fallthrough_parent(_command)
                                  : _command);
    int               status = D_ARG_OK;

    // a name or alias taken already, or a list that could not grow
    if (clash != NULL)
    {
        status = d_arg_internal_fail_definition(
                     _command,
                     NULL,
                     D_ARG_ERROR_OPTION_ALREADY_ADDED,
                     "subcommand name or alias matches existing "
                     "subcommand: %s is already added",
                     clash);
    }
    else
    {
        status = d_arg_internal_command_push(&_command->subcommands,
                                             _subcommand);
        status = (status == D_ARG_OK)
                 ? D_ARG_OK
                 : d_arg_internal_fail_definition(_command,
                                                  NULL,
                                                  status,
                                                  NULL);
    }

    // a refused subcommand is not kept
    if (status != D_ARG_OK)
    {
        d_arg_internal_command_discard(_subcommand);

        return NULL;
    }

    return _subcommand;
}

/*
d_arg_command_add_subcommand
  CLI11's add_subcommand(name, desc): the name checked, the subcommand
made, then adopted.
*/
struct d_arg_command*
d_arg_command_add_subcommand(
    struct d_arg_command* _command,
    const char*           _name,
    const char*           _description
)
{
    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    struct d_arg_command* const subcommand =
        (d_arg_internal_command_check_subcommand_name(_command,
                                                      _name) == D_ARG_OK)
        ? d_arg_internal_command_make(_command,
                                      _name,
                                      _description)
        : NULL;

    return (subcommand != NULL)
           ? d_arg_internal_command_adopt(_command,
                                          subcommand)
           : NULL;
}

/*
d_arg_internal_command_make_group
  CLI11's Option_group constructor after App's, then what add_option_group
changes: the group's name, no help-all flag for a hidden or `+` group, and
for every group no help flag, no footer and no fallthrough.
*/
D_STATIC int
d_arg_internal_command_make_group(
    struct d_arg_command* _group,
    const char*           _group_name
)
{
    char* const group = d_arg_internal_copy_cstr(_group_name);

    // check if memory allocation was successful
    if (!group)
    {
        return d_arg_internal_fail_definition(_group->parent,
                                              NULL,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    free(_group->group);
    _group->group = group;

    // a hidden or `+` group has no help-all flag
    if ( (group[0] == '\0') ||
         (group[0] == '+') )
    {
        (void)d_arg_command_set_help_all_flag(_group,
                                              NULL,
                                              NULL);
    }

    (void)d_arg_command_set_help_flag(_group,
                                      NULL,
                                      NULL);
    free(_group->footer);
    _group->footer = NULL;
    _group->flags &= ~D_ARG_COMMAND_FALLTHROUGH;

    return D_ARG_OK;
}

/*
d_arg_command_add_option_group
  CLI11's add_option_group: the name checked for newlines, the group made
nameless under the command and finished, then adopted as a subcommand is.
*/
struct d_arg_command*
d_arg_command_add_option_group(
    struct d_arg_command* _command,
    const char*           _group_name,
    const char*           _description
)
{
    const char* const name = (_group_name != NULL) ? _group_name : "";

    // validate the parameters
    if (!_command)
    {
        return NULL;
    }

    // a newline is refused
    if (strchr(name,
               '\n') != NULL)
    {
        (void)d_arg_internal_fail_definition(
                  _command,
                  NULL,
                  D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                  "option group names may not contain newlines or null "
                  "characters");

        return NULL;
    }

    struct d_arg_command* const group = d_arg_internal_command_make(
                                            _command,
                                            NULL,
                                            _description);

    // made, finished and adopted; or recorded
    if (!group)
    {
        return NULL;
    }

    if (d_arg_internal_command_make_group(group,
                                          name) != D_ARG_OK)
    {
        d_arg_internal_command_discard(group);

        return NULL;
    }

    return d_arg_internal_command_adopt(_command,
                                        group);
}

//==============================================================================
// 3.  MOVING INTO A GROUP
//==============================================================================

/*
d_arg_internal_command_move_clash
  CLI11's refusal of an option whose name the group has already, worded as
CLI11 words it, with the option's display name.
*/
D_STATIC int
d_arg_internal_command_move_clash(
    const struct d_arg_command* _group,
    const struct d_arg_option*  _option
)
{
    struct d_arg_buffer name   = D_ARG_BUFFER_INIT;
    int                 status = D_ARG_OK;

    (void)d_arg_option_name(_option,
                            0u,
                            &name);
    status = d_arg_internal_fail_definition(
                 _group,
                 NULL,
                 D_ARG_ERROR_OPTION_ALREADY_ADDED,
                 "option was not located: %s is already added",
                 d_arg_buffer_text(&name).data);
    d_arg_buffer_release(&name);

    return status;
}

/*
d_arg_internal_command_group_has
  CLI11's `*v == *opt_p` over a group's options: whether one shares a name
with `_option`.
*/
D_STATIC bool
d_arg_internal_command_group_has(
    const struct d_arg_command* _group,
    const struct d_arg_option*  _option
)
{
    // each option of the group
    for (size_t index = 0u; index < _group->options.count; ++index)
    {
        if (d_arg_option_matching_name(_group->options.items[index],
                                       _option) != NULL)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_command_move_check
  CLI11's _move_option checks, in its order: the help options stay, the
configuration option stays, only the parent's own option moves, and not
into a group that has its name already.
*/
D_STATIC int
d_arg_internal_command_move_check(
    const struct d_arg_command* _group,
    const struct d_arg_option*  _option
)
{
    const struct d_arg_command* const parent = _group->parent;

    // the help options stay
    if ( (_option == parent->help) ||
         (_option == parent->help_all) )
    {
        return d_arg_internal_fail_definition(
                   _group,
                   NULL,
                   D_ARG_ERROR_OPTION_ALREADY_ADDED,
                   "cannot move help options is already added");
    }

    // so does the configuration option
    if (_option == parent->config)
    {
        return d_arg_internal_fail_definition(
                   _group,
                   NULL,
                   D_ARG_ERROR_OPTION_ALREADY_ADDED,
                   "cannot move config file options is already added");
    }

    // only the parent's own option moves
    if (!d_arg_internal_option_has(&parent->options,
                                   _option))
    {
        return d_arg_internal_fail_definition(
                   _group,
                   NULL,
                   D_ARG_ERROR_OPTION_NOT_FOUND,
                   "could not locate the given Option not found");
    }

    return (d_arg_internal_command_group_has(_group,
                                             _option))
           ? d_arg_internal_command_move_clash(_group,
                                               _option)
           : D_ARG_OK;
}

/*
d_arg_command_group_add_option
  Option_group::add_option(Option*): CLI11's _move_option on the group's
parent, the option then the group's.
*/
int
d_arg_command_group_add_option(
    struct d_arg_command* _group,
    struct d_arg_option*  _option
)
{
    // validate the parameters
    if (!d_arg_command_is_option_group(_group))
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // CLI11's refusal of nothing
    if (!_option)
    {
        return d_arg_internal_fail_definition(
                   _group,
                   NULL,
                   D_ARG_ERROR_OPTION_NOT_FOUND,
                   "the option is NULL not found");
    }

    int status = d_arg_internal_command_move_check(_group,
                                                   _option);

    // into the group first, so that a refused allocation moves nothing
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_push(&_group->options,
                                            _option);
        status = (status == D_ARG_OK)
                 ? D_ARG_OK
                 : d_arg_internal_fail_definition(_group,
                                                  NULL,
                                                  status,
                                                  NULL);
    }

    // then out of the parent
    if (status == D_ARG_OK)
    {
        (void)d_arg_internal_option_remove(&_group->parent->options,
                                           _option);
        _option->parent = _group;
    }

    return status;
}

/*
d_arg_internal_command_within
  Whether a command is `_ancestor` or lies below it.
*/
D_STATIC bool
d_arg_internal_command_within(
    const struct d_arg_command* _command,
    const struct d_arg_command* _ancestor
)
{
    const struct d_arg_command* at = _command;

    // up toward the root
    while ( (at != NULL) &&
            (at != _ancestor) )
    {
        at = at->parent;
    }

    return (at != NULL);
}

/*
d_arg_internal_command_unlink
  CLI11's first loop in remove_subcommand: each subcommand of the command
drops its exclusion of `_subcommand` -- both halves, as CLI11's
remove_excludes drops them -- and its need of it.
*/
D_STATIC void
d_arg_internal_command_unlink(
    struct d_arg_command* _command,
    struct d_arg_command* _subcommand
)
{
    // each subcommand of the command
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const sibling =
            _command->subcommands.items[index];

        if (d_arg_internal_command_remove(&sibling->exclude_subcommands,
                                          _subcommand))
        {
            (void)d_arg_internal_command_remove(
                      &_subcommand->exclude_subcommands,
                      sibling);
        }

        (void)d_arg_internal_command_remove(&sibling->need_subcommands,
                                            _subcommand);
    }

    return;
}

/*
d_arg_command_group_add_subcommand
  Option_group::add_subcommand(App*), made safe: the names are checked and
the group's list grown before anything moves; then the subcommand leaves
its parent as CLI11's remove_subcommand has it leave, and joins the group.
*/
int
d_arg_command_group_add_subcommand(
    struct d_arg_command* _group,
    struct d_arg_command* _subcommand
)
{
    // validate the parameters
    if ( (!d_arg_command_is_option_group(_group))           ||
         (!_subcommand)                                      ||
         (!_subcommand->parent)                              ||
         (_subcommand->parser != _group->parser)             ||
         (d_arg_internal_command_within(_group,
                                        _subcommand)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const char* const clash = d_arg_internal_compare_subcommand_names(
                                  _subcommand,
                                  d_arg_internal_fallthrough_parent(_group));

    // a name or alias taken already: nothing moves
    if (clash != NULL)
    {
        return d_arg_internal_fail_definition(
                   _group,
                   NULL,
                   D_ARG_ERROR_OPTION_ALREADY_ADDED,
                   "subcommand name or alias matches existing "
                   "subcommand: %s is already added",
                   clash);
    }

    const int status = d_arg_internal_command_push(&_group->subcommands,
                                                   _subcommand);

    // a list that could not grow: nothing moves
    if (status != D_ARG_OK)
    {
        return d_arg_internal_fail_definition(_group,
                                              NULL,
                                              status,
                                              NULL);
    }

    d_arg_internal_command_unlink(_subcommand->parent,
                                  _subcommand);
    (void)d_arg_internal_command_remove(&_subcommand->parent->subcommands,
                                        _subcommand);
    _subcommand->parent = _group;

    return D_ARG_OK;
}

//==============================================================================
// 4.  FINDING AND REMOVING
//==============================================================================

/*
d_arg_command_subcommand
  CLI11's get_subcommand_no_throw(name): _find_subcommand passing over
neither disabled nor used subcommands.
*/
struct d_arg_command*
d_arg_command_subcommand(
    const struct d_arg_command* _command,
    const char*                 _name
)
{
    // nothing to find without a name
    if (!_name)
    {
        return NULL;
    }

    return d_arg_internal_find_subcommand(_command,
                                          d_arg_internal_text_of(_name),
                                          false,
                                          false);
}

/*
d_arg_command_subcommand_at
  CLI11's get_subcommand(index), NULL past the end.
*/
struct d_arg_command*
d_arg_command_subcommand_at(
    const struct d_arg_command* _command,
    size_t                      _index
)
{
    return ( (_command != NULL) &&
             (_index < _command->subcommands.count) )
           ? _command->subcommands.items[_index]
           : NULL;
}

/*
d_arg_command_option_group
  CLI11's get_option_group: the first nameless subcommand whose group reads
`_group_name`, an unset group reading "SUBCOMMANDS".
*/
struct d_arg_command*
d_arg_command_option_group(
    const struct d_arg_command* _command,
    const char*                 _group_name
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_group_name) )
    {
        return NULL;
    }

    // each nameless subcommand, by its group
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const group = _command->subcommands.items[index];
        const char* const           text  = (group->group != NULL)
                                            ? group->group
                                            : "SUBCOMMANDS";

        if ( (d_arg_command_is_option_group(group)) &&
             (strcmp(text,
                     _group_name) == 0) )
        {
            return group;
        }
    }

    return NULL;
}

/*
d_arg_internal_command_forget_command
  Every reference to a command in a command and below it: needs, excludes,
the last parse's list of subcommands, and a trigger's targets.
*/
D_STATIC void
d_arg_internal_command_forget_command(
    struct d_arg_command*       _command,
    const struct d_arg_command* _gone
)
{
    (void)d_arg_internal_command_remove(&_command->exclude_subcommands,
                                        _gone);
    (void)d_arg_internal_command_remove(&_command->need_subcommands,
                                        _gone);

    d_arg_internal_command_untrigger(_command,
                                     _gone);

    // every place it took among the parsed subcommands
    while (d_arg_internal_command_remove(&_command->parsed_subcommands,
                                         _gone))
    {
    }

    // the subcommands', in turn
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        d_arg_internal_command_forget_command(
            _command->subcommands.items[index],
            _gone);
    }

    return;
}

/*
d_arg_internal_command_forget_tree
  Every reference from the tree under `_root` into the subtree of `_gone`,
which has left it: to each of its options, to each command below it, and
to itself.
*/
D_STATIC void
d_arg_internal_command_forget_tree(
    struct d_arg_command*       _root,
    const struct d_arg_command* _gone
)
{
    // its options
    for (size_t index = 0u; index < _gone->options.count; ++index)
    {
        d_arg_internal_command_forget_option(_root,
                                             _gone->options.items[index]);
    }

    // the commands below it
    for (size_t index = 0u; index < _gone->subcommands.count; ++index)
    {
        d_arg_internal_command_forget_tree(_root,
                                           _gone->subcommands.items[index]);
    }

    d_arg_internal_command_forget_command(_root,
                                          _gone);

    return;
}

/*
d_arg_command_remove_subcommand
  CLI11's remove_subcommand -- its subcommands' links to the subcommand
dropped first, owned or not -- made safe: an owned subcommand leaves the
tree, every reference into its subtree goes, and it is released.
*/
bool
d_arg_command_remove_subcommand(
    struct d_arg_command* _command,
    struct d_arg_command* _subcommand
)
{
    struct d_arg_command* root = _command;

    // validate the parameters
    if ( (!_command) ||
         (!_subcommand) )
    {
        return false;
    }

    d_arg_internal_command_unlink(_command,
                                  _subcommand);

    // only a subcommand the command owns
    if (!d_arg_internal_command_remove(&_command->subcommands,
                                       _subcommand))
    {
        return false;
    }

    // from the root down
    while (root->parent != NULL)
    {
        root = root->parent;
    }

    d_arg_internal_command_forget_tree(root,
                                       _subcommand);
    d_arg_internal_command_discard(_subcommand);

    return true;
}
