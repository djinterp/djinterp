/*******************************************************************************
* djinterp [container]                             vector_common_tests_element.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_element.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/**
 * @brief Part 1 of d_tests_vector_common_push_back.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL elements pointer should fail
 *   - test 2: NULL count pointer should fail
 *   - test 3: NULL value should fail
 *   - test 4: successful push to empty vector
 *   - test 5: successful push to non-empty vector
 */
static bool
d_internal_tests_vector_common_push_back_part1(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = D_TEST_CHECK_MSG(_suite,
                                "push_back_null_elements",
                                d_vector_common_push_back(NULL, &count, &capacity, sizeof(int), &value) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "push_back_null_count",
                                d_vector_common_push_back(&elements, NULL, &capacity, sizeof(int), &value) == D_FAILURE,
                                "NULL count pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: NULL value should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "push_back_null_value",
                                d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), NULL) == D_FAILURE,
                                "NULL value should return D_FAILURE") && result;

    free(elements);

    // test 4: successful push to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 5);

    value  = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_empty_success",
                              d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Push to empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_empty_count",
                              count == 1,
                              "Count should be 1 after first push") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_empty_value",
                              ((int*)elements)[0] == 100,
                              "Pushed value should be at index 0") && result;

    // test 5: successful push to non-empty vector
    value  = 200;
    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_nonempty_success",
                              d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Push to non-empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_nonempty_value",
                              count == 2 && ((int*)elements)[1] == 200,
                              "Second value should be at index 1") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_vector_common_push_back.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 6: automatic capacity growth when full
 */
static bool
d_internal_tests_vector_common_push_back_part2(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 6: automatic capacity growth when full
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 2);

    value = 1;
    d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value);
    value = 2;
    d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value);

    size_t old_capacity = capacity;
    value               = 3;
    result              = D_TEST_CHECK_MSG(_suite,
                                           "push_back_grow_success",
                                           d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                                           "Push beyond capacity should succeed (with growth)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_grow_capacity",
                              capacity > old_capacity,
                              "Capacity should have grown") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_push_back
  Tests the d_vector_common_push_back function for appending elements.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - NULL value rejection
  - successful push to empty vector
  - successful push to non-empty vector
  - automatic capacity growth when full
  - count correctly incremented
*/
bool d_tests_vector_common_push_back(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "vector_common_push_back");

    result = true;
    result &= d_internal_tests_vector_common_push_back_part1(_suite);
    result &= d_internal_tests_vector_common_push_back_part2(_suite);

    return result;
}


/*
d_tests_vector_common_push_front
  Tests the d_vector_common_push_front function for prepending elements.
  Tests the following:
  - NULL parameter handling
  - successful push to empty vector
  - successful push to non-empty vector (shifts existing)
*/
bool d_tests_vector_common_push_front(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "vector_common_push_front");

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = D_TEST_CHECK_MSG(_suite,
                                "push_front_null_elements",
                                d_vector_common_push_front(NULL, &count, &capacity, sizeof(int), &value) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: successful push to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 5);

    value  = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "push_front_empty_success",
                              d_vector_common_push_front(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Push front to empty vector should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "push_front_empty_value",
                              count == 1 && ((int*)elements)[0] == 100,
                              "Pushed value should be at index 0") && result;

    // test 3: push to non-empty vector (shifts existing)
    value  = 200;
    result = D_TEST_CHECK_MSG(_suite,
                              "push_front_nonempty_success",
                              d_vector_common_push_front(&elements, &count, &capacity, sizeof(int), &value) == D_SUCCESS,
                              "Push front to non-empty vector should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 200) && (arr[1] == 100);

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_shifted",
                                  correct,
                                  "Elements should be shifted right") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_pop_back
  Tests the d_vector_common_pop_back function for removing the last element.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - successful pop with output parameter
  - successful pop without output parameter
*/
bool d_tests_vector_common_pop_back(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    out_value;

    d_test_suite_test(_suite, "vector_common_pop_back");

    result = true;

    // test 1: NULL elements should fail
    count  = 1;
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_back_null_elements",
                              d_vector_common_pop_back(NULL, &count, sizeof(int), &out_value) == D_FAILURE,
                              "NULL elements should return D_FAILURE") && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "pop_back_empty",
                                d_vector_common_pop_back(elements, &count, sizeof(int), &out_value) == D_FAILURE,
                                "Empty vector should return D_FAILURE") && result;

    free(elements);

    // test 3: successful pop with output
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        count  = 3;
    }

    out_value = 0;
    result    = D_TEST_CHECK_MSG(_suite,
                                 "pop_back_valid_success",
                                 d_vector_common_pop_back(elements, &count, sizeof(int), &out_value) == D_SUCCESS,
                                 "Pop back should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "pop_back_valid_result",
                              out_value == 30 && count == 2,
                              "Should pop value 30 and decrement count") && result;

    // test 4: pop with NULL out_value
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_back_null_out",
                              d_vector_common_pop_back(elements, &count, sizeof(int), NULL) == D_SUCCESS,
                              "Pop with NULL out_value should succeed") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_pop_front
  Tests the d_vector_common_pop_front function for removing the first element.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - successful pop with output parameter
  - remaining elements correctly shifted
*/
bool d_tests_vector_common_pop_front(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    out_value;

    d_test_suite_test(_suite, "vector_common_pop_front");

    result = true;

    // test 1: NULL elements should fail
    count  = 1;
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_front_null_elements",
                              d_vector_common_pop_front(NULL, &count, sizeof(int), &out_value) == D_FAILURE,
                              "NULL elements should return D_FAILURE") && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "pop_front_empty",
                                d_vector_common_pop_front(elements, &count, sizeof(int), &out_value) == D_FAILURE,
                                "Empty vector should return D_FAILURE") && result;

    free(elements);

    // test 3: successful pop
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        count  = 3;
    }

    out_value = 0;
    result    = D_TEST_CHECK_MSG(_suite,
                                 "pop_front_valid_success",
                                 d_vector_common_pop_front(elements, &count, sizeof(int), &out_value) == D_SUCCESS,
                                 "Pop front should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "pop_front_valid_result",
                              out_value == 10 && count == 2,
                              "Should pop value 10 and decrement count") && result;

    // test 4: verify elements shifted
    {
        int* arr     = (int*)elements;
        bool shifted = (arr[0] == 20) && (arr[1] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_shifted",
                                  shifted,
                                  "Remaining elements should be shifted left") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_insert_element
  Tests the d_vector_common_insert_element function for inserting at specific index.
  Tests the following:
  - NULL parameter handling
  - successful insert at beginning, middle, and end
  - negative index support
*/
bool d_tests_vector_common_insert_element(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    d_test_suite_test(_suite, "vector_common_insert_element");

    result = true;

    // test 1: NULL elements should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = D_TEST_CHECK_MSG(_suite,
                                "insert_null_elements",
                                d_vector_common_insert_element(NULL, &count, &capacity, sizeof(int), 0, &value) == D_FAILURE,
                                "NULL elements should return D_FAILURE") && result;

    // test 2: successful insert at beginning
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 20;
        arr[1] = 30;
        count  = 2;
    }

    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_beginning_success",
                              d_vector_common_insert_element(&elements, &count, &capacity, sizeof(int), 0, &value) == D_SUCCESS,
                              "Insert at beginning should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_beginning_order",
                                  correct && count == 3,
                                  "Elements should be [10, 20, 30]") && result;
    }

    // test 3: insert at middle
    value  = 25;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_middle_success",
                              d_vector_common_insert_element(&elements, &count, &capacity, sizeof(int), 2, &value) == D_SUCCESS,
                              "Insert at middle should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[2] == 25) && (arr[3] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_middle_order",
                                  correct,
                                  "25 should be at index 2, 30 shifted to 3") && result;
    }

    // test 4: insert at end
    value  = 40;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_end_success",
                              d_vector_common_insert_element(&elements, &count, &capacity, sizeof(int), (dj_index)count, &value) == D_SUCCESS,
                              "Insert at end should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "insert_end_value",
                              ((int*)elements)[count - 1] == 40,
                              "40 should be at end") && result;

    // test 5: negative index
    value  = 99;
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_negative_success",
                              d_vector_common_insert_element(&elements, &count, &capacity, sizeof(int), -1, &value) == D_SUCCESS,
                              "Insert with negative index should succeed") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_insert_elements
  Tests the d_vector_common_insert_elements function for inserting multiple elements.
  Tests the following:
  - NULL parameter handling
  - zero source_count (no-op)
  - NULL source handling
  - successful range insert
*/
bool d_tests_vector_common_insert_elements(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {100, 200, 300};

    d_test_suite_test(_suite, "vector_common_insert_elements");

    result = true;

    // test 1: NULL elements should fail
    count    = 0;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "insert_range_null_elements",
                                d_vector_common_insert_elements(NULL, &count, &capacity, sizeof(int), 0, source, 3) == D_FAILURE,
                                "NULL elements should return D_FAILURE") && result;

    // test 2: zero source_count is no-op success
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = D_TEST_CHECK_MSG(_suite,
                              "insert_range_zero_count",
                              d_vector_common_insert_elements(&elements, &count, &capacity, sizeof(int), 0, source, 0) == D_SUCCESS,
                              "Zero source_count should return D_SUCCESS") && result;

    // test 3: NULL source with non-zero count should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_range_null_source",
                              d_vector_common_insert_elements(&elements, &count, &capacity, sizeof(int), 0, NULL, 3) == D_FAILURE,
                              "NULL source with non-zero count should fail") && result;

    // test 4: successful range insert
    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 40;
        count  = 2;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "insert_range_valid_success",
                              d_vector_common_insert_elements(&elements, &count, &capacity, sizeof(int), 1, source, 3) == D_SUCCESS,
                              "Insert range should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "insert_range_count",
                              count == 5,
                              "Count should be 5") && result;

    {
        int* arr = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 100) && (arr[2] == 200) &&
                       (arr[3] == 300) && (arr[4] == 40);

        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_order",
                                  correct,
                                  "Elements should be [10, 100, 200, 300, 40]") && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_erase
  Tests the d_vector_common_erase function for removing single elements.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - invalid index handling
  - successful erase at various positions
  - negative index support
*/
bool d_tests_vector_common_erase(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_erase");

    result = true;

    // test 1: NULL elements should fail
    count  = 1;
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_null_elements",
                              d_vector_common_erase(NULL, &count, sizeof(int), 0) == D_FAILURE,
                              "NULL elements should return D_FAILURE") && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "erase_empty",
                                d_vector_common_erase(elements, &count, sizeof(int), 0) == D_FAILURE,
                                "Empty vector should return D_FAILURE") && result;

    free(elements);

    // test 3: invalid index should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        count  = 3;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "erase_invalid_index",
                              d_vector_common_erase(elements, &count, sizeof(int), 10) == D_FAILURE,
                              "Invalid index should return D_FAILURE") && result;

    // test 4: successful erase at beginning
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_beginning_success",
                              d_vector_common_erase(elements, &count, sizeof(int), 0) == D_SUCCESS,
                              "Erase at beginning should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 20) && (arr[1] == 30) && (count == 2);

        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_beginning_result",
                                  correct,
                                  "Elements should be [20, 30]") && result;
    }

    // test 5: erase at end
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_end_success",
                              d_vector_common_erase(elements, &count, sizeof(int), -1) == D_SUCCESS,
                              "Erase at end (negative index) should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "erase_end_result",
                              count == 1 && ((int*)elements)[0] == 20,
                              "Only element 20 should remain") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_erase_range
  Tests the d_vector_common_erase_range function for removing element ranges.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - invalid range handling
  - successful range erase
*/
bool d_tests_vector_common_erase_range(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_erase_range");

    result = true;

    // test 1: NULL elements should fail
    count  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_null_elements",
                              d_vector_common_erase_range(NULL, &count, sizeof(int), 0, 2) == D_FAILURE,
                              "NULL elements should return D_FAILURE") && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "erase_range_empty",
                                d_vector_common_erase_range(elements, &count, sizeof(int), 0, 0) == D_FAILURE,
                                "Empty vector should return D_FAILURE") && result;

    free(elements);

    // test 3: start > end should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        size_t i;
        for (i = 0; i < 5; i++)
        {
            arr[i] = (int)((i + 1) * 10);
        }
        count = 5;  // [10, 20, 30, 40, 50]
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_invalid",
                              d_vector_common_erase_range(elements, &count, sizeof(int), 3, 1) == D_FAILURE,
                              "start > end should return D_FAILURE") && result;

    // test 4: successful range erase in middle
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_middle_success",
                              d_vector_common_erase_range(elements, &count, sizeof(int), 1, 2) == D_SUCCESS,
                              "Erase range [1,2] should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool correct = (count == 3) && (arr[0] == 10) &&
                       (arr[1] == 40) && (arr[2] == 50);

        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_range_middle_result",
                                  correct,
                                  "Elements should be [10, 40, 50]") && result;
    }

    // test 5: erase entire remaining vector
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_all_success",
                              d_vector_common_erase_range(elements, &count, sizeof(int), 0, (dj_index)(count - 1)) == D_SUCCESS,
                              "Erase all elements should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_all_count",
                              count == 0,
                              "Count should be 0") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_element_all
  Aggregation function that runs all element manipulation tests.
*/
bool d_tests_vector_common_element_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_element_all");

    result = true;

    result = d_tests_vector_common_push_back(_suite) && result;
    result = d_tests_vector_common_push_front(_suite) && result;
    result = d_tests_vector_common_pop_back(_suite) && result;
    result = d_tests_vector_common_pop_front(_suite) && result;
    result = d_tests_vector_common_insert_element(_suite) && result;
    result = d_tests_vector_common_insert_elements(_suite) && result;
    result = d_tests_vector_common_erase(_suite) && result;
    result = d_tests_vector_common_erase_range(_suite) && result;

    return result;
}
