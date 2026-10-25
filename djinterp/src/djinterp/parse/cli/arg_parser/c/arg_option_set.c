/*******************************************************************************
* djinterp [parse]                                              arg_option_set.c
*
* Options' settings, declared in arg_option.h (sections 4.1 to 4.5).
*   The text help shows, the counts of values and items, the multi-option
* policy and its neighbours, the switches, and the default -- each setter
* as CLI11's, with CLI11's construction errors recorded as the parser's
* definition error. The relations, validators and callbacks are in
* arg_option_link.c.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_option_set.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // strchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser_error
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_format
#include "./arg_internal.h"  // failures, spans, contexts
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


/*
d_arg_option_set_description
  A copy, or nothing for NULL.
*/
int
d_arg_option_set_description(
    struct d_arg_option* _option,
    const char*          _description
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_option_replace_text(_option,
                                              &_option->description,
                                              _description);
}

/*
d_arg_option_set_option_text
  A copy, or nothing for NULL.
*/
int
d_arg_option_set_option_text(
    struct d_arg_option* _option,
    const char*          _text
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_option_replace_text(_option,
                                              &_option->option_text,
                                              _text);
}

/*
d_arg_option_set_group
  CLI11's group(), which refuses a newline -- the NUL it also refuses
cannot occur in a C string -- with its IncorrectConstruction; NULL returns
to "OPTIONS".
*/
int
d_arg_option_set_group(
    struct d_arg_option* _option,
    const char*          _group
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a group name holds no newline
    if ( (_group != NULL) &&
         (strchr(_group,
                 '\n') != NULL) )
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                   "Group names may not contain newlines or null "
                   "characters");
    }

    return d_arg_internal_option_replace_text(_option,
                                              &_option->group,
                                              _group);
}

/*
d_arg_option_set_type_name
  CLI11's type_name(string) replaces the type-name function with one that
returns the string, so a fixed name also drops the function.
*/
int
d_arg_option_set_type_name(
    struct d_arg_option* _option,
    const char*          _type_name
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const int status = d_arg_internal_option_replace_text(_option,
                                                          &_option->type_name,
                                                          _type_name);

    // the fixed name replaces any function
    if (status == D_ARG_OK)
    {
        d_arg_option_set_type_name_fn(_option,
                                      NULL,
                                      NULL,
                                      NULL);
    }

    return status;
}

/*
d_arg_option_set_env_name
  A copy, or nothing for NULL.
*/
int
d_arg_option_set_env_name(
    struct d_arg_option* _option,
    const char*          _env_name
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_option_replace_text(_option,
                                              &_option->env_name,
                                              _env_name);
}

/*
d_arg_option_set_type_name_fn
  The old context is released before the new one is kept, and only when
the two differ, so setting the same context again does not free it; with
no option to keep it, the new context is freed at once.
*/
void
d_arg_option_set_type_name_fn(
    struct d_arg_option* _option,
    d_arg_option_text_fn _write,
    void*                _context,
    d_arg_release_fn     _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_option)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_context_replace(&_option->type_name_context,
                                   &_option->type_name_release,
                                   _context,
                                   _release);
    _option->type_name_fn = _write;

    return;
}

/*
d_arg_option_set_expected
  CLI11's expected(int), case for case.
*/
void
d_arg_option_set_expected(
    struct d_arg_option* _option,
    int                  _count
)
{
    // nothing to set without an option
    if (!_option)
    {
        return;
    }

    // a negative count: at least its magnitude, and extra arguments
    if (_count < 0)
    {
        _option->expected_min = -_count;

        // the maximum rises to the minimum
        if (_option->expected_max < _option->expected_min)
        {
            _option->expected_max = _option->expected_min;
        }

        _option->flags |= D_ARG_OPTION_ALLOW_EXTRA_ARGS;
        _option->flags &= ~D_ARG_OPTION_FLAG_LIKE;
    }
    // unlimited: at least one, and extra arguments
    else if (_count == (int)D_ARG_EXPECTED_UNLIMITED)
    {
        _option->expected_min = 1;
        _option->expected_max = (int)D_ARG_EXPECTED_UNLIMITED;
        _option->flags       |= D_ARG_OPTION_ALLOW_EXTRA_ARGS;
        _option->flags       &= ~D_ARG_OPTION_FLAG_LIKE;
    }
    // exactly that many; none makes the option a flag
    else
    {
        _option->expected_min = _count;
        _option->expected_max = _count;

        if (_count == 0)
        {
            _option->flags |= D_ARG_OPTION_FLAG_LIKE;
        }
        else
        {
            _option->flags &= ~D_ARG_OPTION_FLAG_LIKE;
        }
    }

    return;
}

/*
d_arg_option_set_expected_range
  CLI11's expected(min, max): magnitudes for the minimum, unlimited for a
negative maximum, and the pair put in order.
*/
void
d_arg_option_set_expected_range(
    struct d_arg_option* _option,
    int                  _minimum,
    int                  _maximum
)
{
    // nothing to set without an option
    if (!_option)
    {
        return;
    }

    const int minimum = (_minimum < 0) ? -_minimum : _minimum;
    const int maximum = (_maximum < 0)
                        ? (int)D_ARG_EXPECTED_UNLIMITED
                        : _maximum;

    _option->expected_min = (maximum < minimum) ? maximum : minimum;
    _option->expected_max = (maximum < minimum) ? minimum : maximum;

    return;
}

/*
d_arg_option_set_type_size
  CLI11's type_size(int): a negative size sets both bounds to its magnitude
and unlimited items, for compatibility; a size too large to bound is a
variable size, the minimum kept and separators injected; 0 clears required.
*/
void
d_arg_option_set_type_size(
    struct d_arg_option* _option,
    int                  _size
)
{
    // nothing to set without an option
    if (!_option)
    {
        return;
    }

    // the compatibility case
    if (_size < 0)
    {
        _option->type_size_max = -_size;
        _option->type_size_min = -_size;
        _option->expected_max  = (int)D_ARG_EXPECTED_UNLIMITED;

        return;
    }

    _option->type_size_max = _size;

    // a size within bounds is both bounds; an unbounded one injects
    // separators
    if (_size < (int)D_ARG_EXPECTED_UNLIMITED)
    {
        _option->type_size_min = _size;
    }
    else
    {
        _option->flags |= D_ARG_OPTION_INJECT_SEPARATOR;
    }

    // a type of no values cannot be required
    if (_size == 0)
    {
        _option->flags &= ~D_ARG_OPTION_REQUIRED;
    }

    return;
}

/*
d_arg_option_set_type_size_range
  CLI11's type_size(min, max): a negative bound means unlimited items and
its magnitude; the bounds put in order; 0 clears required, and an
unbounded maximum injects separators.
*/
void
d_arg_option_set_type_size_range(
    struct d_arg_option* _option,
    int                  _minimum,
    int                  _maximum
)
{
    // nothing to set without an option
    if (!_option)
    {
        return;
    }

    int minimum = _minimum;
    int maximum = _maximum;

    // the compatibility case
    if ( (minimum < 0) ||
         (maximum < 0) )
    {
        _option->expected_max = (int)D_ARG_EXPECTED_UNLIMITED;
        minimum               = (minimum < 0) ? -minimum : minimum;
        maximum               = (maximum < 0) ? -maximum : maximum;
    }

    _option->type_size_min = (minimum > maximum) ? maximum : minimum;
    _option->type_size_max = (minimum > maximum) ? minimum : maximum;

    // a type of no values cannot be required
    if (_option->type_size_max == 0)
    {
        _option->flags &= ~D_ARG_OPTION_REQUIRED;
    }

    // an unbounded size injects separators
    if (_option->type_size_max >= (int)D_ARG_EXPECTED_UNLIMITED)
    {
        _option->flags |= D_ARG_OPTION_INJECT_SEPARATOR;
    }

    return;
}

/*
d_arg_option_set_multi_option_policy
  CLI11's multi_option_policy(): only a change to a policy that exists does
anything, and leaving THROW on an option taking at least two of an
unlimited number limits it to its minimum, as CLI11 keeps for
compatibility.
*/
void
d_arg_option_set_multi_option_policy(
    struct d_arg_option* _option,
    int                  _policy
)
{
    // nothing to set without an option, nor to change to
    if ( (!_option)                                                 ||
         (_option->multi_option_policy == _policy)                  ||
         (!d_arg_internal_in_range(_policy,
                                   (int)D_ARG_MULTI_THROW,
                                   (int)D_ARG_MULTI_REVERSE)) )
    {
        return;
    }

    // the compatibility case
    if ( (_option->multi_option_policy == (int)D_ARG_MULTI_THROW)       &&
         (_option->expected_max == (int)D_ARG_EXPECTED_UNLIMITED)       &&
         (_option->expected_min > 1) )
    {
        _option->expected_max = _option->expected_min;
    }

    _option->multi_option_policy = _policy;
    _option->state               = (int)D_ARG_OPTION_STATE_PARSING;

    return;
}

/*
d_arg_option_set_delimiter
  The character itself; NUL for none.
*/
void
d_arg_option_set_delimiter(
    struct d_arg_option* _option,
    char                 _delimiter
)
{
    // nothing to set without an option
    if (_option != NULL)
    {
        _option->delimiter = _delimiter;
    }

    return;
}

/*
d_arg_option_set_callback_priority
  A d_arg_callback_priority value; any other is ignored.
*/
void
d_arg_option_set_callback_priority(
    struct d_arg_option* _option,
    int                  _priority
)
{
    // nothing to set without an option, nor outside the enum
    if ( (_option != NULL) &&
         (d_arg_internal_in_range(_priority,
                                  (int)D_ARG_PRIORITY_FIRST_PRE_HELP,
                                  (int)D_ARG_PRIORITY_LAST)) )
    {
        _option->callback_priority = _priority;
    }

    return;
}

/*
d_arg_option_set_integer_rule
  A d_arg_integer_rule value; any other is ignored.
*/
void
d_arg_option_set_integer_rule(
    struct d_arg_option* _option,
    int                  _rule
)
{
    // nothing to set without an option, nor outside the enum
    if ( (_option != NULL) &&
         (d_arg_internal_in_range(_rule,
                                  (int)D_ARG_INTEGER_DECIMAL,
                                  (int)D_ARG_INTEGER_C)) )
    {
        _option->integer_rule = _rule;
    }

    return;
}

/**
 * @brief Sets or clears one D_ARG_OPTION_* flag of an option.
 *
 * @param[in,out] _option  the option; NULL is ignored.
 * @param[in]     _flag    the flag.
 * @param[in]     _value   whether it is set.
 *
 * Sets or clears one D_ARG_OPTION_* flag.
 */
D_STATIC void
d_arg_internal_option_switch(
    struct d_arg_option* _option,
    uint32_t             _flag,
    bool                 _value
)
{
    // nothing to set without an option
    if (!_option)
    {
        return;
    }

    // set or clear the flag
    if (_value)
    {
        _option->flags |= _flag;
    }
    else
    {
        _option->flags &= ~_flag;
    }

    return;
}

/*
d_arg_option_set_required
  D_ARG_OPTION_REQUIRED alone; nothing else changes.
*/
void
d_arg_option_set_required(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_REQUIRED,
                                 _value);

    return;
}

/*
d_arg_option_set_configurable
  D_ARG_OPTION_CONFIGURABLE alone; nothing else changes.
*/
void
d_arg_option_set_configurable(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_CONFIGURABLE,
                                 _value);

    return;
}

/*
d_arg_option_set_disable_flag_override
  D_ARG_OPTION_DISABLE_FLAG_OVERRIDE alone; nothing else changes.
*/
void
d_arg_option_set_disable_flag_override(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_DISABLE_FLAG_OVERRIDE,
                                 _value);

    return;
}

/*
d_arg_option_set_always_capture_default
  D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT alone; nothing else changes.
*/
void
d_arg_option_set_always_capture_default(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT,
                                 _value);

    return;
}

/*
d_arg_option_set_allow_extra_args
  D_ARG_OPTION_ALLOW_EXTRA_ARGS alone; nothing else changes.
*/
void
d_arg_option_set_allow_extra_args(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_ALLOW_EXTRA_ARGS,
                                 _value);

    return;
}

/*
d_arg_option_set_trigger_on_parse
  D_ARG_OPTION_TRIGGER_ON_PARSE alone; nothing else changes.
*/
void
d_arg_option_set_trigger_on_parse(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_TRIGGER_ON_PARSE,
                                 _value);

    return;
}

/*
d_arg_option_set_force_callback
  D_ARG_OPTION_FORCE_CALLBACK alone; nothing else changes.
*/
void
d_arg_option_set_force_callback(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_FORCE_CALLBACK,
                                 _value);

    return;
}

/*
d_arg_option_set_run_callback_for_default
  D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT alone; nothing else changes.
*/
void
d_arg_option_set_run_callback_for_default(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT,
                                 _value);

    return;
}

/*
d_arg_option_set_inject_separator
  D_ARG_OPTION_INJECT_SEPARATOR alone; nothing else changes.
*/
void
d_arg_option_set_inject_separator(
    struct d_arg_option* _option,
    bool                 _value
)
{
    d_arg_internal_option_switch(_option,
                                 D_ARG_OPTION_INJECT_SEPARATOR,
                                 _value);

    return;
}

/**
 * @brief Turns an option's ignore-case or ignore-underscore switch on or off.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _flag    the switch: D_ARG_OPTION_IGNORE_CASE or
 *                         D_ARG_OPTION_IGNORE_UNDERSCORE.
 * @param[in]     _value   whether it is turned on.
 * @param[in]     _what    the switch's words in CLI11's message.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_ALREADY_ADDED, recorded as a definition
 *         error with the switch left off, when another option of the command
 *         would then match this one.
 *
 * CLI11's ignore_case and ignore_underscore, which share their shape: turning
 * the switch on is tried against every other option of the command -- each
 * asked whether it now matches this one -- and undone, with CLI11's error, at
 * the first that does. Turning it off, or on again, cannot conflict.
 */
D_STATIC int
d_arg_internal_option_fold(
    struct d_arg_option* _option,
    uint32_t             _flag,
    bool                 _value,
    const char*          _what
)
{
    // only switching on can conflict
    if ( (!_value) ||
         (d_arg_option_has_flag(_option,
                                _flag)) )
    {
        d_arg_internal_option_switch(_option,
                                     _flag,
                                     _value);

        return D_ARG_OK;
    }

    _option->flags |= _flag;

    const struct d_arg_command* const parent = _option->parent;

    // ask every other option of the command
    for (size_t index = 0u;
         (parent != NULL) && (index < parent->options.count);
         ++index)
    {
        const struct d_arg_option* const other = parent->options.items[index];
        const char* const                match = (other != _option)
                                                 ? d_arg_option_matching_name(
                                                       other,
                                                       _option)
                                                 : NULL;

        // a conflict undoes the switch
        if (match != NULL)
        {
            _option->flags &= ~_flag;

            return d_arg_internal_fail_definition(
                       NULL,
                       _option,
                       D_ARG_ERROR_OPTION_ALREADY_ADDED,
                       "adding %s caused a name conflict with %s is already "
                       "added",
                       _what,
                       match);
        }
    }

    return D_ARG_OK;
}

/*
d_arg_option_set_ignore_case
  d_arg_internal_option_fold with CLI11's wording for case.
*/
int
d_arg_option_set_ignore_case(
    struct d_arg_option* _option,
    bool                 _value
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_option_fold(_option,
                                      D_ARG_OPTION_IGNORE_CASE,
                                      _value,
                                      "ignore case");
}

/*
d_arg_option_set_ignore_underscore
  d_arg_internal_option_fold with CLI11's wording for underscores.
*/
int
d_arg_option_set_ignore_underscore(
    struct d_arg_option* _option,
    bool                 _value
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_option_fold(_option,
                                      D_ARG_OPTION_IGNORE_UNDERSCORE,
                                      _value,
                                      "ignore underscore");
}

/*
d_arg_option_set_default_text
  CLI11's default_str(): the text alone; nothing is checked.
*/
int
d_arg_option_set_default_text(
    struct d_arg_option* _option,
    const char*          _text
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_option_replace_text(_option,
                                              &_option->default_text,
                                              _text);
}

// d_arg_internal_option_trial
//   struct: what a default_val trial sets aside and puts back -- the parser's
// error report, the option's values, and its state -- and where the trial's
// copies begin in the option's `owned`, so that they can be kept.
struct d_arg_internal_option_trial
{
    struct d_arg_error     error;
    struct d_arg_text_list results;
    size_t                 first;
    int                    state;
};

/**
 * @brief Sets an option's values and its parser's report aside for a trial.
 *
 * @param[in,out] _option  the option, left with no values.
 * @param[in,out] _parser  its parser, left with a clean report; NULL for none.
 * @param[out]    _trial   receives what was set aside.
 *
 * Sets the report and the values aside, leaving a clean report and no
 * values for the trial.
 */
D_STATIC void
d_arg_internal_option_trial_begin(
    struct d_arg_option*                _option,
    struct d_arg_parser*                _parser,
    struct d_arg_internal_option_trial* _trial
)
{
    const struct d_arg_error     no_error = D_ARG_ERROR_INIT;
    const struct d_arg_text_list empty    = D_ARG_LIST_INIT;

    _trial->error   = (_parser != NULL) ? _parser->error : no_error;
    _trial->results = _option->results;
    _trial->first   = _option->owned.count;
    _trial->state   = _option->state;

    // the trial starts with a clean report
    if (_parser != NULL)
    {
        _parser->error = no_error;
    }

    _option->results = empty;

    return;
}

/**
 * @brief Reverses a run of strings in place.
 *
 * @param[in,out] _items  the strings.
 * @param[in]     _from   the first of the run.
 * @param[in]     _to     one past its last.
 *
 * Reverses the strings of a list from `_from` up to `_to`.
 */
D_STATIC void
d_arg_internal_option_flip(
    char** _items,
    size_t _from,
    size_t _to
)
{
    size_t low  = _from;
    size_t high = _to;

    // swap from both ends toward the middle
    while ((high - low) > 1u)
    {
        char* const swap = _items[low];

        high--;
        _items[low]  = _items[high];
        _items[high] = swap;
        low++;
    }

    return;
}

/**
 * @brief Moves the copies a trial made to just after those kept already.
 *
 * @param[in,out] _option  the option, whose copies in `owned` from `_first` on
 *                         are kept.
 * @param[in]     _first   the first copy the trial made.
 *
 * Moves the copies a trial made -- `owned` from `_first` on -- to just after
 * those kept already, where clearing leaves them, so that a variable bound to
 * text keeps pointing at its default: three reversals rotate them past the
 * copies the values made, without allocating.
 */
D_STATIC void
d_arg_internal_option_keep(
    struct d_arg_option* _option,
    size_t               _first
)
{
    char** const items = _option->owned.items;
    const size_t kept  = _option->kept;
    const size_t count = _option->owned.count;

    // nothing new to keep
    if ( (_first >= count) ||
         (_first < kept) )
    {
        return;
    }

    d_arg_internal_option_flip(items,
                               kept,
                               _first);
    d_arg_internal_option_flip(items,
                               _first,
                               count);
    d_arg_internal_option_flip(items,
                               kept,
                               count);
    _option->kept = kept + (count - _first);

    return;
}

/**
 * @brief Ends a trial, putting back what was set aside.
 *
 * @param[in,out] _option    the option, whose values and state are put back.
 * @param[in,out] _parser    its parser, whose report is put back; NULL for
 *                           none.
 * @param[in]     _trial     what was set aside.
 * @param[in]     _callback  whether the trial ran the option's callback.
 * @param[in,out] _message   the buffer that gains the trial's message, when it
 *                           failed.
 *
 * Copies the trial's message when it failed, then puts the report and the
 * values back, with the state CLI11 leaves: PARSING after a callback, the
 * old state otherwise. The trial's copies are kept, whether it passed or not,
 * since its callback may have bound a variable to them.
 */
D_STATIC void
d_arg_internal_option_trial_end(
    struct d_arg_option*                _option,
    struct d_arg_parser*                _parser,
    struct d_arg_internal_option_trial* _trial,
    bool                                _callback,
    struct d_arg_buffer*                _message
)
{
    // keep the trial's message, then the report as it was
    if (_parser != NULL)
    {
        if (_parser->error.status != (int)D_ARG_OK)
        {
            (void)d_arg_buffer_append_cstr(_message,
                                           d_arg_error_message(
                                               &_parser->error));
        }

        d_arg_error_release(&_parser->error);
        _parser->error = _trial->error;
    }

    d_arg_text_list_release(&_option->results);
    _option->results = _trial->results;
    _option->state   = _callback
                       ? (int)D_ARG_OPTION_STATE_PARSING
                       : _trial->state;
    d_arg_internal_option_keep(_option,
                               _trial->first);

    return;
}

/**
 * @brief Tries a default as CLI11's default_val does.
 *
 * @param[in,out] _option   the option.
 * @param[in]     _text     the default.
 * @param[in,out] _message  the buffer that gains the message of a failure.
 * @return D_ARG_OK when the default passed; otherwise the failure, unrecorded,
 *         its message in `_message`.
 *
 * The trial of CLI11's default_val: the default added alone, then the
 * callback -- when it sets defaults and does not already run on every result
 * -- else the validators alone, as CLI11's _validate_results.
 */
D_STATIC int
d_arg_internal_option_try_default(
    struct d_arg_option* _option,
    const char*          _text,
    struct d_arg_buffer* _message
)
{
    struct d_arg_parser* const         parser   = d_arg_internal_parser_of(
                                                      _option);
    const bool                         callback =
        ( (d_arg_option_has_flag(_option,
                                 D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT)) &&
          (!d_arg_option_has_flag(_option,
                                  D_ARG_OPTION_TRIGGER_ON_PARSE)) );
    struct d_arg_internal_option_trial trial    = { D_ARG_ERROR_INIT,
                                                    D_ARG_LIST_INIT,
                                                    0u,
                                                    0 };

    d_arg_internal_option_trial_begin(_option,
                                      parser,
                                      &trial);

    int status = d_arg_internal_option_add(_option,
                                           &_option->results,
                                           d_arg_internal_text_of(_text),
                                           false,
                                           NULL);

    // the callback when it sets defaults, else the validators
    if (status == D_ARG_OK)
    {
        status = callback
                 ? d_arg_option_run_callback(_option)
                 : d_arg_internal_option_validate(_option,
                                                  &_option->results);
    }

    d_arg_internal_option_trial_end(_option,
                                    parser,
                                    &trial,
                                    callback,
                                    _message);

    return status;
}

/*
d_arg_option_set_default_value
  CLI11's default_val: the trial, then the default text. A conversion
failure is reported as CLI11 reports it -- through ConversionError's
two-string constructor, whose wording that is -- and any other as raised;
both as definition errors, since CLI11 throws them where the default is
set.
*/
int
d_arg_option_set_default_value(
    struct d_arg_option* _option,
    const char*          _text
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_text) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer message = D_ARG_BUFFER_INIT;
    struct d_arg_buffer name    = D_ARG_BUFFER_INIT;
    int                 status  = d_arg_internal_option_try_default(_option,
                                                                    _text,
                                                                    &message);

    // a conversion failure in CLI11's words
    if (status == D_ARG_ERROR_CONVERSION)
    {
        (void)d_arg_option_name(_option,
                                0u,
                                &name);
        status = d_arg_internal_fail_definition(
                     NULL,
                     _option,
                     D_ARG_ERROR_CONVERSION,
                     "The value %s is not an allowed value for given default "
                     "value(\"%s\") produces an error : %s",
                     (name.data != NULL) ? name.data : "",
                     _text,
                     (message.data != NULL) ? message.data : "");
    }
    // any other failure as raised
    else if (status != D_ARG_OK)
    {
        status = d_arg_internal_fail_definition(
                     NULL,
                     _option,
                     status,
                     (message.data != NULL) ? "%s" : NULL,
                     message.data);
    }
    // the default text, once the trial passed
    else
    {
        status = d_arg_internal_option_replace_text(_option,
                                                    &_option->default_text,
                                                    _text);
    }

    d_arg_buffer_release(&name);
    d_arg_buffer_release(&message);

    return status;
}

/*
d_arg_option_set_default_typed
  CLI11's default_val given a value: the value written into a buffer -- a
full buffer is a refused allocation -- then set as text; a value that could
not be written -- its type has no text form, or its writer failed -- is
recorded as the definition error and the default kept, where CLI11 writes a
value it cannot write as nothing and sets that as the default.
*/
int
d_arg_option_set_default_typed(
    struct d_arg_option*     _option,
    const struct d_arg_type* _type,
    const void*              _value
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_type)   ||
         (!_value) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_format(_type,
                                              _value,
                                              d_arg_buffer_sink(&text));

    status = ( (status == D_ARG_OK) &&
               (text.failed) )
             ? D_ARG_ERROR_OUT_OF_MEMORY
             : status;

    // the text set as a default value, or the failure recorded
    status = (status == D_ARG_OK)
             ? d_arg_option_set_default_value(_option,
                                              (text.data != NULL)
                                              ? text.data
                                              : "")
             : d_arg_internal_fail_definition(NULL,
                                              _option,
                                              status,
                                              NULL);
    d_arg_buffer_release(&text);

    return status;
}

/*
d_arg_option_set_default_fn
  As d_arg_option_set_type_name_fn, for the default function.
*/
void
d_arg_option_set_default_fn(
    struct d_arg_option* _option,
    d_arg_option_text_fn _write,
    void*                _context,
    d_arg_release_fn     _release
)
{
    void*            context = _context;
    d_arg_release_fn release = _release;

    // nothing to keep the context in
    if (!_option)
    {
        d_arg_internal_context_release(&context,
                                       &release);

        return;
    }

    d_arg_internal_context_replace(&_option->default_context,
                                   &_option->default_release,
                                   _context,
                                   _release);
    _option->default_fn = _write;

    return;
}

/*
d_arg_option_capture_default
  CLI11's capture_default_str: the default function's text, built in a
buffer, replaces the default only once it is complete -- a full buffer is a
refused allocation; without a function nothing changes.
*/
int
d_arg_option_capture_default(
    struct d_arg_option* _option
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // without a function there is nothing to capture
    if (_option->default_fn == NULL)
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = _option->default_fn(_option,
                                                     d_arg_buffer_sink(&text),
                                                     _option->default_context);

    status = ( (status == D_ARG_OK) &&
               (text.failed) )
             ? D_ARG_ERROR_OUT_OF_MEMORY
             : status;

    // the text replaces the default, or the failure is recorded
    status = (status == D_ARG_OK)
             ? d_arg_internal_option_replace_text(
                   _option,
                   &_option->default_text,
                   (text.data != NULL) ? text.data : "")
             : d_arg_internal_fail_definition(NULL,
                                              _option,
                                              status,
                                              NULL);
    d_arg_buffer_release(&text);

    return status;
}
