/*******************************************************************************
* djinterp [container]                              ptr_array_tests_destructor.c
*
*
* path:      /tests/djinterp/c/container/array/ptr_array_tests_destructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./ptr_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// counter for tracking deep_free calls
static size_t g_deep_free_call_count = 0;

// helper free function that tracks calls
static
void tracking_free(
    void* _ptr
)
{
    if (_ptr)
    {
        g_deep_free_call_count++;
        free(_ptr);
    }

    return;
}


/******************************************************************************
 * III. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_ptr_array_free
  Tests the d_ptr_array_free function.
  Tests the following:
  - NULL array does nothing (no crash)
  - free empty array
  - free array with elements
  - elements themselves are NOT freed (shallow free)
*/
bool d_tests_ptr_array_free(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int                  values[3];

    d_test_suite_test(_suite, "ptr_array_free");

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    // test 1: NULL array does nothing (no crash)
    d_ptr_array_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_null",
                              // if we get here, no crash
                              true,
                              "NULL array should not crash") && result;

    // test 2: free empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        d_ptr_array_free(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_empty",
                                  // if we get here, no crash
                                  true,
                                  "Free empty array should not crash") && result;
    }

    // test 3: free array with elements (shallow free)
    // the values are stack-allocated, so they should NOT be freed
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        d_ptr_array_free(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_with_elements",
                                  // if we get here, no crash
                                  true,
                                  "Free array with elements should not crash") && result;

        // verify stack values are still accessible (they were NOT freed)
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_shallow",
                                  values[0] == 10 && values[1] == 20 && values[2] == 30,
                                  "Stack values should still be accessible after shallow free") && result;
    }

    return result;
}


/*
d_tests_ptr_array_deep_free
  Tests the d_ptr_array_deep_free function.
  Tests the following:
  - NULL array does nothing (no crash)
  - NULL free function does nothing (no crash)
  - deep free with valid free function
  - all elements are freed
*/
bool d_tests_ptr_array_deep_free(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_array*  arr;
    int*                 heap_values[3];

    d_test_suite_test(_suite, "ptr_array_deep_free");

    result = true;

    // test 1: NULL array does nothing (no crash)
    d_ptr_array_deep_free(NULL, free);
    result = D_TEST_CHECK_MSG(_suite,
                              "deep_free_null_array",
                              // if we get here, no crash
                              true,
                              "NULL array should not crash") && result;

    // test 2: NULL free function does nothing (no crash)
    arr = d_ptr_array_new(10);

    if (arr)
    {
        d_ptr_array_deep_free(arr, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "deep_free_null_fn",
                                  // if we get here, no crash
                                  true,
                                  "NULL free function should not crash") && result;

        // with no free function nothing was freed: the array is still ours
        d_ptr_array_free(arr);
    }

    // test 3: deep free with valid free function
    // allocate heap values
    heap_values[0] = (int*)malloc(sizeof(int));
    heap_values[1] = (int*)malloc(sizeof(int));
    heap_values[2] = (int*)malloc(sizeof(int));

    if (heap_values[0] && heap_values[1] && heap_values[2])
    {
        *(heap_values[0]) = 100;
        *(heap_values[1]) = 200;
        *(heap_values[2]) = 300;

        arr = d_ptr_array_new_from_args(3,
                                        heap_values[0],
                                        heap_values[1],
                                        heap_values[2]);

        if (arr)
        {
            // reset counter
            g_deep_free_call_count = 0;

            // use tracking free function
            d_ptr_array_deep_free(arr, tracking_free);

            result = D_TEST_CHECK_MSG(_suite,
                                      "deep_free_valid",
                                      // if we get here, no crash
                                      true,
                                      "Deep free with valid function should not crash") && result;

            // test 4: verify all elements were freed
            result = D_TEST_CHECK_MSG(_suite,
                                      "deep_free_all_elements",
                                      g_deep_free_call_count == 3,
                                      "All 3 elements should have been freed") && result;
        }
        else
        {
            // cleanup if array creation failed
            free(heap_values[0]);
            free(heap_values[1]);
            free(heap_values[2]);
        }
    }
    else
    {
        // cleanup if malloc failed
        if (heap_values[0])
        {
            free(heap_values[0]);
        }

        if (heap_values[1])
        {
            free(heap_values[1]);
        }

        if (heap_values[2])
        {
            free(heap_values[2]);
        }
    }

    // test 5: deep free with NULL elements in array
    heap_values[0] = (int*)malloc(sizeof(int));
    heap_values[1] = NULL;  // NULL element
    heap_values[2] = (int*)malloc(sizeof(int));

    if (heap_values[0] && heap_values[2])
    {
        *(heap_values[0]) = 111;
        *(heap_values[2]) = 333;

        arr = d_ptr_array_new_from_args(3,
                                        heap_values[0],
                                        heap_values[1],
                                        heap_values[2]);

        if (arr)
        {
            g_deep_free_call_count = 0;
            d_ptr_array_deep_free(arr, tracking_free);

            result = D_TEST_CHECK_MSG(_suite,
                                      "deep_free_skip_null",
                                      g_deep_free_call_count == 2,
                                      "Should free 2 elements (skip NULL)") && result;
        }
        else
        {
            free(heap_values[0]);
            free(heap_values[2]);
        }
    }
    else
    {
        if (heap_values[0])
        {
            free(heap_values[0]);
        }

        if (heap_values[2])
        {
            free(heap_values[2]);
        }
    }

    return result;
}


/*
d_tests_ptr_array_destructor_all
  Aggregation function that runs all destructor tests.
*/
bool d_tests_ptr_array_destructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_array_destructor_all");

    result = true;

    result = d_tests_ptr_array_free(_suite) && result;
    result = d_tests_ptr_array_deep_free(_suite) && result;

    return result;
}
