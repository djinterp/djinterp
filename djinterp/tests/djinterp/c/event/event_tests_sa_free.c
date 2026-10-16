/*******************************************************************************
* djinterp [event]                                         event_tests_sa_free.c
*
*
* path:      /tests/djinterp/c/event/event_tests_sa_free.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./event_tests_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


// d_tests_sa_event_free_dummy_cb
//   helper: minimal callback for listener construction in destructor tests.
static void
d_tests_sa_event_free_dummy_cb(void* _args)
{
    (void)_args;

    return;
}


/*
d_tests_sa_event_free_null
  Tests d_event_free with NULL.
  Tests the following:
  - d_event_free(NULL) does not crash
*/
bool
d_tests_sa_event_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_free(NULL);

    result = d_assert_standalone(
        true,
        "free_event_null",
        "d_event_free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_free_basic
  Tests freeing an event from d_event_new.
  Tests the following:
  - Event from d_event_new is freed without crash
*/
bool
d_tests_sa_event_free_basic
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;

    result = true;
    ev     = d_event_new(100);

    if (ev)
    {
        d_event_free(ev);
    }

    result = d_assert_standalone(
        true,
        "free_event_basic",
        "Freeing event from d_event_new should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_free_with_context
  Tests that d_event_free does NOT free the context pointer.
  Tests the following:
  - Context data is still accessible after event is freed
  - d_event_free only frees the event struct itself
*/
bool
d_tests_sa_event_free_with_context
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* ev;
    int             val;

    result = true;
    val    = 42;
    ev     = d_event_new_ctx(300, &val);

    if (ev)
    {
        d_event_free(ev);

        // val is stack-allocated — should still be 42 after event free
        result = d_assert_standalone(
            val == 42,
            "free_event_context_preserved",
            "Context data should survive d_event_free (caller-owned)",
            _counter) && result;
    }

    return result;
}

/*
d_tests_sa_event_free_sequential
  Tests freeing multiple events sequentially.
  Tests the following:
  - Multiple sequential frees do not crash
*/
bool
d_tests_sa_event_free_sequential
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct d_event* a;
    struct d_event* b;
    struct d_event* c;

    result = true;
    a      = d_event_new(1);
    b      = d_event_new(2);
    c      = d_event_new(3);

    if (a)
    {
        d_event_free(a);
    }

    if (b)
    {
        d_event_free(b);
    }

    if (c)
    {
        d_event_free(c);
    }

    result = d_assert_standalone(
        true,
        "free_event_sequential",
        "Freeing multiple events sequentially should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_free_null
  Tests d_event_listener_free with NULL.
  Tests the following:
  - d_event_listener_free(NULL) does not crash
*/
bool
d_tests_sa_event_listener_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_listener_free(NULL);

    result = d_assert_standalone(
        true,
        "free_listener_null",
        "d_event_listener_free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_free_enabled
  Tests freeing an enabled listener.
  Tests the following:
  - Enabled listener from d_event_listener_new is freed
*/
bool
d_tests_sa_event_listener_free_enabled
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new(
                 10,
                 (fn_callback)d_tests_sa_event_free_dummy_cb,
                 true);

    if (lis)
    {
        d_event_listener_free(lis);
    }

    result = d_assert_standalone(
        true,
        "free_listener_enabled",
        "Freeing enabled listener should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_free_disabled
  Tests freeing a disabled listener.
  Tests the following:
  - Disabled listener from d_event_listener_new is freed
*/
bool
d_tests_sa_event_listener_free_disabled
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new(
                 20,
                 (fn_callback)d_tests_sa_event_free_dummy_cb,
                 false);

    if (lis)
    {
        d_event_listener_free(lis);
    }

    result = d_assert_standalone(
        true,
        "free_listener_disabled",
        "Freeing disabled listener should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_free_default
  Tests freeing a listener from d_event_listener_new_default.
  Tests the following:
  - Default listener is freed without crash
*/
bool
d_tests_sa_event_listener_free_default
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new_default(
                 30,
                 (fn_callback)d_tests_sa_event_free_dummy_cb);

    if (lis)
    {
        d_event_listener_free(lis);
    }

    result = d_assert_standalone(
        true,
        "free_listener_default",
        "Freeing default listener should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_free_sequential
  Tests freeing multiple listeners sequentially.
  Tests the following:
  - Mixed enabled/disabled/default listeners freed sequentially
*/
bool
d_tests_sa_event_listener_free_sequential
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* a;
    struct d_event_listener* b;
    struct d_event_listener* c;

    result = true;
    a      = d_event_listener_new(40, (fn_callback)d_tests_sa_event_free_dummy_cb, true);
    b      = d_event_listener_new_default(50, (fn_callback)d_tests_sa_event_free_dummy_cb);
    c      = d_event_listener_new(60, (fn_callback)d_tests_sa_event_free_dummy_cb, false);

    if (a)
    {
        d_event_listener_free(a);
    }

    if (b)
    {
        d_event_listener_free(b);
    }

    if (c)
    {
        d_event_listener_free(c);
    }

    result = d_assert_standalone(
        true,
        "free_listener_sequential",
        "Freeing multiple listeners sequentially should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_free_all
  Runs all destructor tests.
*/
bool
d_tests_sa_event_free_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Destructors\n");
    printf("  ------------------------\n");

    result = d_tests_sa_event_free_null(_counter)              && result;
    result = d_tests_sa_event_free_basic(_counter)             && result;
    result = d_tests_sa_event_free_with_context(_counter)      && result;
    result = d_tests_sa_event_free_sequential(_counter)        && result;
    result = d_tests_sa_event_listener_free_null(_counter)     && result;
    result = d_tests_sa_event_listener_free_enabled(_counter)  && result;
    result = d_tests_sa_event_listener_free_disabled(_counter) && result;
    result = d_tests_sa_event_listener_free_default(_counter)  && result;
    result = d_tests_sa_event_listener_free_sequential(_counter) && result;

    return result;
}
