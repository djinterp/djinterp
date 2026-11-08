/*******************************************************************************
* djinterp [test]                                         test_cvar_tests_init.c
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests_init.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_cvar_tests.h"


/******************************************************************************
 * IV. INITIALIZATION AND REGISTRY ACCESS TESTS
 *****************************************************************************/

/*
d_tests_cvar_init_safe
  Tests that d_test_registry_init does not crash.
  Tests the following:
  - Calling init completes without error
*/
bool d_tests_cvar_init_safe(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "cvar_init_safe");

    result = true;

    // test 1: init does not crash
    d_test_registry_init();

    result = D_TEST_CHECK_MSG(_suite,
                              "init_safe",
                              true,
                              "d_test_registry_init should not crash") && result;

    return result;
}

/*
d_tests_cvar_init_idempotent
  Tests that calling d_test_registry_init multiple times is safe.
  Tests the following:
  - Double init does not crash
  - Registry pointer is the same after both calls
*/
bool d_tests_cvar_init_idempotent(
    struct dj_test_suite* _suite
)
{
    bool                result;
    struct dj_registry*  reg1;
    struct dj_registry*  reg2;

    d_test_suite_test(_suite, "cvar_init_idempotent");

    result = true;

    // test 1: double init does not crash
    d_test_registry_init();
    d_test_registry_init();

    result = D_TEST_CHECK_MSG(_suite,
                              "init_idempotent_no_crash",
                              true,
                              "Double d_test_registry_init should not crash") && result;

    // test 2: registry pointer is consistent across calls
    reg1 = d_test_registry_registry();
    reg2 = d_test_registry_registry();

    result = D_TEST_CHECK_MSG(_suite,
                              "init_idempotent_same_ptr",
                              reg1 == reg2,
                              "d_test_registry_registry should return the same pointer") && result;

    return result;
}

/*
d_tests_cvar_registry_non_null
  Tests that d_test_registry_registry returns non-NULL.
  Tests the following:
  - Return value is non-NULL
*/
bool d_tests_cvar_registry_non_null(
    struct dj_test_suite* _suite
)
{
    bool                result;
    struct dj_registry*  reg;

    d_test_suite_test(_suite, "cvar_registry_non_null");

    result = true;

    // test 1: registry returns non-NULL
    d_test_registry_init();
    reg = d_test_registry_registry();

    result = D_TEST_CHECK_MSG(_suite,
                              "registry_non_null",
                              reg != NULL,
                              "d_test_registry_registry should return non-NULL") && result;

    return result;
}

/*
d_tests_cvar_registry_row_count
  Tests that the registry has the expected number of rows.
  Tests the following:
  - Registry count is 25 (8 config + 17 metadata)
*/
bool d_tests_cvar_registry_row_count(
    struct dj_test_suite* _suite
)
{
    bool                result;
    struct dj_registry*  reg;

    d_test_suite_test(_suite, "cvar_registry_row_count");

    result = true;

    d_test_registry_init();
    reg = d_test_registry_registry();

    // test 1: row count is 25
    result = D_TEST_CHECK_MSG(_suite,
                              "registry_row_count_25",
                              reg != NULL && reg->count == 25,
                              "Registry should contain 25 rows (8 config + 17 metadata)") && result;

    return result;
}

/*
d_tests_cvar_registry_static_flag
  Tests that the registry has the STATIC_ROWS flag set.
  Tests the following:
  - Registry flags include D_REGISTRY_FLAG_STATIC_ROWS
*/
bool d_tests_cvar_registry_static_flag(
    struct dj_test_suite* _suite
)
{
    bool                result;
    struct dj_registry*  reg;

    d_test_suite_test(_suite, "cvar_registry_static_flag");

    result = true;

    d_test_registry_init();
    reg = d_test_registry_registry();

    // test 1: static rows flag is set
    result = D_TEST_CHECK_MSG(_suite,
                              "registry_static_rows_flag",
                              reg != NULL && (reg->flags & D_REGISTRY_FLAG_STATIC_ROWS) != 0,
                              "Registry should have D_REGISTRY_FLAG_STATIC_ROWS set") && result;

    return result;
}

/*
d_tests_cvar_init_all
  Aggregation function that runs all initialization tests.
*/
bool d_tests_cvar_init_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "cvar_init_all");

    result = true;

    result = d_tests_cvar_init_safe(_suite)          && result;
    result = d_tests_cvar_init_idempotent(_suite)     && result;
    result = d_tests_cvar_registry_non_null(_suite)   && result;
    result = d_tests_cvar_registry_row_count(_suite)  && result;
    result = d_tests_cvar_registry_static_flag(_suite) && result;

    return result;
}
