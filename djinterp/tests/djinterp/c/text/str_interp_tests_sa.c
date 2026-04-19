#include "./str_interp_tests_sa.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_str_interp_setup
  Sets up test environment for string interpolation tests.
*/
bool
d_tests_sa_str_interp_setup
(
    void
)
{
    /* no specific setup needed; present for consistency */

    return true;
}

/*
d_tests_sa_str_interp_teardown
  Cleans up test environment.
*/
bool
d_tests_sa_str_interp_teardown
(
    void
)
{
    /* no specific teardown needed; present for consistency */

    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_sa_str_interp_run_all
  Master test runner for all str_interp module tests.
  Tests the following:
  - Context management (new, free, clear)
  - Specifier management (add, add_n, remove, update, get, has)
  - Interpolation functions (interp, alloc, length)
  - Convenience functions (quick)
  - Utility functions (error_string)
*/
struct d_test_object*
d_tests_sa_str_interp_run_all
(
    void
)
{
    struct d_test_object* group;
    bool                  setup_ok;
    size_t                idx;

    // setup test environment
    setup_ok = d_tests_sa_str_interp_setup();

    if (!setup_ok)
    {
        printf("Failed to setup str_interp test environment\n");

        return NULL;
    }

    // create master group
    group = d_test_object_new_interior("str_interp Module Tests", 5);

    if (!group)
    {
        d_tests_sa_str_interp_teardown();

        return NULL;
    }

    // run all test categories
    idx = 0;
    group->elements[idx++] = d_tests_sa_str_interp_context_all();
    group->elements[idx++] = d_tests_sa_str_interp_specifier_all();
    group->elements[idx++] = d_tests_sa_str_interp_interp_all();
    group->elements[idx++] = d_tests_sa_str_interp_convenience_all();
    group->elements[idx++] = d_tests_sa_str_interp_utility_all();

    // cleanup
    d_tests_sa_str_interp_teardown();

    return group;
}
