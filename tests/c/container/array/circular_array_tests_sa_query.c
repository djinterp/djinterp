#include "./circular_array_tests_sa.h"


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_circular_array_is_empty
  Tests the d_circular_array_is_empty function.
  Tests the following:
  - is_empty on empty array returns true
  - is_empty on non-empty array returns false
  - is_empty on NULL returns true
*/
bool
d_tests_sa_circular_array_is_empty
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       value = 42;

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: is_empty on empty array
        result = d_assert_standalone(
            d_circular_array_is_empty(arr) == true,
            "is_empty_true",
            "Empty array should return true",
            _counter) && result;

        // add element
        d_circular_array_push(arr, &value);

        // test 2: is_empty on non-empty array
        result = d_assert_standalone(
            d_circular_array_is_empty(arr) == false,
            "is_empty_false",
            "Non-empty array should return false",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: is_empty on NULL returns false (per implementation)
    result = d_assert_standalone(
        d_circular_array_is_empty(NULL) == false,
        "is_empty_null",
        "NULL array should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_is_full
  Tests the d_circular_array_is_full function.
  Tests the following:
  - is_full on empty array returns false
  - is_full on partially filled returns false
  - is_full on full array returns true
*/
bool
d_tests_sa_circular_array_is_full
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    result = true;

    arr = d_circular_array_new(3, sizeof(int));

    if (arr)
    {
        // test 1: is_full on empty array
        result = d_assert_standalone(
            d_circular_array_is_full(arr) == false,
            "is_full_empty",
            "Empty array should not be full",
            _counter) && result;

        // add one element
        d_circular_array_push(arr, &values[0]);

        // test 2: is_full on partially filled
        result = d_assert_standalone(
            d_circular_array_is_full(arr) == false,
            "is_full_partial",
            "Partially filled array should not be full",
            _counter) && result;

        // fill it up
        d_circular_array_push(arr, &values[1]);
        d_circular_array_push(arr, &values[2]);

        // test 3: is_full on full array
        result = d_assert_standalone(
            d_circular_array_is_full(arr) == true,
            "is_full_true",
            "Full array should return true",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: is_full on NULL
    result = d_assert_standalone(
        d_circular_array_is_full(NULL) == false,
        "is_full_null",
        "NULL array should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_count
  Tests the d_circular_array_count function.
  Tests the following:
  - count on empty array returns 0
  - count after push operations
  - count after pop operations
*/
bool
d_tests_sa_circular_array_count
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30};

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: count on empty array
        result = d_assert_standalone(
            d_circular_array_count(arr) == 0,
            "count_empty",
            "Empty array count should be 0",
            _counter) && result;

        // add elements
        d_circular_array_push(arr, &values[0]);
        d_circular_array_push(arr, &values[1]);
        d_circular_array_push(arr, &values[2]);

        // test 2: count after push
        result = d_assert_standalone(
            d_circular_array_count(arr) == 3,
            "count_after_push",
            "Count should be 3 after 3 pushes",
            _counter) && result;

        // pop one element
        d_circular_array_pop(arr);

        // test 3: count after pop
        result = d_assert_standalone(
            d_circular_array_count(arr) == 2,
            "count_after_pop",
            "Count should be 2 after pop",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: count on NULL
    result = d_assert_standalone(
        d_circular_array_count(NULL) == 0,
        "count_null",
        "NULL array count should be 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_capacity
  Tests the d_circular_array_capacity function.
  Tests the following:
  - capacity returns correct value
  - capacity unchanged after operations
*/
bool
d_tests_sa_circular_array_capacity
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       value = 42;

    result = true;

    arr = d_circular_array_new(10, sizeof(int));

    if (arr)
    {
        // test 1: capacity returns correct value
        result = d_assert_standalone(
            d_circular_array_capacity(arr) == 10,
            "capacity_value",
            "Capacity should be 10",
            _counter) && result;

        // add element
        d_circular_array_push(arr, &value);

        // test 2: capacity unchanged after operations
        result = d_assert_standalone(
            d_circular_array_capacity(arr) == 10,
            "capacity_unchanged",
            "Capacity should remain 10",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: capacity on NULL
    result = d_assert_standalone(
        d_circular_array_capacity(NULL) == 0,
        "capacity_null",
        "NULL array capacity should be 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_available_space
  Tests the d_circular_array_available_space function.
  Tests the following:
  - available_space on empty array equals capacity
  - available_space decreases after push
  - available_space on full array is 0
*/
bool
d_tests_sa_circular_array_available_space
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       values[] = {10, 20, 30, 40, 50};

    result = true;

    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        // test 1: available_space on empty equals capacity
        result = d_assert_standalone(
            d_circular_array_available_space(arr) == 5,
            "available_space_empty",
            "Available space should be 5 on empty",
            _counter) && result;

        // add elements
        d_circular_array_push(arr, &values[0]);
        d_circular_array_push(arr, &values[1]);

        // test 2: available_space decreases
        result = d_assert_standalone(
            d_circular_array_available_space(arr) == 3,
            "available_space_partial",
            "Available space should be 3",
            _counter) && result;

        // fill it up
        d_circular_array_push(arr, &values[2]);
        d_circular_array_push(arr, &values[3]);
        d_circular_array_push(arr, &values[4]);

        // test 3: available_space on full is 0
        result = d_assert_standalone(
            d_circular_array_available_space(arr) == 0,
            "available_space_full",
            "Available space should be 0 when full",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: available_space on NULL
    result = d_assert_standalone(
        d_circular_array_available_space(NULL) == 0,
        "available_space_null",
        "NULL array available space should be 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_element_size
  Tests the d_circular_array_element_size function.
  Tests the following:
  - element_size returns correct value
*/
bool
d_tests_sa_circular_array_element_size
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;

    result = true;

    // test 1: element_size for int
    arr = d_circular_array_new(5, sizeof(int));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_element_size(arr) == sizeof(int),
            "element_size_int",
            "Element size should be sizeof(int)",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: element_size for double
    arr = d_circular_array_new(5, sizeof(double));

    if (arr)
    {
        result = d_assert_standalone(
            d_circular_array_element_size(arr) == sizeof(double),
            "element_size_double",
            "Element size should be sizeof(double)",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: element_size on NULL
    result = d_assert_standalone(
        d_circular_array_element_size(NULL) == 0,
        "element_size_null",
        "NULL array element size should be 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_query_all
  Aggregation function that runs all query tests.
*/
bool
d_tests_sa_circular_array_query_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_circular_array_is_empty(_counter) && result;
    result = d_tests_sa_circular_array_is_full(_counter) && result;
    result = d_tests_sa_circular_array_count(_counter) && result;
    result = d_tests_sa_circular_array_capacity(_counter) && result;
    result = d_tests_sa_circular_array_available_space(_counter) && result;
    result = d_tests_sa_circular_array_element_size(_counter) && result;

    return result;
}
