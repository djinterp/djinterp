/*******************************************************************************
* djinterp [test]                                             assert_tests_sa.c
*
*   Master test runner and utility functions for all d_assert unit tests.
*   This file aggregates all test categories and provides common helpers.
*
*
* link:      TBA
* file:      \tests\assert_tests_sa.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_assert_setup
  Sets up test environment for assertion tests.
*/
bool
d_tests_assert_setup
(
    void
)
{
    // no specific setup needed for assertion tests
    // this function is here for consistency and potential future use

    return true;
}

/*
d_tests_assert_teardown
  Cleans up test environment.
*/
bool
d_tests_assert_teardown
(
    void
)
{
    // no specific teardown needed for assertion tests
    // this function is here for consistency and potential future use

    return true;
}

/*
d_test_int_comparator
  Integer comparator for use with assertion functions.

Parameters:
  _a - pointer to first integer
  _b - pointer to second integer

Returns:
  Negative if *_a < *_b, zero if *_a == *_b, positive if *_a > *_b.
  Returns -1 if either pointer is NULL.
*/
int
d_test_int_comparator
(
    const void* _a,
    const void* _b
)
{
    int val_a;
    int val_b;

    if (!_a || !_b)
    {
        return -1;
    }

    val_a = *(const int*)_a;
    val_b = *(const int*)_b;

    if (val_a < val_b)
    {
        return -1;
    }

    if (val_a > val_b)
    {
        return 1;
    }

    return 0;
}

/*
d_test_match_function
  Simple match function that returns true for non-NULL values.

Parameters:
  _value - pointer to check

Returns:
  true if _value is not NULL, false otherwise.
*/
bool
d_test_match_function
(
    const void* _value
)
{
    return _value != NULL;
}

/*
d_test_create_int_standalone
  Allocates and returns a pointer to an int with given value.

Parameters:
  _value - the integer value to store

Returns:
  Pointer to allocated int, or NULL on allocation failure.
  Caller is responsible for freeing the returned memory.
*/
int*
d_test_create_int_standalone
(
    int _value
)
{
    int* result;

    result = (int*)malloc(sizeof(int));

    if (result)
    {
        *result = _value;
    }

    return result;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_all
  Master test runner for all d_assert unit tests.
  Tests the following:
  - Core functions (new, free, assert, assert_false)
  - Comparison functions (eq, neq, lt, lt_eq, gt, gt_eq)
  - String functions (str_eq, str_neq)
  - Null checking functions (null, nonnull)
  - Array functions (array_is_valid, arrays_eq)
  - Utility functions (default_compare)
  - Advanced tests (integration, stress)
*/
struct d_test_object*
d_tests_sa_assert_all
(
    void
)
{
    struct d_test_object* group;
    bool                  setup_success;
    size_t                idx;

    // setup test environment
    setup_success = d_tests_assert_setup();

    if (!setup_success)
    {
        printf("Failed to setup assertion test environment\n");
        return NULL;
    }

    // create master group
    group = d_test_object_new_interior("d_assert Module Tests", 7);

    if (!group)
    {
        d_tests_assert_teardown();
        return NULL;
    }

    // run all test categories
    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_core_all();
    group->elements[idx++] = d_tests_sa_assert_compare_all();
    group->elements[idx++] = d_tests_sa_assert_string_all();
    group->elements[idx++] = d_tests_sa_assert_null_all();
    group->elements[idx++] = d_tests_sa_assert_array_all();
    group->elements[idx++] = d_tests_sa_assert_utility_all();
    group->elements[idx++] = d_tests_sa_assert_advanced_all();

    // cleanup
    d_tests_assert_teardown();

    return group;
}
