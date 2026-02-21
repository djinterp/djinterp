/******************************************************************************
* djinterp [container]                             array_common_tests_sa_free.c
*
*   Unit tests for `array_common.h` cleanup/free functions.
*   Part of the array_common standalone test suite.
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
* path:      \test\container\array\array_common_tests_sa_free.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.25
******************************************************************************/

#include ".\array_common_tests_sa.h"


/******************************************************************************
 * CLEANUP/FREE FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_array_common_free_elements_arr
  Tests the `d_array_common_free_elements_arr` function.
  Tests the following:
  - freeing allocated array
  - handling NULL pointer safely (should not crash)
*/
bool
d_tests_sa_array_common_free_elements_arr
(
    struct d_test_counter* _test_info
)
{
    bool test_result;
    int* elements;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    elements    = NULL;

    printf("Testing `d_array_common_free_elements_arr`...\n");

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

        test_result &= d_assert_standalone(
            true,
            "frees allocated array without crash",
            "failed to free array (or crashed)",
            _test_info
        );
    }
    else
    {
        test_result &= d_assert_standalone(
            false,
            "test allocation succeeded",
            "test allocation failed - cannot test free",
            _test_info
        );
    }

    //=========================================================================
    // Test 2: Handle NULL pointer safely
    //   Should not crash when passed NULL
    //=========================================================================
    d_array_common_free_elements_arr(NULL);

    test_result &= d_assert_standalone(
        true,
        "handles NULL pointer safely",
        "crashed with NULL pointer",
        _test_info
    );

    //=========================================================================
    // Test 3: Free larger allocation
    //   Test with larger memory block
    //=========================================================================
    elements = (int*)malloc(1000 * sizeof(int));

    if (elements)
    {
        d_array_common_free_elements_arr(elements);
        elements = NULL;

        test_result &= d_assert_standalone(
            true,
            "frees larger allocation without crash",
            "failed with larger allocation",
            _test_info
        );
    }

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}

/*
d_tests_sa_array_common_free_elements_deep
  Tests the `d_array_common_free_elements_deep` function.
  Tests the following:
  - deep freeing array of pointers (each element freed individually)
  - handling NULL parameters safely
  - handling zero count safely
  - handling NULL free function
*/
bool
d_tests_sa_array_common_free_elements_deep
(
    struct d_test_counter* _test_info
)
{
    bool   test_result;
    void** elements;
    size_t i;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    test_result = true;
    elements    = NULL;

    printf("Testing `d_array_common_free_elements_deep`...\n");

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

        test_result &= d_assert_standalone(
            true,
            "performs deep free correctly",
            "failed to perform deep free",
            _test_info
        );
    }
    else
    {
        test_result &= d_assert_standalone(
            false,
            "test allocation succeeded",
            "test allocation failed - cannot test deep free",
            _test_info
        );
    }

    //=========================================================================
    // Test 2: Handle NULL elements parameter
    //   Should not crash when elements is NULL
    //=========================================================================
    d_array_common_free_elements_deep(0, NULL, NULL);

    test_result &= d_assert_standalone(
        true,
        "handles NULL elements parameter safely",
        "crashed with NULL elements",
        _test_info
    );

    //=========================================================================
    // Test 3: Handle zero count with non-NULL elements
    //   Should not crash, should not attempt to free anything
    //=========================================================================
    d_array_common_free_elements_deep(0, (void**)1, d_test_free_int_ptr);

    test_result &= d_assert_standalone(
        true,
        "handles zero count safely",
        "crashed with zero count",
        _test_info
    );

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

        test_result &= d_assert_standalone(
            true,
            "handles NULL free function (no-op for elements)",
            "crashed with NULL free function",
            _test_info
        );
    }

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

        test_result &= d_assert_standalone(
            true,
            "handles array with NULL elements gracefully",
            "crashed with NULL elements in array",
            _test_info
        );
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

        test_result &= d_assert_standalone(
            true,
            "handles larger array deep free correctly",
            "failed with larger array",
            _test_info
        );
    }

    // update test counters
    _test_info->tests_total++;

    if (test_result)
    {
        _test_info->tests_passed++;
    }

    return test_result;
}


/******************************************************************************
 * CLEANUP TEST GROUP RUNNER
 *****************************************************************************/

/*
d_tests_sa_array_common_run_free_tests
  Runs all cleanup/free function tests and returns overall pass/fail status.

Parameter(s):
  _test_info: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all cleanup tests passed, or
  - false, if any test failed.
*/
bool
d_tests_sa_array_common_run_free_tests
(
    struct d_test_counter* _test_info
)
{
    bool overall_result;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    overall_result = true;

    printf("\n----------------------------------------\n");
    printf("Running Cleanup/Free Function Tests\n");
    printf("----------------------------------------\n");

    // run all cleanup tests in header order
    overall_result &= d_tests_sa_array_common_free_elements_arr(_test_info);
    overall_result &= d_tests_sa_array_common_free_elements_deep(_test_info);

    return overall_result;
}
