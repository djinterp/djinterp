/*******************************************************************************
* djinterp [container]                              vector_common_tests_append.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_append.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_append_element
  Tests the d_vector_common_append_element function for appending a single element.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - successful append of single element
  - automatic capacity growth
*/
bool d_tests_vector_common_append_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "vector_common_append_element");

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_element_null_elements",
                                d_vector_common_append_element(NULL, &count, &capacity, sizeof(int), &value) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_element_null_count",
                                d_vector_common_append_element(&elements, NULL, &capacity, sizeof(int), &value) == D_FAILURE,
                                "NULL count pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_element_null_capacity",
                                d_vector_common_append_element(&elements, &count, NULL, sizeof(int), &value) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_element_zero_element_size",
                                d_vector_common_append_element(&elements, &count, &capacity, 0, &value) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 5: successful append to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    value  = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_element_empty_success",
                              d_vector_common_append_element(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Append to empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_element_empty_result",
                              count == 1 && ((int*)elements)[0] == 100,
                              "Count=1, element[0]=100") && result;

    // test 6: append to non-empty vector
    value  = 200;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_element_nonempty_success",
                              d_vector_common_append_element(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Append to non-empty vector should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_element_nonempty_result",
                              count == 2 && ((int*)elements)[1] == 200,
                              "Count=2, element[1]=200") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_append_elements
  Tests the d_vector_common_append_elements function for appending multiple elements.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - zero source_count (no-op success)
  - NULL source with non-zero count failure
  - successful append to empty vector
  - successful append to non-empty vector
  - automatic capacity growth
*/
bool d_tests_vector_common_append_elements(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {100, 200, 300};

    d_test_suite_test(_suite, "vector_common_append_elements");

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_elements_null_elements",
                                d_vector_common_append_elements(NULL, &count, &capacity, sizeof(int), source, 3) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_elements_null_count",
                                d_vector_common_append_elements(&elements, NULL, &capacity, sizeof(int), source, 3) == D_FAILURE,
                                "NULL count pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_elements_null_capacity",
                                d_vector_common_append_elements(&elements, &count, NULL, sizeof(int), source, 3) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "append_elements_zero_element_size",
                                d_vector_common_append_elements(&elements, &count, &capacity, 0, source, 3) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 5: zero source_count is no-op success
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_zero_count",
                              d_vector_common_append_elements(&elements, &count, &capacity, sizeof(int), source, 0) == D_SUCCESS,
                              "Zero source_count should return D_SUCCESS (no-op)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_zero_count_unchanged",
                              count == 0,
                              "Count should remain unchanged") && result;

    // test 6: NULL source with non-zero count should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_null_source",
                              d_vector_common_append_elements(&elements, &count, &capacity, sizeof(int), NULL, 3) == D_FAILURE,
                              "NULL source with non-zero count should return D_FAILURE") && result;

    // test 7: successful append to empty vector
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_empty_success",
                              d_vector_common_append_elements(&elements, &count, &capacity, sizeof(int), source, 3) == D_SUCCESS,
                              "Append to empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_empty_count",
                              count == 3,
                              "Count should be 3") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300);

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_empty_values",
                                  correct,
                                  "Elements should be [100, 200, 300]") && result;
    }

    // test 8: successful append to non-empty vector
    int more_source[2] = {400, 500};
    result             = D_TEST_CHECK_MSG(_suite,
                                          "append_elements_nonempty_success",
                                          d_vector_common_append_elements(&elements, &count, &capacity, sizeof(int), more_source, 2) == D_SUCCESS,
                                          "Append to non-empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_nonempty_count",
                              count == 5,
                              "Count should be 5") && result;

    {
        int* arr = (int*)elements;
        bool correct = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300) &&
                       (arr[3] == 400) && (arr[4] == 500);

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elements_nonempty_values",
                                  correct,
                                  "Elements should be [100, 200, 300, 400, 500]") && result;
    }

    if (elements)
    {
        free(elements);
    }

    // test 9: automatic capacity growth
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 2);

    int large_source[10];
    size_t i;
    for (i = 0; i < 10; i++)
    {
        large_source[i] = (int)(i * 10);
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_grow_success",
                              d_vector_common_append_elements(&elements, &count, &capacity, sizeof(int), large_source, 10) == D_SUCCESS,
                              "Append beyond capacity should succeed (with growth)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_grow_count",
                              count == 10,
                              "Count should be 10") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "append_elements_grow_capacity",
                              capacity >= 10,
                              "Capacity should be at least 10") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_prepend_element
  Tests the d_vector_common_prepend_element function for prepending a single element.
  Tests the following:
  - NULL parameter handling
  - successful prepend to empty vector
  - successful prepend to non-empty vector (shifts existing)
*/
bool d_tests_vector_common_prepend_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "vector_common_prepend_element");

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = D_TEST_CHECK_MSG(_suite,
                                "prepend_element_null_elements",
                                d_vector_common_prepend_element(NULL, &count, &capacity, sizeof(int), &value) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: successful prepend to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    value  = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_element_empty_success",
                              d_vector_common_prepend_element(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Prepend to empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_element_empty_result",
                              count == 1 && ((int*)elements)[0] == 100,
                              "Count=1, element[0]=100") && result;

    // test 3: prepend to non-empty vector (shifts existing)
    value  = 50;
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_element_nonempty_success",
                              d_vector_common_prepend_element(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Prepend to non-empty vector should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 50) && (arr[1] == 100);

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_element_shifted",
                                  correct && count == 2,
                                  "Elements should be [50, 100]") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_prepend_elements
  Tests the d_vector_common_prepend_elements function for prepending multiple elements.
  Tests the following:
  - NULL parameter handling
  - zero source_count (no-op success)
  - NULL source with non-zero count failure
  - successful prepend to empty vector
  - successful prepend to non-empty vector
  - elements correctly shifted
*/
bool d_tests_vector_common_prepend_elements(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {10, 20, 30};

    d_test_suite_test(_suite, "vector_common_prepend_elements");

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "prepend_elements_null_elements",
                                d_vector_common_prepend_elements(NULL, &count, &capacity, sizeof(int), source, 3) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "prepend_elements_null_count",
                                d_vector_common_prepend_elements(&elements, NULL, &capacity, sizeof(int), source, 3) == D_FAILURE,
                                "NULL count pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: zero source_count is no-op success
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_zero_count",
                              d_vector_common_prepend_elements(&elements, &count, &capacity, sizeof(int), source, 0) == D_SUCCESS,
                              "Zero source_count should return D_SUCCESS (no-op)") && result;

    // test 4: NULL source with non-zero count should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_null_source",
                              d_vector_common_prepend_elements(&elements, &count, &capacity, sizeof(int), NULL, 3) == D_FAILURE,
                              "NULL source with non-zero count should fail") && result;

    // test 5: successful prepend to empty vector
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_empty_success",
                              d_vector_common_prepend_elements(&elements, &count, &capacity, sizeof(int), source, 3) == D_SUCCESS,
                              "Prepend to empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_empty_count",
                              count == 3,
                              "Count should be 3") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_empty_values",
                                  correct,
                                  "Elements should be [10, 20, 30]") && result;
    }

    // test 6: successful prepend to non-empty vector (shifts existing)
    int prepend_source[2] = {1, 2};
    result                = D_TEST_CHECK_MSG(_suite,
                                             "prepend_elements_nonempty_success",
                                             d_vector_common_prepend_elements(&elements, &count, &capacity, sizeof(int), prepend_source, 2) == D_SUCCESS,
                                             "Prepend to non-empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elements_nonempty_count",
                              count == 5,
                              "Count should be 5") && result;

    {
        int* arr = (int*)elements;
        bool correct = (arr[0] == 1) && (arr[1] == 2) && (arr[2] == 10) &&
                       (arr[3] == 20) && (arr[4] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elements_nonempty_values",
                                  correct,
                                  "Elements should be [1, 2, 10, 20, 30]") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_append_all
  Aggregation function that runs all append/extend tests.
*/
bool d_tests_vector_common_append_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_append_all");

    result = true;

    result = d_tests_vector_common_append_element(_suite) && result;
    result = d_tests_vector_common_append_elements(_suite) && result;
    result = d_tests_vector_common_prepend_element(_suite) && result;
    result = d_tests_vector_common_prepend_elements(_suite) && result;

    return result;
}
