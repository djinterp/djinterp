/*******************************************************************************
* djinterp [test]                                        test_cvar_tests_reset.c
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_reset.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_cvar_tests.h"


/******************************************************************************
 * VIII. RESET FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_cvar_reset_single
  Tests d_test_registry_reset restores a single value to default.
  NOTE: Uses key-based macros for unambiguous access since DTestConfigKey
  and DTestMetadataFlag enum values overlap.
  Tests the following:
  - Modify config-enabled to false via key, then reset restores default
  - Modify timeout to 9999 via key, then reset restores D_TEST_DEFAULT_TIMEOUT
*/
bool d_tests_cvar_reset_single(
    struct dj_test_suite* _suite
)
{
    bool                        result;
    struct dj_test_registry_row* row;

    d_test_suite_test(_suite, "cvar_reset_single");

    result = true;

    d_test_registry_init();
    d_test_registry_reset_all();

    // test 1: modify config-enabled to false via row, then reset by flag
    row = d_test_registry_find("config-enabled");

    if (row)
    {
        row->value.b = false;
    }

    d_test_registry_reset(D_TEST_OPTIONS_ENABLED);

    result = D_TEST_CHECK_MSG(_suite,
                              "reset_single_enabled",
                              D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
                              "reset should restore 'config-enabled' default (true)") && result;

    // test 2: modify timeout to 9999 via row, then reset by flag
    row = d_test_registry_find("timeout");

    if (row)
    {
        row->value.z = 9999;
    }

    d_test_registry_reset(D_TEST_OPTIONS_TIMEOUT_MS);

    result = D_TEST_CHECK_MSG(_suite,
                              "reset_single_timeout",
                              D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
                              "reset should restore 'timeout' to D_TEST_DEFAULT_TIMEOUT") && result;

    return result;
}

/*
d_tests_cvar_reset_invalid_flag
  Tests d_test_registry_reset with an invalid flag.
  Tests the following:
  - Invalid flag does not crash
*/
bool d_tests_cvar_reset_invalid_flag(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "cvar_reset_invalid_flag");

    result = true;

    d_test_registry_init();

    // test 1: reset with invalid flag does not crash
    d_test_registry_reset(9999);

    result = D_TEST_CHECK_MSG(_suite,
                              "reset_invalid_flag_safe",
                              true,
                              "reset(9999) should not crash") && result;

    // test 2: reset with 0xFFFFFFFF does not crash
    d_test_registry_reset(0xFFFFFFFF);

    result = D_TEST_CHECK_MSG(_suite,
                              "reset_invalid_flag_max_safe",
                              true,
                              "reset(0xFFFFFFFF) should not crash") && result;

    return result;
}

/*
d_tests_cvar_reset_all_values
  Tests d_test_registry_reset_all restores all values to defaults.
  Tests the following:
  - Modify multiple values via row pointers, then reset_all restores all
*/
bool d_tests_cvar_reset_all_values(
    struct dj_test_suite* _suite
)
{
    bool                        result;
    struct dj_test_registry_row* row;

    d_test_suite_test(_suite, "cvar_reset_all_values");

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
    result = D_TEST_CHECK_MSG(_suite,
                              "reset_all_enabled",
                              D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
                              "reset_all should restore 'config-enabled' to true") && result;

    // test 2: timeout restored to default
    result = D_TEST_CHECK_MSG(_suite,
                              "reset_all_timeout",
                              D_TEST_REGISTRY_VALUE_SIZE_T("timeout") == D_TEST_DEFAULT_TIMEOUT,
                              "reset_all should restore 'timeout' to default") && result;

    // test 3: max-failures restored to default (0)
    result = D_TEST_CHECK_MSG(_suite,
                              "reset_all_max_failures",
                              D_TEST_REGISTRY_VALUE_SIZE_T("max-failures") == D_TEST_DEFAULT_MAX_FAILURES,
                              "reset_all should restore 'max-failures' to default") && result;

    // test 4: priority restored to default (0)
    result = D_TEST_CHECK_MSG(_suite,
                              "reset_all_priority",
                              D_TEST_REGISTRY_VALUE_INT32("priority") == 0,
                              "reset_all should restore 'priority' to 0") && result;

    return result;
}

/*
d_tests_cvar_reset_all_idempotent
  Tests that calling d_test_registry_reset_all multiple times is safe.
  Tests the following:
  - Double reset_all does not crash
  - Values remain at defaults after double reset
*/
bool d_tests_cvar_reset_all_idempotent(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "cvar_reset_all_idempotent");

    result = true;

    d_test_registry_init();

    // test 1: double reset does not crash
    d_test_registry_reset_all();
    d_test_registry_reset_all();

    result = D_TEST_CHECK_MSG(_suite,
                              "reset_all_idempotent_no_crash",
                              true,
                              "Double reset_all should not crash") && result;

    // test 2: values remain at defaults (key-based check)
    result = D_TEST_CHECK_MSG(_suite,
                              "reset_all_idempotent_value",
                              D_TEST_REGISTRY_VALUE_BOOL("config-enabled") == true,
                              "Values should remain at defaults after double reset_all") && result;

    return result;
}

/*
d_tests_cvar_reset_all_fn
  Aggregation function that runs all reset function tests.
*/
bool d_tests_cvar_reset_all_fn(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "cvar_reset_all_fn");

    result = true;

    result = d_tests_cvar_reset_single(_suite)         && result;
    result = d_tests_cvar_reset_invalid_flag(_suite)   && result;
    result = d_tests_cvar_reset_all_values(_suite)     && result;
    result = d_tests_cvar_reset_all_idempotent(_suite) && result;

    return result;
}
