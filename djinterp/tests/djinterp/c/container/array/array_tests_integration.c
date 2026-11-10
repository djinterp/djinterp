/*******************************************************************************
* djinterp [container]                                 array_tests_integration.c
*
* Integration and edge case tests for `array.h` module.
* Part of the array suite, on DTest's C face.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_integration.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_array_null_parameter_safety
  Tests NULL parameter handling across multiple functions.
  Tests the following:
  - all creation functions reject NULL where appropriate
  - all manipulation functions reject NULL arrays
*/
bool d_tests_array_null_parameter_safety(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    int             value = 42;

    d_test_suite_test(_suite, "array_null_parameter_safety");

    // test d_array_new_copy with NULL
    arr = d_array_new_copy(NULL, sizeof(int));

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_new_copy rejects NULL source",
                                    arr == NULL,
                                    "d_array_new_copy accepted NULL source");

    // test d_array_append_element with NULL array
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_append_element rejects NULL array",
                                    d_array_append_element(NULL, sizeof(int), &value) == false,
                                    "d_array_append_element accepted NULL array");

    // test d_array_contains with NULL array
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_contains rejects NULL array",
                                    d_array_contains(NULL, sizeof(int), &value) == false,
                                    "d_array_contains accepted NULL array");

    // test d_array_find with NULL array
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_find returns -1 for NULL array",
                                    d_array_find(NULL, sizeof(int), &value) == -1,
                                    "d_array_find did not return -1 for NULL array");

    return test_result;
}

/*
d_tests_array_zero_element_size_safety
  Tests zero element size handling across multiple functions.
  Tests the following:
  - all creation functions reject zero element size
  - all manipulation functions reject zero element size
*/
bool d_tests_array_zero_element_size_safety(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    struct dj_array* test_arr;
    int             data[] = { 1, 2, 3, 4, 5 };
    int             value = 42;

    d_test_suite_test(_suite, "array_zero_element_size_safety");

    test_arr = d_array_new_from_arr(sizeof(int), data, 5);

    if (!test_arr)
    {
        return false;
    }

    // test d_array_new with zero element size
    arr = d_array_new(0, 10);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_new rejects zero element size",
                                    arr == NULL,
                                    "d_array_new accepted zero element size");

    // test d_array_new_from_arr with zero element size
    arr = d_array_new_from_arr(0, data, 5);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_new_from_arr rejects zero element size",
                                    arr == NULL,
                                    "d_array_new_from_arr accepted zero element size");

    // test d_array_append_element with zero element size
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_append_element rejects zero element size",
                                    d_array_append_element(test_arr, 0, &value) == false,
                                    "d_array_append_element accepted zero element size");

    // test d_array_contains with zero element size
    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_contains rejects zero element size",
                                    d_array_contains(test_arr, 0, &value) == false,
                                    "d_array_contains accepted zero element size");

    d_array_free(test_arr);

    return test_result;
}

/*
d_tests_array_negative_indexing
  Tests negative index support in various functions.
  Tests the following:
  - copy_range with negative indices
  - slice with negative indices
*/
bool d_tests_array_negative_indexing(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    struct dj_array* source;
    int             data[] = { 1, 2, 3, 4, 5 };
    int*            int_elements;

    d_test_suite_test(_suite, "array_negative_indexing");

    source = d_array_new_from_arr(sizeof(int), data, 5);

    if (!source)
    {
        return false;
    }

    // test d_array_new_copy_range with negative indices
    // -3 to -1 -> indices 2 to 4 -> [3, 4, 5]
    arr = d_array_new_copy_range(source, sizeof(int), -3, -1);
    int_elements = arr ? (int*)arr->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_new_copy_range handles negative indices correctly",
                                    arr != NULL && arr->count == 3 && int_elements[0] == 3 && int_elements[2] == 5,
                                    "d_array_new_copy_range failed with negative indices");

    if (arr)
    {
        d_array_free(arr);
    }

    // test d_array_new_copy_reverse with negative indices
    arr = d_array_new_copy_reverse(source, sizeof(int), -3, -1);
    int_elements = arr ? (int*)arr->elements : NULL;

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_new_copy_reverse handles negative indices correctly",
                                    arr != NULL && arr->count == 3 && int_elements[0] == 5 && int_elements[2] == 3,
                                    "d_array_new_copy_reverse failed with negative indices");

    if (arr)
    {
        d_array_free(arr);
    }

    d_array_free(source);

    return test_result;
}

/*
d_tests_array_large_data_sets
  Tests handling of large data sets.
  Tests the following:
  - creating large arrays
  - operations on large arrays
*/
bool d_tests_array_large_data_sets(
    struct dj_test_suite* _suite
)
{
    bool            test_result = true;
    struct dj_array* arr;
    const size_t    large_size = 10000;
    int             fill_value = 42;
    int*            int_elements;

    d_test_suite_test(_suite, "array_large_data_sets");

    // test creating large filled array using d_array_new_fill
    // (d_array_new only allocates space but sets count=0)
    arr = d_array_new_fill(sizeof(int), large_size, &fill_value);

    test_result &= D_TEST_CHECK_MSG(_suite,
                                    "d_array_new_fill handles large size",
                                    arr != NULL && arr->elements != NULL,
                                    "d_array_new_fill failed with large size");

    if (arr)
    {
        // verify the fill worked correctly by checking first and last elements
        int_elements = (int*)arr->elements;

        test_result &= D_TEST_CHECK_MSG(_suite,
                                        "d_array_new_fill fills large array correctly",
                                        arr->count == large_size && int_elements[0] == 42 && int_elements[large_size - 1] == 42,
                                        "d_array_new_fill did not fill large array correctly");

        d_array_free(arr);
    }

    return test_result;
}


/*
===============================================================================
                        INTEGRATION TEST GROUP RUNNER
===============================================================================
*/

/*
d_tests_array_run_integration_tests
  Runs all integration and edge case tests.
*/
bool d_tests_array_run_integration_tests(
    struct dj_test_suite* _suite
)
{
    bool overall_result = true;

    d_test_suite_block(_suite, "array_run_integration_tests");

    printf("\n----------------------------------------\n");
    printf("Running Integration and Edge Case Tests\n");

    overall_result &= d_tests_array_null_parameter_safety(_suite);
    overall_result &= d_tests_array_zero_element_size_safety(_suite);
    overall_result &= d_tests_array_negative_indexing(_suite);
    overall_result &= d_tests_array_large_data_sets(_suite);

    return overall_result;
}
