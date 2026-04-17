/******************************************************************************
* djinterp [test]                                     ptr_vector_tests_sa_search.c
*
*   Unit tests for ptr_vector search functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_search.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_search_test_values[] = {10, 20, 30, 20, 50, 20};

static int
int_ptr_comparator(const void* _a, const void* _b)
{
    int val_a, val_b;
    if (!_a && !_b) return 0;
    if (!_a) return -1;
    if (!_b) return 1;
    val_a = *(const int*)_a;
    val_b = *(const int*)_b;
    return (val_a > val_b) - (val_a < val_b);
}


bool
d_tests_sa_ptr_vector_find
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int                  search_val;

    result = true;

    /* test 1: NULL vector should return -1 */
    search_val = 20;
    result = d_assert_standalone(
        d_ptr_vector_find(NULL, &search_val, int_ptr_comparator) == -1,
        "find_null_vector",
        "NULL vector should return -1",
        _counter) && result;

    /* test 2: find existing value */
    vec = d_ptr_vector_new_from_args(6,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2],
        &g_search_test_values[3], &g_search_test_values[4], &g_search_test_values[5]);
    if (vec)
    {
        search_val = 20;
        result = d_assert_standalone(
            d_ptr_vector_find(vec, &search_val, int_ptr_comparator) == 1,
            "find_first_occurrence",
            "Find 20 should return index 1",
            _counter) && result;

        search_val = 999;
        result = d_assert_standalone(
            d_ptr_vector_find(vec, &search_val, int_ptr_comparator) == -1,
            "find_not_found",
            "Find non-existent should return -1",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_find_last
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int                  search_val;

    result = true;

    /* test 1: NULL vector should return -1 */
    search_val = 20;
    result = d_assert_standalone(
        d_ptr_vector_find_last(NULL, &search_val, int_ptr_comparator) == -1,
        "find_last_null_vector",
        "NULL vector should return -1",
        _counter) && result;

    /* test 2: find last occurrence */
    vec = d_ptr_vector_new_from_args(6,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2],
        &g_search_test_values[3], &g_search_test_values[4], &g_search_test_values[5]);
    if (vec)
    {
        search_val = 20;
        result = d_assert_standalone(
            d_ptr_vector_find_last(vec, &search_val, int_ptr_comparator) == 5,
            "find_last_occurrence",
            "Find last 20 should return index 5",
            _counter) && result;

        search_val = 10;
        result = d_assert_standalone(
            d_ptr_vector_find_last(vec, &search_val, int_ptr_comparator) == 0,
            "find_last_single",
            "Find last 10 should return index 0",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_find_ptr
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int                  other_val;

    result = true;
    other_val = 20;

    /* test 1: NULL vector should return -1 */
    result = d_assert_standalone(
        d_ptr_vector_find_ptr(NULL, &g_search_test_values[0]) == -1,
        "find_ptr_null_vector",
        "NULL vector should return -1",
        _counter) && result;

    /* test 2: find by pointer identity */
    vec = d_ptr_vector_new_from_args(3,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_find_ptr(vec, &g_search_test_values[1]) == 1,
            "find_ptr_found",
            "Find by ptr should return correct index",
            _counter) && result;

        result = d_assert_standalone(
            d_ptr_vector_find_ptr(vec, &other_val) == -1,
            "find_ptr_not_found",
            "Different ptr with same value should return -1",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_contains
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int                  search_val;

    result = true;

    /* test 1: NULL vector should return false */
    search_val = 20;
    result = d_assert_standalone(
        d_ptr_vector_contains(NULL, &search_val, int_ptr_comparator) == false,
        "contains_null_vector",
        "NULL vector should return false",
        _counter) && result;

    /* test 2: contains tests */
    vec = d_ptr_vector_new_from_args(3,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2]);
    if (vec)
    {
        search_val = 20;
        result = d_assert_standalone(
            d_ptr_vector_contains(vec, &search_val, int_ptr_comparator) == true,
            "contains_found",
            "Contains existing should return true",
            _counter) && result;

        search_val = 999;
        result = d_assert_standalone(
            d_ptr_vector_contains(vec, &search_val, int_ptr_comparator) == false,
            "contains_not_found",
            "Contains non-existent should return false",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_contains_ptr
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int                  other_val;

    result = true;
    other_val = 10;

    /* test 1: contains by pointer identity */
    vec = d_ptr_vector_new_from_args(3,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2]);
    if (vec)
    {
        result = d_assert_standalone(
            d_ptr_vector_contains_ptr(vec, &g_search_test_values[0]) == true,
            "contains_ptr_found",
            "Contains ptr should return true",
            _counter) && result;

        result = d_assert_standalone(
            d_ptr_vector_contains_ptr(vec, &other_val) == false,
            "contains_ptr_not_found",
            "Different ptr should return false",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_count_value
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int                  search_val;

    result = true;

    /* test 1: NULL vector should return 0 */
    search_val = 20;
    result = d_assert_standalone(
        d_ptr_vector_count_value(NULL, &search_val, int_ptr_comparator) == 0,
        "count_value_null_vector",
        "NULL vector should return 0",
        _counter) && result;

    /* test 2: count occurrences */
    vec = d_ptr_vector_new_from_args(6,
        &g_search_test_values[0], &g_search_test_values[1], &g_search_test_values[2],
        &g_search_test_values[3], &g_search_test_values[4], &g_search_test_values[5]);
    if (vec)
    {
        search_val = 20;
        result = d_assert_standalone(
            d_ptr_vector_count_value(vec, &search_val, int_ptr_comparator) == 3,
            "count_value_multiple",
            "Count of 20 should be 3",
            _counter) && result;

        search_val = 10;
        result = d_assert_standalone(
            d_ptr_vector_count_value(vec, &search_val, int_ptr_comparator) == 1,
            "count_value_single",
            "Count of 10 should be 1",
            _counter) && result;

        search_val = 999;
        result = d_assert_standalone(
            d_ptr_vector_count_value(vec, &search_val, int_ptr_comparator) == 0,
            "count_value_none",
            "Count of non-existent should be 0",
            _counter) && result;

        d_ptr_vector_free(vec);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_search_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Search Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_ptr_vector_find(_counter) && result;
    result = d_tests_sa_ptr_vector_find_last(_counter) && result;
    result = d_tests_sa_ptr_vector_find_ptr(_counter) && result;
    result = d_tests_sa_ptr_vector_contains(_counter) && result;
    result = d_tests_sa_ptr_vector_contains_ptr(_counter) && result;
    result = d_tests_sa_ptr_vector_count_value(_counter) && result;

    return result;
}
