/*******************************************************************************
* djinterp [parse]                                                 arg_command.c
*
* Commands' lives and lookups, declared in arg_command.h (3.4.1, 6.1.3 and
* 6.3.2) and arg_internal.h (7.3).
*   Making a command -- a root with CLI11's App defaults, or a subcommand
* inheriting what CLI11's App constructor copies from its parent -- and
* releasing one; matching names against a command's; finding subcommands
* and options the way CLI11's _find_subcommand and get_option_no_throw find
* them; and collecting the options a command lists -- every one, or those a
* filter passes, as CLI11's get_options(filter) collects them.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_command.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "./arg_internal.h"  // failures, lists, copies, characters


//==============================================================================
// 1.  LIFETIME
//==============================================================================

// COMMAND_EMPTY
//   constant: a command holding nothing, every list empty and every pointer
// NULL, from which a command is built.
static const struct d_arg_command COMMAND_EMPTY;

/**
 * @brief Gives a new command CLI11's App defaults.
 *
 * @param[out] _command  the command, all of it written.
 * @param[in]  _parser   the parser it belongs to.
 * @param[in]  _parent   its parent; NULL for a root.
 *
 * CLI11's App defaults: errors for extras, configuration extras ignored, no
 * prefix command, a stable start, subcommand fallthrough, Windows-style
 * options where the configuration says, and options starting configurable.
 */
D_STATIC void
d_arg_internal_command_defaults(
    struct d_arg_command* _command,
    struct d_arg_parser*  _parser,
    struct d_arg_command* _parent
)
{
    *_command                                 = COMMAND_EMPTY;
    _command->parser                          = _parser;
    _command->parent                          = _parent;
    _command->allow_extras                    = (int)D_ARG_EXTRAS_ERROR;
    _command->allow_config_extras             = (int)D_ARG_CONFIG_EXTRAS_IGNORE;
    _command->prefix_command                  = (int)D_ARG_PREFIX_OFF;
    _command->startup                         = (int)D_ARG_STARTUP_STABLE;
    _command->flags                           =
        D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH |
        ( (D_INTERNAL_ARG_PARSER_WINDOWS_STYLE == 1)
          ? D_ARG_COMMAND_WINDOWS_STYLE
          : 0u );
    _command->option_defaults.flags           = D_ARG_OPTION_CONFIGURABLE;
    _command->option_defaults.multi_option_policy =
        (int)D_ARG_MULTI_THROW;
    _command->option_defaults.callback_priority   =
        (int)D_ARG_PRIORITY_NORMAL;
    _command->option_defaults.integer_rule        =
        (int)D_INTERNAL_ARG_PARSER_INTEGER_RULE;

    return;
}

/**
 * @brief Makes one of a parent's help flags again on a command.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _source   the parent's flag.
 * @param[in]     _all      whether it is the help-all flag.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when the names could not be
 *         written or the flag could not be made, the setter recording its
 *         refusal.
 *
 * One of the parent's help flags -- the help-all flag when `_all` -- made
 * again on the command under all its names, with their flag values, and its
 * description, as CLI11's App constructor makes it; the setter records what it
 * refuses.
 */
D_STATIC int
d_arg_internal_command_copy_flag(
    struct d_arg_command*      _command,
    const struct d_arg_option* _source,
    bool                       _all
)
{
    struct d_arg_buffer names  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_option_name(_source,
                                                   D_ARG_NAME_ALL,
                                                   &names);

    // the copy, under the same names
    if (status == D_ARG_OK)
    {
        const char* const name        = d_arg_buffer_text(&names).data;
        const char* const description = d_arg_option_description(_source);
        const bool        made        =
            (_all)
            ? (d_arg_command_set_help_all_flag(_command,
                                               name,
                                               description) != NULL)
            : (d_arg_command_set_help_flag(_command,
                                           name,
                                           description) != NULL);

        status = made ? D_ARG_OK : D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_arg_buffer_release(&names);

    return status;
}

/**
 * @brief Copies a parent's help flags, as CLI11's App constructor does.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _parent   the parent.
 * @return D_ARG_OK, or the failure of a flag's copy.
 *
 * The help flags CLI11's App constructor copies from the parent. They are
 * made before anything else is inherited, as in CLI11, so they start from
 * the plain option defaults.
 */
D_STATIC int
d_arg_internal_command_copy_help(
    struct d_arg_command*       _command,
    const struct d_arg_command* _parent
)
{
    int status = D_ARG_OK;

    // the help flag
    if (_parent->help != NULL)
    {
        status = d_arg_internal_command_copy_flag(_command,
                                                  _parent->help,
                                                  false);
    }

    // the help-all flag
    if ( (status == D_ARG_OK) &&
         (_parent->help_all != NULL) )
    {
        status = d_arg_internal_command_copy_flag(_command,
                                                  _parent->help_all,
                                                  true);
    }

    return status;
}

/**
 * @brief Copies a parent's text field.
 *
 * @param[in]  _text   the parent's text; NULL for none.
 * @param[out] _field  receives the copy; NULL for none.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with `*_field` NULL.
 *
 * A copy of a parent's text field; NULL stays NULL.
 */
D_STATIC int
d_arg_internal_command_copy_text(
    const char* _text,
    char**      _field
)
{
    *_field = (_text != NULL)
              ? d_arg_internal_copy_cstr(_text)
              : NULL;

    return ( (_text != NULL) &&
             (*_field == NULL) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/**
 * @brief Copies the rest of what CLI11's App constructor inherits.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _parent   the parent.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when a text could not be
 *         copied, the command then releasable.
 *
 * The rest of what CLI11's App constructor copies: the option defaults, the
 * three modes, the inherited switches, the group, usage and footer text, the
 * formatter and configuration format, and the subcommand maximum. The failure
 * message is not copied: a command without one of its own uses its parent's,
 * so that no command keeps a context its parent may free.
 */
D_STATIC int
d_arg_internal_command_inherit(
    struct d_arg_command*       _command,
    const struct d_arg_command* _parent
)
{
    const uint32_t inherited = D_ARG_COMMAND_IMMEDIATE_CALLBACK          |
                               D_ARG_COMMAND_IGNORE_CASE                 |
                               D_ARG_COMMAND_IGNORE_UNDERSCORE           |
                               D_ARG_COMMAND_FALLTHROUGH                 |
                               D_ARG_COMMAND_VALIDATE_POSITIONALS        |
                               D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS |
                               D_ARG_COMMAND_CONFIGURABLE                |
                               D_ARG_COMMAND_WINDOWS_STYLE               |
                               D_ARG_COMMAND_PREFIX_MATCHING;

    _command->option_defaults        = _parent->option_defaults;
    _command->option_defaults.group  = NULL;
    _command->allow_extras           = _parent->allow_extras;
    _command->allow_config_extras    = _parent->allow_config_extras;
    _command->prefix_command         = _parent->prefix_command;
    _command->flags                  = (_command->flags & ~inherited) |
                                       (_parent->flags & inherited);
    _command->formatter              = _parent->formatter;
    _command->config_format          = _parent->config_format;
    _command->require_subcommand_max = _parent->require_subcommand_max;

    int status = d_arg_internal_command_copy_text(
                     _parent->option_defaults.group,
                     &_command->option_defaults.group);

    // the three texts
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_copy_text(_parent->group,
                                                  &_command->group);
    }

    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_copy_text(_parent->usage,
                                                  &_command->usage);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_command_copy_text(_parent->footer,
                                              &_command->footer)
           : status;
}

/*
d_arg_internal_command_init
  The defaults, the name and description, then -- for a subcommand -- the
help flags and the inherited settings, in CLI11's order; a failure is
recorded against the parent, which outlives a subcommand that failed, and
leaves the command releasable.
*/
int
d_arg_internal_command_init(
    struct d_arg_command* _command,
    struct d_arg_parser*  _parser,
    struct d_arg_command* _parent,
    const char*           _name,
    const char*           _description
)
{
    d_arg_internal_command_defaults(_command,
                                    _parser,
                                    _parent);

    int status = d_arg_internal_command_copy_text(
                     ( (_name != NULL) &&
                       (_name[0] != '\0') )
                     ? _name
                     : NULL,
                     &_command->name);

    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_copy_text(_description,
                                                  &_command->description);
    }

    // a subcommand copies its parent's help flags, then the rest
    if ( (status == D_ARG_OK) &&
         (_parent != NULL) )
    {
        status = d_arg_internal_command_copy_help(_command,
                                                  _parent);
        status = (status == D_ARG_OK)
                 ? d_arg_internal_command_inherit(_command,
                                                  _parent)
                 : status;
    }

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail_definition((_parent != NULL)
                                            ? _parent
                                            : _command,
                                            NULL,
                                            status,
                                            NULL);
}

/**
 * @brief Frees every callback's context.
 *
 * @param[in,out] _command  the command, whose contexts and release functions
 *                          are forgotten.
 *
 * Every callback's context, through the function kept beside it.
 */
D_STATIC void
d_arg_internal_command_release_contexts(
    struct d_arg_command* _command
)
{
    d_arg_internal_context_release(&_command->usage_context,
                                   &_command->usage_release);
    d_arg_internal_context_release(&_command->footer_context,
                                   &_command->footer_release);
    d_arg_internal_context_release(&_command->version_context,
                                   &_command->version_release);
    d_arg_internal_context_release(&_command->preparse_context,
                                   &_command->preparse_release);
    d_arg_internal_context_release(&_command->parse_complete_context,
                                   &_command->parse_complete_release);
    d_arg_internal_context_release(&_command->final_context,
                                   &_command->final_release);
    d_arg_internal_context_release(&_command->pre_callback_context,
                                   &_command->pre_callback_release);
    d_arg_internal_context_release(&_command->failure_context,
                                   &_command->failure_release);

    return;
}

/**
 * @brief Releases the lists that point at records owned elsewhere, and the
 *        parse state.
 *
 * @param[in,out] _command  the command.
 *
 * The lists that point at records owned elsewhere, and the parse state.
 */
D_STATIC void
d_arg_internal_command_release_lists(
    struct d_arg_command* _command
)
{
    d_arg_internal_extra_list_release(&_command->missing);
    d_arg_internal_option_list_release(&_command->parse_order);
    d_arg_internal_command_list_release(&_command->parsed_subcommands);
    d_arg_internal_command_list_release(&_command->exclude_subcommands);
    d_arg_internal_option_list_release(&_command->exclude_options);
    d_arg_internal_command_list_release(&_command->need_subcommands);
    d_arg_internal_option_list_release(&_command->need_options);

    return;
}

/*
d_arg_internal_command_release
  The parser's error reports forget the command, then go the contexts, the
texts, the options, the subcommands in turn, and the lists; the command is
left empty, as if never made.
*/
void
d_arg_internal_command_release(
    struct d_arg_command* _command
)
{
    // nothing to release
    if (!_command)
    {
        return;
    }

    d_arg_internal_unreport(_command->parser,
                            _command,
                            NULL);
    d_arg_internal_command_release_contexts(_command);
    free(_command->name);
    free(_command->description);
    free(_command->group);
    free(_command->usage);
    free(_command->footer);
    free(_command->version_text);
    free(_command->option_defaults.group);
    d_arg_string_list_release(&_command->aliases);

    // the options it owns
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        d_arg_internal_option_destroy(_command->options.items[index]);
    }

    // the subcommands it owns, and theirs
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        d_arg_internal_command_release(_command->subcommands.items[index]);
        free(_command->subcommands.items[index]);
    }

    d_arg_internal_option_list_release(&_command->options);
    d_arg_internal_command_list_release(&_command->subcommands);
    d_arg_internal_command_release_lists(_command);
    *_command = COMMAND_EMPTY;

    return;
}

//==============================================================================
// 2.  NAMES
//==============================================================================

/**
 * @brief Finds the next character a comparison reads.
 *
 * @param[in] _text        the name.
 * @param[in] _at          where to look from.
 * @param[in] _underscore  whether underscores are ignored.
 * @return its position; the name's length when there is none.
 *
 * The position of the next character a comparison reads from `_at` on: past
 * underscores when they are ignored.
 */
D_STATIC size_t
d_arg_internal_command_skip(
    struct d_pack_text _text,
    size_t             _at,
    bool               _underscore
)
{
    size_t at = _at;

    // underscores are not read
    while ( (_underscore)          &&
            (at < _text.length)    &&
            (_text.data[at] == '_') )
    {
        at++;
    }

    return at;
}

/**
 * @brief Compares a name with a command's, as CLI11's check_name_detail does.
 *
 * @param[in] _name        the name given.
 * @param[in] _local       the command's name or alias.
 * @param[in] _case        whether case is ignored.
 * @param[in] _underscore  whether underscores are ignored.
 * @param[in] _prefix      whether a prefix may match.
 * @return D_ARG_NAME_MATCH_EXACT, D_ARG_NAME_MATCH_PREFIX, or
 *         D_ARG_NAME_MATCH_NONE.
 *
 * CLI11's comparison in check_name_detail, without copying either name:
 * underscores skipped and case folded as asked, the name is EXACT when the two
 * read the same, and PREFIX -- when prefixes may match -- when it reads as a
 * proper beginning of `_local`.
 */
D_STATIC int
d_arg_internal_command_compare(
    struct d_pack_text _name,
    struct d_pack_text _local,
    bool               _case,
    bool               _underscore,
    bool               _prefix
)
{
    size_t in_name  = d_arg_internal_command_skip(_name,
                                                  0u,
                                                  _underscore);
    size_t in_local = d_arg_internal_command_skip(_local,
                                                  0u,
                                                  _underscore);

    // the characters both have, folded as asked
    while ( (in_name < _name.length) &&
            (in_local < _local.length) )
    {
        const char a = _case
                       ? d_arg_internal_to_lower(_name.data[in_name])
                       : _name.data[in_name];
        const char b = _case
                       ? d_arg_internal_to_lower(_local.data[in_local])
                       : _local.data[in_local];

        if (a != b)
        {
            return (int)D_ARG_NAME_MATCH_NONE;
        }

        in_name  = d_arg_internal_command_skip(_name,
                                               in_name + 1u,
                                               _underscore);
        in_local = d_arg_internal_command_skip(_local,
                                               in_local + 1u,
                                               _underscore);
    }

    // the same, or the name a beginning of the other
    return (in_name < _name.length)
           ? (int)D_ARG_NAME_MATCH_NONE
           : ( (in_local == _local.length)
               ? (int)D_ARG_NAME_MATCH_EXACT
               : ( _prefix
                   ? (int)D_ARG_NAME_MATCH_PREFIX
                   : (int)D_ARG_NAME_MATCH_NONE ) );
}

/*
d_arg_command_check_name_detail
  CLI11's check_name_detail: the name, then each alias, as
d_arg_internal_command_compare reads them, the first match deciding.
*/
int
d_arg_command_check_name_detail(
    const struct d_arg_command* _command,
    struct d_pack_text          _name
)
{
    // no command matches nothing
    if (!_command)
    {
        return (int)D_ARG_NAME_MATCH_NONE;
    }

    const bool case_free  = d_arg_command_has_flag(_command,
                                                   D_ARG_COMMAND_IGNORE_CASE);
    const bool underscore = d_arg_command_has_flag(
                                _command,
                                D_ARG_COMMAND_IGNORE_UNDERSCORE);
    const bool prefix     = d_arg_command_has_flag(
                                _command,
                                D_ARG_COMMAND_PREFIX_MATCHING);
    int        match      = d_arg_internal_command_compare(
                                _name,
                                d_arg_internal_text_of(_command->name),
                                case_free,
                                underscore,
                                prefix);

    // then the aliases, until one matches
    for (size_t index = 0u;
         (match == (int)D_ARG_NAME_MATCH_NONE) &&
         (index < _command->aliases.count);
         ++index)
    {
        match = d_arg_internal_command_compare(
                    _name,
                    d_arg_internal_text_of(_command->aliases.items[index]),
                    case_free,
                    underscore,
                    prefix);
    }

    return match;
}

/*
d_arg_command_check_name
  Any match at all.
*/
bool
d_arg_command_check_name(
    const struct d_arg_command* _command,
    struct d_pack_text          _name
)
{
    return (d_arg_command_check_name_detail(_command,
                                            _name) !=
            (int)D_ARG_NAME_MATCH_NONE);
}

/*
d_arg_command_is_option_group
  CLI11's test `name_.empty() && parent_ != nullptr`: a command without a
name that is not the root.
*/
bool
d_arg_command_is_option_group(
    const struct d_arg_command* _command
)
{
    return ( (_command != NULL)          &&
             (_command->parent != NULL)  &&
             ( (_command->name == NULL) ||
               (_command->name[0] == '\0') ) );
}

//==============================================================================
// 3.  LOOKUPS
//==============================================================================

/**
 * @brief Tells whether a command has a name.
 *
 * @param[in] _command  the command.
 * @return `true` when its name is not empty, as a named subcommand's is;
 *         `false` otherwise, as for an option group.
 *
 * Whether a command has a name: the root or a named subcommand, not an
 * option group.
 */
D_STATIC bool
d_arg_internal_command_named(
    const struct d_arg_command* _command
)
{
    return ( (_command->name != NULL) &&
             (_command->name[0] != '\0') );
}

/*
d_arg_internal_fallthrough_parent
  Up from the parent past every nameless command that is not the root.
*/
struct d_arg_command*
d_arg_internal_fallthrough_parent(
    const struct d_arg_command* _command
)
{
    struct d_arg_command* parent = (_command != NULL)
                                   ? _command->parent
                                   : NULL;

    // past the option groups
    while ( (parent != NULL)                       &&
            (parent->parent != NULL)               &&
            (!d_arg_internal_command_named(parent)) )
    {
        parent = parent->parent;
    }

    return parent;
}

/**
 * @brief Finds a name on which two subcommands clash, as one pair of CLI11's
 *        _compare_subcommand_names.
 *
 * @param[in] _subcommand  one subcommand.
 * @param[in] _other       the other.
 * @return the name or alias that clashes, owned by one of the two; NULL when
 *         none does.
 *
 * CLI11's tests of one pair in _compare_subcommand_names: each one's name
 * against the other, then each one's aliases against the other; the name
 * that clashes, or NULL.
 */
D_STATIC const char*
d_arg_internal_command_names_clash(
    const struct d_arg_command* _subcommand,
    const struct d_arg_command* _other
)
{
    // the names
    if ( (d_arg_internal_command_named(_subcommand)) &&
         (d_arg_command_check_name(_other,
                                   d_arg_internal_text_of(_subcommand->name))) )
    {
        return _subcommand->name;
    }

    if ( (d_arg_internal_command_named(_other)) &&
         (d_arg_command_check_name(_subcommand,
                                   d_arg_internal_text_of(_other->name))) )
    {
        return _other->name;
    }

    // the aliases, each way
    for (size_t index = 0u; index < _subcommand->aliases.count; ++index)
    {
        if (d_arg_command_check_name(
                _other,
                d_arg_internal_text_of(_subcommand->aliases.items[index])))
        {
            return _subcommand->aliases.items[index];
        }
    }

    for (size_t index = 0u; index < _other->aliases.count; ++index)
    {
        if (d_arg_command_check_name(
                _subcommand,
                d_arg_internal_text_of(_other->aliases.items[index])))
        {
            return _other->aliases.items[index];
        }
    }

    return NULL;
}

/*
d_arg_internal_subcommand_clash
  CLI11's _compare_subcommand_names: a disabled subcommand clashes with
nothing; otherwise each enabled sibling under `_base` is tested, and an
option group on either side is searched in turn.
*/
const char*
d_arg_internal_subcommand_clash(
    const struct d_arg_command* _subcommand,
    const struct d_arg_command* _base
)
{
    // a disabled subcommand clashes with nothing
    if (d_arg_command_has_flag(_subcommand,
                               D_ARG_COMMAND_DISABLED))
    {
        return NULL;
    }

    // each other enabled subcommand of the base
    for (size_t index = 0u; index < _base->subcommands.count; ++index)
    {
        const struct d_arg_command* const other =
            _base->subcommands.items[index];

        if ( (other == _subcommand) ||
             (d_arg_command_has_flag(other,
                                     D_ARG_COMMAND_DISABLED)) )
        {
            continue;
        }

        const char* clash = d_arg_internal_command_names_clash(_subcommand,
                                                               other);

        clash = ( (clash == NULL) &&
                  (!d_arg_internal_command_named(other)) )
                ? d_arg_internal_subcommand_clash(_subcommand,
                                                  other)
                : clash;
        clash = ( (clash == NULL) &&
                  (!d_arg_internal_command_named(_subcommand)) )
                ? d_arg_internal_subcommand_clash(other,
                                                  _subcommand)
                : clash;

        if (clash != NULL)
        {
            return clash;
        }
    }

    return NULL;
}

// d_arg_internal_command_search
//   struct: one search of CLI11's _find_subcommand: the name, the two
// switches, and the candidate found so far.
struct d_arg_internal_command_search
{
    struct d_pack_text    name;
    bool                  ignore_disabled;
    bool                  ignore_used;
    struct d_arg_command* found;
};

/**
 * @brief Offers a candidate to a search for a subcommand.
 *
 * @param[in,out] _search     the search, whose result is `found`.
 * @param[in]     _candidate  the candidate.
 * @param[in]     _prefix     whether prefix matching is on.
 * @return `true` when the search is over; `false` when it goes on.
 *
 * A candidate for a search: with prefix matching it is kept and the search
 * goes on, unless one was found already, which makes the name ambiguous and
 * the result NULL; without, it ends the search. Returns whether the search
 * is over, with its result in `found`.
 */
D_STATIC bool
d_arg_internal_command_offer(
    struct d_arg_internal_command_search* _search,
    struct d_arg_command*                 _candidate,
    bool                                  _prefix
)
{
    // a second candidate makes the name ambiguous
    if (_search->found != NULL)
    {
        _search->found = NULL;

        return true;
    }

    _search->found = _candidate;

    return (!_prefix);
}

/**
 * @brief Searches an option group, as part of CLI11's _find_subcommand loop.
 *
 * @param[in]     _group   the option group.
 * @param[in,out] _search  the search, whose result is `found`.
 * @param[in]     _prefix  whether prefix matching is on.
 * @return `true` when the search is over; `false` when it goes on.
 *
 * The part of CLI11's _find_subcommand loop for an option group: the group
 * is searched in turn, and what it finds is offered. Returns whether the
 * search is over.
 */
D_STATIC bool
d_arg_internal_command_search_group(
    struct d_arg_command*                 _group,
    struct d_arg_internal_command_search* _search,
    bool                                  _prefix
)
{
    struct d_arg_command* const inner =
        d_arg_internal_find_subcommand(_group,
                                       _search->name,
                                       _search->ignore_disabled,
                                       _search->ignore_used);

    return ( (inner != NULL) &&
             (d_arg_internal_command_offer(_search,
                                           inner,
                                           _prefix)) );
}

/**
 * @brief Tests a subcommand's own name, as part of CLI11's _find_subcommand
 *        loop.
 *
 * @param[in]     _candidate  the subcommand.
 * @param[in,out] _search     the search, whose result is `found`.
 * @param[in]     _prefix     whether prefix matching is on.
 * @return `true` when the search is over; `false` when it goes on.
 *
 * The part of the loop for a subcommand's own name: an exact match ends the
 * search with it, a prefix is offered; a subcommand used already is passed
 * over when asked. Returns whether the search is over.
 */
D_STATIC bool
d_arg_internal_command_search_name(
    struct d_arg_command*                 _candidate,
    struct d_arg_internal_command_search* _search,
    bool                                  _prefix
)
{
    const int  match  = d_arg_command_check_name_detail(_candidate,
                                                        _search->name);
    const bool usable = ( (_candidate->parsed == 0u) ||
                          (!_search->ignore_used) );

    // no match, or one that may not be used
    if ( (match == (int)D_ARG_NAME_MATCH_NONE) ||
         (!usable) )
    {
        return false;
    }

    // an exact match ends the search
    if (match == (int)D_ARG_NAME_MATCH_EXACT)
    {
        _search->found = _candidate;

        return true;
    }

    return d_arg_internal_command_offer(_search,
                                        _candidate,
                                        _prefix);
}

/**
 * @brief Searches one command's subcommands, as CLI11's _find_subcommand.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _search   the search, whose result is `found`.
 * @return `true` when the search ended early; `false` when it went through
 *         every subcommand.
 *
 * CLI11's _find_subcommand over one command's subcommands, as its loop
 * writes it: an option group is searched first, then each subcommand's own
 * name is tested, a disabled one passed over when asked.
 */
D_STATIC bool
d_arg_internal_command_search_in(
    const struct d_arg_command*           _command,
    struct d_arg_internal_command_search* _search
)
{
    const bool prefix = d_arg_command_has_flag(_command,
                                               D_ARG_COMMAND_PREFIX_MATCHING);

    // each subcommand that may be found
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const candidate =
            _command->subcommands.items[index];

        if ( (_search->ignore_disabled) &&
             (d_arg_command_has_flag(candidate,
                                     D_ARG_COMMAND_DISABLED)) )
        {
            continue;
        }

        // inside an option group, then by its own name
        if ( ( (!d_arg_internal_command_named(candidate)) &&
               (d_arg_internal_command_search_group(candidate,
                                                    _search,
                                                    prefix)) ) ||
             (d_arg_internal_command_search_name(candidate,
                                                 _search,
                                                 prefix)) )
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_find_subcommand
  A search over the command's subcommands, its result whatever the search
ended with.
*/
struct d_arg_command*
d_arg_internal_find_subcommand(
    const struct d_arg_command* _command,
    struct d_pack_text          _name,
    bool                        _skip_disabled,
    bool                        _skip_used
)
{
    struct d_arg_internal_command_search search = { _name,
                                                    _skip_disabled,
                                                    _skip_used,
                                                    NULL };

    // nothing to search without a command
    if (!_command)
    {
        return NULL;
    }

    (void)d_arg_internal_command_search_in(_command,
                                           &search);

    return search.found;
}

/*
d_arg_internal_find_option
  CLI11's get_option_no_throw: the command's options, then its option
groups', then, for a named command that falls through, its fallthrough
parent's.
*/
struct d_arg_option*
d_arg_internal_find_option(
    const struct d_arg_command* _command,
    struct d_pack_text          _name
)
{
    // the command's own options
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        if (d_arg_option_check_name(_command->options.items[index],
                                    _name))
        {
            return _command->options.items[index];
        }
    }

    // those of its option groups
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* const group =
            _command->subcommands.items[index];
        struct d_arg_option* const        found =
            (!d_arg_internal_command_named(group))
            ? d_arg_internal_find_option(group,
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
             (d_arg_internal_command_named(_command)) )
           ? d_arg_internal_find_option(
                 d_arg_internal_fallthrough_parent(_command),
                 _name)
           : NULL;
}

/*
d_arg_command_option
  d_arg_internal_find_option, for a NUL-terminated name.
*/
struct d_arg_option*
d_arg_command_option(
    const struct d_arg_command* _command,
    const char*                 _name
)
{
    // nothing to find without a command or a name
    if ( (!_command) ||
         (!_name) )
    {
        return NULL;
    }

    return d_arg_internal_find_option(_command,
                                      d_arg_internal_text_of(_name));
}

/*
d_arg_internal_parser_of
  An option's command's parser.
*/
struct d_arg_parser*
d_arg_internal_parser_of(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->parent != NULL) )
           ? _option->parent->parser
           : NULL;
}

//==============================================================================
// 4.  OPTION LISTS
//==============================================================================

/**
 * @brief Tells whether an option answers to the display name of one a list
 *        holds.
 *
 * @param[in] _list    the options.
 * @param[in] _option  the option.
 * @return `true` when it does; `false` otherwise, or when a name could not be
 *         written.
 *
 * Whether `_option` answers to the display name of an option the list
 * already holds, as CLI11 checks before adding a fallthrough parent's option;
 * a name that cannot be written is taken as no match.
 */
D_STATIC bool
d_arg_internal_command_has_display(
    const struct d_arg_option_list* _list,
    const struct d_arg_option*      _option
)
{
    struct d_arg_buffer name  = D_ARG_BUFFER_INIT;
    bool                found = false;

    // each option kept, by its display name
    for (size_t index = 0u; (!found) && (index < _list->count); ++index)
    {
        d_arg_buffer_clear(&name);
        found = ( (d_arg_option_name(_list->items[index],
                                     0u,
                                     &name) == D_ARG_OK) &&
                  (d_arg_option_check_name(_option,
                                           d_arg_buffer_text(&name))) );
    }

    d_arg_buffer_release(&name);

    return found;
}

/**
 * @brief Collects the fallthrough parent's options a command lists.
 *
 * @param[in]     _command  the command.
 * @param[in]     _help     which form, as d_arg_internal_command_options.
 * @param[in]     _test     the filter; NULL passes every option.
 * @param[in]     _context  `_test`'s context.
 * @param[in,out] _out      the list that gains the options that answer to none
 *                          of the display names it holds.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with the options collected
 *         before the failure kept.
 *
 * The fallthrough parent's options a command lists -- those that pass the
 * test -- that answer to none of the display names collected so far, as
 * CLI11 compares them against the list its filter has already narrowed.
 */
D_STATIC int
d_arg_internal_command_collect_parent(
    const struct d_arg_command* _command,
    bool                        _help,
    d_arg_internal_filter_fn    _test,
    const void*                 _context,
    struct d_arg_option_list*   _out
)
{
    struct d_arg_option_list parent = D_ARG_LIST_INIT;
    int                      status = d_arg_internal_command_options_where(
                                          d_arg_internal_fallthrough_parent(
                                              _command),
                                          _help,
                                          _test,
                                          _context,
                                          &parent);

    // each of the parent's that is not shadowed
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < parent.count);
         ++index)
    {
        if (!d_arg_internal_command_has_display(_out,
                                                parent.items[index]))
        {
            status = d_arg_internal_option_push(_out,
                                                parent.items[index]);
        }
    }

    d_arg_internal_option_list_release(&parent);

    return status;
}

/**
 * @brief Tells whether a command's get_options takes in a subcommand's options.
 *
 * @param[in] _sub   the subcommand.
 * @param[in] _help  which form, as d_arg_internal_command_options.
 * @return `true` when it does; `false` otherwise.
 *
 * Whether a command's get_options takes in a subcommand's: with `_help`, a
 * nameless one whose group starts with `+`, as CLI11's const form; otherwise
 * every nameless one and every one whose group starts with `+`.
 */
D_STATIC bool
d_arg_internal_command_merges(
    const struct d_arg_command* _sub,
    bool                        _help
)
{
    const bool plus  = ( (_sub->group != NULL) &&
                         (_sub->group[0] == '+') );
    const bool named = d_arg_internal_command_named(_sub);

    return (_help)
           ? ( (!named) &&
               (plus) )
           : ( (!named) ||
               (plus) );
}

/*
d_arg_internal_command_options_where
  CLI11's get_options(filter), in either form: the command's own options
that pass the test, then those of the subcommands it merges, then, for a
named command that falls through, its parent's that it does not shadow.
*/
int
d_arg_internal_command_options_where(
    const struct d_arg_command* _command,
    bool                        _help,
    d_arg_internal_filter_fn    _test,
    const void*                 _context,
    struct d_arg_option_list*   _out
)
{
    int status = D_ARG_OK;

    // the command's own that pass
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        status = ( (_test == NULL) ||
                   (_test(_command->options.items[index],
                          _context)) )
                 ? d_arg_internal_option_push(_out,
                                              _command->options.items[index])
                 : D_ARG_OK;
    }

    // the subcommands it merges
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        status = (d_arg_internal_command_merges(
                      _command->subcommands.items[index],
                      _help))
                 ? d_arg_internal_command_options_where(
                       _command->subcommands.items[index],
                       _help,
                       _test,
                       _context,
                       _out)
                 : D_ARG_OK;
    }

    return ( (status == D_ARG_OK)                                    &&
             (d_arg_command_has_flag(_command,
                                     D_ARG_COMMAND_FALLTHROUGH))     &&
             (_command->parent != NULL)                              &&
             (d_arg_internal_command_named(_command)) )
           ? d_arg_internal_command_collect_parent(_command,
                                                   _help,
                                                   _test,
                                                   _context,
                                                   _out)
           : status;
}

/*
d_arg_internal_command_options
  CLI11's get_options() with no filter.
*/
int
d_arg_internal_command_options(
    const struct d_arg_command* _command,
    bool                        _help,
    struct d_arg_option_list*   _out
)
{
    return d_arg_internal_command_options_where(_command,
                                                _help,
                                                NULL,
                                                NULL,
                                                _out);
}
