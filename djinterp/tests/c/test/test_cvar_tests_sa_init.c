#include ".\test_cvar_tests_sa.h"


/******************************************************************************
 * IV. INITIALIZATION AND REGISTRY ACCESS TESTS
 *****************************************************************************/

/*
d_tests_sa_cvar_init_safe
  Tests that d_test_registry_init does not crash.
  Tests the following:
  - Calling init completes without error
*/
bool
d_tests_sa_cvar_init_safe
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: init does not crash
    d_test_registry_init();

    result = d_assert_standalone(
        true,
        "init_safe",
        "d_test_registry_init should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_init_idempotent
  Tests that calling d_test_registry_init multiple times is safe.
  Tests the following:
  - Double init does not crash
  - Registry pointer is the same after both calls
*/
bool
d_tests_sa_cvar_init_idempotent
(
    struct d_test_counter* _counter
)
{
    bool                result;
    struct d_registry*  reg1;
    struct d_registry*  reg2;

    result = true;

    // test 1: double init does not crash
    d_test_registry_init();
    d_test_registry_init();

    result = d_assert_standalone(
        true,
        "init_idempotent_no_crash",
        "Double d_test_registry_init should not crash",
        _counter) && result;

    // test 2: registry pointer is consistent across calls
    reg1 = d_test_registry_registry();
    reg2 = d_test_registry_registry();

    result = d_assert_standalone(
        reg1 == reg2,
        "init_idempotent_same_ptr",
        "d_test_registry_registry should return the same pointer",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_registry_non_null
  Tests that d_test_registry_registry returns non-NULL.
  Tests the following:
  - Return value is non-NULL
*/
bool
d_tests_sa_cvar_registry_non_null
(
    struct d_test_counter* _counter
)
{
    bool                result;
    struct d_registry*  reg;

    result = true;

    // test 1: registry returns non-NULL
    d_test_registry_init();
    reg = d_test_registry_registry();

    result = d_assert_standalone(
        reg != NULL,
        "registry_non_null",
        "d_test_registry_registry should return non-NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_registry_row_count
  Tests that the registry has the expected number of rows.
  Tests the following:
  - Registry count is 25 (8 config + 17 metadata)
*/
bool
d_tests_sa_cvar_registry_row_count
(
    struct d_test_counter* _counter
)
{
    bool                result;
    struct d_registry*  reg;

    result = true;

    d_test_registry_init();
    reg = d_test_registry_registry();

    // test 1: row count is 25
    result = d_assert_standalone(
        reg != NULL && reg->count == 25,
        "registry_row_count_25",
        "Registry should contain 25 rows (8 config + 17 metadata)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_registry_static_flag
  Tests that the registry has the STATIC_ROWS flag set.
  Tests the following:
  - Registry flags include D_REGISTRY_FLAG_STATIC_ROWS
*/
bool
d_tests_sa_cvar_registry_static_flag
(
    struct d_test_counter* _counter
)
{
    bool                result;
    struct d_registry*  reg;

    result = true;

    d_test_registry_init();
    reg = d_test_registry_registry();

    // test 1: static rows flag is set
    result = d_assert_standalone(
        reg != NULL &&
            (reg->flags & D_REGISTRY_FLAG_STATIC_ROWS) != 0,
        "registry_static_rows_flag",
        "Registry should have D_REGISTRY_FLAG_STATIC_ROWS set",
        _counter) && result;

    return result;
}


/*
d_tests_sa_cvar_init_all
  Aggregation function that runs all initialization tests.
*/
bool
d_tests_sa_cvar_init_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Initialization and Registry Access\n");
    printf("  ----------------------\n");

    result = d_tests_sa_cvar_init_safe(_counter)          && result;
    result = d_tests_sa_cvar_init_idempotent(_counter)     && result;
    result = d_tests_sa_cvar_registry_non_null(_counter)   && result;
    result = d_tests_sa_cvar_registry_row_count(_counter)  && result;
    result = d_tests_sa_cvar_registry_static_flag(_counter) && result;

    return result;
}
