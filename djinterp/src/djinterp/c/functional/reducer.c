/*******************************************************************************
* djinterp [c]                                                         reducer.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/reducer.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/reducer.h"


/*
d_internal_reducer_fold_step
  Adapts a plain `fn_fold` to the `fn_reducer_step` shape. A plain fold has no
access to the termination latch, so this trampoline never sets it.

Parameter(s):
  _state:   the reducing state whose accumulator is folded into.
  _element: the element being folded.
  _context: the `d_fold_binding` carrying the fold step and its context.
Return:
  none.
*/
static void
d_internal_reducer_fold_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    struct d_fold_binding* binding;

    // a NULL state or binding is a caller error, not a fold outcome
    if ( (!_state)   ||
         (!_context) )
    {
        return;
    }

    binding = (struct d_fold_binding*)_context;

    // an unset step is a no-op rather than a crash
    if (binding->step)
    {
        binding->step(_state->accumulator,
                      _element,
                      binding->context);
    }

    return;
}

/*
d_internal_reducer_consumer_step
  Adapts a consumer to the `fn_reducer_step` shape. A consumer ignores the
accumulator entirely and never terminates early, so it is the canonical sink.

Parameter(s):
  _state:   the reducing state; its accumulator is unused.
  _element: the element handed to the sink.
  _context: the `d_consumer_binding` carrying the sink and its context.
Return:
  none.
*/
static void
d_internal_reducer_consumer_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    struct d_consumer_binding* binding;

    (void)_state;

    // a NULL binding is a caller error
    if (!_context)
    {
        return;
    }

    binding = (struct d_consumer_binding*)_context;

    // an unset sink discards the element
    if (binding->consume)
    {
        binding->consume(_element, binding->context);
    }

    return;
}

/*
d_internal_reducer_quantifier_step
  Folds one element under a quantifier, setting the termination latch as soon
as the answer is decided. This is the whole of "a quantifier is a fold with
short-circuit": the only difference from an ordinary fold is the assignment to
`_state->done`.

Parameter(s):
  _state:   the reducing state; its accumulator must point at a `bool`.
  _element: the element being tested.
  _context: the `d_quantifier` carrying the predicate and the quantifier kind.
Return:
  none.
*/
static void
d_internal_reducer_quantifier_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    struct d_quantifier* quantifier;
    bool*                result;
    bool                 satisfied;

    // a NULL state or quantifier is a caller error
    if ( (!_state)   ||
         (!_context) )
    {
        return;
    }

    quantifier = (struct d_quantifier*)_context;
    result     = (bool*)_state->accumulator;

    // without a predicate or an accumulator there is no answer to fold into
    if ( (!quantifier->predicate) ||
         (!result)                )
    {
        return;
    }

    satisfied = quantifier->predicate(_element, quantifier->context);

    // decide the answer and latch termination the moment it is known
    switch (quantifier->kind)
    {
        case D_QUANTIFIER_ANY:
        {
            if (satisfied)
            {
                *result      = true;
                _state->done = true;
            }

            break;
        }

        case D_QUANTIFIER_ALL:
        {
            if (!satisfied)
            {
                *result      = false;
                _state->done = true;
            }

            break;
        }

        case D_QUANTIFIER_NONE:
        {
            if (satisfied)
            {
                *result      = false;
                _state->done = true;
            }

            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

/*
d_internal_reducer_count_step
  Counts elements satisfying a predicate. Runs to exhaustion: a count has no
decidable early answer.

Parameter(s):
  _state:   the reducing state; its accumulator must point at a `size_t`.
  _element: the element being tested.
  _context: the `d_quantifier` whose predicate selects countable elements.
Return:
  none.
*/
static void
d_internal_reducer_count_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    struct d_quantifier* quantifier;
    size_t*              total;

    // a NULL state or quantifier is a caller error
    if ( (!_state)   ||
         (!_context) )
    {
        return;
    }

    quantifier = (struct d_quantifier*)_context;
    total      = (size_t*)_state->accumulator;

    // without a predicate or an accumulator there is nothing to count into
    if ( (!quantifier->predicate) ||
         (!total)                 )
    {
        return;
    }

    // count only the elements the predicate admits
    if (quantifier->predicate(_element, quantifier->context))
    {
        *total = *total + 1;
    }

    return;
}

/*
d_internal_reducer_quantify
  Shared driver for the three quantifiers: seeds the accumulator from the
quantifier kind, drives the array through the short-circuiting reducer, and
returns the decided answer.

Parameter(s):
  _elements:     the base of the array to quantify over.
  _count:        the number of elements; 0 yields the vacuous answer.
  _element_size: the stride between elements in bytes; must be non-zero.
  _predicate:    the test applied to each element.
  _context:      context forwarded to `_predicate`; may be NULL.
  _kind:         which quantifier to fold.
Return:
  A boolean value corresponding to either:
  - the quantified answer over the array, or
  - the vacuous answer for `_kind` if the parameters are unusable.
*/
static bool
d_internal_reducer_quantify
(
    const void*            _elements,
    size_t                 _count,
    size_t                 _element_size,
    fn_predicate           _predicate,
    void*                  _context,
    enum d_quantifier_kind _kind
)
{
    struct d_quantifier     quantifier;
    struct d_reducer        reducer;
    struct d_reducing_state state;
    bool                    answer;

    answer = d_quantifier_seed(_kind);

    // an unusable request yields the vacuous answer rather than an error
    if ( (!_predicate)         ||
         (_element_size == 0)  ||
         ( (!_elements) && (_count > 0) ) )
    {
        return answer;
    }

    quantifier.predicate = _predicate;
    quantifier.context   = _context;
    quantifier.kind      = _kind;

    reducer = d_reducer_from_quantifier(&quantifier);

    d_reducing_state_init(&state, &answer);
    d_reducer_drive_array(&reducer,
                          &state,
                          _elements,
                          _count,
                          _element_size);

    return answer;
}


/*
d_reducing_state_init
  Initialises a reducing state over a caller-owned accumulator, clearing the
termination latch.

Parameter(s):
  _state:       the state to initialise; ignored if NULL.
  _accumulator: caller-owned accumulator storage; may be NULL for sinks.
Return:
  none.
*/
void
d_reducing_state_init
(
    struct d_reducing_state* _state,
    void*                    _accumulator
)
{
    // a NULL state is a caller error
    if (!_state)
    {
        return;
    }

    _state->accumulator = _accumulator;
    _state->done        = false;

    return;
}

/*
d_reducing_state_mark_done
  Sets the termination latch, asking the driver to stop after this step.

Parameter(s):
  _state: the state to latch; ignored if NULL.
Return:
  none.
*/
void
d_reducing_state_mark_done
(
    struct d_reducing_state* _state
)
{
    // a NULL state is a caller error
    if (!_state)
    {
        return;
    }

    _state->done = true;

    return;
}

/*
d_reducing_state_is_done
  Reports whether the termination latch is set.

Parameter(s):
  _state: the state to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the state is non-NULL and its latch is set, or
  - false, otherwise.
*/
bool
d_reducing_state_is_done
(
    const struct d_reducing_state* _state
)
{
    // a NULL state is treated as not-done so drivers fail safe
    if (!_state)
    {
        return false;
    }

    return _state->done;
}

/*
d_reducing_state_accumulator
  Returns the accumulator storage carried by a reducing state.

Parameter(s):
  _state: the state to inspect; may be NULL.
Return:
  The accumulator pointer, or NULL if `_state` is NULL.
*/
void*
d_reducing_state_accumulator
(
    const struct d_reducing_state* _state
)
{
    // a NULL state carries no accumulator
    if (!_state)
    {
        return NULL;
    }

    return _state->accumulator;
}

/*
d_reducer_make
  Builds a reducer from a step and a bound context.

Parameter(s):
  _step:    the folding step; may be NULL, yielding an invalid reducer.
  _context: context forwarded to `_step`; may be NULL.
Return:
  A reducer by value.
*/
struct d_reducer
d_reducer_make
(
    fn_reducer_step _step,
    void*           _context
)
{
    struct d_reducer reducer;

    reducer.step    = _step;
    reducer.context = _context;

    return reducer;
}

/*
d_reducer_from_fold
  Lifts a plain fold step into a reducer. The binding is borrowed, not copied,
so it must outlive every reducer derived from it.

Parameter(s):
  _binding: the caller-owned fold binding; may be NULL.
Return:
  A reducer by value; an invalid reducer if `_binding` is NULL.
*/
struct d_reducer
d_reducer_from_fold
(
    struct d_fold_binding* _binding
)
{
    // a NULL binding has no step to lift
    if (!_binding)
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_reducer_make(&d_internal_reducer_fold_step, _binding);
}

/*
d_reducer_from_consumer
  Lifts a consumer into a reducer, giving the canonical sink. The binding is
borrowed, not copied.

Parameter(s):
  _binding: the caller-owned consumer binding; may be NULL.
Return:
  A reducer by value; an invalid reducer if `_binding` is NULL.
*/
struct d_reducer
d_reducer_from_consumer
(
    struct d_consumer_binding* _binding
)
{
    // a NULL binding has no sink to lift
    if (!_binding)
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_reducer_make(&d_internal_reducer_consumer_step, _binding);
}

/*
d_reducer_from_quantifier
  Lifts a quantifier into a short-circuiting reducer whose accumulator is a
`bool`. The quantifier is borrowed, not copied.

Parameter(s):
  _quantifier: the caller-owned quantifier; may be NULL.
Return:
  A reducer by value; an invalid reducer if `_quantifier` is NULL.
*/
struct d_reducer
d_reducer_from_quantifier
(
    struct d_quantifier* _quantifier
)
{
    // a NULL quantifier has no predicate to fold
    if (!_quantifier)
    {
        return d_reducer_make(NULL, NULL);
    }

    return d_reducer_make(&d_internal_reducer_quantifier_step, _quantifier);
}

/*
d_reducer_is_valid
  Reports whether a reducer carries a callable step.

Parameter(s):
  _reducer: the reducer to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the reducer is non-NULL and its step is set, or
  - false, otherwise.
*/
bool
d_reducer_is_valid
(
    const struct d_reducer* _reducer
)
{
    // a NULL reducer is not valid
    if (!_reducer)
    {
        return false;
    }

    return (_reducer->step != NULL);
}

/*
d_reducer_step_once
  Applies a reducer to a single element. Does nothing once the state's latch is
set, so callers may step unconditionally.

Parameter(s):
  _reducer: the reducer to apply.
  _state:   the reducing state to fold into.
  _element: the element to fold.
Return:
  none.
*/
void
d_reducer_step_once
(
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state,
    const void*              _element
)
{
    // an invalid reducer, a missing state, or a latched state is a no-op
    if ( (!d_reducer_is_valid(_reducer)) ||
         (!_state)                       ||
         (_state->done)                  )
    {
        return;
    }

    _reducer->step(_state, _element, _reducer->context);

    return;
}

/*
d_reducer_drive_array
  Pushes a contiguous array through a reducer in ascending order, stopping as
soon as the reducing state latches.

Parameter(s):
  _reducer:      the reducer to drive.
  _state:        the reducing state to fold into.
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to deliver.
  _element_size: the stride between elements in bytes; must be non-zero.
Return:
  The number of elements delivered to the reducer. A result less than `_count`
means the drive short-circuited or the parameters were unusable.
*/
size_t
d_reducer_drive_array
(
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state,
    const void*              _elements,
    size_t                   _count,
    size_t                   _element_size
)
{
    const unsigned char* cursor;
    size_t               delivered;

    // an unusable request delivers nothing
    if ( (!d_reducer_is_valid(_reducer))    ||
         (!_state)                          ||
         (_element_size == 0)               ||
         ( (!_elements) && (_count > 0) )   )
    {
        return 0;
    }

    cursor    = (const unsigned char*)_elements;
    delivered = 0;

    // deliver in order, honouring the termination latch between elements
    while ( (delivered < _count)  &&
            (!_state->done)       )
    {
        _reducer->step(_state,
                       (const void*)cursor,
                       _reducer->context);

        cursor    = cursor + _element_size;
        delivered = delivered + 1;
    }

    return delivered;
}

/*
d_reducer_drive_array_reverse
  Pushes a contiguous array through a reducer in descending order, stopping as
soon as the reducing state latches. This is the driver a right fold uses.

Parameter(s):
  _reducer:      the reducer to drive.
  _state:        the reducing state to fold into.
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to deliver.
  _element_size: the stride between elements in bytes; must be non-zero.
Return:
  The number of elements delivered to the reducer.
*/
size_t
d_reducer_drive_array_reverse
(
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state,
    const void*              _elements,
    size_t                   _count,
    size_t                   _element_size
)
{
    const unsigned char* base;
    size_t               remaining;
    size_t               delivered;

    // an unusable request delivers nothing
    if ( (!d_reducer_is_valid(_reducer))    ||
         (!_state)                          ||
         (_element_size == 0)               ||
         ( (!_elements) && (_count > 0) )   )
    {
        return 0;
    }

    base      = (const unsigned char*)_elements;
    remaining = _count;
    delivered = 0;

    // walk backwards from the last element, honouring the latch
    while ( (remaining > 0)   &&
            (!_state->done)   )
    {
        remaining = remaining - 1;

        _reducer->step(_state,
                       (const void*)(base + (remaining * _element_size)),
                       _reducer->context);

        delivered = delivered + 1;
    }

    return delivered;
}

/*
d_fold_left
  Folds an array left-to-right through a plain fold step.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to fold.
  _element_size: the stride between elements in bytes; must be non-zero.
  _accumulator:  caller-owned accumulator storage.
  _step:         the fold step.
  _context:      context forwarded to `_step`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if every element was folded, or
  - false, if the parameters were unusable.
*/
bool
d_fold_left
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    void*       _accumulator,
    fn_fold     _step,
    void*       _context
)
{
    struct d_fold_binding   binding;
    struct d_reducer        reducer;
    struct d_reducing_state state;
    size_t                  delivered;

    // an unusable request folds nothing and reports failure
    if ( (!_step)                        ||
         (!_accumulator)                 ||
         (_element_size == 0)            ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    binding.step    = _step;
    binding.context = _context;

    reducer = d_reducer_from_fold(&binding);

    d_reducing_state_init(&state, _accumulator);

    delivered = d_reducer_drive_array(&reducer,
                                      &state,
                                      _elements,
                                      _count,
                                      _element_size);

    return (delivered == _count);
}

/*
d_fold_right
  Folds an array right-to-left through a plain fold step. The step still
receives the accumulator first; only the visitation order is reversed.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to fold.
  _element_size: the stride between elements in bytes; must be non-zero.
  _accumulator:  caller-owned accumulator storage.
  _step:         the fold step.
  _context:      context forwarded to `_step`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if every element was folded, or
  - false, if the parameters were unusable.
*/
bool
d_fold_right
(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    void*       _accumulator,
    fn_fold     _step,
    void*       _context
)
{
    struct d_fold_binding   binding;
    struct d_reducer        reducer;
    struct d_reducing_state state;
    size_t                  delivered;

    // an unusable request folds nothing and reports failure
    if ( (!_step)                        ||
         (!_accumulator)                 ||
         (_element_size == 0)            ||
         ( (!_elements) && (_count > 0) ) )
    {
        return false;
    }

    binding.step    = _step;
    binding.context = _context;

    reducer = d_reducer_from_fold(&binding);

    d_reducing_state_init(&state, _accumulator);

    delivered = d_reducer_drive_array_reverse(&reducer,
                                              &state,
                                              _elements,
                                              _count,
                                              _element_size);

    return (delivered == _count);
}

/*
d_quantifier_seed
  Returns the vacuous answer for a quantifier, which is also the value its
accumulator must start from.

Parameter(s):
  _kind: the quantifier kind.
Return:
  A boolean value corresponding to either:
  - false, for D_QUANTIFIER_ANY, or
  - true, for D_QUANTIFIER_ALL and D_QUANTIFIER_NONE.
*/
bool
d_quantifier_seed
(
    enum d_quantifier_kind _kind
)
{
    // an existential starts false; the two universals start true
    if (_kind == D_QUANTIFIER_ANY)
    {
        return false;
    }

    return true;
}

/*
d_reducer_any
  Reports whether any element of an array satisfies a predicate. Folds with
short-circuit: stops at the first satisfying element.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to test.
  _element_size: the stride between elements in bytes; must be non-zero.
  _predicate:    the test applied to each element.
  _context:      context forwarded to `_predicate`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if some element satisfied the predicate, or
  - false, if none did or the parameters were unusable.
*/
bool
d_reducer_any
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context
)
{
    return d_internal_reducer_quantify(_elements,
                                       _count,
                                       _element_size,
                                       _predicate,
                                       _context,
                                       D_QUANTIFIER_ANY);
}

/*
d_reducer_all
  Reports whether every element of an array satisfies a predicate. Vacuously
true over an empty array; stops at the first counterexample.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to test.
  _element_size: the stride between elements in bytes; must be non-zero.
  _predicate:    the test applied to each element.
  _context:      context forwarded to `_predicate`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if every element satisfied the predicate or the array was empty, or
  - false, if some element did not.
*/
bool
d_reducer_all
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context
)
{
    return d_internal_reducer_quantify(_elements,
                                       _count,
                                       _element_size,
                                       _predicate,
                                       _context,
                                       D_QUANTIFIER_ALL);
}

/*
d_reducer_none
  Reports whether no element of an array satisfies a predicate. Vacuously true
over an empty array; stops at the first satisfying element.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to test.
  _element_size: the stride between elements in bytes; must be non-zero.
  _predicate:    the test applied to each element.
  _context:      context forwarded to `_predicate`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if no element satisfied the predicate, or
  - false, if some element did.
*/
bool
d_reducer_none
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context
)
{
    return d_internal_reducer_quantify(_elements,
                                       _count,
                                       _element_size,
                                       _predicate,
                                       _context,
                                       D_QUANTIFIER_NONE);
}

/*
d_reducer_count_if
  Counts the elements of an array satisfying a predicate. Runs to exhaustion,
since a count has no decidable early answer.

Parameter(s):
  _elements:     the base of the array; may be NULL only if `_count` is 0.
  _count:        the number of elements to test.
  _element_size: the stride between elements in bytes; must be non-zero.
  _predicate:    the test selecting countable elements.
  _context:      context forwarded to `_predicate`; may be NULL.
Return:
  The number of satisfying elements, or 0 if the parameters were unusable.
*/
size_t
d_reducer_count_if
(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context
)
{
    struct d_quantifier     quantifier;
    struct d_reducer        reducer;
    struct d_reducing_state state;
    size_t                  total;

    // an unusable request counts nothing
    if ( (!_predicate)                   ||
         (_element_size == 0)            ||
         ( (!_elements) && (_count > 0) ) )
    {
        return 0;
    }

    quantifier.predicate = _predicate;
    quantifier.context   = _context;
    quantifier.kind      = D_QUANTIFIER_ANY;

    total = 0;

    reducer = d_reducer_make(&d_internal_reducer_count_step, &quantifier);

    d_reducing_state_init(&state, &total);
    d_reducer_drive_array(&reducer,
                          &state,
                          _elements,
                          _count,
                          _element_size);

    return total;
}
