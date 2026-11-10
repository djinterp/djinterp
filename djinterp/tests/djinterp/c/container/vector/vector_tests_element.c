/*******************************************************************************
* djinterp [container]                                    vector_tests_element.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_element.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_push_back
  Tests the d_vector_push_back function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - successful push to empty vector
  - successful push to non-empty vector
  - automatic growth when full
*/
bool d_tests_vector_push_back(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_push_back");

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_null_vector",
                              d_vector_push_back(NULL, &value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: NULL value should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_null_value",
                                  d_vector_push_back(vec, NULL) == D_FAILURE,
                                  "NULL value should return D_FAILURE") && result;

        // test 3: successful push to empty vector
        value  = 100;
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_empty_success",
                                  d_vector_push_back(vec, &value) == D_SUCCESS,
                                  "Push to empty vector should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_empty_result",
                                  vec->count == 1 && ((int*)vec->elements)[0] == 100,
                                  "Count=1, element[0]=100") && result;

        // test 4: successful push to non-empty vector
        value  = 200;
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_nonempty_success",
                                  d_vector_push_back(vec, &value) == D_SUCCESS,
                                  "Push to non-empty vector should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_nonempty_result",
                                  vec->count == 2 && ((int*)vec->elements)[1] == 200,
                                  "Count=2, element[1]=200") && result;

        d_vector_free(vec);
    }

    // test 5: automatic growth when full
    vec = d_vector_new(sizeof(int), 2);

    if (vec)
    {
        value = 1;
        d_vector_push_back(vec, &value);
        value = 2;
        d_vector_push_back(vec, &value);

        size_t old_capacity = vec->capacity;
        value               = 3;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_grow_success",
                                  d_vector_push_back(vec, &value) == D_SUCCESS,
                                  "Push beyond capacity should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_grow_result",
                                  vec->capacity > old_capacity && vec->count == 3,
                                  "Capacity should grow, count=3") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_push_front
  Tests the d_vector_push_front function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - successful push front
  - existing elements shifted
*/
bool d_tests_vector_push_front(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_push_front");

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "push_front_null_vector",
                              d_vector_push_front(NULL, &value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful push front
    vec = d_vector_new_from_args(sizeof(int), 2, 20, 30);

    if (vec)
    {
        value  = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_success",
                                  d_vector_push_front(vec, &value) == D_SUCCESS,
                                  "Push front should succeed") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "push_front_shifted",
                                      correct && vec->count == 3,
                                      "Elements should be [10, 20, 30]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_pop_back
  Tests the d_vector_pop_back function.
  Tests the following:
  - NULL vector rejection
  - empty vector rejection
  - successful pop with output
  - successful pop without output
*/
bool d_tests_vector_pop_back(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              out_value;

    d_test_suite_test(_suite, "vector_pop_back");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_back_null_vector",
                              d_vector_pop_back(NULL, &out_value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: empty vector should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_empty",
                                  d_vector_pop_back(vec, &out_value) == D_FAILURE,
                                  "Empty vector should return D_FAILURE") && result;

        d_vector_free(vec);
    }

    // test 3: successful pop with output
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        out_value = 0;
        result    = D_TEST_CHECK_MSG(_suite,
                                     "pop_back_success",
                                     d_vector_pop_back(vec, &out_value) == D_SUCCESS,
                                     "Pop back should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_result",
                                  out_value == 30 && vec->count == 2,
                                  "out_value=30, count=2") && result;

        // test 4: pop without output
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_null_out",
                                  d_vector_pop_back(vec, NULL) == D_SUCCESS,
                                  "Pop with NULL out should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_null_out_count",
                                  vec->count == 1,
                                  "count should be 1") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_pop_front
  Tests the d_vector_pop_front function.
  Tests the following:
  - NULL vector rejection
  - empty vector rejection
  - successful pop with output
  - remaining elements shifted
*/
bool d_tests_vector_pop_front(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              out_value;

    d_test_suite_test(_suite, "vector_pop_front");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_front_null_vector",
                              d_vector_pop_front(NULL, &out_value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: empty vector should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_empty",
                                  d_vector_pop_front(vec, &out_value) == D_FAILURE,
                                  "Empty vector should return D_FAILURE") && result;

        d_vector_free(vec);
    }

    // test 3: successful pop
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        out_value = 0;
        result    = D_TEST_CHECK_MSG(_suite,
                                     "pop_front_success",
                                     d_vector_pop_front(vec, &out_value) == D_SUCCESS,
                                     "Pop front should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_result",
                                  out_value == 10 && vec->count == 2,
                                  "out_value=10, count=2") && result;

        // test 4: remaining elements shifted
        {
            int* arr     = (int*)vec->elements;
            bool shifted = (arr[0] == 20) && (arr[1] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "pop_front_shifted",
                                      shifted,
                                      "Elements should be [20, 30]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_insert_element
  Tests the d_vector_insert_element function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - successful insert at various positions
  - negative index support
*/
bool d_tests_vector_insert_element(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              value;

    d_test_suite_test(_suite, "vector_insert_element");

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_null_vector",
                              d_vector_insert_element(NULL, 0, &value) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: NULL value should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_null_value",
                                  d_vector_insert_element(vec, 0, NULL) == D_FAILURE,
                                  "NULL value should return D_FAILURE") && result;

        d_vector_free(vec);
    }

    // test 3: insert at various positions
    vec = d_vector_new_from_args(sizeof(int), 2, 10, 30);

    if (vec)
    {
        // insert in middle
        value  = 20;
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_middle_success",
                                  d_vector_insert_element(vec, 1, &value) == D_SUCCESS,
                                  "Insert at middle should succeed") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = D_TEST_CHECK_MSG(_suite,
                                      "insert_middle_result",
                                      correct,
                                      "Elements should be [10, 20, 30]") && result;
        }

        // test 4: negative index
        value  = 25;
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_negative_success",
                                  d_vector_insert_element(vec, -1, &value) == D_SUCCESS,
                                  "Insert with negative index should succeed") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_insert_elements
  Tests the d_vector_insert_elements function.
  Tests the following:
  - NULL vector rejection
  - zero count no-op
  - successful range insert
*/
bool d_tests_vector_insert_elements(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    int              source[3] = {100, 200, 300};

    d_test_suite_test(_suite, "vector_insert_elements");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_range_null_vector",
                              d_vector_insert_elements(NULL, 0, source, 3) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: zero count no-op
    vec = d_vector_new_from_args(sizeof(int), 2, 10, 40);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_zero_count",
                                  d_vector_insert_elements(vec, 1, source, 0) == D_SUCCESS,
                                  "Zero count should return D_SUCCESS") && result;

        // test 3: successful range insert
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_success",
                                  d_vector_insert_elements(vec, 1, source, 3) == D_SUCCESS,
                                  "Insert range should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_count",
                                  vec->count == 5,
                                  "Count should be 5") && result;

        {
            int* arr = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 100) && (arr[2] == 200) &&
                           (arr[3] == 300) && (arr[4] == 40);

            result = D_TEST_CHECK_MSG(_suite,
                                      "insert_range_order",
                                      correct,
                                      "Elements should be [10, 100, 200, 300, 40]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_erase
  Tests the d_vector_erase function.
  Tests the following:
  - NULL vector rejection
  - invalid index rejection
  - successful erase
  - negative index support
*/
bool d_tests_vector_erase(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_erase");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_null_vector",
                              d_vector_erase(NULL, 0) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful erase
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_middle_success",
                                  d_vector_erase(vec, 1) == D_SUCCESS,
                                  "Erase at middle should succeed") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 30) && (vec->count == 2);

            result = D_TEST_CHECK_MSG(_suite,
                                      "erase_middle_result",
                                      correct,
                                      "Elements should be [10, 30]") && result;
        }

        // test 3: erase with negative index
        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_negative_success",
                                  d_vector_erase(vec, -1) == D_SUCCESS,
                                  "Erase with negative index should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_negative_result",
                                  vec->count == 1 && ((int*)vec->elements)[0] == 10,
                                  "Only element 10 should remain") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_erase_range
  Tests the d_vector_erase_range function.
  Tests the following:
  - NULL vector rejection
  - invalid range rejection
  - successful range erase
*/
bool d_tests_vector_erase_range(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_erase_range");

    result = true;

    // test 1: NULL vector should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_null_vector",
                              d_vector_erase_range(NULL, 0, 1) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    // test 2: successful range erase
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 30, 40, 50);

    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_range_success",
                                  d_vector_erase_range(vec, 1, 3) == D_SUCCESS,
                                  "Erase range [1,3] should succeed") && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (vec->count == 2) && (arr[0] == 10) && (arr[1] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "erase_range_result",
                                      correct,
                                      "Elements should be [10, 50]") && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_clear
  Tests the d_vector_clear function.
  Tests the following:
  - NULL vector handling
  - successful clear
  - capacity preserved after clear
*/
bool d_tests_vector_clear(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_clear");

    result = true;

    // test 1: NULL vector should not crash
    d_vector_clear(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_null_vector",
                              true,
                              "NULL vector should not crash") && result;

    // test 2: successful clear
    vec = d_vector_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    if (vec)
    {
        size_t old_capacity = vec->capacity;

        d_vector_clear(vec);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_count_zero",
                                  vec->count == 0,
                                  "Count should be 0 after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_capacity_preserved",
                                  vec->capacity == old_capacity,
                                  "Capacity should be preserved after clear") && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_vector_element_all
  Aggregation function that runs all element manipulation tests.
*/
bool d_tests_vector_element_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_element_all");

    result = true;

    result = d_tests_vector_push_back(_suite) && result;
    result = d_tests_vector_push_front(_suite) && result;
    result = d_tests_vector_pop_back(_suite) && result;
    result = d_tests_vector_pop_front(_suite) && result;
    result = d_tests_vector_insert_element(_suite) && result;
    result = d_tests_vector_insert_elements(_suite) && result;
    result = d_tests_vector_erase(_suite) && result;
    result = d_tests_vector_erase_range(_suite) && result;
    result = d_tests_vector_clear(_suite) && result;

    return result;
}
