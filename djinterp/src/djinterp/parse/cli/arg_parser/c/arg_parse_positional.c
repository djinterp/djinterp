/*******************************************************************************
* djinterp [parse]                                        arg_parse_positional.c
*
* The positional step of the parse, declared in arg_internal.h (8.1.7).
*   CLI11's _parse_positional: an argument recognized as nothing else is
* offered to the command's positional options -- the required ones first,
* when positionals come at the end and only their arguments are left -- and
* placed in the first that takes it, as a flag's value for one that expects
* none; else it goes to an option group, to the fallthrough parent, to a
* subcommand of the command's met again, or to the parents, or is kept as
* an unused argument.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_parse_positional.c
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
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // d_arg_config_item
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser


//==============================================================================
// 1.  CHOOSING
//==============================================================================

/**
 * @brief Tells whether a positional takes another value, as CLI11 asks.
 *
 * @param[in] _option    the option.
 * @param[in] _required  whether only required positionals short of their
 *                       minimums count.
 * @return `true` for a positional open to another value as asked; `false`
 *         otherwise.
 *
 * Whether a positional takes another value, as CLI11 asks: the required
 * ones still short of their minimums, when `_required`; otherwise any with
 * room left, or taking extra values.
 */
D_STATIC bool
d_arg_internal_parse_open(
    const struct d_arg_option* _option,
    bool                       _required
)
{
    const int count = (int)d_arg_option_count(_option);

    // a required positional short of its minimum
    if (_required)
    {
        return ( (d_arg_option_is_positional(_option))               &&
                 (d_arg_option_has_flag(_option,
                                        D_ARG_OPTION_REQUIRED))      &&
                 (count < d_arg_option_items_expected_min(_option)) );
    }

    return ( (d_arg_option_is_positional(_option))                 &&
             ( (count < d_arg_option_items_expected_max(_option)) ||
               (d_arg_option_has_flag(_option,
                                      D_ARG_OPTION_ALLOW_EXTRA_ARGS)) ) );
}

/**
 * @brief Finds the first of a command's options open to a positional that
 *        accepts it.
 *
 * @param[in]  _command     the command.
 * @param[in]  _positional  the positional argument.
 * @param[in]  _required    whether only required positionals short of their
 *                          minimums are open.
 * @param[out] _option      receives the option; left as it was when none is
 *                          found, NULL when a validator stopped the parse.
 * @return D_ARG_OK, or the error a validator stopped with, recorded.
 *
 * The first of the command's options open to a positional, as
 * d_arg_internal_parse_open asks it, whose validators -- when the command
 * validates positionals -- accept it; `_option` is left as it was when none
 * is.
 */
D_STATIC int
d_arg_internal_parse_scan(
    const struct d_arg_command* _command,
    struct d_pack_text          _positional,
    bool                        _required,
    struct d_arg_option**       _option
)
{
    const bool validate = d_arg_command_has_flag(
                              _command,
                              D_ARG_COMMAND_VALIDATE_POSITIONALS);

    // each open option in turn
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        struct d_arg_option* const option   = _command->options.items[index];
        bool                       accepted = true;
        int                        status   = D_ARG_OK;

        if (d_arg_internal_parse_open(option,
                                      _required))
        {
            status = (validate)
                     ? d_arg_internal_parse_judge(_command,
                                                  option,
                                                  _positional,
                                                  &accepted)
                     : D_ARG_OK;

            // a validator stopped the parse, or this one takes it
            if ( (status != D_ARG_OK) ||
                 (accepted) )
            {
                *_option = (status == D_ARG_OK) ? option : NULL;

                return status;
            }
        }
    }

    return D_ARG_OK;
}

/**
 * @brief Chooses the option to take the positional on top of the stack, as
 *        CLI11 does.
 *
 * @param[in]  _command  the command.
 * @param[out] _option   receives the option; NULL when none takes it.
 * @return D_ARG_OK, or the error a validator stopped with, recorded.
 *
 * CLI11's choice of the positional option to take an argument: with
 * positionals at the end and no more arguments left than the required ones
 * need, a required one first; otherwise the first open one.
 */
D_STATIC int
d_arg_internal_parse_choose(
    const struct d_arg_command* _command,
    struct d_arg_option**       _option
)
{
    const struct d_pack_text positional = d_arg_internal_parse_back(_command);
    int                      status     = D_ARG_OK;

    *_option = NULL;

    // the required ones first, when the arguments left are theirs
    if ( (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_POSITIONALS_AT_END)) &&
         (d_arg_internal_parse_left(_command) <=
          d_arg_internal_parse_remaining(_command,
                                         true)) )
    {
        status = d_arg_internal_parse_scan(_command,
                                           positional,
                                           true,
                                           _option);
    }

    return ( (status != D_ARG_OK) ||
             (*_option != NULL) )
           ? status
           : d_arg_internal_parse_scan(_command,
                                       positional,
                                       false,
                                       _option);
}

//==============================================================================
// 2.  PLACING
//==============================================================================

/**
 * @brief Gives a positional that expects no values its argument, as CLI11 gives
 *        it.
 *
 * @param[in]     _command     the command whose parse reads the argument.
 * @param[in,out] _option      the option.
 * @param[in]     _positional  the argument.
 * @return D_ARG_OK, or the error CLI11 throws, recorded.
 *
 * A positional that expects no values given one, as CLI11 gives it: as a
 * configuration item named for the positional and holding the argument, both
 * borrowed for the call, so that the item owns no copies.
 */
D_STATIC int
d_arg_internal_parse_flag_like(
    struct d_arg_command* _command,
    struct d_arg_option*  _option,
    struct d_pack_text    _positional
)
{
    struct d_arg_config_item item    = D_ARG_CONFIG_ITEM_INIT;
    bool                     handled = false;
    int                      status  = d_arg_internal_text_push(&item.inputs,
                                                                _positional);

    item.name = d_arg_internal_text_of(_option->positional_name);

    // a single input, which CLI11 always takes as a flag's
    status = (status == D_ARG_OK)
             ? d_arg_internal_flag_like_result(_command,
                                               _option,
                                               &item,
                                               &item.inputs,
                                               &handled)
             : d_arg_internal_parse_stored(_command,
                                           status);
    d_arg_text_list_release(&item.inputs);

    return status;
}

/**
 * @brief Ends the positional step for an option that triggers on parse.
 *
 * @param[in,out] _option     the option.
 * @param[in]     _set_aside  the values set aside when it started afresh.
 * @return D_ARG_OK, or the failure of its callback or of a value added back,
 *         recorded.
 *
 * The end of CLI11's positional step for an option that triggers on parse:
 * its callback, now that it has values; else the values set aside before,
 * each added again, as CLI11's add_result(prev) adds them.
 */
D_STATIC int
d_arg_internal_parse_triggered(
    struct d_arg_option*          _option,
    const struct d_arg_text_list* _set_aside
)
{
    // values to call back with
    if (_option->results.count > 0u)
    {
        return d_arg_option_run_callback(_option);
    }

    int status = D_ARG_OK;

    // the values set aside, back again
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _set_aside->count);
         ++index)
    {
        status = d_arg_internal_parse_add(_option,
                                          _set_aside->items[index],
                                          NULL);
    }

    return status;
}

/**
 * @brief Places the positional on top of the stack in the option chosen for it.
 *
 * @param[in,out] _command  the command, whose parse order gains the option and
 *                          whose stack loses the argument.
 * @param[in,out] _option   the option.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * A positional placed in the option CLI11's _parse_positional chose: the
 * option in the parse order, a separator before the value where asked, a
 * trigger whose callback ran started afresh with its values set aside, the
 * value -- as a flag's, for an option that expects none -- then the
 * trigger's callback, or its values back.
 */
D_STATIC int
d_arg_internal_parse_place(
    struct d_arg_command* _command,
    struct d_arg_option*  _option
)
{
    const struct d_pack_text positional = d_arg_internal_parse_back(_command);
    const bool               trigger    = d_arg_option_has_flag(
                                              _option,
                                              D_ARG_OPTION_TRIGGER_ON_PARSE);
    struct d_arg_text_list   set_aside  = D_ARG_LIST_INIT;
    int                      status     = d_arg_internal_parse_stored(
                                              _command,
                                              d_arg_internal_option_push(
                                                  &_command->parse_order,
                                                  _option));

    status = (status == D_ARG_OK)
             ? d_arg_internal_parse_separate(_option)
             : status;

    // a trigger met again after its callback ran
    if ( (trigger) &&
         (_option->state == (int)D_ARG_OPTION_STATE_CALLBACK_RUN) )
    {
        d_arg_internal_parse_restart(_option,
                                     &set_aside);
    }

    // the value, as a flag's for an option expecting none
    if (status == D_ARG_OK)
    {
        status = (_option->expected_min == 0)
                 ? d_arg_internal_parse_flag_like(_command,
                                                  _option,
                                                  positional)
                 : d_arg_internal_parse_add(_option,
                                            positional,
                                            NULL);
    }

    status = ( (status == D_ARG_OK) &&
               (trigger) )
             ? d_arg_internal_parse_triggered(_option,
                                              &set_aside)
             : status;
    d_arg_text_list_release(&set_aside);
    d_arg_internal_parse_pop(_command);

    return status;
}

//==============================================================================
// 3.  WHAT IS LEFT
//==============================================================================

/**
 * @brief Offers the positional on top of the stack to a command's option
 *        groups.
 *
 * @param[in,out] _command  the command.
 * @param[out]    _taken    receives whether a group took it.
 * @return D_ARG_OK, or the error a step raised, recorded.
 *
 * CLI11's offer of a positional to the command's option groups, each
 * enabled one in turn, until one takes it.
 */
D_STATIC int
d_arg_internal_parse_group_positional(
    struct d_arg_command* _command,
    bool*                 _taken
)
{
    *_taken = false;

    // each enabled option group
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        struct d_arg_command* const group = _command->subcommands.items[index];

        if (d_arg_internal_parse_open_group(group))
        {
            const int status = d_arg_internal_parse_positional(group,
                                                               false,
                                                               _taken);

            // taken, or the parse stopped
            if ( (status != D_ARG_OK) ||
                 (*_taken) )
            {
                return (status == D_ARG_OK)
                       ? d_arg_internal_parse_entered(group)
                       : status;
            }
        }
    }

    return D_ARG_OK;
}

/**
 * @brief Tells whether an argument is left to the parents, as CLI11's last look
 *        in _parse_positional.
 *
 * @param[in] _command   the command, whose subcommands fall through.
 * @param[in] _argument  the argument.
 * @return `true` when a subcommand the fallthrough parent finds by the argument
 *         has a parent that can take another; `false` otherwise.
 *
 * CLI11's last look in _parse_positional, with subcommand fallthrough: a
 * subcommand the fallthrough parent -- or, at the root, the command itself --
 * finds by the argument, used before or not, whose own parent can take
 * another; it is left to the parents.
 */
D_STATIC bool
d_arg_internal_parse_parent_takes(
    const struct d_arg_command* _command,
    struct d_pack_text          _argument
)
{
    const struct d_arg_command* const base  =
        (_command->parent != NULL)
        ? d_arg_internal_fallthrough_parent(_command)
        : _command;
    const struct d_arg_command* const found =
        d_arg_internal_find_subcommand(base,
                                       _argument,
                                       true,
                                       false);

    return ( (found != NULL)                                  &&
             (found->parent != NULL)                          &&
             (d_arg_internal_parse_below_max(found->parent)) );
}

/**
 * @brief Keeps an argument nothing takes as unused.
 *
 * @param[in,out] _command   the command, whose stack loses the argument and
 *                           whose unused arguments gain it.
 * @param[in]     _argument  the argument.
 * @return D_ARG_OK, or a failure, recorded.
 *
 * A positional nothing takes, an unused argument; a prefix command keeps
 * everything after it too.
 */
D_STATIC int
d_arg_internal_parse_unused(
    struct d_arg_command* _command,
    struct d_pack_text    _argument
)
{
    d_arg_internal_parse_pop(_command);

    const int status = d_arg_internal_parse_to_missing(_command,
                                                       (int)D_ARG_CLASS_NONE,
                                                       _argument);

    return ( (status == D_ARG_OK) &&
             (_command->prefix_command != (int)D_ARG_PREFIX_OFF) )
           ? d_arg_internal_parse_rest_to_missing(_command)
           : status;
}

/**
 * @brief Deals with an argument no option and no subcommand took, as CLI11's
 *        _parse_positional does.
 *
 * @param[in,out] _command   the command.
 * @param[in]     _argument  the argument, on top of the stack.
 * @param[out]    _go_on     receives `false` when the argument is left to a
 *                           parent; untouched otherwise.
 * @return D_ARG_OK; D_ARG_ERROR_EXTRAS, recorded, when positionals at the end
 *         admit nothing more; or another failure, recorded.
 *
 * What CLI11's _parse_positional does with an argument no option and no
 * subcommand took: a parent's subcommand, with subcommand fallthrough, is
 * left to the parents; positionals at the end admit nothing more; an option
 * group leaves it to its parent; and anything else is unused.
 */
D_STATIC int
d_arg_internal_parse_nowhere(
    struct d_arg_command* _command,
    struct d_pack_text    _argument,
    bool*                 _go_on
)
{
    // a parent's subcommand
    if ( (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH)) &&
         (d_arg_internal_parse_parent_takes(_command,
                                            _argument)) )
    {
        *_go_on = false;

        return D_ARG_OK;
    }

    // positionals at the end admit nothing more
    if (d_arg_command_has_flag(_command,
                               D_ARG_COMMAND_POSITIONALS_AT_END))
    {
        return d_arg_internal_parse_extras_error(
                   _command,
                   _command->parser->pending.items,
                   NULL,
                   _command->parser->pending.count);
    }

    // an option group leaves it to its parent
    if ( (_command->parent != NULL)                &&
         (d_arg_command_name(_command)[0] == '\0') )
    {
        *_go_on = false;

        return D_ARG_OK;
    }

    return d_arg_internal_parse_unused(_command,
                                       _argument);
}

/**
 * @brief Ends CLI11's _parse_positional when no option took the argument.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _halt     whether a subcommand met again is left on the stack
 *                          for the caller.
 * @param[out]    _go_on    receives `false` when the argument is left on the
 *                          stack for the caller or a parent; untouched
 *                          otherwise.
 * @return D_ARG_OK, or the error a step raised, recorded.
 *
 * The end of CLI11's _parse_positional, when no option took the argument:
 * a subcommand of the command's met again, which the command can take once
 * more, is parsed again -- without being listed again -- unless the caller
 * halts at one; anything else goes where d_arg_internal_parse_nowhere sends
 * it.
 */
D_STATIC int
d_arg_internal_parse_leftover(
    struct d_arg_command* _command,
    bool                  _halt,
    bool*                 _go_on
)
{
    const struct d_pack_text    argument = d_arg_internal_parse_back(_command);
    struct d_arg_command* const again    = d_arg_internal_find_subcommand(
                                               _command,
                                               argument,
                                               true,
                                               false);

    // no subcommand met again
    if ( (!again)                                      ||
         (!d_arg_internal_parse_below_max(_command)) )
    {
        return d_arg_internal_parse_nowhere(_command,
                                            argument,
                                            _go_on);
    }

    // halted at it, for the caller
    if (_halt)
    {
        *_go_on = false;

        return D_ARG_OK;
    }

    d_arg_internal_parse_pop(_command);

    return d_arg_internal_parse_command(again);
}

/*
d_arg_internal_parse_positional
  CLI11's _parse_positional: the command's own positional options first,
then its option groups', then -- for a command that falls through -- its
fallthrough parent's, which halts at a subcommand when the command has a
parse-complete callback; then what is left.
*/
int
d_arg_internal_parse_positional(
    struct d_arg_command* _command,
    bool                  _halt,
    bool*                 _go_on
)
{
    struct d_arg_option* option = NULL;
    bool                 taken  = false;
    int                  status = d_arg_internal_parse_choose(_command,
                                                              &option);

    *_go_on = true;

    // an option of the command's takes it
    if ( (status != D_ARG_OK) ||
         (option != NULL) )
    {
        return (status == D_ARG_OK)
               ? d_arg_internal_parse_place(_command,
                                            option)
               : status;
    }

    status = d_arg_internal_parse_group_positional(_command,
                                                   &taken);

    // an option group's took it
    if ( (status != D_ARG_OK) ||
         (taken) )
    {
        return status;
    }

    // the fallthrough parent's
    if ( (_command->parent != NULL)                    &&
         (d_arg_command_has_flag(_command,
                                 D_ARG_COMMAND_FALLTHROUGH)) )
    {
        return d_arg_internal_parse_positional(
                   d_arg_internal_fallthrough_parent(_command),
                   (_command->parse_complete_fn != NULL),
                   _go_on);
    }

    return d_arg_internal_parse_leftover(_command,
                                         _halt,
                                         _go_on);
}
