/*******************************************************************************
* djinterp [c]                                                        foldable.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/foldable.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/foldable.h"


/*
d_internal_foldable_drive_array
  The Foldable instance for a plain array.

Parameter(s):
  _carrier: a `d_array_carrier`.
  _reducer: the reducer to fold into.
  _state:   the reducing state.
Return:
  The number of elements delivered.
*/
static size_t
d_internal_foldable_drive_array
(
    const void*              _carrier,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    const struct d_array_carrier* carrier;

    // a NULL carrier delivers nothing
    if (!_carrier)
    {
        return 0;
    }

    carrier = (const struct d_array_carrier*)_carrier;

    return d_reducer_drive_array(_reducer,
                                 _state,
                                 carrier->elements,
                                 carrier->count,
                                 carrier->element_size);
}

/*
d_internal_foldable_drive_maybe
  The Foldable instance for `maybe`: 0 or 1 elements.

Parameter(s):
  _carrier: a `struct d_maybe`.
  _reducer: the reducer to fold into.
  _state:   the reducing state.
Return:
  1 for a just, 0 for a nothing.
*/
static size_t
d_internal_foldable_drive_maybe
(
    const void*              _carrier,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    // a NULL carrier delivers nothing
    if (!_carrier)
    {
        return 0;
    }

    return d_maybe_drive((const struct d_maybe*)_carrier, _reducer, _state);
}

/*
d_internal_foldable_drive_result
  The Foldable instance for `result`: the success arm only.

Parameter(s):
  _carrier: a `struct d_result`.
  _reducer: the reducer to fold into.
  _state:   the reducing state.
Return:
  1 for an ok, 0 for an err.
*/
static size_t
d_internal_foldable_drive_result
(
    const void*              _carrier,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    // a NULL carrier delivers nothing
    if (!_carrier)
    {
        return 0;
    }

    return d_result_drive((const struct d_result*)_carrier, _reducer, _state);
}

/*
d_internal_foldable_drive_producer
  The Foldable instance for a producer. Draining is destructive, so a producer
may be folded only once -- which is a property of the carrier, not of Foldable.

Parameter(s):
  _carrier: a `d_producer_carrier`.
  _reducer: the reducer to fold into.
  _state:   the reducing state.
Return:
  The number of values pulled.
*/
static size_t
d_internal_foldable_drive_producer
(
    const void*              _carrier,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    const struct d_producer_carrier* carrier;

    // a NULL carrier delivers nothing
    if (!_carrier)
    {
        return 0;
    }

    carrier = (const struct d_producer_carrier*)_carrier;

    return d_producer_drive(carrier->producer,
                            _reducer,
                            _state,
                            carrier->scratch);
}

/*
d_internal_foldable_count_step
  Counts everything delivered, which is how `length` is expressed as a fold.

Parameter(s):
  _state:   the reducing state; its accumulator must point at a `size_t`.
  _element: the incoming element, unused beyond its arrival.
  _context: unused.
Return:
  none.
*/
static void
d_internal_foldable_count_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    (void)_element;
    (void)_context;

    // a NULL state or accumulator is a caller error
    if ( (!_state)               ||
         (!_state->accumulator)  )
    {
        return;
    }

    *(size_t*)_state->accumulator = (*(size_t*)_state->accumulator) + 1;

    return;
}

/*
d_internal_foldable_first_step
  Latches on the first element delivered, which is how `is_empty` avoids
draining the whole carrier.

Parameter(s):
  _state:   the reducing state; its accumulator must point at a `bool`.
  _element: the incoming element, unused beyond its arrival.
  _context: unused.
Return:
  none.
*/
static void
d_internal_foldable_first_step
(
    struct d_reducing_state* _state,
    const void*              _element,
    void*                    _context
)
{
    (void)_element;
    (void)_context;

    // a NULL state or accumulator is a caller error
    if ( (!_state)              ||
         (!_state->accumulator) )
    {
        return;
    }

    *(bool*)_state->accumulator = false;

    _state->done = true;

    return;
}

/*
d_internal_alternative_alt_maybe
  The Alternative choice for `maybe`.

Parameter(s):
  _first:  a `struct d_maybe`.
  _second: a `struct d_maybe`.
  _out:    a `struct d_maybe` to write through.
Return:
  A boolean value corresponding to whether the result holds a value.
*/
static bool
d_internal_alternative_alt_maybe
(
    const void* _first,
    const void* _second,
    void*       _out
)
{
    return d_maybe_alt((const struct d_maybe*)_first,
                       (const struct d_maybe*)_second,
                       (struct d_maybe*)_out);
}

/*
d_internal_alternative_aempty_maybe
  The Alternative identity for `maybe`.

Parameter(s):
  _out: a `struct d_maybe` to write through.
Return:
  A boolean value corresponding to whether the cell was marked empty.
*/
static bool
d_internal_alternative_aempty_maybe
(
    void* _out
)
{
    return d_maybe_aempty((struct d_maybe*)_out);
}


/*
d_foldable_array
  The Foldable dictionary for a plain array. Its carrier is a
`d_array_carrier`.

Parameter(s):
  _element_size: the width of a yielded element.
Return:
  A dictionary by value.
*/
struct d_foldable
d_foldable_array
(
    size_t _element_size
)
{
    struct d_foldable foldable;

    foldable.drive        = &d_internal_foldable_drive_array;
    foldable.element_size = _element_size;

    return foldable;
}

/*
d_foldable_maybe
  The Foldable dictionary for `maybe`. Its carrier is a `struct d_maybe`.

Parameter(s):
  _element_size: the width of a yielded element.
Return:
  A dictionary by value.
*/
struct d_foldable
d_foldable_maybe
(
    size_t _element_size
)
{
    struct d_foldable foldable;

    foldable.drive        = &d_internal_foldable_drive_maybe;
    foldable.element_size = _element_size;

    return foldable;
}

/*
d_foldable_result
  The Foldable dictionary for `result`. Its carrier is a `struct d_result`, and
it yields the success arm only -- an error is an empty sequence, not a failure.

Parameter(s):
  _element_size: the width of a yielded element.
Return:
  A dictionary by value.
*/
struct d_foldable
d_foldable_result
(
    size_t _element_size
)
{
    struct d_foldable foldable;

    foldable.drive        = &d_internal_foldable_drive_result;
    foldable.element_size = _element_size;

    return foldable;
}

/*
d_foldable_producer
  The Foldable dictionary for a producer. Its carrier is a
`d_producer_carrier`, and folding it consumes it.

Parameter(s):
  _element_size: the width of a yielded element.
Return:
  A dictionary by value.
*/
struct d_foldable
d_foldable_producer
(
    size_t _element_size
)
{
    struct d_foldable foldable;

    foldable.drive        = &d_internal_foldable_drive_producer;
    foldable.element_size = _element_size;

    return foldable;
}

/*
d_foldable_is_valid
  Reports whether a dictionary carries a usable drive.

Parameter(s):
  _foldable: the dictionary to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the drive is set and the element width is non-zero, or
  - false, otherwise.
*/
bool
d_foldable_is_valid
(
    const struct d_foldable* _foldable
)
{
    // a NULL dictionary is not valid
    if (!_foldable)
    {
        return false;
    }

    return ( (_foldable->drive != NULL) &&
             (_foldable->element_size != 0) );
}

/*
d_foldable_drive
  Pushes a carrier's elements at a reducer through its dictionary. Every other
operation in this header is this call plus a tier 1 reducer.

Parameter(s):
  _foldable: the carrier's dictionary.
  _carrier:  the carrier, of whatever type the dictionary expects.
  _reducer:  the reducer to fold into.
  _state:    the reducing state.
Return:
  The number of elements delivered.
*/
size_t
d_foldable_drive
(
    const struct d_foldable* _foldable,
    const void*              _carrier,
    const struct d_reducer*  _reducer,
    struct d_reducing_state* _state
)
{
    // an unusable dictionary delivers nothing
    if (!d_foldable_is_valid(_foldable))
    {
        return 0;
    }

    return _foldable->drive(_carrier, _reducer, _state);
}

/*
d_foldable_length
  Counts a carrier's elements, as a fold. For a producer this drains it.

Parameter(s):
  _foldable: the carrier's dictionary.
  _carrier:  the carrier.
Return:
  The number of elements the carrier yielded.
*/
size_t
d_foldable_length
(
    const struct d_foldable* _foldable,
    const void*              _carrier
)
{
    struct d_reducer        reducer;
    struct d_reducing_state state;
    size_t                  total;

    total   = 0;
    reducer = d_reducer_make(&d_internal_foldable_count_step, NULL);

    d_reducing_state_init(&state, &total);
    d_foldable_drive(_foldable, _carrier, &reducer, &state);

    return total;
}

/*
d_foldable_is_empty
  Reports whether a carrier yields no elements, latching on the first one so a
long or unbounded carrier is not drained to answer.

Parameter(s):
  _foldable: the carrier's dictionary.
  _carrier:  the carrier.
Return:
  A boolean value corresponding to either:
  - true, if the carrier yielded nothing, or
  - false, if it yielded at least one element.
*/
bool
d_foldable_is_empty
(
    const struct d_foldable* _foldable,
    const void*              _carrier
)
{
    struct d_reducer        reducer;
    struct d_reducing_state state;
    bool                    empty;

    empty   = true;
    reducer = d_reducer_make(&d_internal_foldable_first_step, NULL);

    d_reducing_state_init(&state, &empty);
    d_foldable_drive(_foldable, _carrier, &reducer, &state);

    return empty;
}

/*
d_foldable_fold
  Folds a carrier through a plain fold step.

Parameter(s):
  _foldable:    the carrier's dictionary.
  _carrier:     the carrier.
  _accumulator: caller-owned accumulator storage.
  _step:        the fold step.
  _context:     context forwarded to `_step`; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the fold ran, or
  - false, if the parameters were unusable.
*/
bool
d_foldable_fold
(
    const struct d_foldable* _foldable,
    const void*              _carrier,
    void*                    _accumulator,
    fn_fold                  _step,
    void*                    _context
)
{
    struct d_fold_binding   binding;
    struct d_reducer        reducer;
    struct d_reducing_state state;

    // an unusable request folds nothing
    if ( (!d_foldable_is_valid(_foldable)) ||
         (!_accumulator)                   ||
         (!_step)                          )
    {
        return false;
    }

    binding.step    = _step;
    binding.context = _context;

    reducer = d_reducer_from_fold(&binding);

    d_reducing_state_init(&state, _accumulator);
    d_foldable_drive(_foldable, _carrier, &reducer, &state);

    return true;
}

/*
d_foldable_any
  Reports whether any element of a carrier satisfies a predicate, short-circuit
included: the quantifier's latch stops the carrier's own drive.

Parameter(s):
  _foldable:  the carrier's dictionary.
  _carrier:   the carrier.
  _predicate: the test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A boolean value corresponding to whether some element satisfied the predicate.
*/
bool
d_foldable_any
(
    const struct d_foldable* _foldable,
    const void*              _carrier,
    fn_predicate             _predicate,
    void*                    _context
)
{
    struct d_quantifier     quantifier;
    struct d_reducer        reducer;
    struct d_reducing_state state;
    bool                    answer;

    answer = false;

    // an unusable request answers vacuously
    if (!_predicate)
    {
        return answer;
    }

    quantifier.predicate = _predicate;
    quantifier.context   = _context;
    quantifier.kind      = D_QUANTIFIER_ANY;

    reducer = d_reducer_from_quantifier(&quantifier);

    d_reducing_state_init(&state, &answer);
    d_foldable_drive(_foldable, _carrier, &reducer, &state);

    return answer;
}

/*
d_foldable_all
  Reports whether every element of a carrier satisfies a predicate. Vacuously
true over an empty carrier.

Parameter(s):
  _foldable:  the carrier's dictionary.
  _carrier:   the carrier.
  _predicate: the test.
  _context:   context forwarded to `_predicate`; may be NULL.
Return:
  A boolean value corresponding to whether every element satisfied the
predicate.
*/
bool
d_foldable_all
(
    const struct d_foldable* _foldable,
    const void*              _carrier,
    fn_predicate             _predicate,
    void*                    _context
)
{
    struct d_quantifier     quantifier;
    struct d_reducer        reducer;
    struct d_reducing_state state;
    bool                    answer;

    answer = true;

    // an unusable request answers vacuously
    if (!_predicate)
    {
        return answer;
    }

    quantifier.predicate = _predicate;
    quantifier.context   = _context;
    quantifier.kind      = D_QUANTIFIER_ALL;

    reducer = d_reducer_from_quantifier(&quantifier);

    d_reducing_state_init(&state, &answer);
    d_foldable_drive(_foldable, _carrier, &reducer, &state);

    return answer;
}

/*
d_foldable_mconcat
  Folds a carrier under a monoid. This is the join of all three tiers: the
carrier comes from tier 2, the monoid from tier 1, the dictionary from tier 3,
and none of them knows about the others.

Parameter(s):
  _foldable: the carrier's dictionary.
  _carrier:  the carrier.
  _monoid:   the monoid supplying the combine and the identity.
  _out:      destination of the monoid's `value_size` bytes.
Return:
  A boolean value corresponding to either:
  - true, if the combination (or the identity) was written, or
  - false, if the parameters were unusable.
*/
bool
d_foldable_mconcat
(
    const struct d_foldable* _foldable,
    const void*              _carrier,
    const struct d_monoid*   _monoid,
    void*                    _out
)
{
    struct d_reducer        reducer;
    struct d_reducing_state state;

    // an unusable request writes nothing
    if ( (!d_foldable_is_valid(_foldable)) ||
         (!d_monoid_is_valid(_monoid))     ||
         (!_out)                           )
    {
        return false;
    }

    // the carrier's elements and the monoid's values must be the same width
    if (_foldable->element_size != _monoid->semigroup.value_size)
    {
        return false;
    }

    // seeding with the identity is what makes the empty carrier answerable
    _monoid->mempty(_monoid, _out);

    reducer = d_monoid_reducer(_monoid);

    d_reducing_state_init(&state, _out);
    d_foldable_drive(_foldable, _carrier, &reducer, &state);

    return true;
}

/*
d_alternative_maybe
  The Alternative dictionary for `maybe`. Its carrier is a `struct d_maybe`.
  Note that `producer` is also an Alternative in Table 7.3 and has no instance
here: `d_producer_alt` needs caller-owned concat state, and a signature of three
`void*` has nowhere to put it. That is the same shape of problem as the
semigroup's width, and it should be solved the same way -- by giving the carrier
a struct that carries its state -- rather than by a file-static.

Parameter(s):
  none.
Return:
  A dictionary by value.
*/
struct d_alternative
d_alternative_maybe
(
    void
)
{
    struct d_alternative alternative;

    alternative.alt    = &d_internal_alternative_alt_maybe;
    alternative.aempty = &d_internal_alternative_aempty_maybe;

    return alternative;
}

/*
d_alternative_is_valid
  Reports whether a dictionary carries both operations.

Parameter(s):
  _alternative: the dictionary to inspect; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if both operations are set, or
  - false, otherwise.
*/
bool
d_alternative_is_valid
(
    const struct d_alternative* _alternative
)
{
    // a NULL dictionary is not valid
    if (!_alternative)
    {
        return false;
    }

    return ( (_alternative->alt != NULL) &&
             (_alternative->aempty != NULL) );
}

/*
d_alternative_alt
  The Alternative choice, through a dictionary.

Parameter(s):
  _alternative: the carrier's dictionary.
  _first:       the preferred carrier.
  _second:      the fallback carrier.
  _out:         the destination carrier.
Return:
  A boolean value corresponding to whether the result holds a value.
*/
bool
d_alternative_alt
(
    const struct d_alternative* _alternative,
    const void*                 _first,
    const void*                 _second,
    void*                       _out
)
{
    // an unusable dictionary chooses nothing
    if (!d_alternative_is_valid(_alternative))
    {
        return false;
    }

    return _alternative->alt(_first, _second, _out);
}

/*
d_alternative_aempty
  The Alternative identity, through a dictionary.

Parameter(s):
  _alternative: the carrier's dictionary.
  _out:         the destination carrier.
Return:
  A boolean value corresponding to whether the identity was written.
*/
bool
d_alternative_aempty
(
    const struct d_alternative* _alternative,
    void*                       _out
)
{
    // an unusable dictionary writes nothing
    if (!d_alternative_is_valid(_alternative))
    {
        return false;
    }

    return _alternative->aempty(_out);
}
