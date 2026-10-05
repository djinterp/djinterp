/*******************************************************************************
* djinterp [c]                                                      transducer.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/transducer.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/transducer.h"


/*
d_internal_transducer_step_map
  Maps one value and emits the image downstream. A transform that reports
failure drops the value rather than terminating the chain, so a partial
transform behaves as a filter over its domain.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_map
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // without a transform or somewhere to put its output there is no mapping
    if ( (!stage->callable.transform) ||
         (!stage->scratch)            )
    {
        return;
    }

    // a failed transform drops the value
    if (!stage->callable.transform(_element,
                                   stage->scratch,
                                   stage->context))
    {
        return;
    }

    d_transducer_emit(_transducer, _index + 1, _state, stage->scratch);

    return;
}

/*
d_internal_transducer_step_filter
  Emits only the values the predicate admits.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_filter
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // without a predicate nothing can be admitted
    if (!stage->callable.predicate)
    {
        return;
    }

    // pass through only on a positive test
    if (stage->callable.predicate(_element, stage->context))
    {
        d_transducer_emit(_transducer, _index + 1, _state, _element);
    }

    return;
}

/*
d_internal_transducer_step_filter_not
  Emits only the values the predicate rejects.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_filter_not
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // without a predicate nothing can be rejected
    if (!stage->callable.predicate)
    {
        return;
    }

    // pass through only on a negative test
    if (!stage->callable.predicate(_element, stage->context))
    {
        d_transducer_emit(_transducer, _index + 1, _state, _element);
    }

    return;
}

/*
d_internal_transducer_step_take
  Passes through the first `bound` values and then latches termination. The
latch is set after the final value is emitted, so a bound of n yields exactly n
outputs when the source has at least that many.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_take
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // a spent bound emits nothing and stops the drive
    if (stage->seen >= stage->bound)
    {
        d_reducing_state_mark_done(_state);

        return;
    }

    stage->seen = stage->seen + 1;

    d_transducer_emit(_transducer, _index + 1, _state, _element);

    // stop as soon as the bound is met, so no further pull is made
    if (stage->seen >= stage->bound)
    {
        d_reducing_state_mark_done(_state);
    }

    return;
}

/*
d_internal_transducer_step_drop
  Discards the first `bound` values and passes the rest through.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_drop
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // count off the discarded prefix without emitting
    if (stage->seen < stage->bound)
    {
        stage->seen = stage->seen + 1;

        return;
    }

    d_transducer_emit(_transducer, _index + 1, _state, _element);

    return;
}

/*
d_internal_transducer_step_take_while
  Passes values through until the predicate first fails, then latches
termination.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_take_while
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // without a predicate the run cannot be bounded, so it stops immediately
    if (!stage->callable.predicate)
    {
        d_reducing_state_mark_done(_state);

        return;
    }

    // the first failure ends the stream
    if (!stage->callable.predicate(_element, stage->context))
    {
        d_reducing_state_mark_done(_state);

        return;
    }

    d_transducer_emit(_transducer, _index + 1, _state, _element);

    return;
}

/*
d_internal_transducer_step_drop_while
  Discards values until the predicate first fails, then passes everything
through, including values that would satisfy the predicate again.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_drop_while
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // once the latch is set the predicate is never consulted again
    if (!stage->latched)
    {
        if ( (stage->callable.predicate) &&
             (stage->callable.predicate(_element, stage->context)) )
        {
            return;
        }

        stage->latched = true;
    }

    d_transducer_emit(_transducer, _index + 1, _state, _element);

    return;
}

/*
d_internal_transducer_step_tap
  Hands each value to a side-effecting consumer and forwards it unchanged.

Parameter(s):
  _transducer: the chain being run.
  _index:      this stage's index within the chain.
  _state:      the reducing state.
  _element:    the incoming value.
Return:
  none.
*/
static void
d_internal_transducer_step_tap
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    struct d_transducer_stage* stage;

    // a NULL chain or state is a caller error
    if ( (!_transducer) ||
         (!_state)      )
    {
        return;
    }

    stage = &_transducer->stages[_index];

    // the side effect is optional; the forwarding is not
    if (stage->callable.consume)
    {
        stage->callable.consume(_element, stage->context);
    }

    d_transducer_emit(_transducer, _index + 1, _state, _element);

    return;
}

/*
d_internal_transducer_entry_step
  The reducer step that feeds the head of a chain. This is what makes a whole
transducer chain indistinguishable from a plain reducer to every driver.

Parameter(s):
  _state:   the reducing state.
  _element: the incoming value.
  _context: the `d_transducer` whose head receives the value.
Return:
  none.
*/
static void
d_internal_transducer_entry_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    // a NULL chain is a caller error
    if (!_context)
    {
        return;
    }

    d_transducer_emit((struct d_transducer*)_context, 0, _state, _element);

    return;
}

/*
d_internal_array_sink_step
  Appends a value into a bounded array, counting rather than discarding
silently once the array is full.

Parameter(s):
  _state:   the reducing state; unused, since the sink carries its own.
  _element: the value to append.
  _context: the `d_array_sink` being filled.
Return:
  none.
*/
static void
d_internal_array_sink_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    struct d_array_sink* sink;

    (void)_state;

    // a NULL sink or value is a caller error
    if ( (!_context) ||
         (!_element) )
    {
        return;
    }

    sink = (struct d_array_sink*)_context;

    // a full sink records the loss instead of writing out of bounds
    if (sink->remaining == 0)
    {
        sink->overflow = sink->overflow + 1;

        return;
    }

    memcpy(sink->cursor, _element, sink->element_size);

    sink->cursor    = sink->cursor + sink->element_size;
    sink->remaining = sink->remaining - 1;
    sink->written   = sink->written + 1;

    return;
}

/*
d_internal_transducer_stage_blank
  Returns a zeroed stage, so every factory sets only the fields it needs.

Parameter(s):
  none.
Return:
  A stage by value with all fields cleared.
*/
static struct d_transducer_stage
d_internal_transducer_stage_blank
(
    void
)
{
    struct d_transducer_stage stage;

    stage.step               = NULL;
    stage.callable.predicate = NULL;
    stage.context            = NULL;
    stage.scratch            = NULL;
    stage.bound              = 0;
    stage.seen               = 0;
    stage.latched            = false;

    return stage;
}


/*
d_transducer_map
  Builds a mapping stage.

Parameter(s):
  _transform: the mapping applied to each value.
  _context:   context forwarded to `_transform`; may be NULL.
  _scratch:   caller-owned buffer for the mapped value; must be at least as
              large as the transform's output and must outlive the chain.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_map
(
    fn_transformer _transform,
    void*          _context,
    void*          _scratch
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step               = &d_internal_transducer_step_map;
    stage.callable.transform = _transform;
    stage.context            = _context;
    stage.scratch            = _scratch;

    return stage;
}

/*
d_transducer_filter
  Builds a stage passing through only the values a predicate admits.

Parameter(s):
  _predicate: the admission test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_filter
(
    fn_predicate _predicate,
    void*        _context
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step               = &d_internal_transducer_step_filter;
    stage.callable.predicate = _predicate;
    stage.context            = _context;

    return stage;
}

/*
d_transducer_filter_not
  Builds a stage passing through only the values a predicate rejects.

Parameter(s):
  _predicate: the rejection test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_filter_not
(
    fn_predicate _predicate,
    void*        _context
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step               = &d_internal_transducer_step_filter_not;
    stage.callable.predicate = _predicate;
    stage.context            = _context;

    return stage;
}

/*
d_transducer_take
  Builds a stage bounding the stream to its first `_count` values. A bound of 0
emits nothing and stops the drive at the first value.

Parameter(s):
  _count: the maximum number of values to pass through.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_take
(
    size_t _count
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step  = &d_internal_transducer_step_take;
    stage.bound = _count;

    return stage;
}

/*
d_transducer_drop
  Builds a stage discarding the first `_count` values of the stream.

Parameter(s):
  _count: the number of leading values to discard.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_drop
(
    size_t _count
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step  = &d_internal_transducer_step_drop;
    stage.bound = _count;

    return stage;
}

/*
d_transducer_take_while
  Builds a stage passing values through until the predicate first fails.

Parameter(s):
  _predicate: the continuation test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_take_while
(
    fn_predicate _predicate,
    void*        _context
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step               = &d_internal_transducer_step_take_while;
    stage.callable.predicate = _predicate;
    stage.context            = _context;

    return stage;
}

/*
d_transducer_drop_while
  Builds a stage discarding values until the predicate first fails.

Parameter(s):
  _predicate: the discard test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_drop_while
(
    fn_predicate _predicate,
    void*        _context
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step               = &d_internal_transducer_step_drop_while;
    stage.callable.predicate = _predicate;
    stage.context            = _context;

    return stage;
}

/*
d_transducer_tap
  Builds a stage that observes each value and forwards it unchanged.

Parameter(s):
  _consume: the side effect applied to each value.
  _context: context forwarded to `_consume`; may be NULL.
Return:
  A stage by value.
*/
struct d_transducer_stage
d_transducer_tap
(
    fn_consumer_const _consume,
    void*             _context
)
{
    struct d_transducer_stage stage;

    stage = d_internal_transducer_stage_blank();

    stage.step             = &d_internal_transducer_step_tap;
    stage.callable.consume = _consume;
    stage.context          = _context;

    return stage;
}

/*
d_transducer_init
  Composes a stage array into a chain terminating in a sink, and clears the
mutable run state of every stage.

Parameter(s):
  _transducer: the chain to initialise.
  _stages:     caller-owned stage array; may be NULL only if `_count` is 0.
  _count:      the number of stages; 0 makes the chain the identity.
  _sink:       the terminal reducer.
Return:
  A boolean value corresponding to either:
  - true, if the chain was initialised, or
  - false, if the parameters were unusable.
*/
bool
d_transducer_init
(
    struct d_transducer*       _transducer,
    struct d_transducer_stage* _stages,
    size_t                     _count,
    struct d_reducer           _sink
)
{
    // a chain needs somewhere to live and stages if it claims to have any
    if ( (!_transducer)                ||
         ( (!_stages) && (_count > 0) ) )
    {
        return false;
    }

    _transducer->stages = _stages;
    _transducer->count  = _count;
    _transducer->sink   = _sink;

    d_transducer_reset(_transducer);

    return true;
}

/*
d_transducer_reset
  Clears the mutable per-run state of every stage, making the chain drivable
again. Configuration is untouched.

Parameter(s):
  _transducer: the chain to reset; ignored if NULL.
Return:
  none.
*/
void
d_transducer_reset
(
    struct d_transducer* _transducer
)
{
    size_t index;

    // a NULL or stageless chain has no run state to clear
    if ( (!_transducer)         ||
         (!_transducer->stages) )
    {
        return;
    }

    // clear counters and latches, leaving callables and bounds in place
    for (index = 0; index < _transducer->count; index = index + 1)
    {
        _transducer->stages[index].seen    = 0;
        _transducer->stages[index].latched = false;
    }

    return;
}

/*
d_transducer_is_valid
  Reports whether a chain is drivable: it must have a sink, and every stage it
claims must carry a step.

Parameter(s):
  _transducer: the chain to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the chain is drivable, or
  - false, otherwise.
*/
bool
d_transducer_is_valid
(
    const struct d_transducer* _transducer
)
{
    size_t index;

    // a NULL chain, or one without a sink, is not drivable
    if ( (!_transducer)                       ||
         (!d_reducer_is_valid(&_transducer->sink)) )
    {
        return false;
    }

    // a claimed stage without a step would silently swallow the stream
    for (index = 0; index < _transducer->count; index = index + 1)
    {
        if (!_transducer->stages[index].step)
        {
            return false;
        }
    }

    return true;
}

/*
d_transducer_emit
  Hands a value to stage `_index` of a chain, or to the sink when the index is
past the last stage. Stages call this to pass values downstream; drivers do not
call it directly.

Parameter(s):
  _transducer: the chain being run.
  _index:      the stage to receive the value.
  _state:      the reducing state.
  _element:    the value to hand on.
Return:
  none.
*/
void
d_transducer_emit
(
    struct d_transducer*     _transducer,
    size_t                   _index,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    // a latched state stops the chain wherever it currently is
    if ( (!_transducer) ||
         (!_state)      ||
         (_state->done) )
    {
        return;
    }

    // past the last stage lies the sink
    if (_index >= _transducer->count)
    {
        if (_transducer->sink.step)
        {
            _transducer->sink.step(_state,
                                   _element,
                                   _transducer->sink.context);
        }

        return;
    }

    // a stage without a step drops the value rather than crashing
    if (_transducer->stages[_index].step)
    {
        _transducer->stages[_index].step(_transducer,
                                         _index,
                                         _state,
                                         _element);
    }

    return;
}

/*
d_transducer_into_reducer
  Collapses a whole chain into a single reducer. The returned reducer borrows
the chain, which must outlive it.

Parameter(s):
  _transducer: the chain to wrap; may be NULL.
Return:
  A reducer by value; an invalid reducer if `_transducer` is NULL.
*/
struct d_reducer
d_transducer_into_reducer
(
    struct d_transducer* _transducer
)
{
    // a NULL chain has nothing to wrap
    if (!_transducer)
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_reducer_make(&d_internal_transducer_entry_step, _transducer);
}

/*
d_array_sink_init
  Initialises an array sink over caller-owned storage.

Parameter(s):
  _sink:         the sink state to initialise.
  _out_array:    the destination array; may be NULL only if `_capacity` is 0.
  _capacity:     the number of values the array can hold.
  _element_size: the stride between slots in bytes; must be non-zero.
Return:
  A boolean value corresponding to either:
  - true, if the sink was initialised, or
  - false, if the parameters were unusable.
*/
bool
d_array_sink_init
(
    struct d_array_sink* _sink,
    void*                _out_array,
    size_t               _capacity,
    size_t               _element_size
)
{
    // an unusable request leaves the sink untouched
    if ( (!_sink)                            ||
         (_element_size == 0)                ||
         ( (!_out_array) && (_capacity > 0) ) )
    {
        return false;
    }

    _sink->cursor       = (unsigned char*)_out_array;
    _sink->remaining    = _capacity;
    _sink->element_size = _element_size;
    _sink->written      = 0;
    _sink->overflow     = 0;

    return true;
}

/*
d_reducer_from_array_sink
  Lifts an array sink into a reducer. The sink is borrowed, not copied.

Parameter(s):
  _sink: the caller-owned sink state; may be NULL.
Return:
  A reducer by value; an invalid reducer if `_sink` is NULL.
*/
struct d_reducer
d_reducer_from_array_sink
(
    struct d_array_sink* _sink
)
{
    // a NULL sink has nowhere to append
    if (!_sink)
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_reducer_make(&d_internal_array_sink_step, _sink);
}

/*
d_transduce_array
  Pushes a contiguous array through a chain.

Parameter(s):
  _transducer:   the chain to drive.
  _state:        the reducing state.
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to deliver.
  _element_size: the stride between elements in bytes; must be non-zero.
Return:
  The number of input elements delivered to the chain, which may be fewer than
`_count` if a stage latched termination.
*/
size_t
d_transduce_array
(
    struct d_transducer*     _transducer,
    struct d_reducing_state* _state,
    const void*              _elements,
    size_t                   _count,
    size_t                   _element_size
)
{
    struct d_reducer reducer;

    // an undrivable chain consumes nothing
    if (!d_transducer_is_valid(_transducer))
    {
        return 0;
    }

    reducer = d_transducer_into_reducer(_transducer);

    return d_reducer_drive_array(&reducer,
                                 _state,
                                 _elements,
                                 _count,
                                 _element_size);
}

/*
d_transduce_producer
  Pushes every value of a producer through a chain. The same chain would serve
an array without modification; only the driver differs.

Parameter(s):
  _transducer: the chain to drive.
  _producer:   the source to drain.
  _state:      the reducing state.
  _scratch:    caller-owned buffer of at least `_producer->value_size` bytes.
Return:
  The number of values pulled from the producer.
*/
size_t
d_transduce_producer
(
    struct d_transducer*     _transducer,
    struct d_producer*       _producer,
    struct d_reducing_state* _state,
    void*                    _scratch
)
{
    struct d_reducer reducer;

    // an undrivable chain consumes nothing
    if (!d_transducer_is_valid(_transducer))
    {
        return 0;
    }

    reducer = d_transducer_into_reducer(_transducer);

    return d_producer_drive(_producer, &reducer, _state, _scratch);
}

/*
d_transduce_into_array
  Runs a stage array over a contiguous input and collects the surviving values
into a bounded output array.

Parameter(s):
  _stages:       caller-owned stage array; may be NULL only if count is 0.
  _stage_count:  the number of stages.
  _elements:     the base of the input array.
  _count:        the number of input elements.
  _element_size: the stride between elements in bytes; must be non-zero.
  _out_array:    the destination array.
  _capacity:     the number of values the destination can hold.
  _out_written:  receives the number of values written; may be NULL.
Return:
  The number of input elements consumed.
*/
size_t
d_transduce_into_array
(
    struct d_transducer_stage* _stages,
    size_t                     _stage_count,
    const void*                _elements,
    size_t                     _count,
    size_t                     _element_size,
    void*                      _out_array,
    size_t                     _capacity,
    size_t*                    _out_written
)
{
    struct d_array_sink     sink;
    struct d_transducer     transducer;
    struct d_reducing_state state;
    size_t                  consumed;

    // report nothing written up front, so every early exit is consistent
    if (_out_written)
    {
        *_out_written = 0;
    }

    // an unusable request consumes nothing
    if (!d_array_sink_init(&sink, _out_array, _capacity, _element_size))
    {
        return 0;
    }

    if (!d_transducer_init(&transducer,
                           _stages,
                           _stage_count,
                           d_reducer_from_array_sink(&sink)))
    {
        return 0;
    }

    d_reducing_state_init(&state, NULL);

    consumed = d_transduce_array(&transducer,
                                 &state,
                                 _elements,
                                 _count,
                                 _element_size);

    // hand back how much of the output array was filled
    if (_out_written)
    {
        *_out_written = sink.written;
    }

    return consumed;
}

/*
d_transduce_producer_to_consumer
  Runs a chain from a producer straight into a consumer, with no intermediate
storage for the stream itself.

Parameter(s):
  _transducer: the chain to drive.
  _producer:   the source to drain.
  _consume:    the sink applied to each surviving value.
  _context:    context forwarded to `_consume`; may be NULL.
  _scratch:    caller-owned buffer of at least `_producer->value_size` bytes.
Return:
  The number of values pulled from the producer.
*/
size_t
d_transduce_producer_to_consumer
(
    struct d_transducer* _transducer,
    struct d_producer*   _producer,
    fn_consumer_const    _consume,
    void*                _context,
    void*                _scratch
)
{
    struct d_consumer_binding binding;
    struct d_reducing_state   state;

    // an unusable request consumes nothing
    if ( (!_transducer) ||
         (!_consume)    )
    {
        return 0;
    }

    binding.consume = _consume;
    binding.context = _context;

    _transducer->sink = d_reducer_from_consumer(&binding);

    d_transducer_reset(_transducer);
    d_reducing_state_init(&state, NULL);

    return d_transduce_producer(_transducer, _producer, &state, _scratch);
}

/*
d_filter_into_array
  Filters a contiguous array into a bounded output array. This is the whole of
"a filter is a predicate plus a sink": one filter stage, one array sink.

Parameter(s):
  _elements:     the base of the input array.
  _count:        the number of input elements.
  _element_size: the stride between elements in bytes; must be non-zero.
  _predicate:    the admission test.
  _context:      context forwarded to `_predicate`; may be NULL.
  _out_array:    the destination array.
  _capacity:     the number of values the destination can hold.
Return:
  The number of values written to `_out_array`, which never exceeds the lesser
of `_count` and `_capacity`.
*/
size_t
d_filter_into_array
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context,
    void*        _out_array,
    size_t       _capacity
)
{
    struct d_transducer_stage stages[1];
    size_t                    written;

    // an unusable request writes nothing
    if (!_predicate)
    {
        return 0;
    }

    stages[0] = d_transducer_filter(_predicate, _context);
    written   = 0;

    d_transduce_into_array(stages,
                           1,
                           _elements,
                           _count,
                           _element_size,
                           _out_array,
                           _capacity,
                           &written);

    return written;
}

/*
d_filter_to_consumer
  Filters a contiguous array straight into a consumer, with no intermediate
storage.

Parameter(s):
  _elements:          the base of the input array.
  _count:             the number of input elements.
  _element_size:      the stride between elements in bytes; must be non-zero.
  _predicate:         the admission test.
  _predicate_context: context forwarded to `_predicate`; may be NULL.
  _consume:           the sink applied to each admitted value.
  _consumer_context:  context forwarded to `_consume`; may be NULL.
Return:
  The number of input elements consumed.
*/
size_t
d_filter_to_consumer
(
    const void*       _elements,
    size_t            _count,
    size_t            _element_size,
    fn_predicate      _predicate,
    void*             _predicate_context,
    fn_consumer_const _consume,
    void*             _consumer_context
)
{
    struct d_transducer_stage stages[1];
    struct d_consumer_binding binding;
    struct d_transducer       transducer;
    struct d_reducing_state   state;

    // an unusable request consumes nothing
    if ( (!_predicate) ||
         (!_consume)   )
    {
        return 0;
    }

    stages[0] = d_transducer_filter(_predicate, _predicate_context);

    binding.consume = _consume;
    binding.context = _consumer_context;

    if (!d_transducer_init(&transducer,
                           stages,
                           1,
                           d_reducer_from_consumer(&binding)))
    {
        return 0;
    }

    d_reducing_state_init(&state, NULL);

    return d_transduce_array(&transducer,
                             &state,
                             _elements,
                             _count,
                             _element_size);
}
