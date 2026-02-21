#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * III. IN-PLACE FILTER OPERATIONS
 *****************************************************************************/

/*
d_tests_sa_array_filter_in_place
  Tests the d_array_filter_in_place function.
  Tests the following:
  - Filters in-place by predicate (even numbers)
  - Returns correct new count
  - Surviving elements are compacted at front
  - No predicate match returns 0
  - All match returns original count
  - NULL input is handled safely
  - NULL predicate is handled safely
*/
bool
d_tests_sa_array_filter_in_place
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t new_count;

    result = true;

    // test 1: filter even from {0..9} -> compacted {0,2,4,6,8}
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place(data,
                                        D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                        sizeof(int),
                                        d_tests_array_filter_is_even,
                                        NULL);

    result = d_assert_standalone(
        new_count == 5,
        "in_place_even_count",
        "in_place(is_even) on {0..9} should return 5",
        _counter) && result;

    // test 2: surviving elements are correct
    result = d_assert_standalone(
        ( (data[0] == 0) &&
          (data[1] == 2) &&
          (data[2] == 4) &&
          (data[3] == 6) &&
          (data[4] == 8) ),
        "in_place_even_values",
        "First 5 elements should be {0, 2, 4, 6, 8}",
        _counter) && result;

    // test 3: always_false -> 0 survivors
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place(data,
                                        D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                        sizeof(int),
                                        d_tests_array_filter_always_false,
                                        NULL);

    result = d_assert_standalone(
        new_count == 0,
        "in_place_none",
        "in_place(always_false) should return 0",
        _counter) && result;

    // test 4: always_true -> all survive
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place(data,
                                        D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                        sizeof(int),
                                        d_tests_array_filter_always_true,
                                        NULL);

    result = d_assert_standalone(
        new_count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
        "in_place_all",
        "in_place(always_true) should return original count",
        _counter) && result;

    // test 5: NULL input
    new_count = d_array_filter_in_place(NULL,
                                        D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                        sizeof(int),
                                        d_tests_array_filter_is_even,
                                        NULL);

    result = d_assert_standalone(
        new_count == 0,
        "in_place_null_input",
        "in_place with NULL input should return 0",
        _counter) && result;

    // test 6: NULL predicate
    new_count = d_array_filter_in_place(data,
                                        D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                        sizeof(int),
                                        NULL,
                                        NULL);

    result = d_assert_standalone(
        new_count == 0,
        "in_place_null_pred",
        "in_place with NULL predicate should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_in_place_not
  Tests the d_array_filter_in_place_not function.
  Tests the following:
  - Inverts predicate: keeps elements that do NOT match
  - Returns correct new count
  - Surviving elements compacted correctly
*/
bool
d_tests_sa_array_filter_in_place_not
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t new_count;

    result = true;

    // test 1: remove even (keep odd) from {0..9} -> {1,3,5,7,9}
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place_not(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                            sizeof(int),
                                            d_tests_array_filter_is_even,
                                            NULL);

    result = d_assert_standalone(
        new_count == 5,
        "in_place_not_count",
        "in_place_not(is_even) on {0..9} should return 5",
        _counter) && result;

    // test 2: values
    result = d_assert_standalone(
        ( (data[0] == 1) &&
          (data[1] == 3) &&
          (data[2] == 5) &&
          (data[3] == 7) &&
          (data[4] == 9) ),
        "in_place_not_values",
        "First 5 elements should be {1, 3, 5, 7, 9}",
        _counter) && result;

    // test 3: in_place_not(always_false) -> keep all
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place_not(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                            sizeof(int),
                                            d_tests_array_filter_always_false,
                                            NULL);

    result = d_assert_standalone(
        new_count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
        "in_place_not_keep_all",
        "in_place_not(always_false) should keep all elements",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_in_place_take_first
  Tests the d_array_filter_in_place_take_first function.
  Tests the following:
  - Truncates to first n elements
  - n=0 returns 0
  - n >= count returns original count
*/
bool
d_tests_sa_array_filter_in_place_take_first
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t new_count;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: truncate to first 4
    new_count = d_array_filter_in_place_take_first(data,
                                                   D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                   sizeof(int),
                                                   4);

    result = d_assert_standalone(
        new_count == 4,
        "in_place_take_first_count",
        "in_place_take_first(4) should return 4",
        _counter) && result;

    result = d_assert_standalone(
        ( (data[0] == 0) &&
          (data[3] == 3) ),
        "in_place_take_first_values",
        "First 4 elements should be {0, 1, 2, 3}",
        _counter) && result;

    // test 2: n=0
    new_count = d_array_filter_in_place_take_first(data,
                                                   D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                   sizeof(int),
                                                   0);

    result = d_assert_standalone(
        new_count == 0,
        "in_place_take_first_zero",
        "in_place_take_first(0) should return 0",
        _counter) && result;

    // test 3: n >= count
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place_take_first(data,
                                                   D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                   sizeof(int),
                                                   100);

    result = d_assert_standalone(
        new_count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
        "in_place_take_first_overflow",
        "in_place_take_first(100) should return original count",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_in_place_skip_first
  Tests the d_array_filter_in_place_skip_first function.
  Tests the following:
  - Removes first n elements by shifting remainder left
  - n=0 returns original count
  - n >= count returns 0
*/
bool
d_tests_sa_array_filter_in_place_skip_first
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t new_count;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: skip first 3 -> {3,4,5,6,7,8,9}
    new_count = d_array_filter_in_place_skip_first(data,
                                                   D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                   sizeof(int),
                                                   3);

    result = d_assert_standalone(
        new_count == 7,
        "in_place_skip_first_count",
        "in_place_skip_first(3) should return 7",
        _counter) && result;

    result = d_assert_standalone(
        ( (data[0] == 3) &&
          (data[6] == 9) ),
        "in_place_skip_first_values",
        "Elements should be shifted: data[0]=3, data[6]=9",
        _counter) && result;

    // test 2: n=0
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place_skip_first(data,
                                                   D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                   sizeof(int),
                                                   0);

    result = d_assert_standalone(
        new_count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
        "in_place_skip_first_zero",
        "in_place_skip_first(0) should return original count",
        _counter) && result;

    // test 3: n >= count
    new_count = d_array_filter_in_place_skip_first(data,
                                                   D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                   sizeof(int),
                                                   100);

    result = d_assert_standalone(
        new_count == 0,
        "in_place_skip_first_overflow",
        "in_place_skip_first(100) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_in_place_distinct
  Tests the d_array_filter_in_place_distinct function.
  Tests the following:
  - Removes duplicates in-place
  - Returns new count of unique elements
  - Already-unique array returns original count
*/
bool
d_tests_sa_array_filter_in_place_distinct
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t new_count;

    result = true;

    // test 1: duplicates -> 7 unique
    d_tests_array_filter_fill_with_duplicates(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place_distinct(data,
                                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                 sizeof(int),
                                                 d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        new_count == 7,
        "in_place_distinct_count",
        "in_place_distinct should return 7 unique elements",
        _counter) && result;

    // test 2: already unique -> same count
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    new_count = d_array_filter_in_place_distinct(data,
                                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                                 sizeof(int),
                                                 d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        new_count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
        "in_place_distinct_unique",
        "in_place_distinct on unique data should return original count",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_in_place_all
  Aggregation function that runs all in-place filter tests.
*/
bool
d_tests_sa_array_filter_in_place_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] In-Place Filter Operations\n");
    printf("  --------------------------------------\n");

    result = d_tests_sa_array_filter_in_place(_counter) && result;
    result = d_tests_sa_array_filter_in_place_not(_counter) && result;
    result = d_tests_sa_array_filter_in_place_take_first(_counter) && result;
    result = d_tests_sa_array_filter_in_place_skip_first(_counter) && result;
    result = d_tests_sa_array_filter_in_place_distinct(_counter) && result;

    return result;
}
