#include "./event_tests_sa.h"


/*
d_tests_sa_event_new_basic
  Tests d_event_new basic creation.
  Tests the following:
  - Returns non-NULL
  - id is set correctly
  - args is NULL, num_args is 0
*/
bool
d_tests_sa_event_new_basic
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new(42);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_non_null",
        "d_event_new should return non-NULL",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 42 && ev->args == NULL && ev->num_args == 0,
            "event_new_fields",
            "New event should have id=42, NULL args, num_args=0",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_ids
  Tests d_event_new with various id values.
  Tests the following:
  - Zero id works
  - Positive id works
  - Negative id works
*/
bool
d_tests_sa_event_new_ids
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;

    ev = d_event_new(0);

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 0,
            "event_new_zero_id",
            "d_event_new(0) should set id=0",
            _counter) && result;

        d_event_free(ev);
    }

    ev = d_event_new(-1);

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == -1,
            "event_new_neg_id",
            "d_event_new(-1) should set id=-1",
            _counter) && result;

        d_event_free(ev);
    }

    ev = d_event_new(9999);

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 9999,
            "event_new_large_id",
            "d_event_new(9999) should set id=9999",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_distinct
  Tests that multiple d_event_new calls return distinct pointers.
  Tests the following:
  - Two events have different addresses
  - Two events have different ids
*/
bool
d_tests_sa_event_new_distinct
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev_a;
    struct d_event* ev_b;

    result = true;
    ev_a   = d_event_new(10);
    ev_b   = d_event_new(20);

    if ( (ev_a) &&
         (ev_b) )
    {
        result = d_assert_standalone(
            ev_a != ev_b,
            "event_new_distinct_ptrs",
            "Separate calls should return distinct pointers",
            _counter) && result;

        result = d_assert_standalone(
            ev_a->id == 10 && ev_b->id == 20,
            "event_new_distinct_ids",
            "Events should store their respective ids",
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
d_tests_sa_event_new_args_null
  Tests d_event_new_args with NULL args and 0 size.
  Tests the following:
  - Returns non-NULL
  - args is NULL, num_args is 0
*/
bool
d_tests_sa_event_new_args_null
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new_args(5, NULL, 0);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_args_null_non_null",
        "d_event_new_args with NULL/0 should return non-NULL",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 5 && ev->args == NULL && ev->num_args == 0,
            "event_new_args_null_fields",
            "Should have id=5, NULL args, num_args=0",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_args_valid
  Tests d_event_new_args with valid arguments.
  Tests the following:
  - Returns non-NULL
  - args pointer is stored directly (no copy)
  - num_args reflects passed count
*/
bool
d_tests_sa_event_new_args_valid
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;
    int             val_a;
    int             val_b;
    void*           args[2];

    result  = true;
    val_a   = 100;
    val_b   = 200;
    args[0] = &val_a;
    args[1] = &val_b;

    ev = d_event_new_args(7, args, 2);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_args_valid_non_null",
        "Valid args should create non-NULL event",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 7 && ev->num_args == 2,
            "event_new_args_valid_id_count",
            "Should have id=7 and num_args=2",
            _counter) && result;

        result = d_assert_standalone(
            ev->args == (void*)args,
            "event_new_args_stores_ptr",
            "Args should store the original pointer directly",
            _counter) && result;

        // args points to stack; free struct only
        free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_args_single
  Tests d_event_new_args with a single argument.
  Tests the following:
  - num_args is 1
  - args pointer matches source
*/
bool
d_tests_sa_event_new_args_single
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;
    int             val;
    void*           args[1];

    result  = true;
    val     = 999;
    args[0] = &val;

    ev = d_event_new_args(1, args, 1);

    if (ev)
    {
        result = d_assert_standalone(
            ev->num_args == 1 && ev->args == (void*)args,
            "event_new_args_single",
            "Single-arg event should have num_args=1 and matching ptr",
            _counter) && result;

        free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_args_inconsistent
  Tests d_event_new_args with inconsistent NULL/size combinations.
  Tests the following:
  - NULL args with non-zero size still creates event (stores as-is)
  - Non-NULL args with 0 size still creates event (stores as-is)
*/
bool
d_tests_sa_event_new_args_inconsistent
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;
    int             dummy;
    void*           args[1];

    result  = true;
    dummy   = 0;
    args[0] = &dummy;

    // NULL args with non-zero size
    ev = d_event_new_args(10, NULL, 3);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_args_null_nonzero",
        "NULL args with non-zero size should still create event",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->args == NULL && ev->num_args == 3,
            "event_new_args_null_nonzero_fields",
            "Should store NULL args and num_args=3 as given",
            _counter) && result;

        d_event_free(ev);
    }

    // non-NULL args with 0 size
    ev = d_event_new_args(11, args, 0);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_args_nonnull_zero",
        "Non-NULL args with 0 size should still create event",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->args == (void*)args && ev->num_args == 0,
            "event_new_args_nonnull_zero_fields",
            "Should store args pointer and num_args=0 as given",
            _counter) && result;

        free(ev);
    }

    return result;
}

/*
d_tests_sa_event_constructor_all
  Runs all d_event constructor tests.
*/
bool
d_tests_sa_event_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] d_event Constructors\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_event_new_basic(_counter)            && result;
    result = d_tests_sa_event_new_ids(_counter)              && result;
    result = d_tests_sa_event_new_distinct(_counter)         && result;
    result = d_tests_sa_event_new_args_null(_counter)        && result;
    result = d_tests_sa_event_new_args_valid(_counter)       && result;
    result = d_tests_sa_event_new_args_single(_counter)      && result;
    result = d_tests_sa_event_new_args_inconsistent(_counter) && result;

    return result;
}
