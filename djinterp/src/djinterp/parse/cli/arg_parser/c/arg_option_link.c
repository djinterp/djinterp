/*******************************************************************************
* djinterp [parse]                                             arg_option_link.c
*
* Options' links to other things, declared in arg_option.h (sections 4.6
* to 4.8).
*   The options an option needs and excludes, the validators it runs --
* CLI11's check, transform and each -- and the callback and bound variable
* its values are handed to.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_option_link.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator
#include "./arg_internal.h"  // failures, lists, spans


/*
d_arg_internal_option_check_link
  D_ARG_OK for two options of one parser; a link between two parsers'
options is refused and recorded, for no parse could honour it and its
pointers would outlive the other parser.
*/
D_STATIC int
d_arg_internal_option_check_link(
    const struct d_arg_option* _option,
    const struct d_arg_option* _other
)
{
    return (d_arg_internal_parser_of(_option) ==
            d_arg_internal_parser_of(_other))
           ? D_ARG_OK
           : d_arg_internal_fail_definition(
                 NULL,
                 _option,
                 D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                 "options of different parsers cannot be linked");
}

/*
d_arg_option_needs
  CLI11's needs(Option*): a set insert, ignoring the option itself.
*/
int
d_arg_option_needs(
    struct d_arg_option* _option,
    struct d_arg_option* _other
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_other) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // an option needing itself is ignored
    if (_other == _option)
    {
        return D_ARG_OK;
    }

    const int link = d_arg_internal_option_check_link(_option,
                                                      _other);

    // one of another parser is refused
    if (link != D_ARG_OK)
    {
        return link;
    }

    const int status = d_arg_internal_option_insert(&_option->needs,
                                                    _other);

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail_definition(NULL,
                                            _option,
                                            status,
                                            NULL);
}

/*
d_arg_option_excludes
  CLI11's excludes(Option*), recorded on both options so that help shows it
on both; a failure to record the second half undoes the first.
*/
int
d_arg_option_excludes(
    struct d_arg_option* _option,
    struct d_arg_option* _other
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_other) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // an option cannot exclude itself
    if (_other == _option)
    {
        return d_arg_internal_fail_definition(
                   NULL,
                   _option,
                   D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                   "and option cannot exclude itself");
    }

    const int link = d_arg_internal_option_check_link(_option,
                                                      _other);

    // one of another parser is refused
    if (link != D_ARG_OK)
    {
        return link;
    }

    const bool had    = d_arg_internal_option_has(&_option->excludes,
                                                  _other);
    int        status = d_arg_internal_option_insert(&_option->excludes,
                                                     _other);

    // then the other half
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_option_insert(&_other->excludes,
                                              _option);

        // undo the first half when the second could not be recorded
        if ( (status != D_ARG_OK) &&
             (!had) )
        {
            (void)d_arg_internal_option_remove(&_option->excludes,
                                               _other);
        }
    }

    return (status == D_ARG_OK)
           ? D_ARG_OK
           : d_arg_internal_fail_definition(NULL,
                                            _option,
                                            status,
                                            NULL);
}

/*
d_arg_internal_option_named
  The option a needs or an excludes by name refers to, found as CLI11's
get_option_no_throw finds it from the option's command, then -- for an
option in an option group -- from the command the group belongs to, where
CLI11 looks for an option moved into the group, since its option keeps the
command it was made on; CLI11's MissingOption when neither has it.
*/
D_STATIC struct d_arg_option*
d_arg_internal_option_named(
    struct d_arg_option* _option,
    const char*          _name,
    int*                 _status
)
{
    struct d_arg_option* other = d_arg_command_option(_option->parent,
                                                      _name);

    // an option group's option looks beyond the group
    if ( (!other) &&
         (d_arg_command_is_option_group(_option->parent)) )
    {
        other = d_arg_command_option(
                    d_arg_internal_fallthrough_parent(_option->parent),
                    _name);
    }

    // a name nothing matches
    if (!other)
    {
        *_status = d_arg_internal_fail_definition(
                       NULL,
                       _option,
                       D_ARG_ERROR_INCORRECT_CONSTRUCTION,
                       "Option %s is not defined",
                       _name);

        return NULL;
    }

    *_status = D_ARG_OK;

    return other;
}

/*
d_arg_option_needs_name
  The option found by name, then d_arg_option_needs.
*/
int
d_arg_option_needs_name(
    struct d_arg_option* _option,
    const char*          _name
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_name) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int                        status = D_ARG_OK;
    struct d_arg_option* const other  = d_arg_internal_option_named(_option,
                                                                    _name,
                                                                    &status);

    return (other != NULL)
           ? d_arg_option_needs(_option,
                                other)
           : status;
}

/*
d_arg_option_excludes_name
  The option found by name, then d_arg_option_excludes.
*/
int
d_arg_option_excludes_name(
    struct d_arg_option* _option,
    const char*          _name
)
{
    // validate the parameters
    if ( (!_option) ||
         (!_name) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    int                        status = D_ARG_OK;
    struct d_arg_option* const other  = d_arg_internal_option_named(_option,
                                                                    _name,
                                                                    &status);

    return (other != NULL)
           ? d_arg_option_excludes(_option,
                                   other)
           : status;
}

/*
d_arg_option_remove_needs
  The first occurrence, as CLI11 removes it.
*/
bool
d_arg_option_remove_needs(
    struct d_arg_option*       _option,
    const struct d_arg_option* _other
)
{
    return ( (_option != NULL) &&
             (d_arg_internal_option_remove(&_option->needs,
                                           _other)) );
}

/*
d_arg_option_remove_excludes
  This option's half only, as CLI11 removes it.
*/
bool
d_arg_option_remove_excludes(
    struct d_arg_option*       _option,
    const struct d_arg_option* _other
)
{
    return ( (_option != NULL) &&
             (d_arg_internal_option_remove(&_option->excludes,
                                           _other)) );
}

/*
d_arg_internal_option_name_validator
  Gives a validator its name before it is stored; a refused name releases
the validator, which is the option's to dispose of from the call on.
*/
D_STATIC int
d_arg_internal_option_name_validator(
    struct d_arg_option*    _option,
    struct d_arg_validator* _validator,
    const char*             _name
)
{
    // only a name given is set
    if ( (_name == NULL) ||
         (_name[0] == '\0') )
    {
        return D_ARG_OK;
    }

    const int status = d_arg_validator_set_name(_validator,
                                                _name);

    // a refused name releases the validator
    if (status != D_ARG_OK)
    {
        d_arg_validator_release(_validator);

        return d_arg_internal_fail_definition(NULL,
                                              _option,
                                              status,
                                              NULL);
    }

    return D_ARG_OK;
}

/*
d_arg_internal_option_take_validator
  What check and transform share: a failed validator becomes the option's
definition error, the name is given, and the validator is stored first or
last; the values return to PARSING so that they are validated again.
*/
D_STATIC int
d_arg_internal_option_take_validator(
    struct d_arg_option*   _option,
    struct d_arg_validator _validator,
    const char*            _name,
    bool                   _first
)
{
    // a failed validator is a failed definition
    if (_validator.status != D_ARG_OK)
    {
        const int failed = d_arg_internal_fail_definition(
                               NULL,
                               _option,
                               _validator.status,
                               (_validator.description != NULL) ? "%s" : NULL,
                               _validator.description);

        d_arg_validator_release(&_validator);

        return failed;
    }

    int status = d_arg_internal_option_name_validator(_option,
                                                      &_validator,
                                                      _name);

    // the list releases what it cannot take
    if (status == D_ARG_OK)
    {
        status = _first
                 ? d_arg_internal_validator_insert(&_option->validators,
                                                   _validator)
                 : d_arg_internal_validator_push(&_option->validators,
                                                 _validator);
        status = (status == D_ARG_OK)
                 ? D_ARG_OK
                 : d_arg_internal_fail_definition(NULL,
                                                  _option,
                                                  status,
                                                  NULL);
    }

    // a stored validator sends the values back to be validated
    if (status == D_ARG_OK)
    {
        _option->state = (int)D_ARG_OPTION_STATE_PARSING;
    }

    return status;
}

/*
d_arg_option_check
  CLI11's check(Validator, name): made non-modifying, then stored last.
*/
int
d_arg_option_check(
    struct d_arg_option*   _option,
    struct d_arg_validator _validator,
    const char*            _name
)
{
    // validate the parameters
    if (!_option)
    {
        d_arg_validator_release(&_validator);

        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    d_arg_validator_set_modifying(&_validator,
                                  false);

    return d_arg_internal_option_take_validator(_option,
                                                _validator,
                                                _name,
                                                false);
}

/*
d_arg_option_transform
  CLI11's transform(Validator, description, name): the description fixed
when given, then stored first.
*/
int
d_arg_option_transform(
    struct d_arg_option*   _option,
    struct d_arg_validator _validator,
    const char*            _name,
    const char*            _description
)
{
    // validate the parameters
    if (!_option)
    {
        d_arg_validator_release(&_validator);

        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a description given replaces the validator's own
    if ( (_validator.status == D_ARG_OK) &&
         (_description != NULL) )
    {
        (void)d_arg_validator_set_description(&_validator,
                                              _description);
    }

    return d_arg_internal_option_take_validator(_option,
                                                _validator,
                                                _name,
                                                true);
}

// d_arg_internal_option_each_context
//   struct: what an each() validator keeps: the caller's function, its
// context, and the function that frees the context.
struct d_arg_internal_option_each_context
{
    d_arg_each_fn    each;
    void*            context;
    d_arg_release_fn release;
};

/*
d_arg_internal_option_each_validate
  Runs the caller's function on the value, watching the parser's report: a
status other than D_ARG_OK rejects the value with the message the function
recorded through d_arg_option_fail, or the status's own, and is returned --
so a validation error stands as the rejection CLI11 catches, and any other
ends the parse as CLI11 lets anything else an each() function throws.
*/
D_STATIC int
d_arg_internal_option_each_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_option_each_context* const each =
        (const struct d_arg_internal_option_each_context*)_validator->data;
    const struct d_arg_option* const option = _validation->option;
    struct d_arg_internal_watch      watch  = { NULL,
                                                D_ARG_ERROR_INIT };

    // without a function there is nothing to run
    if ( (!each) ||
         (!each->each) )
    {
        return D_ARG_OK;
    }

    d_arg_internal_watch_begin(&watch,
                               d_arg_internal_parser_of(option));

    const int  status   = each->each(option,
                                     _validation->value,
                                     each->context);
    const bool recorded = d_arg_internal_watch_end(&watch,
                                                   status);

    // the value passes
    if (status == D_ARG_OK)
    {
        return D_ARG_OK;
    }

    const int rejected = d_arg_validation_reject(
                             _validation,
                             recorded
                             ? d_arg_error_message(&watch.parser->error)
                             : d_arg_status_name(status));

    return (rejected != D_ARG_OK) ? rejected : status;
}

/*
d_arg_internal_option_each_release
  Frees the caller's context through its function, then the record.
*/
D_STATIC void
d_arg_internal_option_each_release(
    void* _context
)
{
    struct d_arg_internal_option_each_context* const each =
        (struct d_arg_internal_option_each_context*)_context;

    // nothing to free without a record
    if (!each)
    {
        return;
    }

    // the caller's context, when it is owned
    if ( (each->release != NULL) &&
         (each->context != NULL) )
    {
        each->release(each->context);
    }

    free(each);

    return;
}

/*
d_arg_internal_option_each_copy
  Duplicates the record when the caller's context is borrowed; an owned
context cannot be duplicated, so neither can the validator.
*/
D_STATIC void*
d_arg_internal_option_each_copy(
    const void* _context
)
{
    const struct d_arg_internal_option_each_context* const each =
        (const struct d_arg_internal_option_each_context*)_context;

    // an owned context has no copy
    if ( (!each) ||
         (each->release != NULL) )
    {
        return NULL;
    }

    struct d_arg_internal_option_each_context* const copy =
        malloc(sizeof(*copy));

    // check if memory allocation was successful
    if (copy != NULL)
    {
        *copy = *each;
    }

    return copy;
}

/*
d_arg_internal_option_each_free
  Frees a caller's context that never reached a validator, through its
release function, as the call promises to dispose of it.
*/
D_STATIC void
d_arg_internal_option_each_free(
    void*            _context,
    d_arg_release_fn _release
)
{
    // only an owned context is freed
    if ( (_release != NULL) &&
         (_context != NULL) )
    {
        _release(_context);
    }

    return;
}

/*
d_arg_internal_option_each_make
  The validator an each() runs as: the record around the caller's function,
owned by the validator from the start, so every failure frees it once.
*/
D_STATIC struct d_arg_validator
d_arg_internal_option_each_make(
    d_arg_each_fn    _each,
    void*            _context,
    d_arg_release_fn _release
)
{
    struct d_arg_internal_option_each_context* const each =
        malloc(sizeof(*each));

    // check if memory allocation was successful
    if (!each)
    {
        d_arg_internal_option_each_free(_context,
                                        _release);

        return d_arg_validator_custom(NULL,
                                      NULL,
                                      NULL);
    }

    each->each    = _each;
    each->context = _context;
    each->release = _release;

    struct d_arg_validator validator = d_arg_validator_custom(
                                           d_arg_internal_option_each_validate,
                                           each,
                                           NULL);

    d_arg_validator_own_context(&validator,
                                d_arg_internal_option_each_copy,
                                d_arg_internal_option_each_release);

    return validator;
}

/*
d_arg_option_each
  CLI11's each(): a validator with no description around the function,
stored last and left modifying, as CLI11 leaves it.
*/
int
d_arg_option_each(
    struct d_arg_option* _option,
    d_arg_each_fn        _each,
    void*                _context,
    d_arg_release_fn     _release
)
{
    // validate the parameters; the context is ours to free either way
    if ( (!_option) ||
         (!_each) )
    {
        d_arg_internal_option_each_free(_context,
                                        _release);

        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_validator validator = d_arg_internal_option_each_make(
                                           _each,
                                           _context,
                                           _release);

    // a record that could not be made is a refused allocation
    if (validator.data == NULL)
    {
        d_arg_validator_release(&validator);

        return d_arg_internal_fail_definition(NULL,
                                              _option,
                                              D_ARG_ERROR_OUT_OF_MEMORY,
                                              NULL);
    }

    return d_arg_internal_option_take_validator(_option,
                                                validator,
                                                NULL,
                                                false);
}

/*
d_arg_option_validator
  CLI11's get_validator(name): the first validator whose name is the one
asked for -- an empty name matches an unnamed validator -- else, for an
empty name, the first validator.
*/
struct d_arg_validator*
d_arg_option_validator(
    struct d_arg_option* _option,
    const char*          _name
)
{
    // no option has no validators
    if (!_option)
    {
        return NULL;
    }

    const struct d_pack_text name = d_arg_internal_text_of(_name);

    // the first one so named
    for (size_t index = 0u; index < _option->validators.count; ++index)
    {
        if (d_arg_internal_equal(
                name,
                d_arg_internal_text_of(_option->validators.items[index].name)))
        {
            return &_option->validators.items[index];
        }
    }

    // the first, for an empty name
    if ( (name.length == 0u) &&
         (_option->validators.count > 0u) )
    {
        return &_option->validators.items[0];
    }

    return NULL;
}

/*
d_arg_option_validator_at
  CLI11's get_validator(int).
*/
struct d_arg_validator*
d_arg_option_validator_at(
    struct d_arg_option* _option,
    size_t               _index
)
{
    return ( (_option != NULL) &&
             (_index < _option->validators.count) )
           ? &_option->validators.items[_index]
           : NULL;
}

/*
d_arg_option_set_callback
  As d_arg_option_set_type_name_fn, for the callback.
*/
void
d_arg_option_set_callback(
    struct d_arg_option* _option,
    d_arg_results_fn     _callback,
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

    d_arg_internal_context_replace(&_option->callback_context,
                                   &_option->callback_release,
                                   _context,
                                   _release);
    _option->callback = _callback;

    return;
}

/*
d_arg_internal_option_write_target
  The default function a bound variable is: its value as its type writes
it, or nothing for a type that cannot write one, as CLI11's checked_to_string
gives the empty string.
*/
D_STATIC int
d_arg_internal_option_write_target(
    const struct d_arg_option* _option,
    struct d_pack_sink         _sink,
    void*                      _context
)
{
    (void)_context;

    // a type without a format writes nothing
    if ( (_option->type == NULL)         ||
         (_option->type->format == NULL) ||
         (_option->target == NULL) )
    {
        return D_ARG_OK;
    }

    return d_arg_format(_option->type,
                        _option->target,
                        _sink);
}

/*
d_arg_internal_option_attach
  The variable, its type, and the variable as the default function; NULL
detaches the variable and drops it as the default function.
*/
int
d_arg_internal_option_attach(
    struct d_arg_option*     _option,
    void*                    _target,
    const struct d_arg_type* _type
)
{
    // NULL detaches
    if (!_target)
    {
        _option->type   = NULL;
        _option->target = NULL;

        // the variable is no longer the default function
        if (_option->default_fn == d_arg_internal_option_write_target)
        {
            d_arg_option_set_default_fn(_option,
                                        NULL,
                                        NULL,
                                        NULL);
        }

        return D_ARG_OK;
    }

    // a variable needs a type that converts
    if ( (!_type) ||
         (!_type->convert) )
    {
        return d_arg_internal_fail_definition(NULL,
                                              _option,
                                              D_ARG_ERROR_BAD_ARGUMENT,
                                              NULL);
    }

    _option->type   = _type;
    _option->target = _target;
    d_arg_option_set_default_fn(_option,
                                d_arg_internal_option_write_target,
                                NULL,
                                NULL);

    return D_ARG_OK;
}

/*
d_arg_option_bind
  What CLI11's add_option(name, variable) does after making the option: the
variable attached as d_arg_internal_option_attach attaches it, then the
type's sizes and expected count, and the callback run for defaults.
*/
int
d_arg_option_bind(
    struct d_arg_option*     _option,
    void*                    _target,
    const struct d_arg_type* _type
)
{
    // validate the parameters
    if (!_option)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const int status = d_arg_internal_option_attach(_option,
                                                    _target,
                                                    _type);

    // detached, or refused
    if ( (status != D_ARG_OK) ||
         (!_target) )
    {
        return status;
    }

    d_arg_option_set_type_size_range(_option,
                                     _type->type_size_min,
                                     _type->type_size_max);
    d_arg_option_set_expected(_option,
                              _type->expected);
    _option->flags |= D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT;

    return D_ARG_OK;
}
