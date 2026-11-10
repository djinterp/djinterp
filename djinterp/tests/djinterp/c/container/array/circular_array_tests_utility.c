/*******************************************************************************
* djinterp [container]                            circular_array_tests_utility.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// helper comparator for int values
static
int int_comparator_util(
    const void* _a,
    const void* _b
)
{
    int a = *(const int*)_a;
    int b = *(const int*)_b;

    return (a > b) - (a < b);
}

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
        // _ptr points to the element in the buffer (which is a void*)
        void* actual_ptr = *(void**)_ptr;

        if (actual_ptr)
        {
            g_deep_free_call_count++;
            free(actual_ptr);
        }
    }

    return;
}


/******************************************************************************
 * VII. CONVERSION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_to_linear_array
  Tests the d_circular_array_to_linear_array function.
  Tests the following:
  - converts to linear array in logical order
  - handles wraparound correctly
  - returns NULL for empty array
*/
bool d_tests_circular_array_to_linear_array(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      linear;

    d_test_suite_test(_suite, "circular_array_to_linear_array");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: convert to linear array
        linear = (int*)d_circular_array_to_linear_array(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_linear_valid",
                                  linear != NULL,
                                  "Should convert to linear array") && result;

        if (linear)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_linear_data",
                                      linear[0] == 10 && linear[4] == 50,
                                      "Linear array should have correct data") && result;

            free(linear);
        }

        d_circular_array_free(arr);
    }

    // test 2: with wraparound
    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        d_circular_array_pop(arr);
        d_circular_array_push(arr, &values[3]);

        // logical order: 20, 30, 40
        linear = (int*)d_circular_array_to_linear_array(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_linear_wraparound",
                                  linear != NULL,
                                  "Should convert with wraparound") && result;

        if (linear)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_linear_wraparound_order",
                                      linear[0] == 20 && linear[1] == 30 && linear[2] == 40,
                                      "Linear array should preserve logical order") && result;

            free(linear);
        }

        d_circular_array_free(arr);
    }

    // test 3: empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        linear = (int*)d_circular_array_to_linear_array(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "to_linear_empty",
                                  linear == NULL,
                                  "Empty array should return NULL") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    linear = (int*)d_circular_array_to_linear_array(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "to_linear_null",
                              linear == NULL,
                              "NULL array should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_copy_to
  Tests the d_circular_array_copy_to function.
  Tests the following:
  - copies data to destination buffer
  - handles insufficient capacity
*/
bool d_tests_circular_array_copy_to(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       dest[10];

    d_test_suite_test(_suite, "circular_array_copy_to");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: copy to sufficient buffer
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_success",
                                  d_circular_array_copy_to(arr, dest, 10) == true,
                                  "Copy to should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_data",
                                  dest[0] == 10 && dest[1] == 20 && dest[2] == 30,
                                  "Destination should have correct data") && result;

        // test 2: copy to insufficient buffer
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_insufficient",
                                  d_circular_array_copy_to(arr, dest, 2) == false,
                                  "Copy to insufficient buffer should fail") && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_array",
                              d_circular_array_copy_to(NULL, dest, 10) == false,
                              "Copy to from NULL should fail") && result;

    // test 4: NULL destination
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push(arr, &values[0]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_null_dest",
                                  d_circular_array_copy_to(arr, NULL, 10) == false,
                                  "Copy to NULL destination should fail") && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_circular_array_conversion_all
  Aggregation function that runs all conversion tests.
*/
bool d_tests_circular_array_conversion_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_conversion_all");

    result = true;

    result = d_tests_circular_array_to_linear_array(_suite) && result;
    result = d_tests_circular_array_copy_to(_suite) && result;

    return result;
}


/******************************************************************************
 * VIII. UTILITY AND MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_sort
  Tests the d_circular_array_sort function.
  Tests the following:
  - sort orders elements correctly
  - sort linearizes the array
  - sort on empty array
*/
bool d_tests_circular_array_sort(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {50, 20, 40, 10, 30};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_sort");

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: sort orders elements
        d_circular_array_sort(arr, int_comparator_util);

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_first",
                                  elem != NULL && *elem == 10,
                                  "First element should be 10 after sort") && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_last",
                                  elem != NULL && *elem == 50,
                                  "Last element should be 50 after sort") && result;

        // test 2: sort linearizes
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_linearized",
                                  arr->head == 0,
                                  "Array should be linearized after sort") && result;

        d_circular_array_free(arr);
    }

    // test 3: sort empty array (should not crash)
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_sort(arr, int_comparator_util);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_empty",
                                  true,
                                  "Sort on empty should not crash") && result;

        d_circular_array_free(arr);
    }

    // test 4: sort NULL array (should not crash)
    d_circular_array_sort(NULL, int_comparator_util);
    result = D_TEST_CHECK_MSG(_suite,
                              "sort_null",
                              true,
                              "Sort on NULL should not crash") && result;

    return result;
}


/*
d_tests_circular_array_linearize
  Tests the d_circular_array_linearize function.
  Tests the following:
  - linearize rearranges elements so head is at 0
  - linearize on already linear array
  - linearize on empty array
*/
bool d_tests_circular_array_linearize(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40};
    int*                      elem;

    d_test_suite_test(_suite, "circular_array_linearize");

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // create wraparound condition
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        d_circular_array_pop(arr);
        d_circular_array_push(arr, &values[3]);

        // logical order: 20, 30, 40 but head != 0
        result = D_TEST_CHECK_MSG(_suite,
                                  "linearize_precondition",
                                  arr->head != 0,
                                  "Array should have non-zero head before linearize") && result;

        // test 1: linearize
        result = D_TEST_CHECK_MSG(_suite,
                                  "linearize_success",
                                  d_circular_array_linearize(arr) == true,
                                  "Linearize should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "linearize_head_zero",
                                  arr->head == 0,
                                  "Head should be 0 after linearize") && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "linearize_order",
                                  elem != NULL && *elem == 20,
                                  "Logical order should be preserved") && result;

        d_circular_array_free(arr);
    }

    // test 2: linearize already linear array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "linearize_already_linear",
                                  d_circular_array_linearize(arr) == true,
                                  "Linearize on linear array should succeed") && result;

        d_circular_array_free(arr);
    }

    // test 3: linearize empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "linearize_empty",
                                  d_circular_array_linearize(arr) == true,
                                  "Linearize on empty should succeed") && result;

        d_circular_array_free(arr);
    }

    // test 4: linearize NULL array
    result = D_TEST_CHECK_MSG(_suite,
                              "linearize_null",
                              d_circular_array_linearize(NULL) == false,
                              "Linearize on NULL should fail") && result;

    return result;
}


/*
d_tests_circular_array_free
  Tests the d_circular_array_free function.
  Tests the following:
  - free NULL array (no crash)
  - free empty array
  - free populated array
*/
bool d_tests_circular_array_free(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    d_test_suite_test(_suite, "circular_array_free");

    result = true;

    // test 1: free NULL array
    d_circular_array_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_null",
                              true,
                              "Free NULL should not crash") && result;

    // test 2: free empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_free(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_empty",
                                  true,
                                  "Free empty array should not crash") && result;
    }

    // test 3: free populated array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        d_circular_array_free(arr);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_populated",
                                  true,
                                  "Free populated array should not crash") && result;
    }

    return result;
}


/*
d_tests_circular_array_free_deep
  Tests the d_circular_array_free_deep function.
  Tests the following:
  - free_deep calls free function for each element
  - free_deep with NULL free function
  - free_deep on empty array
*/
bool d_tests_circular_array_free_deep(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    void*                     heap_values[3];
    d_test_suite_test(_suite, "circular_array_free_deep");

    result = true;

    // test 1: free_deep NULL array
    d_circular_array_free_deep(NULL, free);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_deep_null_array",
                              true,
                              "Free deep NULL should not crash") && result;

    // test 2: free_deep with NULL free function
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_free_deep(arr, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_deep_null_fn",
                                  true,
                                  "Free deep with NULL fn should not crash") && result;
    }

    // test 3: free_deep with tracking
    heap_values[0] = malloc(sizeof(int));
    heap_values[1] = malloc(sizeof(int));
    heap_values[2] = malloc(sizeof(int));

    if (heap_values[0] && heap_values[1] && heap_values[2])
    {
        *(int*)heap_values[0] = 100;
        *(int*)heap_values[1] = 200;
        *(int*)heap_values[2] = 300;

        arr = d_circular_array_new(5, sizeof(void*));

        if (arr)
        {
            for (int i = 0; i < 3; i++)
            {
                d_circular_array_push(arr, &heap_values[i]);
            }

            g_deep_free_call_count = 0;
            d_circular_array_free_deep(arr, tracking_free);

            result = D_TEST_CHECK_MSG(_suite,
                                      "free_deep_all_elements",
                                      g_deep_free_call_count == 3,
                                      "Free deep should call free for all 3 elements") && result;
        }
        else
        {
            free(heap_values[0]);
            free(heap_values[1]);
            free(heap_values[2]);
        }
    }
    else
    {
        if (heap_values[0]) free(heap_values[0]);
        if (heap_values[1]) free(heap_values[1]);
        if (heap_values[2]) free(heap_values[2]);
    }

    // test 4: free_deep on empty array
    arr = d_circular_array_new(5, sizeof(void*));

    if (arr)
    {
        g_deep_free_call_count = 0;
        d_circular_array_free_deep(arr, tracking_free);

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_deep_empty",
                                  g_deep_free_call_count == 0,
                                  "Free deep on empty should call free 0 times") && result;
    }

    return result;
}


/*
d_tests_circular_array_utility_all
  Aggregation function that runs all utility tests.
*/
bool d_tests_circular_array_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_utility_all");

    result = true;

    result = d_tests_circular_array_sort(_suite) && result;
    result = d_tests_circular_array_linearize(_suite) && result;
    result = d_tests_circular_array_free(_suite) && result;
    result = d_tests_circular_array_free_deep(_suite) && result;

    return result;
}
