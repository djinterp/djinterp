/*******************************************************************************
* djinterp [container]                        circular_array_tests_constructor.c
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_constructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./circular_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_circular_array_new
  Tests the d_circular_array_new function.
  Tests the following:
  - creation with valid parameters
  - zero capacity returns NULL
  - zero element_size returns NULL
  - overflow protection
  - proper initialization of all fields
*/
bool d_tests_circular_array_new(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;

    d_test_suite_test(_suite, "circular_array_new");

    result = true;

    // test 1: creation with valid parameters
    arr = d_circular_array_new(5, sizeof(int));
    result = D_TEST_CHECK_MSG(_suite,
                              "new_valid",
                              arr != NULL,
                              "Should create array with valid parameters") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_capacity",
                                  arr->capacity == 5,
                                  "Capacity should be 5") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_element_size",
                                  arr->element_size == sizeof(int),
                                  "Element size should match sizeof(int)") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_count_zero",
                                  arr->count == 0,
                                  "Count should be 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_head_zero",
                                  arr->head == 0,
                                  "Head should be 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_tail_zero",
                                  arr->tail == 0,
                                  "Tail should be 0") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_elements_allocated",
                                  arr->elements != NULL,
                                  "Elements buffer should be allocated") && result;

        d_circular_array_free(arr);
    }

    // test 2: zero capacity returns NULL
    arr = d_circular_array_new(0, sizeof(int));
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_capacity",
                              arr == NULL,
                              "Zero capacity should return NULL") && result;

    // test 3: zero element_size returns NULL
    arr = d_circular_array_new(5, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_element_size",
                              arr == NULL,
                              "Zero element_size should return NULL") && result;

    // test 4: overflow protection
    arr = d_circular_array_new(SIZE_MAX / 2, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_overflow",
                              arr == NULL,
                              "Overflow should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_new_default_capacity
  Tests the d_circular_array_new_default_capacity function.
  Tests the following:
  - creation with default capacity
  - zero element_size returns NULL
*/
bool d_tests_circular_array_new_default_capacity(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;

    d_test_suite_test(_suite, "circular_array_new_default_capacity");

    result = true;

    // test 1: creation with default capacity
    arr = d_circular_array_new_default_capacity(sizeof(int));
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_valid",
                              arr != NULL,
                              "Should create array with default capacity") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_capacity_value",
                                  arr->capacity == D_CIRCULAR_ARRAY_DEFAULT_CAPACITY,
                                  "Capacity should match default") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_count",
                                  arr->count == 0,
                                  "Count should be 0") && result;

        d_circular_array_free(arr);
    }

    // test 2: zero element_size returns NULL
    arr = d_circular_array_new_default_capacity(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_zero_element_size",
                              arr == NULL,
                              "Zero element_size should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_new_from_arr
  Tests the d_circular_array_new_from_arr function.
  Tests the following:
  - creation from valid array
  - array smaller than capacity
  - array larger than capacity (truncation)
  - NULL source returns NULL
  - zero source count returns NULL
*/
bool d_tests_circular_array_new_from_arr(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       test_data[] = {10, 20, 30, 40, 50};
    int*                      data_ptr;

    d_test_suite_test(_suite, "circular_array_new_from_arr");

    result = true;

    // test 1: array fits exactly
    arr = d_circular_array_new_from_arr(5, sizeof(int), test_data, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_exact_fit",
                              arr != NULL,
                              "Should create array from exact-fit source") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_exact_count",
                                  arr->count == 5,
                                  "Count should be 5") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_exact_wrapped",
                                  arr->head == 0 && arr->tail == 0,
                                  "Full array should have tail wrapped to 0") && result;

        data_ptr = (int*)arr->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_exact_data",
                                  data_ptr[0] == 10 && data_ptr[4] == 50,
                                  "Data should be copied correctly") && result;

        d_circular_array_free(arr);
    }

    // test 2: array smaller than capacity
    arr = d_circular_array_new_from_arr(10, sizeof(int), test_data, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_partial",
                              arr != NULL,
                              "Should create partially filled array") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_partial_count",
                                  arr->count == 3,
                                  "Count should be 3") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_partial_tail",
                                  arr->tail == 3,
                                  "Tail should be 3 for partial fill") && result;

        d_circular_array_free(arr);
    }

    // test 3: array larger than capacity (truncation)
    arr = d_circular_array_new_from_arr(3, sizeof(int), test_data, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_truncate",
                              arr != NULL,
                              "Should create truncated array") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_truncate_count",
                                  arr->count == 3,
                                  "Count should be limited to capacity") && result;

        data_ptr = (int*)arr->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_truncate_data",
                                  data_ptr[0] == 10 && data_ptr[1] == 20 && data_ptr[2] == 30,
                                  "Only first 3 elements should be copied") && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL source returns NULL
    arr = d_circular_array_new_from_arr(5, sizeof(int), NULL, 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_null_source",
                              arr == NULL,
                              "NULL source should return NULL") && result;

    // test 5: zero source count returns NULL
    arr = d_circular_array_new_from_arr(5, sizeof(int), test_data, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_zero_count",
                              arr == NULL,
                              "Zero source count should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_new_from_args
  Tests the d_circular_array_new_from_args function.
  Tests the following:
  - creation from variadic arguments
  - arguments limited by capacity
  - zero arg count returns NULL
*/
bool d_tests_circular_array_new_from_args(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int*                      data_ptr;

    d_test_suite_test(_suite, "circular_array_new_from_args");

    result = true;

    // test 1: creation from variadic arguments
    arr = d_circular_array_new_from_args(5, sizeof(int), 3, 100, 200, 300);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_args_valid",
                              arr != NULL,
                              "Should create array from args") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_args_count",
                                  arr->count == 3,
                                  "Count should be 3") && result;

        data_ptr = (int*)arr->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_args_data",
                                  data_ptr[0] == 100 && data_ptr[1] == 200 && data_ptr[2] == 300,
                                  "Data should match arguments") && result;

        d_circular_array_free(arr);
    }

    // test 2: more args than capacity
    arr = d_circular_array_new_from_args(2, sizeof(int), 5, 1, 2, 3, 4, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_args_truncate",
                              arr != NULL,
                              "Should create array with truncated args") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_args_truncate_count",
                                  arr->count == 2,
                                  "Count should be limited to capacity") && result;

        d_circular_array_free(arr);
    }

    // test 3: zero arg count returns NULL
    arr = d_circular_array_new_from_args(5, sizeof(int), 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_args_zero_count",
                              arr == NULL,
                              "Zero arg count should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_new_copy
  Tests the d_circular_array_new_copy function.
  Tests the following:
  - copy of empty array
  - copy of partially filled array
  - copy with wraparound preserves logical order
  - NULL source returns NULL
  - copy has independent memory
*/
bool d_tests_circular_array_new_copy(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  source;
    struct dj_circular_array*  copy;
    int                       values[5] = {10, 20, 30, 40, 50};
    int*                      data_ptr;

    d_test_suite_test(_suite, "circular_array_new_copy");

    result = true;

    // test 1: copy of empty array
    source = d_circular_array_new(5, sizeof(int));

    if (source)
    {
        copy = d_circular_array_new_copy(source);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_empty",
                                  copy != NULL,
                                  "Should copy empty array") && result;

        if (copy)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_empty_properties",
                                      copy->count == 0 && copy->capacity == 5,
                                      "Copy should have same properties") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_empty_independent",
                                      copy->elements != source->elements,
                                      "Copy should have independent memory") && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 2: copy of partially filled array
    source = d_circular_array_new(5, sizeof(int));

    if (source)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        copy = d_circular_array_new_copy(source);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_partial",
                                  copy != NULL,
                                  "Should copy partially filled array") && result;

        if (copy)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_partial_count",
                                      copy->count == 3,
                                      "Copy should have same count") && result;

            data_ptr = (int*)copy->elements;
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_partial_data",
                                      data_ptr[0] == 10 && data_ptr[1] == 20 && data_ptr[2] == 30,
                                      "Copy should have same data") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_partial_linearized",
                                      copy->head == 0 && copy->tail == 3,
                                      "Copy should be linearized") && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 3: copy with wraparound preserves logical order
    source = d_circular_array_new(3, sizeof(int));

    if (source)
    {
        // fill, then pop and push to create wraparound
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        // After push 10, 20, 30: head=0, tail=0 (wrapped), count=3
        // Elements at physical [0,1,2] = [10, 20, 30]
        d_circular_array_pop(source);
        // After pop: head=1, tail=0, count=2
        // Logical order: 20, 30
        d_circular_array_push(source, &values[3]);
        // After push 40: head=1, tail=1, count=3
        // Physical [0,1,2] = [40, 20, 30], logical order: 20, 30, 40

        copy = d_circular_array_new_copy(source);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_wraparound",
                                  copy != NULL,
                                  "Should copy array with wraparound") && result;

        if (copy)
        {
            int* data_ptr = (int*)copy->elements;
            // logical order should be: 20, 30, 40
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_wraparound_order",
                                      data_ptr[0] == 20 && data_ptr[1] == 30 && data_ptr[2] == 40,
                                      "Copy should preserve logical order") && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 4: NULL source returns NULL
    copy = d_circular_array_new_copy(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_null_source",
                              copy == NULL,
                              "NULL source should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_new_copy_resized
  Tests the d_circular_array_new_copy_resized function.
  Tests the following:
  - copy with larger capacity
  - copy with smaller capacity (truncates)
  - NULL source returns NULL
*/
bool d_tests_circular_array_new_copy_resized(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  source;
    struct dj_circular_array*  copy;
    int                       values[5] = {10, 20, 30, 40, 50};

    d_test_suite_test(_suite, "circular_array_new_copy_resized");

    result = true;

    // test 1: copy with larger capacity
    source = d_circular_array_new(3, sizeof(int));

    if (source)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        copy = d_circular_array_new_copy_resized(source, 10);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_resized_larger",
                                  copy != NULL,
                                  "Should create larger copy") && result;

        if (copy)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_resized_larger_properties",
                                      copy->capacity == 10 && copy->count == 3,
                                      "Copy should have new capacity but same count") && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 2: copy with smaller capacity returns NULL (doesn't truncate)
    source = d_circular_array_new(5, sizeof(int));

    if (source)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        // Implementation returns NULL if new_capacity < count
        copy = d_circular_array_new_copy_resized(source, 3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_resized_smaller",
                                  copy == NULL,
                                  "Smaller capacity than count should return NULL") && result;

        d_circular_array_free(source);
    }

    // test 3: NULL source returns NULL
    copy = d_circular_array_new_copy_resized(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_resized_null",
                              copy == NULL,
                              "NULL source should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_new_fill
  Tests the d_circular_array_new_fill function.
  Tests the following:
  - creation with fill value
  - NULL fill value returns NULL
  - zero capacity returns NULL
*/
bool d_tests_circular_array_new_fill(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_circular_array*  arr;
    int                       fill_value = 42;
    int*                      data_ptr;

    d_test_suite_test(_suite, "circular_array_new_fill");

    result = true;

    // test 1: creation with fill value
    arr = d_circular_array_new_fill(5, sizeof(int), &fill_value);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_valid",
                              arr != NULL,
                              "Should create filled array") && result;

    if (arr)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_count",
                                  arr->count == 5,
                                  "Count should equal capacity") && result;

        data_ptr = (int*)arr->elements;
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_fill_data",
                                  data_ptr[0] == 42 && data_ptr[2] == 42 && data_ptr[4] == 42,
                                  "All elements should be filled") && result;

        d_circular_array_free(arr);
    }

    // test 2: NULL fill value returns NULL
    arr = d_circular_array_new_fill(5, sizeof(int), NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_null_value",
                              arr == NULL,
                              "NULL fill value should return NULL") && result;

    // test 3: zero capacity returns NULL
    arr = d_circular_array_new_fill(0, sizeof(int), &fill_value);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_fill_zero_capacity",
                              arr == NULL,
                              "Zero capacity should return NULL") && result;

    return result;
}


/*
d_tests_circular_array_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool d_tests_circular_array_constructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "circular_array_constructor_all");

    result = true;

    result = d_tests_circular_array_new(_suite) && result;
    result = d_tests_circular_array_new_default_capacity(_suite) && result;
    result = d_tests_circular_array_new_from_arr(_suite) && result;
    result = d_tests_circular_array_new_from_args(_suite) && result;
    result = d_tests_circular_array_new_copy(_suite) && result;
    result = d_tests_circular_array_new_copy_resized(_suite) && result;
    result = d_tests_circular_array_new_fill(_suite) && result;

    return result;
}
