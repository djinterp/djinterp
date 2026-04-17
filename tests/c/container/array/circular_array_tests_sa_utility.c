#include "./circular_array_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// helper comparator for int values
static int
int_comparator_util
(
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
static void
tracking_free
(
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
d_tests_sa_circular_array_to_linear_array
  Tests the d_circular_array_to_linear_array function.
  Tests the following:
  - converts to linear array in logical order
  - handles wraparound correctly
  - returns NULL for empty array
*/
bool
d_tests_sa_circular_array_to_linear_array
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int*                      linear;

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
        result = d_assert_standalone(
            linear != NULL,
            "to_linear_valid",
            "Should convert to linear array",
            _counter) && result;

        if (linear)
        {
            result = d_assert_standalone(
                linear[0] == 10 && linear[4] == 50,
                "to_linear_data",
                "Linear array should have correct data",
                _counter) && result;

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
        result = d_assert_standalone(
            linear != NULL,
            "to_linear_wraparound",
            "Should convert with wraparound",
            _counter) && result;

        if (linear)
        {
            result = d_assert_standalone(
                linear[0] == 20 && linear[1] == 30 && linear[2] == 40,
                "to_linear_wraparound_order",
                "Linear array should preserve logical order",
                _counter) && result;

            free(linear);
        }

        d_circular_array_free(arr);
    }

    // test 3: empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        linear = (int*)d_circular_array_to_linear_array(arr);
        result = d_assert_standalone(
            linear == NULL,
            "to_linear_empty",
            "Empty array should return NULL",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    linear = (int*)d_circular_array_to_linear_array(NULL);
    result = d_assert_standalone(
        linear == NULL,
        "to_linear_null",
        "NULL array should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_copy_to
  Tests the d_circular_array_copy_to function.
  Tests the following:
  - copies data to destination buffer
  - handles insufficient capacity
*/
bool
d_tests_sa_circular_array_copy_to
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};
    int                       dest[10];

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: copy to sufficient buffer
        result = d_assert_standalone(
            d_circular_array_copy_to(arr, dest, 10) == true,
            "copy_to_success",
            "Copy to should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest[0] == 10 && dest[1] == 20 && dest[2] == 30,
            "copy_to_data",
            "Destination should have correct data",
            _counter) && result;

        // test 2: copy to insufficient buffer
        result = d_assert_standalone(
            d_circular_array_copy_to(arr, dest, 2) == false,
            "copy_to_insufficient",
            "Copy to insufficient buffer should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: NULL array
    result = d_assert_standalone(
        d_circular_array_copy_to(NULL, dest, 10) == false,
        "copy_to_null_array",
        "Copy to from NULL should fail",
        _counter) && result;

    // test 4: NULL destination
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_push(arr, &values[0]);

        result = d_assert_standalone(
            d_circular_array_copy_to(arr, NULL, 10) == false,
            "copy_to_null_dest",
            "Copy to NULL destination should fail",
            _counter) && result;

        d_circular_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_circular_array_conversion_all
  Aggregation function that runs all conversion tests.
*/
bool
d_tests_sa_circular_array_conversion_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Conversion Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_circular_array_to_linear_array(_counter) && result;
    result = d_tests_sa_circular_array_copy_to(_counter) && result;

    return result;
}


/******************************************************************************
 * VIII. UTILITY AND MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_circular_array_sort
  Tests the d_circular_array_sort function.
  Tests the following:
  - sort orders elements correctly
  - sort linearizes the array
  - sort on empty array
*/
bool
d_tests_sa_circular_array_sort
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {50, 20, 40, 10, 30};
    int*                      elem;

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
        result = d_assert_standalone(
            elem != NULL && *elem == 10,
            "sort_first",
            "First element should be 10 after sort",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 4);
        result = d_assert_standalone(
            elem != NULL && *elem == 50,
            "sort_last",
            "Last element should be 50 after sort",
            _counter) && result;

        // test 2: sort linearizes
        result = d_assert_standalone(
            arr->head == 0,
            "sort_linearized",
            "Array should be linearized after sort",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: sort empty array (should not crash)
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_sort(arr, int_comparator_util);
        result = d_assert_standalone(
            true,
            "sort_empty",
            "Sort on empty should not crash",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: sort NULL array (should not crash)
    d_circular_array_sort(NULL, int_comparator_util);
    result = d_assert_standalone(
        true,
        "sort_null",
        "Sort on NULL should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_linearize
  Tests the d_circular_array_linearize function.
  Tests the following:
  - linearize rearranges elements so head is at 0
  - linearize on already linear array
  - linearize on empty array
*/
bool
d_tests_sa_circular_array_linearize
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40};
    int*                      elem;

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
        result = d_assert_standalone(
            arr->head != 0,
            "linearize_precondition",
            "Array should have non-zero head before linearize",
            _counter) && result;

        // test 1: linearize
        result = d_assert_standalone(
            d_circular_array_linearize(arr) == true,
            "linearize_success",
            "Linearize should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->head == 0,
            "linearize_head_zero",
            "Head should be 0 after linearize",
            _counter) && result;

        elem = (int*)d_circular_array_get(arr, 0);
        result = d_assert_standalone(
            elem != NULL && *elem == 20,
            "linearize_order",
            "Logical order should be preserved",
            _counter) && result;

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

        result = d_assert_standalone(
            d_circular_array_linearize(arr) == true,
            "linearize_already_linear",
            "Linearize on linear array should succeed",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: linearize empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_linearize(arr) == true,
            "linearize_empty",
            "Linearize on empty should succeed",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: linearize NULL array
    result = d_assert_standalone(
        d_circular_array_linearize(NULL) == false,
        "linearize_null",
        "Linearize on NULL should fail",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_free
  Tests the d_circular_array_free function.
  Tests the following:
  - free NULL array (no crash)
  - free empty array
  - free populated array
*/
bool
d_tests_sa_circular_array_free
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    result = true;

    // test 1: free NULL array
    d_circular_array_free(NULL);
    result = d_assert_standalone(
        true,
        "free_null",
        "Free NULL should not crash",
        _counter) && result;

    // test 2: free empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_free(arr);
        result = d_assert_standalone(
            true,
            "free_empty",
            "Free empty array should not crash",
            _counter) && result;
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
        result = d_assert_standalone(
            true,
            "free_populated",
            "Free populated array should not crash",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_circular_array_free_deep
  Tests the d_circular_array_free_deep function.
  Tests the following:
  - free_deep calls free function for each element
  - free_deep with NULL free function
  - free_deep on empty array
*/
bool
d_tests_sa_circular_array_free_deep
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    void*                     heap_values[3];
    result = true;

    // test 1: free_deep NULL array
    d_circular_array_free_deep(NULL, free);
    result = d_assert_standalone(
        true,
        "free_deep_null_array",
        "Free deep NULL should not crash",
        _counter) && result;

    // test 2: free_deep with NULL free function
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        d_circular_array_free_deep(arr, NULL);
        result = d_assert_standalone(
            true,
            "free_deep_null_fn",
            "Free deep with NULL fn should not crash",
            _counter) && result;
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

            result = d_assert_standalone(
                g_deep_free_call_count == 3,
                "free_deep_all_elements",
                "Free deep should call free for all 3 elements",
                _counter) && result;
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

        result = d_assert_standalone(
            g_deep_free_call_count == 0,
            "free_deep_empty",
            "Free deep on empty should call free 0 times",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_circular_array_utility_all
  Aggregation function that runs all utility tests.
*/
bool
d_tests_sa_circular_array_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility and Memory Management Functions\n");
    printf("  ---------------------------------------------------\n");

    result = d_tests_sa_circular_array_sort(_counter) && result;
    result = d_tests_sa_circular_array_linearize(_counter) && result;
    result = d_tests_sa_circular_array_free(_counter) && result;
    result = d_tests_sa_circular_array_free_deep(_counter) && result;

    return result;
}