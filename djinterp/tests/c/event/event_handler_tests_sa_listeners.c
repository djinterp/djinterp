#include "./event_handler_tests_sa.h"


/******************************************************************************
 * II. LISTENER MANAGEMENT TESTS
 *****************************************************************************/

// d_tests_sa_event_handler_mgmt_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_event_handler_mgmt_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_event_handler_bind_valid
  Tests d_event_handler_bind with valid parameters.
  Tests the following:
  - Returns true
  - listener_count increases
  - Multiple binds work
*/
bool
d_tests_sa_event_handler_bind_valid
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* l1;
    struct d_event_listener* l2;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        l1 = d_event_listener_new(100, d_tests_sa_event_handler_mgmt_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_event_handler_mgmt_cb, false);

        result = d_assert_standalone(
            d_event_handler_bind(handler, l1) == true,
            "eh_bind_valid_returns_true",
            "Bind with valid params should return true",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_listener_count(handler) == 1,
            "eh_bind_valid_count",
            "listener_count should be 1 after first bind",
            _counter) && result;

        d_event_handler_bind(handler, l2);

        result = d_assert_standalone(
            d_event_handler_listener_count(handler) == 2,
            "eh_bind_valid_multiple",
            "listener_count should be 2 after second bind",
            _counter) && result;

        d_event_handler_free(handler);

        if (l1)
        {
            d_event_listener_free(l1);
        }

        if (l2)
        {
            d_event_listener_free(l2);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_bind_update
  Tests d_event_handler_bind with duplicate key (update path).
  Tests the following:
  - Returns true
  - listener_count remains unchanged
*/
bool
d_tests_sa_event_handler_bind_update
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* l_orig;
    struct d_event_listener* l_repl;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        l_orig = d_event_listener_new(100, d_tests_sa_event_handler_mgmt_cb, true);
        l_repl = d_event_listener_new(100, d_tests_sa_event_handler_mgmt_cb, false);

        d_event_handler_bind(handler, l_orig);

        result = d_assert_standalone(
            d_event_handler_bind(handler, l_repl) == true &&
            d_event_handler_listener_count(handler) == 1,
            "eh_bind_update",
            "Rebind same ID should update, count remains 1",
            _counter) && result;

        d_event_handler_free(handler);

        if (l_orig)
        {
            d_event_listener_free(l_orig);
        }

        if (l_repl)
        {
            d_event_listener_free(l_repl);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_bind_null
  Tests d_event_handler_bind with NULL parameters.
  Tests the following:
  - NULL handler returns false
  - NULL listener returns false
*/
bool
d_tests_sa_event_handler_bind_null
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;

    result = true;

    // NULL handler
    lis = d_event_listener_new(300, d_tests_sa_event_handler_mgmt_cb, true);

    result = d_assert_standalone(
        d_event_handler_bind(NULL, lis) == false,
        "eh_bind_null_handler",
        "Bind with NULL handler should return false",
        _counter) && result;

    if (lis)
    {
        d_event_listener_free(lis);
    }

    // NULL listener
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        result = d_assert_standalone(
            d_event_handler_bind(handler, NULL) == false,
            "eh_bind_null_listener",
            "Bind with NULL listener should return false",
            _counter) && result;

        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_unbind_valid
  Tests d_event_handler_unbind with valid parameters.
  Tests the following:
  - Returns true
  - listener_count decreases
  - Listener no longer found via get_listener
*/
bool
d_tests_sa_event_handler_unbind_valid
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* l1;
    struct d_event_listener* l2;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        l1 = d_event_listener_new(100, d_tests_sa_event_handler_mgmt_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_event_handler_mgmt_cb, true);

        d_event_handler_bind(handler, l1);
        d_event_handler_bind(handler, l2);

        result = d_assert_standalone(
            d_event_handler_unbind(handler, 100) == true,
            "eh_unbind_valid_returns_true",
            "Unbind existing listener should return true",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_listener_count(handler) == 1,
            "eh_unbind_valid_count",
            "listener_count should decrease to 1",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_get_listener(handler, 100) == NULL,
            "eh_unbind_valid_gone",
            "Unbound listener should not be found",
            _counter) && result;

        d_event_handler_free(handler);

        if (l1)
        {
            d_event_listener_free(l1);
        }

        if (l2)
        {
            d_event_listener_free(l2);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_unbind_missing_and_null
  Tests d_event_handler_unbind with missing key and NULL handler.
  Tests the following:
  - Missing key returns false
  - NULL handler returns false
*/
bool
d_tests_sa_event_handler_unbind_missing_and_null
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        result = d_assert_standalone(
            d_event_handler_unbind(handler, 999) == false,
            "eh_unbind_missing",
            "Unbind non-existent listener should return false",
            _counter) && result;

        d_event_handler_free(handler);
    }

    result = d_assert_standalone(
        d_event_handler_unbind(NULL, 100) == false,
        "eh_unbind_null",
        "Unbind on NULL handler should return false",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_get_listener_basic
  Tests d_event_handler_get_listener.
  Tests the following:
  - Returns correct listener for existing key
  - Returns NULL for missing key
  - Returns NULL for NULL handler
*/
bool
d_tests_sa_event_handler_get_listener_basic
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        lis = d_event_listener_new(100, d_tests_sa_event_handler_mgmt_cb, true);
        d_event_handler_bind(handler, lis);

        result = d_assert_standalone(
            d_event_handler_get_listener(handler, 100) == lis,
            "eh_get_listener_found",
            "Should return correct listener for existing key",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_get_listener(handler, 999) == NULL,
            "eh_get_listener_missing",
            "Should return NULL for missing key",
            _counter) && result;

        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    result = d_assert_standalone(
        d_event_handler_get_listener(NULL, 100) == NULL,
        "eh_get_listener_null",
        "Should return NULL for NULL handler",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_enable_disable
  Tests d_event_handler_enable_listener and d_event_handler_disable_listener.
  Tests the following:
  - Disable returns true and sets enabled=false
  - enabled_count decreases after disable
  - Enable returns true and sets enabled=true
  - enabled_count increases after enable
  - Disable on non-existent ID returns false
*/
bool
d_tests_sa_event_handler_enable_disable
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        lis = d_event_listener_new(100, d_tests_sa_event_handler_mgmt_cb, true);
        d_event_handler_bind(handler, lis);

        // disable
        result = d_assert_standalone(
            d_event_handler_disable_listener(handler, 100) == true &&
            lis->enabled == false,
            "eh_disable_valid",
            "Disable should return true and set enabled=false",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_enabled_count(handler) == 0,
            "eh_disable_count",
            "enabled_count should be 0 after disable",
            _counter) && result;

        // enable
        result = d_assert_standalone(
            d_event_handler_enable_listener(handler, 100) == true &&
            lis->enabled == true,
            "eh_enable_valid",
            "Enable should return true and set enabled=true",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_enabled_count(handler) == 1,
            "eh_enable_count",
            "enabled_count should be 1 after enable",
            _counter) && result;

        // non-existent
        result = d_assert_standalone(
            d_event_handler_disable_listener(handler, 999) == false,
            "eh_disable_missing",
            "Disable non-existent should return false",
            _counter) && result;

        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_enable_disable_null
  Tests enable/disable with NULL handler.
  Tests the following:
  - Enable returns false for NULL handler
  - Disable returns false for NULL handler
*/
bool
d_tests_sa_event_handler_enable_disable_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_handler_enable_listener(NULL, 100) == false,
        "eh_enable_null",
        "Enable on NULL handler should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_event_handler_disable_listener(NULL, 100) == false,
        "eh_disable_null",
        "Disable on NULL handler should return false",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_listener_mgmt_all
  Runs all listener management tests.
*/
bool
d_tests_sa_event_handler_listener_mgmt_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Listener Management\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_event_handler_bind_valid(_counter)               && result;
    result = d_tests_sa_event_handler_bind_update(_counter)              && result;
    result = d_tests_sa_event_handler_bind_null(_counter)                && result;
    result = d_tests_sa_event_handler_unbind_valid(_counter)             && result;
    result = d_tests_sa_event_handler_unbind_missing_and_null(_counter)  && result;
    result = d_tests_sa_event_handler_get_listener_basic(_counter)       && result;
    result = d_tests_sa_event_handler_enable_disable(_counter)           && result;
    result = d_tests_sa_event_handler_enable_disable_null(_counter)      && result;

    return result;
}
