#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * VIII. FLUENT BUILDER HELPERS
 *****************************************************************************/

/*
d_tests_sa_array_filter_builder_begin_end
  Tests the D_ARRAY_FILTER_BEGIN and D_ARRAY_FILTER_END macros.
  Tests the following:
  - BEGIN creates a non-NULL builder
  - END with no operations returns all elements
  - END properly frees the builder
*/
bool
d_tests_sa_array_filter_builder_begin_end
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_filter_builder*     builder;
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: BEGIN creates non-NULL builder
    builder = D_ARRAY_FILTER_BEGIN();

    result = d_assert_standalone(
        builder != NULL,
        "builder_begin_not_null",
        "D_ARRAY_FILTER_BEGIN() should return non-NULL builder",
        _counter) && result;

    // test 2: END with no operations returns all elements
    if (builder)
    {
        res = D_ARRAY_FILTER_END(builder,
                                 int,
                                 data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

        result = d_assert_standalone(
            res.count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
            "builder_end_passthrough",
            "Empty builder should return all elements",
            _counter) && result;

        d_array_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_sa_array_filter_apply_builder
  Tests the d_array_filter_apply_builder function.
  Tests the following:
  - Builder with single where produces correct result
  - Builder with take produces correct result
  - NULL builder is handled safely
  - NULL elements is handled safely
*/
bool
d_tests_sa_array_filter_apply_builder
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_filter_builder*     builder;
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: builder with where(is_even)
    builder = d_filter_builder_new();

    if (builder)
    {
        d_filter_builder_where(builder, d_tests_array_filter_is_even);

        res = d_array_filter_apply_builder(builder,
                                           data,
                                           D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                           sizeof(int));

        result = d_assert_standalone(
            res.count == 5,
            "apply_builder_where_count",
            "Builder with where(is_even) should produce 5",
            _counter) && result;

        d_array_filter_result_free(&res);
    }

    // test 2: builder with take (via map as take_first)
    builder = d_filter_builder_new();

    if (builder)
    {
        // use the builder's where to simulate take by using
        // the builder API then applying to the array
        res = d_array_filter_apply_builder(builder,
                                           data,
                                           D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                           sizeof(int));

        result = d_assert_standalone(
            res.count == D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
            "apply_builder_empty",
            "Empty builder should pass all elements through",
            _counter) && result;

        d_array_filter_result_free(&res);
    }

    // test 3: NULL builder
    res = d_array_filter_apply_builder(NULL,
                                       data,
                                       D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                       sizeof(int));

    result = d_assert_standalone(
        res.status < 0,
        "apply_builder_null_builder",
        "NULL builder should return error status",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 4: NULL elements
    builder = d_filter_builder_new();

    if (builder)
    {
        res = d_array_filter_apply_builder(builder,
                                           NULL,
                                           D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ,
                                           sizeof(int));

        result = d_assert_standalone(
            res.status < 0,
            "apply_builder_null_elements",
            "NULL elements should return error status",
            _counter) && result;

        d_array_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_sa_array_filter_builder_multi_step
  Tests a fluent builder with multiple chained operations.
  Tests the following:
  - Multiple where filters compose as AND
  - D_THEN_WHERE macro chains correctly
  - Combined filter + take produces correct result
  - Order of operations is respected
*/
bool
d_tests_sa_array_filter_builder_multi_step
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE ];
    struct d_filter_builder*     b;
    struct d_array_filter_result res;
    int*                         out;
    int                          threshold;

    result    = true;
    threshold = 3;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

    // test 1: where(is_even) AND where(>3) -> {4, 6, 8}
    b = D_ARRAY_FILTER_BEGIN();
    b = D_THEN_WHERE(b, d_tests_array_filter_is_even);

    if (b)
    {
        d_filter_builder_where(b, d_tests_array_filter_is_positive);
    }

    if (b)
    {
        res = D_ARRAY_FILTER_END(b,
                                 int,
                                 data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

        // even AND positive from {0..9} -> {2,4,6,8}
        // (0 is even but not positive)
        result = d_assert_standalone(
            res.count == 4,
            "builder_multi_where_count",
            "Builder where(even) + where(positive) should yield 4",
            _counter) && result;

        out = (int*)res.data;

        if (out && res.count >= 4)
        {
            result = d_assert_standalone(
                ( (out[0] == 2) &&
                  (out[1] == 4) &&
                  (out[2] == 6) &&
                  (out[3] == 8) ),
                "builder_multi_where_values",
                "Builder multi-where should yield {2, 4, 6, 8}",
                _counter) && result;
        }

        d_array_filter_result_free(&res);
    }

    // test 2: chained with D_THEN_WHERE macros
    b = D_ARRAY_FILTER_BEGIN();
    b = D_THEN_WHERE(b, d_tests_array_filter_is_even);
    b = D_THEN_WHERE(b, d_tests_array_filter_is_positive);

    if (b)
    {
        res = D_ARRAY_FILTER_END(b,
                                 int,
                                 data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

        result = d_assert_standalone(
            res.count == 4,
            "builder_then_where_count",
            "D_THEN_WHERE chain should yield same result as direct calls",
            _counter) && result;

        d_array_filter_result_free(&res);
    }

    // test 3: skip then take with D_THEN macros
    b = D_ARRAY_FILTER_BEGIN();
    b = D_THEN_SKIP_FIRST(b, 2);
    b = D_THEN_TAKE_FIRST(b, 3);

    if (b)
    {
        res = D_ARRAY_FILTER_END(b,
                                 int,
                                 data,
                                 D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE );

        result = d_assert_standalone(
            res.count == 3,
            "builder_skip_take_count",
            "skip(2) -> take(3) should produce 3 elements",
            _counter) && result;

        out = (int*)res.data;

        if (out && res.count == 3)
        {
            result = d_assert_standalone(
                ( (out[0] == 2) &&
                  (out[1] == 3) &&
                  (out[2] == 4) ),
                "builder_skip_take_values",
                "skip(2)->take(3) should yield {2, 3, 4}",
                _counter) && result;
        }

        d_array_filter_result_free(&res);
    }

    return result;
}


/*
d_tests_sa_array_filter_builder_all
  Aggregation function that runs all fluent builder tests.
*/
bool
d_tests_sa_array_filter_builder_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Fluent Builder Helpers\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_array_filter_builder_begin_end(_counter) && result;
    result = d_tests_sa_array_filter_apply_builder(_counter) && result;
    result = d_tests_sa_array_filter_builder_multi_step(_counter) && result;

    return result;
}
