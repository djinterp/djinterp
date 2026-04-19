#include "./ptr_array_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// counter for tracking deep_free calls
static size_t g_deep_free_call_count = 0;

// helper free function that tracks calls
static void
tracking_free
(
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
d_tests_sa_ptr_array_free
  Tests the d_ptr_array_free function.
  Tests the following:
  - NULL array does nothing (no crash)
  - free empty array
  - free array with elements
  - elements themselves are NOT freed (shallow free)
*/
bool
d_tests_sa_ptr_array_free
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[3];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    // test 1: NULL array does nothing (no crash)
    d_ptr_array_free(NULL);
    result = d_assert_standalone(
        true,  // if we get here, no crash
        "free_null",
        "NULL array should not crash",
        _counter) && result;

    // test 2: free empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        d_ptr_array_free(arr);
        result = d_assert_standalone(
            true,  // if we get here, no crash
            "free_empty",
            "Free empty array should not crash",
            _counter) && result;
    }

    // test 3: free array with elements (shallow free)
    // the values are stack-allocated, so they should NOT be freed
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        d_ptr_array_free(arr);
        result = d_assert_standalone(
            true,  // if we get here, no crash
            "free_with_elements",
            "Free array with elements should not crash",
            _counter) && result;

        // verify stack values are still accessible (they were NOT freed)
        result = d_assert_standalone(
            values[0] == 10 && values[1] == 20 && values[2] == 30,
            "free_shallow",
            "Stack values should still be accessible after shallow free",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_ptr_array_deep_free
  Tests the d_ptr_array_deep_free function.
  Tests the following:
  - NULL array does nothing (no crash)
  - NULL free function does nothing (no crash)
  - deep free with valid free function
  - all elements are freed
*/
bool
d_tests_sa_ptr_array_deep_free
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int*                 heap_values[3];

    result = true;

    // test 1: NULL array does nothing (no crash)
    d_ptr_array_deep_free(NULL, free);
    result = d_assert_standalone(
        true,  // if we get here, no crash
        "deep_free_null_array",
        "NULL array should not crash",
        _counter) && result;

    // test 2: NULL free function does nothing (no crash)
    arr = d_ptr_array_new(10);

    if (arr)
    {
        d_ptr_array_deep_free(arr, NULL);
        result = d_assert_standalone(
            true,  // if we get here, no crash
            "deep_free_null_fn",
            "NULL free function should not crash (array leaks but no crash)",
            _counter) && result;
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

            result = d_assert_standalone(
                true,  // if we get here, no crash
                "deep_free_valid",
                "Deep free with valid function should not crash",
                _counter) && result;

            // test 4: verify all elements were freed
            result = d_assert_standalone(
                g_deep_free_call_count == 3,
                "deep_free_all_elements",
                "All 3 elements should have been freed",
                _counter) && result;
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

            result = d_assert_standalone(
                g_deep_free_call_count == 2,
                "deep_free_skip_null",
                "Should free 2 elements (skip NULL)",
                _counter) && result;
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
d_tests_sa_ptr_array_destructor_all
  Aggregation function that runs all destructor tests.
*/
bool
d_tests_sa_ptr_array_destructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Destructor Functions\n");
    printf("  -------------------------------\n");

    result = d_tests_sa_ptr_array_free(_counter) && result;
    result = d_tests_sa_ptr_array_deep_free(_counter) && result;

    return result;
}
