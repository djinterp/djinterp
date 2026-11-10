/*******************************************************************************
* djinterp [container]                              buffer_common_tests_insert.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_insert.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_append_element
  Tests the d_buffer_common_append_element function.
  Tests the following:
  - NULL elements rejection
  - NULL count rejection
  - NULL value rejection
  - zero element_size rejection
  - full buffer rejection
  - successful append to empty buffer
  - successful append to non-empty buffer
*/
bool d_tests_buffer_common_append_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "buffer_common_append_element");

    result = true;

    // test 1: NULL elements should fail
    count = 0;
    value = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_elem_null_elements",
                              d_buffer_common_append_element(NULL, &count, 10, sizeof(int), &value) == false,
                              "NULL elements should return false") && result;

    // test 2: NULL count should fail
    {
        int buf[10] = {0};

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elem_null_count",
                                  d_buffer_common_append_element(buf, NULL, 10, sizeof(int), &value) == false,
                                  "NULL count should return false") && result;
    }

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_sized(&elements, &count, &capacity,
                               sizeof(int), 4);

    if (elements)
    {
        // test 3: successful append to empty buffer
        value  = 100;
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elem_success",
                                  d_buffer_common_append_element(elements, &count, capacity, sizeof(int), &value) == true,
                                  "Append to empty should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elem_result",
                                  count == 1 && ((int*)elements)[0] == 100,
                                  "Count=1, element[0]=100") && result;

        // test 4: append to non-empty buffer
        value  = 200;
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elem_nonempty",
                                  d_buffer_common_append_element(elements, &count, capacity, sizeof(int), &value) == true,
                                  "Append to non-empty should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elem_nonempty_result",
                                  count == 2 && ((int*)elements)[1] == 200,
                                  "Count=2, element[1]=200") && result;

        // fill to capacity
        value = 300;
        d_buffer_common_append_element(elements, &count, capacity,
                                       sizeof(int), &value);
        value = 400;
        d_buffer_common_append_element(elements, &count, capacity,
                                       sizeof(int), &value);

        // test 5: full buffer rejection
        value  = 500;
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_elem_full",
                                  d_buffer_common_append_element(elements, &count, capacity, sizeof(int), &value) == false,
                                  "Full buffer should return false") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_append_data
  Tests the d_buffer_common_append_data function.
  Tests the following:
  - NULL elements rejection
  - NULL data rejection
  - exceeding capacity rejection
  - successful append of multiple elements
  - data integrity after append
*/
bool d_tests_buffer_common_append_data(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    data[3] = {40, 50, 60};

    d_test_suite_test(_suite, "buffer_common_append_data");

    result = true;

    // test 1: NULL elements should fail
    count  = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "append_data_null_elements",
                              d_buffer_common_append_data(NULL, &count, 10, sizeof(int), data, 3) == false,
                              "NULL elements should return false") && result;

    // test 2: NULL data should fail
    {
        int buf[10] = {0};
        count = 0;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_data_null_data",
                                  d_buffer_common_append_data(buf, &count, 10, sizeof(int), NULL, 3) == false,
                                  "NULL data should return false") && result;
    }

    // initialize buffer with some data
    {
        int    source[2] = {10, 20};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 3: successful append
        result = D_TEST_CHECK_MSG(_suite,
                                  "append_data_success",
                                  d_buffer_common_append_data(elements, &count, capacity, sizeof(int), data, 3) == true,
                                  "Append data should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "append_data_count",
                                  count == 5,
                                  "Count should be 5") && result;

        // test 4: data integrity
        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 40) && (arr[3] == 50) &&
                           (arr[4] == 60);

            result = D_TEST_CHECK_MSG(_suite,
                                      "append_data_values",
                                      correct,
                                      "Elements should be [10, 20, 40, 50, 60]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_prepend_element
  Tests the d_buffer_common_prepend_element function.
  Tests the following:
  - NULL elements rejection
  - full buffer rejection
  - successful prepend shifts existing elements
*/
bool d_tests_buffer_common_prepend_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "buffer_common_prepend_element");

    result = true;

    // test 1: NULL elements should fail
    count = 0;
    value = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_elem_null_elements",
                              d_buffer_common_prepend_element(NULL, &count, 10, sizeof(int), &value) == false,
                              "NULL elements should return false") && result;

    // initialize buffer with data
    {
        int source[2] = {20, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 2: successful prepend
        value  = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_elem_success",
                                  d_buffer_common_prepend_element(elements, &count, capacity, sizeof(int), &value) == true,
                                  "Prepend should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "prepend_elem_shifted",
                                      correct && count == 3,
                                      "Elements should be [10, 20, 30]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_prepend_data
  Tests the d_buffer_common_prepend_data function.
  Tests the following:
  - NULL elements rejection
  - successful prepend of multiple elements
  - existing elements shifted correctly
*/
bool d_tests_buffer_common_prepend_data(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    prefix[2] = {1, 2};

    d_test_suite_test(_suite, "buffer_common_prepend_data");

    result = true;

    // test 1: NULL elements should fail
    count  = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "prepend_data_null_elements",
                              d_buffer_common_prepend_data(NULL, &count, 10, sizeof(int), prefix, 2) == false,
                              "NULL elements should return false") && result;

    // initialize buffer
    {
        int source[3] = {10, 20, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 3, 10);
    }

    if (elements)
    {
        // test 2: successful prepend
        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_data_success",
                                  d_buffer_common_prepend_data(elements, &count, capacity, sizeof(int), prefix, 2) == true,
                                  "Prepend data should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "prepend_data_count",
                                  count == 5,
                                  "Count should be 5") && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) &&
                           (arr[2] == 10) && (arr[3] == 20) &&
                           (arr[4] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "prepend_data_values",
                                      correct,
                                      "Elements should be [1, 2, 10, 20, 30]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_insert_element
  Tests the d_buffer_common_insert_element function.
  Tests the following:
  - NULL elements rejection
  - full buffer rejection
  - out-of-bounds index rejection
  - successful insert at beginning
  - successful insert at middle
  - successful insert at end
*/
bool d_tests_buffer_common_insert_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "buffer_common_insert_element");

    result = true;

    // test 1: NULL elements should fail
    count = 0;
    value = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_elem_null_elements",
                              d_buffer_common_insert_element(NULL, &count, 10, sizeof(int), &value, 0) == false,
                              "NULL elements should return false") && result;

    // initialize buffer
    {
        int source[2] = {10, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 2: out-of-bounds index rejection
        value  = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elem_oob",
                                  d_buffer_common_insert_element(elements, &count, capacity, sizeof(int), &value, (dj_index)100) == false,
                                  "Out-of-bounds index should return false") && result;

        // test 3: insert at middle
        value  = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elem_middle_success",
                                  d_buffer_common_insert_element(elements, &count, capacity, sizeof(int), &value, 1) == true,
                                  "Insert at middle should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "insert_elem_middle_result",
                                      correct && count == 3,
                                      "Elements should be [10, 20, 30]") && result;
        }

        // test 4: insert at end (append position)
        value  = 40;
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elem_end_success",
                                  d_buffer_common_insert_element(elements, &count, capacity, sizeof(int), &value, (dj_index)count) == true,
                                  "Insert at end should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elem_end_result",
                                  ((int*)elements)[3] == 40 && count == 4,
                                  "Last element should be 40, count=4") && result;

        // test 5: insert at beginning
        value  = 5;
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elem_begin_success",
                                  d_buffer_common_insert_element(elements, &count, capacity, sizeof(int), &value, 0) == true,
                                  "Insert at beginning should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_elem_begin_result",
                                  ((int*)elements)[0] == 5 && count == 5,
                                  "First element should be 5, count=5") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_insert_data
  Tests the d_buffer_common_insert_data function.
  Tests the following:
  - NULL elements rejection
  - out-of-bounds index rejection
  - successful range insert at middle
  - correct element ordering after insert
*/
bool d_tests_buffer_common_insert_data(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    insert_data[2] = {20, 30};

    d_test_suite_test(_suite, "buffer_common_insert_data");

    result = true;

    // test 1: NULL elements should fail
    count  = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_data_null_elements",
                              d_buffer_common_insert_data(NULL, &count, 10, sizeof(int), insert_data, 2, 0) == false,
                              "NULL elements should return false") && result;

    // initialize buffer
    {
        int source[2] = {10, 40};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 2: out-of-bounds index rejection
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_data_oob",
                                  d_buffer_common_insert_data(elements, &count, capacity, sizeof(int), insert_data, 2, (dj_index)100) == false,
                                  "Out-of-bounds index should return false") && result;

        // test 3: successful insert at middle
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_data_success",
                                  d_buffer_common_insert_data(elements, &count, capacity, sizeof(int), insert_data, 2, 1) == true,
                                  "Insert data at middle should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_data_count",
                                  count == 4,
                                  "Count should be 4") && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40);

            result = D_TEST_CHECK_MSG(_suite,
                                      "insert_data_values",
                                      correct,
                                      "Elements should be [10, 20, 30, 40]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_insert_all
  Aggregation function that runs all insertion (resize mode) tests.
*/
bool d_tests_buffer_common_insert_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_insert_all");

    result = true;

    result = d_tests_buffer_common_append_element(_suite) && result;
    result = d_tests_buffer_common_append_data(_suite) && result;
    result = d_tests_buffer_common_prepend_element(_suite) && result;
    result = d_tests_buffer_common_prepend_data(_suite) && result;
    result = d_tests_buffer_common_insert_element(_suite) && result;
    result = d_tests_buffer_common_insert_data(_suite) && result;

    return result;
}
