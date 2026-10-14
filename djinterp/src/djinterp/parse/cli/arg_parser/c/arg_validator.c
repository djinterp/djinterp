/*******************************************************************************
* djinterp [parse]                                               arg_validator.c
*
* Validators, declared in arg_validator.h.
*   A value's validation -- its replacement and its rejection -- then the
* validator record's lifetime, settings and use; CLI11's composition with
* AND, OR and NOT, word for word; custom validators; and CLI11's default
* validators: Range, PositiveNumber, NonNegativeNumber and EscapedString.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_validator.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // corresponding header
// std
#include <float.h>   // DBL_MAX, DBL_MIN, DBL_EPSILON
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memchr, memcpy
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_convert_context, d_arg_type_builtin
#include "./arg_internal.h"  // spans, copies, numbers, scratch, escapes


//==============================================================================
// 1.  VALIDATION
//==============================================================================

/*
d_arg_validation_init
  The value borrowed, both buffers empty, no option, and the configured
integer rule.
*/
void
d_arg_validation_init(
    struct d_arg_validation* _validation,
    struct d_pack_text       _value
)
{
    // nothing to start
    if (!_validation)
    {
        return;
    }

    _validation->value = _value;
    d_arg_buffer_init(&_validation->storage);
    d_arg_buffer_init(&_validation->message);
    _validation->option       = NULL;
    _validation->integer_rule = (int)D_INTERNAL_ARG_PARSER_INTEGER_RULE;

    return;
}

/*
d_arg_validation_release
  Frees both buffers; the value, which may have pointed into the storage,
becomes the empty text.
*/
void
d_arg_validation_release(
    struct d_arg_validation* _validation
)
{
    // nothing to release
    if (!_validation)
    {
        return;
    }

    d_arg_buffer_release(&_validation->storage);
    d_arg_buffer_release(&_validation->message);
    _validation->value = d_arg_internal_span("",
                                             0u);

    return;
}

/*
d_arg_validation_replace
  The new value is copied into a buffer of its own before the old storage
goes, since it may lie inside it.
*/
int
d_arg_validation_replace(
    struct d_arg_validation* _validation,
    struct d_pack_text       _value
)
{
    // validate the parameters
    if ( (!_validation) ||
         ( (_value.data == NULL) &&
           (_value.length > 0u) ) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_buffer copy   = D_ARG_BUFFER_INIT;
    const int           status = d_arg_buffer_append_text(&copy,
                                                          _value);

    // the old value stands when the copy could not be made
    if (status != D_ARG_OK)
    {
        d_arg_buffer_release(&copy);

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_arg_buffer_release(&_validation->storage);
    _validation->storage = copy;
    _validation->value   = d_arg_buffer_text(&_validation->storage);

    return D_ARG_OK;
}

/*
d_arg_validation_reject
  Replaces whatever reason was recorded; an empty one records nothing, as
CLI11's empty string is a pass.
*/
int
d_arg_validation_reject(
    struct d_arg_validation* _validation,
    const char*              _message
)
{
    // validate the parameters
    if (!_validation)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // no reason is no rejection
    if ( (!_message) ||
         (_message[0] == '\0') )
    {
        return D_ARG_OK;
    }

    d_arg_buffer_clear(&_validation->message);

    return d_arg_buffer_append_cstr(&_validation->message,
                                    _message);
}

/*
d_arg_validation_rejectf
  As d_arg_validation_reject, the reason formatted straight into the
message; an empty expansion records nothing.
*/
int
d_arg_validation_rejectf(
    struct d_arg_validation* _validation,
    const char*              _format,
    ...
)
{
    // validate the parameters
    if ( (!_validation) ||
         (!_format) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    va_list arguments;

    d_arg_buffer_clear(&_validation->message);
    va_start(arguments,
             _format);

    const int status = d_arg_internal_vformat(&_validation->message,
                                              _format,
                                              arguments);

    va_end(arguments);

    return status;
}

/*
d_arg_internal_validator_reject
  The three parts appended in turn, the text as a span so that its bytes
survive whole; a failed append is reported by the last, since a failed
buffer refuses every append after it.
*/
int
d_arg_internal_validator_reject(
    struct d_arg_validation* _validation,
    const char*              _before,
    struct d_pack_text       _text,
    const char*              _after
)
{
    d_arg_buffer_clear(&_validation->message);
    (void)d_arg_buffer_append_cstr(&_validation->message,
                                   _before);
    (void)d_arg_buffer_append_text(&_validation->message,
                                   _text);

    return d_arg_buffer_append_cstr(&_validation->message,
                                    _after);
}

/*
d_arg_internal_validator_stop
  A rejection returned as the validation error CLI11 throws, which the
option's judging reads back as the rejection and a composite stops at.
*/
int
d_arg_internal_validator_stop(
    struct d_arg_validation* _validation,
    const char*              _before,
    struct d_pack_text       _text,
    const char*              _after
)
{
    const int status = d_arg_internal_validator_reject(_validation,
                                                       _before,
                                                       _text,
                                                       _after);

    return (status == D_ARG_OK)
           ? D_ARG_ERROR_VALIDATION
           : status;
}

/*
d_arg_internal_validator_replace
  The object written into a buffer of its own, which then replaces the
value; a writing that fails leaves the value as it was.
*/
int
d_arg_internal_validator_replace(
    struct d_arg_validation* _validation,
    const struct d_arg_type* _type,
    const void*              _object
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_number_write(_type,
                                                             _object,
                                                             &text);

    // the written object replaces the value
    if (status == D_ARG_OK)
    {
        status = d_arg_validation_replace(_validation,
                                          d_arg_buffer_text(&text));
    }

    d_arg_buffer_release(&text);

    return status;
}

//==============================================================================
// 2.  THE RECORD
//==============================================================================

// VALIDATOR_DEFAULT
//   constant: CLI11's default Validator -- active, modifying, applying to
// every value, passing every value, describing nothing.
static const struct d_arg_validator VALIDATOR_DEFAULT =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    { 0 },
    { 0 },
    NULL,
    NULL,
    -1,
    D_ARG_OK,
    D_ARG_VALIDATOR_ACTIVE | D_ARG_VALIDATOR_MODIFYING
};

/*
d_arg_internal_validator_failed
  A validator holding only a failure: its status and, when one is given and
can be kept, the reason as its description.
*/
struct d_arg_validator
d_arg_internal_validator_failed(
    int         _status,
    const char* _reason
)
{
    struct d_arg_validator failed = VALIDATOR_DEFAULT;

    failed.status      = _status;
    failed.description = (_reason != NULL)
                         ? d_arg_internal_copy_cstr(_reason)
                         : NULL;

    return failed;
}

/*
d_arg_validator_init
  A copy of the default validator.
*/
void
d_arg_validator_init(
    struct d_arg_validator* _validator
)
{
    // nothing to initialize
    if (!_validator)
    {
        return;
    }

    *_validator = VALIDATOR_DEFAULT;

    return;
}

/*
d_arg_validator_release
  An owned context through its release function, then both texts, then the
default validator in its place.
*/
void
d_arg_validator_release(
    struct d_arg_validator* _validator
)
{
    // nothing to release
    if (!_validator)
    {
        return;
    }

    // the context, when the validator owns it
    if ( (_validator->release != NULL) &&
         (_validator->data != NULL) )
    {
        _validator->release(_validator->data);
    }

    free(_validator->name);
    free(_validator->description);
    *_validator = VALIDATOR_DEFAULT;

    return;
}

/*
d_arg_internal_validator_raw_describe
  The description a validator has whatever its state, as CLI11's
desc_function_ gives it: the fixed text, else the worked-out one, else
nothing.
*/
D_STATIC int
d_arg_internal_validator_raw_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    // a fixed description
    if (_validator->description != NULL)
    {
        return d_arg_buffer_append_cstr(_out,
                                        _validator->description);
    }

    return (_validator->describe != NULL)
           ? _validator->describe(_validator,
                                  _out)
           : D_ARG_OK;
}

//==============================================================================
// 3.  COMPOSITES
//==============================================================================

// d_arg_internal_validator_pair
//   struct: what a composite owns: its operands -- the second unused, and
// as the default validator leaves it, for a NOT.
struct d_arg_internal_validator_pair
{
    struct d_arg_validator first;
    struct d_arg_validator second;
};

/*
d_arg_internal_validator_merge
  CLI11's join of two texts: "(first)<merger>(second)" when both are there,
else whichever is, else nothing.
*/
D_STATIC int
d_arg_internal_validator_merge(
    struct d_pack_text   _first,
    struct d_pack_text   _second,
    const char*          _merger,
    struct d_arg_buffer* _out
)
{
    // one of them, or neither
    if ( (_first.length == 0u) ||
         (_second.length == 0u) )
    {
        (void)d_arg_buffer_append_text(_out,
                                       _first);

        return d_arg_buffer_append_text(_out,
                                        _second);
    }

    (void)d_arg_buffer_append_char(_out,
                                   '(');
    (void)d_arg_buffer_append_text(_out,
                                   _first);
    (void)d_arg_buffer_append_char(_out,
                                   ')');
    (void)d_arg_buffer_append_cstr(_out,
                                   _merger);
    (void)d_arg_buffer_append_char(_out,
                                   '(');
    (void)d_arg_buffer_append_text(_out,
                                   _second);

    return d_arg_buffer_append_char(_out,
                                    ')');
}

/*
d_arg_internal_validator_operand
  Runs an operand through its function directly -- its own flags do not
apply inside a composite -- and moves its rejection out of the validation,
which then reads as passing again; an error it stops with is left where it
is, with its message, for the composite to stop with too.
*/
D_STATIC int
d_arg_internal_validator_operand(
    const struct d_arg_validator* _operand,
    struct d_arg_validation*      _validation,
    struct d_arg_buffer*          _message
)
{
    const int status = (_operand->validate != NULL)
                       ? _operand->validate(_operand,
                                            _validation)
                       : D_ARG_OK;

    // the rejection moves out
    if (status == D_ARG_OK)
    {
        *_message = _validation->message;
        d_arg_buffer_init(&_validation->message);
    }

    return status;
}

/*
d_arg_internal_validator_both
  Runs both operands of a composite, the second on whatever the first made
of the value, their rejections moved into `_first` and `_second`.
*/
D_STATIC int
d_arg_internal_validator_both(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation,
    struct d_arg_buffer*          _first,
    struct d_arg_buffer*          _second
)
{
    const struct d_arg_internal_validator_pair* const pair =
        (const struct d_arg_internal_validator_pair*)_validator->data;
    const int status = d_arg_internal_validator_operand(&pair->first,
                                                        _validation,
                                                        _first);

    return (status == D_ARG_OK)
           ? d_arg_internal_validator_operand(&pair->second,
                                              _validation,
                                              _second)
           : status;
}

/*
d_arg_internal_validator_and_validate
  CLI11's `a & b`: both run; both rejections in parentheses, or the one
there is.
*/
D_STATIC int
d_arg_internal_validator_and_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_buffer first  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer second = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_validator_both(_validator,
                                                               _validation,
                                                               &first,
                                                               &second);

    // the verdict, written where a rejection is read
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_validator_merge(d_arg_buffer_text(&first),
                                                d_arg_buffer_text(&second),
                                                " AND ",
                                                &_validation->message);
    }

    d_arg_buffer_release(&second);
    d_arg_buffer_release(&first);

    return status;
}

/*
d_arg_internal_validator_or_validate
  CLI11's `a | b`: both run; the value passes when either passes, and is
otherwise rejected with both reasons in parentheses.
*/
D_STATIC int
d_arg_internal_validator_or_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_buffer first  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer second = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_validator_both(_validator,
                                                               _validation,
                                                               &first,
                                                               &second);

    // rejected only when both rejected
    if ( (status == D_ARG_OK)     &&
         (first.length > 0u)      &&
         (second.length > 0u) )
    {
        status = d_arg_internal_validator_merge(d_arg_buffer_text(&first),
                                                d_arg_buffer_text(&second),
                                                " OR ",
                                                &_validation->message);
    }

    d_arg_buffer_release(&second);
    d_arg_buffer_release(&first);

    return status;
}

/*
d_arg_internal_validator_not_validate
  CLI11's `!a`: the operand runs, and its pass is the rejection "check
<description> succeeded improperly", its rejection a pass.
*/
D_STATIC int
d_arg_internal_validator_not_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_validator_pair* const pair =
        (const struct d_arg_internal_validator_pair*)_validator->data;
    struct d_arg_buffer rejection = D_ARG_BUFFER_INIT;
    int                 status    = d_arg_internal_validator_operand(
                                        &pair->first,
                                        _validation,
                                        &rejection);

    // a pass is the inversion's rejection
    if ( (status == D_ARG_OK) &&
         (rejection.length == 0u) )
    {
        (void)d_arg_buffer_append_cstr(&_validation->message,
                                       "check ");
        (void)d_arg_internal_validator_raw_describe(&pair->first,
                                                    &_validation->message);
        status = d_arg_buffer_append_cstr(&_validation->message,
                                          " succeeded improperly");
    }

    d_arg_buffer_release(&rejection);

    return status;
}

/*
d_arg_internal_validator_describe_pair
  CLI11's _merge_description: the operands' descriptions -- whatever their
state -- joined by `_merger`.
*/
D_STATIC int
d_arg_internal_validator_describe_pair(
    const struct d_arg_validator* _validator,
    const char*                   _merger,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_internal_validator_pair* const pair =
        (const struct d_arg_internal_validator_pair*)_validator->data;
    struct d_arg_buffer first  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer second = D_ARG_BUFFER_INIT;

    (void)d_arg_internal_validator_raw_describe(&pair->first,
                                                &first);
    (void)d_arg_internal_validator_raw_describe(&pair->second,
                                                &second);

    const int status = ( (first.failed) ||
                         (second.failed) )
                       ? D_ARG_ERROR_OUT_OF_MEMORY
                       : d_arg_internal_validator_merge(
                             d_arg_buffer_text(&first),
                             d_arg_buffer_text(&second),
                             _merger,
                             _out);

    d_arg_buffer_release(&second);
    d_arg_buffer_release(&first);

    return status;
}

/*
d_arg_internal_validator_and_describe
  The operands' descriptions joined by " AND ".
*/
D_STATIC int
d_arg_internal_validator_and_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_internal_validator_describe_pair(_validator,
                                                  " AND ",
                                                  _out);
}

/*
d_arg_internal_validator_or_describe
  The operands' descriptions joined by " OR ".
*/
D_STATIC int
d_arg_internal_validator_or_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_internal_validator_describe_pair(_validator,
                                                  " OR ",
                                                  _out);
}

/*
d_arg_internal_validator_not_describe
  "NOT " and the operand's description, when it has one.
*/
D_STATIC int
d_arg_internal_validator_not_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_internal_validator_pair* const pair =
        (const struct d_arg_internal_validator_pair*)_validator->data;
    struct d_arg_buffer operand = D_ARG_BUFFER_INIT;

    (void)d_arg_internal_validator_raw_describe(&pair->first,
                                                &operand);

    // NOT before a description; nothing without one
    if (operand.length > 0u)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "NOT ");
        (void)d_arg_buffer_append_text(_out,
                                       d_arg_buffer_text(&operand));
    }

    const int status = ( (operand.failed) ||
                         (_out->failed) )
                       ? D_ARG_ERROR_OUT_OF_MEMORY
                       : D_ARG_OK;

    d_arg_buffer_release(&operand);

    return status;
}

/*
d_arg_internal_validator_and_choices
  The values of the first operand that lists any.
*/
D_STATIC int
d_arg_internal_validator_and_choices(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    const struct d_arg_internal_validator_pair* const pair =
        (const struct d_arg_internal_validator_pair*)_validator->data;
    const struct d_arg_validator* const lister =
        (pair->first.choices != NULL)
        ? &pair->first
        : ( (pair->second.choices != NULL)
            ? &pair->second
            : NULL );

    return (lister != NULL)
           ? lister->choices(lister,
                             _each,
                             _context)
           : D_ARG_OK;
}

/*
d_arg_internal_validator_or_choices
  The values of both operands, the first's first.
*/
D_STATIC int
d_arg_internal_validator_or_choices(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    const struct d_arg_internal_validator_pair* const pair =
        (const struct d_arg_internal_validator_pair*)_validator->data;
    int status = (pair->first.choices != NULL)
                 ? pair->first.choices(&pair->first,
                                       _each,
                                       _context)
                 : D_ARG_OK;

    // then the second's
    if ( (status == D_ARG_OK) &&
         (pair->second.choices != NULL) )
    {
        status = pair->second.choices(&pair->second,
                                      _each,
                                      _context);
    }

    return status;
}

/*
d_arg_internal_validator_pair_release
  Both operands, then the pair.
*/
D_STATIC void
d_arg_internal_validator_pair_release(
    void* _context
)
{
    struct d_arg_internal_validator_pair* const pair =
        (struct d_arg_internal_validator_pair*)_context;

    // nothing to free without a pair
    if (!pair)
    {
        return;
    }

    d_arg_validator_release(&pair->first);
    d_arg_validator_release(&pair->second);
    free(pair);

    return;
}

/*
d_arg_internal_validator_pair_clone
  Copies both operands into a new pair; the first failure is returned, and
whatever was made of the copy released.
*/
D_STATIC int
d_arg_internal_validator_pair_clone(
    const struct d_arg_internal_validator_pair* _pair,
    void**                                      _out
)
{
    struct d_arg_internal_validator_pair copy   =
    {
        d_arg_validator_copy(&_pair->first),
        d_arg_validator_copy(&_pair->second)
    };
    const int                            status =
        (copy.first.status != D_ARG_OK)
        ? copy.first.status
        : copy.second.status;
    struct d_arg_internal_validator_pair* const pair =
        (status == D_ARG_OK)
        ? (struct d_arg_internal_validator_pair*)malloc(sizeof(*pair))
        : NULL;

    // the copy failed, or found no home
    if (!pair)
    {
        d_arg_validator_release(&copy.first);
        d_arg_validator_release(&copy.second);

        return (status != D_ARG_OK)
               ? status
               : D_ARG_ERROR_OUT_OF_MEMORY;
    }

    *pair = copy;
    *_out = pair;

    return D_ARG_OK;
}

/*
d_arg_internal_validator_pair_copy
  The pair's d_arg_copy_fn: a clone, or NULL; d_arg_validator_copy clones a
composite itself, to report the status a failed operand copy had.
*/
D_STATIC void*
d_arg_internal_validator_pair_copy(
    const void* _context
)
{
    void* copy = NULL;

    // the pair is cloned whole, or not at all
    if ( (_context == NULL) ||
         (d_arg_internal_validator_pair_clone(
              (const struct d_arg_internal_validator_pair*)_context,
              &copy) != D_ARG_OK) )
    {
        return NULL;
    }

    return copy;
}

/*
d_arg_internal_validator_kind_and
  The completion flags of a value both operands accept: a number when either
takes numbers; otherwise a path when either takes paths, and a directory
when either takes directories.
*/
D_STATIC uint32_t
d_arg_internal_validator_kind_and(
    uint32_t _first,
    uint32_t _second
)
{
    const uint32_t either = _first | _second;

    return ((either & D_ARG_VALIDATOR_NUMBER) != 0u)
           ? D_ARG_VALIDATOR_NUMBER
           : (either & (D_ARG_VALIDATOR_PATH | D_ARG_VALIDATOR_DIRECTORY));
}

/*
d_arg_internal_validator_kind_or
  The completion flags of a value either operand accepts: a number when both
take numbers; otherwise a path when either takes paths, and a directory
when every operand that takes paths takes directories.
*/
D_STATIC uint32_t
d_arg_internal_validator_kind_or(
    uint32_t _first,
    uint32_t _second
)
{
    const uint32_t path      = D_ARG_VALIDATOR_PATH;
    const uint32_t directory = D_ARG_VALIDATOR_DIRECTORY;
    const bool     any       = ( ((_first & path) != 0u) ||
                                 ((_second & path) != 0u) );
    const bool     all_dirs  = ( ( ((_first & path) == 0u) ||
                                   ((_first & directory) != 0u) ) &&
                                 ( ((_second & path) == 0u) ||
                                   ((_second & directory) != 0u) ) );

    // numbers on both sides
    if ((_first & _second & D_ARG_VALIDATOR_NUMBER) != 0u)
    {
        return D_ARG_VALIDATOR_NUMBER;
    }

    return (!any)
           ? 0u
           : (path | (all_dirs ? directory : 0u));
}

/*
d_arg_internal_validator_compose
  A composite over a pair: a failed operand fails it, with the first
failure's status and reason, and both operands are released; otherwise the
pair is its context, freed and copied with it. A NOT passes a NULL second.
*/
D_STATIC struct d_arg_validator
d_arg_internal_validator_compose(
    struct d_arg_validator  _first,
    struct d_arg_validator* _second,
    d_arg_validate_fn       _validate,
    d_arg_describe_fn       _describe,
    d_arg_choices_fn        _choices
)
{
    struct d_arg_validator               spare  = VALIDATOR_DEFAULT;
    struct d_arg_validator* const        second = (_second != NULL)
                                                  ? _second
                                                  : &spare;
    const struct d_arg_validator* const  failed =
        (_first.status != D_ARG_OK)
        ? &_first
        : ( (second->status != D_ARG_OK) ? second : NULL );
    struct d_arg_internal_validator_pair* const pair =
        (!failed)
        ? (struct d_arg_internal_validator_pair*)malloc(sizeof(*pair))
        : NULL;

    // a failed operand, or no room for the pair
    if (!pair)
    {
        struct d_arg_validator result = d_arg_internal_validator_failed(
            (failed != NULL) ? failed->status : D_ARG_ERROR_OUT_OF_MEMORY,
            (failed != NULL) ? failed->description : NULL);

        d_arg_validator_release(&_first);
        d_arg_validator_release(second);

        return result;
    }

    struct d_arg_validator composite = VALIDATOR_DEFAULT;

    pair->first        = _first;
    pair->second       = *second;
    composite.validate = _validate;
    composite.describe = _describe;
    composite.choices  = _choices;
    composite.data     = pair;
    composite.copy     = d_arg_internal_validator_pair_copy;
    composite.release  = d_arg_internal_validator_pair_release;

    return composite;
}

//==============================================================================
// 4.  LIFETIME AND SETTINGS
//==============================================================================

/*
d_arg_internal_validator_copy_context
  A context for a copy: a borrowed one is shared; a composite's is cloned,
its status kept; any other owned one goes through its copy function, and
without one cannot be copied.
*/
D_STATIC int
d_arg_internal_validator_copy_context(
    const struct d_arg_validator* _source,
    void**                        _data
)
{
    *_data = NULL;

    // a borrowed context, or none, is shared
    if ( (_source->release == NULL) ||
         (_source->data == NULL) )
    {
        *_data = _source->data;

        return D_ARG_OK;
    }

    // a composite clones its operands
    if (_source->copy == d_arg_internal_validator_pair_copy)
    {
        return d_arg_internal_validator_pair_clone(
                   (const struct d_arg_internal_validator_pair*)_source->data,
                   _data);
    }

    // an owned context needs its own way to be copied
    if (!_source->copy)
    {
        return D_ARG_ERROR_UNSUPPORTED;
    }

    *_data = _source->copy(_source->data);

    return (*_data != NULL)
           ? D_ARG_OK
           : D_ARG_ERROR_OUT_OF_MEMORY;
}

/*
d_arg_internal_validator_copy_text
  A copy of an owned text field; NULL stays NULL.
*/
D_STATIC int
d_arg_internal_validator_copy_text(
    const char* _text,
    char**      _copy
)
{
    *_copy = (_text != NULL)
             ? d_arg_internal_copy_cstr(_text)
             : NULL;

    return ( (_text != NULL) &&
             (*_copy == NULL) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

/*
d_arg_validator_copy
  Field by field: the context as d_arg_internal_validator_copy_context
copies it, then the two texts; a failure releases what was made and
returns a validator holding only the failure.
*/
struct d_arg_validator
d_arg_validator_copy(
    const struct d_arg_validator* _source
)
{
    // nothing to copy, or a failure to carry
    if ( (!_source) ||
         (_source->status != D_ARG_OK) )
    {
        return d_arg_internal_validator_failed(
                   (_source != NULL)
                   ? _source->status
                   : D_ARG_ERROR_BAD_ARGUMENT,
                   (_source != NULL) ? _source->description : NULL);
    }

    struct d_arg_validator copy   = *_source;
    void*                  data   = NULL;
    int                    status = d_arg_internal_validator_copy_context(
                                        _source,
                                        &data);

    copy.data        = data;
    copy.name        = NULL;
    copy.description = NULL;

    // the texts, once the context is copied
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_validator_copy_text(_source->name,
                                                    &copy.name);
    }

    if (status == D_ARG_OK)
    {
        status = d_arg_internal_validator_copy_text(_source->description,
                                                    &copy.description);
    }

    // a failure undoes the copy
    if (status != D_ARG_OK)
    {
        d_arg_validator_release(&copy);

        return d_arg_internal_validator_failed(status,
                                               NULL);
    }

    return copy;
}

/*
d_arg_internal_validator_set_text
  Replaces an owned text field with a copy, NULL clearing it; a refused copy
fails the validator and leaves the field as it was.
*/
D_STATIC int
d_arg_internal_validator_set_text(
    struct d_arg_validator* _validator,
    char**                  _field,
    const char*             _text
)
{
    char*     copy   = NULL;
    const int status = d_arg_internal_validator_copy_text(_text,
                                                          &copy);

    // the validator fails with the copy
    if (status != D_ARG_OK)
    {
        _validator->status = status;

        return status;
    }

    free(*_field);
    *_field = copy;

    return D_ARG_OK;
}

/*
d_arg_validator_set_name
  d_arg_internal_validator_set_text on the name.
*/
int
d_arg_validator_set_name(
    struct d_arg_validator* _validator,
    const char*             _name
)
{
    // validate the parameters
    if (!_validator)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_validator_set_text(_validator,
                                             &_validator->name,
                                             _name);
}

/*
d_arg_validator_set_description
  d_arg_internal_validator_set_text on the description; NULL lets the
describe function speak again.
*/
int
d_arg_validator_set_description(
    struct d_arg_validator* _validator,
    const char*             _description
)
{
    // validate the parameters
    if (!_validator)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_internal_validator_set_text(_validator,
                                             &_validator->description,
                                             _description);
}

/*
d_arg_internal_validator_switch
  Sets or clears one of a validator's flags.
*/
D_STATIC void
d_arg_internal_validator_switch(
    struct d_arg_validator* _validator,
    uint32_t                _flag,
    bool                    _on
)
{
    // nothing to switch without a validator
    if (!_validator)
    {
        return;
    }

    _validator->flags = _on
                        ? (_validator->flags | _flag)
                        : (_validator->flags & ~_flag);

    return;
}

/*
d_arg_validator_set_active
  CLI11's active(): the ACTIVE flag.
*/
void
d_arg_validator_set_active(
    struct d_arg_validator* _validator,
    bool                    _active
)
{
    d_arg_internal_validator_switch(_validator,
                                    D_ARG_VALIDATOR_ACTIVE,
                                    _active);

    return;
}

/*
d_arg_validator_set_modifying
  CLI11's non_modifying(), reversed: the MODIFYING flag.
*/
void
d_arg_validator_set_modifying(
    struct d_arg_validator* _validator,
    bool                    _modifying
)
{
    d_arg_internal_validator_switch(_validator,
                                    D_ARG_VALIDATOR_MODIFYING,
                                    _modifying);

    return;
}

/*
d_arg_validator_set_application_index
  CLI11's application_index(): the position the validator applies to.
*/
void
d_arg_validator_set_application_index(
    struct d_arg_validator* _validator,
    int                     _index
)
{
    // nothing to set without a validator
    if (!_validator)
    {
        return;
    }

    _validator->application_index = _index;

    return;
}

//==============================================================================
// 5.  USE
//==============================================================================

/*
d_arg_internal_validator_run_on_copy
  CLI11's non_modifying run: the function judges a validation of its own,
whose verdict -- rejection or stop -- is moved into `_validation` while
whatever it made of the value is dropped.
*/
D_STATIC int
d_arg_internal_validator_run_on_copy(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_validation copy = { { NULL, 0u },
                                     D_ARG_BUFFER_INIT,
                                     D_ARG_BUFFER_INIT,
                                     NULL,
                                     0 };

    d_arg_validation_init(&copy,
                          _validation->value);
    copy.option       = _validation->option;
    copy.integer_rule = _validation->integer_rule;

    const int status = _validator->validate(_validator,
                                            &copy);

    // the verdict comes back; the rewriting does not
    d_arg_buffer_release(&_validation->message);
    _validation->message = copy.message;
    d_arg_buffer_init(&copy.message);
    d_arg_validation_release(&copy);

    return status;
}

/*
d_arg_validator_run
  CLI11's operator(): a failed validator stops with its status; an inactive
one, or one with no function, passes; a modifying one works on the
validation itself, any other on a copy.
*/
int
d_arg_validator_run(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    // validate the parameters
    if ( (!_validator) ||
         (!_validation) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // a failed validator runs as its failure
    if (_validator->status != D_ARG_OK)
    {
        return _validator->status;
    }

    // inactive, or nothing to run
    if ( ((_validator->flags & D_ARG_VALIDATOR_ACTIVE) == 0u) ||
         (_validator->validate == NULL) )
    {
        return D_ARG_OK;
    }

    return ((_validator->flags & D_ARG_VALIDATOR_MODIFYING) != 0u)
           ? _validator->validate(_validator,
                                  _validation)
           : d_arg_internal_validator_run_on_copy(_validator,
                                                  _validation);
}

/*
d_arg_validator_describe
  CLI11's get_description: nothing while inactive -- or failed -- else the
raw description.
*/
int
d_arg_validator_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_validator) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // an inactive or failed validator describes nothing
    if ( (_validator->status != D_ARG_OK) ||
         ((_validator->flags & D_ARG_VALIDATOR_ACTIVE) == 0u) )
    {
        return D_ARG_OK;
    }

    const int status = d_arg_internal_validator_raw_describe(_validator,
                                                             _out);

    return ( (status == D_ARG_OK) &&
             (_out->failed) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

/*
d_arg_validator_each_choice
  The choices function, for an active validator that has one.
*/
int
d_arg_validator_each_choice(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    // validate the parameters
    if ( (!_validator) ||
         (!_each) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // nothing listed by an inactive or failed validator, or without a list
    if ( (_validator->status != D_ARG_OK)                       ||
         ((_validator->flags & D_ARG_VALIDATOR_ACTIVE) == 0u)   ||
         (_validator->choices == NULL) )
    {
        return D_ARG_OK;
    }

    return _validator->choices(_validator,
                               _each,
                               _context);
}

//==============================================================================
// 6.  COMPOSITION
//==============================================================================

/*
d_arg_validator_and
  A composite of both: active when both are, applying where the first does,
modifying, completing as d_arg_internal_validator_kind_and says.
*/
struct d_arg_validator
d_arg_validator_and(
    struct d_arg_validator _first,
    struct d_arg_validator _second
)
{
    const int      application = _first.application_index;
    const bool     active      =
        ( ((_first.flags & D_ARG_VALIDATOR_ACTIVE) != 0u) &&
          ((_second.flags & D_ARG_VALIDATOR_ACTIVE) != 0u) );
    const uint32_t kind        = d_arg_internal_validator_kind_and(
                                     _first.flags,
                                     _second.flags);
    struct d_arg_validator composite =
        d_arg_internal_validator_compose(
            _first,
            &_second,
            d_arg_internal_validator_and_validate,
            d_arg_internal_validator_and_describe,
            d_arg_internal_validator_and_choices);

    composite.application_index = application;
    composite.flags             = D_ARG_VALIDATOR_MODIFYING |
                                  (active ? D_ARG_VALIDATOR_ACTIVE : 0u) |
                                  kind;

    return composite;
}

/*
d_arg_validator_or
  As d_arg_validator_and, combining as CLI11's OR and completing as
d_arg_internal_validator_kind_or says.
*/
struct d_arg_validator
d_arg_validator_or(
    struct d_arg_validator _first,
    struct d_arg_validator _second
)
{
    const int      application = _first.application_index;
    const bool     active      =
        ( ((_first.flags & D_ARG_VALIDATOR_ACTIVE) != 0u) &&
          ((_second.flags & D_ARG_VALIDATOR_ACTIVE) != 0u) );
    const uint32_t kind        = d_arg_internal_validator_kind_or(
                                     _first.flags,
                                     _second.flags);
    struct d_arg_validator composite =
        d_arg_internal_validator_compose(
            _first,
            &_second,
            d_arg_internal_validator_or_validate,
            d_arg_internal_validator_or_describe,
            d_arg_internal_validator_or_choices);

    composite.application_index = application;
    composite.flags             = D_ARG_VALIDATOR_MODIFYING |
                                  (active ? D_ARG_VALIDATOR_ACTIVE : 0u) |
                                  kind;

    return composite;
}

/*
d_arg_validator_not
  The inversion: active when the operand is, applying where it does,
modifying, listing nothing and completing as nothing.
*/
struct d_arg_validator
d_arg_validator_not(
    struct d_arg_validator _operand
)
{
    const int              application = _operand.application_index;
    const bool             active      =
        ((_operand.flags & D_ARG_VALIDATOR_ACTIVE) != 0u);
    struct d_arg_validator composite   =
        d_arg_internal_validator_compose(
            _operand,
            NULL,
            d_arg_internal_validator_not_validate,
            d_arg_internal_validator_not_describe,
            NULL);

    composite.application_index = application;
    composite.flags             = D_ARG_VALIDATOR_MODIFYING |
                                  (active ? D_ARG_VALIDATOR_ACTIVE : 0u);

    return composite;
}

//==============================================================================
// 7.  CUSTOM VALIDATORS
//==============================================================================

/*
d_arg_validator_custom
  The default validator with the function, its borrowed context, and a copy
of the description.
*/
struct d_arg_validator
d_arg_validator_custom(
    d_arg_validate_fn _validate,
    void*             _context,
    const char*       _description
)
{
    struct d_arg_validator validator = VALIDATOR_DEFAULT;

    validator.validate = _validate;
    validator.data     = _context;

    // the description, when there is one to copy
    if ( (_description != NULL) &&
         (d_arg_validator_set_description(&validator,
                                          _description) != D_ARG_OK) )
    {
        return d_arg_internal_validator_failed(D_ARG_ERROR_OUT_OF_MEMORY,
                                               NULL);
    }

    return validator;
}

/*
d_arg_validator_own_context
  The two functions that free and copy the context.
*/
void
d_arg_validator_own_context(
    struct d_arg_validator* _validator,
    d_arg_copy_fn           _copy,
    d_arg_release_fn        _release
)
{
    // nothing to own without a validator
    if (!_validator)
    {
        return;
    }

    _validator->copy    = _copy;
    _validator->release = _release;

    return;
}

/*
d_arg_validator_set_describe
  The function the description is worked out with.
*/
void
d_arg_validator_set_describe(
    struct d_arg_validator* _validator,
    d_arg_describe_fn       _describe
)
{
    // nothing to set without a validator
    if (!_validator)
    {
        return;
    }

    _validator->describe = _describe;

    return;
}

/*
d_arg_validator_set_choices
  The function the accepted values are listed with.
*/
void
d_arg_validator_set_choices(
    struct d_arg_validator* _validator,
    d_arg_choices_fn        _choices
)
{
    // nothing to set without a validator
    if (!_validator)
    {
        return;
    }

    _validator->choices = _choices;

    return;
}

/*
d_arg_validator_set_value_kind
  The completion flags: a directory is a path too; a number is neither.
*/
void
d_arg_validator_set_value_kind(
    struct d_arg_validator* _validator,
    uint32_t                _kind
)
{
    const uint32_t all = D_ARG_VALIDATOR_PATH      |
                         D_ARG_VALIDATOR_DIRECTORY |
                         D_ARG_VALIDATOR_NUMBER;

    // nothing to set without a validator
    if (!_validator)
    {
        return;
    }

    _validator->flags &= ~all;

    // the one kind asked for
    if ((_kind & D_ARG_VALIDATOR_NUMBER) != 0u)
    {
        _validator->flags |= D_ARG_VALIDATOR_NUMBER;
    }
    else if ((_kind & D_ARG_VALIDATOR_DIRECTORY) != 0u)
    {
        _validator->flags |= D_ARG_VALIDATOR_PATH | D_ARG_VALIDATOR_DIRECTORY;
    }
    else
    {
        _validator->flags |= (_kind & D_ARG_VALIDATOR_PATH);
    }

    return;
}

//==============================================================================
// 8.  DEFAULT VALIDATORS
//==============================================================================

/*
d_arg_internal_validator_bounds
  A range's bounds, loaded through its type from where the validator keeps
them.
*/
void
d_arg_internal_validator_bounds(
    const struct d_arg_validator* _validator,
    struct d_arg_internal_number* _low,
    struct d_arg_internal_number* _high
)
{
    const struct d_arg_type* const type =
        (const struct d_arg_type*)_validator->parameter;

    (void)d_arg_internal_number_load(type,
                                     &_validator->low,
                                     _low);
    (void)d_arg_internal_number_load(type,
                                     &_validator->high,
                                     _high);

    return;
}

/*
d_arg_internal_validator_interval
  "[<low> - <high>]", the bounds streamed as CLI11 streams them.
*/
int
d_arg_internal_validator_interval(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_type* const    type = (const struct d_arg_type*)
                                             _validator->parameter;
    struct d_arg_internal_number      low  = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number      high = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number_text text = D_INTERNAL_ARG_NUMBER_TEXT_INIT;

    d_arg_internal_validator_bounds(_validator,
                                    &low,
                                    &high);
    d_arg_internal_number_stream(&text,
                                 type,
                                 &low);
    (void)d_arg_buffer_append_char(_out,
                                   '[');
    (void)d_arg_buffer_append(_out,
                              text.data,
                              text.length);
    (void)d_arg_buffer_append_cstr(_out,
                                   " - ");
    d_arg_internal_number_stream(&text,
                                 type,
                                 &high);
    (void)d_arg_buffer_append(_out,
                              text.data,
                              text.length);

    return d_arg_buffer_append_char(_out,
                                    ']');
}

/*
d_arg_internal_validator_read
  CLI11's lexical_cast for a range's type: the value converted into a scratch
object, as d_arg_internal_lexical_cast converts it, and widened. A
conversion that fails is no reading; one that stops -- a refused allocation
-- stops the validator too.
*/
int
d_arg_internal_validator_read(
    const struct d_arg_validator*  _validator,
    const struct d_arg_validation* _validation,
    struct d_arg_internal_number*  _value,
    bool*                          _read
)
{
    const struct d_arg_type* const     type    =
        (const struct d_arg_type*)_validator->parameter;
    const struct d_arg_convert_context context = { _validation->integer_rule,
                                                   _validation->option,
                                                   NULL };
    union d_arg_internal_scratch       scratch = { 0.0L };
    void* const                        object  =
        d_arg_internal_scratch_acquire(&scratch,
                                       type->size);

    // check if memory allocation was successful
    if (!object)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const int status = d_arg_internal_lexical_cast(type,
                                                   _validation->value,
                                                   &context,
                                                   object);

    *_read = ( (status == D_ARG_OK) &&
               (d_arg_internal_number_load(type,
                                           object,
                                           _value)) );

    d_arg_internal_scratch_release(&scratch,
                                   object);

    return ( (status == D_ARG_OK) ||
             (status == D_ARG_ERROR_CONVERSION) )
           ? D_ARG_OK
           : status;
}

/*
d_arg_internal_validator_range_validate
  CLI11's Range: a value that does not convert, or lies below the low bound
or above the high one, is rejected with "Value <value> not in range [<low>
- <high>]"; a NaN, which neither comparison catches, passes, as in CLI11.
*/
D_STATIC int
d_arg_internal_validator_range_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_internal_number value  = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number low    = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number high   = D_INTERNAL_ARG_NUMBER_INIT;
    bool                         read   = false;
    const int                    status =
        d_arg_internal_validator_read(_validator,
                                      _validation,
                                      &value,
                                      &read);

    d_arg_internal_validator_bounds(_validator,
                                    &low,
                                    &high);

    // a refused allocation, or a value in range
    if ( (status != D_ARG_OK) ||
         ( (read) &&
           (d_arg_internal_number_compare(&value,
                                          &low) != -1) &&
           (d_arg_internal_number_compare(&value,
                                          &high) != 1) ) )
    {
        return status;
    }

    d_arg_buffer_clear(&_validation->message);
    (void)d_arg_buffer_append_cstr(&_validation->message,
                                   "Value ");
    (void)d_arg_buffer_append_text(&_validation->message,
                                   _validation->value);
    (void)d_arg_buffer_append_cstr(&_validation->message,
                                   " not in range ");

    return d_arg_internal_validator_interval(_validator,
                                             &_validation->message);
}

/*
d_arg_internal_validator_range_describe
  CLI11's Range description: "<TYPE> in [<low> - <high>]", the type named
by its descriptor.
*/
D_STATIC int
d_arg_internal_validator_range_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_type* const type =
        (const struct d_arg_type*)_validator->parameter;

    (void)d_arg_buffer_append_cstr(_out,
                                   type->name);
    (void)d_arg_buffer_append_cstr(_out,
                                   " in ");

    return d_arg_internal_validator_interval(_validator,
                                             _out);
}

/*
d_arg_internal_validator_positive_describe
  PositiveNumber's description, as CLI11 names it.
*/
D_STATIC int
d_arg_internal_validator_positive_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    (void)_validator;

    return d_arg_buffer_append_cstr(_out,
                                    "POSITIVE");
}

/*
d_arg_internal_validator_non_negative_describe
  NonNegativeNumber's description, as CLI11 names it.
*/
D_STATIC int
d_arg_internal_validator_non_negative_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    (void)_validator;

    return d_arg_buffer_append_cstr(_out,
                                    "NONNEGATIVE");
}

/*
d_arg_validator_range
  The type kept as the parameter, the bounds copied into the validator, so
nothing is allocated; the type must be arithmetic, convert into a scratch
object -- which a container's does not, whatever its flags and size say --
and fit the bounds' room. Its values complete as numbers.
*/
struct d_arg_validator
d_arg_validator_range(
    const struct d_arg_type* _type,
    const void*              _low,
    const void*              _high
)
{
    struct d_arg_validator validator = VALIDATOR_DEFAULT;

    // an arithmetic type that converts and whose objects fit, and both
    // bounds
    if ( (!d_arg_internal_type_convertible(_type))            ||
         (!_low)                                              ||
         (!_high)                                             ||
         (_type->size > sizeof(union d_arg_validator_value))  ||
         (d_arg_internal_number_of(_type) ==
          (int)D_ARG_INTERNAL_NUMBER_NONE) )
    {
        validator.status = D_ARG_ERROR_BAD_ARGUMENT;

        return validator;
    }

    validator.validate  = d_arg_internal_validator_range_validate;
    validator.describe  = d_arg_internal_validator_range_describe;
    validator.parameter = _type;
    validator.flags    |= D_ARG_VALIDATOR_NUMBER;
    memcpy(&validator.low,
           _low,
           _type->size);
    memcpy(&validator.high,
           _high,
           _type->size);

    return validator;
}

/*
d_arg_validator_range_long
  d_arg_validator_range over the built-in `long`.
*/
struct d_arg_validator
d_arg_validator_range_long(
    long _low,
    long _high
)
{
    return d_arg_validator_range(d_arg_type_builtin(D_ARG_BUILTIN_LONG),
                                 &_low,
                                 &_high);
}

/*
d_arg_validator_range_ulong
  d_arg_validator_range over the built-in `unsigned long`.
*/
struct d_arg_validator
d_arg_validator_range_ulong(
    unsigned long _low,
    unsigned long _high
)
{
    return d_arg_validator_range(
               d_arg_type_builtin(D_ARG_BUILTIN_UNSIGNED_LONG),
               &_low,
               &_high);
}

/*
d_arg_validator_range_double
  d_arg_validator_range over the built-in `double`.
*/
struct d_arg_validator
d_arg_validator_range_double(
    double _low,
    double _high
)
{
    return d_arg_validator_range(d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE),
                                 &_low,
                                 &_high);
}

/*
d_arg_validator_positive_number
  CLI11's PositiveNumber: a `double` range from the smallest positive value
-- the least subnormal, DBL_MIN * DBL_EPSILON, CLI11's denorm_min -- to the
largest.
*/
struct d_arg_validator
d_arg_validator_positive_number(void)
{
    struct d_arg_validator validator =
        d_arg_validator_range_double(DBL_MIN * DBL_EPSILON,
                                     DBL_MAX);

    validator.describe = d_arg_internal_validator_positive_describe;

    return validator;
}

/*
d_arg_validator_non_negative_number
  CLI11's NonNegativeNumber: a `double` range from 0 to the largest value.
*/
struct d_arg_validator
d_arg_validator_non_negative_number(void)
{
    struct d_arg_validator validator =
        d_arg_validator_range_double(0.0,
                                     DBL_MAX);

    validator.describe = d_arg_internal_validator_non_negative_describe;

    return validator;
}

/*
d_arg_internal_validator_unquote
  CLI11's process_quoted_string, without its secondary-array escape: a
binary string becomes its bytes; double quotes come off, escapes resolved
within; single or back quotes come off, nothing more.
*/
D_STATIC int
d_arg_internal_validator_unquote(
    struct d_pack_text   _value,
    struct d_arg_buffer* _out,
    struct d_arg_buffer* _error
)
{
    const struct d_pack_text inner = d_arg_internal_span(_value.data + 1,
                                                         _value.length - 2u);

    // a binary string
    if (d_arg_internal_is_binary_string(_value))
    {
        return d_arg_internal_extract_binary(_value,
                                             _out);
    }

    // double quotes, with escapes
    if ( (_value.data[0] == '"') &&
         (memchr(inner.data,
                 '\\',
                 inner.length) != NULL) )
    {
        return d_arg_internal_unescape(inner,
                                       _out,
                                       _error);
    }

    return d_arg_buffer_append_text(_out,
                                    inner);
}

/*
d_arg_internal_validator_escaped_validate
  CLI11's EscapedString: a quoted value is unquoted; an unquoted one with a
backslash is a binary string's bytes or has its escapes resolved; anything
else stands. A malformed escape is the rejection, as CLI11 returns its
invalid_argument's what().
*/
D_STATIC int
d_arg_internal_validator_escaped_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_pack_text value  = _validation->value;
    const bool               quoted = ( (value.length > 1u)                 &&
                                        (memchr("\"'`",
                                                value.data[0],
                                                3u) != NULL)                &&
                                        (value.data[0] ==
                                         value.data[value.length - 1u]) );
    struct d_arg_buffer      out    = D_ARG_BUFFER_INIT;
    struct d_arg_buffer      error  = D_ARG_BUFFER_INIT;
    int                      status = D_ARG_OK;

    (void)_validator;

    // a quoted value, or one with escapes; anything else stands
    if (quoted)
    {
        status = d_arg_internal_validator_unquote(value,
                                                  &out,
                                                  &error);
    }
    else if ( (value.length == 0u) ||
              (memchr(value.data,
                      '\\',
                      value.length) == NULL) )
    {
        return D_ARG_OK;
    }
    else
    {
        status = d_arg_internal_is_binary_string(value)
                 ? d_arg_internal_extract_binary(value,
                                                 &out)
                 : d_arg_internal_unescape(value,
                                           &out,
                                           &error);
    }

    // the result replaces the value; a malformed escape rejects it
    status = (status == D_ARG_OK)
             ? d_arg_validation_replace(_validation,
                                        d_arg_buffer_text(&out))
             : ( (status == D_ARG_ERROR_VALIDATION)
                 ? d_arg_validation_reject(_validation,
                                           d_arg_buffer_text(&error).data)
                 : status );

    d_arg_buffer_release(&error);
    d_arg_buffer_release(&out);

    return status;
}

/*
d_arg_validator_escaped_string
  The default validator with the EscapedString function, described by
nothing.
*/
struct d_arg_validator
d_arg_validator_escaped_string(void)
{
    struct d_arg_validator validator = VALIDATOR_DEFAULT;

    validator.validate = d_arg_internal_validator_escaped_validate;

    return validator;
}
