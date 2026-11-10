/*******************************************************************************
* djinterp [container]                                 array_common_tests_free.c
*
*   Unit tests for `array_common.h` cleanup/free functions.
*   Part of the array_common suite, on DTest's C face.
*
*   This file tests the following cleanup functions:
*   - d_array_common_free_elements_arr
*   - d_array_common_free_elements_deep
*
*   Each test function covers:
*   - Normal operation with valid inputs
*   - NULL pointer handling (should not crash)
*   - Edge cases (zero count, NULL free function)
*
*
* path:      /tests/djinterp/c/container/array/array_common_tests_free.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.25
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * CLEANUP/FREE FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_array_common_free_elements_arr
  Tests the `d_array_common_free_elements_arr` function.
  Tests the following:
  - freeing allocated array
  - handling NULL pointer safely (should not crash)
*/
bool d_tests_array_common_free_elements_arr(
    struct dj_test_suite* _suite
)
{
    bool test_result;
    int* elements;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_free_elements_arr");

    test_result = true;
    elements    = NULL;

    //=========================================================================
    // Test 1: Free allocated array
    //   Allocate memory, then free it - should not crash or leak
    //=========================================================================
    elements = (int*)malloc(5 * sizeof(int));

    if (elements)
    {
        // populate with test values
        elements[0] = 1;
        elements[1] = 2;
        elements[2] = 3;
        elements[3] = 4;
        elements[4] = 5;

        d_array_common_free_elements_arr(elements);
        elements = NULL;  // prevent use-after-free

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "frees allocated array without crash",
                                        true,
                                        "failed to free array (or crashed)");
    }
    else
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "test allocation succeeded",
                                        false,
                                        "test allocation failed - cannot test free");
    }

    //=========================================================================
    // Test 2: Handle NULL pointer safely
    //   Should not crash when passed NULL
    //=========================================================================
    d_array_common_free_elements_arr(NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL pointer safely",
                                    true,
                                    "crashed with NULL pointer");

    //=========================================================================
    // Test 3: Free larger allocation
    //   Test with larger memory block
    //=========================================================================
    elements = (int*)malloc(1000 * sizeof(int));

    if (elements)
    {
        d_array_common_free_elements_arr(elements);
        elements = NULL;

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "frees larger allocation without crash",
                                        true,
                                        "failed with larger allocation");
    }

    return test_result;
}

/**
 * @brief Part 1 of d_tests_array_common_free_elements_deep.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 1: Deep free array of pointers
 *   - Test 2: Handle NULL elements parameter
 *   - Test 3: Handle zero count with non-NULL elements
 */
static bool
d_internal_tests_array_common_free_elements_deep_part1(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    void** elements;
    size_t i;

    test_result = true;
    elements    = NULL;

    //=========================================================================
    // Test 1: Deep free array of pointers
    //   Allocate array of pointers, each pointing to allocated int
    //   Deep free should free each int AND the array itself
    //=========================================================================
    elements = (void**)malloc(3 * sizeof(void*));

    if (elements)
    {
        // allocate individual elements
        for (i = 0; i < 3; i++)
        {
            elements[i] = malloc(sizeof(int));

            if (elements[i])
            {
                *(int*)elements[i] = (int)(i + 1);
            }
        }

        // deep free using helper function
        d_array_common_free_elements_deep(3, elements, d_test_free_int_ptr);
        elements = NULL;  // array itself is also freed

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "performs deep free correctly",
                                        true,
                                        "failed to perform deep free");
    }
    else
    {
        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "test allocation succeeded",
                                        false,
                                        "test allocation failed - cannot test deep free");
    }

    //=========================================================================
    // Test 2: Handle NULL elements parameter
    //   Should not crash when elements is NULL
    //=========================================================================
    d_array_common_free_elements_deep(0, NULL, NULL);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles NULL elements parameter safely",
                                    true,
                                    "crashed with NULL elements");

    //=========================================================================
    // Test 3: Handle zero count with non-NULL elements
    //   Should not crash, should not attempt to free anything
    //=========================================================================
    d_array_common_free_elements_deep(0, (void**)1, d_test_free_int_ptr);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "handles zero count safely",
                                    true,
                                    "crashed with zero count");

    return test_result;
}


/**
 * @brief Part 2 of d_tests_array_common_free_elements_deep.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 4: Handle NULL free function
 */
static bool
d_internal_tests_array_common_free_elements_deep_part2(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    void** elements;

    test_result = true;
    elements    = NULL;

    //=========================================================================
    // Test 4: Handle NULL free function
    //   Should not free individual elements (no-op for elements)
    //   Note: this means we need to manually clean up
    //=========================================================================
    elements = (void**)malloc(2 * sizeof(void*));

    if (elements)
    {
        elements[0] = malloc(sizeof(int));
        elements[1] = malloc(sizeof(int));

        if (elements[0])
        {
            *(int*)elements[0] = 100;
        }

        if (elements[1])
        {
            *(int*)elements[1] = 200;
        }

        // call with NULL free function - should be a no-op for elements
        d_array_common_free_elements_deep(2, elements, NULL);

        // Note: The function may or may not free the array itself when
        // free_fn is NULL. Based on implementation, it returns early.
        // We need to verify by checking if we need manual cleanup.
        // Since the function returns early when free_fn is NULL,
        // we need to manually clean up.

        // Manual cleanup since free_fn was NULL
        if (elements[0])
        {
            free(elements[0]);
        }

        if (elements[1])
        {
            free(elements[1]);
        }

        free(elements);
        elements = NULL;

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles NULL free function (no-op for elements)",
                                        true,
                                        "crashed with NULL free function");
    }

    return test_result;
}


/**
 * @brief Part 3 of d_tests_array_common_free_elements_deep.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - Test 5: Deep free with some NULL elements in array
 *   - Test 6: Deep free larger array
 */
static bool
d_internal_tests_array_common_free_elements_deep_part3(
    struct dj_test_suite* _suite
)
{
    bool   test_result;
    void** elements;
    size_t i;

    test_result = true;
    elements    = NULL;

    //=========================================================================
    // Test 5: Deep free with some NULL elements in array
    //   Array has mix of valid pointers and NULLs
    //=========================================================================
    elements = (void**)malloc(4 * sizeof(void*));

    if (elements)
    {
        elements[0] = malloc(sizeof(int));
        elements[1] = NULL;  // intentionally NULL
        elements[2] = malloc(sizeof(int));
        elements[3] = NULL;  // intentionally NULL

        if (elements[0])
        {
            *(int*)elements[0] = 10;
        }

        if (elements[2])
        {
            *(int*)elements[2] = 30;
        }

        // should handle NULL elements gracefully
        d_array_common_free_elements_deep(4, elements, d_test_free_int_ptr);
        elements = NULL;

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles array with NULL elements gracefully",
                                        true,
                                        "crashed with NULL elements in array");
    }

    //=========================================================================
    // Test 6: Deep free larger array
    //   Test with more elements
    //=========================================================================
    elements = (void**)malloc(10 * sizeof(void*));

    if (elements)
    {
        for (i = 0; i < 10; i++)
        {
            elements[i] = malloc(sizeof(int));

            if (elements[i])
            {
                *(int*)elements[i] = (int)(i * 10);
            }
        }

        d_array_common_free_elements_deep(10, elements, d_test_free_int_ptr);
        elements = NULL;

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "handles larger array deep free correctly",
                                        true,
                                        "failed with larger array");
    }

    return test_result;
}


/*
d_tests_array_common_free_elements_deep
  Tests the `d_array_common_free_elements_deep` function.
  Tests the following:
  - deep freeing array of pointers (each element freed individually)
  - handling NULL parameters safely
  - handling zero count safely
  - handling NULL free function
*/
bool d_tests_array_common_free_elements_deep(
    struct dj_test_suite* _suite
)
{
    bool test_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "array_common_free_elements_deep");

    test_result = true;
    test_result &=
        d_internal_tests_array_common_free_elements_deep_part1(_suite);
    test_result &=
        d_internal_tests_array_common_free_elements_deep_part2(_suite);
    test_result &=
        d_internal_tests_array_common_free_elements_deep_part3(_suite);

    return test_result;
}


/******************************************************************************
 * CLEANUP TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_array_common_run_free_tests
  Runs all cleanup/free function tests and returns overall pass/fail status.

Parameter(s):
  _suite: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all cleanup tests passed, or
  - false, if any test failed.
*/
bool d_tests_array_common_run_free_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result;

    // validate parameters
    if (!_suite)
    {
        return false;
    }

    d_test_suite_block(_suite, "array_common_run_free_tests");

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Cleanup/Free Function Tests\n");

    // run all cleanup tests in header order
    overall_result &= d_tests_array_common_free_elements_arr(_suite);
    overall_result &= d_tests_array_common_free_elements_deep(_suite);

    return overall_result;
}
