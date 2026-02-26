/******************************************************************************
* djinterp [test]                                    ptr_vector_tests_sa_element.c
*
*   Unit tests for ptr_vector element manipulation functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_element.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_elem_test_values[] = {111, 222, 333, 444, 555, 666, 777, 888};


bool
d_tests_sa_ptr_vector_push_back
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_push_back(NULL, &g_elem_test_values[0]) == D_FAILURE,
        "push_back_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: push to empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_push_back(vec, &g_elem_test_values[0]) == D_SUCCESS &&
            vec->count == 1 && vec->elements[0] == &g_elem_test_values[0],
            "push_back_empty",
            "Push to empty vector should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: push multiple elements */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_elem_test_values[0]);
        d_ptr_vector_push_back(vec, &g_elem_test_values[1]);
        d_ptr_vector_push_back(vec, &g_elem_test_values[2]);

        result = d_assert_standalone(
            vec->count == 3 &&
            vec->elements[0] == &g_elem_test_values[0] &&
            vec->elements[1] == &g_elem_test_values[1] &&
            vec->elements[2] == &g_elem_test_values[2],
            "push_back_multiple",
            "Multiple pushes should maintain order",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: push NULL pointer (should be valid) */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_push_back(vec, NULL) == D_SUCCESS &&
            vec->elements[0] == NULL,
            "push_back_null_ptr",
            "Push NULL pointer should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: automatic capacity growth */
    vec = d_ptr_vector_new(2);
    if (vec)
    {
        d_ptr_vector_push_back(vec, &g_elem_test_values[0]);
        d_ptr_vector_push_back(vec, &g_elem_test_values[1]);

        result = d_assert_standalone(
            d_ptr_vector_push_back(vec, &g_elem_test_values[2]) == D_SUCCESS &&
            vec->count == 3 && vec->capacity > 2,
            "push_back_grow",
            "Push beyond capacity should trigger growth",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_push_front
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_push_front(NULL, &g_elem_test_values[0]) == D_FAILURE,
        "push_front_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: push to empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_push_front(vec, &g_elem_test_values[0]) == D_SUCCESS &&
            vec->count == 1 && vec->elements[0] == &g_elem_test_values[0],
            "push_front_empty",
            "Push to empty vector should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: push to non-empty vector (shifts existing) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_push_front(vec, &g_elem_test_values[0]) == D_SUCCESS &&
            vec->count == 3 &&
            vec->elements[0] == &g_elem_test_values[0] &&
            vec->elements[1] == &g_elem_test_values[1] &&
            vec->elements[2] == &g_elem_test_values[2],
            "push_front_shift",
            "Push front should shift existing elements",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_pop_back
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                popped;

    result = true;

    /* test 1: NULL vector should return NULL */
    result = d_assert_standalone(
        d_ptr_vector_pop_back(NULL) == NULL,
        "pop_back_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    /* test 2: pop from empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_pop_back(vec) == NULL,
            "pop_back_empty",
            "Pop from empty vector should return NULL",
            _counter) && result;

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
        result = d_assert_standalone(
            popped == &g_elem_test_values[2] && vec->count == 2,
            "pop_back_value",
            "Pop should return last element",
            _counter) && result;

        popped = d_ptr_vector_pop_back(vec);
        result = d_assert_standalone(
            popped == &g_elem_test_values[1] && vec->count == 1,
            "pop_back_second",
            "Second pop should return second-to-last",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_pop_front
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                popped;

    result = true;

    /* test 1: NULL vector should return NULL */
    result = d_assert_standalone(
        d_ptr_vector_pop_front(NULL) == NULL,
        "pop_front_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    /* test 2: pop from empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_pop_front(vec) == NULL,
            "pop_front_empty",
            "Pop from empty vector should return NULL",
            _counter) && result;

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
        result = d_assert_standalone(
            popped == &g_elem_test_values[0] && vec->count == 2,
            "pop_front_value",
            "Pop should return first element",
            _counter) && result;

        result = d_assert_standalone(
            vec->elements[0] == &g_elem_test_values[1] &&
            vec->elements[1] == &g_elem_test_values[2],
            "pop_front_shift",
            "Remaining elements should be shifted",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_insert
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_insert(NULL, 0, &g_elem_test_values[0]) == D_FAILURE,
        "insert_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: insert at beginning */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert(vec, 0, &g_elem_test_values[0]) == D_SUCCESS &&
            vec->count == 3 &&
            vec->elements[0] == &g_elem_test_values[0],
            "insert_beginning",
            "Insert at beginning should work",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: insert in middle */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert(vec, 1, &g_elem_test_values[1]) == D_SUCCESS &&
            vec->elements[1] == &g_elem_test_values[1],
            "insert_middle",
            "Insert in middle should work",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: insert with negative index */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert(vec, -1, &g_elem_test_values[1]) == D_SUCCESS,
            "insert_negative",
            "Insert with negative index should work",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 5: out-of-bounds should fail */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert(vec, 100, &g_elem_test_values[2]) == D_FAILURE,
            "insert_oob",
            "Out-of-bounds insert should fail",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_insert_range
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                source[3];

    result = true;

    source[0] = &g_elem_test_values[2];
    source[1] = &g_elem_test_values[3];
    source[2] = &g_elem_test_values[4];

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_insert_range(NULL, 0, (const void**)source, 3) == D_FAILURE,
        "insert_range_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: NULL source with non-zero count should fail */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert_range(vec, 0, NULL, 3) == D_FAILURE,
            "insert_range_null_source",
            "NULL source with non-zero count should fail",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: zero count should succeed (no-op) */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert_range(vec, 1, (const void**)source, 0) == D_SUCCESS &&
            vec->count == 2,
            "insert_range_zero_count",
            "Zero count should be no-op",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: successful range insert */
    vec = d_ptr_vector_new_from_args(2,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[5]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_insert_range(vec, 1, (const void**)source, 3) == D_SUCCESS &&
            vec->count == 5,
            "insert_range_success",
            "Insert range should succeed",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_erase
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_erase(NULL, 0) == D_FAILURE,
        "erase_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: erase valid index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_erase(vec, 1) == D_SUCCESS &&
            vec->count == 2 &&
            vec->elements[0] == &g_elem_test_values[0] &&
            vec->elements[1] == &g_elem_test_values[2],
            "erase_middle",
            "Erase middle element should work",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: erase with negative index */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_erase(vec, -1) == D_SUCCESS &&
            vec->count == 2,
            "erase_negative",
            "Erase with negative index should work",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_erase_range
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;

    result = true;

    /* test 1: NULL vector should fail */
    result = d_assert_standalone(
        d_ptr_vector_erase_range(NULL, 0, 1) == D_FAILURE,
        "erase_range_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    /* test 2: valid range erase */
    vec = d_ptr_vector_new_from_args(5,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2],
                                     &g_elem_test_values[3],
                                     &g_elem_test_values[4]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_erase_range(vec, 1, 3) == D_SUCCESS &&
            vec->count == 2 &&
            vec->elements[0] == &g_elem_test_values[0] &&
            vec->elements[1] == &g_elem_test_values[4],
            "erase_range_success",
            "Erase range should remove middle elements",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_remove
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    void*                removed;

    result = true;

    /* test 1: NULL vector should return NULL */
    result = d_assert_standalone(
        d_ptr_vector_remove(NULL, 0) == NULL,
        "remove_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    /* test 2: empty vector should return NULL */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_remove(vec, 0) == NULL,
            "remove_empty",
            "Empty vector should return NULL",
            _counter) && result;

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
        result = d_assert_standalone(
            removed == &g_elem_test_values[1] && vec->count == 2,
            "remove_value",
            "Remove should return correct value",
            _counter) && result;

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
        result = d_assert_standalone(
            removed == &g_elem_test_values[2],
            "remove_negative",
            "Remove -1 should return last element",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_clear
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    size_t               original_capacity;

    result = true;

    /* test 1: NULL vector should not crash */
    d_ptr_vector_clear(NULL);
    result = d_assert_standalone(
        true,
        "clear_null_vector",
        "Clear NULL vector should not crash",
        _counter) && result;

    /* test 2: clear non-empty vector */
    vec = d_ptr_vector_new_from_args(3,
                                     &g_elem_test_values[0],
                                     &g_elem_test_values[1],
                                     &g_elem_test_values[2]);
    if (vec)
    {
        original_capacity = vec->capacity;
        d_ptr_vector_clear(vec);

        result = d_assert_standalone(
            vec->count == 0 && vec->capacity == original_capacity,
            "clear_result",
            "Clear should zero count but preserve capacity",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_element_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Element Manipulation Functions\n");
    printf("  -----------------------------------------\n");

    result = d_tests_sa_ptr_vector_push_back(_counter) && result;
    result = d_tests_sa_ptr_vector_push_front(_counter) && result;
    result = d_tests_sa_ptr_vector_pop_back(_counter) && result;
    result = d_tests_sa_ptr_vector_pop_front(_counter) && result;
    result = d_tests_sa_ptr_vector_insert(_counter) && result;
    result = d_tests_sa_ptr_vector_insert_range(_counter) && result;
    result = d_tests_sa_ptr_vector_erase(_counter) && result;
    result = d_tests_sa_ptr_vector_erase_range(_counter) && result;
    result = d_tests_sa_ptr_vector_remove(_counter) && result;
    result = d_tests_sa_ptr_vector_clear(_counter) && result;

    return result;
}
