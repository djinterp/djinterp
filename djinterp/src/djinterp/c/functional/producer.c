/*******************************************************************************
* djinterp [c]                                                        producer.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/producer.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/producer.h"


/*
d_internal_producer_next_empty
  The exhausted producer: never emits.

Parameter(s):
  _state: unused.
  _out:   unused.
Return:
  Always false.
*/
static bool
d_internal_producer_next_empty
(
    void* _state,
    void* _out
)
{
    (void)_state;
    (void)_out;

    return false;
}

/*
d_internal_producer_next_array
  Emits the elements of a contiguous array in order.

Parameter(s):
  _state: the `d_producer_array_state` to advance.
  _out:   destination for the emitted element.
Return:
  A boolean value corresponding to either:
  - true, if an element was written to `_out`, or
  - false, if the array is exhausted or the parameters are unusable.
*/
static bool
d_internal_producer_next_array
(
    void* _state,
    void* _out
)
{
    struct d_producer_array_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_array_state*)_state;

    // exhaustion is the ordinary end of the array
    if (state->remaining == 0)
    {
        return false;
    }

    memcpy(_out, state->cursor, state->element_size);

    state->cursor    = state->cursor + state->element_size;
    state->remaining = state->remaining - 1;

    return true;
}

/*
d_internal_producer_next_range
  Emits successive `intmax_t` values of a half-open range.

Parameter(s):
  _state: the `d_producer_range_state` to advance.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if a value was written to `_out`, or
  - false, if the range is exhausted, the step is zero, or the parameters are
    unusable.
*/
static bool
d_internal_producer_next_range
(
    void* _state,
    void* _out
)
{
    struct d_producer_range_state* state;
    intmax_t*                     out;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_range_state*)_state;
    out   = (intmax_t*)_out;

    // a zero step would never terminate, so it emits nothing at all
    if (state->step == 0)
    {
        return false;
    }

    // an ascending range ends at `end`; a descending one ends below it
    if ( ( (state->step > 0) && (state->current >= state->end) ) ||
         ( (state->step < 0) && (state->current <= state->end) ) )
    {
        return false;
    }

    *out = state->current;

    state->current = state->current + state->step;

    return true;
}

/*
d_internal_producer_next_repeat
  Emits one borrowed value repeatedly, either a bounded number of times or
without end.

Parameter(s):
  _state: the `d_producer_repeat_state` to advance.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if a value was written to `_out`, or
  - false, if the bound is spent or the parameters are unusable.
*/
static bool
d_internal_producer_next_repeat
(
    void* _state,
    void* _out
)
{
    struct d_producer_repeat_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_repeat_state*)_state;

    // a spent bound, or nothing to copy, ends the producer
    if ( (state->remaining == 0) ||
         (!state->value)         )
    {
        return false;
    }

    memcpy(_out, state->value, state->value_size);

    // an unbounded producer never decrements, so it never exhausts
    if (state->remaining != D_PRODUCER_UNBOUNDED)
    {
        state->remaining = state->remaining - 1;
    }

    return true;
}

/*
d_internal_producer_next_generate
  Emits whatever an `fn_producer` callback writes, until it reports failure.

Parameter(s):
  _state: the `d_producer_generate_state` carrying the callback.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if the callback produced a value, or
  - false, otherwise.
*/
static bool
d_internal_producer_next_generate
(
    void* _state,
    void* _out
)
{
    struct d_producer_generate_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_generate_state*)_state;

    // an unset callback produces nothing
    if (!state->generate)
    {
        return false;
    }

    return state->generate(_out, state->context);
}

/*
d_internal_producer_next_iterate
  Emits the seed, then each successive image of the step transform. The current
value lives in caller-owned scratch, so the transform never aliases its own
input and output.

Parameter(s):
  _state: the `d_producer_iterate_state` to advance.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if a value was written to `_out`, or
  - false, if the transform failed or the parameters are unusable.
*/
static bool
d_internal_producer_next_iterate
(
    void* _state,
    void* _out
)
{
    struct d_producer_iterate_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_iterate_state*)_state;

    // without scratch or a step there is no sequence to walk
    if ( (!state->current) ||
         (!state->step)    )
    {
        return false;
    }

    // the seed is emitted before the transform is applied even once
    if (!state->primed)
    {
        memcpy(_out, state->current, state->value_size);

        state->primed = true;

        return true;
    }

    // advance into the destination, then adopt it as the new current value
    if (!state->step(state->current, _out, state->context))
    {
        return false;
    }

    memcpy(state->current, _out, state->value_size);

    return true;
}

/*
d_internal_producer_next_take
  Passes through at most a fixed number of values from an inner producer.

Parameter(s):
  _state: the `d_producer_take_state` to advance.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if a value was passed through, or
  - false, if the bound is spent, the inner producer is exhausted, or the
    parameters are unusable.
*/
static bool
d_internal_producer_next_take
(
    void* _state,
    void* _out
)
{
    struct d_producer_take_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_take_state*)_state;

    // a spent bound or a missing inner producer ends the sequence
    if ( (state->remaining == 0) ||
         (!state->inner)         ||
         (!state->inner->next)   )
    {
        return false;
    }

    // the inner producer may end before the bound is reached
    if (!state->inner->next(state->inner->state, _out))
    {
        state->remaining = 0;

        return false;
    }

    state->remaining = state->remaining - 1;

    return true;
}

/*
d_internal_producer_next_drop
  Discards a fixed number of leading values from an inner producer, then passes
the rest through.

Parameter(s):
  _state: the `d_producer_drop_state` to advance.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if a value was passed through, or
  - false, if the inner producer is exhausted or the parameters are unusable.
*/
static bool
d_internal_producer_next_drop
(
    void* _state,
    void* _out
)
{
    struct d_producer_drop_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_drop_state*)_state;

    // a missing inner producer ends the sequence
    if ( (!state->inner)       ||
         (!state->inner->next) )
    {
        return false;
    }

    // burn the prefix through the destination buffer before emitting
    while (state->remaining > 0)
    {
        if (!state->inner->next(state->inner->state, _out))
        {
            state->remaining = 0;

            return false;
        }

        state->remaining = state->remaining - 1;
    }

    return state->inner->next(state->inner->state, _out);
}

/*
d_internal_producer_next_concat
  Emits every value of the first producer, then every value of the second.

Parameter(s):
  _state: the `d_producer_concat_state` to advance.
  _out:   destination for the emitted value.
Return:
  A boolean value corresponding to either:
  - true, if a value was emitted from either producer, or
  - false, if both are exhausted or the parameters are unusable.
*/
static bool
d_internal_producer_next_concat
(
    void* _state,
    void* _out
)
{
    struct d_producer_concat_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_concat_state*)_state;

    // drain the first producer before touching the second
    if (!state->first_done)
    {
        if ( (state->first)       &&
             (state->first->next) )
        {
            if (state->first->next(state->first->state, _out))
            {
                return true;
            }
        }

        state->first_done = true;
    }

    // a missing second producer ends the concatenation
    if ( (!state->second)       ||
         (!state->second->next) )
    {
        return false;
    }

    return state->second->next(state->second->state, _out);
}

/*
d_internal_producer_next_filter
  Pulls until the inner producer yields a value the predicate admits.

Parameter(s):
  _state: the `d_producer_filter_state` to advance.
  _out:   destination for the admitted value.
Return:
  A boolean value corresponding to either:
  - true, if an admitted value was written to `_out`, or
  - false, if the inner producer exhausted first or the parameters are
    unusable.
*/
static bool
d_internal_producer_next_filter
(
    void* _state,
    void* _out
)
{
    struct d_producer_filter_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_filter_state*)_state;

    // without an inner producer or a predicate nothing can be admitted
    if ( (!state->inner)       ||
         (!state->inner->next) ||
         (!state->predicate)   )
    {
        return false;
    }

    // skip rejected values; the destination doubles as the test buffer
    while (state->inner->next(state->inner->state, _out))
    {
        if (state->predicate(_out, state->context))
        {
            return true;
        }
    }

    return false;
}

/*
d_internal_producer_next_transform
  Maps each value of an inner producer through a transform. The inner value
lands in caller-owned scratch so the transform's input and output never alias.

Parameter(s):
  _state: the `d_producer_transform_state` to advance.
  _out:   destination for the mapped value.
Return:
  A boolean value corresponding to either:
  - true, if a mapped value was written to `_out`, or
  - false, if the inner producer exhausted, the transform failed, or the
    parameters are unusable.
*/
static bool
d_internal_producer_next_transform
(
    void* _state,
    void* _out
)
{
    struct d_producer_transform_state* state;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_producer_transform_state*)_state;

    // without an inner producer, a transform, or scratch there is no mapping
    if ( (!state->inner)       ||
         (!state->inner->next) ||
         (!state->transform)   ||
         (!state->scratch)     )
    {
        return false;
    }

    // exhaustion of the inner producer ends the mapped sequence
    if (!state->inner->next(state->inner->state, state->scratch))
    {
        return false;
    }

    return state->transform(state->scratch, _out, state->context);
}


/*
d_producer_make
  Builds a producer from a pull step, its state, and the emitted value size.

Parameter(s):
  _next:       the pull step; may be NULL, yielding an invalid producer.
  _state:      caller-owned producer state; may be NULL for stateless steps.
  _value_size: byte size of each emitted value.
Return:
  A producer by value.
*/
struct d_producer
d_producer_make
(
    fn_producer_next _next,
    void*            _state,
    size_t           _value_size
)
{
    struct d_producer producer;

    producer.next       = _next;
    producer.state      = _state;
    producer.value_size = _value_size;

    return producer;
}

/*
d_producer_is_valid
  Reports whether a producer carries a callable pull step.

Parameter(s):
  _producer: the producer to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the producer is non-NULL and its step is set, or
  - false, otherwise.
*/
bool
d_producer_is_valid
(
    const struct d_producer* _producer
)
{
    // a NULL producer is not valid
    if (!_producer)
    {
        return false;
    }

    return (_producer->next != NULL);
}

/*
d_producer_empty
  Builds the producer that emits nothing. It is the identity of `concat` and the
`aempty` of the sequence Alternative.

Parameter(s):
  _value_size: byte size the producer would have emitted.
Return:
  A producer by value that always reports exhaustion.
*/
struct d_producer
d_producer_empty
(
    size_t _value_size
)
{
    return d_producer_make(&d_internal_producer_next_empty,
                           NULL,
                           _value_size);
}

/*
d_producer_single
  Builds a producer emitting one borrowed value exactly once.

Parameter(s):
  _state:      caller-owned state to initialise; must outlive the producer.
  _value:      the borrowed value to emit.
  _value_size: byte size of `_value`.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_single
(
    struct d_producer_repeat_state* _state,
    const void*                     _value,
    size_t                          _value_size
)
{
    return d_producer_repeat_n(_state, _value, _value_size, 1);
}

/*
d_producer_from_array
  Builds a finite producer over a contiguous array. The array is borrowed and
must outlive the producer.

Parameter(s):
  _state:        caller-owned state to initialise; must outlive the producer.
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to emit.
  _element_size: the stride between elements in bytes; must be non-zero.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_from_array
(
    struct d_producer_array_state* _state,
    const void*                    _elements,
    size_t                         _count,
    size_t                         _element_size
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)                        ||
         (_element_size == 0)             ||
         ( (!_elements) && (_count > 0) ) )
    {
        return d_producer_empty(_element_size);
    }

    _state->cursor       = (const unsigned char*)_elements;
    _state->remaining    = _count;
    _state->element_size = _element_size;

    return d_producer_make(&d_internal_producer_next_array,
                           _state,
                           _element_size);
}

/*
d_producer_range
  Builds a producer over the half-open integer range from `_start` to `_end`,
advancing by `_step`. Emits `intmax_t`.

Parameter(s):
  _state: caller-owned state to initialise; must outlive the producer.
  _start: the first value emitted.
  _end:   the exclusive bound.
  _step:  the increment; may be negative, and a zero step emits nothing.
Return:
  A producer by value; the empty producer if `_state` is NULL.
*/
struct d_producer
d_producer_range
(
    struct d_producer_range_state* _state,
    intmax_t                       _start,
    intmax_t                       _end,
    intmax_t                       _step
)
{
    // without state there is nowhere to hold the cursor
    if (!_state)
    {
        return d_producer_empty(sizeof(intmax_t));
    }

    _state->current = _start;
    _state->end     = _end;
    _state->step    = _step;

    return d_producer_make(&d_internal_producer_next_range,
                           _state,
                           sizeof(intmax_t));
}

/*
d_producer_iota
  Builds an ascending unit-step range from `_start` to `_end`.

Parameter(s):
  _state: caller-owned state to initialise; must outlive the producer.
  _start: the first value emitted.
  _end:   the exclusive bound.
Return:
  A producer by value.
*/
struct d_producer
d_producer_iota
(
    struct d_producer_range_state* _state,
    intmax_t                       _start,
    intmax_t                       _end
)
{
    return d_producer_range(_state, _start, _end, 1);
}

/*
d_producer_repeat
  Builds an unbounded producer emitting one borrowed value forever. It must be
bounded, by `d_producer_take_n` or a short-circuiting reducer, before it is
drained.

Parameter(s):
  _state:      caller-owned state to initialise; must outlive the producer.
  _value:      the borrowed value to emit.
  _value_size: byte size of `_value`.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_repeat
(
    struct d_producer_repeat_state* _state,
    const void*                     _value,
    size_t                          _value_size
)
{
    return d_producer_repeat_n(_state,
                               _value,
                               _value_size,
                               D_PRODUCER_UNBOUNDED);
}

/*
d_producer_repeat_n
  Builds a producer emitting one borrowed value a bounded number of times.

Parameter(s):
  _state:      caller-owned state to initialise; must outlive the producer.
  _value:      the borrowed value to emit.
  _value_size: byte size of `_value`; must be non-zero.
  _count:      how many times to emit it, or D_PRODUCER_UNBOUNDED.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_repeat_n
(
    struct d_producer_repeat_state* _state,
    const void*                     _value,
    size_t                          _value_size,
    size_t                          _count
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)           ||
         (!_value)           ||
         (_value_size == 0)  )
    {
        return d_producer_empty(_value_size);
    }

    _state->value      = _value;
    _state->value_size = _value_size;
    _state->remaining  = _count;

    return d_producer_make(&d_internal_producer_next_repeat,
                           _state,
                           _value_size);
}

/*
d_producer_generate
  Builds a producer driven by an `fn_producer` callback, which ends the sequence
by returning false.

Parameter(s):
  _state:      caller-owned state to initialise; must outlive the producer.
  _generate:   the callback writing each value.
  _context:    context forwarded to `_generate`; may be NULL.
  _value_size: byte size of each emitted value.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_generate
(
    struct d_producer_generate_state* _state,
    fn_producer                       _generate,
    void*                             _context,
    size_t                            _value_size
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)    ||
         (!_generate) )
    {
        return d_producer_empty(_value_size);
    }

    _state->generate = _generate;
    _state->context  = _context;

    return d_producer_make(&d_internal_producer_next_generate,
                           _state,
                           _value_size);
}

/*
d_producer_iterate
  Builds the unbounded producer `seed, step(seed), step(step(seed)), ...`. The
seed buffer is caller-owned and is overwritten as the sequence advances.

Parameter(s):
  _state:      caller-owned state to initialise; must outlive the producer.
  _seed:       caller-owned buffer holding the first value; overwritten.
  _value_size: byte size of the value; must be non-zero.
  _step:       the successor transform.
  _context:    context forwarded to `_step`; may be NULL.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_iterate
(
    struct d_producer_iterate_state* _state,
    void*                            _seed,
    size_t                           _value_size,
    fn_transformer                   _step,
    void*                            _context
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)          ||
         (!_seed)           ||
         (!_step)           ||
         (_value_size == 0) )
    {
        return d_producer_empty(_value_size);
    }

    _state->current    = _seed;
    _state->value_size = _value_size;
    _state->step       = _step;
    _state->context    = _context;
    _state->primed     = false;

    return d_producer_make(&d_internal_producer_next_iterate,
                           _state,
                           _value_size);
}

/*
d_producer_take_n
  Bounds an inner producer to at most `_count` values. This is what makes an
infinite producer drainable.

Parameter(s):
  _state: caller-owned state to initialise; must outlive the producer.
  _inner: the borrowed inner producer; must outlive the result.
  _count: the maximum number of values to pass through.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_take_n
(
    struct d_producer_take_state* _state,
    struct d_producer*            _inner,
    size_t                        _count
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)                     ||
         (!d_producer_is_valid(_inner)) )
    {
        return d_producer_empty((_inner) ? _inner->value_size : 0);
    }

    _state->inner     = _inner;
    _state->remaining = _count;

    return d_producer_make(&d_internal_producer_next_take,
                           _state,
                           _inner->value_size);
}

/*
d_producer_drop_n
  Skips the first `_count` values of an inner producer.

Parameter(s):
  _state: caller-owned state to initialise; must outlive the producer.
  _inner: the borrowed inner producer; must outlive the result.
  _count: the number of leading values to discard.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_drop_n
(
    struct d_producer_drop_state* _state,
    struct d_producer*            _inner,
    size_t                        _count
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)                     ||
         (!d_producer_is_valid(_inner)) )
    {
        return d_producer_empty((_inner) ? _inner->value_size : 0);
    }

    _state->inner     = _inner;
    _state->remaining = _count;

    return d_producer_make(&d_internal_producer_next_drop,
                           _state,
                           _inner->value_size);
}

/*
d_producer_concat
  Sequences two producers, emitting all of the first and then all of the second.
Both must emit the same value size.

Parameter(s):
  _state:  caller-owned state to initialise; must outlive the producer.
  _first:  the borrowed first producer; must outlive the result.
  _second: the borrowed second producer; must outlive the result.
Return:
  A producer by value; the empty producer if the parameters are unusable or the
value sizes disagree.
*/
struct d_producer
d_producer_concat
(
    struct d_producer_concat_state* _state,
    struct d_producer*              _first,
    struct d_producer*              _second
)
{
    // both sides must exist and agree on the emitted value size
    if ( (!_state)                          ||
         (!d_producer_is_valid(_first))      ||
         (!d_producer_is_valid(_second))     ||
         (_first->value_size != _second->value_size) )
    {
        return d_producer_empty((_first) ? _first->value_size : 0);
    }

    _state->first      = _first;
    _state->second     = _second;
    _state->first_done = false;

    return d_producer_make(&d_internal_producer_next_concat,
                           _state,
                           _first->value_size);
}

/*
d_producer_filter
  Passes through only the values of an inner producer that a predicate admits.

Parameter(s):
  _state:     caller-owned state to initialise; must outlive the producer.
  _inner:     the borrowed inner producer; must outlive the result.
  _predicate: the admission test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_filter
(
    struct d_producer_filter_state* _state,
    struct d_producer*              _inner,
    fn_predicate                    _predicate,
    void*                           _context
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)                      ||
         (!d_producer_is_valid(_inner)) ||
         (!_predicate)                  )
    {
        return d_producer_empty((_inner) ? _inner->value_size : 0);
    }

    _state->inner     = _inner;
    _state->predicate = _predicate;
    _state->context   = _context;

    return d_producer_make(&d_internal_producer_next_filter,
                           _state,
                           _inner->value_size);
}

/*
d_producer_transform
  Maps every value of an inner producer through a transform.

Parameter(s):
  _state:      caller-owned state to initialise; must outlive the producer.
  _inner:      the borrowed inner producer; must outlive the result.
  _transform:  the mapping applied to each value.
  _context:    context forwarded to `_transform`; may be NULL.
  _scratch:    caller-owned buffer of at least `_inner->value_size` bytes.
  _value_size: byte size of the mapped values.
Return:
  A producer by value; the empty producer if the parameters are unusable.
*/
struct d_producer
d_producer_transform
(
    struct d_producer_transform_state* _state,
    struct d_producer*                 _inner,
    fn_transformer                     _transform,
    void*                              _context,
    void*                              _scratch,
    size_t                             _value_size
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state)                      ||
         (!d_producer_is_valid(_inner)) ||
         (!_transform)                  ||
         (!_scratch)                    ||
         (_value_size == 0)             )
    {
        return d_producer_empty(_value_size);
    }

    _state->inner     = _inner;
    _state->transform = _transform;
    _state->context   = _context;
    _state->scratch   = _scratch;

    return d_producer_make(&d_internal_producer_next_transform,
                           _state,
                           _value_size);
}

/*
d_producer_pull
  Pulls one value from a producer into a caller-supplied buffer.

Parameter(s):
  _producer: the producer to pull from.
  _out:      destination buffer of at least `_producer->value_size` bytes.
Return:
  A `d_producer_step` whose `has_value` reports whether a value was written and
whose `value` aliases `_out` when it was.
*/
struct d_producer_step
d_producer_pull
(
    struct d_producer* _producer,
    void*              _out
)
{
    struct d_producer_step step;

    step.has_value = false;
    step.value     = NULL;

    // an unusable request reports exhaustion
    if ( (!d_producer_is_valid(_producer)) ||
         (!_out)                           )
    {
        return step;
    }

    // a successful pull hands back the caller's own buffer
    if (_producer->next(_producer->state, _out))
    {
        step.has_value = true;
        step.value     = _out;
    }

    return step;
}

/*
d_producer_drive
  Pushes every value of a producer through a reducer, stopping when the producer
exhausts or the reducing state latches. This is the bridge that lets one reducer
serve both arrays and producers.

Parameter(s):
  _producer: the producer to drain.
  _reducer:  the reducer to fold each value into.
  _state:    the reducing state.
  _scratch:  caller-owned buffer of at least `_producer->value_size` bytes.
Return:
  The number of values delivered to the reducer.
*/
size_t
d_producer_drive
(
    struct d_producer*       _producer,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state,
    void*                    _scratch
)
{
    size_t delivered;

    // an unusable request delivers nothing
    if ( (!d_producer_is_valid(_producer)) ||
         (!d_reducer_is_valid(_reducer))   ||
         (!_state)                         ||
         (!_scratch)                       )
    {
        return 0;
    }

    delivered = 0;

    // pull and fold until either side says stop
    while (!_state->done)
    {
        if (!_producer->next(_producer->state, _scratch))
        {
            break;
        }

        _reducer->step(_state, _scratch, _reducer->context);

        delivered = delivered + 1;
    }

    return delivered;
}

/*
d_producer_collect
  Drains a producer into a contiguous array, stopping at the array's capacity.
The producer must be finite or bounded, or the capacity will bound it.

Parameter(s):
  _producer:  the producer to drain.
  _out_array: destination array of at least `_capacity` values.
  _capacity:  the number of values the array can hold.
Return:
  The number of values written to `_out_array`.
*/
size_t
d_producer_collect
(
    struct d_producer* _producer,
    void*              _out_array,
    size_t             _capacity
)
{
    unsigned char* cursor;
    size_t         written;

    // an unusable request writes nothing
    if ( (!d_producer_is_valid(_producer)) ||
         (!_out_array)                     ||
         (_producer->value_size == 0)      )
    {
        return 0;
    }

    cursor  = (unsigned char*)_out_array;
    written = 0;

    // fill the array until the producer or the capacity runs out
    while (written < _capacity)
    {
        if (!_producer->next(_producer->state, (void*)cursor))
        {
            break;
        }

        cursor  = cursor + _producer->value_size;
        written = written + 1;
    }

    return written;
}

/*
d_producer_for_each
  Drains a producer into a consumer.

Parameter(s):
  _producer: the producer to drain.
  _consume:  the sink applied to each value.
  _context:  context forwarded to `_consume`; may be NULL.
  _scratch:  caller-owned buffer of at least `_producer->value_size` bytes.
Return:
  The number of values handed to the consumer.
*/
size_t
d_producer_for_each
(
    struct d_producer* _producer,
    fn_consumer_const  _consume,
    void*              _context,
    void*              _scratch
)
{
    struct d_consumer_binding binding;
    struct d_reducer          reducer;
    struct d_reducing_state   state;

    // an unusable request consumes nothing
    if (!_consume)
    {
        return 0;
    }

    binding.consume = _consume;
    binding.context = _context;

    reducer = d_reducer_from_consumer(&binding);

    d_reducing_state_init(&state, NULL);

    return d_producer_drive(_producer, &reducer, &state, _scratch);
}

/*
d_producer_fold
  Drains a producer through a plain fold step.

Parameter(s):
  _producer:    the producer to drain.
  _accumulator: caller-owned accumulator storage.
  _step:        the fold step.
  _context:     context forwarded to `_step`; may be NULL.
  _scratch:     caller-owned buffer of at least `_producer->value_size` bytes.
Return:
  The number of values folded.
*/
size_t
d_producer_fold
(
    struct d_producer* _producer,
    void*              _accumulator,
    fn_fold            _step,
    void*              _context,
    void*              _scratch
)
{
    struct d_fold_binding   binding;
    struct d_reducer        reducer;
    struct d_reducing_state state;

    // an unusable request folds nothing
    if ( (!_step)        ||
         (!_accumulator) )
    {
        return 0;
    }

    binding.step    = _step;
    binding.context = _context;

    reducer = d_reducer_from_fold(&binding);

    d_reducing_state_init(&state, _accumulator);

    return d_producer_drive(_producer, &reducer, &state, _scratch);
}

/*
d_producer_alt
  The Alternative choice over the sequence carrier, and equally the Semigroup
combine: concatenation. Named for the protocol vocabulary of
functional_types.tex Table 7.3; `d_producer_concat` is the same operation named
for what it does.

Parameter(s):
  _state:  caller-owned state to initialise; must outlive the producer.
  _first:  the borrowed first producer; must outlive the result.
  _second: the borrowed second producer; must outlive the result.
Return:
  A producer by value emitting all of `_first` then all of `_second`.
*/
struct d_producer
d_producer_alt
(
    struct d_producer_concat_state* _state,
    struct d_producer*              _first,
    struct d_producer*              _second
)
{
    return d_producer_concat(_state, _first, _second);
}

/*
d_producer_aempty
  The Alternative identity over the sequence carrier: the empty sequence. It is
the two-sided identity of `d_producer_alt`, which is what makes the sequence
carrier a monoid and not merely a semigroup.

Parameter(s):
  _value_size: byte size the producer would have emitted.
Return:
  A producer by value that always reports exhaustion.
*/
struct d_producer
d_producer_aempty
(
    size_t _value_size
)
{
    return d_producer_empty(_value_size);
}
