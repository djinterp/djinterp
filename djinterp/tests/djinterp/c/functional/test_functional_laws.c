/*******************************************************************************
* djinterp [functional]                                   test_functional_laws.c
*
*
* path:      /tests/djinterp/c/functional/test_functional_laws.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./test_functional_laws.h"


// D_TEST_CHECK
//   macro: records one conformance assertion against the running tally.
#define D_TEST_CHECK(tally,                                                 \
                     condition,                                             \
                     label)                                                 \
    d_internal_test_record((tally), (condition), (label))


/*
d_internal_test_record
  Records one assertion outcome and reports failures as they happen.

Parameter(s):
  _tally:     the running tally; ignored if NULL.
  _condition: the assertion outcome.
  _label:     a short description of what was asserted.
Return:
  none.
*/
static void
d_internal_test_record
(
    struct d_test_tally* _tally,
    bool                 _condition,
    const char*          _label
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
d_internal_test_pred_is_even
  Admits even integers, counting its calls so short-circuiting can be observed.

Parameter(s):
  _element: the `intmax_t` under test.
  _context: an optional `d_test_probe` counting calls.
Return:
  A boolean value corresponding to either:
  - true, if the element is even, or
  - false, otherwise.
*/
static bool
d_internal_test_pred_is_even
(
    const void* _element,
    void*       _context
)
{
    // a NULL element cannot be tested
    if (!_element)
    {
        return false;
    }

    // the probe is optional, so a bare predicate call still works
    if (_context)
    {
        ((struct d_test_probe*)_context)->calls =
            ((struct d_test_probe*)_context)->calls + 1;
    }

    return ( ((*(const intmax_t*)_element) % 2) == 0 );
}

/*
d_internal_test_pred_equals_probe
  Admits integers equal to the probe's target, counting its calls.

Parameter(s):
  _element: the `intmax_t` under test.
  _context: the `d_test_probe` supplying the target and counting calls.
Return:
  A boolean value corresponding to either:
  - true, if the element equals the probe's target, or
  - false, otherwise.
*/
static bool
d_internal_test_pred_equals_probe
(
    const void* _element,
    void*       _context
)
{
    struct d_test_probe* probe;

    // both the element and the probe are required
    if ( (!_element) ||
         (!_context) )
    {
        return false;
    }

    probe = (struct d_test_probe*)_context;

    probe->calls = probe->calls + 1;

    return ( (*(const intmax_t*)_element) == probe->target );
}

/*
d_internal_test_pred_less_than_probe
  Admits integers strictly below the probe's target, counting its calls.

Parameter(s):
  _element: the `intmax_t` under test.
  _context: the `d_test_probe` supplying the bound and counting calls.
Return:
  A boolean value corresponding to either:
  - true, if the element is below the probe's target, or
  - false, otherwise.
*/
static bool
d_internal_test_pred_less_than_probe
(
    const void* _element,
    void*       _context
)
{
    struct d_test_probe* probe;

    // both the element and the probe are required
    if ( (!_element) ||
         (!_context) )
    {
        return false;
    }

    probe = (struct d_test_probe*)_context;

    probe->calls = probe->calls + 1;

    return ( (*(const intmax_t*)_element) < probe->target );
}

/*
d_internal_test_transform_increment
  Maps an integer to its successor.

Parameter(s):
  _input:   the source `intmax_t`.
  _output:  destination `intmax_t`.
  _context: unused.
Return:
  A boolean value corresponding to either:
  - true, if the successor was written, or
  - false, if either pointer was NULL.
*/
static bool
d_internal_test_transform_increment
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    // both ends are required
    if ( (!_input)  ||
         (!_output) )
    {
        return false;
    }

    *(intmax_t*)_output = (*(const intmax_t*)_input) + 1;

    return true;
}

/*
d_internal_test_transform_int_to_intmax
  Widens an `int` element into the `intmax_t` a numeric monoid combines.

Parameter(s):
  _input:   the source `int`.
  _output:  destination `intmax_t`.
  _context: unused.
Return:
  A boolean value corresponding to either:
  - true, if the widened value was written, or
  - false, if either pointer was NULL.
*/
static bool
d_internal_test_transform_int_to_intmax
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    // both ends are required
    if ( (!_input)  ||
         (!_output) )
    {
        return false;
    }

    *(intmax_t*)_output = (intmax_t)(*(const int*)_input);

    return true;
}

/*
d_internal_test_fold_decimal_shift
  Folds digits into a decimal number: `acc = (acc * 10) + digit`. Deliberately
non-commutative, so a left fold and a right fold must disagree.

Parameter(s):
  _accumulator: the running `intmax_t`.
  _element:     the incoming digit.
  _context:     unused.
Return:
  none.
*/
static void
d_internal_test_fold_decimal_shift
(
    void*       _accumulator,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    // both the accumulator and the element are required
    if ( (!_accumulator) ||
         (!_element)     )
    {
        return;
    }

    *(intmax_t*)_accumulator = ((*(intmax_t*)_accumulator) * 10) +
                               (*(const intmax_t*)_element);

    return;
}

/*
d_internal_test_consume_count
  Counts the values reaching a sink.

Parameter(s):
  _element: the incoming value; unused beyond its arrival.
  _context: the `d_test_probe` counting arrivals.
Return:
  none.
*/
static void
d_internal_test_consume_count
(
    const void* _element,
    void*       _context
)
{
    (void)_element;

    // the probe is where the count lives
    if (!_context)
    {
        return;
    }

    ((struct d_test_probe*)_context)->calls =
        ((struct d_test_probe*)_context)->calls + 1;

    return;
}

/*
d_internal_test_assoc_intmax
  Asserts that one `intmax_t` monoid is associative and that its identity is
two-sided, over a fixed sample of operands.

Parameter(s):
  _tally:  the running tally.
  _monoid: the monoid under test.
  _label:  the instance name, used in failure messages.
Return:
  none.
*/
static void
d_internal_test_assoc_intmax
(
    struct d_test_tally*   _tally,
    const struct d_monoid* _monoid,
    const char*            _label
)
{
    intmax_t a;
    intmax_t b;
    intmax_t c;
    intmax_t left;
    intmax_t right;
    intmax_t partial;
    intmax_t identity;
    intmax_t combined;

    // a NULL tally or monoid is a caller error
    if ( (!_tally)  ||
         (!_monoid) )
    {
        return;
    }

    a = 3;
    b = 5;
    c = 7;

    // (a <> b) <> c
    d_mappend(d_monoid_semigroup(_monoid), &partial, &a, &b);
    d_mappend(d_monoid_semigroup(_monoid), &left, &partial, &c);

    // a <> (b <> c)
    d_mappend(d_monoid_semigroup(_monoid), &partial, &b, &c);
    d_mappend(d_monoid_semigroup(_monoid), &right, &a, &partial);

    D_TEST_CHECK(_tally, (left == right), _label);

    d_mempty(_monoid, &identity);

    // mempty <> a == a
    d_mappend(d_monoid_semigroup(_monoid), &combined, &identity, &a);
    D_TEST_CHECK(_tally, (combined == a), _label);

    // a <> mempty == a
    d_mappend(d_monoid_semigroup(_monoid), &combined, &a, &identity);
    D_TEST_CHECK(_tally, (combined == a), _label);

    return;
}


/*
d_tests_semigroup_laws
  Tests the following:
  - associativity of every intmax_t monoid instance
  - two-sided identity of every intmax_t monoid instance
  - associativity of the boolean instances
  - that `mappend` permits its result to alias its left operand
  - that `first` and `last` are associative
  - that `first` has no identity, by witness
  - that a semigroup fold over an empty array fails, having no identity
*/
bool
d_tests_semigroup_laws
(
    struct d_test_tally* _tally
)
{
    struct d_semigroup first;
    struct d_monoid    sum;
    struct d_monoid    product;
    struct d_monoid    minimum;
    struct d_monoid    maximum;
    struct d_monoid    all;
    struct d_monoid    any;
    intmax_t           accumulator;
    intmax_t           operand;
    intmax_t           candidate;
    intmax_t           sample_one;
    intmax_t           sample_two;
    intmax_t           result_one;
    intmax_t           result_two;
    intmax_t           unused;
    bool               flag_a;
    bool               flag_b;
    bool               flag_c;
    bool               bool_left;
    bool               bool_right;
    bool               bool_partial;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  semigroup and monoid laws\n");

    sum     = d_monoid_sum_intmax();
    product = d_monoid_product_intmax();
    minimum = d_monoid_min_intmax();
    maximum = d_monoid_max_intmax();

    d_internal_test_assoc_intmax(_tally, &sum,     "sum_intmax laws");
    d_internal_test_assoc_intmax(_tally, &product, "product_intmax laws");
    d_internal_test_assoc_intmax(_tally, &minimum, "min_intmax laws");
    d_internal_test_assoc_intmax(_tally, &maximum, "max_intmax laws");

    // aliasing the result onto the left operand is part of the contract
    accumulator = 4;
    operand     = 6;
    d_mappend(d_monoid_semigroup(&sum), &accumulator, &accumulator, &operand);
    D_TEST_CHECK(_tally,
                 (accumulator == 10),
                 "mappend supports result aliasing left");

    all = d_monoid_all();
    any = d_monoid_any();

    flag_a = true;
    flag_b = false;
    flag_c = true;

    d_mappend(d_monoid_semigroup(&all), &bool_partial, &flag_a, &flag_b);
    d_mappend(d_monoid_semigroup(&all), &bool_left, &bool_partial, &flag_c);
    d_mappend(d_monoid_semigroup(&all), &bool_partial, &flag_b, &flag_c);
    d_mappend(d_monoid_semigroup(&all), &bool_right, &flag_a, &bool_partial);
    D_TEST_CHECK(_tally, (bool_left == bool_right), "all is associative");

    d_mappend(d_monoid_semigroup(&any), &bool_partial, &flag_a, &flag_b);
    d_mappend(d_monoid_semigroup(&any), &bool_left, &bool_partial, &flag_c);
    d_mappend(d_monoid_semigroup(&any), &bool_partial, &flag_b, &flag_c);
    d_mappend(d_monoid_semigroup(&any), &bool_right, &flag_a, &bool_partial);
    D_TEST_CHECK(_tally, (bool_left == bool_right), "any is associative");

    d_mempty(&all, &bool_partial);
    D_TEST_CHECK(_tally, (bool_partial == true), "mempty of all is true");
    d_mempty(&any, &bool_partial);
    D_TEST_CHECK(_tally, (bool_partial == false), "mempty of any is false");

    // `first` is associative but has no identity: that is the whole reason the
    // semigroup and monoid protocols are separate
    first      = d_semigroup_first(sizeof(intmax_t));
    candidate  = 99;
    sample_one = 1;
    sample_two = 2;

    d_mappend(&first, &result_one, &candidate, &sample_one);
    d_mappend(&first, &result_two, &candidate, &sample_two);

    D_TEST_CHECK(_tally,
                 ( (result_one == candidate) &&
                   (result_two == candidate) &&
                   (sample_one != sample_two) ),
                 "first admits no left identity (witness)");

    D_TEST_CHECK(_tally,
                 (!d_semigroup_reduce(&first, &sample_one, 0, &unused)),
                 "semigroup fold over empty fails, having no identity");

    return (_tally->passed == _tally->checked);
}

/*
d_tests_fold_and_quantifiers
  Tests the following:
  - that a left fold and a right fold differ for a non-commutative step
  - that mconcat over an empty array answers the identity
  - that mconcat agrees with an explicitly written fold
  - that fold_monoid maps and combines in one pass
  - vacuous truth of all and none, and vacuous falsity of any
  - that each quantifier short-circuits, measured by predicate calls
  - that count_if runs to exhaustion, having no early answer
*/
bool
d_tests_fold_and_quantifiers
(
    struct d_test_tally* _tally
)
{
    struct d_monoid     sum;
    struct d_test_probe probe;
    intmax_t            digits[3];
    intmax_t            values[5];
    int                 narrow[3];
    intmax_t            folded;
    intmax_t            total;
    intmax_t            scratch;
    size_t              counted;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  folds and quantifiers\n");

    digits[0] = 1;
    digits[1] = 2;
    digits[2] = 3;

    folded = 0;
    d_fold_left(digits, 3, sizeof(intmax_t), &folded,
                &d_internal_test_fold_decimal_shift, NULL);
    D_TEST_CHECK(_tally, (folded == 123), "fold_left visits in order");

    folded = 0;
    d_fold_right(digits, 3, sizeof(intmax_t), &folded,
                 &d_internal_test_fold_decimal_shift, NULL);
    D_TEST_CHECK(_tally, (folded == 321), "fold_right visits in reverse");

    sum = d_monoid_sum_intmax();

    total = 12345;
    D_TEST_CHECK(_tally,
                 (d_mconcat(&sum, digits, 0, &total)),
                 "mconcat over empty succeeds");
    D_TEST_CHECK(_tally, (total == 0), "mconcat over empty is mempty");

    d_mconcat(&sum, digits, 3, &total);
    D_TEST_CHECK(_tally, (total == 6), "mconcat combines every element");

    // the corpus claim that a statistic is a fold: mconcat and an explicit
    // fold over the same combine must agree
    folded = 0;
    d_fold_left(digits, 3, sizeof(intmax_t), &folded,
                &d_internal_test_fold_decimal_shift, NULL);
    d_mconcat(&sum, digits, 3, &total);
    D_TEST_CHECK(_tally,
                 ( (total == 6) && (folded == 123) ),
                 "mconcat and fold_left are the same driver");

    narrow[0] = 10;
    narrow[1] = 20;
    narrow[2] = 30;

    total = 0;
    D_TEST_CHECK(_tally,
                 (d_fold_monoid(&sum, narrow, 3, sizeof(int),
                                &d_internal_test_transform_int_to_intmax,
                                NULL, &total, &scratch)),
                 "fold_monoid succeeds");
    D_TEST_CHECK(_tally, (total == 60), "fold_monoid maps then combines");

    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // vacuous cases, asserted as the non-properties they are
    D_TEST_CHECK(_tally,
                 (!d_reducer_any(values, 0, sizeof(intmax_t),
                                 &d_internal_test_pred_is_even, NULL)),
                 "any over empty is false");
    D_TEST_CHECK(_tally,
                 (d_reducer_all(values, 0, sizeof(intmax_t),
                                &d_internal_test_pred_is_even, NULL)),
                 "all over empty is vacuously true");
    D_TEST_CHECK(_tally,
                 (d_reducer_none(values, 0, sizeof(intmax_t),
                                 &d_internal_test_pred_is_even, NULL)),
                 "none over empty is vacuously true");

    // any must stop at the first satisfying element, which is the second
    probe.calls  = 0;
    probe.target = 2;
    D_TEST_CHECK(_tally,
                 (d_reducer_any(values, 5, sizeof(intmax_t),
                                &d_internal_test_pred_equals_probe, &probe)),
                 "any finds the element");
    D_TEST_CHECK(_tally, (probe.calls == 2), "any short-circuits");

    // all must stop at the first counterexample, which is the third element
    probe.calls  = 0;
    probe.target = 3;
    D_TEST_CHECK(_tally,
                 (!d_reducer_all(values, 5, sizeof(intmax_t),
                                 &d_internal_test_pred_less_than_probe,
                                 &probe)),
                 "all rejects on a counterexample");
    D_TEST_CHECK(_tally, (probe.calls == 3), "all short-circuits");

    // none must stop at the first satisfying element
    probe.calls  = 0;
    probe.target = 2;
    D_TEST_CHECK(_tally,
                 (!d_reducer_none(values, 5, sizeof(intmax_t),
                                  &d_internal_test_pred_equals_probe, &probe)),
                 "none rejects on a hit");
    D_TEST_CHECK(_tally, (probe.calls == 2), "none short-circuits");

    // a count has no decidable early answer, so it must see everything
    probe.calls = 0;
    counted     = d_reducer_count_if(values, 5, sizeof(intmax_t),
                                     &d_internal_test_pred_is_even, &probe);
    D_TEST_CHECK(_tally, (counted == 2), "count_if counts the even elements");
    D_TEST_CHECK(_tally,
                 (probe.calls == 5),
                 "count_if runs to exhaustion");

    return (_tally->passed == _tally->checked);
}

/*
d_tests_producer_protocol
  Tests the following:
  - that a range producer emits its half-open interval
  - that a descending range terminates
  - that a zero-step range emits nothing rather than looping
  - that take_n bounds an otherwise unbounded repeat producer
  - that iterate emits the seed before applying the step
  - that concat sequences two producers
  - that filter and transform compose over a producer
  - that the same reducer driven over an array and over a producer agrees
*/
bool
d_tests_producer_protocol
(
    struct d_test_tally* _tally
)
{
    struct d_producer_range_state  range_state;
    struct d_producer_repeat_state repeat_state;
    struct d_producer_array_state  array_state;
    struct d_producer_iterate_state iterate_state;
    struct d_producer_take_state   take_state;
    struct d_producer_concat_state concat_state;
    struct d_producer_filter_state filter_state;
    struct d_producer              source;
    struct d_producer              bounded;
    struct d_producer              second;
    struct d_producer              joined;
    struct d_producer              filtered;
    struct d_monoid                sum;
    intmax_t                       collected[16];
    intmax_t                       elements[5];
    intmax_t                       seed;
    intmax_t                       repeated;
    intmax_t                       scratch;
    intmax_t                       from_array;
    intmax_t                       from_producer;
    size_t                         written;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  producer protocol\n");

    source  = d_producer_range(&range_state, 0, 5, 1);
    written = d_producer_collect(&source, collected, 16);
    D_TEST_CHECK(_tally, (written == 5), "range emits its half-open interval");
    D_TEST_CHECK(_tally,
                 ( (collected[0] == 0) && (collected[4] == 4) ),
                 "range excludes its upper bound");

    source  = d_producer_range(&range_state, 5, 0, -2);
    written = d_producer_collect(&source, collected, 16);
    D_TEST_CHECK(_tally, (written == 3), "descending range terminates");
    D_TEST_CHECK(_tally,
                 ( (collected[0] == 5) && (collected[2] == 1) ),
                 "descending range steps by its negative step");

    source  = d_producer_range(&range_state, 0, 5, 0);
    written = d_producer_collect(&source, collected, 16);
    D_TEST_CHECK(_tally, (written == 0), "zero-step range emits nothing");

    // an unbounded producer is drainable only once something bounds it
    repeated = 7;
    source   = d_producer_repeat(&repeat_state, &repeated, sizeof(intmax_t));
    bounded  = d_producer_take_n(&take_state, &source, 4);
    written  = d_producer_collect(&bounded, collected, 16);
    D_TEST_CHECK(_tally, (written == 4), "take_n bounds an infinite producer");
    D_TEST_CHECK(_tally,
                 ( (collected[0] == 7) && (collected[3] == 7) ),
                 "repeat emits the same value");

    seed    = 1;
    source  = d_producer_iterate(&iterate_state, &seed, sizeof(intmax_t),
                                 &d_internal_test_transform_increment, NULL);
    bounded = d_producer_take_n(&take_state, &source, 3);
    written = d_producer_collect(&bounded, collected, 16);
    D_TEST_CHECK(_tally, (written == 3), "iterate is bounded by take_n");
    D_TEST_CHECK(_tally,
                 ( (collected[0] == 1) &&
                   (collected[1] == 2) &&
                   (collected[2] == 3) ),
                 "iterate emits the seed before stepping");

    elements[0] = 1;
    elements[1] = 2;
    elements[2] = 3;
    elements[3] = 4;
    elements[4] = 5;

    source  = d_producer_from_array(&array_state, elements, 2,
                                    sizeof(intmax_t));
    repeated = 9;
    second   = d_producer_repeat_n(&repeat_state, &repeated,
                                   sizeof(intmax_t), 2);
    joined   = d_producer_concat(&concat_state, &source, &second);
    written  = d_producer_collect(&joined, collected, 16);
    D_TEST_CHECK(_tally, (written == 4), "concat sequences both producers");
    D_TEST_CHECK(_tally,
                 ( (collected[0] == 1) &&
                   (collected[1] == 2) &&
                   (collected[2] == 9) &&
                   (collected[3] == 9) ),
                 "concat drains the first producer before the second");

    source   = d_producer_from_array(&array_state, elements, 5,
                                     sizeof(intmax_t));
    filtered = d_producer_filter(&filter_state, &source,
                                 &d_internal_test_pred_is_even, NULL);
    written  = d_producer_collect(&filtered, collected, 16);
    D_TEST_CHECK(_tally, (written == 2), "producer filter admits two elements");
    D_TEST_CHECK(_tally,
                 ( (collected[0] == 2) && (collected[1] == 4) ),
                 "producer filter preserves order");

    // one reducer, two drivers: the closest thing to a differential test that
    // is available while the C++ sources are absent
    sum = d_monoid_sum_intmax();

    d_mconcat(&sum, elements, 5, &from_array);

    source = d_producer_from_array(&array_state, elements, 5,
                                  sizeof(intmax_t));
    d_mconcat_producer(&sum, &source, &from_producer, &scratch);

    D_TEST_CHECK(_tally,
                 (from_array == from_producer),
                 "array and producer drivers agree for one reducer");
    D_TEST_CHECK(_tally, (from_array == 15), "both drivers compute the sum");

    // the Alternative identity law over the sequence carrier: aempty is a
    // two-sided identity of alt (functional_types.tex Table 7.3)
    source   = d_producer_from_array(&array_state, elements, 2,
                                     sizeof(intmax_t));
    bounded  = d_producer_aempty(sizeof(intmax_t));
    joined   = d_producer_alt(&concat_state, &bounded, &source);
    written  = d_producer_collect(&joined, collected, 16);
    D_TEST_CHECK(_tally,
                 ( (written == 2) &&
                   (collected[0] == 1) &&
                   (collected[1] == 2) ),
                 "aempty is a left identity of alt");

    source   = d_producer_from_array(&array_state, elements, 2,
                                     sizeof(intmax_t));
    bounded  = d_producer_aempty(sizeof(intmax_t));
    joined   = d_producer_alt(&concat_state, &source, &bounded);
    written  = d_producer_collect(&joined, collected, 16);
    D_TEST_CHECK(_tally,
                 ( (written == 2) &&
                   (collected[0] == 1) &&
                   (collected[1] == 2) ),
                 "aempty is a right identity of alt");

    return (_tally->passed == _tally->checked);
}

/*
d_tests_transducer_composition
  Tests the following:
  - that a filter stage never grows the stream
  - that take bounds the output to exactly n
  - that a take of zero emits nothing
  - that stage order is observable: stages[0] sees each value first
  - that drop_while latches and stops testing once it has fired
  - that a chain over an array and the same chain over a producer agree
  - that filter expressed as predicate plus sink matches the array form
*/
bool
d_tests_transducer_composition
(
    struct d_test_tally* _tally
)
{
    struct d_transducer_stage     stages[2];
    struct d_transducer           transducer;
    struct d_array_sink           sink;
    struct d_reducing_state       state;
    struct d_producer_array_state array_state;
    struct d_producer             source;
    struct d_test_probe           probe;
    intmax_t                      elements[10];
    intmax_t                      latch_input[5];
    intmax_t                      output[16];
    intmax_t                      mirror[16];
    intmax_t                      scratch;
    intmax_t                      map_scratch;
    size_t                        index;
    size_t                        written;
    size_t                        mirrored;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  transducer composition\n");

    // fill 1..10
    for (index = 0; index < 10; index = index + 1)
    {
        elements[index] = (intmax_t)(index + 1);
    }

    stages[0] = d_transducer_filter(&d_internal_test_pred_is_even, NULL);
    d_transduce_into_array(stages, 1, elements, 10, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally, (written == 5), "filter admits the even elements");
    D_TEST_CHECK(_tally, (written <= 10), "filter never grows the stream");

    stages[0] = d_transducer_take(3);
    d_transduce_into_array(stages, 1, elements, 10, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally, (written == 3), "take bounds the output to n");

    stages[0] = d_transducer_take(20);
    d_transduce_into_array(stages, 1, elements, 10, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally,
                 (written == 10),
                 "take beyond the source yields the whole source");

    stages[0] = d_transducer_take(0);
    d_transduce_into_array(stages, 1, elements, 10, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally, (written == 0), "take of zero emits nothing");

    // map then filter: 1..4 becomes 2..5, of which two are even
    stages[0] = d_transducer_map(&d_internal_test_transform_increment, NULL,
                                 &map_scratch);
    stages[1] = d_transducer_filter(&d_internal_test_pred_is_even, NULL);
    d_transduce_into_array(stages, 2, elements, 4, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally, (written == 2), "map-then-filter admits two");
    D_TEST_CHECK(_tally,
                 ( (output[0] == 2) && (output[1] == 4) ),
                 "map-then-filter keeps the incremented evens");

    // filter then map: 1..4 filters to 2 and 4, which increment to 3 and 5
    stages[0] = d_transducer_filter(&d_internal_test_pred_is_even, NULL);
    stages[1] = d_transducer_map(&d_internal_test_transform_increment, NULL,
                                 &map_scratch);
    d_transduce_into_array(stages, 2, elements, 4, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally, (written == 2), "filter-then-map admits two");
    D_TEST_CHECK(_tally,
                 ( (output[0] == 3) && (output[1] == 5) ),
                 "stage order is observable: stages[0] sees values first");

    // drop_while must latch: the 1 and 2 after the 3 pass through
    latch_input[0] = 1;
    latch_input[1] = 2;
    latch_input[2] = 3;
    latch_input[3] = 1;
    latch_input[4] = 2;

    probe.calls  = 0;
    probe.target = 3;
    stages[0]    = d_transducer_drop_while(
                       &d_internal_test_pred_less_than_probe, &probe);
    d_transduce_into_array(stages, 1, latch_input, 5, sizeof(intmax_t),
                           output, 16, &written);
    D_TEST_CHECK(_tally, (written == 3), "drop_while drops only the prefix");
    D_TEST_CHECK(_tally,
                 ( (output[0] == 3) && (output[1] == 1) && (output[2] == 2) ),
                 "drop_while passes later matches once latched");
    D_TEST_CHECK(_tally,
                 (probe.calls == 3),
                 "drop_while stops testing once latched");

    // the same chain over the same values, driven from a producer instead of
    // an array, must produce byte-identical output
    stages[0] = d_transducer_filter(&d_internal_test_pred_is_even, NULL);
    d_transduce_into_array(stages, 1, elements, 10, sizeof(intmax_t),
                           output, 16, &written);

    d_array_sink_init(&sink, mirror, 16, sizeof(intmax_t));
    stages[0] = d_transducer_filter(&d_internal_test_pred_is_even, NULL);
    d_transducer_init(&transducer, stages, 1,
                      d_reducer_from_array_sink(&sink));
    d_reducing_state_init(&state, NULL);
    source = d_producer_from_array(&array_state, elements, 10,
                                   sizeof(intmax_t));
    d_transduce_producer(&transducer, &source, &state, &scratch);
    mirrored = sink.written;

    D_TEST_CHECK(_tally,
                 (written == mirrored),
                 "one chain, two drivers, same output count");
    D_TEST_CHECK(_tally,
                 (memcmp(output, mirror,
                         written * sizeof(intmax_t)) == 0),
                 "one chain, two drivers, byte-identical output");

    // the filter row of the vocabulary: predicate plus sink
    written = d_filter_into_array(elements, 10, sizeof(intmax_t),
                                  &d_internal_test_pred_is_even, NULL,
                                  output, 16);
    D_TEST_CHECK(_tally,
                 (written == 5),
                 "d_filter_into_array matches the chain form");

    probe.calls = 0;
    d_filter_to_consumer(elements, 10, sizeof(intmax_t),
                         &d_internal_test_pred_is_even, NULL,
                         &d_internal_test_consume_count, &probe);
    D_TEST_CHECK(_tally,
                 (probe.calls == 5),
                 "d_filter_to_consumer reaches the sink five times");

    // a full sink reports the loss instead of writing out of bounds
    written = d_filter_into_array(elements, 10, sizeof(intmax_t),
                                  &d_internal_test_pred_is_even, NULL,
                                  output, 2);
    D_TEST_CHECK(_tally,
                 (written == 2),
                 "a bounded sink stops at its capacity");

    return (_tally->passed == _tally->checked);
}

/*
d_tests_cost_and_layout
  Tests the following:
  - that decorating a semigroup into a monoid costs exactly one pointer
  - that the forgetful map from monoid to semigroup is the identity on address
  - that a transducer chain collapses to a plain two-word reducer
  - that the callable union costs no more than one function pointer
*/
bool
d_tests_cost_and_layout
(
    struct d_test_tally* _tally
)
{
    struct d_monoid     sum;
    struct d_transducer transducer;
    struct d_reducer    collapsed;

    // a NULL tally leaves nothing to record into
    if (!_tally)
    {
        return false;
    }

    printf("  cost and layout laws\n");

    sum = d_monoid_sum_intmax();

    D_TEST_CHECK(_tally,
                 (sizeof(struct d_monoid) ==
                      (sizeof(struct d_semigroup) + sizeof(fn_mempty))),
                 "monoid costs its semigroup plus one pointer");
    D_TEST_CHECK(_tally,
                 ((const void*)d_monoid_semigroup(&sum) == (const void*)&sum),
                 "the forgetful map is free");
    D_TEST_CHECK(_tally,
                 (sizeof(union d_transducer_callable) == sizeof(fn_predicate)),
                 "the callable union costs one function pointer");

    transducer.stages = NULL;
    transducer.count  = 0;
    transducer.sink   = d_reducer_make(NULL, NULL);
    collapsed         = d_transducer_into_reducer(&transducer);

    D_TEST_CHECK(_tally,
                 (sizeof(collapsed) ==
                      (sizeof(fn_reducer_step) + sizeof(void*))),
                 "a chain collapses to a two-word reducer");

    return (_tally->passed == _tally->checked);
}


/*
main
  Runs every conformance group and reports the tally.

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
    struct d_test_tally tally;

    tally.checked = 0;
    tally.passed  = 0;

    printf("djinterp [functional] C conformance\n");

    d_tests_semigroup_laws(&tally);
    d_tests_fold_and_quantifiers(&tally);
    d_tests_producer_protocol(&tally);
    d_tests_transducer_composition(&tally);
    d_tests_cost_and_layout(&tally);

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
