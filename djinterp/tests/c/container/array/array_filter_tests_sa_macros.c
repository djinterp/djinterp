#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * VII. CONVENIENCE MACROS
 *****************************************************************************/

/*
d_tests_sa_array_filter_macro_where
  Tests the D_ARRAY_FILTER_WHERE macro.
  Tests the following:
  - Expands correctly and produces right result
  - Infers element_size from type parameter
*/
bool
d_tests_sa_array_filter_macro_where
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: macro filters even from {0..9}
    res = D_ARRAY_FILTER_WHERE(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                               d_tests_array_filter_is_even);

    result = d_assert_standalone(
        res.count == 5,
        "macro_where_count",
        "D_ARRAY_FILTER_WHERE(int, ..., is_even) should produce 5",
        _counter) && result;

    result = d_assert_standalone(
        d_array_filter_result_ok(&res) == true,
        "macro_where_ok",
        "D_ARRAY_FILTER_WHERE result should be ok",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_where_ctx
  Tests the D_ARRAY_FILTER_WHERE_CTX macro.
  Tests the following:
  - Passes context through correctly
  - Produces correct filtered count
*/
bool
d_tests_sa_array_filter_macro_where_ctx
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;
    int                          threshold;

    result    = true;
    threshold = 6;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: elements > 6 -> {7, 8, 9}
    res = D_ARRAY_FILTER_WHERE_CTX(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                   d_tests_array_filter_is_greater_than,
                                   &threshold);

    result = d_assert_standalone(
        res.count == 3,
        "macro_where_ctx_count",
        "D_ARRAY_FILTER_WHERE_CTX(>6) should produce 3",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_first_n
  Tests the D_ARRAY_FILTER_FIRST_N macro.
  Tests the following:
  - Expands to d_array_filter_take_first with correct sizeof
  - Returns correct count
*/
bool
d_tests_sa_array_filter_macro_first_n
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;
    int*                         out;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: first 4 elements
    res = D_ARRAY_FILTER_FIRST_N(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE , 4);

    result = d_assert_standalone(
        res.count == 4,
        "macro_first_n_count",
        "D_ARRAY_FILTER_FIRST_N(4) should produce 4 elements",
        _counter) && result;

    out = (int*)res.data;

    if (out && res.count == 4)
    {
        result = d_assert_standalone(
            ( (out[0] == 0) &&
              (out[3] == 3) ),
            "macro_first_n_values",
            "First 4 elements should be {0, 1, 2, 3}",
            _counter) && result;
    }

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_last_n
  Tests the D_ARRAY_FILTER_LAST_N macro.
  Tests the following:
  - Expands to d_array_filter_take_last with correct sizeof
  - Returns correct values
*/
bool
d_tests_sa_array_filter_macro_last_n
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;
    int*                         out;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: last 2 elements
    res = D_ARRAY_FILTER_LAST_N(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE , 2);

    result = d_assert_standalone(
        res.count == 2,
        "macro_last_n_count",
        "D_ARRAY_FILTER_LAST_N(2) should produce 2 elements",
        _counter) && result;

    out = (int*)res.data;

    if (out && res.count == 2)
    {
        result = d_assert_standalone(
            ( (out[0] == 8) &&
              (out[1] == 9) ),
            "macro_last_n_values",
            "Last 2 elements should be {8, 9}",
            _counter) && result;
    }

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_range
  Tests the D_ARRAY_FILTER_RANGE macro.
  Tests the following:
  - Expands correctly for half-open range
*/
bool
d_tests_sa_array_filter_macro_range
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;
    int*                         out;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: range [3, 7) -> {3, 4, 5, 6}
    res = D_ARRAY_FILTER_RANGE(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE , 3, 7);

    result = d_assert_standalone(
        res.count == 4,
        "macro_range_count",
        "D_ARRAY_FILTER_RANGE(3,7) should produce 4 elements",
        _counter) && result;

    out = (int*)res.data;

    if (out && res.count == 4)
    {
        result = d_assert_standalone(
            ( (out[0] == 3) &&
              (out[3] == 6) ),
            "macro_range_values",
            "D_ARRAY_FILTER_RANGE(3,7) should yield {3, 4, 5, 6}",
            _counter) && result;
    }

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_slice
  Tests the D_ARRAY_FILTER_SLICE macro.
  Tests the following:
  - Expands correctly for [start:end:step]
*/
bool
d_tests_sa_array_filter_macro_slice
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: slice [0:10:3] -> {0, 3, 6, 9}
    res = D_ARRAY_FILTER_SLICE(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE , 0, 10, 3);

    result = d_assert_standalone(
        res.count == 4,
        "macro_slice_count",
        "D_ARRAY_FILTER_SLICE(0,10,3) should produce 4 elements",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_distinct
  Tests the D_ARRAY_FILTER_DISTINCT macro.
  Tests the following:
  - Expands correctly and removes duplicates
*/
bool
d_tests_sa_array_filter_macro_distinct
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_with_duplicates(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: removes duplicates
    res = D_ARRAY_FILTER_DISTINCT(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                  d_tests_array_filter_compare_int);

    result = d_assert_standalone(
        res.count == 7,
        "macro_distinct_count",
        "D_ARRAY_FILTER_DISTINCT should produce 7 unique elements",
        _counter) && result;

    d_array_filter_result_free(&res);

    return result;
}


/*
d_tests_sa_array_filter_macro_in_place
  Tests the D_ARRAY_FILTER_IN_PLACE macro.
  Tests the following:
  - Expands correctly and filters in-place
  - Infers element_size and passes NULL context
*/
bool
d_tests_sa_array_filter_macro_in_place
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    size_t new_count;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: in-place filter even
    new_count = D_ARRAY_FILTER_IN_PLACE(int, data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                        d_tests_array_filter_is_even);

    result = d_assert_standalone(
        new_count == 5,
        "macro_in_place_count",
        "D_ARRAY_FILTER_IN_PLACE(is_even) should return 5",
        _counter) && result;

    result = d_assert_standalone(
        ( (data[0] == 0) &&
          (data[4] == 8) ),
        "macro_in_place_values",
        "Compacted array should start with 0 and end with 8",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_macro_all
  Aggregation function that runs all convenience macro tests.
*/
bool
d_tests_sa_array_filter_macro_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Convenience Macros\n");
    printf("  ------------------------------\n");

    result = d_tests_sa_array_filter_macro_where(_counter) && result;
    result = d_tests_sa_array_filter_macro_where_ctx(_counter) && result;
    result = d_tests_sa_array_filter_macro_first_n(_counter) && result;
    result = d_tests_sa_array_filter_macro_last_n(_counter) && result;
    result = d_tests_sa_array_filter_macro_range(_counter) && result;
    result = d_tests_sa_array_filter_macro_slice(_counter) && result;
    result = d_tests_sa_array_filter_macro_distinct(_counter) && result;
    result = d_tests_sa_array_filter_macro_in_place(_counter) && result;

    return result;
}
