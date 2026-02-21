#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * V. QUERY FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_array_filter_count_where
  Tests the d_array_filter_count_where function.
  Tests the following:
  - Counts even numbers correctly
  - Counts with context-based predicate
  - always_true returns full count
  - always_false returns 0
  - Empty array returns 0
  - NULL input returns 0
*/
bool
d_tests_sa_array_filter_count_where
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t cnt;
    int    threshold;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: count even in {0..9} -> 5
    cnt = d_array_filter_count_where(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                     sizeof(int),
                                     d_tests_array_filter_is_even,
                                     NULL);

    result = d_assert_standalone(
        cnt == 5,
        "count_where_even",
        "count_where(is_even) on {0..9} should return 5",
        _counter) && result;

    // test 2: count > 5 -> 4 elements (6,7,8,9)
    threshold = 5;
    cnt = d_array_filter_count_where(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                     sizeof(int),
                                     d_tests_array_filter_is_greater_than,
                                     &threshold);

    result = d_assert_standalone(
        cnt == 4,
        "count_where_ctx",
        "count_where(>5) on {0..9} should return 4",
        _counter) && result;

    // test 3: always_true -> full count
    cnt = d_array_filter_count_where(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                     sizeof(int),
                                     d_tests_array_filter_always_true,
                                     NULL);

    result = d_assert_standalone(
        cnt == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
        "count_where_all",
        "count_where(always_true) should return full count",
        _counter) && result;

    // test 4: always_false -> 0
    cnt = d_array_filter_count_where(data,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                     sizeof(int),
                                     d_tests_array_filter_always_false,
                                     NULL);

    result = d_assert_standalone(
        cnt == 0,
        "count_where_none",
        "count_where(always_false) should return 0",
        _counter) && result;

    // test 5: empty array
    cnt = d_array_filter_count_where(data,
                                     0,
                                     sizeof(int),
                                     d_tests_array_filter_is_even,
                                     NULL);

    result = d_assert_standalone(
        cnt == 0,
        "count_where_empty",
        "count_where on empty array should return 0",
        _counter) && result;

    // test 6: NULL input
    cnt = d_array_filter_count_where(NULL,
                                     D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                     sizeof(int),
                                     d_tests_array_filter_is_even,
                                     NULL);

    result = d_assert_standalone(
        cnt == 0,
        "count_where_null",
        "count_where with NULL input should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_any_match
  Tests the d_array_filter_any_match function.
  Tests the following:
  - Returns true when at least one element matches
  - Returns false when no elements match
  - Returns false for empty array
  - Edge: single matching element
*/
bool
d_tests_sa_array_filter_any_match
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    int  threshold;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: at least one even in {0..9} -> true
    result = d_assert_standalone(
        d_array_filter_any_match(data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                 sizeof(int),
                                 d_tests_array_filter_is_even,
                                 NULL) == true,
        "any_match_even",
        "any_match(is_even) on {0..9} should be true",
        _counter) && result;

    // test 2: none > 100 -> false
    threshold = 100;

    result = d_assert_standalone(
        d_array_filter_any_match(data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                 sizeof(int),
                                 d_tests_array_filter_is_greater_than,
                                 &threshold) == false,
        "any_match_none",
        "any_match(>100) on {0..9} should be false",
        _counter) && result;

    // test 3: empty array -> false
    result = d_assert_standalone(
        d_array_filter_any_match(data,
                                 0,
                                 sizeof(int),
                                 d_tests_array_filter_is_even,
                                 NULL) == false,
        "any_match_empty",
        "any_match on empty array should be false",
        _counter) && result;

    // test 4: single matching element
    {
        int single;

        single = 4;

        result = d_assert_standalone(
            d_array_filter_any_match(&single,
                                     1,
                                     sizeof(int),
                                     d_tests_array_filter_is_even,
                                     NULL) == true,
            "any_match_single",
            "any_match(is_even) on {4} should be true",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_array_filter_all_match
  Tests the d_array_filter_all_match function.
  Tests the following:
  - Returns true when all elements match
  - Returns false when at least one doesn't match
  - Returns true for empty array (vacuous truth)
*/
bool
d_tests_sa_array_filter_all_match
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    int  all_even[4];
    int  threshold;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: not all even in {0..9} -> false
    result = d_assert_standalone(
        d_array_filter_all_match(data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                 sizeof(int),
                                 d_tests_array_filter_is_even,
                                 NULL) == false,
        "all_match_mixed",
        "all_match(is_even) on {0..9} should be false",
        _counter) && result;

    // test 2: all even in {2, 4, 6, 8} -> true
    all_even[0] = 2;
    all_even[1] = 4;
    all_even[2] = 6;
    all_even[3] = 8;

    result = d_assert_standalone(
        d_array_filter_all_match(all_even,
                                 4,
                                 sizeof(int),
                                 d_tests_array_filter_is_even,
                                 NULL) == true,
        "all_match_all_even",
        "all_match(is_even) on {2,4,6,8} should be true",
        _counter) && result;

    // test 3: all > -1 in {0..9} -> true
    threshold = -1;

    result = d_assert_standalone(
        d_array_filter_all_match(data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                 sizeof(int),
                                 d_tests_array_filter_is_greater_than,
                                 &threshold) == true,
        "all_match_ctx",
        "all_match(>-1) on {0..9} should be true",
        _counter) && result;

    // test 4: empty array -> true (vacuous truth)
    result = d_assert_standalone(
        d_array_filter_all_match(data,
                                 0,
                                 sizeof(int),
                                 d_tests_array_filter_is_even,
                                 NULL) == true,
        "all_match_empty",
        "all_match on empty array should be true (vacuous truth)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_none_match
  Tests the d_array_filter_none_match function.
  Tests the following:
  - Returns true when no elements match
  - Returns false when at least one matches
  - Returns true for empty array
*/
bool
d_tests_sa_array_filter_none_match
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    int  threshold;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: none > 100 in {0..9} -> true
    threshold = 100;

    result = d_assert_standalone(
        d_array_filter_none_match(data,
                                  D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                  sizeof(int),
                                  d_tests_array_filter_is_greater_than,
                                  &threshold) == true,
        "none_match_true",
        "none_match(>100) on {0..9} should be true",
        _counter) && result;

    // test 2: none even in {0..9} -> false (there are even numbers)
    result = d_assert_standalone(
        d_array_filter_none_match(data,
                                  D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                  sizeof(int),
                                  d_tests_array_filter_is_even,
                                  NULL) == false,
        "none_match_false",
        "none_match(is_even) on {0..9} should be false",
        _counter) && result;

    // test 3: empty array -> true
    result = d_assert_standalone(
        d_array_filter_none_match(data,
                                  0,
                                  sizeof(int),
                                  d_tests_array_filter_is_even,
                                  NULL) == true,
        "none_match_empty",
        "none_match on empty array should be true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_find_first
  Tests the d_array_filter_find_first function.
  Tests the following:
  - Finds the first matching element
  - Returns correct pointer into the source array
  - Returns NULL when no match
  - Returns NULL for empty array
  - Returns NULL for NULL input
*/
bool
d_tests_sa_array_filter_find_first
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    int  threshold;
    int* found;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: first even in {0..9} -> pointer to 0
    found = (int*)d_array_filter_find_first(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                            sizeof(int),
                                            d_tests_array_filter_is_even,
                                            NULL);

    result = d_assert_standalone(
        ( (found != NULL) &&
          (*found == 0) ),
        "find_first_even",
        "find_first(is_even) on {0..9} should find 0",
        _counter) && result;

    // test 2: first > 5 -> pointer to 6
    threshold = 5;
    found = (int*)d_array_filter_find_first(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                            sizeof(int),
                                            d_tests_array_filter_is_greater_than,
                                            &threshold);

    result = d_assert_standalone(
        ( (found != NULL) &&
          (*found == 6) ),
        "find_first_ctx",
        "find_first(>5) on {0..9} should find 6",
        _counter) && result;

    // test 3: no match -> NULL
    threshold = 100;
    found = (int*)d_array_filter_find_first(data,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                            sizeof(int),
                                            d_tests_array_filter_is_greater_than,
                                            &threshold);

    result = d_assert_standalone(
        found == NULL,
        "find_first_no_match",
        "find_first(>100) should return NULL",
        _counter) && result;

    // test 4: empty array -> NULL
    found = (int*)d_array_filter_find_first(data,
                                            0,
                                            sizeof(int),
                                            d_tests_array_filter_is_even,
                                            NULL);

    result = d_assert_standalone(
        found == NULL,
        "find_first_empty",
        "find_first on empty array should return NULL",
        _counter) && result;

    // test 5: NULL input -> NULL
    found = (int*)d_array_filter_find_first(NULL,
                                            D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                            sizeof(int),
                                            d_tests_array_filter_is_even,
                                            NULL);

    result = d_assert_standalone(
        found == NULL,
        "find_first_null",
        "find_first with NULL input should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_find_last
  Tests the d_array_filter_find_last function.
  Tests the following:
  - Finds the last matching element
  - Returns correct pointer
  - Returns NULL when no match
*/
bool
d_tests_sa_array_filter_find_last
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    int  threshold;
    int* found;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: last even in {0..9} -> pointer to 8
    found = (int*)d_array_filter_find_last(data,
                                           D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                           sizeof(int),
                                           d_tests_array_filter_is_even,
                                           NULL);

    result = d_assert_standalone(
        ( (found != NULL) &&
          (*found == 8) ),
        "find_last_even",
        "find_last(is_even) on {0..9} should find 8",
        _counter) && result;

    // test 2: last > 5 -> pointer to 9
    threshold = 5;
    found = (int*)d_array_filter_find_last(data,
                                           D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                           sizeof(int),
                                           d_tests_array_filter_is_greater_than,
                                           &threshold);

    result = d_assert_standalone(
        ( (found != NULL) &&
          (*found == 9) ),
        "find_last_ctx",
        "find_last(>5) on {0..9} should find 9",
        _counter) && result;

    // test 3: no match -> NULL
    threshold = 100;
    found = (int*)d_array_filter_find_last(data,
                                           D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                           sizeof(int),
                                           d_tests_array_filter_is_greater_than,
                                           &threshold);

    result = d_assert_standalone(
        found == NULL,
        "find_last_no_match",
        "find_last(>100) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_query_all
  Aggregation function that runs all query function tests.
*/
bool
d_tests_sa_array_filter_query_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_array_filter_count_where(_counter) && result;
    result = d_tests_sa_array_filter_any_match(_counter) && result;
    result = d_tests_sa_array_filter_all_match(_counter) && result;
    result = d_tests_sa_array_filter_none_match(_counter) && result;
    result = d_tests_sa_array_filter_find_first(_counter) && result;
    result = d_tests_sa_array_filter_find_last(_counter) && result;

    return result;
}
