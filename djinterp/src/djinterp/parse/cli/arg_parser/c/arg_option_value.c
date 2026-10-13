/*******************************************************************************
* djinterp [parse]                                            arg_option_value.c
*
* The values given to an option, declared in arg_option.h (6.2.2, 6.3.1 and
* 6.3.2) and arg_internal.h (7.2.3 to 7.2.7).
*   How a value is added -- CLI11's escape for a literal bracketed value, its
* bracketed lists, its delimiter -- how an option's validators judge its
* values, and how its multi-option policy reduces them, completing the
* indicator CLI11 writes for an empty container.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_option_value.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // memchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_sum_values
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validation, d_arg_validator_run
#include "./arg_internal.h"  // spans, lists, failures, d_arg_internal_split_up


//==============================================================================
// 1.  ADDING
//==============================================================================

/*
d_arg_internal_option_is_separator
  CLI11's is_separator: an empty value, or "%%".
*/
D_STATIC bool
d_arg_internal_option_is_separator(
    struct d_pack_text _value
)
{
    return ( (_value.length == 0u) ||
             (d_arg_internal_equal_cstr(_value,
                                        "%%")) );
}

/*
d_arg_internal_option_push_value
  One value onto a list: kept as the span it is when it is stable and ends
at the NUL of the string it lies in, else copied into the option's `owned`.
Room in the list is made first, so a copy is never left without the value
that refers to it.
*/
D_STATIC int
d_arg_internal_option_push_value(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_pack_text      _value,
    bool                    _stable
)
{
    // a stable value that ends at its terminator is kept as it is
    if ( (_stable)                              &&
         (_value.data != NULL)                  &&
         (_value.data[_value.length] == '\0') )
    {
        return d_arg_internal_text_push(_list,
                                        _value);
    }

    const char* copy   = NULL;
    int         status = d_arg_internal_text_reserve(_list,
                                                     _list->count + 1u);

    // the copy, once the list has room for it
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_string_copy(&_option->owned,
                                            _value,
                                            &copy);
    }

    // the copy's span, which the reserve lets in
    if (status == D_ARG_OK)
    {
        (void)d_arg_internal_text_push(_list,
                                       d_arg_internal_span(copy,
                                                           _value.length));
    }

    return status;
}

/*
d_arg_internal_option_push_built
  A value built in a buffer, onto a list: the buffer's storage becomes one of
the option's copies, so nothing is copied again, and an empty value, which
has no storage, is the empty literal. The buffer is left empty or failed.
*/
D_STATIC int
d_arg_internal_option_push_built(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_arg_buffer*    _built
)
{
    // nothing built: the empty text needs no storage
    if ( (!_built->failed) &&
         (_built->data == NULL) )
    {
        return d_arg_internal_text_push(_list,
                                        d_arg_internal_span("",
                                                            0u));
    }

    const size_t length = _built->length;
    int          status = (_built->failed)
                          ? D_ARG_ERROR_OUT_OF_MEMORY
                          : d_arg_internal_text_reserve(_list,
                                                        _list->count + 1u);
    char* const  string = d_arg_buffer_detach(_built);

    // the storage goes to the option, which frees it if it cannot take it
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_string_adopt(&_option->owned,
                                             string);
    }
    else
    {
        free(string);
    }

    // its span, which the reserve lets in
    if (status == D_ARG_OK)
    {
        (void)d_arg_internal_text_push(_list,
                                       d_arg_internal_span(string,
                                                           length));
    }

    return status;
}

/*
d_arg_internal_option_is_escape
  CLI11's escape for a value that would read as a bracketed list: `[[` and
`]]` around text in which every character comes twice.
*/
D_STATIC bool
d_arg_internal_option_is_escape(
    struct d_pack_text _value
)
{
    const char* const data   = _value.data;
    const size_t      length = _value.length;

    // `[[` and `]]` around the text
    if ( (length < 4u)              ||
         (data[0] != '[')           ||
         (data[1] != '[')           ||
         (data[length - 1u] != ']') ||
         (data[length - 2u] != ']') )
    {
        return false;
    }

    // every character between comes twice, pair by pair
    for (size_t index = 2u; index < (length - 2u); index += 2u)
    {
        if (data[index] != data[index + 1u])
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_option_add_escaped
  An escape as the one value it stands for: the text with each doubled
character once, in single brackets.
*/
D_STATIC int
d_arg_internal_option_add_escaped(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_pack_text      _value,
    int*                    _added
)
{
    struct d_arg_buffer literal = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_char(&literal,
                                   '[');

    // one of each pair
    for (size_t index = 2u; index < (_value.length - 2u); index += 2u)
    {
        (void)d_arg_buffer_append_char(&literal,
                                       _value.data[index]);
    }

    (void)d_arg_buffer_append_char(&literal,
                                   ']');

    const int status = d_arg_internal_option_push_built(_option,
                                                        _list,
                                                        &literal);

    // one value, counted
    if (status == D_ARG_OK)
    {
        (*_added)++;
    }

    d_arg_buffer_release(&literal);

    return status;
}

/*
d_arg_internal_option_is_list
  CLI11's test for a bracketed list: a value in brackets, for an option that
takes extra arguments, several values, or items of several values.
*/
D_STATIC bool
d_arg_internal_option_is_list(
    const struct d_arg_option* _option,
    struct d_pack_text         _value
)
{
    return ( ( (d_arg_option_has_flag(_option,
                                      D_ARG_OPTION_ALLOW_EXTRA_ARGS)) ||
               (_option->expected_max > 1)                           ||
               (_option->type_size_min > 1) )                        &&
             (_value.length > 0u)                                    &&
             (_value.data[0] == '[')                                 &&
             (_value.data[_value.length - 1u] == ']') );
}

/*
d_arg_internal_option_add_split
  CLI11's split at the option's delimiter: each nonempty piece is a value,
copied, since a piece is not terminated where it ends.
*/
D_STATIC int
d_arg_internal_option_add_split(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_pack_text      _value,
    int*                    _added
)
{
    size_t start = 0u;

    // the pieces between delimiters, and after the last one
    while (start < _value.length)
    {
        const char* const next = (const char*)memchr(
                                     _value.data + start,
                                     (unsigned char)_option->delimiter,
                                     _value.length - start);
        const size_t      end  = (next != NULL)
                                 ? (size_t)(next - _value.data)
                                 : _value.length;

        // an empty piece is no value
        if (end > start)
        {
            const int status = d_arg_internal_option_push_value(
                                   _option,
                                   _list,
                                   d_arg_internal_span(_value.data + start,
                                                       end - start),
                                   false);

            if (status != D_ARG_OK)
            {
                return status;
            }

            (*_added)++;
        }

        start = end + 1u;
    }

    return D_ARG_OK;
}

/*
d_arg_internal_option_add_one
  A value that is not a bracketed list: the value itself when the option has
no delimiter or the value holds none, else its pieces.
*/
D_STATIC int
d_arg_internal_option_add_one(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_pack_text      _value,
    bool                    _stable,
    int*                    _added
)
{
    // the delimiter splits the value into pieces
    if ( (_option->delimiter != '\0') &&
         (_value.length > 0u)         &&
         (memchr(_value.data,
                 (unsigned char)_option->delimiter,
                 _value.length) != NULL) )
    {
        return d_arg_internal_option_add_split(_option,
                                               _list,
                                               _value,
                                               _added);
    }

    const int status = d_arg_internal_option_push_value(_option,
                                                        _list,
                                                        _value,
                                                        _stable);

    // one value, counted
    if (status == D_ARG_OK)
    {
        (*_added)++;
    }

    return status;
}

/*
d_arg_internal_option_add_value
  CLI11's _add_result: an escape is one value; a bracketed list is split as
d_arg_internal_split_up splits it at commas, and each nonempty piece is
added as a value of its own -- itself a list, perhaps -- from a copy, since
the pieces are freed here; anything else is one value, or its pieces.
*/
D_STATIC int
d_arg_internal_option_add_value(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_pack_text      _value,
    bool                    _stable,
    int*                    _added
)
{
    // the escape for a literal bracketed value
    if (d_arg_internal_option_is_escape(_value))
    {
        return d_arg_internal_option_add_escaped(_option,
                                                 _list,
                                                 _value,
                                                 _added);
    }

    // anything but a bracketed list
    if (!d_arg_internal_option_is_list(_option,
                                       _value))
    {
        return d_arg_internal_option_add_one(_option,
                                             _list,
                                             _value,
                                             _stable,
                                             _added);
    }

    struct d_arg_string_list pieces = D_ARG_LIST_INIT;
    int                      status = d_arg_internal_split_up(
                                          d_arg_internal_span(
                                              _value.data + 1,
                                              _value.length - 2u),
                                          ',',
                                          &pieces);

    // each nonempty piece in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < pieces.count);
         ++index)
    {
        const struct d_pack_text piece = d_arg_internal_text_of(
                                             pieces.items[index]);

        if (piece.length > 0u)
        {
            status = d_arg_internal_option_add_value(_option,
                                                     _list,
                                                     piece,
                                                     false,
                                                     _added);
        }
    }

    d_arg_string_list_release(&pieces);

    return status;
}

/*
d_arg_internal_option_add
  d_arg_internal_option_add_value, counting what it added, with a failure
recorded.
*/
int
d_arg_internal_option_add(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_pack_text      _value,
    bool                    _stable,
    int*                    _added
)
{
    int       added  = 0;
    const int status = d_arg_internal_option_add_value(_option,
                                                       _list,
                                                       _value,
                                                       _stable,
                                                       &added);

    // the count, when asked for
    if (_added != NULL)
    {
        *_added = added;
    }

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail(NULL,
                                 _option,
                                 status,
                                 NULL);
}

/*
d_arg_internal_option_add_built
  The buffer's storage adopted into the option's `owned` before the value is
added, so that d_arg_internal_option_add, told the value is stable, keeps
it as its span -- it ends at its terminator -- and copies only the pieces of
one it splits. Nothing built is the empty literal, which needs no storage;
a buffer that failed adds nothing.
*/
int
d_arg_internal_option_add_built(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list,
    struct d_arg_buffer*    _built,
    int*                    _added
)
{
    const size_t length = _built->length;

    // nothing added yet
    if (_added != NULL)
    {
        *_added = 0;
    }

    // a buffer that failed
    if (_built->failed != 0)
    {
        return d_arg_internal_fail(NULL,
                                   _option,
                                   D_ARG_ERROR_OUT_OF_MEMORY,
                                   NULL);
    }

    // nothing built: the empty literal
    if (length == 0u)
    {
        return d_arg_internal_option_add(_option,
                                         _list,
                                         d_arg_internal_span("",
                                                             0u),
                                         true,
                                         _added);
    }

    char* const string = d_arg_buffer_detach(_built);
    const int   status = d_arg_internal_string_adopt(&_option->owned,
                                                     string);

    // the option keeps the storage, the list its span
    return (status == D_ARG_OK)
           ? d_arg_internal_option_add(_option,
                                       _list,
                                       d_arg_internal_span(string,
                                                           length),
                                       true,
                                       _added)
           : d_arg_internal_fail(NULL,
                                 _option,
                                 status,
                                 NULL);
}

/*
d_arg_option_add_result
  CLI11's add_result(value, count): the value copied into the results, and
the option returned to PARSING.
*/
int
d_arg_option_add_result(
    struct d_arg_option* _option,
    struct d_pack_text   _value,
    int*                 _added
)
{
    // validate the parameters
    if ( (!_option) ||
         ( (_value.data == NULL) &&
           (_value.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const int status = d_arg_internal_option_add(_option,
                                                 &_option->results,
                                                 _value,
                                                 false,
                                                 _added);

    _option->state = (int)D_ARG_OPTION_STATE_PARSING;

    return status;
}

/*
d_arg_option_add_results
  CLI11's add_result(vector): each value as d_arg_option_add_result adds
it, until one fails.
*/
int
d_arg_option_add_results(
    struct d_arg_option*      _option,
    const struct d_pack_text* _values,
    size_t                    _count
)
{
    // validate the parameters
    if ( (!_option) ||
         ( (!_values) &&
           (_count > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = D_ARG_OK;

    _option->state = (int)D_ARG_OPTION_STATE_PARSING;

    // each value in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _count);
         ++index)
    {
        status = d_arg_option_add_result(_option,
                                         _values[index],
                                         NULL);
    }

    return status;
}

/*
d_arg_option_clear
  Empties both lists and frees the copies past those kept for a default
value, which a bound variable may point at.
*/
void
d_arg_option_clear(
    struct d_arg_option* _option
)
{
    // nothing to clear without an option
    if (!_option)
    {
        return;
    }

    _option->results.count   = 0u;
    _option->processed.count = 0u;
    d_arg_internal_string_truncate(&_option->owned,
                                   _option->kept);
    _option->state = (int)D_ARG_OPTION_STATE_PARSING;

    return;
}

//==============================================================================
// 2.  JUDGING
//==============================================================================

/*
d_arg_internal_option_run_all
  The validators that apply to `_index`, in order, each on what the one
before made of the value, until one rejects it or stops; a validation error
is the rejection CLI11 catches, its message the verdict -- with none, a
pass.
*/
D_STATIC int
d_arg_internal_option_run_all(
    const struct d_arg_option* _option,
    int                        _index,
    struct d_arg_validation*   _validation
)
{
    int status = D_ARG_OK;

    // each validator that applies
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->validators.count);
         ++index)
    {
        const struct d_arg_validator* const validator =
            &_option->validators.items[index];

        // one for another position is skipped
        if ( (validator->application_index != -1) &&
             (validator->application_index != _index) )
        {
            continue;
        }

        const int ran = d_arg_validator_run(validator,
                                            _validation);

        status = (ran == D_ARG_ERROR_VALIDATION) ? D_ARG_OK : ran;

        // a rejection ends the judgement
        if (!d_arg_validation_passed(_validation))
        {
            break;
        }
    }

    return status;
}

/*
d_arg_internal_option_judge
  CLI11's _validate on a validation of the value: an empty value passes when
nothing is expected; otherwise the validators run, and the verdict -- the
rejection, or what they made of the value -- is written.
*/
int
d_arg_internal_option_judge(
    const struct d_arg_option* _option,
    struct d_pack_text         _value,
    int                        _index,
    struct d_arg_buffer*       _message,
    struct d_arg_buffer*       _result
)
{
    struct d_arg_validation validation = { { NULL, 0u },
                                           D_ARG_BUFFER_INIT,
                                           D_ARG_BUFFER_INIT,
                                           NULL,
                                           0 };
    int                     status     = D_ARG_OK;

    d_arg_validation_init(&validation,
                          _value);
    validation.option       = _option;
    validation.integer_rule = _option->integer_rule;

    // an empty value with nothing expected is allowed
    if ( (_value.length > 0u) ||
         (_option->expected_min != 0) )
    {
        status = d_arg_internal_option_run_all(_option,
                                               _index,
                                               &validation);
    }

    int written = D_ARG_OK;

    // the rejection, or why the judgement stopped
    if ( (status != D_ARG_OK) ||
         (!d_arg_validation_passed(&validation)) )
    {
        written = d_arg_buffer_append_text(
                      _message,
                      d_arg_buffer_text(&validation.message));
    }
    // what the validators made of the value
    else if (_result != NULL)
    {
        written = d_arg_buffer_append_text(_result,
                                           validation.value);
    }

    d_arg_validation_release(&validation);

    return (status != D_ARG_OK)
           ? status
           : ( (written != D_ARG_OK)
               ? D_ARG_ERROR_OUT_OF_MEMORY
               : D_ARG_OK );
}

// d_arg_internal_option_pass
//   struct: one pass of an option's validators over a list of its values,
// with the buffers each value's verdict and result are written into, reused
// from value to value.
struct d_arg_internal_option_pass
{
    struct d_arg_option*    option;
    struct d_arg_text_list* list;
    struct d_arg_buffer     message;
    struct d_arg_buffer     result;
};

/*
d_arg_internal_option_first_index
  Where CLI11's _validate_results starts counting: 0, or -- when the policy
keeps the last of more values than the option takes -- minus the number of
values the policy will drop, so the values it keeps count from 0. The limit
is the item count for a multi-value type, the expected count otherwise.
*/
D_STATIC int
d_arg_internal_option_first_index(
    const struct d_arg_option* _option,
    size_t                     _count
)
{
    const int  limit    = (_option->type_size_max > 1)
                          ? d_arg_option_items_expected_max(_option)
                          : _option->expected_max;
    const int  policy   = _option->multi_option_policy;
    const bool from_end = ( (policy == (int)D_ARG_MULTI_TAKE_LAST) ||
                            (policy == (int)D_ARG_MULTI_REVERSE) );

    return ( (from_end)                    &&
             (_count <= (size_t)INT_MAX)   &&
             ((size_t)limit < _count) )
           ? (limit - (int)_count)
           : 0;
}

/*
d_arg_internal_option_restarts
  CLI11's restart of the count at a separator, between items of a variable
size, while counting from the start.
*/
D_STATIC bool
d_arg_internal_option_restarts(
    const struct d_arg_option* _option,
    struct d_pack_text         _value,
    int                        _index
)
{
    return ( (_option->type_size_max > 1)                        &&
             (_option->type_size_max != _option->type_size_min)  &&
             (_index >= 0)                                       &&
             (d_arg_internal_option_is_separator(_value)) );
}

/*
d_arg_internal_option_position
  The index a validator sees: the position within an item for a multi-value
type, counted from the start; otherwise the count itself.
*/
D_STATIC int
d_arg_internal_option_position(
    const struct d_arg_option* _option,
    int                        _index
)
{
    return ( (_option->type_size_max > 1) &&
             (_index >= 0) )
           ? (_index % _option->type_size_max)
           : _index;
}

/*
d_arg_internal_option_fail_named
  Records a failure in CLI11's "<name>: <detail>" form, the name as
d_arg_option_name writes it: nothing for a hidden option.
*/
D_STATIC int
d_arg_internal_option_fail_named(
    const struct d_arg_option* _option,
    int                        _status,
    const char*                _detail
)
{
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;

    (void)d_arg_option_name(_option,
                            0u,
                            &name);

    const int status = d_arg_internal_fail(NULL,
                                           _option,
                                           _status,
                                           "%s: %s",
                                           (name.data != NULL) ? name.data : "",
                                           _detail);

    d_arg_buffer_release(&name);

    return status;
}

/*
d_arg_internal_option_rewrite
  Puts what the validators made of a value in its place, as CLI11's
_validate_results writes into the list; an unchanged value keeps its span.
*/
D_STATIC int
d_arg_internal_option_rewrite(
    struct d_arg_internal_option_pass* _pass,
    size_t                             _at
)
{
    const struct d_pack_text result = d_arg_buffer_text(&_pass->result);

    // an unchanged value stays as it is
    if (d_arg_internal_equal(_pass->list->items[_at],
                             result))
    {
        return D_ARG_OK;
    }

    const char* copy   = NULL;
    const int   status = d_arg_internal_string_copy(&_pass->option->owned,
                                                    result,
                                                    &copy);

    // the copy takes the value's place
    if (status == D_ARG_OK)
    {
        _pass->list->items[_at] = d_arg_internal_span(copy,
                                                      result.length);
    }

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail(NULL,
                                 _pass->option,
                                 status,
                                 NULL);
}

/*
d_arg_internal_option_validate_one
  One value's judgement: an error a validator stopped with is recorded as
raised, a rejection as CLI11's ValidationError, and a value that passes is
replaced by what the validators made of it.
*/
D_STATIC int
d_arg_internal_option_validate_one(
    struct d_arg_internal_option_pass* _pass,
    size_t                             _at,
    int                                _position
)
{
    d_arg_buffer_clear(&_pass->message);
    d_arg_buffer_clear(&_pass->result);

    const int status = d_arg_internal_option_judge(_pass->option,
                                                   _pass->list->items[_at],
                                                   _position,
                                                   &_pass->message,
                                                   &_pass->result);

    // an error, as the validator raised it
    if (status != D_ARG_OK)
    {
        return d_arg_internal_fail(NULL,
                                   _pass->option,
                                   status,
                                   (_pass->message.length > 0u) ? "%s" : NULL,
                                   _pass->message.data);
    }

    // a rejection
    if (_pass->message.length > 0u)
    {
        return d_arg_internal_option_fail_named(_pass->option,
                                                D_ARG_ERROR_VALIDATION,
                                                _pass->message.data);
    }

    return d_arg_internal_option_rewrite(_pass,
                                         _at);
}

/*
d_arg_internal_option_validate
  CLI11's _validate_results: each value judged at the position CLI11 gives
it, until one fails.
*/
int
d_arg_internal_option_validate(
    struct d_arg_option*    _option,
    struct d_arg_text_list* _list
)
{
    // nothing to run without validators
    if (_option->validators.count == 0u)
    {
        return D_ARG_OK;
    }

    struct d_arg_internal_option_pass pass   = { _option,
                                                 _list,
                                                 D_ARG_BUFFER_INIT,
                                                 D_ARG_BUFFER_INIT };
    int                               index  =
        d_arg_internal_option_first_index(_option,
                                          _list->count);
    int                               status = D_ARG_OK;

    // each value at its position
    for (size_t at = 0u;
         (status == D_ARG_OK) && (at < _list->count);
         ++at)
    {
        // a separator between items of a variable size restarts the count
        if (d_arg_internal_option_restarts(_option,
                                           _list->items[at],
                                           index))
        {
            index = 0;

            continue;
        }

        status = d_arg_internal_option_validate_one(
                     &pass,
                     at,
                     d_arg_internal_option_position(_option,
                                                    index));
        index++;
    }

    d_arg_buffer_release(&pass.message);
    d_arg_buffer_release(&pass.result);

    return status;
}

//==============================================================================
// 3.  REDUCING
//==============================================================================

/*
d_arg_internal_option_reverse
  Turns a list of spans around in place.
*/
D_STATIC void
d_arg_internal_option_reverse(
    struct d_arg_text_list* _list
)
{
    size_t low  = 0u;
    size_t high = _list->count;

    // swap from both ends toward the middle
    while ((high - low) > 1u)
    {
        const struct d_pack_text swap = _list->items[low];

        high--;
        _list->items[low]  = _list->items[high];
        _list->items[high] = swap;
        low++;
    }

    return;
}

/*
d_arg_internal_option_trim_to
  CLI11's TakeLast, Reverse and TakeFirst: of more values than the option
takes -- at least one -- the last or the first ones; Reverse also takes the
last ones, all of them, when it keeps several, and turns them around.
*/
D_STATIC int
d_arg_internal_option_trim_to(
    const struct d_arg_option*    _option,
    struct d_arg_text_list*       _out,
    const struct d_arg_text_list* _original
)
{
    const int    maximum = d_arg_option_items_expected_max(_option);
    const size_t count   = _original->count;
    const size_t limit   = (maximum > 1) ? (size_t)maximum : 1u;
    const size_t keep    = (limit < count) ? limit : count;
    const int    policy  = _option->multi_option_policy;
    int          status  = D_ARG_OK;

    // the first ones
    if (policy == (int)D_ARG_MULTI_TAKE_FIRST)
    {
        return (count != keep)
               ? d_arg_internal_text_append(_out,
                                            _original,
                                            0u,
                                            keep)
               : D_ARG_OK;
    }

    // the last ones
    if ( (count != keep) ||
         ( (policy == (int)D_ARG_MULTI_REVERSE) &&
           (keep > 1u) ) )
    {
        status = d_arg_internal_text_append(_out,
                                            _original,
                                            count - keep,
                                            keep);
    }

    // turned around
    if ( (status == D_ARG_OK) &&
         (policy == (int)D_ARG_MULTI_REVERSE) )
    {
        d_arg_internal_option_reverse(_out);
    }

    return status;
}

/*
d_arg_internal_option_join
  CLI11's join: the values with the delimiter between them, then -- CLI11's
own rule -- a delimiter that ends the text dropped, as when the last value
is empty or ends with one.
*/
D_STATIC void
d_arg_internal_option_join(
    const struct d_arg_text_list* _values,
    char                          _delimiter,
    struct d_arg_buffer*          _out
)
{
    const size_t start = _out->length;

    // each value, after a delimiter but the first
    for (size_t index = 0u; index < _values->count; ++index)
    {
        if (index > 0u)
        {
            (void)d_arg_buffer_append_char(_out,
                                           _delimiter);
        }

        (void)d_arg_buffer_append_text(_out,
                                       _values->items[index]);
    }

    // a delimiter at the end is dropped
    if ( (_out->length > start) &&
         (_out->data[_out->length - 1u] == _delimiter) )
    {
        d_arg_buffer_truncate(_out,
                              _out->length - 1u);
    }

    return;
}

/*
d_arg_internal_option_join_into
  CLI11's Join: several values become one, joined by the delimiter, or by
`\n` without one; a single value stands as it is.
*/
D_STATIC int
d_arg_internal_option_join_into(
    struct d_arg_option*          _option,
    struct d_arg_text_list*       _out,
    const struct d_arg_text_list* _original
)
{
    // a single value stands
    if (_original->count <= 1u)
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer joined = D_ARG_BUFFER_INIT;

    d_arg_internal_option_join(_original,
                               (_option->delimiter != '\0')
                               ? _option->delimiter
                               : '\n',
                               &joined);

    const int status = d_arg_internal_option_push_built(_option,
                                                        _out,
                                                        &joined);

    d_arg_buffer_release(&joined);

    return status;
}

/*
d_arg_internal_option_sum_into
  CLI11's Sum: the values become their sum, as d_arg_sum_values writes it.
*/
D_STATIC int
d_arg_internal_option_sum_into(
    struct d_arg_option*          _option,
    struct d_arg_text_list*       _out,
    const struct d_arg_text_list* _original
)
{
    struct d_arg_buffer sum    = D_ARG_BUFFER_INIT;
    int                 status = d_arg_sum_values(_original->items,
                                                  _original->count,
                                                  _option->integer_rule,
                                                  &sum);

    // the sum is the one value
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_push_built(_option,
                                                  _out,
                                                  &sum);
    }

    d_arg_buffer_release(&sum);

    return status;
}

/*
d_arg_internal_option_mismatch
  CLI11's ArgumentMismatch::AtLeast and AtMost: "<name>: At least <n>
required but received <count>", and "At most" likewise.
*/
D_STATIC int
d_arg_internal_option_mismatch(
    const struct d_arg_option* _option,
    const char*                _bound,
    size_t                     _number,
    size_t                     _count
)
{
    struct d_arg_buffer detail = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_printf(&detail,
                              "%s %zu required but received %zu",
                              _bound,
                              _number,
                              _count);

    const int status = d_arg_internal_option_fail_named(
                           _option,
                           D_ARG_ERROR_ARGUMENT_MISMATCH,
                           (detail.data != NULL) ? detail.data : "");

    d_arg_buffer_release(&detail);

    return status;
}

/*
d_arg_internal_option_check_count
  CLI11's Throw: too few values or too many is a mismatch, each bound at
least 1 -- but for `{}` followed by `%%`, an empty container, which passes
as the one value it stands for, as CLI11 lets it.
*/
D_STATIC int
d_arg_internal_option_check_count(
    const struct d_arg_option*    _option,
    struct d_arg_text_list*       _out,
    const struct d_arg_text_list* _original
)
{
    const int    minimum = d_arg_option_items_expected_min(_option);
    const int    maximum = d_arg_option_items_expected_max(_option);
    const size_t least   = (minimum > 0) ? (size_t)minimum : 1u;
    const size_t most    = (maximum > 0) ? (size_t)maximum : 1u;
    const size_t count   = _original->count;

    // too few
    if (count < least)
    {
        return d_arg_internal_option_mismatch(_option,
                                              "At least",
                                              least,
                                              count);
    }

    // enough, and not too many
    if (count <= most)
    {
        return D_ARG_OK;
    }

    // too many, but for the empty container
    return ( (count == 2u)                                   &&
             (most == 1u)                                    &&
             (d_arg_internal_equal_cstr(_original->items[0],
                                        "{}"))               &&
             (d_arg_internal_equal_cstr(_original->items[1],
                                        "%%")) )
           ? d_arg_internal_text_append(_out,
                                        _original,
                                        0u,
                                        count)
           : d_arg_internal_option_mismatch(_option,
                                            "At most",
                                            most,
                                            count);
}

/*
d_arg_internal_option_mark_empty
  CLI11's indicator of an empty container, for an option that needs
values: when the policy kept nothing of a lone `{}`, or kept a lone `{}`,
`%%` follows it, so that the conversion reads an empty container.
*/
D_STATIC int
d_arg_internal_option_mark_empty(
    const struct d_arg_option*    _option,
    struct d_arg_text_list*       _out,
    const struct d_arg_text_list* _original
)
{
    const struct d_pack_text empty     = d_arg_internal_span("{}",
                                                             2u);
    const struct d_pack_text separator = d_arg_internal_span("%%",
                                                             2u);
    int                      status    = D_ARG_OK;

    // only an option that needs values
    if (d_arg_option_items_expected_min(_option) <= 0)
    {
        return D_ARG_OK;
    }

    // nothing kept of a lone `{}`
    if ( (_out->count == 0u)      &&
         (_original->count == 1u) &&
         (d_arg_internal_equal(_original->items[0],
                               empty)) )
    {
        status = d_arg_internal_text_push(_out,
                                          empty);
    }
    // a lone `{}` kept, or none
    else if ( (_out->count != 1u) ||
              (!d_arg_internal_equal(_out->items[0],
                                     empty)) )
    {
        return D_ARG_OK;
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_text_push(_out,
                                      separator)
           : status;
}

/*
d_arg_internal_option_reduce
  CLI11's _reduce_results: `_out` emptied, filled by the option's policy,
and the empty-container indicator completed; a refused allocation is
recorded here, a mismatch where it was found.
*/
int
d_arg_internal_option_reduce(
    struct d_arg_option*          _option,
    struct d_arg_text_list*       _out,
    const struct d_arg_text_list* _original
)
{
    int status = D_ARG_OK;

    _out->count = 0u;

    switch (_option->multi_option_policy)
    {
        case D_ARG_MULTI_TAKE_ALL:
            break;

        case D_ARG_MULTI_TAKE_LAST:
        case D_ARG_MULTI_TAKE_FIRST:
        case D_ARG_MULTI_REVERSE:
            status = d_arg_internal_option_trim_to(_option,
                                                   _out,
                                                   _original);
            break;

        case D_ARG_MULTI_JOIN:
            status = d_arg_internal_option_join_into(_option,
                                                     _out,
                                                     _original);
            break;

        case D_ARG_MULTI_SUM:
            status = d_arg_internal_option_sum_into(_option,
                                                    _out,
                                                    _original);
            break;

        default:
            status = d_arg_internal_option_check_count(_option,
                                                       _out,
                                                       _original);
            break;
    }

    // the indicator, once the policy kept what it keeps
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_mark_empty(_option,
                                                  _out,
                                                  _original);
    }

    return (status == D_ARG_ERROR_OUT_OF_MEMORY)
           ? d_arg_internal_fail(NULL,
                                 _option,
                                 status,
                                 NULL)
           : status;
}

/*
d_arg_option_reduce
  The first half of CLI11's run_callback: new values validated, then the
values reduced into `processed` when they are not yet.
*/
int
d_arg_option_reduce(
    struct d_arg_option* _option
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int status = D_ARG_OK;

    // new values are validated first
    if (_option->state == (int)D_ARG_OPTION_STATE_PARSING)
    {
        status = d_arg_internal_option_validate(_option,
                                                &_option->results);

        if (status != D_ARG_OK)
        {
            return status;
        }

        _option->state = (int)D_ARG_OPTION_STATE_VALIDATED;
    }

    // then reduced, once
    if (_option->state < (int)D_ARG_OPTION_STATE_REDUCED)
    {
        status = d_arg_internal_option_reduce(_option,
                                              &_option->processed,
                                              &_option->results);

        if (status == D_ARG_OK)
        {
            _option->state = (int)D_ARG_OPTION_STATE_REDUCED;
        }
    }

    return status;
}

/*
d_arg_internal_option_results_text
  d_arg_internal_option_join of the values as given, with a comma.
*/
int
d_arg_internal_option_results_text(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    d_arg_internal_option_join(&_option->results,
                               ',',
                               _out);

    return (_out->failed)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}
