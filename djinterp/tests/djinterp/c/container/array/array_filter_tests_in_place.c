/*******************************************************************************
* djinterp [container]                             array_filter_tests_in_place.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_in_place.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * III. IN-PLACE FILTER OPERATIONS
 *****************************************************************************/

/*
d_tests_array_filter_in_place
  Tests the d_contiguous_filter_in_place function.
  Tests the following:
  - Filters in-place by predicate (even numbers)
  - Returns correct new count
  - Surviving elements are compacted at front
  - No predicate match returns 0
  - All match returns original count
  - NULL input is handled safely
  - NULL predicate is handled safely
*/
bool d_tests_array_filter_in_place(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    size_t new_count;

    d_test_suite_test(_suite, "array_filter_in_place");

    result = true;

    // test 1: filter even from {0..9} -> compacted {0,2,4,6,8}
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place(data,
                                        D_TEST_ARRAY_FILTER_DATA_SIZE,
                                        sizeof(int),
                                        d_test_af_is_even,
                                        NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_even_count",
                              new_count == 5,
                              "in_place(is_even) on {0..9} should return 5") && result;

    // test 2: surviving elements are correct
    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_even_values",
                              ( (data[0] == 0) && (data[1] == 2) && (data[2] == 4) && (data[3] == 6) && (data[4] == 8) ),
                              "First 5 elements should be {0, 2, 4, 6, 8}") && result;

    // test 3: always_false -> 0 survivors
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place(data,
                                        D_TEST_ARRAY_FILTER_DATA_SIZE,
                                        sizeof(int),
                                        d_test_af_always_false,
                                        NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_none",
                              new_count == 0,
                              "in_place(always_false) should return 0") && result;

    // test 4: always_true -> all survive
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place(data,
                                        D_TEST_ARRAY_FILTER_DATA_SIZE,
                                        sizeof(int),
                                        d_test_af_always_true,
                                        NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_all",
                              new_count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "in_place(always_true) should return original count") && result;

    // test 5: NULL input
    new_count = d_contiguous_filter_in_place(NULL,
                                        D_TEST_ARRAY_FILTER_DATA_SIZE,
                                        sizeof(int),
                                        d_test_af_is_even,
                                        NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_null_input",
                              new_count == 0,
                              "in_place with NULL input should return 0") && result;

    // test 6: NULL predicate
    new_count = d_contiguous_filter_in_place(data,
                                        D_TEST_ARRAY_FILTER_DATA_SIZE,
                                        sizeof(int),
                                        NULL,
                                        NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_null_pred",
                              new_count == 0,
                              "in_place with NULL predicate should return 0") && result;

    return result;
}


/*
d_tests_array_filter_in_place_not
  Tests the d_contiguous_filter_in_place_not function.
  Tests the following:
  - Inverts predicate: keeps elements that do NOT match
  - Returns correct new count
  - Surviving elements compacted correctly
*/
bool d_tests_array_filter_in_place_not(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    size_t new_count;

    d_test_suite_test(_suite, "array_filter_in_place_not");

    result = true;

    // test 1: remove even (keep odd) from {0..9} -> {1,3,5,7,9}
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place_not(data,
                                            D_TEST_ARRAY_FILTER_DATA_SIZE,
                                            sizeof(int),
                                            d_test_af_is_even,
                                            NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_not_count",
                              new_count == 5,
                              "in_place_not(is_even) on {0..9} should return 5") && result;

    // test 2: values
    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_not_values",
                              ( (data[0] == 1) && (data[1] == 3) && (data[2] == 5) && (data[3] == 7) && (data[4] == 9) ),
                              "First 5 elements should be {1, 3, 5, 7, 9}") && result;

    // test 3: in_place_not(always_false) -> keep all
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place_not(data,
                                            D_TEST_ARRAY_FILTER_DATA_SIZE,
                                            sizeof(int),
                                            d_test_af_always_false,
                                            NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_not_keep_all",
                              new_count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "in_place_not(always_false) should keep all elements") && result;

    return result;
}


/*
d_tests_array_filter_in_place_take_first
  Tests the d_contiguous_filter_in_place_take_first function.
  Tests the following:
  - Truncates to first n elements
  - n=0 returns 0
  - n >= count returns original count
*/
bool d_tests_array_filter_in_place_take_first(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    size_t new_count;

    d_test_suite_test(_suite, "array_filter_in_place_take_first");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: truncate to first 4
    new_count = d_contiguous_filter_in_place_take_first(data,
                                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                   sizeof(int),
                                                   4);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_take_first_count",
                              new_count == 4,
                              "in_place_take_first(4) should return 4") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_take_first_values",
                              ( (data[0] == 0) && (data[3] == 3) ),
                              "First 4 elements should be {0, 1, 2, 3}") && result;

    // test 2: n=0
    new_count = d_contiguous_filter_in_place_take_first(data,
                                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                   sizeof(int),
                                                   0);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_take_first_zero",
                              new_count == 0,
                              "in_place_take_first(0) should return 0") && result;

    // test 3: n >= count
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place_take_first(data,
                                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                   sizeof(int),
                                                   100);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_take_first_overflow",
                              new_count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "in_place_take_first(100) should return original count") && result;

    return result;
}


/*
d_tests_array_filter_in_place_skip_first
  Tests the d_contiguous_filter_in_place_skip_first function.
  Tests the following:
  - Removes first n elements by shifting remainder left
  - n=0 returns original count
  - n >= count returns 0
*/
bool d_tests_array_filter_in_place_skip_first(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    size_t new_count;

    d_test_suite_test(_suite, "array_filter_in_place_skip_first");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: skip first 3 -> {3,4,5,6,7,8,9}
    new_count = d_contiguous_filter_in_place_skip_first(data,
                                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                   sizeof(int),
                                                   3);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_skip_first_count",
                              new_count == 7,
                              "in_place_skip_first(3) should return 7") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_skip_first_values",
                              ( (data[0] == 3) && (data[6] == 9) ),
                              "Elements should be shifted: data[0]=3, data[6]=9") && result;

    // test 2: n=0
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place_skip_first(data,
                                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                   sizeof(int),
                                                   0);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_skip_first_zero",
                              new_count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "in_place_skip_first(0) should return original count") && result;

    // test 3: n >= count
    new_count = d_contiguous_filter_in_place_skip_first(data,
                                                   D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                   sizeof(int),
                                                   100);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_skip_first_overflow",
                              new_count == 0,
                              "in_place_skip_first(100) should return 0") && result;

    return result;
}


/*
d_tests_array_filter_in_place_distinct
  Tests the d_contiguous_filter_in_place_distinct function.
  Tests the following:
  - Removes duplicates in-place
  - Returns new count of unique elements
  - Already-unique array returns original count
*/
bool d_tests_array_filter_in_place_distinct(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    size_t new_count;

    d_test_suite_test(_suite, "array_filter_in_place_distinct");

    result = true;

    // test 1: duplicates -> 7 unique
    d_test_af_fill_with_duplicates(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place_distinct(data,
                                                 D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                 sizeof(int),
                                                 d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_distinct_count",
                              new_count == 7,
                              "in_place_distinct should return 7 unique elements") && result;

    // test 2: already unique -> same count
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    new_count = d_contiguous_filter_in_place_distinct(data,
                                                 D_TEST_ARRAY_FILTER_DATA_SIZE,
                                                 sizeof(int),
                                                 d_test_af_compare_int);

    result = D_TEST_CHECK_MSG(_suite,
                              "in_place_distinct_unique",
                              new_count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                              "in_place_distinct on unique data should return original count") && result;

    return result;
}


/*
d_tests_array_filter_in_place_all
  Aggregation function that runs all in-place filter tests.
*/
bool d_tests_array_filter_in_place_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_in_place_all");

    result = true;

    result = d_tests_array_filter_in_place(_suite) && result;
    result = d_tests_array_filter_in_place_not(_suite) && result;
    result = d_tests_array_filter_in_place_take_first(_suite) && result;
    result = d_tests_array_filter_in_place_skip_first(_suite) && result;
    result = d_tests_array_filter_in_place_distinct(_suite) && result;

    return result;
}
