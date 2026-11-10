/*******************************************************************************
* djinterp [container]                                     vector_tests_resize.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_resize.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_resize
  Tests the d_vector_resize function.
  Tests the following:
  - NULL vector rejection
  - successful shrink
  - successful grow (zero-initialized)
*/
bool d_tests_vector_resize(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_resize");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_null_vector",
                              d_vector_resize(NULL, 10) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful shrink
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 30, 40, 50);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_shrink_success",
                                  d_vector_resize(vec, 3) == D_SUCCESS,
                                  "Resize shrink should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_shrink_count",
                                  vec->count == 3,
                                  "Count should be 3") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "resize_shrink_values",
                                      correct,
                                      "First 3 elements preserved") && result;
        }

        // test 3: successful grow (zero-initialized)
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_grow_success",
                                  d_vector_resize(vec, 5) == D_SUCCESS,
                                  "Resize grow should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_grow_count",
                                  vec->count == 5,
                                  "Count should be 5") && result;

        {
            int* arr          = (int*)vec->elements;
            bool original_ok  = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);
            bool new_zeroed   = (arr[3] == 0) && (arr[4] == 0);

            result = D_TEST_CHECK_MSG(_suite,
                                      "resize_grow_values",
                                      original_ok && new_zeroed,
                                      "Original preserved, new elements zero-initialized") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_resize_fill
  Tests the d_vector_resize_fill function.
  Tests the following:
  - NULL vector rejection
  - successful shrink (fill ignored)
  - successful grow with fill value
  - NULL fill when growing should fail
*/
bool d_tests_vector_resize_fill(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              fill_value;

    d_test_suite_test(_suite, "vector_resize_fill");

    result = true;

    // test 1: NULL vector should fail
    fill_value = 99;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_null_vector",
                                  d_vector_resize_fill(NULL, 10, &fill_value) == D_FAILURE,
                                  "NULL vector should return D_FAILURE") && result;

    // test 2: successful shrink (fill ignored)
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 30, 40, 50);

    if (vec)
    {
        fill_value = 999;
        result     = D_TEST_CHECK_MSG(_suite,
                                      "resize_fill_shrink_success",
                                      d_vector_resize_fill(vec, 2, &fill_value) == D_SUCCESS,
                                      "Resize fill shrink should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_shrink_count",
                                  vec->count == 2,
                                  "Count should be 2") && result;

        // test 3: successful grow with fill value
        fill_value = 777;
        result     = D_TEST_CHECK_MSG(_suite,
                                      "resize_fill_grow_success",
                                      d_vector_resize_fill(vec, 5, &fill_value) == D_SUCCESS,
                                      "Resize fill grow should succeed") && result;

        {
            int* arr          = (int*)vec->elements;
            bool original_ok  = (arr[0] == 10) && (arr[1] == 20);
            bool new_filled   = (arr[2] == 777) && (arr[3] == 777) && (arr[4] == 777);

            result = D_TEST_CHECK_MSG(_suite,
                                      "resize_fill_grow_values",
                                      original_ok && new_filled,
                                      "Original preserved, new elements filled with 777") && result;
        }

        // test 4: NULL fill when growing should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_null_grow",
                                  d_vector_resize_fill(vec, 10, NULL) == D_FAILURE,
                                  "NULL fill when growing should fail") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_resize_all
  Aggregation function that runs all resize tests.
*/
bool d_tests_vector_resize_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_resize_all");

    result = true;

    result = d_tests_vector_resize(_suite) && result;
    result = d_tests_vector_resize_fill(_suite) && result;

    return result;
}
