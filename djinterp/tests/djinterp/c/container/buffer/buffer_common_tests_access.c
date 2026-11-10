/*******************************************************************************
* djinterp [container]                              buffer_common_tests_access.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_access.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_get_element
  Tests the d_buffer_common_get_element function.
  Tests the following:
  - NULL elements returns NULL
  - zero element_size returns NULL
  - out-of-bounds index returns NULL
  - successful retrieval of first, middle, and last elements
*/
bool d_tests_buffer_common_get_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[5] = {10, 20, 30, 40, 50};
    void*  got;

    d_test_suite_test(_suite, "buffer_common_get_element");

    result = true;

    // test 1: NULL elements should return NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "get_element_null",
                              d_buffer_common_get_element(NULL, 5, sizeof(int), 0) == NULL,
                              "NULL elements should return NULL") && result;

    // test 2: zero element_size should return NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "get_element_zero_size",
                              d_buffer_common_get_element(source, 5, 0, 0) == NULL,
                              "Zero element_size should return NULL") && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 5, 0);

    if (elements)
    {
        // test 3: out-of-bounds index returns NULL
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_element_oob",
                                  d_buffer_common_get_element(elements, count, sizeof(int), 5) == NULL,
                                  "Out-of-bounds index should return NULL") && result;

        // test 4: get first element
        got = d_buffer_common_get_element(elements, count,
                                           sizeof(int), 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_element_first",
                                  got != NULL && *(int*)got == 10,
                                  "First element should be 10") && result;

        // test 5: get middle element
        got = d_buffer_common_get_element(elements, count,
                                           sizeof(int), 2);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_element_middle",
                                  got != NULL && *(int*)got == 30,
                                  "Middle element should be 30") && result;

        // test 6: get last element
        got = d_buffer_common_get_element(elements, count,
                                           sizeof(int), 4);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_element_last",
                                  got != NULL && *(int*)got == 50,
                                  "Last element should be 50") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_set_element
  Tests the d_buffer_common_set_element function.
  Tests the following:
  - NULL elements rejection
  - NULL value rejection
  - zero element_size rejection
  - out-of-bounds index rejection
  - successful set overwrites the correct element
*/
bool d_tests_buffer_common_set_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {10, 20, 30};
    int    new_val;

    d_test_suite_test(_suite, "buffer_common_set_element");

    result = true;

    // test 1: NULL elements should fail
    new_val = 99;
    result  = D_TEST_CHECK_MSG(_suite,
                               "set_element_null_elements",
                               d_buffer_common_set_element(NULL, 3, sizeof(int), 0, &new_val) == false,
                               "NULL elements should return false") && result;

    // test 2: NULL value should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "set_element_null_value",
                              d_buffer_common_set_element(source, 3, sizeof(int), 0, NULL) == false,
                              "NULL value should return false") && result;

    // test 3: zero element_size should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "set_element_zero_size",
                              d_buffer_common_set_element(source, 3, 0, 0, &new_val) == false,
                              "Zero element_size should return false") && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 3, 0);

    if (elements)
    {
        // test 4: out-of-bounds index should fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_element_oob",
                                  d_buffer_common_set_element(elements, count, sizeof(int), 3, &new_val) == false,
                                  "Out-of-bounds index should return false") && result;

        // test 5: successful set
        new_val = 999;
        result  = D_TEST_CHECK_MSG(_suite,
                                   "set_element_success",
                                   d_buffer_common_set_element(elements, count, sizeof(int), 1, &new_val) == true,
                                   "Set element should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_element_value",
                                  ((int*)elements)[1] == 999,
                                  "Element at index 1 should be 999") && result;

        // verify neighbouring elements unchanged
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_element_neighbours",
                                  ((int*)elements)[0] == 10 && ((int*)elements)[2] == 30,
                                  "Neighbouring elements should be unchanged") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_peek_front
  Tests the d_buffer_common_peek_front function.
  Tests the following:
  - NULL elements returns NULL
  - zero count returns NULL
  - zero element_size returns NULL
  - successful peek returns pointer to first element
*/
bool d_tests_buffer_common_peek_front(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {11, 22, 33};
    void*  front;

    d_test_suite_test(_suite, "buffer_common_peek_front");

    result = true;

    // test 1: NULL elements
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_front_null",
                              d_buffer_common_peek_front(NULL, 3, sizeof(int)) == NULL,
                              "NULL elements should return NULL") && result;

    // test 2: zero count
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_front_zero_count",
                              d_buffer_common_peek_front(source, 0, sizeof(int)) == NULL,
                              "Zero count should return NULL") && result;

    // test 3: zero element_size
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_front_zero_size",
                              d_buffer_common_peek_front(source, 3, 0) == NULL,
                              "Zero element_size should return NULL") && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 3, 0);

    if (elements)
    {
        // test 4: successful peek
        front = d_buffer_common_peek_front(elements, count,
                                            sizeof(int));
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_front_success",
                                  front != NULL && *(int*)front == 11,
                                  "Front element should be 11") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_peek_back
  Tests the d_buffer_common_peek_back function.
  Tests the following:
  - NULL elements returns NULL
  - zero count returns NULL
  - zero element_size returns NULL
  - successful peek returns pointer to last element
*/
bool d_tests_buffer_common_peek_back(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {11, 22, 33};
    void*  back;

    d_test_suite_test(_suite, "buffer_common_peek_back");

    result = true;

    // test 1: NULL elements
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_back_null",
                              d_buffer_common_peek_back(NULL, 3, sizeof(int)) == NULL,
                              "NULL elements should return NULL") && result;

    // test 2: zero count
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_back_zero_count",
                              d_buffer_common_peek_back(source, 0, sizeof(int)) == NULL,
                              "Zero count should return NULL") && result;

    // test 3: zero element_size
    result = D_TEST_CHECK_MSG(_suite,
                              "peek_back_zero_size",
                              d_buffer_common_peek_back(source, 3, 0) == NULL,
                              "Zero element_size should return NULL") && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 3, 0);

    if (elements)
    {
        // test 4: successful peek
        back = d_buffer_common_peek_back(elements, count,
                                          sizeof(int));
        result = D_TEST_CHECK_MSG(_suite,
                                  "peek_back_success",
                                  back != NULL && *(int*)back == 33,
                                  "Back element should be 33") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_access_all
  Aggregation function that runs all element access tests.
*/
bool d_tests_buffer_common_access_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_access_all");

    result = true;

    result = d_tests_buffer_common_get_element(_suite) && result;
    result = d_tests_buffer_common_set_element(_suite) && result;
    result = d_tests_buffer_common_peek_front(_suite) && result;
    result = d_tests_buffer_common_peek_back(_suite) && result;

    return result;
}
