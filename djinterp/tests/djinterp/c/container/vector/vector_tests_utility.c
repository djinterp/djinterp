/*******************************************************************************
* djinterp [container]                                    vector_tests_utility.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


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


/*
d_tests_vector_swap
  Tests the d_vector_swap function.
  Tests the following:
  - NULL vector rejection
  - empty vector rejection
  - successful swap
  - negative index support
*/
bool d_tests_vector_swap(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_swap");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_null_vector",
                              d_vector_swap(NULL, 0, 1) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: empty vector should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_empty_vector",
                                  d_vector_swap(vec, 0, 1) == D_FAILURE,
                                  "Empty vector should return D_FAILURE") && result;

        d_vector_free(vec);
    }

    // test 3: successful swap
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_success",
                                  d_vector_swap(vec, 0, 2) == D_SUCCESS,
                                  "Swap should succeed") && result;

        {
            int* arr     = (int*)vec->elements;
            bool swapped = (arr[0] == 30) && (arr[2] == 10);

            result = D_TEST_CHECK_MSG(_suite,
                                      "swap_result",
                                      swapped,
                                      "Elements 0 and 2 should be swapped") && result;
        }

        // test 4: negative index support
        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_negative",
                                  d_vector_swap(vec, 0, -1) == D_SUCCESS,
                                  "Swap with negative index should succeed") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_reverse
  Tests the d_vector_reverse function.
  Tests the following:
  - NULL vector rejection
  - successful reverse
*/
bool d_tests_vector_reverse(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_reverse");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_null_vector",
                              d_vector_reverse(NULL) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful reverse
    vec = d_vector_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_success",
                                  d_vector_reverse(vec) == D_SUCCESS,
                                  "Reverse should succeed") && result;

        {
            int* arr = (int*)vec->elements;
            bool reversed = (arr[0] == 5) && (arr[1] == 4) && (arr[2] == 3) &&
                            (arr[3] == 2) && (arr[4] == 1);

            result = D_TEST_CHECK_MSG(_suite,
                                      "reverse_result",
                                      reversed,
                                      "Elements should be [5, 4, 3, 2, 1]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_sort
  Tests the d_vector_sort function.
  Tests the following:
  - NULL vector handling
  - NULL comparator handling
  - successful sort
*/
bool d_tests_vector_sort(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_sort");

    result = true;

    // test 1: NULL vector should not crash
    d_vector_sort(NULL, int_comparator_util);
    result = D_TEST_CHECK_MSG(_suite,
                              "sort_null_vector",
                              true,
                              "NULL vector should not crash") && result;

    // test 2: NULL comparator should not crash
    vec = d_vector_new_from_args(sizeof(int), 3, 30, 10, 20);

    if (vec)
    {
        d_vector_sort(vec, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_null_comparator",
                                  true,
                                  "NULL comparator should not crash") && result;

        // test 3: successful sort
        d_vector_sort(vec, int_comparator_util);

        {
            int* arr    = (int*)vec->elements;
            bool sorted = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "sort_result",
                                      sorted,
                                      "Elements should be sorted [10, 20, 30]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_copy_to
  Tests the d_vector_copy_to function.
  Tests the following:
  - NULL vector rejection
  - NULL destination rejection
  - destination too small rejection
  - successful copy
*/
bool d_tests_vector_copy_to(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              dest[10];

    d_test_suite_test(_suite, "vector_copy_to");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_vector",
                              d_vector_copy_to(NULL, dest, 10) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: NULL destination should fail
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_null_dest",
                                  d_vector_copy_to(vec, NULL, 10) == D_FAILURE,
                                  "NULL destination should return D_FAILURE") && result;

        // test 3: destination too small should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_small_dest",
                                  d_vector_copy_to(vec, dest, 2) == D_FAILURE,
                                  "Small destination should return D_FAILURE") && result;

        // test 4: successful copy
        memset(dest, 0, sizeof(dest));
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_success",
                                  d_vector_copy_to(vec, dest, 10) == D_SUCCESS,
                                  "Copy should succeed") && result;

        {
            bool copied = (dest[0] == 10) && (dest[1] == 20) && (dest[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_to_values",
                                      copied,
                                      "Destination should contain [10, 20, 30]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_utility_all
  Aggregation function that runs all utility tests.
*/
bool d_tests_vector_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_utility_all");

    result = true;

    result = d_tests_vector_swap(_suite) && result;
    result = d_tests_vector_reverse(_suite) && result;
    result = d_tests_vector_sort(_suite) && result;
    result = d_tests_vector_copy_to(_suite) && result;

    return result;
}
