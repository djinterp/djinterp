/*******************************************************************************
* djinterp [parse]                                           arg_option_result.c
*
* What an option reports, declared in arg_option.h (sections 5 and 6, but
* for what arg_option_value.c defines), and CLI11's reduced_results, declared
* in arg_internal.h (7.2.9), which writing configuration shares.
*   Its text fields and counts, the type name help shows, its values as given
* and as reduced, their conversion into a variable, the bound variable and
* callback they are handed to, and the value a flag name stands for.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_option_result.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_convert, d_arg_flag_value_intmax
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator_describe
#include "./arg_internal.h"  // spans, lists, failures, watches, numbers
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intmax_t, uintmax_t


//==============================================================================
// 1.  FIELDS
//==============================================================================

/*
d_arg_option_description
  The owned text, or "" while it is unset.
*/
const char*
d_arg_option_description(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->description != NULL) )
           ? _option->description
           : "";
}

/*
d_arg_option_option_text
  As d_arg_option_description, for the text help shows in place of the type.
*/
const char*
d_arg_option_option_text(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->option_text != NULL) )
           ? _option->option_text
           : "";
}

/*
d_arg_option_group
  As d_arg_option_description, but "OPTIONS" while the group is unset, as
CLI11's options start in it.
*/
const char*
d_arg_option_group(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->group != NULL) )
           ? _option->group
           : "OPTIONS";
}

/*
d_arg_option_default_text
  As d_arg_option_description, for the default text.
*/
const char*
d_arg_option_default_text(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->default_text != NULL) )
           ? _option->default_text
           : "";
}

/*
d_arg_option_env_name
  As d_arg_option_description, for the environment variable's name.
*/
const char*
d_arg_option_env_name(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->env_name != NULL) )
           ? _option->env_name
           : "";
}

/*
d_arg_option_positional_name
  As d_arg_option_description, for the positional name.
*/
const char*
d_arg_option_positional_name(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->positional_name != NULL) )
           ? _option->positional_name
           : "";
}

/*
d_arg_option_items_expected_min
  CLI11's type_size_min times expected_min, held at D_ARG_EXPECTED_UNLIMITED
where the product would overflow, as the maximum is. CLI11 checks only the
maximum's product: its minimum overflows an `int` -- undefined behaviour,
which its builds turn into 0 or a negative count -- where this one holds.
*/
int
d_arg_option_items_expected_min(
    const struct d_arg_option* _option
)
{
    // nothing is expected of no option
    if (!_option)
    {
        return 0;
    }

    int product = _option->type_size_min;

    return d_arg_internal_checked_multiply(&product,
                                           _option->expected_min)
           ? product
           : (int)D_ARG_EXPECTED_UNLIMITED;
}

/*
d_arg_option_items_expected_max
  CLI11's get_items_expected_max: type_size_max times expected_max, through
checked_multiply, D_ARG_EXPECTED_UNLIMITED where it would overflow.
*/
int
d_arg_option_items_expected_max(
    const struct d_arg_option* _option
)
{
    // nothing is expected of no option
    if (!_option)
    {
        return 0;
    }

    int product = _option->type_size_max;

    return d_arg_internal_checked_multiply(&product,
                                           _option->expected_max)
           ? product
           : (int)D_ARG_EXPECTED_UNLIMITED;
}

/*
d_arg_option_is_positional
  CLI11's get_positional: a positional name that is not empty.
*/
bool
d_arg_option_is_positional(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL)                   &&
             (_option->positional_name != NULL)  &&
             (_option->positional_name[0] != '\0') );
}

/*
d_arg_option_is_nonpositional
  CLI11's nonpositional: a short name or a long name.
*/
bool
d_arg_option_is_nonpositional(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             ( (_option->short_names.count > 0u) ||
               (_option->long_names.count > 0u) ) );
}

/*
d_arg_internal_option_base_type_name
  The type name before the validators': the function's text, the fixed
name, or the bound type's.
*/
D_STATIC int
d_arg_internal_option_base_type_name(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    // a function writes it
    if (_option->type_name_fn != NULL)
    {
        return _option->type_name_fn(_option,
                                     d_arg_buffer_sink(_out),
                                     _option->type_name_context);
    }

    // a fixed name, else the bound type's, else none
    return d_arg_buffer_append_cstr(
               _out,
               (_option->type_name != NULL)
               ? _option->type_name
               : ( (_option->type != NULL)
                   ? _option->type->name
                   : NULL ));
}

/*
d_arg_internal_option_describe_one
  `:` and a validator's description, the colon taken back when the
description is empty.
*/
D_STATIC int
d_arg_internal_option_describe_one(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const size_t mark = _out->length;

    (void)d_arg_buffer_append_char(_out,
                                   ':');

    const int status = d_arg_validator_describe(_validator,
                                                _out);

    // an empty description takes no colon
    if (_out->length == (mark + 1u))
    {
        d_arg_buffer_truncate(_out,
                              mark);
    }

    return status;
}

/*
d_arg_option_type_name
  CLI11's get_type_name: the base name, then each validator's description
that is not empty after a colon -- an inactive validator describes nothing.
*/
int
d_arg_option_type_name(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = d_arg_internal_option_base_type_name(_option,
                                                      _out);

    // each validator's description
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->validators.count);
         ++index)
    {
        status = d_arg_internal_option_describe_one(
                     &_option->validators.items[index],
                     _out);
    }

    return ( (status == D_ARG_OK) &&
             (_out->failed) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

//==============================================================================
// 2.  READING
//==============================================================================

/*
d_arg_option_count
  The number of values as given.
*/
size_t
d_arg_option_count(
    const struct d_arg_option* _option
)
{
    return (_option != NULL)
           ? _option->results.count
           : 0u;
}

/*
d_arg_option_is_given
  CLI11's operator bool: a value, or a forced callback.
*/
bool
d_arg_option_is_given(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             ( (_option->results.count > 0u) ||
               (d_arg_option_has_flag(_option,
                                      D_ARG_OPTION_FORCE_CALLBACK)) ) );
}

/*
d_arg_internal_option_view
  A list as a view: its count written, its array returned, NULL when empty.
*/
D_STATIC const struct d_pack_text*
d_arg_internal_option_view(
    const struct d_arg_text_list* _list,
    size_t*                       _count
)
{
    // the count, when asked for
    if (_count != NULL)
    {
        *_count = _list->count;
    }

    return (_list->count > 0u)
           ? _list->items
           : NULL;
}

/*
d_arg_option_results
  The values as given, as a view.
*/
const struct d_pack_text*
d_arg_option_results(
    const struct d_arg_option* _option,
    size_t*                    _count
)
{
    const struct d_arg_text_list empty = D_ARG_LIST_INIT;

    return d_arg_internal_option_view((_option != NULL)
                                      ? &_option->results
                                      : &empty,
                                      _count);
}

/*
d_arg_option_values
  CLI11's send_results as a view: the values the policy kept, once the
option is reduced and the policy kept any, else those given.
*/
const struct d_pack_text*
d_arg_option_values(
    const struct d_arg_option* _option,
    size_t*                    _count
)
{
    const struct d_arg_text_list empty = D_ARG_LIST_INIT;

    // no option, no values
    if (!_option)
    {
        return d_arg_internal_option_view(&empty,
                                          _count);
    }

    return d_arg_internal_option_view(
               ( (_option->state >= (int)D_ARG_OPTION_STATE_REDUCED) &&
                 (_option->processed.count > 0u) )
               ? &_option->processed
               : &_option->results,
               _count);
}

/*
d_arg_option_callback_run
  CLI11's get_callback_run: the state is CALLBACK_RUN.
*/
bool
d_arg_option_callback_run(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             (_option->state == (int)D_ARG_OPTION_STATE_CALLBACK_RUN) );
}

//==============================================================================
// 3.  CONVERSION AND CALLBACKS
//==============================================================================

/*
d_arg_internal_option_checked
  Records a list's refused allocation, the one failure a list call reports.
*/
D_STATIC int
d_arg_internal_option_checked(
    const struct d_arg_option* _option,
    int                        _status
)
{
    return (_status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail(NULL,
                                 _option,
                                 _status,
                                 NULL);
}

/*
d_arg_internal_option_convert_failed
  CLI11's ConversionError(name, results): "Could not convert: <name> =
<values>", the values as given, joined by commas.
*/
D_STATIC int
d_arg_internal_option_convert_failed(
    const struct d_arg_option* _option
)
{
    struct d_arg_buffer name    = D_ARG_BUFFER_INIT;
    struct d_arg_buffer results = D_ARG_BUFFER_INIT;

    (void)d_arg_option_name(_option,
                            0u,
                            &name);
    (void)d_arg_internal_option_results_text(_option,
                                             &results);

    const int status = d_arg_internal_fail(
                           NULL,
                           _option,
                           D_ARG_ERROR_CONVERSION,
                           "Could not convert: %s = %s",
                           (name.data != NULL) ? name.data : "",
                           (results.data != NULL) ? results.data : "");

    d_arg_buffer_release(&results);
    d_arg_buffer_release(&name);

    return status;
}

/*
d_arg_internal_option_refused
  Records why values were refused: the reason a converter gave; for a
conversion with none, CLI11's ConversionError; any other status as it is.
*/
D_STATIC int
d_arg_internal_option_refused(
    const struct d_arg_option* _option,
    int                        _status,
    const struct d_arg_buffer* _reason
)
{
    // the converter's reason
    if (_reason->length > 0u)
    {
        return d_arg_internal_fail(NULL,
                                   _option,
                                   _status,
                                   "%s",
                                   _reason->data);
    }

    // CLI11's own words for a conversion
    if (_status == D_ARG_ERROR_CONVERSION)
    {
        return d_arg_internal_option_convert_failed(_option);
    }

    return d_arg_internal_fail(NULL,
                               _option,
                               _status,
                               NULL);
}

/*
d_arg_internal_option_has_callback
  Whether values have anywhere to go: a bound variable, or a callback.
*/
D_STATIC bool
d_arg_internal_option_has_callback(
    const struct d_arg_option* _option
)
{
    return ( ( (_option->type != NULL) &&
               (_option->target != NULL) ) ||
             (_option->callback != NULL) );
}

/*
d_arg_internal_option_invoke
  Hands values to the bound variable, through its type's conversion, then
to the callback; the first refusal ends the call, a converter's reason
written into `_reason`.
*/
D_STATIC int
d_arg_internal_option_invoke(
    struct d_arg_option*          _option,
    const struct d_arg_text_list* _values,
    struct d_arg_buffer*          _reason
)
{
    const struct d_arg_convert_context context = { _option->integer_rule,
                                                   _option,
                                                   _reason };
    int                                status  = D_ARG_OK;

    // the bound variable first
    if ( (_option->type != NULL) &&
         (_option->target != NULL) )
    {
        status = d_arg_convert(_option->type,
                               _values->items,
                               _values->count,
                               _option->target,
                               &context);
    }

    // then the callback
    if ( (status == D_ARG_OK) &&
         (_option->callback != NULL) )
    {
        status = _option->callback(_option,
                                   _values->items,
                                   _values->count,
                                   _option->callback_context);
    }

    return status;
}

/*
d_arg_internal_option_call_back
  The second half of CLI11's run_callback: the values the policy kept, else
those given, handed on -- nothing when there are none or nowhere to hand
them -- with the parser's report watched; then a forced default dropped
again, before a refusal the callback did not record is reported over the
values that are left, as CLI11 reports it.
*/
D_STATIC int
d_arg_internal_option_call_back(
    struct d_arg_option* _option,
    bool                 _forced
)
{
    const struct d_arg_text_list* const values =
        (_option->processed.count > 0u)
        ? &_option->processed
        : &_option->results;

    // nothing to hand on, or nowhere to hand it
    if ( (values->count == 0u) ||
         (!d_arg_internal_option_has_callback(_option)) )
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer         reason = D_ARG_BUFFER_INIT;
    struct d_arg_internal_watch watch  = { NULL,
                                           D_ARG_ERROR_INIT };

    d_arg_internal_watch_begin(&watch,
                               d_arg_internal_parser_of(_option));

    int        status   = d_arg_internal_option_invoke(_option,
                                                       values,
                                                       &reason);
    const bool recorded = d_arg_internal_watch_end(&watch,
                                                   status);

    // a forced default goes once it was handed on
    if (_forced)
    {
        _option->results.count   = 0u;
        _option->processed.count = 0u;
    }

    // a refusal the callback did not record
    if ( (status != D_ARG_OK) &&
         (!recorded) )
    {
        status = d_arg_internal_option_refused(_option,
                                               status,
                                               &reason);
    }

    d_arg_buffer_release(&reason);

    return status;
}

/*
d_arg_option_run_callback
  CLI11's run_callback: a forced option with no values takes its default
text, as a new value; the values are validated and reduced as far as they
are not yet, the option is CALLBACK_RUN, and the values are handed on.
*/
int
d_arg_option_run_callback(
    struct d_arg_option* _option
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool forced = ( (_option->results.count == 0u) &&
                          (d_arg_option_has_flag(
                               _option,
                               D_ARG_OPTION_FORCE_CALLBACK)) );
    int        status = D_ARG_OK;

    // the default text stands in for the values
    if (forced)
    {
        status = d_arg_internal_option_add(
                     _option,
                     &_option->results,
                     d_arg_internal_text_of(_option->default_text),
                     false,
                     NULL);
        _option->state = (int)D_ARG_OPTION_STATE_PARSING;
    }

    // validated and reduced, as far as they are not yet
    if (status == D_ARG_OK)
    {
        status = d_arg_option_reduce(_option);
    }

    // a value refused stops the callback
    if (status != D_ARG_OK)
    {
        return status;
    }

    _option->state = (int)D_ARG_OPTION_STATE_CALLBACK_RUN;

    return d_arg_internal_option_call_back(_option,
                                           forced);
}

/*
d_arg_internal_option_reduce_over
  Reduces a list into a second one, which takes the first's place when the
policy kept anything.
*/
D_STATIC int
d_arg_internal_option_reduce_over(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list
)
{
    struct d_arg_text_list extra  = D_ARG_LIST_INIT;
    const int              status = d_arg_internal_option_reduce(_option,
                                                                 &extra,
                                                                 _list);

    // the reduction stands for the values when it kept any
    if ( (status == D_ARG_OK) &&
         (extra.count > 0u) )
    {
        d_arg_text_list_release(_list);
        *_list = extra;

        return D_ARG_OK;
    }

    d_arg_text_list_release(&extra);

    return status;
}

/*
d_arg_internal_option_reduced
  CLI11's reduced_results, into `_out`: the values as given while they are
new or none were kept by a reduction, else those kept; new ones validated
on the copy, and those not yet reduced reduced over, when there are any.
*/
int
d_arg_internal_option_reduced(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _out
)
{
    const bool parsing = (_option->state ==
                          (int)D_ARG_OPTION_STATE_PARSING);
    const struct d_arg_text_list* const source =
        ( (parsing) ||
          (_option->processed.count == 0u) )
        ? &_option->results
        : &_option->processed;
    int status = d_arg_internal_option_checked(
                     _option,
                     d_arg_internal_text_append(_out,
                                                source,
                                                0u,
                                                source->count));

    // reduced values stand as they are
    if ( (status != D_ARG_OK) ||
         (_option->state >= (int)D_ARG_OPTION_STATE_REDUCED) )
    {
        return status;
    }

    // new values are validated on the copy
    if (parsing)
    {
        status = d_arg_internal_option_validate(_option,
                                                _out);
    }

    return ( (status == D_ARG_OK) &&
             (_out->count > 0u) )
           ? d_arg_internal_option_reduce_over(_option,
                                               _out)
           : status;
}

/*
d_arg_internal_option_as_default
  The default text as CLI11's results(T&) takes it for an option without
values: added to the list, validated, and reduced over.
*/
D_STATIC int
d_arg_internal_option_as_default(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _values
)
{
    int status = d_arg_internal_option_add(
                     _option,
                     _values,
                     d_arg_internal_text_of(_option->default_text),
                     false,
                     NULL);

    // the default is judged as a value is
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_validate(_option,
                                                _values);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_option_reduce_over(_option,
                                               _values)
           : status;
}

/*
d_arg_internal_option_as_fresh
  CLI11's results(T&) for values not yet reduced: the reduced values; with
none, the default text's; without one, a single empty value. Whichever they
are become `processed`.
*/
D_STATIC int
d_arg_internal_option_as_fresh(
    struct d_arg_option* _option
)
{
    struct d_arg_text_list values = D_ARG_LIST_INIT;
    int                    status = D_ARG_OK;

    // the values, the default's, or one empty value
    if (_option->results.count > 0u)
    {
        status = d_arg_internal_option_reduced(_option,
                                               &values);
    }
    else if ( (_option->default_text != NULL) &&
              (_option->default_text[0] != '\0') )
    {
        status = d_arg_internal_option_as_default(_option,
                                                  &values);
    }
    else
    {
        status = d_arg_internal_option_checked(
                     _option,
                     d_arg_internal_text_push(&values,
                                              d_arg_internal_span("",
                                                                  0u)));
    }

    // they become the processed values
    if (status == D_ARG_OK)
    {
        d_arg_text_list_release(&_option->processed);
        _option->processed = values;

        return D_ARG_OK;
    }

    d_arg_text_list_release(&values);

    return status;
}

/*
d_arg_internal_option_as_settled
  CLI11's results(T&) for reduced values, or a single value with no
validator to run: the values stand, but none at all become a single empty
value in `processed`.
*/
D_STATIC int
d_arg_internal_option_as_settled(
    struct d_arg_option* _option
)
{
    // there are values to convert
    if ( (_option->processed.count > 0u) ||
         (_option->results.count > 0u) )
    {
        return D_ARG_OK;
    }

    return d_arg_internal_option_checked(
               _option,
               d_arg_internal_text_push(&_option->processed,
                                        d_arg_internal_span("",
                                                            0u)));
}

/*
d_arg_internal_option_convert_into
  The values the policy kept, else those given, into the variable through
the type, a refusal reported as d_arg_internal_option_refused reports it.
*/
D_STATIC int
d_arg_internal_option_convert_into(
    const struct d_arg_option* _option,
    const struct d_arg_type*   _type,
    void*                      _target
)
{
    const struct d_arg_text_list* const values =
        (_option->processed.count > 0u)
        ? &_option->processed
        : &_option->results;
    struct d_arg_buffer                 reason  = D_ARG_BUFFER_INIT;
    const struct d_arg_convert_context  context = { _option->integer_rule,
                                                    _option,
                                                    &reason };
    int                                 status  = d_arg_convert(
                                                      _type,
                                                      values->items,
                                                      values->count,
                                                      _target,
                                                      &context);

    // a refusal, reported
    if (status != D_ARG_OK)
    {
        status = d_arg_internal_option_refused(_option,
                                               status,
                                               &reason);
    }

    d_arg_buffer_release(&reason);

    return status;
}

/*
d_arg_option_as
  CLI11's results(T&): values already reduced, or a single value with no
validator, are converted as they stand; others are reduced first, into
`processed`, without changing the option's state.
*/
int
d_arg_option_as(
    struct d_arg_option*     _option,
    const struct d_arg_type* _type,
    void*                    _target
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_type)   ||
         (!_target) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool settled = ( (_option->state >=
                            (int)D_ARG_OPTION_STATE_REDUCED) ||
                           ( (_option->results.count == 1u) &&
                             (_option->validators.count == 0u) ) );
    const int  status  = settled
                         ? d_arg_internal_option_as_settled(_option)
                         : d_arg_internal_option_as_fresh(_option);

    return (status == D_ARG_OK)
           ? d_arg_internal_option_convert_into(_option,
                                                _type,
                                                _target)
           : status;
}

//==============================================================================
// 4.  FLAGS
//==============================================================================

/*
d_arg_internal_option_override
  CLI11's check of a flag that disables overrides: a name with a value of
its own takes only that value -- or the default text of a forced option,
which then stands as given, setting `*_stands` -- and any other name takes
only "true". A refusal is CLI11's ArgumentMismatch::FlagOverride.
*/
D_STATIC int
d_arg_internal_option_override(
    const struct d_arg_option* _option,
    struct d_pack_text         _name,
    struct d_pack_text         _input,
    long                       _index,
    bool*                      _stands
)
{
    // a name with a value of its own
    if (_index >= 0)
    {
        // its own value
        if (d_arg_internal_equal_cstr(_input,
                                      _option->flag_values.items[_index]))
        {
            return D_ARG_OK;
        }

        // the default text of a forced option stands as given
        if ( (d_arg_option_has_flag(_option,
                                    D_ARG_OPTION_FORCE_CALLBACK)) &&
             (d_arg_internal_equal_cstr(_input,
                                        d_arg_option_default_text(
                                            _option))) )
        {
            *_stands = true;

            return D_ARG_OK;
        }
    }
    // any other name takes "true" alone
    else if (d_arg_internal_equal_cstr(_input,
                                       "true"))
    {
        return D_ARG_OK;
    }

    return d_arg_internal_fail(NULL,
                               _option,
                               D_ARG_ERROR_ARGUMENT_MISMATCH,
                               "%.*s was given a disallowed flag override",
                               d_arg_internal_print_length(_name),
                               _name.data);
}

/*
d_arg_internal_option_negated
  The text of a flag value's negation, worked out in uintmax_t where the
value is negative, so that the most negative `long long` negates too: to
its magnitude, where CLI11's std::to_string(-val) overflows.
*/
D_STATIC int
d_arg_internal_option_negated(
    intmax_t             _value,
    struct d_arg_buffer* _out
)
{
    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    // a positive value negates within its type; any other through unsigned
    if (_value > 0)
    {
        d_arg_internal_text_signed(&text,
                                   -_value);
    }
    else
    {
        d_arg_internal_text_unsigned(&text,
                                     (uintmax_t)0 - (uintmax_t)_value);
    }

    return d_arg_buffer_append(_out,
                               text.data,
                               text.length);
}

/*
d_arg_internal_option_invert
  An input to a name standing for "false", inverted as CLI11 inverts it: 1
is "false", -1 is "true", any other flag value its negation; an input that
is no flag value stands as given. The value is read over CLI11's whole
range, which a `long` may not hold.
*/
D_STATIC int
d_arg_internal_option_invert(
    const struct d_arg_option* _option,
    struct d_pack_text         _input,
    struct d_arg_buffer*       _out
)
{
    intmax_t value = 0;

    // an input that is no flag value stands
    if (d_arg_flag_value_intmax(_input,
                                _option->integer_rule,
                                &value) != D_ARG_OK)
    {
        return d_arg_buffer_append_text(_out,
                                        _input);
    }

    // the two truth values swap
    if ( (value == 1) ||
         (value == -1) )
    {
        return d_arg_buffer_append_cstr(_out,
                                        (value == 1) ? "false" : "true");
    }

    return d_arg_internal_option_negated(value,
                                         _out);
}

/*
d_arg_internal_option_flag_default
  The value of a flag name given without an input: the name's own value;
for a name without one, "true" for a flag, the default text otherwise.
*/
D_STATIC int
d_arg_internal_option_flag_default(
    const struct d_arg_option* _option,
    long                       _index,
    struct d_arg_buffer*       _out
)
{
    // the name's own value
    if (_index >= 0)
    {
        return d_arg_buffer_append_cstr(_out,
                                        _option->flag_values.items[_index]);
    }

    return d_arg_buffer_append_cstr(
               _out,
               (d_arg_option_has_flag(_option,
                                      D_ARG_OPTION_FLAG_LIKE))
               ? "true"
               : d_arg_option_default_text(_option));
}

/*
d_arg_internal_option_flag_index
  CLI11's find_member of a name among the flag names, under the option's
ignore-case and ignore-underscore switches: its index, or -1.
*/
D_STATIC long
d_arg_internal_option_flag_index(
    const struct d_arg_option* _option,
    struct d_pack_text         _name
)
{
    return d_arg_internal_find_member(
               _name,
               &_option->flag_names,
               d_arg_option_has_flag(_option,
                                     D_ARG_OPTION_IGNORE_CASE),
               d_arg_option_has_flag(_option,
                                     D_ARG_OPTION_IGNORE_UNDERSCORE));
}

/*
d_arg_internal_option_flag_input
  The value of a flag name given an input: the input inverted for a name
standing for "false", the input itself otherwise.
*/
D_STATIC int
d_arg_internal_option_flag_input(
    const struct d_arg_option* _option,
    long                       _index,
    struct d_pack_text         _input,
    struct d_arg_buffer*       _out
)
{
    // a name standing for "false" inverts its input
    if ( (_index >= 0) &&
         (d_arg_internal_equal_cstr(
              d_arg_internal_text_of(_option->flag_values.items[_index]),
              "false")) )
    {
        return d_arg_internal_option_invert(_option,
                                            _input,
                                            _out);
    }

    return d_arg_buffer_append_text(_out,
                                    _input);
}

/*
d_arg_option_flag_value
  CLI11's get_flag_value: an input other than nothing or `{}` is checked
first when the option disables overrides; then a name without an input
gives its default, and one with an input gives the input, inverted or not.
*/
int
d_arg_option_flag_value(
    const struct d_arg_option* _option,
    struct d_pack_text         _name,
    struct d_pack_text         _input,
    struct d_arg_buffer*       _out
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool none   = ( (_input.length == 0u) ||
                          (d_arg_internal_equal_cstr(_input,
                                                     "{}")) );
    const long index  = d_arg_internal_option_flag_index(_option,
                                                         _name);
    bool       stands = false;

    // a flag that refuses overrides checks the input first
    if ( (!none) &&
         (d_arg_option_has_flag(_option,
                                D_ARG_OPTION_DISABLE_FLAG_OVERRIDE)) )
    {
        const int status = d_arg_internal_option_override(_option,
                                                          _name,
                                                          _input,
                                                          index,
                                                          &stands);

        // refused, or standing as given
        if ( (status != D_ARG_OK) ||
             (stands) )
        {
            return (status != D_ARG_OK)
                   ? status
                   : d_arg_buffer_append_text(_out,
                                              _input);
        }
    }

    // no input: the default the name gives
    if (none)
    {
        return d_arg_internal_option_flag_default(_option,
                                                  index,
                                                  _out);
    }

    return d_arg_internal_option_flag_input(_option,
                                            index,
                                            _input,
                                            _out);
}
