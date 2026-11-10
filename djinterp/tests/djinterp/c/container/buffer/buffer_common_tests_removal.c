/*******************************************************************************
* djinterp [container]                             buffer_common_tests_removal.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_removal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_remove_element
  Tests the d_buffer_common_remove_element function.
  Tests the following:
  - NULL elements rejection
  - NULL count rejection
  - zero element_size rejection
  - out-of-bounds index rejection
  - successful removal at middle shifts elements
  - successful removal at last element
  - count decremented after removal
*/
bool d_tests_buffer_common_remove_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_remove_element");

    result = true;

    // test 1: NULL elements should fail
    count  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_elem_null_elements",
                              d_buffer_common_remove_element(NULL, &count, sizeof(int), 0) == false,
                              "NULL elements should return false") && result;

    // test 2: NULL count should fail
    {
        int buf[5] = {0};

        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_elem_null_count",
                                  d_buffer_common_remove_element(buf, NULL, sizeof(int), 0) == false,
                                  "NULL count should return false") && result;
    }

    // initialize buffer
    {
        int source[5] = {10, 20, 30, 40, 50};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0);
    }

    if (elements)
    {
        // test 3: out-of-bounds index should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_elem_oob",
                                  d_buffer_common_remove_element(elements, &count, sizeof(int), 5) == false,
                                  "Out-of-bounds index should return false") && result;

        // test 4: remove middle element
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_elem_middle_success",
                                  d_buffer_common_remove_element(elements, &count, sizeof(int), 2) == true,
                                  "Remove at middle should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 4) &&
                           (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 40) && (arr[3] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "remove_elem_middle_result",
                                      correct,
                                      "Elements should be [10, 20, 40, 50]") && result;
        }

        // test 5: remove last element
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_elem_last_success",
                                  d_buffer_common_remove_element(elements, &count, sizeof(int), (dj_index)(count - 1)) == true,
                                  "Remove last should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_elem_last_count",
                                  count == 3,
                                  "Count should be 3") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_remove_range
  Tests the d_buffer_common_remove_range function.
  Tests the following:
  - NULL elements rejection
  - invalid range (start > end) rejection
  - range exceeding count rejection
  - zero-length range is a no-op
  - successful range removal
  - remaining elements shifted correctly
*/
bool d_tests_buffer_common_remove_range(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_remove_range");

    result = true;

    // test 1: NULL elements should fail
    count  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_range_null_elements",
                              d_buffer_common_remove_range(NULL, &count, sizeof(int), 0, 2) == false,
                              "NULL elements should return false") && result;

    // initialize buffer
    {
        int source[6] = {10, 20, 30, 40, 50, 60};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 6, 0);
    }

    if (elements)
    {
        // test 2: invalid range (start > end)
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_invalid",
                                  d_buffer_common_remove_range(elements, &count, sizeof(int), 3, 1) == false,
                                  "start > end should return false") && result;

        // test 3: range exceeding count
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_exceeds",
                                  d_buffer_common_remove_range(elements, &count, sizeof(int), 0, 100) == false,
                                  "Range exceeding count should return false") && result;

        // test 4: zero-length range is a no-op
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_zero_len",
                                  d_buffer_common_remove_range(elements, &count, sizeof(int), 2, 2) == true,
                                  "Zero-length range should return true") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_zero_len_count",
                                  count == 6,
                                  "Count should remain 6") && result;

        // test 5: successful range removal [1, 4)
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_range_success",
                                  d_buffer_common_remove_range(elements, &count, sizeof(int), 1, 4) == true,
                                  "Remove range [1,4) should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 3) &&
                           (arr[0] == 10) && (arr[1] == 50) &&
                           (arr[2] == 60);

            result = D_TEST_CHECK_MSG(_suite,
                                      "remove_range_result",
                                      correct,
                                      "Elements should be [10, 50, 60]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_consume_front
  Tests the d_buffer_common_consume_front function.
  Tests the following:
  - NULL elements rejection
  - amount exceeding count rejection
  - zero amount is a no-op
  - successful consume shifts remaining elements
*/
bool d_tests_buffer_common_consume_front(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_consume_front");

    result = true;

    // test 1: NULL elements should fail
    count  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "consume_front_null",
                              d_buffer_common_consume_front(NULL, &count, sizeof(int), 2) == false,
                              "NULL elements should return false") && result;

    // initialize buffer
    {
        int source[5] = {10, 20, 30, 40, 50};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0);
    }

    if (elements)
    {
        // test 2: amount exceeding count should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_exceeds",
                                  d_buffer_common_consume_front(elements, &count, sizeof(int), 100) == false,
                                  "Amount exceeding count should return false") && result;

        // test 3: zero amount is a no-op
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_zero",
                                  d_buffer_common_consume_front(elements, &count, sizeof(int), 0) == true,
                                  "Zero amount should return true") && result;

        // test 4: successful consume of 2 from front
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_front_success",
                                  d_buffer_common_consume_front(elements, &count, sizeof(int), 2) == true,
                                  "Consume front(2) should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 3) &&
                           (arr[0] == 30) && (arr[1] == 40) &&
                           (arr[2] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "consume_front_result",
                                      correct,
                                      "Elements should be [30, 40, 50]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_consume_back
  Tests the d_buffer_common_consume_back function.
  Tests the following:
  - NULL elements rejection
  - amount exceeding count rejection
  - successful consume from back
  - front elements preserved
*/
bool d_tests_buffer_common_consume_back(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_consume_back");

    result = true;

    // test 1: NULL elements should fail
    count  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "consume_back_null",
                              d_buffer_common_consume_back(NULL, &count, sizeof(int), 2) == false,
                              "NULL elements should return false") && result;

    // initialize buffer
    {
        int source[5] = {10, 20, 30, 40, 50};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0);
    }

    if (elements)
    {
        // test 2: amount exceeding count should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_back_exceeds",
                                  d_buffer_common_consume_back(elements, &count, sizeof(int), 100) == false,
                                  "Amount exceeding count should return false") && result;

        // test 3: successful consume of 2 from back
        result = D_TEST_CHECK_MSG(_suite,
                                  "consume_back_success",
                                  d_buffer_common_consume_back(elements, &count, sizeof(int), 2) == true,
                                  "Consume back(2) should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 3) &&
                           (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "consume_back_result",
                                      correct,
                                      "Elements should be [10, 20, 30]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_removal_all
  Aggregation function that runs all removal tests.
*/
bool d_tests_buffer_common_removal_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_removal_all");

    result = true;

    result = d_tests_buffer_common_remove_element(_suite) && result;
    result = d_tests_buffer_common_remove_range(_suite) && result;
    result = d_tests_buffer_common_consume_front(_suite) && result;
    result = d_tests_buffer_common_consume_back(_suite) && result;

    return result;
}
