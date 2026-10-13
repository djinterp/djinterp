/*******************************************************************************
* djinterp [parse]                                            arg_command_util.c
*
* CLI11's free functions over commands, declared in arg_command.h (section
* 7) and arg_internal.h (7.3.9).
*   TriggerOn and TriggerOff, whose pre-parse callback keeps its own list of
* targets so that a target removed later can leave it; deprecate_option and
* retire_option, whose warnings go where the parser writes help; and
* clear(), which forgets a parse.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_command_util.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, sinks, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator
#include "./arg_internal.h"  // failures, contexts, commands


//==============================================================================
// 1.  TRIGGERS
//==============================================================================

// d_arg_internal_command_trigger
//   struct: what a trigger's pre-parse callback keeps: its targets -- a list
// in the same allocation, after the record -- how many remain, and whether
// it enables them or disables them.
struct d_arg_internal_command_trigger
{
    struct d_arg_command** targets;
    size_t                 count;
    bool                   enable;
};

/*
d_arg_internal_command_trigger_fn
  The callback CLI11's TriggerOn and TriggerOff install: each target
enabled, or disabled, as the trigger is met.
*/
D_STATIC int
d_arg_internal_command_trigger_fn(
    struct d_arg_command* _command,
    size_t                _remaining,
    void*                 _context
)
{
    const struct d_arg_internal_command_trigger* const trigger =
        (const struct d_arg_internal_command_trigger*)_context;

    (void)_command;
    (void)_remaining;

    // each target, switched
    for (size_t index = 0u; index < trigger->count; ++index)
    {
        d_arg_command_set_disabled(trigger->targets[index],
                                   !trigger->enable);
    }

    return D_ARG_OK;
}

/*
d_arg_internal_command_check_targets
  The targets a trigger may take: none NULL, and every one of the
trigger's parser, another parser's being refused and recorded.
*/
D_STATIC int
d_arg_internal_command_check_targets(
    const struct d_arg_command*  _trigger,
    struct d_arg_command* const* _targets,
    size_t                       _count
)
{
    // a list must be given for targets to be read from it
    if ( (!_targets) &&
         (_count > 0u) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // each target
    for (size_t index = 0u; index < _count; ++index)
    {
        if (!_targets[index])
        {
            return D_ARG_ERROR_BAD_ARGUMENT;
        }

        if (_targets[index]->parser != _trigger->parser)
        {
            return d_arg_internal_fail_definition(
                       _trigger,
                       NULL,
                       D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                       "commands of different parsers cannot be linked");
        }
    }

    return D_ARG_OK;
}

/*
d_arg_internal_command_install_trigger
  CLI11's TriggerOn and TriggerOff: the targets checked and copied first,
so that a refusal changes nothing; then each target's startup -- disabled
for TriggerOn, enabled for TriggerOff -- and the trigger's pre-parse
callback, replacing any it had.
*/
D_STATIC int
d_arg_internal_command_install_trigger(
    struct d_arg_command*        _trigger,
    struct d_arg_command* const* _targets,
    size_t                       _count,
    bool                         _enable
)
{
    const int status = (_trigger != NULL)
                       ? d_arg_internal_command_check_targets(_trigger,
                                                              _targets,
                                                              _count)
                       : D_ARG_ERROR_BAD_ARGUMENT;

    // refused, and recorded where it should be
    if (status != D_ARG_OK)
    {
        return status;
    }

    const size_t room = ((size_t)-1 -
                         sizeof(struct d_arg_internal_command_trigger)) /
                        sizeof(struct d_arg_command*);
    struct d_arg_internal_command_trigger* const trigger =
        (_count <= room)
        ? (struct d_arg_internal_command_trigger*)malloc(
              sizeof(*trigger) + (_count * sizeof(*trigger->targets)))
        : NULL;

    // check if memory allocation was successful
    if (!trigger)
    {
        return d_arg_internal_fail_definition(_trigger,
                                              NULL,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    trigger->targets = (struct d_arg_command**)(void*)(trigger + 1);
    trigger->count   = _count;
    trigger->enable  = _enable;

    // the targets, each starting the way the trigger does not leave it
    for (size_t index = 0u; index < _count; ++index)
    {
        trigger->targets[index]  = _targets[index];
        _targets[index]->startup = _enable
                                   ? (int)D_ARG_STARTUP_DISABLED
                                   : (int)D_ARG_STARTUP_ENABLED;
    }

    d_arg_command_set_preparse_callback(_trigger,
                                        d_arg_internal_command_trigger_fn,
                                        trigger,
                                        free);

    return D_ARG_OK;
}

/*
d_arg_command_trigger_on
  CLI11's TriggerOn.
*/
int
d_arg_command_trigger_on(
    struct d_arg_command*        _trigger,
    struct d_arg_command* const* _targets,
    size_t                       _count
)
{
    return d_arg_internal_command_install_trigger(_trigger,
                                                  _targets,
                                                  _count,
                                                  true);
}

/*
d_arg_command_trigger_off
  CLI11's TriggerOff.
*/
int
d_arg_command_trigger_off(
    struct d_arg_command*        _trigger,
    struct d_arg_command* const* _targets,
    size_t                       _count
)
{
    return d_arg_internal_command_install_trigger(_trigger,
                                                  _targets,
                                                  _count,
                                                  false);
}

/*
d_arg_internal_command_untrigger
  A command about to be released leaves the target list of a trigger's
callback, the entries after it moving up; any other callback is left
alone.
*/
void
d_arg_internal_command_untrigger(
    struct d_arg_command*       _trigger,
    const struct d_arg_command* _gone
)
{
    struct d_arg_internal_command_trigger* trigger = NULL;
    size_t                                 kept    = 0u;

    // only a trigger's callback holds targets
    if (_trigger->preparse_fn != d_arg_internal_command_trigger_fn)
    {
        return;
    }

    trigger = (struct d_arg_internal_command_trigger*)
                  _trigger->preparse_context;

    // every target but the one going
    for (size_t index = 0u; index < trigger->count; ++index)
    {
        if (trigger->targets[index] != _gone)
        {
            trigger->targets[kept] = trigger->targets[index];
            kept++;
        }
    }

    trigger->count = kept;

    return;
}

//==============================================================================
// 2.  LIFE CYCLE OF OPTIONS
//==============================================================================

/*
d_arg_internal_command_warn
  Writes one warning about the option a validation runs for, where its
parser writes help; nothing without a parser.
*/
D_STATIC void
d_arg_internal_command_warn(
    const struct d_arg_option* _option,
    const char*                _before,
    const char*                _after
)
{
    struct d_arg_parser* const parser = d_arg_internal_parser_of(_option);
    struct d_arg_buffer        name   = D_ARG_BUFFER_INIT;

    // no parser, nowhere to write
    if (!parser)
    {
        return;
    }

    (void)d_arg_option_name(_option,
                            0u,
                            &name);
    (void)d_arg_sink_printf(parser->out,
                            "%s%s%s\n",
                            _before,
                            d_arg_buffer_text(&name).data,
                            _after);
    d_arg_buffer_release(&name);

    return;
}

/*
d_arg_internal_command_deprecated
  CLI11's deprecation warning: "<name> is deprecated please use
'<replacement>' instead", the value passing.
*/
D_STATIC int
d_arg_internal_command_deprecated(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_buffer after  = D_ARG_BUFFER_INIT;
    const int           status = d_arg_buffer_printf(
                                     &after,
                                     " is deprecated please use '%s' instead",
                                     (const char*)_validator->data);

    // the warning, when it could be built
    if (status == D_ARG_OK)
    {
        d_arg_internal_command_warn(_validation->option,
                                    "",
                                    d_arg_buffer_text(&after).data);
    }

    d_arg_buffer_release(&after);

    return D_ARG_OK;
}

/*
d_arg_internal_command_copy_context
  The deprecation validator's context, copied with it.
*/
D_STATIC void*
d_arg_internal_command_copy_context(
    const void* _context
)
{
    return d_arg_internal_copy_cstr((const char*)_context);
}

/*
d_arg_internal_command_warning
  A check made of a warning function, applying to the first value only, as
CLI11's application_index(0) makes it, with the context it owns; a check
that could not be made frees the context and is recorded.
*/
D_STATIC int
d_arg_internal_command_warning(
    struct d_arg_option* _option,
    d_arg_validate_fn    _warn,
    char*                _context,
    const char*          _description
)
{
    struct d_arg_validator warning = d_arg_validator_custom(_warn,
                                                            _context,
                                                            _description);

    // a validator that could not be made never took the context
    if (warning.status != D_ARG_OK)
    {
        free(_context);
        d_arg_validator_release(&warning);

        return d_arg_internal_fail_definition(NULL,
                                              _option,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    d_arg_validator_own_context(&warning,
                                (_context != NULL)
                                ? d_arg_internal_command_copy_context
                                : NULL,
                                (_context != NULL)
                                ? free
                                : NULL);
    d_arg_validator_set_application_index(&warning,
                                          0);

    return d_arg_option_check(_option,
                              warning,
                              NULL);
}

/*
d_arg_internal_command_deprecate_text
  The description CLI11 gives a deprecated option with a replacement: its
own, then " DEPRECATED: please use '<replacement>' instead".
*/
D_STATIC int
d_arg_internal_command_deprecate_text(
    struct d_arg_option* _option,
    const char*          _replacement
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_buffer_printf(
                                     &text,
                                     "%s DEPRECATED: please use '%s' instead",
                                     d_arg_option_description(_option),
                                     _replacement);

    // the new description, or the failure recorded
    status = (status == D_ARG_OK)
             ? d_arg_option_set_description(_option,
                                            d_arg_buffer_text(&text).data)
             : d_arg_internal_fail_definition(NULL,
                                              _option,
                                              status,
                                              NULL);
    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_command_deprecate_option
  CLI11's deprecate_option: a check described "DEPRECATED" that warns on
the first value, then, for a replacement, the description's note.
*/
int
d_arg_command_deprecate_option(
    struct d_arg_option* _option,
    const char*          _replacement
)
{
    const char* const replacement = (_replacement != NULL)
                                    ? _replacement
                                    : "";

    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    char* const context = d_arg_internal_copy_cstr(replacement);

    // check if memory allocation was successful
    if (!context)
    {
        return d_arg_internal_fail_definition(NULL,
                                              _option,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    const int status = d_arg_internal_command_warning(
                           _option,
                           d_arg_internal_command_deprecated,
                           context,
                           "DEPRECATED");

    return ( (status == D_ARG_OK) &&
             (replacement[0] != '\0') )
           ? d_arg_internal_command_deprecate_text(_option,
                                                   replacement)
           : status;
}

/*
d_arg_internal_command_retired
  CLI11's retirement warning: "WARNING <name> is retired and has no
effect", the value passing.
*/
D_STATIC int
d_arg_internal_command_retired(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    d_arg_internal_command_warn(_validation->option,
                                "WARNING ",
                                " is retired and has no effect");

    return D_ARG_OK;
}

/*
d_arg_internal_command_try_names
  The temporary App CLI11's retire_option adds the names to first: a root
of the same parser with CLI11's help flag, refusing what CLI11's would --
a flag's `{value}`, a non-standard short name, `-h` -- and recording it.
*/
D_STATIC bool
d_arg_internal_command_try_names(
    struct d_arg_command* _command,
    const char*           _names
)
{
    struct d_arg_command temporary = { 0 };
    bool                 named     = false;

    // a root of the same parser, with CLI11's help flag
    if ( (d_arg_internal_command_init(&temporary,
                                      _command->parser,
                                      NULL,
                                      NULL,
                                      NULL) == D_ARG_OK) &&
         (d_arg_command_set_help_flag(&temporary,
                                      "-h,--help",
                                      "Print this help message and exit")
          != NULL) )
    {
        named = (d_arg_command_add_option(&temporary,
                                          _names,
                                          NULL) != NULL);
    }

    d_arg_internal_command_release(&temporary);

    return named;
}

// d_arg_internal_command_shape
//   struct: what a retired option takes from the option it replaces: the
// item sizes, the count, and whether it takes extra arguments.
struct d_arg_internal_command_shape
{
    int  type_size_min;
    int  type_size_max;
    int  expected_min;
    int  expected_max;
    bool extra;
};

/*
d_arg_internal_command_retire_shape
  What CLI11's retire_option gives the retired option, in its order: the
type name and default "RETIRED", the old option's item sizes, count and
extra-argument switch, then the warning.
*/
D_STATIC int
d_arg_internal_command_retire_shape(
    struct d_arg_option*                       _retired,
    const struct d_arg_internal_command_shape* _shape
)
{
    int status = d_arg_option_set_type_name(_retired,
                                            "RETIRED");

    status = (status == D_ARG_OK)
             ? d_arg_option_set_default_text(_retired,
                                             "RETIRED")
             : status;

    // the old option's shape
    if (status == D_ARG_OK)
    {
        d_arg_option_set_type_size_range(_retired,
                                         _shape->type_size_min,
                                         _shape->type_size_max);
        d_arg_option_set_expected_range(_retired,
                                        _shape->expected_min,
                                        _shape->expected_max);
        d_arg_option_set_allow_extra_args(_retired,
                                          _shape->extra);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_command_warning(_retired,
                                            d_arg_internal_command_retired,
                                            NULL,
                                            NULL)
           : status;
}

/*
d_arg_command_retire_option
  CLI11's retire_option(app, opt): the names tried on a temporary command,
the shape kept, the option removed -- when the command owns it -- and one
taking the same values, doing nothing, added in its place.
*/
struct d_arg_option*
d_arg_command_retire_option(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    struct d_arg_buffer names = D_ARG_BUFFER_INIT;

    // validate the parameters
    if ( (!_command) ||
         (!_option) )
    {
        return NULL;
    }

    const struct d_arg_internal_command_shape shape =
    {
        _option->type_size_min,
        _option->type_size_max,
        _option->expected_min,
        _option->expected_max,
        d_arg_option_has_flag(_option,
                              D_ARG_OPTION_ALLOW_EXTRA_ARGS)
    };
    const int status = d_arg_option_name(_option,
                                         D_ARG_NAME_ALL,
                                         &names);

    // the names, written and tried
    if (status != D_ARG_OK)
    {
        (void)d_arg_internal_fail_definition(_command,
                                             NULL,
                                             status,
                                             NULL);
    }
    else if (d_arg_internal_command_try_names(_command,
                                              d_arg_buffer_text(&names).data))
    {
        struct d_arg_option* retired = NULL;

        (void)d_arg_command_remove_option(_command,
                                          _option);
        retired = d_arg_command_add_option(
                      _command,
                      d_arg_buffer_text(&names).data,
                      "option has been retired and has no effect");
        d_arg_buffer_release(&names);

        return ( (retired != NULL) &&
                 (d_arg_internal_command_retire_shape(retired,
                                                      &shape) == D_ARG_OK) )
               ? retired
               : NULL;
    }

    d_arg_buffer_release(&names);

    return NULL;
}

/*
d_arg_command_retire_option_name
  CLI11's retire_option(app, name): an option the name finds is retired;
otherwise a retired option is added, taking at most one value.
*/
struct d_arg_option*
d_arg_command_retire_option_name(
    struct d_arg_command* _command,
    const char*           _name
)
{
    // validate the parameters
    if ( (!_command) ||
         (!_name) )
    {
        return NULL;
    }

    struct d_arg_option* const found = d_arg_command_option(_command,
                                                            _name);

    // one there already
    if (found != NULL)
    {
        return d_arg_command_retire_option(_command,
                                           found);
    }

    struct d_arg_option* const retired = d_arg_command_add_option(
        _command,
        _name,
        "option has been retired and has no effect");

    // refused, recorded
    if (!retired)
    {
        return NULL;
    }

    int status = d_arg_option_set_type_name(retired,
                                            "RETIRED");

    // at most one value, the default "RETIRED", and the warning
    if (status == D_ARG_OK)
    {
        d_arg_option_set_expected_range(retired,
                                        0,
                                        1);
        status = d_arg_option_set_default_text(retired,
                                               "RETIRED");
    }

    return ( (status == D_ARG_OK) &&
             (d_arg_internal_command_warning(retired,
                                             d_arg_internal_command_retired,
                                             NULL,
                                             NULL) == D_ARG_OK) )
           ? retired
           : NULL;
}

//==============================================================================
// 3.  PARSE STATE
//==============================================================================

/*
d_arg_command_clear
  CLI11's clear(): the uses and the pre-parse mark forgotten, the lists of
the last parse emptied, every option cleared, then each subcommand in
turn.
*/
void
d_arg_command_clear(
    struct d_arg_command* _command
)
{
    // nothing to clear without a command
    if (!_command)
    {
        return;
    }

    _command->parsed                   = 0u;
    _command->flags                   &= ~D_ARG_COMMAND_PRE_PARSE_CALLED;
    _command->missing.count            = 0u;
    _command->parsed_subcommands.count = 0u;
    _command->parse_order.count        = 0u;

    // its options
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        d_arg_option_clear(_command->options.items[index]);
    }

    // its subcommands, in turn
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        d_arg_command_clear(_command->subcommands.items[index]);
    }

    return;
}
