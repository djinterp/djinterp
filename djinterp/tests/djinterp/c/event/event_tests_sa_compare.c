/*******************************************************************************
* djinterp [event]                                      event_tests_sa_compare.c
*
*
* path:      /tests/djinterp/c/event/event_tests_sa_compare.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./event_tests_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


// d_tests_sa_event_cmp_dummy_cb
//   helper: minimal callback for listener construction in comparison tests.
static void
d_tests_sa_event_cmp_dummy_cb(void* _args)
{
    (void)_args;

    return;
}

// d_tests_sa_event_cmp_int_comparator
//   helper: fn_comparator that compares two int pointers.
static int
d_tests_sa_event_cmp_int_comparator
(
    const void* _a,
    const void* _b
)
{
    int va;
    int vb;

    if ( (!_a) && (!_b) )
    {
        return 0;
    }
    else if (!_a)
    {
        return -1;
    }
    else if (!_b)
    {
        return 1;
    }

    va = *((const int*)_a);
    vb = *((const int*)_b);

    return (va < vb) ? -1 : (va > vb) ? 1 : 0;
}


/*
d_tests_sa_event_compare_null
  Tests d_event_compare NULL handling.
  Tests the following:
  - Both NULL returns 0
  - First NULL returns negative
  - Second NULL returns positive
*/
bool
d_tests_sa_event_compare_null
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new(1);

    result = d_assert_standalone(
        d_event_compare(NULL, NULL) == 0,
        "event_cmp_both_null",
        "Both NULL should return 0",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            d_event_compare(NULL, ev) < 0,
            "event_cmp_first_null",
            "NULL vs non-NULL should return negative",
            _counter) && result;

        result = d_assert_standalone(
            d_event_compare(ev, NULL) > 0,
            "event_cmp_second_null",
            "Non-NULL vs NULL should return positive",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_compare_ids
  Tests d_event_compare ID ordering.
  Tests the following:
  - Smaller id returns negative
  - Larger id returns positive
  - Equal ids with same context returns 0
*/
bool
d_tests_sa_event_compare_ids
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev_lo;
    struct d_event* ev_hi;
    struct d_event* ev_lo2;

    result = true;
    ev_lo  = d_event_new(10);
    ev_hi  = d_event_new(20);
    ev_lo2 = d_event_new(10);

    if ( (ev_lo) && (ev_hi) && (ev_lo2) )
    {
        result = d_assert_standalone(
            d_event_compare(ev_lo, ev_hi) < 0,
            "event_cmp_less",
            "Smaller id should return negative",
            _counter) && result;

        result = d_assert_standalone(
            d_event_compare(ev_hi, ev_lo) > 0,
            "event_cmp_greater",
            "Larger id should return positive",
            _counter) && result;

        result = d_assert_standalone(
            d_event_compare(ev_lo, ev_lo2) == 0,
            "event_cmp_equal",
            "Equal ids with same NULL context should return 0",
            _counter) && result;
    }

    if (ev_lo)
    {
        d_event_free(ev_lo);
    }

    if (ev_hi)
    {
        d_event_free(ev_hi);
    }

    if (ev_lo2)
    {
        d_event_free(ev_lo2);
    }

    return result;
}

/*
d_tests_sa_event_compare_context
  Tests d_event_compare secondary comparison on context pointer.
  Tests the following:
  - Same id, different context pointers produces non-zero result
  - Same id, same context pointer returns 0
*/
bool
d_tests_sa_event_compare_context
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev_a;
    struct d_event* ev_b;
    int             val_a;
    int             val_b;

    result = true;
    val_a  = 10;
    val_b  = 20;
    ev_a   = d_event_new_ctx(5, &val_a);
    ev_b   = d_event_new_ctx(5, &val_b);

    if ( (ev_a) && (ev_b) )
    {
        result = d_assert_standalone(
            d_event_compare(ev_a, ev_b) != 0,
            "event_cmp_context_diff",
            "Same id, different context ptrs should be non-zero",
            _counter) && result;

        // set both to same context
        ev_b->context = &val_a;

        result = d_assert_standalone(
            d_event_compare(ev_a, ev_b) == 0,
            "event_cmp_context_same",
            "Same id, same context ptr should return 0",
            _counter) && result;
    }

    if (ev_a)
    {
        d_event_free(ev_a);
    }

    if (ev_b)
    {
        d_event_free(ev_b);
    }

    return result;
}

/*
d_tests_sa_event_compare_self
  Tests d_event_compare self-comparison.
  Tests the following:
  - Comparing an event to itself returns 0
*/
bool
d_tests_sa_event_compare_self
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new(42);

    if (ev)
    {
        result = d_assert_standalone(
            d_event_compare(ev, ev) == 0,
            "event_cmp_self",
            "Self-comparison should return 0",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_compare_deep_null
  Tests d_event_compare_deep NULL handling.
  Tests the following:
  - Both NULL returns 0
  - First NULL returns negative
  - Second NULL returns positive
*/
bool
d_tests_sa_event_compare_deep_null
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new(1);

    result = d_assert_standalone(
        d_event_compare_deep(NULL, NULL,
            (fn_comparator)d_tests_sa_event_cmp_int_comparator) == 0,
        "deep_cmp_both_null",
        "Both NULL should return 0",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            d_event_compare_deep(NULL, ev,
                (fn_comparator)d_tests_sa_event_cmp_int_comparator) < 0,
            "deep_cmp_first_null",
            "NULL vs non-NULL should return negative",
            _counter) && result;

        result = d_assert_standalone(
            d_event_compare_deep(ev, NULL,
                (fn_comparator)d_tests_sa_event_cmp_int_comparator) > 0,
            "deep_cmp_second_null",
            "Non-NULL vs NULL should return positive",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_compare_deep_ids
  Tests d_event_compare_deep ID-level comparison.
  Tests the following:
  - Different IDs compare on ID regardless of comparator
*/
bool
d_tests_sa_event_compare_deep_ids
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev_lo;
    struct d_event* ev_hi;

    result = true;
    ev_lo  = d_event_new(1);
    ev_hi  = d_event_new(9);

    if ( (ev_lo) && (ev_hi) )
    {
        result = d_assert_standalone(
            d_event_compare_deep(ev_lo, ev_hi,
                (fn_comparator)d_tests_sa_event_cmp_int_comparator) < 0,
            "deep_cmp_diff_id",
            "Different IDs should compare on ID, not context",
            _counter) && result;
    }

    if (ev_lo)
    {
        d_event_free(ev_lo);
    }

    if (ev_hi)
    {
        d_event_free(ev_hi);
    }

    return result;
}

/*
d_tests_sa_event_compare_deep_context
  Tests d_event_compare_deep with comparator on context.
  Tests the following:
  - Lower context value returns negative via comparator
  - Higher context value returns positive via comparator
  - Self-comparison returns 0
*/
bool
d_tests_sa_event_compare_deep_context
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev_a;
    struct d_event* ev_b;
    int             val_lo;
    int             val_hi;

    result = true;
    val_lo = 10;
    val_hi = 99;
    ev_a   = d_event_new_ctx(5, &val_lo);
    ev_b   = d_event_new_ctx(5, &val_hi);

    if ( (ev_a) && (ev_b) )
    {
        result = d_assert_standalone(
            d_event_compare_deep(ev_a, ev_b,
                (fn_comparator)d_tests_sa_event_cmp_int_comparator) < 0,
            "deep_cmp_context_less",
            "Lower context value should return negative via comparator",
            _counter) && result;

        result = d_assert_standalone(
            d_event_compare_deep(ev_b, ev_a,
                (fn_comparator)d_tests_sa_event_cmp_int_comparator) > 0,
            "deep_cmp_context_greater",
            "Higher context value should return positive via comparator",
            _counter) && result;

        result = d_assert_standalone(
            d_event_compare_deep(ev_a, ev_a,
                (fn_comparator)d_tests_sa_event_cmp_int_comparator) == 0,
            "deep_cmp_self",
            "Self-comparison should return 0",
            _counter) && result;
    }

    if (ev_a)
    {
        d_event_free(ev_a);
    }

    if (ev_b)
    {
        d_event_free(ev_b);
    }

    return result;
}

/*
d_tests_sa_event_compare_deep_no_comparator
  Tests d_event_compare_deep with NULL comparator.
  Tests the following:
  - NULL comparator with non-NULL context does not crash
  - Both NULL context with same id returns 0
*/
bool
d_tests_sa_event_compare_deep_no_comparator
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev_a;
    struct d_event* ev_b;
    int             val_a;
    int             val_b;

    result = true;
    val_a  = 10;
    val_b  = 20;
    ev_a   = d_event_new_ctx(5, &val_a);
    ev_b   = d_event_new_ctx(5, &val_b);

    if ( (ev_a) && (ev_b) )
    {
        // NULL comparator with non-NULL context
        {
            int cmp;

            cmp = d_event_compare_deep(ev_a, ev_b, NULL);

            result = d_assert_standalone(
                cmp == -1 || cmp == 0 || cmp == 1,
                "deep_cmp_null_comparator",
                "NULL comparator should not crash",
                _counter) && result;
        }

        // both NULL context
        ev_a->context = NULL;
        ev_b->context = NULL;

        result = d_assert_standalone(
            d_event_compare_deep(ev_a, ev_b, NULL) == 0,
            "deep_cmp_both_null_context",
            "Both NULL context with same id should return 0",
            _counter) && result;
    }

    if (ev_a)
    {
        d_event_free(ev_a);
    }

    if (ev_b)
    {
        d_event_free(ev_b);
    }

    return result;
}

/*
d_tests_sa_event_listener_compare_null
  Tests d_event_listener_compare NULL handling.
  Tests the following:
  - Both NULL returns 0
  - First NULL returns negative
  - Second NULL returns positive
*/
bool
d_tests_sa_event_listener_compare_null
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new(
                 10,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 true);

    result = d_assert_standalone(
        d_event_listener_compare(NULL, NULL) == 0,
        "listener_cmp_both_null",
        "Both NULL should return 0",
        _counter) && result;

    if (lis)
    {
        result = d_assert_standalone(
            d_event_listener_compare(NULL, lis) < 0,
            "listener_cmp_first_null",
            "NULL vs non-NULL should return negative",
            _counter) && result;

        result = d_assert_standalone(
            d_event_listener_compare(lis, NULL) > 0,
            "listener_cmp_second_null",
            "Non-NULL vs NULL should return positive",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_compare_ids
  Tests d_event_listener_compare ID ordering.
  Tests the following:
  - Smaller id returns negative
  - Larger id returns positive
  - Equal ids returns 0
  - Antisymmetric property holds
*/
bool
d_tests_sa_event_listener_compare_ids
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lo;
    struct d_event_listener* hi;
    struct d_event_listener* lo2;

    result = true;
    lo     = d_event_listener_new(
                 10,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 true);
    hi     = d_event_listener_new(
                 20,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 true);
    lo2    = d_event_listener_new(
                 10,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 false);

    if ( (lo) && (hi) && (lo2) )
    {
        result = d_assert_standalone(
            d_event_listener_compare(lo, hi) < 0,
            "listener_cmp_less",
            "Smaller id should return negative",
            _counter) && result;

        result = d_assert_standalone(
            d_event_listener_compare(hi, lo) > 0,
            "listener_cmp_greater",
            "Larger id should return positive",
            _counter) && result;

        result = d_assert_standalone(
            d_event_listener_compare(lo, lo2) == 0,
            "listener_cmp_equal",
            "Equal ids should return 0",
            _counter) && result;

        result = d_assert_standalone(
            d_event_listener_compare(lo, hi) < 0 &&
            d_event_listener_compare(hi, lo) > 0,
            "listener_cmp_antisymmetric",
            "compare(a,b) and compare(b,a) should have opposite signs",
            _counter) && result;
    }

    if (lo)
    {
        d_event_listener_free(lo);
    }

    if (hi)
    {
        d_event_listener_free(hi);
    }

    if (lo2)
    {
        d_event_listener_free(lo2);
    }

    return result;
}

/*
d_tests_sa_event_listener_compare_self
  Tests d_event_listener_compare self-comparison.
  Tests the following:
  - Comparing a listener to itself returns 0
*/
bool
d_tests_sa_event_listener_compare_self
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new(
                 42,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 true);

    if (lis)
    {
        result = d_assert_standalone(
            d_event_listener_compare(lis, lis) == 0,
            "listener_cmp_self",
            "Self-comparison should return 0",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_compare_ignores_enabled
  Tests that d_event_listener_compare is ID-based only.
  Tests the following:
  - Same ID but different enabled flags returns 0
*/
bool
d_tests_sa_event_listener_compare_ignores_enabled
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* a;
    struct d_event_listener* b;

    result = true;
    a      = d_event_listener_new(
                 50,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 true);
    b      = d_event_listener_new(
                 50,
                 (fn_callback)d_tests_sa_event_cmp_dummy_cb,
                 false);

    if ( (a) && (b) )
    {
        result = d_assert_standalone(
            d_event_listener_compare(a, b) == 0,
            "listener_cmp_ignores_enabled",
            "Compare should be ID-based only, not enabled",
            _counter) && result;
    }

    if (a)
    {
        d_event_listener_free(a);
    }

    if (b)
    {
        d_event_listener_free(b);
    }

    return result;
}

/*
d_tests_sa_event_compare_all
  Runs all comparison function tests.
*/
bool
d_tests_sa_event_compare_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Comparison Functions\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_event_compare_null(_counter)                    && result;
    result = d_tests_sa_event_compare_ids(_counter)                     && result;
    result = d_tests_sa_event_compare_context(_counter)                 && result;
    result = d_tests_sa_event_compare_self(_counter)                    && result;
    result = d_tests_sa_event_compare_deep_null(_counter)               && result;
    result = d_tests_sa_event_compare_deep_ids(_counter)                && result;
    result = d_tests_sa_event_compare_deep_context(_counter)            && result;
    result = d_tests_sa_event_compare_deep_no_comparator(_counter)      && result;
    result = d_tests_sa_event_listener_compare_null(_counter)           && result;
    result = d_tests_sa_event_listener_compare_ids(_counter)            && result;
    result = d_tests_sa_event_listener_compare_self(_counter)           && result;
    result = d_tests_sa_event_listener_compare_ignores_enabled(_counter) && result;

    return result;
}
