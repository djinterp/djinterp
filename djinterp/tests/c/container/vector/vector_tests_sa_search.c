#include "./vector_tests_sa.h"


// helper comparator for int values
static int
int_comparator
(
    const void* _a,
    const void* _b
)
{
    int a = *(const int*)_a;
    int b = *(const int*)_b;

    return (a > b) - (a < b);
}


/*
d_tests_sa_vector_find
  Tests the d_vector_find function.
  Tests the following:
  - NULL vector returns -1
  - NULL comparator returns -1
  - value not found returns -1
  - value found returns correct index
*/
bool
d_tests_sa_vector_find
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              search_val;
    ssize_t          idx;

    result = true;

    // test 1: NULL vector returns -1
    search_val = 10;
    idx        = d_vector_find(NULL, &search_val, int_comparator);
    result     = d_assert_standalone(
        idx == -1,
        "find_null_vector",
        "NULL vector should return -1",
        _counter) && result;

    // test 2: NULL comparator returns -1
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        idx    = d_vector_find(vec, &search_val, NULL);
        result = d_assert_standalone(
            idx == -1,
            "find_null_comparator",
            "NULL comparator should return -1",
            _counter) && result;

        // test 3: value not found returns -1
        search_val = 999;
        idx        = d_vector_find(vec, &search_val, int_comparator);
        result     = d_assert_standalone(
            idx == -1,
            "find_not_found",
            "Value not found should return -1",
            _counter) && result;

        // test 4: value found returns correct index
        search_val = 20;
        idx        = d_vector_find(vec, &search_val, int_comparator);
        result     = d_assert_standalone(
            idx == 1,
            "find_found",
            "Value 20 should be at index 1",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_find_last
  Tests the d_vector_find_last function.
  Tests the following:
  - NULL vector returns -1
  - NULL comparator returns -1
  - returns last occurrence index
*/
bool
d_tests_sa_vector_find_last
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              search_val;
    ssize_t          idx;

    result = true;

    // test 1: NULL vector returns -1
    search_val = 10;
    idx        = d_vector_find_last(NULL, &search_val, int_comparator);
    result     = d_assert_standalone(
        idx == -1,
        "find_last_null_vector",
        "NULL vector should return -1",
        _counter) && result;

    // test 2: returns last occurrence
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 10, 30, 10);

    if (vec)
    {
        search_val = 10;
        idx        = d_vector_find_last(vec, &search_val, int_comparator);
        result     = d_assert_standalone(
            idx == 4,
            "find_last_found",
            "Last occurrence of 10 should be at index 4",
            _counter) && result;

        // test 3: not found
        search_val = 999;
        idx        = d_vector_find_last(vec, &search_val, int_comparator);
        result     = d_assert_standalone(
            idx == -1,
            "find_last_not_found",
            "Value not found should return -1",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_contains
  Tests the d_vector_contains function.
  Tests the following:
  - NULL vector returns false
  - value found returns true
  - value not found returns false
*/
bool
d_tests_sa_vector_contains
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              search_val;

    result = true;

    // test 1: NULL vector returns false
    search_val = 10;
    result     = d_assert_standalone(
        d_vector_contains(NULL, &search_val, int_comparator) == false,
        "contains_null_vector",
        "NULL vector should return false",
        _counter) && result;

    // test 2: value found returns true
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        search_val = 20;
        result     = d_assert_standalone(
            d_vector_contains(vec, &search_val, int_comparator) == true,
            "contains_found",
            "Value 20 should be found",
            _counter) && result;

        // test 3: value not found returns false
        search_val = 999;
        result     = d_assert_standalone(
            d_vector_contains(vec, &search_val, int_comparator) == false,
            "contains_not_found",
            "Value 999 should not be found",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_count_value
  Tests the d_vector_count_value function.
  Tests the following:
  - NULL vector returns 0
  - NULL comparator returns 0
  - correct count of occurrences
*/
bool
d_tests_sa_vector_count_value
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              search_val;
    size_t           count;

    result = true;

    // test 1: NULL vector returns 0
    search_val = 10;
    count      = d_vector_count_value(NULL, &search_val, int_comparator);
    result     = d_assert_standalone(
        count == 0,
        "count_value_null_vector",
        "NULL vector should return 0",
        _counter) && result;

    // test 2: NULL comparator returns 0
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 10, 30, 10);

    if (vec)
    {
        count  = d_vector_count_value(vec, &search_val, NULL);
        result = d_assert_standalone(
            count == 0,
            "count_value_null_comparator",
            "NULL comparator should return 0",
            _counter) && result;

        // test 3: correct count of occurrences
        search_val = 10;
        count      = d_vector_count_value(vec, &search_val, int_comparator);
        result     = d_assert_standalone(
            count == 3,
            "count_value_found",
            "Value 10 should occur 3 times",
            _counter) && result;

        // test 4: value not present
        search_val = 999;
        count      = d_vector_count_value(vec, &search_val, int_comparator);
        result     = d_assert_standalone(
            count == 0,
            "count_value_not_found",
            "Value 999 should occur 0 times",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_search_all
  Aggregation function that runs all search tests.
*/
bool
d_tests_sa_vector_search_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Search Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_vector_find(_counter) && result;
    result = d_tests_sa_vector_find_last(_counter) && result;
    result = d_tests_sa_vector_contains(_counter) && result;
    result = d_tests_sa_vector_count_value(_counter) && result;

    return result;
}
