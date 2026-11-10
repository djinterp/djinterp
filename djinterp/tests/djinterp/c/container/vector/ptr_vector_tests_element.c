/*******************************************************************************
* djinterp [test]                                     ptr_vector_tests_element.c
*
*   Unit tests for ptr_vector element manipulation functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_element.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_elem_test_values[] = {111, 222, 333, 444, 555, 666, 777, 888};


bool d_tests_ptr_vector_push_back(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_push_back");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "push_back_null_vector",
                              d_ptr_vector_push_back(NULL, &g_elem_test_values[0]) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: push to empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_empty",
                                  d_ptr_vector_push_back(vec, &g_elem_test_values[0]) == D_SUCCESS && vec->count == 1 && vec->elements[0] == &g_elem_test_values[0],
                                  "Push to empty vector should succeed") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: push multiple elements */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_elem_test_values[0]);
        d_ptr_vector_push_back(vec, &g_elem_test_values[1]);
        d_ptr_vector_push_back(vec, &g_elem_test_values[2]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_multiple",
                                  vec->count == 3 && vec->elements[0] == &g_elem_test_values[0] && vec->elements[1] == &g_elem_test_values[1] && vec->elements[2] == &g_elem_test_values[2],
                                  "Multiple pushes should maintain order") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: push NULL pointer (should be valid) */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_null_ptr",
                                  d_ptr_vector_push_back(vec, NULL) == D_SUCCESS && vec->elements[0] == NULL,
                                  "Push NULL pointer should succeed") && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: automatic capacity growth */
    vec = d_ptr_vector_new(2);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_elem_test_values[0]);
        d_ptr_vector_push_back(vec, &g_elem_test_values[1]);

        result = D_TEST_CHECK_MSG(_suite,
                                  "push_back_grow",
                                  d_ptr_vector_push_back(vec, &g_elem_test_values[2]) == D_SUCCESS && vec->count == 3 && vec->capacity > 2,
                                  "Push beyond capacity should trigger growth") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_push_front(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_push_front");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "push_front_null_vector",
                              d_ptr_vector_push_front(NULL, &g_elem_test_values[0]) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: push to empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_empty",
                                  d_ptr_vector_push_front(vec, &g_elem_test_values[0]) == D_SUCCESS && vec->count == 1 && vec->elements[0] == &g_elem_test_values[0],
                                  "Push to empty vector should succeed") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: push to non-empty vector (shifts existing) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "push_front_shift",
                                  d_ptr_vector_push_front(vec, &g_elem_test_values[0]) == D_SUCCESS && vec->count == 3 && vec->elements[0] == &g_elem_test_values[0] && vec->elements[1] == &g_elem_test_values[1] && vec->elements[2] == &g_elem_test_values[2],
                                  "Push front should shift existing elements") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_pop_back(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                popped;

    d_test_suite_test(_suite, "ptr_vector_pop_back");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_back_null_vector",
                              d_ptr_vector_pop_back(NULL) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: pop from empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_empty",
                                  d_ptr_vector_pop_back(vec) == NULL,
                                  "Pop from empty vector should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: pop from non-empty vector */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        popped = d_ptr_vector_pop_back(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_value",
                                  popped == &g_elem_test_values[2] && vec->count == 2,
                                  "Pop should return last element") && result;

        popped = d_ptr_vector_pop_back(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_back_second",
                                  popped == &g_elem_test_values[1] && vec->count == 1,
                                  "Second pop should return second-to-last") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_pop_front(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                popped;

    d_test_suite_test(_suite, "ptr_vector_pop_front");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "pop_front_null_vector",
                              d_ptr_vector_pop_front(NULL) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: pop from empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_empty",
                                  d_ptr_vector_pop_front(vec) == NULL,
                                  "Pop from empty vector should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: pop from non-empty vector */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        popped = d_ptr_vector_pop_front(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_value",
                                  popped == &g_elem_test_values[0] && vec->count == 2,
                                  "Pop should return first element") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "pop_front_shift",
                                  vec->elements[0] == &g_elem_test_values[1] && vec->elements[1] == &g_elem_test_values[2],
                                  "Remaining elements should be shifted") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_insert(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_insert");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_null_vector",
                              d_ptr_vector_insert(NULL, 0, &g_elem_test_values[0]) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: insert at beginning */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_beginning",
                                  d_ptr_vector_insert(vec, 0, &g_elem_test_values[0]) == D_SUCCESS && vec->count == 3 && vec->elements[0] == &g_elem_test_values[0],
                                  "Insert at beginning should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: insert in middle */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_middle",
                                  d_ptr_vector_insert(vec, 1, &g_elem_test_values[1]) == D_SUCCESS && vec->elements[1] == &g_elem_test_values[1],
                                  "Insert in middle should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: insert with negative index */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_negative",
                                  d_ptr_vector_insert(vec, -1, &g_elem_test_values[1]) == D_SUCCESS,
                                  "Insert with negative index should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: out-of-bounds should fail */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_oob",
                                  d_ptr_vector_insert(vec, 100, &g_elem_test_values[2]) == D_FAILURE,
                                  "Out-of-bounds insert should fail") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_insert_range(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                source[3];

    d_test_suite_test(_suite, "ptr_vector_insert_range");

    result = true;

    source[0] = &g_elem_test_values[2];
    source[1] = &g_elem_test_values[3];
    source[2] = &g_elem_test_values[4];

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "insert_range_null_vector",
                              d_ptr_vector_insert_range(NULL, 0, (const void**)source, 3) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: NULL source with non-zero count should fail */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_null_source",
                                  d_ptr_vector_insert_range(vec, 0, NULL, 3) == D_FAILURE,
                                  "NULL source with non-zero count should fail") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: zero count should succeed (no-op) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_zero_count",
                                  d_ptr_vector_insert_range(vec, 1, (const void**)source, 0) == D_SUCCESS && vec->count == 2,
                                  "Zero count should be no-op") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: successful range insert */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[5]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "insert_range_success",
                                  d_ptr_vector_insert_range(vec, 1, (const void**)source, 3) == D_SUCCESS && vec->count == 5,
                                  "Insert range should succeed") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_erase(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_erase");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_null_vector",
                              d_ptr_vector_erase(NULL, 0) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: erase valid index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_middle",
                                  d_ptr_vector_erase(vec, 1) == D_SUCCESS && vec->count == 2 && vec->elements[0] == &g_elem_test_values[0] && vec->elements[1] == &g_elem_test_values[2],
                                  "Erase middle element should work") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: erase with negative index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_negative",
                                  d_ptr_vector_erase(vec, -1) == D_SUCCESS && vec->count == 2,
                                  "Erase with negative index should work") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_erase_range(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_erase_range");

    result = true;

    /* test 1: NULL vector should fail */
    result = D_TEST_CHECK_MSG(_suite,
                              "erase_range_null_vector",
                              d_ptr_vector_erase_range(NULL, 0, 1) == D_FAILURE,
                              "NULL vector should return D_FAILURE") && result;

    /* test 2: valid range erase */
    vec = d_ptr_vector_new_from_args(5,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2],
                                     &g_elem_test_values[3],
                                     &g_elem_test_values[4]);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "erase_range_success",
                                  d_ptr_vector_erase_range(vec, 1, 3) == D_SUCCESS && vec->count == 2 && vec->elements[0] == &g_elem_test_values[0] && vec->elements[1] == &g_elem_test_values[4],
                                  "Erase range should remove middle elements") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_remove(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    void*                removed;

    d_test_suite_test(_suite, "ptr_vector_remove");

    result = true;

    /* test 1: NULL vector should return NULL */
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_null_vector",
                              d_ptr_vector_remove(NULL, 0) == NULL,
                              "NULL vector should return NULL") && result;

    /* test 2: empty vector should return NULL */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_empty",
                                  d_ptr_vector_remove(vec, 0) == NULL,
                                  "Empty vector should return NULL") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: valid removal */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        removed = d_ptr_vector_remove(vec, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_value",
                                  removed == &g_elem_test_values[1] && vec->count == 2,
                                  "Remove should return correct value") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: remove with negative index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        removed = d_ptr_vector_remove(vec, -1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_negative",
                                  removed == &g_elem_test_values[2],
                                  "Remove -1 should return last element") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_clear(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    size_t               original_capacity;

    d_test_suite_test(_suite, "ptr_vector_clear");

    result = true;

    /* test 1: NULL vector should not crash */
    d_ptr_vector_clear(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_null_vector",
                              true,
                              "Clear NULL vector should not crash") && result;

    /* test 2: clear non-empty vector */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        original_capacity = vec->capacity;
        d_ptr_vector_clear(vec);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_result",
                                  vec->count == 0 && vec->capacity == original_capacity,
                                  "Clear should zero count but preserve capacity") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_element_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_element_all");

    result = true;

    result = d_tests_ptr_vector_push_back(_suite) && result;
    result = d_tests_ptr_vector_push_front(_suite) && result;
    result = d_tests_ptr_vector_pop_back(_suite) && result;
    result = d_tests_ptr_vector_pop_front(_suite) && result;
    result = d_tests_ptr_vector_insert(_suite) && result;
    result = d_tests_ptr_vector_insert_range(_suite) && result;
    result = d_tests_ptr_vector_erase(_suite) && result;
    result = d_tests_ptr_vector_erase_range(_suite) && result;
    result = d_tests_ptr_vector_remove(_suite) && result;
    result = d_tests_ptr_vector_clear(_suite) && result;

    return result;
}
