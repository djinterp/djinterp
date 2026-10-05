/*******************************************************************************
* djinterp [c]                                                          result.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/result.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/result.h"


/*
d_result_view
  Builds a borrowed view onto a caller-owned result cell.

Parameter(s):
  _is_ok:      the cell's tag; may be NULL, yielding an invalid view.
  _payload:    the cell's union slot; may be NULL, yielding an invalid view.
  _value_size: byte width of the success arm.
  _error_size: byte width of the error arm.
Return:
  A view by value.
*/
struct d_result
d_result_view
(
    bool*  _is_ok,
    void*  _payload,
    size_t _value_size,
    size_t _error_size
)
{
    struct d_result result;

    result.is_ok      = _is_ok;
    result.payload    = _payload;
    result.value_size = _value_size;
    result.error_size = _error_size;

    return result;
}

/*
d_result_is_valid
  Reports whether a view describes usable storage. Both arms must have a
non-zero width, since either may become the live one.

Parameter(s):
  _result: the view to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the view is usable, or
  - false, otherwise.
*/
bool
d_result_is_valid
(
    const struct d_result* _result
)
{
    // a NULL view is not valid
    if (!_result)
    {
        return false;
    }

    return ( (_result->is_ok != NULL)    &&
             (_result->payload != NULL)  &&
             (_result->value_size != 0)  &&
             (_result->error_size != 0)  );
}

/*
d_result_ok
  Stores a success value, making the success arm live.

Parameter(s):
  _result: the view to write through.
  _value:  the value to copy in, of `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the value was stored, or
  - false, if the parameters were unusable.
*/
bool
d_result_ok
(
    struct d_result* _result,
    const void*      _value
)
{
    // an unusable request stores nothing
    if ( (!d_result_is_valid(_result)) ||
         (!_value)                     )
    {
        return false;
    }

    memcpy(_result->payload, _value, _result->value_size);

    *(_result->is_ok) = true;

    return true;
}

/*
d_result_err
  Stores an error, making the error arm live. This overwrites the value slot,
which is what the union means.

Parameter(s):
  _result: the view to write through.
  _error:  the error to copy in, of `error_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the error was stored, or
  - false, if the parameters were unusable.
*/
bool
d_result_err
(
    struct d_result* _result,
    const void*      _error
)
{
    // an unusable request stores nothing
    if ( (!d_result_is_valid(_result)) ||
         (!_error)                     )
    {
        return false;
    }

    memcpy(_result->payload, _error, _result->error_size);

    *(_result->is_ok) = false;

    return true;
}

/*
d_result_is_ok
  Reports whether the success arm is live.

Parameter(s):
  _result: the view to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the success arm is live, or
  - false, if the error arm is live or the view was unusable.
*/
bool
d_result_is_ok
(
    const struct d_result* _result
)
{
    // an unusable view does not hold a success
    if (!d_result_is_valid(_result))
    {
        return false;
    }

    return *(_result->is_ok);
}

/*
d_result_is_err
  Reports whether the error arm is live. An unusable view reads as an error
rather than as a success, so callers fail toward the cautious branch.

Parameter(s):
  _result: the view to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the error arm is live or the view was unusable, or
  - false, if the success arm is live.
*/
bool
d_result_is_err
(
    const struct d_result* _result
)
{
    // an unusable view reads as an error
    if (!d_result_is_valid(_result))
    {
        return true;
    }

    return !(*(_result->is_ok));
}

/*
d_result_value_or
  Writes the success value, or a fallback when the error arm is live.

Parameter(s):
  _result:   the view to read.
  _out:      destination of `value_size` bytes.
  _fallback: the value written when the error arm is live.
Return:
  A boolean value corresponding to either:
  - true, if the success value was written, or
  - false, if the fallback was written or the parameters were unusable.
*/
bool
d_result_value_or
(
    const struct d_result* _result,
    void*                  _out,
    const void*            _fallback
)
{
    // without a destination there is nothing to answer into
    if (!_out)
    {
        return false;
    }

    // a live success arm wins; anything else falls back
    if (d_result_is_ok(_result))
    {
        memcpy(_out, _result->payload, _result->value_size);

        return true;
    }

    // a missing fallback leaves the destination untouched
    if ( (!_fallback)                   ||
         (!d_result_is_valid(_result))  )
    {
        return false;
    }

    memcpy(_out, _fallback, _result->value_size);

    return false;
}

/*
d_result_error
  Writes the error when the error arm is live.

Parameter(s):
  _result: the view to read.
  _out:    destination of `error_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the error was written, or
  - false, if the success arm was live or the parameters were unusable.
*/
bool
d_result_error
(
    const struct d_result* _result,
    void*                  _out
)
{
    // only a live error arm has an error to report
    if ( (!_out)                        ||
         (!d_result_is_valid(_result))  ||
         (d_result_is_ok(_result))      )
    {
        return false;
    }

    memcpy(_out, _result->payload, _result->error_size);

    return true;
}

/*
d_result_map
  The Functor instance over the success arm: an error passes through untouched,
which is what makes a chain of maps short-circuit on the first failure.
  A transform that reports failure leaves the destination's success arm live but
holding whatever the transform wrote, so a transform used here must be total on
the success arm; use `d_result_bind` when the mapping can itself fail.

Parameter(s):
  _in:        the source view.
  _out:       the destination view.
  _transform: the mapping applied to a success value.
  _context:   context forwarded to `_transform`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the destination was left definite, or
  - false, if the parameters were unusable or the transform failed.
*/
bool
d_result_map
(
    const struct d_result* _in,
    struct d_result*       _out,
    fn_transformer         _transform,
    void*                  _context
)
{
    // an unusable request leaves the destination alone
    if ( (!d_result_is_valid(_in))  ||
         (!d_result_is_valid(_out)) ||
         (!_transform)              )
    {
        return false;
    }

    // an error passes through unchanged, so the arms must be the same width
    if (!d_result_is_ok(_in))
    {
        if (_in->error_size != _out->error_size)
        {
            return false;
        }

        return d_result_err(_out, _in->payload);
    }

    // the transform writes straight into the destination's live arm
    if (!_transform(_in->payload, _out->payload, _context))
    {
        return false;
    }

    *(_out->is_ok) = true;

    return true;
}

/*
d_result_map_err
  The Functor instance over the error arm: a success passes through untouched.
This is the half of Bifunctor that error-translation actually uses.

Parameter(s):
  _in:        the source view.
  _out:       the destination view.
  _transform: the mapping applied to an error.
  _context:   context forwarded to `_transform`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the destination was left definite, or
  - false, if the parameters were unusable or the transform failed.
*/
bool
d_result_map_err
(
    const struct d_result* _in,
    struct d_result*       _out,
    fn_transformer         _transform,
    void*                  _context
)
{
    // an unusable request leaves the destination alone
    if ( (!d_result_is_valid(_in))  ||
         (!d_result_is_valid(_out)) ||
         (!_transform)              )
    {
        return false;
    }

    // a success passes through unchanged, so the arms must be the same width
    if (d_result_is_ok(_in))
    {
        if (_in->value_size != _out->value_size)
        {
            return false;
        }

        return d_result_ok(_out, _in->payload);
    }

    if (!_transform(_in->payload, _out->payload, _context))
    {
        return false;
    }

    *(_out->is_ok) = false;

    return true;
}

/*
d_result_bimap
  The Bifunctor instance over this carrier: maps whichever arm is live. Exactly
one of the two transforms runs, which is the property that distinguishes bimap
from applying map and map_err in sequence.

Parameter(s):
  _in:       the source view.
  _out:      the destination view.
  _on_value: the mapping applied to a success value.
  _on_error: the mapping applied to an error.
  _context:  context forwarded to whichever mapping runs; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the destination was left definite, or
  - false, if the parameters were unusable or the mapping failed.
*/
bool
d_result_bimap
(
    const struct d_result* _in,
    struct d_result*       _out,
    fn_transformer         _on_value,
    fn_transformer         _on_error,
    void*                  _context
)
{
    // both mappings must exist, since either arm may be the live one
    if ( (!d_result_is_valid(_in))  ||
         (!d_result_is_valid(_out)) ||
         (!_on_value)               ||
         (!_on_error)               )
    {
        return false;
    }

    // only the live arm is mapped
    if (d_result_is_ok(_in))
    {
        if (!_on_value(_in->payload, _out->payload, _context))
        {
            return false;
        }

        *(_out->is_ok) = true;

        return true;
    }

    if (!_on_error(_in->payload, _out->payload, _context))
    {
        return false;
    }

    *(_out->is_ok) = false;

    return true;
}

/*
d_result_bind
  The Monad instance: applies a Kleisli arrow to a success value and flattens.
An error short-circuits without calling the arrow.

Parameter(s):
  _in:      the source view.
  _out:     the destination view, handed to the arrow.
  _arrow:   the Kleisli arrow.
  _context: context forwarded to `_arrow`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the destination was left definite, or
  - false, if the parameters were unusable.
*/
bool
d_result_bind
(
    const struct d_result* _in,
    struct d_result*       _out,
    fn_kleisli             _arrow,
    void*                  _context
)
{
    // an unusable request leaves the destination alone
    if ( (!d_result_is_valid(_in))  ||
         (!d_result_is_valid(_out)) ||
         (!_arrow)                  )
    {
        return false;
    }

    // an error short-circuits, which is the whole point of the instance
    if (!d_result_is_ok(_in))
    {
        if (_in->error_size != _out->error_size)
        {
            return false;
        }

        return d_result_err(_out, _in->payload);
    }

    _arrow(_in->payload, (void*)_out, _context);

    return true;
}

/*
d_result_to_maybe
  Forgets the error, giving the success value as a maybe. This is the forgetful
map between the two carriers, and it is one-way: a maybe cannot recover the
error a result discarded.

Parameter(s):
  _result: the view to convert.
  _out:    the destination maybe view; its width must match the success arm.
Return:
  A boolean value corresponding to either:
  - true, if a success value was written, or
  - false, if the error arm was live or the parameters were unusable.
*/
bool
d_result_to_maybe
(
    const struct d_result* _result,
    struct d_maybe*        _out
)
{
    // an unusable request leaves the destination alone
    if ( (!d_result_is_valid(_result)) ||
         (!d_maybe_is_valid(_out))     )
    {
        return false;
    }

    // the widths must agree for the copy to mean anything
    if (_result->value_size != _out->value_size)
    {
        return false;
    }

    // an error forgets to nothing
    if (!d_result_is_ok(_result))
    {
        d_maybe_nothing(_out);

        return false;
    }

    return d_maybe_just(_out, _result->payload);
}

/*
d_result_drive
  The Foldable instance, expressed over the tier 1 spine: a result is a sequence
of one element when the success arm is live and of none when it is not.

Parameter(s):
  _result:  the view to drain.
  _reducer: the reducer to fold into.
  _state:   the reducing state.
Return:
  The number of elements delivered: 1 for a success, 0 for an error.
*/
size_t
d_result_drive
(
    const struct d_result*   _result,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    // an error is an empty sequence, not a failure of the drive
    if ( (!d_result_is_ok(_result))      ||
         (!d_reducer_is_valid(_reducer)) ||
         (!_state)                       )
    {
        return 0;
    }

    return d_reducer_drive_array(_reducer,
                                 _state,
                                 _result->payload,
                                 1,
                                 _result->value_size);
}

/*
d_result_fold
  Folds the result's 0 or 1 success elements through a plain fold step.

Parameter(s):
  _result:      the view to drain.
  _accumulator: caller-owned accumulator storage.
  _step:        the fold step.
  _context:     context forwarded to `_step`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if a success value was folded, or
  - false, if the error arm was live or the parameters were unusable.
*/
bool
d_result_fold
(
    const struct d_result* _result,
    void*                  _accumulator,
    fn_fold                _step,
    void*                  _context
)
{
    // an error leaves the accumulator exactly as it was
    if (!d_result_is_ok(_result))
    {
        return false;
    }

    return d_fold_left(_result->payload,
                       1,
                       _result->value_size,
                       _accumulator,
                       _step,
                       _context);
}

/*
d_result_producer
  Presents the result as a producer of 0 or 1 success values.

Parameter(s):
  _state:  caller-owned producer state; must outlive the producer.
  _result: the view to present. The cell must outlive the producer.
Return:
  A producer by value; the empty producer if the error arm is live.
*/
struct d_producer
d_result_producer
(
    struct d_producer_array_state* _state,
    const struct d_result*         _result
)
{
    // an error presents as the empty producer
    if (!d_result_is_ok(_result))
    {
        return d_producer_empty((_result) ? _result->value_size : 0);
    }

    return d_producer_from_array(_state,
                                 _result->payload,
                                 1,
                                 _result->value_size);
}
