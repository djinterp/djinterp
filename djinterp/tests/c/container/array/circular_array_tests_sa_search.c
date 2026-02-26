#include "./circular_array_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

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


/******************************************************************************
 * VI. SEARCH FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_circular_array_contains
  Tests the d_circular_array_contains function.
  Tests the following:
  - contains returns true for existing value
  - contains returns false for non-existing value
  - contains on empty array returns false
*/
bool
d_tests_sa_circular_array_contains
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};
    int                       search_value;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: contains on empty array
        search_value = 10;
        result = d_assert_standalone(
            d_circular_array_contains(arr, &search_value, int_comparator) == false,
            "contains_empty",
            "Contains on empty should return false",
            _counter) && result;

        // add elements
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 2: contains returns true for existing value
        search_value = 30;
        result = d_assert_standalone(
            d_circular_array_contains(arr, &search_value, int_comparator) == true,
            "contains_found",
            "Contains should find existing value",
            _counter) && result;

        // test 3: contains returns false for non-existing
        search_value = 99;
        result = d_assert_standalone(
            d_circular_array_contains(arr, &search_value, int_comparator) == false,
            "contains_not_found",
            "Contains should not find non-existing value",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    search_value = 10;
    result = d_assert_standalone(
        d_circular_array_contains(NULL, &search_value, int_comparator) == false,
        "contains_null",
        "Contains on NULL should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_find
  Tests the d_circular_array_find function.
  Tests the following:
  - find returns correct index for existing value
  - find returns -1 for non-existing value
  - find returns first occurrence
*/
bool
d_tests_sa_circular_array_find
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 20, 50};
    int                       search_value;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements (note: 20 appears twice)
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: find returns correct index
        search_value = 30;
        result = d_assert_standalone(
            d_circular_array_find(arr, &search_value, int_comparator) == 2,
            "find_index",
            "Find should return index 2 for value 30",
            _counter) && result;

        // test 2: find returns first occurrence
        search_value = 20;
        result = d_assert_standalone(
            d_circular_array_find(arr, &search_value, int_comparator) == 1,
            "find_first_occurrence",
            "Find should return first occurrence (index 1)",
            _counter) && result;

        // test 3: find returns -1 for non-existing
        search_value = 99;
        result = d_assert_standalone(
            d_circular_array_find(arr, &search_value, int_comparator) == -1,
            "find_not_found",
            "Find should return -1 for non-existing",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: find on empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        search_value = 10;
        result = d_assert_standalone(
            d_circular_array_find(arr, &search_value, int_comparator) == -1,
            "find_empty",
            "Find on empty should return -1",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    search_value = 10;
    result = d_assert_standalone(
        d_circular_array_find(NULL, &search_value, int_comparator) == -1,
        "find_null",
        "Find on NULL should return -1",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_find_last
  Tests the d_circular_array_find_last function.
  Tests the following:
  - find_last returns last occurrence index
  - find_last returns -1 for non-existing value
*/
bool
d_tests_sa_circular_array_find_last
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 20, 50};
    int                       search_value;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements (note: 20 appears at index 1 and 3)
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: find_last returns last occurrence
        search_value = 20;
        result = d_assert_standalone(
            d_circular_array_find_last(arr, &search_value, int_comparator) == 3,
            "find_last_occurrence",
            "Find last should return last occurrence (index 3)",
            _counter) && result;

        // test 2: find_last returns -1 for non-existing
        search_value = 99;
        result = d_assert_standalone(
            d_circular_array_find_last(arr, &search_value, int_comparator) == -1,
            "find_last_not_found",
            "Find last should return -1 for non-existing",
            _counter) && result;

        // test 3: find_last with single occurrence
        search_value = 10;
        result = d_assert_standalone(
            d_circular_array_find_last(arr, &search_value, int_comparator) == 0,
            "find_last_single",
            "Find last with single occurrence should return that index",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL array
    search_value = 10;
    result = d_assert_standalone(
        d_circular_array_find_last(NULL, &search_value, int_comparator) == -1,
        "find_last_null",
        "Find last on NULL should return -1",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_count_value
  Tests the d_circular_array_count_value function.
  Tests the following:
  - count_value returns correct count for duplicates
  - count_value returns 0 for non-existing
  - count_value on empty array returns 0
*/
bool
d_tests_sa_circular_array_count_value
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 20, 20};
    int                       search_value;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // add elements (20 appears 3 times)
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(arr, &values[i]);
        }

        // test 1: count_value with duplicates
        search_value = 20;
        result = d_assert_standalone(
            d_circular_array_count_value(arr, &search_value, int_comparator) == 3,
            "count_value_duplicates",
            "Count value should return 3 for value 20",
            _counter) && result;

        // test 2: count_value with single occurrence
        search_value = 10;
        result = d_assert_standalone(
            d_circular_array_count_value(arr, &search_value, int_comparator) == 1,
            "count_value_single",
            "Count value should return 1 for value 10",
            _counter) && result;

        // test 3: count_value for non-existing
        search_value = 99;
        result = d_assert_standalone(
            d_circular_array_count_value(arr, &search_value, int_comparator) == 0,
            "count_value_not_found",
            "Count value should return 0 for non-existing",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: count_value on empty array
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        search_value = 10;
        result = d_assert_standalone(
            d_circular_array_count_value(arr, &search_value, int_comparator) == 0,
            "count_value_empty",
            "Count value on empty should return 0",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 5: NULL array
    search_value = 10;
    result = d_assert_standalone(
        d_circular_array_count_value(NULL, &search_value, int_comparator) == 0,
        "count_value_null",
        "Count value on NULL should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_search_all
  Aggregation function that runs all search tests.
*/
bool
d_tests_sa_circular_array_search_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Search Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_circular_array_contains(_counter) && result;
    result = d_tests_sa_circular_array_find(_counter) && result;
    result = d_tests_sa_circular_array_find_last(_counter) && result;
    result = d_tests_sa_circular_array_count_value(_counter) && result;

    return result;
}
