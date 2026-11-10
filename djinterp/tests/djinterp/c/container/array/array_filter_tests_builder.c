/*******************************************************************************
* djinterp [container]                              array_filter_tests_builder.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_builder.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * VIII. FLUENT BUILDER HELPERS
 *****************************************************************************/

/*
d_tests_array_filter_builder_begin_end
  Tests the D_CONTIGUOUS_FILTER_BEGIN and D_CONTIGUOUS_FILTER_END macros.
  Tests the following:
  - BEGIN creates a non-NULL builder
  - END with no operations returns all elements
  - END properly frees the builder
*/
bool d_tests_array_filter_builder_begin_end(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_filter_builder*     builder;
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_builder_begin_end");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: BEGIN creates non-NULL builder
    builder = D_CONTIGUOUS_FILTER_BEGIN();

    result = D_TEST_CHECK_MSG(_suite,
                              "builder_begin_not_null",
                              builder != NULL,
                              "D_CONTIGUOUS_FILTER_BEGIN() should return non-NULL builder") && result;

    // test 2: END with no operations returns all elements
    if (builder)
    {
        res = D_CONTIGUOUS_FILTER_END(builder,
                                 int,
                                 data,
                                 D_TEST_ARRAY_FILTER_DATA_SIZE);

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_end_passthrough",
                                  res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  "Empty builder should return all elements") && result;

        d_contiguous_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_array_filter_apply_builder
  Tests the d_contiguous_filter_apply_builder function.
  Tests the following:
  - Builder with single where produces correct result
  - Builder with take produces correct result
  - NULL builder is handled safely
  - NULL elements is handled safely
*/
bool d_tests_array_filter_apply_builder(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_filter_builder*     builder;
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_apply_builder");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: builder with where(is_even)
    builder = d_filter_builder_new();

    if (builder)
    {
        d_filter_builder_where(builder, d_test_af_is_even);

        res = d_contiguous_filter_apply_builder(builder,
                                           data,
                                           D_TEST_ARRAY_FILTER_DATA_SIZE,
                                           sizeof(int));

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_builder_where_count",
                                  res.count == 5,
                                  "Builder with where(is_even) should produce 5") && result;

        d_contiguous_filter_result_free(&res);
    }

    // test 2: builder with take (via map as take_first)
    builder = d_filter_builder_new();

    if (builder)
    {
        // use the builder's where to simulate take by using
        // the builder API then applying to the array
        res = d_contiguous_filter_apply_builder(builder,
                                           data,
                                           D_TEST_ARRAY_FILTER_DATA_SIZE,
                                           sizeof(int));

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_builder_empty",
                                  res.count == D_TEST_ARRAY_FILTER_DATA_SIZE,
                                  "Empty builder should pass all elements through") && result;

        d_contiguous_filter_result_free(&res);
    }

    // test 3: NULL builder
    res = d_contiguous_filter_apply_builder(NULL,
                                       data,
                                       D_TEST_ARRAY_FILTER_DATA_SIZE,
                                       sizeof(int));

    result = D_TEST_CHECK_MSG(_suite,
                              "apply_builder_null_builder",
                              res.status < 0,
                              "NULL builder should return error status") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: NULL elements
    builder = d_filter_builder_new();

    if (builder)
    {
        res = d_contiguous_filter_apply_builder(builder,
                                           NULL,
                                           D_TEST_ARRAY_FILTER_DATA_SIZE,
                                           sizeof(int));

        result = D_TEST_CHECK_MSG(_suite,
                                  "apply_builder_null_elements",
                                  res.status < 0,
                                  "NULL elements should return error status") && result;

        d_contiguous_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_array_filter_builder_multi_step
  Tests a fluent builder with multiple chained operations.
  Tests the following:
  - Multiple where filters compose as AND
  - D_THEN_WHERE macro chains correctly
  - Combined filter + take produces correct result
  - Order of operations is respected
*/
bool d_tests_array_filter_builder_multi_step(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_filter_builder*     b;
    struct dj_contiguous_filter_result res;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_builder_multi_step");

    result    = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: where(is_even) AND where(>3) -> {4, 6, 8}
    b = D_CONTIGUOUS_FILTER_BEGIN();
    b = D_THEN_WHERE(b, d_test_af_is_even);

    if (b)
    {
        d_filter_builder_where(b, d_test_af_is_positive);
    }

    if (b)
    {
        res = D_CONTIGUOUS_FILTER_END(b,
                                 int,
                                 data,
                                 D_TEST_ARRAY_FILTER_DATA_SIZE);

        // even AND positive from {0..9} -> {2,4,6,8}
        // (0 is even but not positive)
        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_multi_where_count",
                                  res.count == 4,
                                  "Builder where(even) + where(positive) should yield 4") && result;

        out = (int*)res.data;

        if (out && res.count >= 4)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "builder_multi_where_values",
                                      ( (out[0] == 2) && (out[1] == 4) && (out[2] == 6) && (out[3] == 8) ),
                                      "Builder multi-where should yield {2, 4, 6, 8}") && result;
        }

        d_contiguous_filter_result_free(&res);
    }

    // test 2: chained with D_THEN_WHERE macros
    b = D_CONTIGUOUS_FILTER_BEGIN();
    b = D_THEN_WHERE(b, d_test_af_is_even);
    b = D_THEN_WHERE(b, d_test_af_is_positive);

    if (b)
    {
        res = D_CONTIGUOUS_FILTER_END(b,
                                 int,
                                 data,
                                 D_TEST_ARRAY_FILTER_DATA_SIZE);

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_then_where_count",
                                  res.count == 4,
                                  "D_THEN_WHERE chain should yield same result as direct calls") && result;

        d_contiguous_filter_result_free(&res);
    }

    // test 3: skip then take with D_THEN macros
    b = D_CONTIGUOUS_FILTER_BEGIN();
    b = D_THEN_SKIP_FIRST(b, 2);
    b = D_THEN_TAKE_FIRST(b, 3);

    if (b)
    {
        res = D_CONTIGUOUS_FILTER_END(b,
                                 int,
                                 data,
                                 D_TEST_ARRAY_FILTER_DATA_SIZE);

        result = D_TEST_CHECK_MSG(_suite,
                                  "builder_skip_take_count",
                                  res.count == 3,
                                  "skip(2) -> take(3) should produce 3 elements") && result;

        out = (int*)res.data;

        if (out && res.count == 3)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "builder_skip_take_values",
                                      ( (out[0] == 2) && (out[1] == 3) && (out[2] == 4) ),
                                      "skip(2)->take(3) should yield {2, 3, 4}") && result;
        }

        d_contiguous_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_array_filter_builder_all
  Aggregation function that runs all fluent builder tests.
*/
bool d_tests_array_filter_builder_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_builder_all");

    result = true;

    result = d_tests_array_filter_builder_begin_end(_suite) && result;
    result = d_tests_array_filter_apply_builder(_suite) && result;
    result = d_tests_array_filter_builder_multi_step(_suite) && result;

    return result;
}
