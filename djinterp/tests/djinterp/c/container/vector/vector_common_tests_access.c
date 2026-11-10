/*******************************************************************************
* djinterp [container]                              vector_common_tests_access.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_access.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_at
  Tests the d_vector_common_at function for element access.
  Tests the following:
  - NULL elements handling
  - zero element_size rejection
  - zero count handling
  - valid positive index access
  - valid negative index access
  - out-of-bounds index handling
*/
bool d_tests_vector_common_at(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  elem_ptr;

    d_test_suite_test(_suite, "vector_common_at");

    result = true;

    // test 1: NULL elements should return NULL
    elem_ptr = d_vector_common_at(NULL, 5, sizeof(int), 0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_null_elements",
                                elem_ptr == NULL,
                                "NULL elements should return NULL") && result;

    // test 2: zero element_size should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_at(elements, 5, 0, 0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_zero_element_size",
                                elem_ptr == NULL,
                                "Zero element_size should return NULL") && result;

    free(elements);

    // test 3: zero count should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_at(elements, 0, sizeof(int), 0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_zero_count",
                                elem_ptr == NULL,
                                "Zero count should return NULL") && result;

    free(elements);

    // test 4: valid positive index access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 100;
        arr[1] = 200;
        arr[2] = 300;
        count  = 3;
    }

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_index_0",
                                elem_ptr != NULL && *(int*)elem_ptr == 100,
                                "Index 0 should return pointer to 100") && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 1);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_index_1",
                                elem_ptr != NULL && *(int*)elem_ptr == 200,
                                "Index 1 should return pointer to 200") && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 2);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_index_2",
                                elem_ptr != NULL && *(int*)elem_ptr == 300,
                                "Index 2 should return pointer to 300") && result;

    // test 5: valid negative index access
    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -1);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_index_neg1",
                                elem_ptr != NULL && *(int*)elem_ptr == 300,
                                "Index -1 should return pointer to last element (300)") && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -2);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_index_neg2",
                                elem_ptr != NULL && *(int*)elem_ptr == 200,
                                "Index -2 should return pointer to second-to-last (200)") && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -3);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_index_neg3",
                                elem_ptr != NULL && *(int*)elem_ptr == 100,
                                "Index -3 should return pointer to first (100)") && result;

    // test 6: out-of-bounds index handling
    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 10);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_out_of_bounds_positive",
                                elem_ptr == NULL,
                                "Out-of-bounds positive index should return NULL") && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -10);
    result   = D_TEST_CHECK_MSG(_suite,
                                "at_out_of_bounds_negative",
                                elem_ptr == NULL,
                                "Out-of-bounds negative index should return NULL") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_front
  Tests the d_vector_common_front function for accessing the first element.
  Tests the following:
  - NULL elements handling
  - zero count handling
  - valid front access
*/
bool d_tests_vector_common_front(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  elem_ptr;

    d_test_suite_test(_suite, "vector_common_front");

    result = true;

    // test 1: NULL elements should return NULL
    elem_ptr = d_vector_common_front(NULL, 5);
    result   = D_TEST_CHECK_MSG(_suite,
                                "front_null_elements",
                                elem_ptr == NULL,
                                "NULL elements should return NULL") && result;

    // test 2: zero count should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_front(elements, 0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "front_zero_count",
                                elem_ptr == NULL,
                                "Zero count should return NULL") && result;

    free(elements);

    // test 3: valid front access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 111;
        arr[1] = 222;
        arr[2] = 333;
        count  = 3;
    }

    elem_ptr = d_vector_common_front(elements, count);
    result   = D_TEST_CHECK_MSG(_suite,
                                "front_valid",
                                elem_ptr != NULL && *(int*)elem_ptr == 111,
                                "Front should return pointer to first element (111)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "front_same_as_elements",
                              elem_ptr == elements,
                              "Front pointer should be same as elements pointer") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_back
  Tests the d_vector_common_back function for accessing the last element.
  Tests the following:
  - NULL elements handling
  - zero count handling
  - zero element_size handling
  - valid back access
*/
bool d_tests_vector_common_back(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  elem_ptr;

    d_test_suite_test(_suite, "vector_common_back");

    result = true;

    // test 1: NULL elements should return NULL
    elem_ptr = d_vector_common_back(NULL, 5, sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "back_null_elements",
                                elem_ptr == NULL,
                                "NULL elements should return NULL") && result;

    // test 2: zero count should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_back(elements, 0, sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "back_zero_count",
                                elem_ptr == NULL,
                                "Zero count should return NULL") && result;

    free(elements);

    // test 3: zero element_size should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_back(elements, 5, 0);
    result   = D_TEST_CHECK_MSG(_suite,
                                "back_zero_element_size",
                                elem_ptr == NULL,
                                "Zero element_size should return NULL") && result;

    free(elements);

    // test 4: valid back access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 111;
        arr[1] = 222;
        arr[2] = 333;
        count  = 3;
    }

    elem_ptr = d_vector_common_back(elements, count, sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "back_valid",
                                elem_ptr != NULL && *(int*)elem_ptr == 333,
                                "Back should return pointer to last element (333)") && result;

    // test 5: single element vector
    count    = 1;
    elem_ptr = d_vector_common_back(elements, count, sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "back_single_element",
                                elem_ptr != NULL && *(int*)elem_ptr == 111,
                                "Back of single-element vector should return first element") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_data
  Tests the d_vector_common_data function for getting raw data pointer.
  Tests the following:
  - NULL elements handling
  - valid data access
*/
bool d_tests_vector_common_data(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  data_ptr;

    d_test_suite_test(_suite, "vector_common_data");

    result = true;

    // test 1: NULL elements should return NULL
    data_ptr = d_vector_common_data(NULL);
    result   = D_TEST_CHECK_MSG(_suite,
                                "data_null",
                                data_ptr == NULL,
                                "NULL elements should return NULL") && result;

    // test 2: valid data access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    data_ptr = d_vector_common_data(elements);
    result   = D_TEST_CHECK_MSG(_suite,
                                "data_same_as_elements",
                                data_ptr == elements,
                                "Data should return same pointer as elements") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_access_all
  Aggregation function that runs all access tests.
*/
bool d_tests_vector_common_access_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_access_all");

    result = true;

    result = d_tests_vector_common_at(_suite) && result;
    result = d_tests_vector_common_front(_suite) && result;
    result = d_tests_vector_common_back(_suite) && result;
    result = d_tests_vector_common_data(_suite) && result;

    return result;
}
