#include "./test_tests_sa.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

// dummy test function for destruction tests
static bool
d_internal_destroy_dummy_fn(void)
{
    return true;
}


/*
d_tests_sa_test_fn_free_null
  Tests d_test_fn_free with NULL argument.
  Tests the following:
  - d_test_fn_free(NULL) does not crash
*/
bool
d_tests_sa_test_fn_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // should not crash
    d_test_fn_free(NULL);

    result = d_assert_standalone(true,
                                 "d_test_fn_free_null",
                                 "d_test_fn_free(NULL) does not crash",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_fn_free_valid
  Tests d_test_fn_free with a valid test function wrapper.
  Tests the following:
  - freeing a valid d_test_fn does not crash
  - wrapper created with d_test_fn_new is properly deallocated
*/
bool
d_tests_sa_test_fn_free_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test_fn* fn;
    bool              result;

    result = true;

    fn = d_test_fn_new((fn_test)d_internal_destroy_dummy_fn);

    result = d_assert_standalone(fn != NULL,
                                 "d_test_fn_free_valid",
                                 "d_test_fn_new returns non-NULL for freeing",
                                 _counter) && result;

    if (fn)
    {
        // free should not crash
        d_test_fn_free(fn);

        result = d_assert_standalone(true,
                                     "d_test_fn_free_valid",
                                     "d_test_fn_free with valid fn succeeds",
                                     _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_free_null
  Tests d_test_free with NULL argument.
  Tests the following:
  - d_test_free(NULL) does not crash
*/
bool
d_tests_sa_test_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // should not crash
    d_test_free(NULL);

    result = d_assert_standalone(true,
                                 "d_test_free_null",
                                 "d_test_free(NULL) does not crash",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_free_valid
  Tests d_test_free with a valid empty test.
  Tests the following:
  - freeing a valid empty d_test does not crash
  - all internal allocations are released
*/
bool
d_tests_sa_test_free_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    result = d_assert_standalone(test != NULL,
                                 "d_test_free_valid",
                                 "d_test_new returns non-NULL for freeing",
                                 _counter) && result;

    if (test)
    {
        // free should not crash
        d_test_free(test);

        result = d_assert_standalone(true,
                                     "d_test_free_valid",
                                     "d_test_free with valid empty test succeeds",
                                     _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_free_with_children
  Tests d_test_free with a test containing child items.
  Tests the following:
  - freeing a test with children (functions and assertions) does not crash
  - test with stage hooks is properly cleaned up
*/
bool
d_tests_sa_test_free_with_children
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_free_with_children",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // add multiple children
    d_test_add_function(test, (fn_test)d_internal_destroy_dummy_fn);
    d_test_add_function(test, (fn_test)d_internal_destroy_dummy_fn);
    d_test_add_function(test, (fn_test)d_internal_destroy_dummy_fn);

    // verify children were added
    result = d_assert_standalone(d_test_child_count(test) == 3,
                                 "d_test_free_with_children",
                                 "test has 3 children before free",
                                 _counter) && result;

    // set a stage hook to ensure hook map is also freed
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_SETUP,
                          (fn_stage)d_internal_destroy_dummy_fn);

    // free the test with all its children
    d_test_free(test);

    result = d_assert_standalone(true,
                                 "d_test_free_with_children",
                                 "d_test_free with children and hooks succeeds",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_destruction_all
  Aggregation function for all destruction tests.
*/
bool
d_tests_sa_test_destruction_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_fn_free_null(_counter) && result;
    result = d_tests_sa_test_fn_free_valid(_counter) && result;
    result = d_tests_sa_test_free_null(_counter) && result;
    result = d_tests_sa_test_free_valid(_counter) && result;
    result = d_tests_sa_test_free_with_children(_counter) && result;

    return result;
}
