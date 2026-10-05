/*******************************************************************************
* djinterp [c]                                                           maybe.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/maybe.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/maybe.h"


/*
d_maybe_view
  Builds a borrowed view onto a caller-owned maybe cell.

Parameter(s):
  _has_value:  the cell's tag; may be NULL, yielding an invalid view.
  _value:      the cell's value slot; may be NULL, yielding an invalid view.
  _value_size: byte width of the value slot; must be non-zero to be valid.
Return:
  A view by value.
*/
struct d_maybe
d_maybe_view
(
    bool*  _has_value,
    void*  _value,
    size_t _value_size
)
{
    struct d_maybe maybe;

    maybe.has_value  = _has_value;
    maybe.value      = _value;
    maybe.value_size = _value_size;

    return maybe;
}

/*
d_maybe_is_valid
  Reports whether a view describes usable storage.

Parameter(s):
  _maybe: the view to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the view points at a tag and a slot of non-zero width, or
  - false, otherwise.
*/
bool
d_maybe_is_valid
(
    const struct d_maybe* _maybe
)
{
    // a NULL view is not valid
    if (!_maybe)
    {
        return false;
    }

    return ( (_maybe->has_value != NULL) &&
             (_maybe->value != NULL)     &&
             (_maybe->value_size != 0)   );
}

/*
d_maybe_just
  Stores a value into the cell and marks it present. This is the `unit` of the
carrier: the introduction form `A -> maybe<A>`.

Parameter(s):
  _maybe: the view to write through.
  _value: the value to copy in, of `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the value was stored, or
  - false, if the parameters were unusable.
*/
bool
d_maybe_just
(
    struct d_maybe* _maybe,
    const void*     _value
)
{
    // an unusable request stores nothing
    if ( (!d_maybe_is_valid(_maybe)) ||
         (!_value)                   )
    {
        return false;
    }

    memcpy(_maybe->value, _value, _maybe->value_size);

    *(_maybe->has_value) = true;

    return true;
}

/*
d_maybe_nothing
  Marks the cell empty. The value slot is left untouched rather than cleared,
since an absent value has no bytes worth reading.

Parameter(s):
  _maybe: the view to write through.
Return:
  A boolean value corresponding to either:
  - true, if the cell was marked empty, or
  - false, if the view was unusable.
*/
bool
d_maybe_nothing
(
    struct d_maybe* _maybe
)
{
    // an unusable view cannot be marked
    if (!d_maybe_is_valid(_maybe))
    {
        return false;
    }

    *(_maybe->has_value) = false;

    return true;
}

/*
d_maybe_is_just
  Reports whether the cell holds a value.

Parameter(s):
  _maybe: the view to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the cell holds a value, or
  - false, if it is empty or the view was unusable.
*/
bool
d_maybe_is_just
(
    const struct d_maybe* _maybe
)
{
    // an unusable view reads as empty, so callers fail safe
    if (!d_maybe_is_valid(_maybe))
    {
        return false;
    }

    return *(_maybe->has_value);
}

/*
d_maybe_is_nothing
  Reports whether the cell is empty. An unusable view reads as empty, so this
is not simply the negation of `d_maybe_is_just` for a NULL argument -- both
answer in the direction that avoids reading an absent value.

Parameter(s):
  _maybe: the view to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the cell is empty or the view was unusable, or
  - false, if it holds a value.
*/
bool
d_maybe_is_nothing
(
    const struct d_maybe* _maybe
)
{
    // an unusable view reads as empty
    if (!d_maybe_is_valid(_maybe))
    {
        return true;
    }

    return !(*(_maybe->has_value));
}

/*
d_maybe_value_or
  Writes the held value, or a fallback when the cell is empty. This is the
elimination form that makes a maybe total.

Parameter(s):
  _maybe:    the view to read.
  _out:      destination of `value_size` bytes.
  _fallback: the value written when the cell is empty.
Return:
  A boolean value corresponding to either:
  - true, if the held value was written, or
  - false, if the fallback was written or the parameters were unusable.
*/
bool
d_maybe_value_or
(
    const struct d_maybe* _maybe,
    void*                 _out,
    const void*           _fallback
)
{
    // without a destination there is nothing to answer into
    if (!_out)
    {
        return false;
    }

    // a present value wins; anything else falls back
    if (d_maybe_is_just(_maybe))
    {
        memcpy(_out, _maybe->value, _maybe->value_size);

        return true;
    }

    // a missing fallback leaves the destination untouched
    if ( (!_fallback)                ||
         (!d_maybe_is_valid(_maybe)) )
    {
        return false;
    }

    memcpy(_out, _fallback, _maybe->value_size);

    return false;
}

/*
d_maybe_map
  The Functor instance: applies a transform under the carrier, leaving an empty
cell empty. The input and output widths may differ, which is what makes this
`map : (A -> B) -> maybe<A> -> maybe<B>` rather than an in-place update.
  A transform that reports failure yields nothing, so a partial function maps a
just into a nothing rather than into an undefined value.

Parameter(s):
  _in:        the source view.
  _out:       the destination view; may alias neither `_in`'s tag nor its slot.
  _transform: the mapping applied to a held value.
  _context:   context forwarded to `_transform`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the output cell was left definite, or
  - false, if the parameters were unusable.
*/
bool
d_maybe_map
(
    const struct d_maybe* _in,
    struct d_maybe*       _out,
    fn_transformer        _transform,
    void*                 _context
)
{
    // an unusable request leaves the destination alone
    if ( (!d_maybe_is_valid(_in))  ||
         (!d_maybe_is_valid(_out)) ||
         (!_transform)             )
    {
        return false;
    }

    // an empty input maps to an empty output without calling the transform
    if (!d_maybe_is_just(_in))
    {
        return d_maybe_nothing(_out);
    }

    // a failed transform collapses to nothing rather than to a stale value
    if (!_transform(_in->value, _out->value, _context))
    {
        return d_maybe_nothing(_out);
    }

    *(_out->has_value) = true;

    return true;
}

/*
d_maybe_bind
  The Monad instance: applies a Kleisli arrow `A -> maybe<B>` to a held value
and flattens, leaving an empty cell empty. The arrow writes through the output
view itself, which is how C returns a carrier without a template parameter.

Parameter(s):
  _in:      the source view.
  _out:     the destination view, handed to the arrow.
  _arrow:   the Kleisli arrow.
  _context: context forwarded to `_arrow`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the output cell was left definite, or
  - false, if the parameters were unusable.
*/
bool
d_maybe_bind
(
    const struct d_maybe* _in,
    struct d_maybe*       _out,
    fn_kleisli            _arrow,
    void*                 _context
)
{
    // an unusable request leaves the destination alone
    if ( (!d_maybe_is_valid(_in))  ||
         (!d_maybe_is_valid(_out)) ||
         (!_arrow)                 )
    {
        return false;
    }

    // an empty input short-circuits, which is the whole point of the instance
    if (!d_maybe_is_just(_in))
    {
        return d_maybe_nothing(_out);
    }

    _arrow(_in->value, (void*)_out, _context);

    return true;
}

/*
d_maybe_filter
  Empties the cell when a held value fails a predicate. This is the maybe
spelling of the Filter/slice shape: 0--1 elements in, 0--1 out, never more.

Parameter(s):
  _maybe:     the view to filter in place.
  _predicate: the admission test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if a value was held and admitted, or
  - false, otherwise.
*/
bool
d_maybe_filter
(
    struct d_maybe* _maybe,
    fn_predicate    _predicate,
    void*           _context
)
{
    // an unusable request leaves the cell alone
    if ( (!d_maybe_is_valid(_maybe)) ||
         (!_predicate)               )
    {
        return false;
    }

    // an already-empty cell has nothing to test
    if (!d_maybe_is_just(_maybe))
    {
        return false;
    }

    // a rejected value empties the cell
    if (!_predicate(_maybe->value, _context))
    {
        d_maybe_nothing(_maybe);

        return false;
    }

    return true;
}

/*
d_maybe_alt
  The Alternative choice: the first cell if it holds a value, otherwise the
second. `d_maybe_aempty` is its two-sided identity.

Parameter(s):
  _first:  the preferred view.
  _second: the fallback view.
  _out:    the destination view.
Return:
  A boolean value corresponding to either:
  - true, if the result holds a value, or
  - false, if both alternatives were empty or the parameters were unusable.
*/
bool
d_maybe_alt
(
    const struct d_maybe* _first,
    const struct d_maybe* _second,
    struct d_maybe*       _out
)
{
    const struct d_maybe* chosen;

    // an unusable destination cannot hold either alternative
    if (!d_maybe_is_valid(_out))
    {
        return false;
    }

    chosen = NULL;

    // the left alternative is preferred, which is what makes alt left-biased
    if (d_maybe_is_just(_first))
    {
        chosen = _first;
    }
    else if (d_maybe_is_just(_second))
    {
        chosen = _second;
    }

    // neither alternative held a value, so the choice is empty
    if (!chosen)
    {
        d_maybe_nothing(_out);

        return false;
    }

    // the widths must agree for the copy to mean anything
    if (chosen->value_size != _out->value_size)
    {
        d_maybe_nothing(_out);

        return false;
    }

    return d_maybe_just(_out, chosen->value);
}

/*
d_maybe_aempty
  The Alternative identity: the empty cell.

Parameter(s):
  _out: the destination view.
Return:
  A boolean value corresponding to either:
  - true, if the cell was marked empty, or
  - false, if the view was unusable.
*/
bool
d_maybe_aempty
(
    struct d_maybe* _out
)
{
    return d_maybe_nothing(_out);
}

/*
d_maybe_drive
  The Foldable instance, expressed over the tier 1 spine: pushes the cell's 0
or 1 elements through an ordinary reducer. Every fold, quantifier and
transducer chain already written against `d_reducer` therefore works on a maybe
without modification.

Parameter(s):
  _maybe:   the view to drain.
  _reducer: the reducer to fold into.
  _state:   the reducing state.
Return:
  The number of elements delivered: 1 for a held value, 0 otherwise.
*/
size_t
d_maybe_drive
(
    const struct d_maybe*    _maybe,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    // an empty cell is an empty sequence, not an error
    if ( (!d_maybe_is_just(_maybe))       ||
         (!d_reducer_is_valid(_reducer))  ||
         (!_state)                        )
    {
        return 0;
    }

    return d_reducer_drive_array(_reducer,
                                 _state,
                                 _maybe->value,
                                 1,
                                 _maybe->value_size);
}

/*
d_maybe_fold
  Folds the cell's 0 or 1 elements through a plain fold step.

Parameter(s):
  _maybe:       the view to drain.
  _accumulator: caller-owned accumulator storage.
  _step:        the fold step.
  _context:     context forwarded to `_step`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if a held value was folded, or
  - false, if the cell was empty or the parameters were unusable.
*/
bool
d_maybe_fold
(
    const struct d_maybe* _maybe,
    void*                 _accumulator,
    fn_fold               _step,
    void*                 _context
)
{
    // an empty cell leaves the accumulator exactly as it was
    if (!d_maybe_is_just(_maybe))
    {
        return false;
    }

    return d_fold_left(_maybe->value,
                       1,
                       _maybe->value_size,
                       _accumulator,
                       _step,
                       _context);
}

/*
d_maybe_producer
  Presents the cell as a producer of 0 or 1 values, which is the bridge Table
7.2 asks for between the maybe carrier and the sequence carriers.

Parameter(s):
  _state: caller-owned producer state; must outlive the producer.
  _maybe: the view to present. The cell must outlive the producer.
Return:
  A producer by value; the empty producer if the cell is empty or unusable.
*/
struct d_producer
d_maybe_producer
(
    struct d_producer_array_state* _state,
    const struct d_maybe*          _maybe
)
{
    // an empty cell presents as the empty producer
    if (!d_maybe_is_just(_maybe))
    {
        return d_producer_empty((_maybe) ? _maybe->value_size : 0);
    }

    return d_producer_from_array(_state,
                                 _maybe->value,
                                 1,
                                 _maybe->value_size);
}
