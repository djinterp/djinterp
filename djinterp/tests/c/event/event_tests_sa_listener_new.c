#include "./event_tests_sa.h"


// d_tests_sa_event_lnew_dummy_cb
//   helper: minimal callback for listener construction tests.
static void
d_tests_sa_event_lnew_dummy_cb(void* _args)
{
    (void)_args;

    return;
}

// d_tests_sa_event_lnew_dummy_cb_alt
//   helper: second distinct callback for fn pointer comparison tests.
static void
d_tests_sa_event_lnew_dummy_cb_alt(void* _args)
{
    (void)_args;

    return;
}


/*
d_tests_sa_event_listener_new_null_cb
  Tests d_event_listener_new with NULL callback.
  Tests the following:
  - NULL callback returns NULL
*/
bool
d_tests_sa_event_listener_new_null_cb
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_listener_new(1, NULL, true) == NULL,
        "listener_new_null_cb",
        "NULL callback should return NULL",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_new_enabled
  Tests d_event_listener_new with enabled=true.
  Tests the following:
  - Returns non-NULL
  - id, fn, enabled are set correctly
*/
bool
d_tests_sa_event_listener_new_enabled
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new(
                 10,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb,
                 true);

    result = d_assert_standalone(
        lis != NULL,
        "listener_new_enabled_non_null",
        "Valid callback with enabled=true should return non-NULL",
        _counter) && result;

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == 10 &&
            lis->fn == (fn_callback)d_tests_sa_event_lnew_dummy_cb &&
            lis->enabled == true,
            "listener_new_enabled_fields",
            "Should have id=10, correct fn, enabled=true",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_new_disabled
  Tests d_event_listener_new with enabled=false.
  Tests the following:
  - Returns non-NULL
  - enabled is false
*/
bool
d_tests_sa_event_listener_new_disabled
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new(
                 20,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb,
                 false);

    result = d_assert_standalone(
        lis != NULL,
        "listener_new_disabled_non_null",
        "Valid callback with enabled=false should return non-NULL",
        _counter) && result;

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == 20 && lis->enabled == false,
            "listener_new_disabled_fields",
            "Should have id=20, enabled=false",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_new_ids
  Tests d_event_listener_new with various id values.
  Tests the following:
  - Zero id is accepted
  - Negative id is accepted
*/
bool
d_tests_sa_event_listener_new_ids
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;

    lis = d_event_listener_new(
              0,
              (fn_callback)d_tests_sa_event_lnew_dummy_cb,
              true);

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == 0,
            "listener_new_zero_id",
            "Zero id should be accepted",
            _counter) && result;

        d_event_listener_free(lis);
    }

    lis = d_event_listener_new(
              -5,
              (fn_callback)d_tests_sa_event_lnew_dummy_cb,
              true);

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == -5,
            "listener_new_neg_id",
            "Negative id should be accepted",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_new_distinct
  Tests that multiple calls return distinct pointers with correct fn ptrs.
  Tests the following:
  - Distinct pointers returned
  - Different fn pointers stored correctly
*/
bool
d_tests_sa_event_listener_new_distinct
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* a;
    struct d_event_listener* b;

    result = true;
    a      = d_event_listener_new(
                 30,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb,
                 true);
    b      = d_event_listener_new(
                 40,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb_alt,
                 true);

    if ( (a) &&
         (b) )
    {
        result = d_assert_standalone(
            a != b,
            "listener_new_distinct_ptrs",
            "Multiple calls should return distinct pointers",
            _counter) && result;

        result = d_assert_standalone(
            a->fn != b->fn,
            "listener_new_distinct_fns",
            "Different callbacks should produce different fn ptrs",
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
d_tests_sa_event_listener_default_null_cb
  Tests d_event_listener_new_default with NULL callback.
  Tests the following:
  - NULL callback returns NULL
*/
bool
d_tests_sa_event_listener_default_null_cb
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_listener_new_default(1, NULL) == NULL,
        "listener_default_null_cb",
        "NULL callback should return NULL",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_listener_default_valid
  Tests d_event_listener_new_default with valid callback.
  Tests the following:
  - Returns non-NULL
  - id and fn are set correctly
  - enabled defaults to true
*/
bool
d_tests_sa_event_listener_default_valid
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;
    lis    = d_event_listener_new_default(
                 50,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb);

    result = d_assert_standalone(
        lis != NULL,
        "listener_default_non_null",
        "Valid callback should return non-NULL",
        _counter) && result;

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == 50 &&
            lis->fn == (fn_callback)d_tests_sa_event_lnew_dummy_cb &&
            lis->enabled == true,
            "listener_default_fields",
            "Should have id=50, correct fn, enabled=true",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_default_ids
  Tests d_event_listener_new_default with zero and negative ids.
  Tests the following:
  - Zero id accepted, enabled defaults to true
  - Negative id accepted
*/
bool
d_tests_sa_event_listener_default_ids
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* lis;

    result = true;

    lis = d_event_listener_new_default(
              0,
              (fn_callback)d_tests_sa_event_lnew_dummy_cb);

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == 0 && lis->enabled == true,
            "listener_default_zero_id",
            "Zero id should be accepted with enabled=true",
            _counter) && result;

        d_event_listener_free(lis);
    }

    lis = d_event_listener_new_default(
              -10,
              (fn_callback)d_tests_sa_event_lnew_dummy_cb);

    if (lis)
    {
        result = d_assert_standalone(
            lis->id == -10,
            "listener_default_neg_id",
            "Negative id should be accepted",
            _counter) && result;

        d_event_listener_free(lis);
    }

    return result;
}

/*
d_tests_sa_event_listener_default_distinct
  Tests that multiple new_default calls return distinct pointers.
  Tests the following:
  - Distinct pointers and different ids
*/
bool
d_tests_sa_event_listener_default_distinct
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_listener* a;
    struct d_event_listener* b;

    result = true;
    a      = d_event_listener_new_default(
                 100,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb);
    b      = d_event_listener_new_default(
                 200,
                 (fn_callback)d_tests_sa_event_lnew_dummy_cb_alt);

    if ( (a) &&
         (b) )
    {
        result = d_assert_standalone(
            a != b && a->id != b->id,
            "listener_default_distinct",
            "Multiple new_default calls should return distinct ptrs",
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
d_tests_sa_event_listener_constructor_all
  Runs all d_event_listener constructor tests.
*/
bool
d_tests_sa_event_listener_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] d_event_listener Constructors\n");
    printf("  -------------------------------------------\n");

    result = d_tests_sa_event_listener_new_null_cb(_counter)      && result;
    result = d_tests_sa_event_listener_new_enabled(_counter)      && result;
    result = d_tests_sa_event_listener_new_disabled(_counter)     && result;
    result = d_tests_sa_event_listener_new_ids(_counter)          && result;
    result = d_tests_sa_event_listener_new_distinct(_counter)     && result;
    result = d_tests_sa_event_listener_default_null_cb(_counter)  && result;
    result = d_tests_sa_event_listener_default_valid(_counter)    && result;
    result = d_tests_sa_event_listener_default_ids(_counter)      && result;
    result = d_tests_sa_event_listener_default_distinct(_counter) && result;

    return result;
}
