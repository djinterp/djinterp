/*******************************************************************************
* djinterp [parse]                                             arg_command_set.c
*
* A command's settings, declared in arg_command.h (section 5).
*   CLI11's App setters: the name, aliases and texts -- a subcommand's names
* checked against its fallthrough parent's other subcommands as CLI11 checks
* them -- the modes and switches, the subcommand and option requirements,
* the callbacks, the needs and excludes, the formatters, and the defaults
* new options start from.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_command_set.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // strchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, callbacks, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option_defaults
#include "./arg_internal.h"  // failures, lists, contexts, commands
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  NAMES AND TEXT
//==============================================================================

/**
 * @brief Finds a name on which a subcommand clashes with its fallthrough
 *        parent's others.
 *
 * @param[in] _command  the subcommand.
 * @return the name or alias that clashes, owned by one of the commands; NULL
 *         when none does, or for a root.
 *
 * CLI11's check in name() and alias(): a name or alias of a subcommand that
 * matches one of its fallthrough parent's other subcommands; NULL for a
 * root, which is not checked.
 */
D_STATIC const char*
d_arg_internal_command_clash(
    const struct d_arg_command* _command
)
{
    return (_command->parent != NULL)
           ? d_arg_internal_subcommand_clash(
                 _command,
                 d_arg_internal_fallthrough_parent(_command))
           : NULL;
}

/*
d_arg_command_set_name
  CLI11's name(): the new name put in place and checked, the old one put
back when it clashes; the name is then no longer automatic.
*/
int
d_arg_command_set_name(
    struct d_arg_command* _command,
    const char*           _name
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool  given = ( (_name != NULL) &&
                          (_name[0] != '\0') );
    char* const name  = given
                        ? d_arg_internal_copy_cstr(_name)
                        : NULL;
    char* const old   = _command->name;

    // check if memory allocation was successful
    if ( (given) &&
         (!name) )
    {
        return d_arg_internal_fail_definition(_command,
                                              NULL,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    _command->name = name;

    // a clash puts the old name back
    if (d_arg_internal_command_clash(_command) != NULL)
    {
        _command->name = old;
        free(name);

        return d_arg_internal_fail_definition(
                   _command,
                   NULL,
                   D_ARG_ERROR_OPTION_ALREADY_ADDED,
                   "%s conflicts with existing subcommand names is already "
                   "added",
                   given ? _name : "");
    }

    free(old);
    _command->flags &= ~D_ARG_COMMAND_AUTOMATIC_NAME;

    return D_ARG_OK;
}

/*
d_arg_command_add_alias
  CLI11's alias(): an empty alias or one with a newline refused; otherwise
the alias added and checked, and taken off again when it clashes.
*/
int
d_arg_command_add_alias(
    struct d_arg_command* _command,
    const char*           _alias
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // an empty alias, or one with a newline
    if ( (!_alias)            ||
         (_alias[0] == '\0')  ||
         (strchr(_alias,
                 '\n') != NULL) )
    {
        return d_arg_internal_fail_definition(
                   _command,
                   NULL,
                   D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                   "Aliases may not be empty or contain newlines or null "
                   "characters");
    }

    const int status = d_arg_internal_string_copy(
                           &_command->aliases,
                           d_arg_internal_text_of(_alias),
                           NULL);

    // check if memory allocation was successful
    if (status != D_ARG_OK)
    {
        return d_arg_internal_fail_definition(_command,
                                              NULL,
                                              status,
                                              NULL);
    }

    // a clash takes it off again
    if (d_arg_internal_command_clash(_command) != NULL)
    {
        d_arg_internal_string_truncate(&_command->aliases,
                                       _command->aliases.count - 1u);

        return d_arg_internal_fail_definition(
                   _command,
                   NULL,
                   D_ARG_ERROR_OPTION_ALREADY_ADDED,
                   "alias already matches an existing subcommand: %s is "
                   "already added",
                   _alias);
    }

    return D_ARG_OK;
}

/*
d_arg_command_clear_aliases
  CLI11's clear_aliases().
*/
void
d_arg_command_clear_aliases(
    struct d_arg_command* _command
)
{
    // nothing to clear without a command
    if (_command != NULL)
    {
        d_arg_internal_string_truncate(&_command->aliases,
                                       0u);
    }

    return;
}

/**
 * @brief Replaces an owned text field of a command with a copy.
 *
 * @param[in,out] _command  the command the field belongs to.
 * @param[in,out] _field    the field, its old text freed once the copy exists.
 * @param[in]     _text     the text, copied; NULL clears the field.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition error,
 *         the field then unchanged.
 *
 * An owned text field replaced with a copy -- NULL clears it -- the old
 * text freed only once the copy exists.
 */
D_STATIC int
d_arg_internal_command_replace_text(
    struct d_arg_command* _command,
    char**                _field,
    const char*           _text
)
{
    char* const copy = (_text != NULL)
                       ? d_arg_internal_copy_cstr(_text)
                       : NULL;

    // check if memory allocation was successful
    if ( (_text != NULL) &&
         (!copy) )
    {
        return d_arg_internal_fail_definition(_command,
                                              NULL,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    free(*_field);
    *_field = copy;

    return D_ARG_OK;
}

/*
d_arg_command_set_description
  CLI11's description().
*/
int
d_arg_command_set_description(
    struct d_arg_command* _command,
    const char*           _description
)
{
    return (_command != NULL)
           ? d_arg_internal_command_replace_text(_command,
                                                 &_command->description,
                                                 _description)
           : D_ARG_ERROR_BAD_ARGUMENT;
}

/*
d_arg_command_set_usage
  CLI11's usage(string).
*/
int
d_arg_command_set_usage(
    struct d_arg_command* _command,
    const char*           _usage
)
{
    return (_command != NULL)
           ? d_arg_internal_command_replace_text(_command,
                                                 &_command->usage,
                                                 _usage)
           : D_ARG_ERROR_BAD_ARGUMENT;
}

/*
d_arg_command_set_footer
  CLI11's footer(string).
*/
int
d_arg_command_set_footer(
    struct d_arg_command* _command,
    const char*           _footer
)
{
    return (_command != NULL)
           ? d_arg_internal_command_replace_text(_command,
                                                 &_command->footer,
                                                 _footer)
           : D_ARG_ERROR_BAD_ARGUMENT;
}

/*
d_arg_command_set_group
  CLI11's group(); NULL reads as "SUBCOMMANDS" again.
*/
int
d_arg_command_set_group(
    struct d_arg_command* _command,
    const char*           _group
)
{
    return (_command != NULL)
           ? d_arg_internal_command_replace_text(_command,
                                                 &_command->group,
                                                 _group)
           : D_ARG_ERROR_BAD_ARGUMENT;
}

/**
 * @brief Puts a callback's new context in place of the one kept.
 *
 * @param[in]     _given        whether a function came with it.
 * @param[in,out] _context      the context kept.
 * @param[in,out] _release      the function that frees it.
 * @param[in]     _new_context  the new context.
 * @param[in]     _new_release  frees it; NULL leaves it borrowed.
 *
 * A callback's context put in place of the one kept -- freed unless given
 * again -- and, when no function came with it, freed too, for a removed
 * callback keeps nothing.
 */
D_STATIC void
d_arg_internal_command_keep_callback(
    bool              _given,
    void**            _context,
    d_arg_release_fn* _release,
    void*             _new_context,
    d_arg_release_fn  _new_release
)
{
    d_arg_internal_context_replace(_context,
                                   _release,
                                   _new_context,
                                   _new_release);

    // a removed callback keeps no context
    if (!_given)
    {
        d_arg_internal_context_release(_context,
                                       _release);
    }

    return;
}

/*
d_arg_command_set_usage_fn
  CLI11's usage(function).
*/
void
d_arg_command_set_usage_fn(
    struct d_arg_command* _command,
    d_arg_command_text_fn _write,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_write != NULL,
                                         &_command->usage_context,
                                         &_command->usage_release,
                                         _context,
                                         _release);
    _command->usage_fn = _write;

    return;
}

/*
d_arg_command_set_footer_fn
  CLI11's footer(function).
*/
void
d_arg_command_set_footer_fn(
    struct d_arg_command* _command,
    d_arg_command_text_fn _write,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_write != NULL,
                                         &_command->footer_context,
                                         &_command->footer_release,
                                         _context,
                                         _release);
    _command->footer_fn = _write;

    return;
}

//==============================================================================
// 2.  MODES AND SWITCHES
//==============================================================================

/*
d_arg_command_set_allow_extras
  CLI11's allow_extras(ExtrasMode).
*/
void
d_arg_command_set_allow_extras(
    struct d_arg_command* _command,
    int                   _mode
)
{
    // nothing to set without a command, nor outside the enum
    if ( (_command != NULL) &&
         (d_arg_internal_in_range(_mode,
                                  (int)D_ARG_EXTRAS_ERROR,
                                  (int)D_ARG_EXTRAS_CAPTURE)) )
    {
        _command->allow_extras = _mode;
    }

    return;
}

/*
d_arg_command_set_allow_config_extras
  CLI11's allow_config_extras(ConfigExtrasMode).
*/
void
d_arg_command_set_allow_config_extras(
    struct d_arg_command* _command,
    int                   _mode
)
{
    // nothing to set without a command, nor outside the enum
    if ( (_command != NULL) &&
         (d_arg_internal_in_range(_mode,
                                  (int)D_ARG_CONFIG_EXTRAS_ERROR,
                                  (int)D_ARG_CONFIG_EXTRAS_CAPTURE)) )
    {
        _command->allow_config_extras = _mode;
    }

    return;
}

/*
d_arg_command_set_prefix_command
  CLI11's prefix_command(PrefixCommandMode).
*/
void
d_arg_command_set_prefix_command(
    struct d_arg_command* _command,
    int                   _mode
)
{
    // nothing to set without a command, nor outside the enum
    if ( (_command != NULL) &&
         (d_arg_internal_in_range(_mode,
                                  (int)D_ARG_PREFIX_OFF,
                                  (int)D_ARG_PREFIX_POSITIONAL_ONLY)) )
    {
        _command->prefix_command = _mode;
    }

    return;
}

/*
d_arg_command_set_allow_config_extras_bool
  CLI11's allow_config_extras(bool): true captures configuration and
command-line extras alike; false makes configuration extras errors and
leaves the other mode alone.
*/
void
d_arg_command_set_allow_config_extras_bool(
    struct d_arg_command* _command,
    bool                  _allow
)
{
    // nothing to set without a command
    if (!_command)
    {
        return;
    }

    _command->allow_config_extras = _allow
                                    ? (int)D_ARG_CONFIG_EXTRAS_CAPTURE
                                    : (int)D_ARG_CONFIG_EXTRAS_ERROR;
    _command->allow_extras        = _allow
                                    ? (int)D_ARG_EXTRAS_CAPTURE
                                    : _command->allow_extras;

    return;
}

/**
 * @brief Sets or clears one D_ARG_COMMAND_* flag of a command.
 *
 * @param[in,out] _command  the command; NULL is ignored.
 * @param[in]     _flag     the flag.
 * @param[in]     _value    whether it is set.
 *
 * Sets or clears one D_ARG_COMMAND_* flag of a command, if there is one.
 */
D_STATIC void
d_arg_internal_command_switch(
    struct d_arg_command* _command,
    uint32_t              _flag,
    bool                  _value
)
{
    // nothing to switch without a command
    if (!_command)
    {
        return;
    }

    _command->flags = _value
                      ? (_command->flags | _flag)
                      : (_command->flags & ~_flag);

    return;
}

/*
d_arg_command_set_required
  CLI11's required().
*/
void
d_arg_command_set_required(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_REQUIRED,
                                  _value);

    return;
}

/*
d_arg_command_set_disabled
  CLI11's disabled().
*/
void
d_arg_command_set_disabled(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_DISABLED,
                                  _value);

    return;
}

/*
d_arg_command_set_silent
  CLI11's silent().
*/
void
d_arg_command_set_silent(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_SILENT,
                                  _value);

    return;
}

/*
d_arg_command_set_windows_style
  CLI11's allow_windows_style_options().
*/
void
d_arg_command_set_windows_style(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_WINDOWS_STYLE,
                                  _value);

    return;
}

/*
d_arg_command_set_positionals_at_end
  CLI11's positionals_at_end().
*/
void
d_arg_command_set_positionals_at_end(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_POSITIONALS_AT_END,
                                  _value);

    return;
}

/*
d_arg_command_set_configurable
  CLI11's configurable().
*/
void
d_arg_command_set_configurable(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_CONFIGURABLE,
                                  _value);

    return;
}

/*
d_arg_command_set_validate_positionals
  CLI11's validate_positionals().
*/
void
d_arg_command_set_validate_positionals(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_VALIDATE_POSITIONALS,
                                  _value);

    return;
}

/*
d_arg_command_set_validate_optional_arguments
  CLI11's validate_optional_arguments().
*/
void
d_arg_command_set_validate_optional_arguments(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS,
                                  _value);

    return;
}

/*
d_arg_command_set_non_standard_names
  CLI11's allow_non_standard_option_names().
*/
void
d_arg_command_set_non_standard_names(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_NON_STANDARD_NAMES,
                                  _value);

    return;
}

/*
d_arg_command_set_prefix_matching
  CLI11's allow_subcommand_prefix_matching().
*/
void
d_arg_command_set_prefix_matching(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_PREFIX_MATCHING,
                                  _value);

    return;
}

/*
d_arg_command_set_fallthrough
  CLI11's fallthrough().
*/
void
d_arg_command_set_fallthrough(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_FALLTHROUGH,
                                  _value);

    return;
}

/*
d_arg_command_set_subcommand_fallthrough
  CLI11's subcommand_fallthrough().
*/
void
d_arg_command_set_subcommand_fallthrough(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH,
                                  _value);

    return;
}

/**
 * @brief Turns a command's ignore-case or ignore-underscore switch on or off.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _flag     the switch: D_ARG_COMMAND_IGNORE_CASE or
 *                          D_ARG_COMMAND_IGNORE_UNDERSCORE.
 * @param[in]     _value    whether it is turned on.
 * @param[in]     _word     the switch's word in CLI11's message: `case` or
 *                          `underscore`.
 * @return D_ARG_OK; D_ARG_ERROR_BAD_ARGUMENT for no command;
 *         D_ARG_ERROR_OPTION_ALREADY_ADDED, recorded as a definition error with
 *         the switch left off, when the command's names would then clash.
 *
 * CLI11's ignore_case() and ignore_underscore(): turning the switch on is
 * refused when the command's names would then match another of its
 * fallthrough parent's subcommands -- a root's own, at the root -- the switch
 * left off; anything else is simply set.
 */
D_STATIC int
d_arg_internal_command_set_matching(
    struct d_arg_command* _command,
    uint32_t              _flag,
    bool                  _value,
    const char*           _word
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // only turning it on can make names clash
    if ( (_value) &&
         (!d_arg_command_has_flag(_command,
                                  _flag)) )
    {
        _command->flags |= _flag;

        const char* const clash = d_arg_internal_subcommand_clash(
                                      _command,
                                      (_command->parent != NULL)
                                      ? d_arg_internal_fallthrough_parent(
                                            _command)
                                      : _command);

        if (clash != NULL)
        {
            _command->flags &= ~_flag;

            return d_arg_internal_fail_definition(
                       _command,
                       NULL,
                       D_ARG_ERROR_OPTION_ALREADY_ADDED,
                       "ignore %s would cause subcommand name conflicts: %s "
                       "is already added",
                       _word,
                       clash);
        }
    }

    d_arg_internal_command_switch(_command,
                                  _flag,
                                  _value);

    return D_ARG_OK;
}

/*
d_arg_command_set_ignore_case
  CLI11's ignore_case().
*/
int
d_arg_command_set_ignore_case(
    struct d_arg_command* _command,
    bool                  _value
)
{
    return d_arg_internal_command_set_matching(_command,
                                               D_ARG_COMMAND_IGNORE_CASE,
                                               _value,
                                               "case");
}

/*
d_arg_command_set_ignore_underscore
  CLI11's ignore_underscore().
*/
int
d_arg_command_set_ignore_underscore(
    struct d_arg_command* _command,
    bool                  _value
)
{
    return d_arg_internal_command_set_matching(
               _command,
               D_ARG_COMMAND_IGNORE_UNDERSCORE,
               _value,
               "underscore");
}

/**
 * @brief Swaps a command's final and parse-complete callbacks.
 *
 * @param[in,out] _command  the command.
 *
 * The final and parse-complete callbacks change places, each with its
 * context and the function that frees it.
 */
D_STATIC void
d_arg_internal_command_swap_callbacks(
    struct d_arg_command* _command
)
{
    const d_arg_command_fn function = _command->final_fn;
    void* const            context  = _command->final_context;
    const d_arg_release_fn release  = _command->final_release;

    _command->final_fn               = _command->parse_complete_fn;
    _command->final_context          = _command->parse_complete_context;
    _command->final_release          = _command->parse_complete_release;
    _command->parse_complete_fn      = function;
    _command->parse_complete_context = context;
    _command->parse_complete_release = release;

    return;
}

/*
d_arg_command_set_immediate_callback
  CLI11's immediate_callback(): turned on, a final callback alone becomes
the parse-complete one; turned off, a parse-complete callback alone
becomes the final one.
*/
void
d_arg_command_set_immediate_callback(
    struct d_arg_command* _command,
    bool                  _value
)
{
    // nothing to set without a command
    if (!_command)
    {
        return;
    }

    d_arg_internal_command_switch(_command,
                                  D_ARG_COMMAND_IMMEDIATE_CALLBACK,
                                  _value);

    // the callback set alone moves to where the switch now runs it
    if ( ( (_value)                                  &&
           (_command->final_fn != NULL)              &&
           (_command->parse_complete_fn == NULL) )   ||
         ( (!_value)                                 &&
           (_command->final_fn == NULL)              &&
           (_command->parse_complete_fn != NULL) ) )
    {
        d_arg_internal_command_swap_callbacks(_command);
    }

    return;
}

/*
d_arg_command_set_disabled_by_default
  CLI11's disabled_by_default(): on, every parse starts the command
disabled; off, it starts as it is, unless it starts enabled.
*/
void
d_arg_command_set_disabled_by_default(
    struct d_arg_command* _command,
    bool                  _value
)
{
    // nothing to set without a command
    if (!_command)
    {
        return;
    }

    _command->startup = (_value)
                        ? (int)D_ARG_STARTUP_DISABLED
                        : ( (_command->startup == (int)D_ARG_STARTUP_ENABLED)
                            ? (int)D_ARG_STARTUP_ENABLED
                            : (int)D_ARG_STARTUP_STABLE );

    return;
}

/*
d_arg_command_set_enabled_by_default
  CLI11's enabled_by_default(), the mirror of disabled_by_default.
*/
void
d_arg_command_set_enabled_by_default(
    struct d_arg_command* _command,
    bool                  _value
)
{
    // nothing to set without a command
    if (!_command)
    {
        return;
    }

    _command->startup = (_value)
                        ? (int)D_ARG_STARTUP_ENABLED
                        : ( (_command->startup == (int)D_ARG_STARTUP_DISABLED)
                            ? (int)D_ARG_STARTUP_DISABLED
                            : (int)D_ARG_STARTUP_STABLE );

    return;
}

//==============================================================================
// 3.  REQUIREMENTS
//==============================================================================

/**
 * @brief Reads a count as CLI11's require_subcommand(int) and
 *        require_option(int) do.
 *
 * @param[in]  _value    the count: `n` for exactly `n`, `-n` for at most `n`.
 * @param[out] _minimum  receives the least.
 * @param[out] _maximum  receives the most.
 *
 * CLI11's reading of require_subcommand(int) and require_option(int): `n`
 * is exactly `n`, `-n` at most `n` -- its magnitude taken without
 * overflowing at INT_MIN, where CLI11's negation overflows.
 */
D_STATIC void
d_arg_internal_command_count_limits(
    int     _value,
    size_t* _minimum,
    size_t* _maximum
)
{
    // a negative count is a maximum alone
    if (_value < 0)
    {
        *_minimum = 0u;
        *_maximum = (size_t)(-(_value + 1)) + 1u;

        return;
    }

    *_minimum = (size_t)_value;
    *_maximum = (size_t)_value;

    return;
}

/*
d_arg_command_require_subcommand
  CLI11's require_subcommand(min, max).
*/
void
d_arg_command_require_subcommand(
    struct d_arg_command* _command,
    size_t                _minimum,
    size_t                _maximum
)
{
    // nothing to set without a command
    if (_command != NULL)
    {
        _command->require_subcommand_min = _minimum;
        _command->require_subcommand_max = _maximum;
    }

    return;
}

/*
d_arg_command_require_subcommand_count
  CLI11's require_subcommand(int).
*/
void
d_arg_command_require_subcommand_count(
    struct d_arg_command* _command,
    int                   _value
)
{
    // nothing to set without a command
    if (_command != NULL)
    {
        d_arg_internal_command_count_limits(_value,
                                            &_command->require_subcommand_min,
                                            &_command->require_subcommand_max);
    }

    return;
}

/*
d_arg_command_require_option
  CLI11's require_option(min, max).
*/
void
d_arg_command_require_option(
    struct d_arg_command* _command,
    size_t                _minimum,
    size_t                _maximum
)
{
    // nothing to set without a command
    if (_command != NULL)
    {
        _command->require_option_min = _minimum;
        _command->require_option_max = _maximum;
    }

    return;
}

/*
d_arg_command_require_option_count
  CLI11's require_option(int).
*/
void
d_arg_command_require_option_count(
    struct d_arg_command* _command,
    int                   _value
)
{
    // nothing to set without a command
    if (_command != NULL)
    {
        d_arg_internal_command_count_limits(_value,
                                            &_command->require_option_min,
                                            &_command->require_option_max);
    }

    return;
}

//==============================================================================
// 4.  CALLBACKS
//==============================================================================

/*
d_arg_command_set_final_callback
  CLI11's final_callback().
*/
void
d_arg_command_set_final_callback(
    struct d_arg_command* _command,
    d_arg_command_fn      _callback,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_callback != NULL,
                                         &_command->final_context,
                                         &_command->final_release,
                                         _context,
                                         _release);
    _command->final_fn = _callback;

    return;
}

/*
d_arg_command_set_parse_complete_callback
  CLI11's parse_complete_callback().
*/
void
d_arg_command_set_parse_complete_callback(
    struct d_arg_command* _command,
    d_arg_command_fn      _callback,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_callback != NULL,
                                         &_command->parse_complete_context,
                                         &_command->parse_complete_release,
                                         _context,
                                         _release);
    _command->parse_complete_fn = _callback;

    return;
}

/*
d_arg_command_set_callback
  CLI11's callback(): the parse-complete callback for a command whose
callback runs at once, the final callback otherwise.
*/
void
d_arg_command_set_callback(
    struct d_arg_command* _command,
    d_arg_command_fn      _callback,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    // which of the two, by the switch
    if (d_arg_command_has_flag(_command,
                               D_ARG_COMMAND_IMMEDIATE_CALLBACK))
    {
        d_arg_command_set_parse_complete_callback(_command,
                                                  _callback,
                                                  _context,
                                                  _release);
    }
    else
    {
        d_arg_command_set_final_callback(_command,
                                         _callback,
                                         _context,
                                         _release);
    }

    return;
}

/*
d_arg_command_set_preparse_callback
  CLI11's preparse_callback().
*/
void
d_arg_command_set_preparse_callback(
    struct d_arg_command* _command,
    d_arg_preparse_fn     _callback,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_callback != NULL,
                                         &_command->preparse_context,
                                         &_command->preparse_release,
                                         _context,
                                         _release);
    _command->preparse_fn = _callback;

    return;
}

/*
d_arg_command_set_pre_callback
  What CLI11's virtual pre_callback() gives a subclass, as a callback.
*/
void
d_arg_command_set_pre_callback(
    struct d_arg_command* _command,
    d_arg_command_fn      _callback,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_callback != NULL,
                                         &_command->pre_callback_context,
                                         &_command->pre_callback_release,
                                         _context,
                                         _release);
    _command->pre_callback_fn = _callback;

    return;
}

/*
d_arg_command_set_failure_message
  CLI11's failure_message(); NULL follows the parent's.
*/
void
d_arg_command_set_failure_message(
    struct d_arg_command* _command,
    d_arg_failure_fn      _write,
    void*                 _context,
    d_arg_release_fn      _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_command)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_command_keep_callback(_write != NULL,
                                         &_command->failure_context,
                                         &_command->failure_release,
                                         _context,
                                         _release);
    _command->failure_fn = _write;

    return;
}

//==============================================================================
// 5.  RELATIONS
//==============================================================================

/**
 * @brief Checks that an option may be linked to a command.
 *
 * @param[in] _command  the command.
 * @param[in] _option   the option.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_NOT_FOUND for no option, or
 *         D_ARG_ERROR_INCORRECT_CONSTRUCTION for another parser's, either
 *         recorded as a definition error.
 *
 * An option a command may be linked to: CLI11's refusal of nothing, then
 * the refusal of another parser's option, either recorded.
 */
D_STATIC int
d_arg_internal_command_check_option(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option
)
{
    // CLI11's refusal of nothing
    if (!_option)
    {
        return d_arg_internal_fail_definition(_command,
                                              NULL,
                                              D_ARG_ERROR_OPTION_NOT_FOUND,
                                              "nullptr passed not found");
    }

    return (d_arg_internal_parser_of(_option) == _command->parser)
           ? D_ARG_OK
           : d_arg_internal_fail_definition(
                 _command,
                 NULL,
                 D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                 "options and commands of different parsers cannot be "
                 "linked");
}

/**
 * @brief Checks that a command may be linked to another.
 *
 * @param[in] _command   the command.
 * @param[in] _other     the other command.
 * @param[in] _relation  the link's word in CLI11's message.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_NOT_FOUND for no command or the command
 *         itself, or D_ARG_ERROR_INCORRECT_CONSTRUCTION for another parser's,
 *         each recorded as a definition error.
 *
 * A command another may be linked to: CLI11's refusals of nothing and of
 * the command itself, worded for `_relation`, then the refusal of another
 * parser's command, each recorded.
 */
D_STATIC int
d_arg_internal_command_check_command(
    const struct d_arg_command* _command,
    const struct d_arg_command* _other,
    const char*                 _relation
)
{
    // CLI11's refusal of nothing
    if (!_other)
    {
        return d_arg_internal_fail_definition(_command,
                                              NULL,
                                              D_ARG_ERROR_OPTION_NOT_FOUND,
                                              "nullptr passed not found");
    }

    // and of the command itself
    if (_other == _command)
    {
        return d_arg_internal_fail_definition(
                   _command,
                   NULL,
                   D_ARG_ERROR_OPTION_NOT_FOUND,
                   "cannot self reference in %s not found",
                   _relation);
    }

    return (_other->parser == _command->parser)
           ? D_ARG_OK
           : d_arg_internal_fail_definition(
                 _command,
                 NULL,
                 D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                 "commands of different parsers cannot be linked");
}

/**
 * @brief Inserts an option into one of a command's lists, as into CLI11's set.
 *
 * @param[in,out] _command  the command the list belongs to.
 * @param[in,out] _list     the list.
 * @param[in]     _option   the option, kept, not owned.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded as a definition
 *         error, the list unchanged.
 *
 * An option inserted into one of a command's lists, as into CLI11's set; a
 * list that could not grow is recorded.
 */
D_STATIC int
d_arg_internal_command_keep_option(
    struct d_arg_command*     _command,
    struct d_arg_option_list* _list,
    struct d_arg_option*      _option
)
{
    const int status = d_arg_internal_option_insert(_list,
                                                    _option);

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail_definition(_command,
                                            NULL,
                                            status,
                                            NULL);
}

/*
d_arg_command_excludes_option
  CLI11's excludes(Option*).
*/
int
d_arg_command_excludes_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const int status = d_arg_internal_command_check_option(_command,
                                                           _option);

    return (status == D_ARG_OK)
           ? d_arg_internal_command_keep_option(_command,
                                                &_command->exclude_options,
                                                _option)
           : status;
}

/*
d_arg_command_needs_option
  CLI11's needs(Option*).
*/
int
d_arg_command_needs_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const int status = d_arg_internal_command_check_option(_command,
                                                           _option);

    return (status == D_ARG_OK)
           ? d_arg_internal_command_keep_option(_command,
                                                &_command->need_options,
                                                _option)
           : status;
}

/*
d_arg_command_excludes_command
  CLI11's excludes(App*): kept on both commands, the second half only when
the first is new, as CLI11's set insert reports; a second half that could
not be kept takes the first back.
*/
int
d_arg_command_excludes_command(
    struct d_arg_command* _command,
    struct d_arg_command* _other
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_internal_command_check_command(_command,
                                                      _other,
                                                      "excludes");

    // already excluded
    if ( (status != D_ARG_OK) ||
         (d_arg_internal_command_has(&_command->exclude_subcommands,
                                     _other)) )
    {
        return status;
    }

    status = d_arg_internal_command_push(&_command->exclude_subcommands,
                                         _other);

    // then the other half, or neither
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_insert(&_other->exclude_subcommands,
                                               _command);

        if (status != D_ARG_OK)
        {
            (void)d_arg_internal_command_remove(&_command->exclude_subcommands,
                                                _other);
        }
    }

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail_definition(_command,
                                            NULL,
                                            status,
                                            NULL);
}

/*
d_arg_command_needs_command
  CLI11's needs(App*).
*/
int
d_arg_command_needs_command(
    struct d_arg_command* _command,
    struct d_arg_command* _other
)
{
    // validate the parameters
    if (!_command)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_internal_command_check_command(_command,
                                                      _other,
                                                      "needs");

    // kept, as in CLI11's set
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_command_insert(&_command->need_subcommands,
                                               _other);
        status = (status == D_ARG_OK)
                 ? D_ARG_OK
                 : d_arg_internal_fail_definition(_command,
                                                  NULL,
                                                  status,
                                                  NULL);
    }

    return status;
}

/*
d_arg_command_remove_excludes_option
  CLI11's remove_excludes(Option*).
*/
bool
d_arg_command_remove_excludes_option(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    return ( (_command != NULL) &&
             (d_arg_internal_option_remove(&_command->exclude_options,
                                           _option)) );
}

/*
d_arg_command_remove_excludes_command
  CLI11's remove_excludes(App*): both halves.
*/
bool
d_arg_command_remove_excludes_command(
    struct d_arg_command* _command,
    struct d_arg_command* _other
)
{
    // only an exclusion that was there
    if ( (!_command) ||
         (!d_arg_internal_command_remove(&_command->exclude_subcommands,
                                         _other)) )
    {
        return false;
    }

    (void)d_arg_internal_command_remove(&_other->exclude_subcommands,
                                        _command);

    return true;
}

/*
d_arg_command_remove_needs_option
  CLI11's remove_needs(Option*).
*/
bool
d_arg_command_remove_needs_option(
    struct d_arg_command*      _command,
    const struct d_arg_option* _option
)
{
    return ( (_command != NULL) &&
             (d_arg_internal_option_remove(&_command->need_options,
                                           _option)) );
}

/*
d_arg_command_remove_needs_command
  CLI11's remove_needs(App*).
*/
bool
d_arg_command_remove_needs_command(
    struct d_arg_command*       _command,
    const struct d_arg_command* _other
)
{
    return ( (_command != NULL) &&
             (d_arg_internal_command_remove(&_command->need_subcommands,
                                            _other)) );
}

//==============================================================================
// 6.  FORMATTING AND OPTION DEFAULTS
//==============================================================================

/*
d_arg_command_set_formatter
  CLI11's formatter(), borrowed.
*/
void
d_arg_command_set_formatter(
    struct d_arg_command*   _command,
    struct d_arg_formatter* _formatter
)
{
    // nothing to set without a command
    if (_command != NULL)
    {
        _command->formatter = _formatter;
    }

    return;
}

/*
d_arg_command_set_config_format
  CLI11's config_formatter(), borrowed.
*/
void
d_arg_command_set_config_format(
    struct d_arg_command*       _command,
    struct d_arg_config_format* _format
)
{
    // nothing to set without a command
    if (_command != NULL)
    {
        _command->config_format = _format;
    }

    return;
}

/*
d_arg_command_option_defaults
  CLI11's option_defaults().
*/
struct d_arg_option_defaults*
d_arg_command_option_defaults(
    struct d_arg_command* _command
)
{
    return (_command != NULL)
           ? &_command->option_defaults
           : NULL;
}

/*
d_arg_option_defaults_set_group
  CLI11's OptionDefaults::group(): a copy, NULL reading as "OPTIONS"; the
old group freed only once the copy exists.
*/
int
d_arg_option_defaults_set_group(
    struct d_arg_option_defaults* _defaults,
    const char*                   _group
)
{
    // validate the parameters
    if (!_defaults)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    char* const copy = (_group != NULL)
                       ? d_arg_internal_copy_cstr(_group)
                       : NULL;

    // check if memory allocation was successful
    if ( (_group != NULL) &&
         (!copy) )
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    free(_defaults->group);
    _defaults->group = copy;

    return D_ARG_OK;
}

/*
d_arg_option_defaults_set_flag
  One of the six switches OptionDefaults carries; any other flag is
ignored.
*/
void
d_arg_option_defaults_set_flag(
    struct d_arg_option_defaults* _defaults,
    uint32_t                      _flag,
    bool                          _value
)
{
    const uint32_t carried = D_ARG_OPTION_REQUIRED               |
                             D_ARG_OPTION_IGNORE_CASE            |
                             D_ARG_OPTION_IGNORE_UNDERSCORE      |
                             D_ARG_OPTION_CONFIGURABLE           |
                             D_ARG_OPTION_DISABLE_FLAG_OVERRIDE  |
                             D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT;

    // nothing to set without defaults, nor for a flag they do not carry
    if ( (!_defaults) ||
         ((_flag & ~carried) != 0u) )
    {
        return;
    }

    _defaults->flags = _value
                       ? (_defaults->flags | _flag)
                       : (_defaults->flags & ~_flag);

    return;
}

/*
d_arg_option_defaults_set_multi_option_policy
  OptionDefaults::multi_option_policy(); a value outside the enum is
ignored.
*/
void
d_arg_option_defaults_set_multi_option_policy(
    struct d_arg_option_defaults* _defaults,
    int                           _policy
)
{
    // nothing to set without defaults, nor outside the enum
    if ( (_defaults != NULL) &&
         (d_arg_internal_in_range(_policy,
                                  (int)D_ARG_MULTI_THROW,
                                  (int)D_ARG_MULTI_REVERSE)) )
    {
        _defaults->multi_option_policy = _policy;
    }

    return;
}

/*
d_arg_option_defaults_set_callback_priority
  OptionDefaults::callback_priority(); a value outside the enum is ignored.
*/
void
d_arg_option_defaults_set_callback_priority(
    struct d_arg_option_defaults* _defaults,
    int                           _priority
)
{
    // nothing to set without defaults, nor outside the enum
    if ( (_defaults != NULL) &&
         (d_arg_internal_in_range(_priority,
                                  (int)D_ARG_PRIORITY_FIRST_PRE_HELP,
                                  (int)D_ARG_PRIORITY_LAST)) )
    {
        _defaults->callback_priority = _priority;
    }

    return;
}

/*
d_arg_option_defaults_set_integer_rule
  The integer rule new options convert by; a value outside the enum is
ignored.
*/
void
d_arg_option_defaults_set_integer_rule(
    struct d_arg_option_defaults* _defaults,
    int                           _rule
)
{
    // nothing to set without defaults, nor outside the enum
    if ( (_defaults != NULL) &&
         (d_arg_internal_in_range(_rule,
                                  (int)D_ARG_INTEGER_DECIMAL,
                                  (int)D_ARG_INTEGER_C)) )
    {
        _defaults->integer_rule = _rule;
    }

    return;
}

/*
d_arg_option_defaults_set_delimiter
  OptionDefaults::delimiter(); NUL for none.
*/
void
d_arg_option_defaults_set_delimiter(
    struct d_arg_option_defaults* _defaults,
    char                          _delimiter
)
{
    // nothing to set without defaults
    if (_defaults != NULL)
    {
        _defaults->delimiter = _delimiter;
    }

    return;
}
