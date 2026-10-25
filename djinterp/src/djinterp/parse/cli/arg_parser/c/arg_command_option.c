/*******************************************************************************
* djinterp [parse]                                          arg_command_option.c
*
* A command's options, declared in arg_command.h (3.1, 3.2 and 3.4.2).
*   Adding an option as CLI11's add_option adds one -- its names checked
* against the command's options, then against the top-level command's for a
* positional or an option group's option, and for non-standard short names
* -- with the command's option defaults applied; adding flags as
* _add_flag_internal does, their `{value}` and `!` names split off first;
* and removing an option and every reference to it.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_command_option.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memchr, memmove, strlen, strpbrk
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type, d_arg_parse_bool
#include "./arg_internal.h"  // failures, lists, spans
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intmax_t, INT64_MIN, INT64_MAX


//==============================================================================
// 1.  NAME CHECKS
//==============================================================================

/**
 * @brief Writes an option's display name into a buffer, for a message.
 *
 * @param[in]     _option  the option.
 * @param[in,out] _out     the buffer, empty at first, that receives the name.
 * @return the name; empty when it could not be written.
 *
 * An option's display name into a buffer, for a message; a refused
 * allocation leaves it empty.
 */
D_STATIC const char*
d_arg_internal_command_display(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    (void)d_arg_option_name(_option,
                            0u,
                            _out);

    return d_arg_buffer_text(_out).data;
}

/**
 * @brief Checks a new option's names against a command's own options, as
 *        CLI11's first check.
 *
 * @param[in] _command  the command.
 * @param[in] _option   the new option.
 * @param[in] _names    its names as given, for the message.
 * @return D_ARG_OK, or D_ARG_ERROR_OPTION_ALREADY_ADDED, recorded as a
 *         definition error.
 *
 * CLI11's first check: an option of the command that shares a name with the
 * new one, the shared name described by its kind -- "short name '-x'", "long
 * name '--x'" or "name 'x'".
 */
D_STATIC int
d_arg_internal_command_clash_local(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option,
    const char*                _names
)
{
    // each option of the command
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        const struct d_arg_option* const existing =
            _command->options.items[index];
        const char* const                match    =
            d_arg_option_matching_name(existing,
                                       _option);
        struct d_arg_buffer              name     = D_ARG_BUFFER_INIT;

        if (match == NULL)
        {
            continue;
        }

        const struct d_pack_text shared = d_arg_internal_text_of(match);
        const int                status = d_arg_internal_fail_definition(
            _command,
            NULL,
            D_ARG_ERROR_OPTION_ALREADY_ADDED,
            "cannot add '%s': %s%s' already used by '%s'",
            _names,
            d_arg_option_check_short_name(existing,
                                          shared)
            ? "short name '-"
            : ( d_arg_option_check_long_name(existing,
                                             shared)
                ? "long name '--"
                : "name '" ),
            match,
            d_arg_internal_command_display(existing,
                                           &name));

        d_arg_buffer_release(&name);

        return status;
    }

    return D_ARG_OK;
}

/**
 * @brief Finds the top-level command, as CLI11's top_level_parent.
 *
 * @param[in] _command  the command.
 * @return the first command up from it, itself included, that has a name or no
 *         parent.
 *
 * CLI11's top_level_parent: up from the command past every nameless one that
 * has a parent.
 */
D_STATIC struct d_arg_command*
d_arg_internal_command_top(
    struct d_arg_command* _command
)
{
    struct d_arg_command* top = _command;

    // past the option groups
    while ( ( (top->name == NULL) ||
              (top->name[0] == '\0') ) &&
            (top->parent != NULL) )
    {
        top = top->parent;
    }

    return top;
}

/**
 * @brief Looks an option up on the top-level command, as get_option_no_throw.
 *
 * @param[in]  _top     the top-level command.
 * @param[in]  _prefix  the dashes before the name.
 * @param[in]  _name    the name.
 * @param[out] _found   receives the option; NULL when none answers.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when the name could not be
 *         built, `*_found` then NULL.
 *
 * get_option_no_throw on the top-level command for a name with `_prefix`
 * before it; a name that cannot be built is a refused allocation.
 */
D_STATIC int
d_arg_internal_command_lookup(
    const struct d_arg_command* _top,
    const char*                 _prefix,
    const char*                 _name,
    struct d_arg_option**       _found
)
{
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&name,
                                   _prefix);
    (void)d_arg_buffer_append_cstr(&name,
                                   _name);

    const int status = (name.failed)
                       ? D_ARG_ERROR_OUT_OF_MEMORY
                       : D_ARG_OK;

    *_found = (status == D_ARG_OK)
              ? d_arg_internal_find_option(_top,
                                           d_arg_buffer_text(&name))
              : NULL;
    d_arg_buffer_release(&name);

    return status;
}

/**
 * @brief Checks a name against the top-level command, as one of CLI11's
 *        lookups.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _message  the message's words before the name.
 * @param[in] _prefix   the dashes before the name.
 * @param[in] _name     the name.
 * @param[in] _any      whether any option is in the way, not only a
 *                      configurable one.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * One of CLI11's top-level lookups: an option found under the name that is
 * configurable -- or, when `_any`, any option at all -- is the conflict
 * "<message><prefix><name> is already added".
 */
D_STATIC int
d_arg_internal_command_clash_with(
    struct d_arg_command* _command,
    const char*           _message,
    const char*           _prefix,
    const char*           _name,
    bool                  _any
)
{
    struct d_arg_option* found  = NULL;
    const int            status =
        d_arg_internal_command_lookup(d_arg_internal_command_top(_command),
                                      _prefix,
                                      _name,
                                      &found);

    // nothing in the way
    if ( (status != D_ARG_OK) ||
         (found == NULL)      ||
         ( (!_any) &&
           (!d_arg_option_has_flag(found,
                                   D_ARG_OPTION_CONFIGURABLE)) ) )
    {
        return (status == D_ARG_OK)
               ? D_ARG_OK
               : d_arg_internal_fail_definition(_command,
                                                NULL,
                                                status,
                                                NULL);
    }

    return d_arg_internal_fail_definition(_command,
                                          NULL,
                                          D_ARG_ERROR_OPTION_ALREADY_ADDED,
                                          "%s%s%s is already added",
                                          _message,
                                          _prefix,
                                          _name);
}

/**
 * @brief Checks an option with only a positional name against the top-level
 *        command, as CLI11 does.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _option   the option.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's checks for an option with only a positional name: its single name
 * with dashes, `-x` for one character, must not name a configurable option
 * of the top-level command, and the plain name must not name another
 * positional-only option there.
 */
D_STATIC int
d_arg_internal_command_clash_positional(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    const char* const single = d_arg_option_single_name(_option);
    const char* const dashes = (strlen(single) == 1u) ? "-" : "--";
    int               status = d_arg_internal_command_clash_with(
                                   _command,
                                   "added option positional name matches "
                                   "existing option: ",
                                   dashes,
                                   single,
                                   false);

    // a positional-only option under the same name
    if (status == D_ARG_OK)
    {
        const struct d_arg_option* const found =
            d_arg_internal_find_option(d_arg_internal_command_top(_command),
                                       d_arg_internal_text_of(single));

        status = ( (found != NULL)                 &&
                   (found->long_names.count == 0u) &&
                   (found->short_names.count == 0u) )
                 ? d_arg_internal_fail_definition(
                       _command,
                       NULL,
                       D_ARG_ERROR_OPTION_ALREADY_ADDED,
                       "unable to disambiguate with existing option: "
                       "%s%s is already added",
                       dashes,
                       single)
                 : D_ARG_OK;
    }

    return status;
}

/**
 * @brief Checks one long name of an option group's option against the top-level
 *        command, as CLI11 does.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _name     the long name.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's checks of one long name of an option group's option against the
 * top-level command: as a positional name, as a long name, and -- for one
 * character, or where the top-level command takes non-standard names -- as a
 * short name.
 */
D_STATIC int
d_arg_internal_command_clash_long(
    struct d_arg_command* _command,
    const char*           _name
)
{
    const struct d_arg_command* const top = d_arg_internal_command_top(
                                                _command);
    int                               status =
        d_arg_internal_command_clash_with(_command,
                                          "added option matches existing "
                                          "positional option: ",
                                          "",
                                          _name,
                                          false);

    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_clash_with(_command,
                                                   "added option matches "
                                                   "existing option: ",
                                                   "--",
                                                   _name,
                                                   false);
    }

    // as a short name, where one may be this long
    if ( (status == D_ARG_OK) &&
         ( (strlen(_name) == 1u) ||
           (d_arg_command_has_flag(top,
                                   D_ARG_COMMAND_NON_STANDARD_NAMES)) ) )
    {
        status = d_arg_internal_command_clash_with(_command,
                                                   "added option matches "
                                                   "existing option: ",
                                                   "-",
                                                   _name,
                                                   false);
    }

    return status;
}

/**
 * @brief Checks one short name of an option group's option against the
 *        top-level command, as CLI11 does.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _name     the short name.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's checks of one short name of an option group's option against the
 * top-level command: as a positional name, a short name and a long name.
 */
D_STATIC int
d_arg_internal_command_clash_short(
    struct d_arg_command* _command,
    const char*           _name
)
{
    int status = d_arg_internal_command_clash_with(_command,
                                                   "added option matches "
                                                   "existing positional "
                                                   "option: ",
                                                   "",
                                                   _name,
                                                   false);

    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_clash_with(_command,
                                                   "added option matches "
                                                   "existing option: ",
                                                   "-",
                                                   _name,
                                                   false);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_command_clash_with(_command,
                                               "added option matches "
                                               "existing option: ",
                                               "--",
                                               _name,
                                               false)
           : status;
}

/**
 * @brief Checks an option added below the top-level command against it, as
 *        CLI11 does.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _option   the option.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's checks for an option added below the top-level command: each of
 * its long names, then each of its short names.
 */
D_STATIC int
d_arg_internal_command_clash_group(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    int status = D_ARG_OK;

    // the long names
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->long_names.count);
         ++index)
    {
        status = d_arg_internal_command_clash_long(
                     _command,
                     _option->long_names.items[index]);
    }

    // the short names
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->short_names.count);
         ++index)
    {
        status = d_arg_internal_command_clash_short(
                     _command,
                     _option->short_names.items[index]);
    }

    return status;
}

/**
 * @brief Checks the options the top-level command lists for a non-standard
 *        short name a new option's short names would interfere with, as CLI11's
 *        second non-standard check.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _option   the new option.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's second non-standard check: an option the top-level command lists
 * whose long short name begins with a character the new option takes as a
 * short name.
 */
D_STATIC int
d_arg_internal_command_interferes(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    struct d_arg_option_list listed = D_ARG_LIST_INIT;
    int                      status =
        d_arg_internal_command_options(d_arg_internal_command_top(_command),
                                       true,
                                       &listed);

    // a list that could not be made is a refused definition, recorded
    status = (status == D_ARG_OK)
             ? D_ARG_OK
             : d_arg_internal_fail_definition(_command,
                                              NULL,
                                              status,
                                              NULL);

    // each listed option's short names longer than a character
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < listed.count);
         ++index)
    {
        const struct d_arg_string_list* const shorts =
            &listed.items[index]->short_names;

        for (size_t at = 0u; (status == D_ARG_OK) && (at < shorts->count); ++at)
        {
            const char* const other = shorts->items[at];

            status = ( (strlen(other) > 1u) &&
                       (d_arg_option_check_short_name(
                            _option,
                            d_arg_internal_span(other,
                                                1u))) )
                     ? d_arg_internal_fail_definition(
                           _command,
                           NULL,
                           D_ARG_ERROR_OPTION_ALREADY_ADDED,
                           "added option interferes with existing non "
                           "standard option: %s is already added",
                           other)
                     : D_ARG_OK;
        }
    }

    d_arg_internal_option_list_release(&listed);

    return status;
}

/**
 * @brief Checks one short name longer than a character, as CLI11's first
 *        non-standard check.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _name     the short name.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's first non-standard check, for one short name longer than a
 * character: an option of any kind answering to `-` and its first character
 * is the conflict, named by the whole short name.
 */
D_STATIC int
d_arg_internal_command_clash_first(
    struct d_arg_command* _command,
    const char*           _name
)
{
    const char           first[2] = { _name[0], '\0' };
    struct d_arg_option* found    = NULL;
    const int            status   = d_arg_internal_command_lookup(
                                        d_arg_internal_command_top(_command),
                                        "-",
                                        first,
                                        &found);

    // a refused allocation, or nothing in the way
    if ( (status != D_ARG_OK) ||
         (found == NULL) )
    {
        return (status == D_ARG_OK)
               ? D_ARG_OK
               : d_arg_internal_fail_definition(_command,
                                                NULL,
                                                status,
                                                NULL);
    }

    return d_arg_internal_fail_definition(_command,
                                          NULL,
                                          D_ARG_ERROR_OPTION_ALREADY_ADDED,
                                          "added option interferes with "
                                          "existing short option: %s is "
                                          "already added",
                                          _name);
}

/**
 * @brief Checks a new option's short names where the command takes non-standard
 *        names, as CLI11 does.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _option   the new option.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's checks for a command taking non-standard names: each short name
 * longer than a character, then every listed option's.
 */
D_STATIC int
d_arg_internal_command_clash_non_standard(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    int status = D_ARG_OK;

    // each short name longer than a character
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->short_names.count);
         ++index)
    {
        const char* const name = _option->short_names.items[index];

        if (strlen(name) > 1u)
        {
            status = d_arg_internal_command_clash_first(_command,
                                                        name);
        }
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_command_interferes(_command,
                                               _option)
           : status;
}

//==============================================================================
// 2.  ADDING
//==============================================================================

// d_arg_internal_command_new
//   struct: what an option is made with besides its names: a callback, its
// context and the function that frees it, and a variable attached before the
// defaults are applied -- each optional.
struct d_arg_internal_command_new
{
    d_arg_results_fn         callback;
    void*                    context;
    d_arg_release_fn         release;
    void*                    target;
    const struct d_arg_type* type;
};

/**
 * @brief Checks a new option's names, as CLI11 does, in its order.
 *
 * @param[in] _command  the command the option is added to.
 * @param[in] _option   the new option.
 * @param[in] _names    its names as given, for the messages.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error.
 *
 * CLI11's checks of a new option's names, in its order: against the
 * command's own options; then, for a positional-only option, its positional
 * name, or, below the top-level command, every name, against the top-level
 * command's; then the non-standard short names.
 */
D_STATIC int
d_arg_internal_command_check_names(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option,
    const char*                _names
)
{
    const bool positional_only = ( (_option->long_names.count == 0u) &&
                                   (_option->short_names.count == 0u) );
    int        status          = d_arg_internal_command_clash_local(_command,
                                                                    _option,
                                                                    _names);

    // the top-level command's names
    if (status == D_ARG_OK)
    {
        status = positional_only
                 ? d_arg_internal_command_clash_positional(_command,
                                                           _option)
                 : ( (d_arg_internal_command_top(_command) != _command)
                     ? d_arg_internal_command_clash_group(_command,
                                                          _option)
                     : D_ARG_OK );
    }

    // non-standard short names
    return ( (status == D_ARG_OK)                                    &&
             (_option->short_names.count > 0u)                       &&
             (d_arg_command_has_flag(_command,
                                     D_ARG_COMMAND_NON_STANDARD_NAMES)) )
           ? d_arg_internal_command_clash_non_standard(_command,
                                                       _option)
           : status;
}

/**
 * @brief Makes and stores a new option, as CLI11's add_option up to storing it.
 *
 * @param[in,out] _command      the command, whose options gain it.
 * @param[in]     _names        its names.
 * @param[in]     _description  its help text, copied; NULL for none.
 * @return the option; NULL when it was refused, the refusal recorded as a
 *         definition error.
 *
 * CLI11's add_option up to storing the option: it is made from the names --
 * a bad name recorded -- its names checked, and stored last; a refused option
 * is not kept.
 */
D_STATIC struct d_arg_option*
d_arg_internal_command_new_option(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description
)
{
    struct d_arg_option* const option = d_arg_internal_option_create(
                                            _command,
                                            d_arg_internal_text_of(_names),
                                            _description);

    // a bad name, recorded
    if (!option)
    {
        return NULL;
    }

    int status = d_arg_internal_command_check_names(_command,
                                                    option,
                                                    _names);

    // stored, once nothing is in the way
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_push(&_command->options,
                                            option);
        status = (status == D_ARG_OK)
                 ? D_ARG_OK
                 : d_arg_internal_fail_definition(_command,
                                                  NULL,
                                                  status,
                                                  NULL);
    }

    // a refused option is not kept
    if (status != D_ARG_OK)
    {
        d_arg_internal_option_destroy(option);

        return NULL;
    }

    return option;
}

/**
 * @brief Gives an option the switches of a command's option defaults, as
 *        CLI11's copy_to.
 *
 * @param[in,out] _option    the option.
 * @param[in]     _defaults  the defaults.
 * @return D_ARG_OK, or D_ARG_ERROR_OPTION_ALREADY_ADDED, recorded as a
 *         definition error, when a switch makes names clash.
 *
 * The switches of CLI11's copy_to, in its order: required, ignore case and
 * ignore underscore -- each refused when it makes names clash -- then
 * configurable, flag overrides and default capture.
 */
D_STATIC int
d_arg_internal_command_switch_defaults(
    struct d_arg_option*                _option,
    const struct d_arg_option_defaults* _defaults
)
{
    const uint32_t flags = _defaults->flags;

    d_arg_option_set_required(_option,
                              (flags & D_ARG_OPTION_REQUIRED) != 0u);

    int status = d_arg_option_set_ignore_case(
                     _option,
                     (flags & D_ARG_OPTION_IGNORE_CASE) != 0u);

    if (status == D_ARG_OK)
    {
        status = d_arg_option_set_ignore_underscore(
                     _option,
                     (flags & D_ARG_OPTION_IGNORE_UNDERSCORE) != 0u);
    }

    // the rest cannot fail
    if (status == D_ARG_OK)
    {
        d_arg_option_set_configurable(
            _option,
            (flags & D_ARG_OPTION_CONFIGURABLE) != 0u);
        d_arg_option_set_disable_flag_override(
            _option,
            (flags & D_ARG_OPTION_DISABLE_FLAG_OVERRIDE) != 0u);
    }

    return status;
}

/**
 * @brief Gives an option a command's option defaults, as CLI11's copy_to, then
 *        captures its default when it always does.
 *
 * @param[in]     _command  the command, whose option defaults apply.
 * @param[in,out] _option   the option.
 * @return D_ARG_OK, or the failure, recorded as a definition error.
 *
 * CLI11's OptionDefaults::copy_to, the integer rule beside it, then the
 * capture CLI11's add_option makes for an option that always captures its
 * default.
 */
D_STATIC int
d_arg_internal_command_adopt_defaults(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    const struct d_arg_option_defaults* const defaults =
        &_command->option_defaults;
    int status = d_arg_option_set_group(_option,
                                        defaults->group);

    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_switch_defaults(_option,
                                                        defaults);
    }

    // the settings that cannot fail
    if (status == D_ARG_OK)
    {
        d_arg_option_set_delimiter(_option,
                                   defaults->delimiter);
        d_arg_option_set_always_capture_default(
            _option,
            (defaults->flags & D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT) != 0u);
        d_arg_option_set_multi_option_policy(_option,
                                             defaults->multi_option_policy);
        d_arg_option_set_callback_priority(_option,
                                           defaults->callback_priority);
        d_arg_option_set_integer_rule(_option,
                                      defaults->integer_rule);
    }

    return ( (status == D_ARG_OK) &&
             (d_arg_option_has_flag(_option,
                                    D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT)) )
           ? d_arg_option_capture_default(_option)
           : status;
}

/**
 * @brief Frees a callback's context that never reached an option.
 *
 * @param[in] _new  what the option was to be given; NULL for nothing.
 *
 * Frees a callback's context that never reached an option, as the calls
 * promise to on failure.
 */
D_STATIC void
d_arg_internal_command_release_new(
    const struct d_arg_internal_command_new* _new
)
{
    // only an owned context is freed
    if ( (_new != NULL)           &&
         (_new->release != NULL)  &&
         (_new->context != NULL) )
    {
        _new->release(_new->context);
    }

    return;
}

/**
 * @brief Adds an option whole, as CLI11's add_option.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the option's names.
 * @param[in]     _description  its help text, copied; NULL for none.
 * @param[in]     _new          the callback, its context and the variable the
 *                              option is given; NULL for none.
 * @return the option; NULL when it was refused, the refusal recorded as a
 *         definition error and `_new`'s context freed.
 *
 * CLI11's add_option whole: the option made and stored, its callback and
 * variable given, the defaults applied. An option whose variable or defaults
 * cannot be given is refused whole, where CLI11's stays in its list when
 * copy_to throws (ISSUES 64).
 */
D_STATIC struct d_arg_option*
d_arg_internal_command_add(
    struct d_arg_command*                    _command,
    const char*                              _names,
    const char*                              _description,
    const struct d_arg_internal_command_new* _new
)
{
    struct d_arg_option* const option =
        d_arg_internal_command_new_option(_command,
                                          _names,
                                          _description);

    // the context goes with a refused option
    if (!option)
    {
        d_arg_internal_command_release_new(_new);

        return NULL;
    }

    int status = D_ARG_OK;

    // the callback and the variable
    if (_new != NULL)
    {
        d_arg_option_set_callback(option,
                                  _new->callback,
                                  _new->context,
                                  _new->release);
        status = (_new->target != NULL)
                 ? d_arg_internal_option_attach(option,
                                                _new->target,
                                                _new->type)
                 : D_ARG_OK;
    }

    // then the defaults
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_adopt_defaults(_command,
                                                       option);
    }

    return (status == D_ARG_OK)
           ? option
           : d_arg_internal_command_refuse_option(_command,
                                                  option,
                                                  status);
}

/*
d_arg_command_add_option
  d_arg_internal_command_add with nothing else.
*/
struct d_arg_option*
d_arg_command_add_option(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_names) )
    {
        return NULL;
    }

    return d_arg_internal_command_add(_command,
                                      _names,
                                      _description,
                                      NULL);
}

/*
d_arg_command_add_option_bound
  The variable attached before the defaults, so that a default captured
then is its value, then bound, as CLI11's add_option(name, variable) sets
the type's sizes and count after add_option; an option that cannot be
bound is refused whole.
*/
struct d_arg_option*
d_arg_command_add_option_bound(
    struct d_arg_command*    _command,
    const char*              _names,
    void*                    _target,
    const struct d_arg_type* _type,
    const char*              _description
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_names)   ||
         (!_target)  ||
         (!_type) )
    {
        return NULL;
    }

    const struct d_arg_internal_command_new bound = { NULL,
                                                      NULL,
                                                      NULL,
                                                      _target,
                                                      _type };

    struct d_arg_option* const option = d_arg_internal_command_add(_command,
                                                                   _names,
                                                                   _description,
                                                                   &bound);

    // refused, recorded
    if (!option)
    {
        return NULL;
    }

    const int status = d_arg_option_bind(option,
                                         _target,
                                         _type);

    return (status == D_ARG_OK)
           ? option
           : d_arg_internal_command_refuse_option(_command,
                                                  option,
                                                  status);
}

/*
d_arg_command_add_option_callback
  d_arg_internal_command_add with the callback, whose context is freed with
the option or on failure.
*/
struct d_arg_option*
d_arg_command_add_option_callback(
    struct d_arg_command* _command,
    const char*           _names,
    d_arg_results_fn      _callback,
    void*                 _context,
    d_arg_release_fn      _release,
    const char*           _description
)
{
    const struct d_arg_internal_command_new call = { _callback,
                                                     _context,
                                                     _release,
                                                     NULL,
                                                     NULL };

    // validate the parameters
    if ( (!_command) ||
         (!_names) )
    {
        d_arg_internal_command_release_new(&call);

        return NULL;
    }

    return d_arg_internal_command_add(_command,
                                      _names,
                                      _description,
                                      &call);
}

//==============================================================================
// 3.  FLAGS
//==============================================================================

// d_arg_internal_command_flag_names
//   struct: a flag's names as CLI11 splits them: the names without their
// values, then the names that carry one, parallel to the values.
struct d_arg_internal_command_flag_names
{
    struct d_arg_buffer      plain;
    struct d_arg_string_list names;
    struct d_arg_string_list values;
};

/**
 * @brief Takes every `!` out of a buffer's text.
 *
 * @param[in,out] _plain  the text, closed up.
 *
 * Every `!` taken out, the rest closed up.
 */
D_STATIC void
d_arg_internal_command_drop_bangs(
    struct d_arg_buffer* _plain
)
{
    size_t kept = 0u;

    // each character but a bang moves up
    for (size_t index = 0u; index < _plain->length; ++index)
    {
        if (_plain->data[index] != '!')
        {
            _plain->data[kept] = _plain->data[index];
            kept++;
        }
    }

    d_arg_buffer_truncate(_plain,
                          kept);

    return;
}

/**
 * @brief Cuts the values out of a flag's names, as CLI11's
 *        remove_default_flag_values on a copy.
 *
 * @param[in,out] _plain  the names, cut in place; a failed buffer is left as it
 *                        is.
 *
 * CLI11's remove_default_flag_values on a copy: each `{...}` that closes
 * before a comma is cut -- the search for the next `{` resuming one past where
 * the cut one began, as CLI11's does -- then every `!` is dropped.
 */
D_STATIC void
d_arg_internal_command_strip_values(
    struct d_arg_buffer* _plain
)
{
    char*  data = _plain->data;
    size_t at   = 2u;

    // each `{` from the third character on
    while ( (!_plain->failed) &&
            (at < _plain->length) )
    {
        char* const open   = (char*)memchr(data + at,
                                           '{',
                                           _plain->length - at);
        const char* finish = (open != NULL)
                             ? strpbrk(open + 1,
                                       "},")
                             : NULL;

        if (!open)
        {
            break;
        }

        // a value that closes before the next name is cut
        if ( (finish != NULL) &&
             (*finish == '}') )
        {
            const size_t cut = (size_t)(finish - open) + 1u;

            memmove(open,
                    open + cut,
                    _plain->length - (size_t)(open - data) - cut + 1u);
            _plain->length -= cut;
        }

        at = (size_t)(open - data) + 1u;
    }

    d_arg_internal_command_drop_bangs(_plain);

    return;
}

/**
 * @brief Keeps a flag name and its value side by side.
 *
 * @param[in]     _flag   the name.
 * @param[in]     _value  its value.
 * @param[in,out] _out    the lists, kept parallel.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with neither kept.
 *
 * A name and its value, kept side by side; a value that cannot be kept takes
 * its name back out, so the lists stay parallel.
 */
D_STATIC int
d_arg_internal_command_keep_value(
    struct d_pack_text                        _flag,
    struct d_pack_text                        _value,
    struct d_arg_internal_command_flag_names* _out
)
{
    const size_t count  = _out->names.count;
    int          status = d_arg_internal_string_copy(&_out->names,
                                                     _flag,
                                                     NULL);

    // the value beside it, or neither
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_string_copy(&_out->values,
                                            _value,
                                            NULL);
        d_arg_internal_string_truncate(&_out->names,
                                       (status == D_ARG_OK)
                                       ? (count + 1u)
                                       : count);
    }

    return status;
}

/**
 * @brief Takes one name as CLI11's get_default_flag_values does.
 *
 * @param[in]     _name  the name, trimmed.
 * @param[in,out] _out   the lists that gain the name and its value, when it
 *                       carries one.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * One name in CLI11's get_default_flag_values: a name ending in `{value}`,
 * or starting with `!`, is kept with its value -- "false" for `!` -- and the
 * dashes and bangs at its front taken off; any other is passed over.
 */
D_STATIC int
d_arg_internal_command_flag_value(
    struct d_pack_text                        _name,
    struct d_arg_internal_command_flag_names* _out
)
{
    const char* const open   = (_name.length > 0u)
                               ? (const char*)memchr(_name.data,
                                                     '{',
                                                     _name.length)
                               : NULL;
    const bool        valued = ( (open != NULL) &&
                                 (_name.data[_name.length - 1u] == '}') );

    // a name with no value and no bang
    if ( (_name.length == 0u) ||
         ( (!valued) &&
           (_name.data[0] != '!') ) )
    {
        return D_ARG_OK;
    }

    struct d_pack_text flag = valued
                              ? d_arg_internal_span(_name.data,
                                                    (size_t)(open -
                                                             _name.data))
                              : _name;

    // the dashes and bangs in front
    while ( (flag.length > 0u) &&
            ( (flag.data[0] == '-') ||
              (flag.data[0] == '!') ) )
    {
        flag = d_arg_internal_span(flag.data + 1,
                                   flag.length - 1u);
    }

    return d_arg_internal_command_keep_value(
               flag,
               valued
               ? d_arg_internal_span(open + 1,
                                     (size_t)(_name.data + _name.length -
                                              open) - 2u)
               : d_arg_internal_span("false",
                                     5u),
               _out);
}

/**
 * @brief Splits a flag's names from their values, as CLI11's
 *        get_default_flag_values and remove_default_flag_values.
 *
 * @param[in]     _names  the names.
 * @param[in,out] _out    receives the names that carry values, their values,
 *                        and the names without values.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * CLI11's get_default_flag_values over split_names -- each name between
 * commas, trimmed -- and remove_default_flag_values on a copy.
 */
D_STATIC int
d_arg_internal_command_split_flag(
    const char*                               _names,
    struct d_arg_internal_command_flag_names* _out
)
{
    const struct d_pack_text names  = d_arg_internal_text_of(_names);
    size_t                   start  = 0u;
    int                      status = D_ARG_OK;

    // each name between commas
    while ( (status == D_ARG_OK) &&
            (start <= names.length) )
    {
        const char* const comma = (const char*)memchr(names.data + start,
                                                      ',',
                                                      names.length - start);
        const size_t      end   = (comma != NULL)
                                  ? (size_t)(comma - names.data)
                                  : names.length;

        status = d_arg_internal_command_flag_value(
                     d_arg_internal_trim(
                         d_arg_internal_span(names.data + start,
                                             end - start)),
                     _out);
        start  = end + 1u;
    }

    // the names without their values
    if (status == D_ARG_OK)
    {
        status = d_arg_buffer_append_text(&_out->plain,
                                          names);
        d_arg_internal_command_strip_values(&_out->plain);
    }

    return status;
}

/**
 * @brief Gives a flag its names that carry values, and the values.
 *
 * @param[in,out] _option  the flag.
 * @param[in]     _split   the split names.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, unrecorded.
 *
 * The names that carry values, and the values, to the flag.
 */
D_STATIC int
d_arg_internal_command_give_values(
    struct d_arg_option*                            _option,
    const struct d_arg_internal_command_flag_names* _split
)
{
    int status = D_ARG_OK;

    // each name and its value, side by side
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _split->names.count);
         ++index)
    {
        status = d_arg_internal_string_copy(
                     &_option->flag_names,
                     d_arg_internal_text_of(_split->names.items[index]),
                     NULL);

        if (status == D_ARG_OK)
        {
            status = d_arg_internal_string_copy(
                         &_option->flag_values,
                         d_arg_internal_text_of(_split->values.items[index]),
                         NULL);
        }
    }

    return status;
}

/**
 * @brief Finishes a flag, as the end of CLI11's _add_flag_internal.
 *
 * @param[in,out] _command  the command.
 * @param[in,out] _option   the flag.
 * @return the flag; NULL for one with a positional name, removed,
 *         D_ARG_ERROR_INCORRECT_CONSTRUCTION recorded as a definition error.
 *
 * The end of CLI11's _add_flag_internal: a flag with a positional name is
 * removed and refused, "<name>: Flags cannot be positional"; any other keeps
 * its last value, expects none, and is never required.
 */
D_STATIC struct d_arg_option*
d_arg_internal_command_finish_flag(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    // a flag cannot be positional
    if (d_arg_option_is_positional(_option))
    {
        struct d_arg_buffer name = D_ARG_BUFFER_INIT;

        (void)d_arg_option_name(_option,
                                D_ARG_NAME_POSITIONAL,
                                &name);
        (void)d_arg_internal_fail_definition(
                  _command,
                  NULL,
                  D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                  "%s: Flags cannot be positional",
                  d_arg_buffer_text(&name).data);
        d_arg_buffer_release(&name);
        (void)d_arg_command_remove_option(_command,
                                          _option);

        return NULL;
    }

    d_arg_option_set_multi_option_policy(_option,
                                         (int)D_ARG_MULTI_TAKE_LAST);
    d_arg_option_set_expected(_option,
                              0);
    d_arg_option_set_required(_option,
                              false);

    return _option;
}

/**
 * @brief Adds the option for a flag whose names carry values.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @param[in]     _new          what the option is given; NULL for nothing.
 * @param[out]    _option       receives the option -- NULL when it was refused,
 *                              the refusal recorded; untouched when the names
 *                              could not be split.
 * @return D_ARG_OK, also for an option refused; D_ARG_ERROR_OUT_OF_MEMORY,
 *         unrecorded, when the names could not be split -- `_new`'s context
 *         then freed -- or the values could not be given.
 *
 * The option for a flag with names carrying values: added under the names
 * without them, then given the values. Returns the status, with the option --
 * NULL when it was refused -- in `*_option`.
 */
D_STATIC int
d_arg_internal_command_add_valued(
    struct d_arg_command*                    _command,
    const char*                              _names,
    const char*                              _description,
    const struct d_arg_internal_command_new* _new,
    struct d_arg_option**                    _option
)
{
    struct d_arg_internal_command_flag_names split  = { D_ARG_BUFFER_INIT,
                                                        D_ARG_LIST_INIT,
                                                        D_ARG_LIST_INIT };
    int                                      status =
        d_arg_internal_command_split_flag(_names,
                                          &split);

    // the option, under the names without values, then the values
    if (status == D_ARG_OK)
    {
        *_option = d_arg_internal_command_add(
                       _command,
                       d_arg_buffer_text(&split.plain).data,
                       _description,
                       _new);
        status   = (*_option != NULL)
                   ? d_arg_internal_command_give_values(*_option,
                                                        &split)
                   : D_ARG_OK;
    }
    else
    {
        d_arg_internal_command_release_new(_new);
    }

    d_arg_buffer_release(&split.plain);
    d_arg_string_list_release(&split.names);
    d_arg_string_list_release(&split.values);

    return status;
}

/**
 * @brief Adds a flag, as CLI11's _add_flag_internal.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names, values among them.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @param[in]     _new          what the flag is given; NULL for nothing.
 * @return the flag; NULL when it was refused, the refusal recorded as a
 *         definition error.
 *
 * CLI11's _add_flag_internal: names carrying values are split off first;
 * the option is added, and finished as a flag. A flag that cannot take its
 * values is removed and the failure recorded.
 */
D_STATIC struct d_arg_option*
d_arg_internal_command_add_flag(
    struct d_arg_command*                    _command,
    const char*                              _names,
    const char*                              _description,
    const struct d_arg_internal_command_new* _new
)
{
    struct d_arg_option* option = NULL;
    int                  status = D_ARG_OK;

    // names carrying values are split off first
    if (strpbrk(_names,
                "{!") != NULL)
    {
        status = d_arg_internal_command_add_valued(_command,
                                                   _names,
                                                   _description,
                                                   _new,
                                                   &option);
    }
    else
    {
        option = d_arg_internal_command_add(_command,
                                            _names,
                                            _description,
                                            _new);
    }

    // a refused split, or values the flag could not take
    if (status != D_ARG_OK)
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             status,
                                             NULL);
        (void)d_arg_command_remove_option(_command,
                                          option);

        return NULL;
    }

    return (option != NULL)
           ? d_arg_internal_command_finish_flag(_command,
                                                option)
           : NULL;
}

/*
d_arg_command_add_flag
  A flag with nothing else.
*/
struct d_arg_option*
d_arg_command_add_flag(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _description
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_names) )
    {
        return NULL;
    }

    return d_arg_internal_command_add_flag(_command,
                                           _names,
                                           _description,
                                           NULL);
}

/**
 * @brief Applies CLI11's default_flag_modifiers for a flag's type.
 *
 * @param[in,out] _option  the flag.
 * @param[in]     _type    the bound variable's type.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition
 *         error, when the default could not be set.
 *
 * CLI11's default_flag_modifiers and its container overload: a container
 * takes every value and is set from a default; an integer wider than a byte
 * sums the values, defaults to "0" and always calls back; anything else is
 * marked to capture its default -- which, as in CLI11, a flag made here
 * never does, its variable not being its default function.
 */
D_STATIC int
d_arg_internal_command_flag_modifiers(
    struct d_arg_option*     _option,
    const struct d_arg_type* _type
)
{
    // a container
    if ((_type->flags & D_ARG_TYPE_FLAG_CONTAINER) != 0u)
    {
        d_arg_option_set_multi_option_policy(_option,
                                             (int)D_ARG_MULTI_TAKE_ALL);
        d_arg_option_set_run_callback_for_default(_option,
                                                  true);

        return D_ARG_OK;
    }

    // a counting integer
    if ( ((_type->flags & D_ARG_TYPE_FLAG_INTEGER) != 0u) &&
         (_type->size > 1u) )
    {
        d_arg_option_set_multi_option_policy(_option,
                                             (int)D_ARG_MULTI_SUM);
        d_arg_option_set_force_callback(_option,
                                        true);

        return d_arg_option_set_default_text(_option,
                                             "0");
    }

    d_arg_option_set_always_capture_default(_option,
                                            true);

    return D_ARG_OK;
}

/*
d_arg_command_add_flag_bound
  CLI11's add_flag(name, variable): a flag whose first value converts into
the variable -- attached without becoming the default function, and with no
type name, as CLI11's flag has neither -- then the type's modifiers; a flag
that cannot be given them is refused whole.
*/
struct d_arg_option*
d_arg_command_add_flag_bound(
    struct d_arg_command*    _command,
    const char*              _names,
    void*                    _target,
    const struct d_arg_type* _type,
    const char*              _description
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_names)   ||
         (!_target)  ||
         (!_type) )
    {
        return NULL;
    }

    // the variable needs a type that converts
    if (!_type->convert)
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             D_ARG_ERROR_BAD_ARGUMENT,
                                             NULL);

        return NULL;
    }

    struct d_arg_option* const option = d_arg_internal_command_add_flag(
                                            _command,
                                            _names,
                                            _description,
                                            NULL);

    // refused, recorded
    if (!option)
    {
        return NULL;
    }

    option->type   = _type;
    option->target = _target;

    int status = d_arg_option_set_type_name(option,
                                            "");

    // the type's modifiers, once the name is in place
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_flag_modifiers(option,
                                                       _type);
    }

    return (status == D_ARG_OK)
           ? option
           : d_arg_internal_command_refuse_option(_command,
                                                  option,
                                                  status);
}

// d_arg_internal_command_flag_call
//   struct: what a flag made by add_flag_callback or add_flag_function keeps:
// the caller's function -- `function`, or `wide` for one given the sum as
// an intmax_t, the other NULL -- its context, and the function that frees
// it.
struct d_arg_internal_command_flag_call
{
    d_arg_flag_fn        function;
    d_arg_flag_intmax_fn wide;
    void*                context;
    d_arg_release_fn     release;
};

/**
 * @brief Frees a flag-callback record and the context it owns.
 *
 * @param[in] _context  the record; NULL is ignored.
 *
 * The caller's context through its function, then the record.
 */
D_STATIC void
d_arg_internal_command_flag_call_release(
    void* _context
)
{
    struct d_arg_internal_command_flag_call* const call =
        (struct d_arg_internal_command_flag_call*)_context;

    // nothing to free without a record
    if (!call)
    {
        return;
    }

    // the caller's context, when it is owned
    if ( (call->release != NULL) &&
         (call->context != NULL) )
    {
        call->release(call->context);
    }

    free(call);

    return;
}

/**
 * @brief Gives the first value a flag's callback is given.
 *
 * @param[in] _values  the values.
 * @param[in] _count   how many.
 * @return the first value; the empty text when there is none.
 *
 * The first value a flag's callback is given, or the empty text.
 */
D_STATIC struct d_pack_text
d_arg_internal_command_flag_first(
    const struct d_pack_text* _values,
    size_t                    _count
)
{
    return (_count > 0u)
           ? _values[0]
           : d_arg_internal_span("",
                                 0u);
}

/**
 * @brief Calls a flag's function when its value is true, as CLI11's
 *        add_flag_callback lambda.
 *
 * @param[in] _option   the flag.
 * @param[in] _values   the values.
 * @param[in] _count    how many.
 * @param[in] _context  the d_arg_internal_command_flag_call.
 * @return D_ARG_OK when the value is false; D_ARG_ERROR_CONVERSION for a value
 *         that is no truth value; otherwise the function's status.
 *
 * CLI11's add_flag_callback lambda: the first value read as a truth value --
 * one that is none refuses the values -- and the function called with 1 when
 * it is true.
 */
D_STATIC int
d_arg_internal_command_flag_trigger(
    struct d_arg_option*      _option,
    const struct d_pack_text* _values,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_arg_internal_command_flag_call* const call =
        (const struct d_arg_internal_command_flag_call*)_context;
    bool                                                 trigger = false;

    // a value that is no truth value refuses the values
    if (d_arg_parse_bool(d_arg_internal_command_flag_first(_values,
                                                           _count),
                         _option->integer_rule,
                         &trigger) != D_ARG_OK)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    return trigger
           ? call->function(_option,
                            1,
                            call->context)
           : D_ARG_OK;
}

/**
 * @brief Calls a flag's function with the sum of its values, as CLI11's
 *        add_flag_function lambda.
 *
 * @param[in] _option   the flag.
 * @param[in] _values   the values: the sum.
 * @param[in] _count    how many.
 * @param[in] _context  the d_arg_internal_command_flag_call.
 * @return the function's status.
 *
 * CLI11's add_flag_function lambda: the first value -- the sum -- read as an
 * integer, 0 when it is none, and the function called with it.
 */
D_STATIC int
d_arg_internal_command_flag_count(
    struct d_arg_option*      _option,
    const struct d_pack_text* _values,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_arg_internal_command_flag_call* const call =
        (const struct d_arg_internal_command_flag_call*)_context;
    long                                                 total = 0;

    // a sum that is no integer counts as 0
    if (d_arg_parse_long(d_arg_internal_command_flag_first(_values,
                                                           _count),
                         _option->integer_rule,
                         &total) != D_ARG_OK)
    {
        total = 0;
    }

    return call->function(_option,
                          total,
                          call->context);
}

/**
 * @brief Calls a flag's wide function with the sum of its values, read as CLI11
 *        reads it.
 *
 * @param[in] _option   the flag.
 * @param[in] _values   the values: the sum.
 * @param[in] _count    how many.
 * @param[in] _context  the d_arg_internal_command_flag_call.
 * @return the function's status.
 *
 * CLI11's add_flag_function lambda as CLI11 reads the sum, into a 64-bit
 * integer: 0 when the sum is no integer or does not fit one.
 */
D_STATIC int
d_arg_internal_command_flag_count_intmax(
    struct d_arg_option*      _option,
    const struct d_pack_text* _values,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_arg_internal_command_flag_call* const call =
        (const struct d_arg_internal_command_flag_call*)_context;
    intmax_t                                             total = 0;

    // a sum that is no 64-bit integer counts as 0
    if ( (d_arg_parse_intmax(d_arg_internal_command_flag_first(_values,
                                                               _count),
                             _option->integer_rule,
                             &total) != D_ARG_OK) ||
         (total < INT64_MIN)                      ||
         (total > INT64_MAX) )
    {
        total = 0;
    }

    return call->wide(_option,
                      total,
                      call->context);
}

/**
 * @brief Adds a flag whose values reach the caller's function through a
 *        wrapper.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @param[in]     _call         the caller's function and context.
 * @param[in]     _wrap         the wrapper.
 * @return the flag; NULL when it was refused, the refusal recorded as a
 *         definition error and the caller's context freed.
 *
 * A flag whose values reach the caller's function through `_wrap`; the
 * caller's context is freed with the flag or, on any failure, at once.
 */
D_STATIC struct d_arg_option*
d_arg_internal_command_add_flag_call(
    struct d_arg_command*                          _command,
    const char*                                    _names,
    const char*                                    _description,
    const struct d_arg_internal_command_flag_call* _call,
    d_arg_results_fn                               _wrap
)
{
    struct d_arg_internal_command_flag_call* const call =
        (struct d_arg_internal_command_flag_call*)malloc(sizeof(*call));

    // check if memory allocation was successful
    if (!call)
    {
        if ( (_call->release != NULL) &&
             (_call->context != NULL) )
        {
            _call->release(_call->context);
        }

        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             D_ARG_ERROR_OUT_OF_MEMORY,
                                             NULL);

        return NULL;
    }

    const struct d_arg_internal_command_new wrapped =
    {
        _wrap,
        call,
        d_arg_internal_command_flag_call_release,
        NULL,
        NULL
    };

    *call = *_call;

    return d_arg_internal_command_add_flag(_command,
                                           _names,
                                           _description,
                                           &wrapped);
}

/*
d_arg_command_add_flag_callback
  A flag calling the function with 1 when given true.
*/
struct d_arg_option*
d_arg_command_add_flag_callback(
    struct d_arg_command* _command,
    const char*           _names,
    d_arg_flag_fn         _function,
    void*                 _context,
    d_arg_release_fn      _release,
    const char*           _description
)
{
    const struct d_arg_internal_command_flag_call call = { _function,
                                                           NULL,
                                                           _context,
                                                           _release };

    // validate the parameters; the context goes either way
    if ( (!_command) ||
         (!_names)   ||
         (!_function) )
    {
        if ( (_release != NULL) &&
             (_context != NULL) )
        {
            _release(_context);
        }

        return NULL;
    }

    return d_arg_internal_command_add_flag_call(
               _command,
               _names,
               _description,
               &call,
               d_arg_internal_command_flag_trigger);
}

/**
 * @brief Adds a flag summing its values, the sum reaching the caller's function
 *        through a wrapper.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @param[in]     _call         the caller's function and context.
 * @param[in]     _wrap         the wrapper.
 * @return the flag; NULL for a NULL command, names or function, or for a flag
 *         refused -- the refusal recorded as a definition error -- the caller's
 *         context freed either way.
 *
 * A flag summing its values, the sum reaching the caller's function through
 * `_wrap`: the parameters checked -- the context freed when they fail -- and
 * the sum policy set.
 */
D_STATIC struct d_arg_option*
d_arg_internal_command_add_flag_sum(
    struct d_arg_command*                          _command,
    const char*                                    _names,
    const char*                                    _description,
    const struct d_arg_internal_command_flag_call* _call,
    d_arg_results_fn                               _wrap
)
{
    // validate the parameters; the context goes either way
    if ( (!_command) ||
         (!_names)   ||
         ( (!_call->function) &&
           (!_call->wide) ) )
    {
        if ( (_call->release != NULL) &&
             (_call->context != NULL) )
        {
            _call->release(_call->context);
        }

        return NULL;
    }

    struct d_arg_option* const option =
        d_arg_internal_command_add_flag_call(_command,
                                             _names,
                                             _description,
                                             _call,
                                             _wrap);

    // it sums its values
    if (option != NULL)
    {
        d_arg_option_set_multi_option_policy(option,
                                             (int)D_ARG_MULTI_SUM);
    }

    return option;
}

/*
d_arg_command_add_flag_function
  A flag summing its values and calling the function with the sum.
*/
struct d_arg_option*
d_arg_command_add_flag_function(
    struct d_arg_command* _command,
    const char*           _names,
    d_arg_flag_fn         _function,
    void*                 _context,
    d_arg_release_fn      _release,
    const char*           _description
)
{
    const struct d_arg_internal_command_flag_call call = { _function,
                                                           NULL,
                                                           _context,
                                                           _release };

    return d_arg_internal_command_add_flag_sum(
               _command,
               _names,
               _description,
               &call,
               d_arg_internal_command_flag_count);
}

/*
d_arg_command_add_flag_function_intmax
  A flag summing its values and calling the function with the sum, read as
CLI11 reads it.
*/
struct d_arg_option*
d_arg_command_add_flag_function_intmax(
    struct d_arg_command* _command,
    const char*           _names,
    d_arg_flag_intmax_fn  _function,
    void*                 _context,
    d_arg_release_fn      _release,
    const char*           _description
)
{
    const struct d_arg_internal_command_flag_call call = { NULL,
                                                           _function,
                                                           _context,
                                                           _release };

    return d_arg_internal_command_add_flag_sum(
               _command,
               _names,
               _description,
               &call,
               d_arg_internal_command_flag_count_intmax);
}

//==============================================================================
// 4.  REMOVING
//==============================================================================

/**
 * @brief Drops every reference to an option in a command and below it.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _option   the option forgotten.
 *
 * Every reference to an option in a command and below it: its options'
 * needs and excludes, the command's own, the special options it points at,
 * and the last parse's order.
 */
D_STATIC void
d_arg_internal_command_sweep_option(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    // the options' links
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        (void)d_arg_option_remove_needs(_command->options.items[index],
                                        _option);
        (void)d_arg_option_remove_excludes(_command->options.items[index],
                                           _option);
    }

    (void)d_arg_internal_option_remove(&_command->exclude_options,
                                       _option);
    (void)d_arg_internal_option_remove(&_command->need_options,
                                       _option);

    // the special options -- a version flag may live in an option group
    _command->help     = (_command->help == _option) ? NULL : _command->help;
    _command->help_all = (_command->help_all == _option)
                         ? NULL
                         : _command->help_all;
    _command->version  = (_command->version == _option)
                         ? NULL
                         : _command->version;
    _command->config   = (_command->config == _option)
                         ? NULL
                         : _command->config;

    // every place it took in the parse order
    while (d_arg_internal_option_remove(&_command->parse_order,
                                        _option))
    {
    }

    // the subcommands', in turn
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        d_arg_internal_command_sweep_option(_command->subcommands.items[index],
                                            _option);
    }

    return;
}

/*
d_arg_internal_command_forget_option
  The sweep from the root of the command's tree down, then the parser's
completion option.
*/
void
d_arg_internal_command_forget_option(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    struct d_arg_command* root = _command;

    // from the root down
    while (root->parent != NULL)
    {
        root = root->parent;
    }

    d_arg_internal_command_sweep_option(root,
                                        _option);

    // the parser's completion option, too
    if ( (root->parser != NULL) &&
         (root->parser->completion == _option) )
    {
        root->parser->completion = NULL;
    }

    return;
}

/*
d_arg_command_remove_option
  CLI11's remove_option, made safe: every reference to the option in the
whole tree is dropped -- CLI11 drops only its own command's options' -- and
it is released.
*/
bool
d_arg_command_remove_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    // only an option the command owns
    if ( (!_command) ||
         (!_option)  ||
         (!d_arg_internal_option_has(&_command->options,
                                     _option)) )
    {
        return false;
    }

    d_arg_internal_command_forget_option(_command,
                                         _option);
    (void)d_arg_internal_option_remove(&_command->options,
                                       _option);
    d_arg_internal_option_destroy(_option);

    return true;
}

/*
d_arg_internal_command_refuse_option
  The failure recorded first -- d_arg_internal_fail_definition keeps the one
the failing step recorded, when it recorded one -- then the option removed
as d_arg_command_remove_option removes one, with every reference to it.
*/
struct d_arg_option*
d_arg_internal_command_refuse_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option,
    int                   _status
)
{
    (void)d_arg_internal_fail_definition(_command,
                                         NULL,
                                         _status,
                                         NULL);
    (void)d_arg_command_remove_option(_command,
                                      _option);

    return NULL;
}
