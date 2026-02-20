#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * VIII. RESET FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_reset_single
  Tests d_test_registry_reset restores a single value to default.
  NOTE: Uses key-based macros for unambiguous access since DTestConfigKey
  and DTestMetadataFlag enum values overlap.
  Tests the following:
  - Modify config-enabled to false via key, then reset restores default
  - Modify timeout to 9999 via key, then reset restores D_TEST_DEFAULT_TIMEOUT
*/
bool
d_tests_sa_cvar_reset_single
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: modify config-enabled to false via row, then reset by flag
    row = d_test_registry_find("config-enabled");

    if (row)
    {
        row->value.b = false;
    }

    d_test_registry_reset(D_TEST_CONFIG_ENABLED);

    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
        "reset_single_enabled",
        "reset should restore 'config-enabled' default (true)",
        _counter) && result;

    // test 2: modify timeout to 9999 via row, then reset by flag
    row = d_test_registry_find("timeout");

    if (row)
    {
        row->value.z = 9999;
    }

    d_test_registry_reset(D_TEST_CONFIG_TIMEOUT_MS);

    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
        "reset_single_timeout",
        "reset should restore 'timeout' to D_TEST_DEFAULT_TIMEOUT",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_reset_invalid_flag
  Tests d_test_registry_reset with an invalid flag.
  Tests the following:
  - Invalid flag does not crash
*/
bool
d_tests_sa_cvar_reset_invalid_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: reset with invalid flag does not crash
    d_test_registry_reset(9999);

    result = d_assert_standalone(
        true,
        "reset_invalid_flag_safe",
        "reset(9999) should not crash",
        _counter) && result;

    // test 2: reset with 0xFFFFFFFF does not crash
    d_test_registry_reset(0xFFFFFFFF);

    result = d_assert_standalone(
        true,
        "reset_invalid_flag_max_safe",
        "reset(0xFFFFFFFF) should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_reset_all_values
  Tests d_test_registry_reset_all restores all values to defaults.
  Tests the following:
  - Modify multiple values via row pointers, then reset_all restores all
*/
bool
d_tests_sa_cvar_reset_all_values
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_test_registry_row* row;

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // modify multiple values directly through row pointers
    row = d_test_registry_find("config-enabled");
    if (row) { row->value.b = false; }

    row = d_test_registry_find("timeout");
    if (row) { row->value.z = 5555; }

    row = d_test_registry_find("max-failures");
    if (row) { row->value.z = 100; }

    row = d_test_registry_find("priority");
    if (row) { row->value.i32 = -10; }

    // reset all
    d_test_registry_reset_all();

    // test 1: enabled restored to true
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
        "reset_all_enabled",
        "reset_all should restore 'config-enabled' to true",
        _counter) && result;

    // test 2: timeout restored to default
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
        "reset_all_timeout",
        "reset_all should restore 'timeout' to default",
        _counter) && result;

    // test 3: max-failures restored to default (0)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_SIZE_T("max-failures") ==
            D_TEST_DEFAULT_MAX_FAILURES,
        "reset_all_max_failures",
        "reset_all should restore 'max-failures' to default",
        _counter) && result;

    // test 4: priority restored to default (0)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_INT32("priority") == 0,
        "reset_all_priority",
        "reset_all should restore 'priority' to 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_reset_all_idempotent
  Tests that calling d_test_registry_reset_all multiple times is safe.
  Tests the following:
  - Double reset_all does not crash
  - Values remain at defaults after double reset
*/
bool
d_tests_sa_cvar_reset_all_idempotent
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_test_registry_init();

    // test 1: double reset does not crash
    d_test_registry_reset_all();
    d_test_registry_reset_all();

    result = d_assert_standalone(
        true,
        "reset_all_idempotent_no_crash",
        "Double reset_all should not crash",
        _counter) && result;

    // test 2: values remain at defaults (key-based check)
    result = d_assert_standalone(
        D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
        "reset_all_idempotent_value",
        "Values should remain at defaults after double reset_all",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_reset_all_fn
  Aggregation function that runs all reset function tests.
*/
bool
d_tests_sa_cvar_reset_all_fn
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Reset Functions\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_reset_single(_counter)         && result;
    result = d_tests_sa_cvar_reset_invalid_flag(_counter)   && result;
    result = d_tests_sa_cvar_reset_all_values(_counter)     && result;
    result = d_tests_sa_cvar_reset_all_idempotent(_counter) && result;

    return result;
}
