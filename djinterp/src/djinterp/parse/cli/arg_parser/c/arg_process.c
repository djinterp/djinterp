/*******************************************************************************
* djinterp [parse]                                                 arg_process.c
*
* The processing that ends a parse, declared in arg_internal.h (8.2).
*   CLI11's _process and its parts: the callbacks of each priority with the
* help flags and the version flag among them, the environment, the
* requirements -- what a command excludes and needs, required options and
* subcommands, the counts of options and subcommands -- the unused
* arguments, the completion callbacks of a subcommand that calls back at
* once, and run_callback, which hands the parse to the commands' own
* callbacks. A failure of a configuration file waits, as in CLI11, until
* the normal callbacks have run, so that a failure they find comes first.
* Configuration is read in arg_process_config.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_process.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, errors
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_parse_bool


//==============================================================================
// 1.  CALLBACKS
//==============================================================================

/**
 * @brief Runs a command's callback, if it has one, watched.
 *
 * @param[in,out] _command   the command.
 * @param[in]     _function  the callback; NULL for none.
 * @param[in]     _context   its context.
 * @return D_ARG_OK, or the failure the callback returned, recorded.
 *
 * A command's callback, if it has one, watched, so that a failure it
 * returns without describing is recorded against the command.
 */
D_STATIC int
d_arg_internal_process_call(
    struct d_arg_command* _command,
    d_arg_command_fn      _function,
    void*                 _context
)
{
    // no callback, nothing to run
    if (!_function)
    {
        return D_ARG_OK;
    }

    struct d_arg_internal_watch watch = { NULL,
                                          D_ARG_ERROR_INIT };

    d_arg_internal_watch_begin(&watch,
                               _command->parser);

    const int status = _function(_command,
                                 _context);

    return d_arg_internal_watch_close(&watch,
                                      _command,
                                      status);
}

/**
 * @brief Runs the callbacks of the subcommands a command received and of its
 *        option groups used, as the middle of CLI11's run_callback.
 *
 * @param[in,out] _command         the command.
 * @param[in]     _suppress_final  whether the final callbacks are suppressed.
 * @return D_ARG_OK, or the failure of a callback, recorded.
 *
 * The middle of CLI11's run_callback: the callbacks of each subcommand the
 * command received -- once for each time it is listed, and only those it is
 * the parent of, an option group running its own -- then of each option
 * group that was used.
 */
D_STATIC int
d_arg_internal_process_run_subcommands(
    struct d_arg_command* _command,
    bool                  _suppress_final
)
{
    int status = D_ARG_OK;

    // each subcommand received, whose parent this is
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->parsed_subcommands.count);
         ++index)
    {
        struct d_arg_command* const sub =
            _command->parsed_subcommands.items[index];

        if (sub->parent == _command)
        {
            status = d_arg_internal_run_callback(sub,
                                                 true,
                                                 _suppress_final);
        }
    }

    // each option group that was used
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        struct d_arg_command* const group = _command->subcommands.items[index];

        if ( (d_arg_command_name(group)[0] == '\0')   &&
             (d_arg_command_count_all(group) > 0u) )
        {
            status = d_arg_internal_run_callback(group,
                                                 true,
                                                 _suppress_final);
        }
    }

    return status;
}

/*
d_arg_internal_run_callback
  CLI11's run_callback: the pre-callback; in the main parse, the
parse-complete callback, before anything the command received; the
subcommands' and option groups' callbacks; then, unless suppressed, the
final callback of a command that was used -- a named one, one given
anything, or the root.
*/
int
d_arg_internal_run_callback(
    struct d_arg_command* _command,
    bool                  _final_mode,
    bool                  _suppress_final
)
{
    int status = d_arg_internal_process_call(_command,
                                             _command->pre_callback_fn,
                                             _command->pre_callback_context);

    // the parse-complete callback, first in the main parse
    if ( (status == D_ARG_OK) &&
         (!_final_mode) )
    {
        status = d_arg_internal_process_call(
                     _command,
                     _command->parse_complete_fn,
                     _command->parse_complete_context);
    }

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_run_subcommands(_command,
                                                      _suppress_final)
             : status;

    // the final callback, of a command that was used
    if ( (status == D_ARG_OK)                           &&
         (_command->parsed > 0u)                        &&
         (!_suppress_final)                             &&
         ( (d_arg_command_name(_command)[0] != '\0') ||
           (d_arg_command_count_all(_command) > 0u)  ||
           (_command->parent == NULL) ) )
    {
        status = d_arg_internal_process_call(_command,
                                             _command->final_fn,
                                             _command->final_context);
    }

    return status;
}

//==============================================================================
// 2.  OPTION CALLBACKS
//==============================================================================

/**
 * @brief Runs the callbacks at a priority, as CLI11's _process_callbacks.
 *
 * @param[in,out] _command   the command.
 * @param[in]     _priority  the d_arg_callback_priority.
 * @return D_ARG_OK, or the failure of a callback or of a step, recorded.
 *
 * Declared ahead of its definition, which follows the steps it takes, one of
 * which -- an option group's turn -- calls it again.
 */
D_STATIC int
     d_arg_internal_process_callbacks(struct d_arg_command* _command,
                                      int                   _priority);

/**
 * @brief Runs the callbacks of a command's used option groups that have
 *        parse-complete callbacks, as the first part of CLI11's
 *        _process_callbacks.
 *
 * @param[in,out] _command   the command.
 * @param[in]     _priority  the d_arg_callback_priority.
 * @return D_ARG_OK, or the failure of a callback, recorded.
 *
 * The first part of CLI11's _process_callbacks: each option group with a
 * parse-complete callback that was used has its callbacks at the priority
 * run, and, at the normal priority, its own.
 */
D_STATIC int
d_arg_internal_process_groups_first(
    struct d_arg_command* _command,
    int                   _priority
)
{
    int status = D_ARG_OK;

    // each used option group with a parse-complete callback
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        struct d_arg_command* const group = _command->subcommands.items[index];

        if ( (d_arg_command_name(group)[0] == '\0')   &&
             (group->parse_complete_fn != NULL)       &&
             (d_arg_command_count_all(group) > 0u) )
        {
            status = d_arg_internal_process_callbacks(group,
                                                      _priority);
            status = ( (status == D_ARG_OK) &&
                       (_priority == (int)D_ARG_PRIORITY_NORMAL) )
                     ? d_arg_internal_run_callback(group,
                                                   false,
                                                   false)
                     : status;
        }
    }

    return status;
}

/**
 * @brief Asks for the version when the version flag was given at a priority, as
 *        CLI11's version trigger.
 *
 * @param[in] _command   the command.
 * @param[in] _priority  the d_arg_callback_priority.
 * @return D_ARG_OK when the flag does not ask; D_ARG_CALL_FOR_VERSION, recorded
 *         with the version as its message; or the failure, recorded.
 *
 * CLI11's version trigger in _process_callbacks: the version flag at the
 * priority, given and not yet called back, whose first value reads as true,
 * asks for the version, the text it gives the report's message.
 */
D_STATIC int
d_arg_internal_process_version(
    const struct d_arg_command* _command,
    int                         _priority
)
{
    const struct d_arg_option* const version = _command->version;
    bool                             trigger = false;

    // the flag given at the priority, and true
    if ( (!version)                                           ||
         (version->callback_priority != _priority)            ||
         (version->results.count == 0u)                       ||
         (d_arg_option_callback_run(version))                 ||
         (d_arg_parse_bool(version->results.items[0],
                           version->integer_rule,
                           &trigger) != D_ARG_OK)             ||
         (!trigger) )
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_command_version(
                                     _command,
                                     d_arg_buffer_sink(&text));

    // a buffer that could not grow refuses what is written to it: the
    // refusal is the memory's, not a file's
    status = (text.failed) ? D_ARG_ERROR_OUT_OF_MEMORY : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_fail_text(_command,
                                        version,
                                        D_ARG_CALL_FOR_VERSION,
                                        d_arg_buffer_text(&text))
             : d_arg_internal_fail(_command,
                                   version,
                                   status,
                                   NULL);
    d_arg_buffer_release(&text);

    return status;
}

/**
 * @brief Runs the callbacks of a command's options given at a priority, as
 *        CLI11's _process_callbacks does.
 *
 * @param[in,out] _command   the command.
 * @param[in]     _priority  the d_arg_callback_priority.
 * @return D_ARG_OK, or the failure of a callback, recorded.
 *
 * The options' part of CLI11's _process_callbacks: each option at the
 * priority that was given -- or is forced -- and has not been called back.
 */
D_STATIC int
d_arg_internal_process_options(
    struct d_arg_command* _command,
    int                   _priority
)
{
    int status = D_ARG_OK;

    // each option at the priority, given and not yet called back
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        struct d_arg_option* const option = _command->options.items[index];

        if ( (option->callback_priority == _priority)  &&
             (d_arg_option_is_given(option))           &&
             (!d_arg_option_callback_run(option)) )
        {
            status = d_arg_option_run_callback(option);
        }
    }

    return status;
}

/*
d_arg_internal_process_callbacks
  CLI11's _process_callbacks for a priority: the option groups with
parse-complete callbacks first, then the version flag, then the command's
options, then each subcommand without a parse-complete callback, used or
not -- one with such a callback runs its own completion callbacks as its
parse ends.
*/
D_STATIC int
d_arg_internal_process_callbacks(
    struct d_arg_command* _command,
    int                   _priority
)
{
    int status = d_arg_internal_process_groups_first(_command,
                                                     _priority);

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_version(_command,
                                              _priority)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_options(_command,
                                              _priority)
             : status;

    // each subcommand without a parse-complete callback
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        struct d_arg_command* const sub = _command->subcommands.items[index];

        if (!sub->parse_complete_fn)
        {
            status = d_arg_internal_process_callbacks(sub,
                                                      _priority);
        }
    }

    return status;
}

//==============================================================================
// 3.  HELP
//==============================================================================

/**
 * @brief Tells whether a help flag asks for help at a priority.
 *
 * @param[in] _flag      the flag; may be NULL.
 * @param[in] _priority  the d_arg_callback_priority.
 * @return `true` for a flag given that calls back at the priority; `false`
 *         otherwise.
 *
 * Whether a help flag asks for help at a priority: it exists, was given,
 * and calls back at that priority.
 */
D_STATIC bool
d_arg_internal_process_asks(
    const struct d_arg_option* _flag,
    int                        _priority
)
{
    return ( (_flag != NULL)                           &&
             (_flag->results.count > 0u)               &&
             (_flag->callback_priority == _priority) );
}

/**
 * @brief Asks for help when a help flag asks for it, as CLI11's
 *        _process_help_flags.
 *
 * @param[in] _command   the command.
 * @param[in] _priority  the d_arg_callback_priority.
 * @param[in] _help      whether a command above asked for help.
 * @param[in] _all       whether a command above asked for all help.
 * @return D_ARG_OK when none asks; D_ARG_CALL_FOR_HELP or
 *         D_ARG_CALL_FOR_ALL_HELP, recorded, when one does.
 *
 * CLI11's _process_help_flags for a priority: the command's help flags
 * given at the priority ask for help, or for all help, which wins; a command
 * that received subcommands passes the request down, each in turn deciding,
 * the first to ask winning, so the last command named answers.
 */
D_STATIC int
d_arg_internal_process_help(
    const struct d_arg_command* _command,
    int                         _priority,
    bool                        _help,
    bool                        _all
)
{
    const bool help   = ( (_help) ||
                          (d_arg_internal_process_asks(_command->help,
                                                       _priority)) );
    const bool all    = ( (_all) ||
                          (d_arg_internal_process_asks(_command->help_all,
                                                       _priority)) );
    int        status = D_ARG_OK;

    // the subcommands received decide
    if (_command->parsed_subcommands.count > 0u)
    {
        for (size_t index = 0u;
             (status == D_ARG_OK) &&
             (index < _command->parsed_subcommands.count);
             ++index)
        {
            status = d_arg_internal_process_help(
                         _command->parsed_subcommands.items[index],
                         _priority,
                         help,
                         all);
        }

        return status;
    }

    // all help, or help
    if ( (all) ||
         (help) )
    {
        return d_arg_internal_fail(_command,
                                   NULL,
                                   (all)
                                   ? D_ARG_CALL_FOR_ALL_HELP
                                   : D_ARG_CALL_FOR_HELP,
                                   NULL);
    }

    return D_ARG_OK;
}

//==============================================================================
// 4.  THE ENVIRONMENT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

/**
 * @brief Gives an option not given the value of its environment variable, as
 *        CLI11's step of _process_env.
 *
 * @param[in,out] _command  the command.
 * @param[in,out] _option   the option.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * CLI11's step of _process_env for one option: one not given whose
 * environment variable holds a value takes it, as given, unless its
 * validators reject it -- when it is passed over without a word. A validator
 * that stops with another error ends the parse with it.
 */
D_STATIC int
d_arg_internal_process_env_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    // given, or without a variable
    if ( (_option->results.count > 0u)  ||
         (!_option->env_name)           ||
         (_option->env_name[0] == '\0') )
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer value  = D_ARG_BUFFER_INIT;
    bool                valid  = false;
    int                 status = d_arg_internal_parse_stored(
                                     _command,
                                     d_arg_internal_getenv(_option->env_name,
                                                           &value));

    // a value the validators accept
    if ( (status == D_ARG_OK) &&
         (value.length > 0u) )
    {
        status = d_arg_internal_parse_judge(_command,
                                            _option,
                                            d_arg_buffer_text(&value),
                                            &valid);
        status = ( (status == D_ARG_OK) &&
                   (valid) )
                 ? d_arg_option_add_result(_option,
                                           d_arg_buffer_text(&value),
                                           NULL)
                 : status;
    }

    d_arg_buffer_release(&value);

    return status;
}

#else

/**
 * @brief Gives an option not given the value of its environment variable, as
 *        CLI11's step of _process_env.
 *
 * @param[in,out] _command  the command.
 * @param[in,out] _option   the option.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * Without the environment built in, nothing to read.
 */
D_STATIC int
d_arg_internal_process_env_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    (void)_command;
    (void)_option;

    return D_ARG_OK;
}

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

/**
 * @brief Reads the environment for a command's options, as CLI11's
 *        _process_env.
 *
 * @param[in,out] _command  the command.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * CLI11's _process_env: each option of the command, then, in turn, its
 * option groups and each subcommand used that has no parse-complete
 * callback, whose environment is read as its own parse ends.
 */
D_STATIC int
d_arg_internal_process_env(
    struct d_arg_command* _command
)
{
    int status = D_ARG_OK;

    // each option
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        status = d_arg_internal_process_env_option(
                     _command,
                     _command->options.items[index]);
    }

    // the option groups, and the subcommands used without such a callback
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        struct d_arg_command* const sub = _command->subcommands.items[index];

        if ( (d_arg_command_name(sub)[0] == '\0')   ||
             ( (d_arg_command_count_all(sub) > 0u) &&
               (!sub->parse_complete_fn) ) )
        {
            status = d_arg_internal_process_env(sub);
        }
    }

    return status;
}

//==============================================================================
// 5.  REQUIREMENTS
//==============================================================================

// d_arg_internal_process_named
//   struct: what a requirement's message names: an option, or, when it is
// NULL, a command -- or nothing, both NULL.
struct d_arg_internal_process_named
{
    const struct d_arg_command* command;
    const struct d_arg_option*  option;
};

/**
 * @brief Checks a command's requirements, as CLI11's _process_requirements.
 *
 * @param[in,out] _command  the command.
 * @return D_ARG_OK, or the requirement not met, recorded as CLI11's error.
 *
 * Declared ahead of its definition, which follows the steps it takes, the
 * last of which -- a subcommand's own -- calls it again.
 */
D_STATIC int
     d_arg_internal_process_requirements(struct d_arg_command* _command);

/**
 * @brief Writes the name of a record in a requirement's message, as CLI11 calls
 *        it.
 *
 * @param[in]     _named  the record: an option, a command, or neither.
 * @param[in,out] _out    the buffer that gains the name.
 * @return D_ARG_OK, or the buffer's status.
 *
 * What CLI11 calls a record in a requirement's message: an option's name, a
 * command's display name, or nothing.
 */
D_STATIC int
d_arg_internal_process_name(
    struct d_arg_internal_process_named _named,
    struct d_arg_buffer*                _out
)
{
    // an option
    if (_named.option != NULL)
    {
        return d_arg_option_name(_named.option,
                                 0u,
                                 _out);
    }

    return (_named.command != NULL)
           ? d_arg_command_display_name(_named.command,
                                        false,
                                        _out)
           : D_ARG_OK;
}

/**
 * @brief Records a requirement not met, as CLI11's RequiredError, RequiresError
 *        and ExcludesError.
 *
 * @param[in] _command  the command where it was checked.
 * @param[in] _status   the failure.
 * @param[in] _first    the record named first.
 * @param[in] _verb     the words after it.
 * @param[in] _second   the record named after them; neither for none.
 * @return `_status`, or D_ARG_ERROR_OUT_OF_MEMORY when the message could not be
 *         written; either recorded.
 *
 * A requirement not met, recorded against the command where it was checked
 * and the first option named: "<first> <verb>", then " <second>" when there
 * is a second -- CLI11's RequiredError, RequiresError and ExcludesError.
 */
D_STATIC int
d_arg_internal_process_refuse(
    const struct d_arg_command*         _command,
    int                                 _status,
    struct d_arg_internal_process_named _first,
    const char*                         _verb,
    struct d_arg_internal_process_named _second
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_process_name(_first,
                                                             &text);

    (void)d_arg_buffer_append_char(&text,
                                   ' ');
    (void)d_arg_buffer_append_cstr(&text,
                                   _verb);

    // the second record, after a space
    if ( (status == D_ARG_OK)          &&
         ( (_second.command != NULL) ||
           (_second.option != NULL) ) )
    {
        (void)d_arg_buffer_append_char(&text,
                                       ' ');
        status = d_arg_internal_process_name(_second,
                                             &text);
    }

    status = ( (status == D_ARG_OK) &&
               (text.failed == 0) )
             ? d_arg_internal_fail_text(_command,
                                        _first.option,
                                        _status,
                                        d_arg_buffer_text(&text))
             : d_arg_internal_fail(_command,
                                   _first.option,
                                   D_ARG_ERROR_OUT_OF_MEMORY,
                                   NULL);
    d_arg_buffer_release(&text);

    return status;
}

/**
 * @brief Checks what a command excludes, as CLI11's first check in
 *        _process_requirements.
 *
 * @param[in]  _command   the command.
 * @param[out] _excluded  receives whether something it excludes was used.
 * @return D_ARG_OK; D_ARG_ERROR_EXCLUDES, recorded, for an excluded command
 *         that was used; or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 *
 * CLI11's first check in _process_requirements: of the options and the
 * subcommands the command excludes, the last used names what excludes it, a
 * subcommand before any option. A command so excluded is refused when it was
 * used itself -- CLI11's ExcludesError -- and otherwise spared every other
 * check, `_excluded` telling the caller.
 */
D_STATIC int
d_arg_internal_process_excluded(
    const struct d_arg_command* _command,
    bool*                       _excluded
)
{
    const struct d_arg_internal_process_named self = { _command,
                                                       NULL };
    struct d_arg_internal_process_named       by   = { NULL,
                                                       NULL };

    // the last excluding option given
    for (size_t index = 0u; index < _command->exclude_options.count; ++index)
    {
        if (d_arg_option_count(_command->exclude_options.items[index]) > 0u)
        {
            by.option = _command->exclude_options.items[index];
        }
    }

    // the last excluding subcommand used, named over any option
    for (size_t index = 0u;
         index < _command->exclude_subcommands.count;
         ++index)
    {
        if (d_arg_command_count_all(
                _command->exclude_subcommands.items[index]) > 0u)
        {
            by.option  = NULL;
            by.command = _command->exclude_subcommands.items[index];
        }
    }

    *_excluded = ( (by.option != NULL) ||
                   (by.command != NULL) );

    return ( (*_excluded) &&
             (d_arg_command_count_all(_command) > 0u) )
           ? d_arg_internal_process_refuse(_command,
                                           D_ARG_ERROR_EXCLUDES,
                                           self,
                                           "excludes",
                                           by)
           : D_ARG_OK;
}

/**
 * @brief Checks what a command needs, as CLI11's second check in
 *        _process_requirements.
 *
 * @param[in]  _command  the command.
 * @param[out] _missing  receives whether something it needs was not used.
 * @return D_ARG_OK; D_ARG_ERROR_REQUIRES, recorded, for a command used without
 *         what it needs; or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 *
 * CLI11's second check: of the options and the subcommands the command
 * needs, the last not used names what it misses, a subcommand before any
 * option. A command so missing what it needs is refused when it was used
 * itself -- CLI11's RequiresError -- and otherwise spared every other check.
 */
D_STATIC int
d_arg_internal_process_needed(
    const struct d_arg_command* _command,
    bool*                       _missing
)
{
    const struct d_arg_internal_process_named self = { _command,
                                                       NULL };
    struct d_arg_internal_process_named       need = { NULL,
                                                       NULL };

    // the last needed option not given
    for (size_t index = 0u; index < _command->need_options.count; ++index)
    {
        if (d_arg_option_count(_command->need_options.items[index]) == 0u)
        {
            need.option = _command->need_options.items[index];
        }
    }

    // the last needed subcommand not used, named over any option
    for (size_t index = 0u; index < _command->need_subcommands.count; ++index)
    {
        if (d_arg_command_count_all(
                _command->need_subcommands.items[index]) == 0u)
        {
            need.option  = NULL;
            need.command = _command->need_subcommands.items[index];
        }
    }

    *_missing = ( (need.option != NULL) ||
                  (need.command != NULL) );

    return ( (*_missing) &&
             (d_arg_command_count_all(_command) > 0u) )
           ? d_arg_internal_process_refuse(_command,
                                           D_ARG_ERROR_REQUIRES,
                                           self,
                                           "requires",
                                           need)
           : D_ARG_OK;
}

/**
 * @brief Checks a given option's needs and excludes, as CLI11 does.
 *
 * @param[in] _command  the command where it is checked.
 * @param[in] _option   the option.
 * @return D_ARG_OK; D_ARG_ERROR_REQUIRES or D_ARG_ERROR_EXCLUDES, recorded; or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 *
 * CLI11's checks of a given option's links: the first option it needs that
 * was not given, then the first it excludes that was, refused.
 */
D_STATIC int
d_arg_internal_process_links(
    const struct d_arg_command* _command,
    const struct d_arg_option*  _option
)
{
    const struct d_arg_internal_process_named self = { NULL,
                                                       _option };

    // what it needs
    for (size_t index = 0u; index < _option->needs.count; ++index)
    {
        const struct d_arg_internal_process_named need =
            { NULL,
              _option->needs.items[index] };

        if (d_arg_option_count(need.option) == 0u)
        {
            return d_arg_internal_process_refuse(_command,
                                                 D_ARG_ERROR_REQUIRES,
                                                 self,
                                                 "requires",
                                                 need);
        }
    }

    // what it excludes
    for (size_t index = 0u; index < _option->excludes.count; ++index)
    {
        const struct d_arg_internal_process_named other =
            { NULL,
              _option->excludes.items[index] };

        if (d_arg_option_count(other.option) != 0u)
        {
            return d_arg_internal_process_refuse(_command,
                                                 D_ARG_ERROR_EXCLUDES,
                                                 self,
                                                 "excludes",
                                                 other);
        }
    }

    return D_ARG_OK;
}

/**
 * @brief Checks each of a command's options in turn, as CLI11 does.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _used     the count of options given, increased.
 * @return D_ARG_OK; D_ARG_ERROR_REQUIRED, D_ARG_ERROR_REQUIRES or
 *         D_ARG_ERROR_EXCLUDES, recorded; or D_ARG_ERROR_OUT_OF_MEMORY,
 *         recorded.
 *
 * CLI11's checks of each option in turn: a required one not given, then a
 * given one's links; the options given counted in `_used`.
 */
D_STATIC int
d_arg_internal_process_option_rules(
    const struct d_arg_command* _command,
    size_t*                     _used
)
{
    const struct d_arg_internal_process_named none   = { NULL,
                                                         NULL };
    int                                       status = D_ARG_OK;

    // each option in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        const struct d_arg_internal_process_named self  =
            { NULL,
              _command->options.items[index] };
        const bool                                given =
            (d_arg_option_count(self.option) > 0u);

        *_used += (given) ? 1u : 0u;

        // required and not given, or given with links
        if ( (!given) &&
             (d_arg_option_has_flag(self.option,
                                    D_ARG_OPTION_REQUIRED)) )
        {
            status = d_arg_internal_process_refuse(_command,
                                                   D_ARG_ERROR_REQUIRED,
                                                   self,
                                                   "is required",
                                                   none);
        }
        else if (given)
        {
            status = d_arg_internal_process_links(_command,
                                                  self.option);
        }
    }

    return status;
}

/**
 * @brief Checks the fewest subcommands a command needs, as CLI11 does.
 *
 * @param[in] _command  the command.
 * @return D_ARG_OK, or D_ARG_ERROR_REQUIRED, recorded.
 *
 * CLI11's check of the fewest subcommands a command needs: RequiredError's
 * "A subcommand is required", or its "Requires at least <n> subcommands".
 */
D_STATIC int
d_arg_internal_process_subcommand_minimum(
    const struct d_arg_command* _command
)
{
    const size_t minimum = _command->require_subcommand_min;

    // enough received
    if ( (minimum == 0u) ||
         (minimum <= _command->parsed_subcommands.count) )
    {
        return D_ARG_OK;
    }

    return (minimum == 1u)
           ? d_arg_internal_fail(_command,
                                 NULL,
                                 D_ARG_ERROR_REQUIRED,
                                 "A subcommand is required")
           : d_arg_internal_fail(_command,
                                 NULL,
                                 D_ARG_ERROR_REQUIRED,
                                 "Requires at least %zu subcommands",
                                 minimum);
}

/**
 * @brief Counts a command's enabled option groups that were used.
 *
 * @param[in] _command  the command.
 * @return the count.
 *
 * How many of the command's enabled option groups were used, each counting
 * as one option toward its option requirement.
 */
D_STATIC size_t
d_arg_internal_process_groups_used(
    const struct d_arg_command* _command
)
{
    size_t used = 0u;

    // each enabled option group used
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* const sub =
            _command->subcommands.items[index];

        if ( (d_arg_command_name(sub)[0] == '\0')               &&
             (!d_arg_command_has_flag(sub,
                                      D_ARG_COMMAND_DISABLED))  &&
             (d_arg_command_count_all(sub) > 0u) )
        {
            used++;
        }
    }

    return used;
}

/**
 * @brief Appends the comma CLI11's detail::join with a function writes before
 *        an element.
 *
 * @param[in,out] _out   the buffer that gains the comma.
 * @param[in,out] _mark  the place of the last comma, moved to the new one.
 * @return D_ARG_OK, or the buffer's status.
 *
 * The comma CLI11's detail::join with a function writes before an element:
 * only when something was written since the last comma's place, so that an
 * empty element at the start leaves nothing, and a later one two commas.
 */
D_STATIC int
d_arg_internal_process_join_step(
    struct d_arg_buffer* _out,
    size_t*              _mark
)
{
    const size_t here = _out->length;

    // nothing written since the last comma's place
    if (here <= *_mark)
    {
        return D_ARG_OK;
    }

    *_mark = here;

    return d_arg_buffer_append_char(_out,
                                    ',');
}

/**
 * @brief Appends the display names of a command's enabled option groups to the
 *        list for an option requirement.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _out      the buffer that gains them.
 * @return D_ARG_OK, or the buffer's status.
 *
 * The end of CLI11's list for an option requirement: a comma, then the
 * display names of the command's enabled option groups, joined as before --
 * nothing at all when it has none.
 */
D_STATIC int
d_arg_internal_process_group_list(
    const struct d_arg_command* _command,
    struct d_arg_buffer*        _out
)
{
    size_t mark   = 0u;
    bool   first  = true;
    int    status = D_ARG_OK;

    // each enabled option group
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const group =
            _command->subcommands.items[index];

        if ( (d_arg_command_name(group)[0] == '\0') &&
             (!d_arg_command_has_flag(group,
                                      D_ARG_COMMAND_DISABLED)) )
        {
            // the comma before the groups, once
            if (first)
            {
                status = d_arg_buffer_append_char(_out,
                                                  ',');
                mark   = _out->length;
                first  = false;
            }

            status = (status == D_ARG_OK)
                     ? d_arg_internal_process_join_step(_out,
                                                        &mark)
                     : status;
            status = (status == D_ARG_OK)
                     ? d_arg_command_display_name(group,
                                                  false,
                                                  _out)
                     : status;
        }
    }

    return status;
}

/**
 * @brief Writes CLI11's list for an option requirement.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _out      the buffer that gains the list.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's list for an option requirement: each option's names, the help
 * flags' as nothing, joined as its detail::join with a function joins them,
 * then the option groups.
 */
D_STATIC int
d_arg_internal_process_option_list(
    const struct d_arg_command* _command,
    struct d_arg_buffer*        _out
)
{
    size_t mark   = _out->length;
    int    status = D_ARG_OK;

    // each option's names, the help flags' empty
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        const struct d_arg_option* const option =
            _command->options.items[index];

        status = d_arg_internal_process_join_step(_out,
                                                  &mark);

        if ( (status == D_ARG_OK)              &&
             (option != _command->help)        &&
             (option != _command->help_all) )
        {
            status = d_arg_option_name(option,
                                       D_ARG_NAME_ALL,
                                       _out);
        }
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_process_group_list(_command,
                                               _out)
           : status;
}

/**
 * @brief Records CLI11's RequiredError::Option for a requirement other than
 *        exactly one option.
 *
 * @param[in] _command  the command.
 * @param[in] _used     how many options were used.
 * @param[in] _names    the list of the options.
 * @return D_ARG_ERROR_REQUIRED, recorded.
 *
 * The rest of CLI11's RequiredError::Option wordings, for a requirement
 * other than exactly one option: at least one when none was given, at least
 * n when too few were, at most one, or at most n.
 */
D_STATIC int
d_arg_internal_process_option_bound(
    const struct d_arg_command* _command,
    size_t                      _used,
    const char*                 _names
)
{
    const size_t min = _command->require_option_min;
    const size_t max = _command->require_option_max;

    // none given where one was needed
    if ( (min == 1u) &&
         (_used == 0u) )
    {
        return d_arg_internal_fail(_command,
                                   NULL,
                                   D_ARG_ERROR_REQUIRED,
                                   "At least 1 option from [%s] is required",
                                   _names);
    }

    // too few
    if (_used < min)
    {
        return d_arg_internal_fail(_command,
                                   NULL,
                                   D_ARG_ERROR_REQUIRED,
                                   "Requires at least %zu options used but "
                                   "only %zu were given from [%s]",
                                   min,
                                   _used,
                                   _names);
    }

    return (max == 1u)
           ? d_arg_internal_fail(_command,
                                 NULL,
                                 D_ARG_ERROR_REQUIRED,
                                 "Requires at most 1 options be given from "
                                 "[%s]",
                                 _names)
           : d_arg_internal_fail(_command,
                                 NULL,
                                 D_ARG_ERROR_REQUIRED,
                                 "Requires at most %zu options be used but "
                                 "%zu were given from [%s]",
                                 max,
                                 _used,
                                 _names);
}

/**
 * @brief Records CLI11's RequiredError::Option for an option requirement not
 *        met.
 *
 * @param[in] _command  the command.
 * @param[in] _used     how many options were used.
 * @return D_ARG_ERROR_REQUIRED, or D_ARG_ERROR_OUT_OF_MEMORY when the list
 *         could not be written; either recorded.
 *
 * CLI11's RequiredError::Option, in its words, for an option requirement
 * not met: exactly one, at least one, at least n, at most one, at most n.
 */
D_STATIC int
d_arg_internal_process_option_count(
    const struct d_arg_command* _command,
    size_t                      _used
)
{
    const size_t        min    = _command->require_option_min;
    const size_t        max    = _command->require_option_max;
    struct d_arg_buffer list   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_process_option_list(_command,
                                                                    &list);
    const char* const   names  = (list.data != NULL) ? list.data : "";

    // each of CLI11's wordings
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_parse_stored(_command,
                                             status);
    }
    else if ( (min == 1u) &&
              (max == 1u) )
    {
        status = (_used == 0u)
                 ? d_arg_internal_fail(_command,
                                       NULL,
                                       D_ARG_ERROR_REQUIRED,
                                       "Exactly 1 option from [%s] is "
                                       "required",
                                       names)
                 : d_arg_internal_fail(_command,
                                       NULL,
                                       D_ARG_ERROR_REQUIRED,
                                       "Exactly 1 option from [%s] is "
                                       "required but %zu were given",
                                       names,
                                       _used);
    }
    else
    {
        status = d_arg_internal_process_option_bound(_command,
                                                     _used,
                                                     names);
    }

    d_arg_buffer_release(&list);

    return status;
}

/**
 * @brief Checks a command's option requirement, as CLI11 does.
 *
 * @param[in] _command  the command.
 * @param[in] _used     how many options were used, each option group used
 *                      counting one.
 * @return D_ARG_OK; D_ARG_ERROR_REQUIRED, recorded, for a requirement not met;
 *         or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 *
 * CLI11's check of a command's option requirement, each option given and
 * each option group used counting one.
 */
D_STATIC int
d_arg_internal_process_option_minimum(
    const struct d_arg_command* _command,
    size_t                      _used
)
{
    const size_t min = _command->require_option_min;
    const size_t max = _command->require_option_max;

    return ( (min > _used)    ||
             ( (max > 0u) &&
               (max < _used) ) )
           ? d_arg_internal_process_option_count(_command,
                                                 _used)
           : D_ARG_OK;
}

/**
 * @brief Tells whether an option group is passed over in the requirements, as
 *        CLI11 passes one over.
 *
 * @param[in] _command  the command.
 * @param[in] _group    the subcommand.
 * @param[in] _used     how many options were used.
 * @return `true` for an option group not required and unused when the option
 *         requirement is met without it; `false` otherwise.
 *
 * CLI11's pass over an option group in the requirements: one not required
 * and unused, when the command's option requirement is met without it.
 */
D_STATIC bool
d_arg_internal_process_skipped(
    const struct d_arg_command* _command,
    const struct d_arg_command* _group,
    size_t                      _used
)
{
    const size_t min = _command->require_option_min;
    const size_t max = _command->require_option_max;

    return ( (d_arg_command_name(_group)[0] == '\0')               &&
             (!d_arg_command_has_flag(_group,
                                      D_ARG_COMMAND_REQUIRED))     &&
             (d_arg_command_count_all(_group) == 0u)               &&
             ( ( (min > 0u) &&
                 (min <= _used) ) ||
               ( (max > 0u) &&
                 (_used >= min) ) ) );
}

/**
 * @brief Checks the requirements of a command's subcommands, as the end of
 *        CLI11's _process_requirements.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _used     how many options were used.
 * @return D_ARG_OK, or the requirement not met, recorded.
 *
 * The end of CLI11's _process_requirements: each enabled subcommand not
 * passed over has its own requirements checked -- one used, or an option
 * group -- and a required one that was not used is refused.
 */
D_STATIC int
d_arg_internal_process_sub_requirements(
    struct d_arg_command* _command,
    size_t                _used
)
{
    const struct d_arg_internal_process_named none   = { NULL,
                                                         NULL };
    int                                       status = D_ARG_OK;

    // each enabled subcommand not passed over
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        struct d_arg_command* const               sub  =
            _command->subcommands.items[index];
        const struct d_arg_internal_process_named self = { sub,
                                                           NULL };

        if ( (!d_arg_command_has_flag(sub,
                                      D_ARG_COMMAND_DISABLED))      &&
             (!d_arg_internal_process_skipped(_command,
                                              sub,
                                              _used)) )
        {
            status = ( (sub->parsed > 0u) ||
                       (d_arg_command_name(sub)[0] == '\0') )
                     ? d_arg_internal_process_requirements(sub)
                     : D_ARG_OK;
            status = ( (status == D_ARG_OK)                        &&
                       (d_arg_command_has_flag(sub,
                                               D_ARG_COMMAND_REQUIRED)) &&
                       (d_arg_command_count_all(sub) == 0u) )
                     ? d_arg_internal_process_refuse(_command,
                                                     D_ARG_ERROR_REQUIRED,
                                                     self,
                                                     "is required",
                                                     none)
                     : status;
        }
    }

    return status;
}

/*
d_arg_internal_process_requirements
  CLI11's _process_requirements: a command excluded, or missing what it
needs, is refused if it was used and otherwise left unchecked; then its
options, its count of subcommands, its option requirement -- the option
groups used counting as options -- and its subcommands' requirements.
*/
D_STATIC int
d_arg_internal_process_requirements(
    struct d_arg_command* _command
)
{
    bool   stop   = false;
    size_t used   = 0u;
    int    status = d_arg_internal_process_excluded(_command,
                                                    &stop);

    status = ( (status == D_ARG_OK) &&
               (!stop) )
             ? d_arg_internal_process_needed(_command,
                                             &stop)
             : status;

    // refused, or spared the rest
    if ( (status != D_ARG_OK) ||
         (stop) )
    {
        return status;
    }

    status = d_arg_internal_process_option_rules(_command,
                                                 &used);
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_subcommand_minimum(_command)
             : status;
    used  += d_arg_internal_process_groups_used(_command);
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_option_minimum(_command,
                                                     used)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_process_sub_requirements(_command,
                                                     used)
           : status;
}

//==============================================================================
// 6.  PROCESSING
//==============================================================================

/**
 * @brief Takes one tier of CLI11's processing.
 *
 * @param[in,out] _command    the command.
 * @param[in]     _priority   the tier's d_arg_callback_priority.
 * @param[in]     _with_help  whether the help flags are processed at the tier.
 * @return D_ARG_OK, or the failure, recorded.
 *
 * One tier of CLI11's processing: the callbacks before help at the tier's
 * priority, the help flags where asked, then the callbacks after help.
 */
D_STATIC int
d_arg_internal_process_tier(
    struct d_arg_command* _command,
    int                   _priority,
    bool                  _with_help
)
{
    int status = d_arg_internal_process_callbacks(_command,
                                                  _priority - 1);

    status = ( (status == D_ARG_OK) &&
               (_with_help) )
             ? d_arg_internal_process_help(_command,
                                           _priority,
                                           false,
                                           false)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_process_callbacks(_command,
                                              _priority)
           : status;
}

/**
 * @brief Reads the configuration files, then the environment, as CLI11's
 *        _process does.
 *
 * @param[in,out] _command   the command.
 * @param[out]    _deferred  receives a file's failure, report and all, to be
 *                           raised once the normal callbacks have run;
 *                           untouched otherwise.
 * @return D_ARG_OK, or a failure other than a file's, recorded.
 *
 * The configuration files, then the environment, as CLI11's _process reads
 * them; a file's failure set aside in `_deferred`, report and all, to be
 * raised once the normal callbacks have run.
 */
D_STATIC int
d_arg_internal_process_sources(
    struct d_arg_command* _command,
    struct d_arg_error*   _deferred
)
{
    struct d_arg_parser* const parser = _command->parser;
    int                        status = d_arg_internal_process_config(
                                            _command);

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_env(_command)
             : status;

    // a file's failure waits
    if (status == D_ARG_ERROR_FILE)
    {
        *_deferred = parser->error;
        d_arg_error_init(&parser->error);
        status     = D_ARG_OK;
    }

    return status;
}

/**
 * @brief Raises a file's failure set aside, as CLI11 rethrows it.
 *
 * @param[in,out] _command   the command, whose parser's report takes the
 *                           failure.
 * @param[in,out] _deferred  the failure set aside, released or moved.
 * @param[in]     _status    the status the processing came to.
 * @return `_status` when it is a failure or nothing was set aside; otherwise
 *         the status of the failure raised.
 *
 * CLI11's rethrow of a file's failure set aside: raised now, unless another
 * failure came first, which stands, the one set aside dropped.
 */
D_STATIC int
d_arg_internal_process_rethrow(
    struct d_arg_command* _command,
    struct d_arg_error*   _deferred,
    int                   _status
)
{
    struct d_arg_parser* const parser = _command->parser;

    // nothing set aside, or another failure first
    if ( (_deferred->status == D_ARG_OK) ||
         (_status != D_ARG_OK) )
    {
        d_arg_error_release(_deferred);

        return _status;
    }

    d_arg_error_release(&parser->error);
    parser->error = *_deferred;
    d_arg_error_init(_deferred);

    return parser->error.status;
}

/*
d_arg_internal_process
  CLI11's _process for the root: the first tier, so that help comes before
everything; the configuration files and the environment; the tier before
the requirements; the requirements; the normal tier; a file's failure, if
one waits; the last tier.
*/
int
d_arg_internal_process(
    struct d_arg_command* _command
)
{
    struct d_arg_error deferred = D_ARG_ERROR_INIT;
    int                status   = d_arg_internal_process_tier(
                                      _command,
                                      (int)D_ARG_PRIORITY_FIRST,
                                      true);

    status = (status == D_ARG_OK)
             ? d_arg_internal_process_sources(_command,
                                              &deferred)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_tier(
                   _command,
                   (int)D_ARG_PRIORITY_PRE_REQUIREMENTS,
                   true)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_requirements(_command)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_tier(_command,
                                           (int)D_ARG_PRIORITY_NORMAL,
                                           true)
             : status;
    status = d_arg_internal_process_rethrow(_command,
                                            &deferred,
                                            status);

    return (status == D_ARG_OK)
           ? d_arg_internal_process_tier(_command,
                                         (int)D_ARG_PRIORITY_LAST,
                                         true)
           : status;
}

/*
d_arg_internal_process_extras
  CLI11's _process_extras: unused arguments are an error for a command in
the default extras mode, unless it is a prefix command -- or, under
SEPARATOR_ONLY, unless they start with `--`; then each subcommand used, in
turn.
*/
int
d_arg_internal_process_extras(
    struct d_arg_command* _command
)
{
    const int  prefix = _command->prefix_command;
    const bool unused = ( (_command->allow_extras ==
                           (int)D_ARG_EXTRAS_ERROR)                     &&
                          (d_arg_command_remaining_size(_command,
                                                        false) > 0u) );
    int        status = D_ARG_OK;

    // unused arguments not allowed
    if ( (unused)                                                     &&
         ( (prefix == (int)D_ARG_PREFIX_OFF) ||
           ( (prefix == (int)D_ARG_PREFIX_SEPARATOR_ONLY) &&
             (!d_arg_internal_equal_cstr(
                   _command->missing.items[0].text,
                   "--")) ) ) )
    {
        return d_arg_internal_parse_extras_error(_command,
                                                 NULL,
                                                 _command->missing.items,
                                                 _command->missing.count);
    }

    // each subcommand used
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        if (_command->subcommands.items[index]->parsed > 0u)
        {
            status = d_arg_internal_process_extras(
                         _command->subcommands.items[index]);
        }
    }

    return status;
}

/*
d_arg_internal_process_completion_callbacks
  CLI11's _process_completion_callbacks, for a subcommand with a
parse-complete callback as its parse ends -- or as its configuration
section closes, without the help flags or the environment: the four tiers
around the environment and the requirements, then its callbacks, its final
one held back while the parse goes on.
*/
int
d_arg_internal_process_completion_callbacks(
    struct d_arg_command* _command,
    bool                  _full
)
{
    int status = d_arg_internal_process_tier(_command,
                                             (int)D_ARG_PRIORITY_FIRST,
                                             _full);

    status = ( (status == D_ARG_OK) &&
               (_full) )
             ? d_arg_internal_process_env(_command)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_tier(
                   _command,
                   (int)D_ARG_PRIORITY_PRE_REQUIREMENTS,
                   _full)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_requirements(_command)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_tier(_command,
                                           (int)D_ARG_PRIORITY_NORMAL,
                                           _full)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_internal_process_tier(_command,
                                           (int)D_ARG_PRIORITY_LAST,
                                           _full)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_internal_run_callback(_command,
                                         false,
                                         _full)
           : status;
}
