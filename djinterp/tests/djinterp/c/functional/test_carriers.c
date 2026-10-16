/*******************************************************************************
* djinterp [functional]                                          test_carriers.c
*
*
* path:      /tests/djinterp/c/functional/test_carriers.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
/*
test_carriers.c
  Conformance for the tier 2 carriers: `maybe` and `result`.

  Asserts the laws each carrier claims AND the ones it deliberately does not.
The Functor and Monad laws are checked as laws, not as spot values, because a
carrier that gets `map` right on one input and wrong on composition is exactly
the failure a spot check misses. The stated non-properties are checked too:
`result` is not Alternative, `to_maybe` is one-way, and a `nothing` never calls
the transform it was mapped with.

  The last group is the one that matters structurally: a maybe and a result are
Foldable, and Foldable's fold_left IS the tier 1 reducer. So the same
`d_monoid_sum_intmax` reducer that folds an array folds a carrier, unchanged.
If that stops holding, the two tiers have drifted apart.
*/

// std
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/functional/maybe.h"
#include "../../../../inc/djinterp/c/functional/result.h"
#include "../../../../inc/djinterp/c/functional/monoid.h"
#include "../../../../inc/djinterp/c/functional/transducer.h"


// d_carrier_tally
//   struct: the running count of conformance assertions.
struct d_carrier_tally
{
    size_t checked;     // assertions attempted
    size_t passed;      // assertions that held
};

// D_CARRIER_CHECK
//   macro: records one assertion against the running tally.
#define D_CARRIER_CHECK(tally,                                              \
                        condition,                                          \
                        label)                                              \
    d_internal_carrier_record((tally), (condition), (label))

D_MAYBE_DECLARE(d_test_maybe_int, intmax_t);
D_RESULT_DECLARE(d_test_result_int, intmax_t, intmax_t);

// d_internal_carrier_probe
//   struct: counts callable invocations, so "was the transform called at all"
// is measured rather than assumed.
struct d_internal_carrier_probe
{
    size_t calls;       // how many times the callable ran
};


/*
d_internal_carrier_record
  Records one assertion outcome, naming the failures.

Parameter(s):
  _tally:     the running tally; ignored if NULL.
  _condition: the assertion outcome.
  _label:     a short description of what was asserted.
Return:
  none.
*/
static void
d_internal_carrier_record
(
    struct d_carrier_tally* _tally,
    bool                    _condition,
    const char*             _label
)
{
    // a NULL tally is a caller error
    if (!_tally)
    {
        return;
    }

    _tally->checked = _tally->checked + 1;

    // a passing assertion is silent; a failing one names itself
    if (_condition)
    {
        _tally->passed = _tally->passed + 1;
    }
    else
    {
        printf("    FAIL  %s\n", (_label) ? _label : "(unnamed)");
    }

    return;
}

/*
d_internal_carrier_identity
  The identity transform, used to check the Functor identity law.

Parameter(s):
  _in:      the source value.
  _out:     the destination.
  _context: an optional probe counting calls.
Return:
  Always true.
*/
static bool
d_internal_carrier_identity
(
    const void* _in,
    void*       _out,
    void*       _context
)
{
    // the probe is optional
    if (_context)
    {
        ((struct d_internal_carrier_probe*)_context)->calls =
            ((struct d_internal_carrier_probe*)_context)->calls + 1;
    }

    *(intmax_t*)_out = *(const intmax_t*)_in;

    return true;
}

/*
d_internal_carrier_add_one
  Adds one. Composed with `times_two` to check the Functor composition law.

Parameter(s):
  _in:      the source value.
  _out:     the destination.
  _context: an optional probe counting calls.
Return:
  Always true.
*/
static bool
d_internal_carrier_add_one
(
    const void* _in,
    void*       _out,
    void*       _context
)
{
    // the probe is optional
    if (_context)
    {
        ((struct d_internal_carrier_probe*)_context)->calls =
            ((struct d_internal_carrier_probe*)_context)->calls + 1;
    }

    *(intmax_t*)_out = (*(const intmax_t*)_in) + 1;

    return true;
}

/*
d_internal_carrier_times_two
  Doubles its input.

Parameter(s):
  _in:      the source value.
  _out:     the destination.
  _context: unused.
Return:
  Always true.
*/
static bool
d_internal_carrier_times_two
(
    const void* _in,
    void*       _out,
    void*       _context
)
{
    (void)_context;

    *(intmax_t*)_out = (*(const intmax_t*)_in) * 2;

    return true;
}

/*
d_internal_carrier_is_even
  Admits even values.

Parameter(s):
  _element: the value under test.
  _context: unused.
Return:
  A boolean value corresponding to whether the value is even.
*/
static bool
d_internal_carrier_is_even
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return ( ((*(const intmax_t*)_element) % 2) == 0 );
}

/*
d_internal_carrier_arrow_just
  The Kleisli arrow `a -> just(a)`, which is `unit` and therefore the right
identity of bind.

Parameter(s):
  _value:   the incoming value.
  _out:     the output view to write through.
  _context: unused.
Return:
  none.
*/
static void
d_internal_carrier_arrow_just
(
    const void* _value,
    void*       _out,
    void*       _context
)
{
    (void)_context;

    d_maybe_just((struct d_maybe*)_out, _value);

    return;
}

/*
d_internal_carrier_arrow_even_only
  The Kleisli arrow admitting only even values, used to show that bind can
introduce a nothing where map cannot.

Parameter(s):
  _value:   the incoming value.
  _out:     the output view to write through.
  _context: unused.
Return:
  none.
*/
static void
d_internal_carrier_arrow_even_only
(
    const void* _value,
    void*       _out,
    void*       _context
)
{
    (void)_context;

    // an odd value is rejected into a nothing
    if (((*(const intmax_t*)_value) % 2) != 0)
    {
        d_maybe_nothing((struct d_maybe*)_out);

        return;
    }

    d_maybe_just((struct d_maybe*)_out, _value);

    return;
}

/*
d_internal_carrier_arrow_result_ok
  The Kleisli arrow `a -> ok(a * 10)` for the result carrier.

Parameter(s):
  _value:   the incoming value.
  _out:     the output view to write through.
  _context: unused.
Return:
  none.
*/
static void
d_internal_carrier_arrow_result_ok
(
    const void* _value,
    void*       _out,
    void*       _context
)
{
    intmax_t scaled;

    (void)_context;

    scaled = (*(const intmax_t*)_value) * 10;

    d_result_ok((struct d_result*)_out, &scaled);

    return;
}


/*
d_tests_maybe
  Tests the following:
  - just and nothing, and that the two readings agree
  - value_or on both arms
  - the Functor identity and composition laws
  - that mapping a nothing never calls the transform
  - the Monad left- and right-identity laws
  - that bind short-circuits on nothing, and can introduce one
  - filter never grows the carrier
  - that alt is left-biased and aempty is its two-sided identity
*/
static bool
d_tests_maybe
(
    struct d_carrier_tally* _tally
)
{
    struct d_test_maybe_int         cell;
    struct d_test_maybe_int         other;
    struct d_test_maybe_int         out;
    struct d_test_maybe_int         composed;
    struct d_maybe                  a;
    struct d_maybe                  b;
    struct d_maybe                  c;
    struct d_maybe                  d;
    struct d_internal_carrier_probe probe;
    intmax_t                        value;
    intmax_t                        fallback;
    intmax_t                        got;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  maybe\n");

    a = D_MAYBE_VIEW(cell);
    b = D_MAYBE_VIEW(other);
    c = D_MAYBE_VIEW(out);
    d = D_MAYBE_VIEW(composed);

    value    = 21;
    fallback = -1;

    d_maybe_just(&a, &value);
    D_CARRIER_CHECK(_tally, (d_maybe_is_just(&a)), "just is just");
    D_CARRIER_CHECK(_tally,
                    (!d_maybe_is_nothing(&a)),
                    "just is not nothing");

    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_value_or(&a, &got, &fallback)) &&
                      (got == 21) ),
                    "value_or returns a held value");

    d_maybe_nothing(&a);
    D_CARRIER_CHECK(_tally, (d_maybe_is_nothing(&a)), "nothing is nothing");
    D_CARRIER_CHECK(_tally,
                    ( (!d_maybe_value_or(&a, &got, &fallback)) &&
                      (got == -1) ),
                    "value_or returns the fallback when empty");

    // Functor identity: map(id) leaves the carrier unchanged
    d_maybe_just(&a, &value);
    d_maybe_map(&a, &c, &d_internal_carrier_identity, NULL);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 21) ),
                    "Functor identity law");

    // Functor composition: map(f) then map(g) equals map(g . f)
    d_maybe_map(&a, &b, &d_internal_carrier_add_one, NULL);
    d_maybe_map(&b, &c, &d_internal_carrier_times_two, NULL);
    d_maybe_just(&d, &value);
    {
        intmax_t direct;

        direct = ((*(intmax_t*)&value) + 1) * 2;
        D_CARRIER_CHECK(_tally,
                        ( (d_maybe_is_just(&c)) && (out.value == direct) ),
                        "Functor composition law");
    }

    // mapping a nothing must not call the transform at all
    probe.calls = 0;
    d_maybe_nothing(&a);
    d_maybe_map(&a, &c, &d_internal_carrier_identity, &probe);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_nothing(&c)) && (probe.calls == 0) ),
                    "map over nothing never calls the transform");

    // Monad left identity: bind(just(x), f) == f(x)
    value = 4;
    d_maybe_just(&a, &value);
    d_maybe_bind(&a, &c, &d_internal_carrier_arrow_even_only, NULL);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 4) ),
                    "Monad left identity law");

    // Monad right identity: bind(m, just) == m
    d_maybe_bind(&a, &c, &d_internal_carrier_arrow_just, NULL);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 4) ),
                    "Monad right identity law");

    // bind may introduce a nothing where map cannot
    value = 5;
    d_maybe_just(&a, &value);
    d_maybe_bind(&a, &c, &d_internal_carrier_arrow_even_only, NULL);
    D_CARRIER_CHECK(_tally,
                    (d_maybe_is_nothing(&c)),
                    "bind can introduce a nothing");

    // bind over a nothing short-circuits
    d_maybe_nothing(&a);
    d_maybe_bind(&a, &c, &d_internal_carrier_arrow_just, NULL);
    D_CARRIER_CHECK(_tally,
                    (d_maybe_is_nothing(&c)),
                    "bind over nothing short-circuits");

    // filter: 0--1 in, 0--1 out, never more
    value = 6;
    d_maybe_just(&a, &value);
    D_CARRIER_CHECK(_tally,
                    (d_maybe_filter(&a, &d_internal_carrier_is_even, NULL)),
                    "filter admits an even value");
    value = 7;
    d_maybe_just(&a, &value);
    d_maybe_filter(&a, &d_internal_carrier_is_even, NULL);
    D_CARRIER_CHECK(_tally,
                    (d_maybe_is_nothing(&a)),
                    "filter empties on rejection");

    // Alternative: left-biased choice, with aempty as two-sided identity
    value = 1;
    d_maybe_just(&a, &value);
    got = 2;
    d_maybe_just(&b, &got);
    d_maybe_alt(&a, &b, &c);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 1) ),
                    "alt is left-biased");

    d_maybe_nothing(&a);
    d_maybe_alt(&a, &b, &c);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 2) ),
                    "alt falls through an empty left");

    d_maybe_aempty(&a);
    value = 9;
    d_maybe_just(&b, &value);
    d_maybe_alt(&a, &b, &c);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 9) ),
                    "aempty is a left identity of alt");
    d_maybe_alt(&b, &a, &c);
    D_CARRIER_CHECK(_tally,
                    ( (d_maybe_is_just(&c)) && (out.value == 9) ),
                    "aempty is a right identity of alt");

    d_maybe_nothing(&a);
    d_maybe_nothing(&b);
    D_CARRIER_CHECK(_tally,
                    (!d_maybe_alt(&a, &b, &c)),
                    "alt of two empties is empty");

    return (_tally->passed == _tally->checked);
}

/*
d_tests_result
  Tests the following:
  - ok and err, and that the two readings agree
  - value_or and error on both arms
  - that map passes an error through untouched
  - that map_err passes a success through untouched
  - that bimap runs exactly one arm
  - that bind short-circuits on an error
  - that to_maybe forgets the error, and is one-way
*/
static bool
d_tests_result
(
    struct d_carrier_tally* _tally
)
{
    struct d_test_result_int        cell;
    struct d_test_result_int        out;
    struct d_test_maybe_int         forgotten;
    struct d_result                 a;
    struct d_result                 b;
    struct d_maybe                  m;
    struct d_internal_carrier_probe probe;
    intmax_t                        value;
    intmax_t                        error;
    intmax_t                        fallback;
    intmax_t                        got;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  result\n");

    a = D_RESULT_VIEW(cell);
    b = D_RESULT_VIEW(out);
    m = D_MAYBE_VIEW(forgotten);

    value    = 12;
    error    = -404;
    fallback = 0;

    d_result_ok(&a, &value);
    D_CARRIER_CHECK(_tally, (d_result_is_ok(&a)), "ok is ok");
    D_CARRIER_CHECK(_tally, (!d_result_is_err(&a)), "ok is not err");
    D_CARRIER_CHECK(_tally,
                    ( (d_result_value_or(&a, &got, &fallback)) &&
                      (got == 12) ),
                    "value_or returns a success value");
    D_CARRIER_CHECK(_tally,
                    (!d_result_error(&a, &got)),
                    "a success has no error to report");

    d_result_err(&a, &error);
    D_CARRIER_CHECK(_tally, (d_result_is_err(&a)), "err is err");
    D_CARRIER_CHECK(_tally,
                    ( (!d_result_value_or(&a, &got, &fallback)) &&
                      (got == 0) ),
                    "value_or falls back on an error");
    D_CARRIER_CHECK(_tally,
                    ( (d_result_error(&a, &got)) && (got == -404) ),
                    "error reports the error");

    // map must pass an error through without calling the transform
    probe.calls = 0;
    d_result_map(&a, &b, &d_internal_carrier_identity, &probe);
    D_CARRIER_CHECK(_tally,
                    ( (d_result_is_err(&b)) && (probe.calls == 0) ),
                    "map passes an error through untouched");

    d_result_ok(&a, &value);
    d_result_map(&a, &b, &d_internal_carrier_times_two, NULL);
    D_CARRIER_CHECK(_tally,
                    ( (d_result_is_ok(&b)) && (out.payload.value == 24) ),
                    "map applies to a success value");

    // map_err is the mirror image
    probe.calls = 0;
    d_result_map_err(&a, &b, &d_internal_carrier_identity, &probe);
    D_CARRIER_CHECK(_tally,
                    ( (d_result_is_ok(&b)) && (probe.calls == 0) ),
                    "map_err passes a success through untouched");

    d_result_err(&a, &error);
    d_result_map_err(&a, &b, &d_internal_carrier_times_two, NULL);
    D_CARRIER_CHECK(_tally,
                    ( (d_result_is_err(&b)) && (out.payload.error == -808) ),
                    "map_err applies to an error");

    // bimap runs exactly one arm, which is what distinguishes it from
    // applying map and map_err in sequence
    probe.calls = 0;
    d_result_ok(&a, &value);
    d_result_bimap(&a, &b, &d_internal_carrier_identity,
                   &d_internal_carrier_identity, &probe);
    D_CARRIER_CHECK(_tally,
                    ( (d_result_is_ok(&b)) && (probe.calls == 1) ),
                    "bimap runs exactly one arm");

    // bind short-circuits on an error
    d_result_err(&a, &error);
    d_result_bind(&a, &b, &d_internal_carrier_arrow_result_ok, NULL);
    D_CARRIER_CHECK(_tally,
                    (d_result_is_err(&b)),
                    "bind short-circuits on an error");

    d_result_ok(&a, &value);
    d_result_bind(&a, &b, &d_internal_carrier_arrow_result_ok, NULL);
    D_CARRIER_CHECK(_tally,
                    ( (d_result_is_ok(&b)) && (out.payload.value == 120) ),
                    "bind applies to a success value");

    // to_maybe forgets the error, one way only
    D_CARRIER_CHECK(_tally,
                    ( (d_result_to_maybe(&a, &m)) &&
                      (d_maybe_is_just(&m))       &&
                      (forgotten.value == 12) ),
                    "to_maybe carries a success across");

    d_result_err(&a, &error);
    D_CARRIER_CHECK(_tally,
                    ( (!d_result_to_maybe(&a, &m)) &&
                      (d_maybe_is_nothing(&m)) ),
                    "to_maybe forgets an error into nothing");

    return (_tally->passed == _tally->checked);
}

/*
d_tests_carriers_are_foldable
  Tests the following:
  - that a maybe and a result deliver 1 or 0 elements to a tier 1 reducer
  - that the SAME monoid reducer folds an array and a carrier
  - that a carrier presents as a producer and drives a transducer chain
  This group is the join between tier 1 and tier 2: if it fails, the carriers
have grown a second, parallel fold rather than reusing the spine.
*/
static bool
d_tests_carriers_are_foldable
(
    struct d_carrier_tally* _tally
)
{
    struct d_test_maybe_int       mcell;
    struct d_test_result_int      rcell;
    struct d_maybe                m;
    struct d_result               r;
    struct d_monoid               sum;
    struct d_reducer              reducer;
    struct d_reducing_state       state;
    struct d_producer_array_state pstate;
    struct d_producer             source;
    intmax_t                      accumulator;
    intmax_t                      value;
    intmax_t                      error;
    intmax_t                      collected[4];
    size_t                        delivered;
    size_t                        written;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  carriers are foldable over the tier 1 spine\n");

    m     = D_MAYBE_VIEW(mcell);
    r     = D_RESULT_VIEW(rcell);
    sum   = d_monoid_sum_intmax();
    value = 7;
    error = -1;

    // the same reducer that folds an array folds a maybe
    reducer = d_monoid_reducer(&sum);

    d_maybe_just(&m, &value);
    accumulator = 0;
    d_reducing_state_init(&state, &accumulator);
    delivered = d_maybe_drive(&m, &reducer, &state);
    D_CARRIER_CHECK(_tally,
                    ( (delivered == 1) && (accumulator == 7) ),
                    "a just delivers one element to a monoid reducer");

    d_maybe_nothing(&m);
    accumulator = 0;
    d_reducing_state_init(&state, &accumulator);
    delivered = d_maybe_drive(&m, &reducer, &state);
    D_CARRIER_CHECK(_tally,
                    ( (delivered == 0) && (accumulator == 0) ),
                    "a nothing delivers no elements");

    d_result_ok(&r, &value);
    accumulator = 0;
    d_reducing_state_init(&state, &accumulator);
    delivered = d_result_drive(&r, &reducer, &state);
    D_CARRIER_CHECK(_tally,
                    ( (delivered == 1) && (accumulator == 7) ),
                    "an ok delivers one element");

    d_result_err(&r, &error);
    accumulator = 0;
    d_reducing_state_init(&state, &accumulator);
    delivered = d_result_drive(&r, &reducer, &state);
    D_CARRIER_CHECK(_tally,
                    ( (delivered == 0) && (accumulator == 0) ),
                    "an err delivers no elements");

    // and a carrier presents as an ordinary producer
    d_maybe_just(&m, &value);
    source  = d_maybe_producer(&pstate, &m);
    written = d_producer_collect(&source, collected, 4);
    D_CARRIER_CHECK(_tally,
                    ( (written == 1) && (collected[0] == 7) ),
                    "a just presents as a one-element producer");

    d_maybe_nothing(&m);
    source  = d_maybe_producer(&pstate, &m);
    written = d_producer_collect(&source, collected, 4);
    D_CARRIER_CHECK(_tally,
                    (written == 0),
                    "a nothing presents as the empty producer");

    // a transducer chain runs over a carrier with no modification
    {
        struct d_transducer_stage stages[1];
        struct d_transducer       transducer;
        struct d_array_sink       sink;
        struct d_reducing_state   tstate;
        intmax_t                  scratch;

        value = 8;
        d_maybe_just(&m, &value);
        source    = d_maybe_producer(&pstate, &m);
        stages[0] = d_transducer_filter(&d_internal_carrier_is_even, NULL);
        d_array_sink_init(&sink, collected, 4, sizeof(intmax_t));
        d_transducer_init(&transducer, stages, 1,
                          d_reducer_from_array_sink(&sink));
        d_reducing_state_init(&tstate, NULL);
        d_transduce_producer(&transducer, &source, &tstate, &scratch);

        D_CARRIER_CHECK(_tally,
                        ( (sink.written == 1) && (collected[0] == 8) ),
                        "a transducer chain runs over a carrier unchanged");
    }

    // the union is the point: a result cell costs the larger arm, not both
    D_CARRIER_CHECK(_tally,
                    (sizeof(rcell.payload) == sizeof(intmax_t)),
                    "a result payload costs one arm, not two");

    return (_tally->passed == _tally->checked);
}


/*
main
  Runs every carrier conformance group and reports the tally.

Parameter(s):
  none.
Return:
  0 if every assertion passed, or 1 otherwise.
*/
int
main
(
    void
)
{
    struct d_carrier_tally tally;

    tally.checked = 0;
    tally.passed  = 0;

    printf("djinterp [functional] C carrier conformance (tier 2)\n");

    d_tests_maybe(&tally);
    d_tests_result(&tally);
    d_tests_carriers_are_foldable(&tally);

    printf("\n  %lu / %lu assertions passed\n",
           (unsigned long)tally.passed,
           (unsigned long)tally.checked);

    // a single failure fails the suite
    if (tally.passed != tally.checked)
    {
        return 1;
    }

    return 0;
}
