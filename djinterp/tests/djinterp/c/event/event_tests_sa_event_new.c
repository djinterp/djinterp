/*******************************************************************************
* djinterp [event]                                    event_tests_sa_event_new.c
*
*
* path:      /tests/djinterp/c/event/event_tests_sa_event_new.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./event_tests_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_event_new_basic
  Tests d_event_new basic creation.
  Tests the following:
  - Returns non-NULL
  - id is set correctly
  - context is NULL
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
            ev->id == 42 && ev->context == NULL,
            "event_new_fields",
            "New event should have id=42 and NULL context",
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
  - Negative id works
  - Large id works
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
d_tests_sa_event_new_ctx_null
  Tests d_event_new_ctx with NULL context.
  Tests the following:
  - Returns non-NULL
  - context is NULL
*/
bool
d_tests_sa_event_new_ctx_null
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new_ctx(5, NULL);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_ctx_null_non_null",
        "d_event_new_ctx with NULL should return non-NULL",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 5 && ev->context == NULL,
            "event_new_ctx_null_fields",
            "Should have id=5 and NULL context",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_ctx_valid
  Tests d_event_new_ctx with non-NULL context.
  Tests the following:
  - Returns non-NULL
  - context stores the pointer directly (no copy)
  - Context is accessible through the stored pointer
*/
bool
d_tests_sa_event_new_ctx_valid
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;
    int             val;

    result = true;
    val    = 42;
    ev     = d_event_new_ctx(7, &val);

    result = d_assert_standalone(
        ev != NULL,
        "event_new_ctx_valid_non_null",
        "Valid context should create non-NULL event",
        _counter) && result;

    if (ev)
    {
        result = d_assert_standalone(
            ev->id == 7 && ev->context == &val,
            "event_new_ctx_stores_ptr",
            "Context should store the original pointer directly",
            _counter) && result;

        result = d_assert_standalone(
            *((int*)ev->context) == 42,
            "event_new_ctx_accessible",
            "Context data should be accessible through stored pointer",
            _counter) && result;

        d_event_free(ev);
    }

    return result;
}

/*
d_tests_sa_event_new_ctx_distinct
  Tests that d_event_new_ctx returns distinct events with independent contexts.
  Tests the following:
  - Two events are distinct pointers
  - Each stores its own context
*/
bool
d_tests_sa_event_new_ctx_distinct
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
    val_a  = 100;
    val_b  = 200;
    ev_a   = d_event_new_ctx(1, &val_a);
    ev_b   = d_event_new_ctx(2, &val_b);

    if ( (ev_a) &&
         (ev_b) )
    {
        result = d_assert_standalone(
            ev_a != ev_b &&
            ev_a->context == &val_a &&
            ev_b->context == &val_b,
            "event_new_ctx_distinct",
            "Distinct events should store independent contexts",
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

    result = d_tests_sa_event_new_basic(_counter)        && result;
    result = d_tests_sa_event_new_ids(_counter)          && result;
    result = d_tests_sa_event_new_distinct(_counter)     && result;
    result = d_tests_sa_event_new_ctx_null(_counter)     && result;
    result = d_tests_sa_event_new_ctx_valid(_counter)    && result;
    result = d_tests_sa_event_new_ctx_distinct(_counter) && result;

    return result;
}
