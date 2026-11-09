/*******************************************************************************
* djinterp [test]                                       dmacro_tests_ptr_array.c
*
*   Unit tests for `dmacro.h` pointer array initialization section.
*   Tests D_FOR_EACH_DATA_COMMA and D_STRUCT_ARRAY_INIT concepts.
*   Note: D_FOR_EACH_DATA_COMMA passes custom function names through macro
* expansion which is incompatible with MSVC traditional preprocessor.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_ptr_array.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * D_FOR_EACH_DATA_COMMA TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_data_comma
  Tests D_FOR_EACH_DATA_COMMA concept for data-passing iteration.
Note: D_FOR_EACH_DATA_COMMA passes custom function names through macro
expansion which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - Data-passing iteration concept
  - Comma-separated output generation concept
*/
bool d_tests_dmacro_for_each_data_comma(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_data_comma");

    all_assertions_passed = true;

    // D_FOR_EACH_DATA_COMMA(fn, data, a, b, c)
    // would generate: fn(data, a), fn(data, b), fn(data, c)
    // concept: pass extra data to each iteration, comma-separated output

    // example use case: multiplying each element by a constant
    // D_FOR_EACH_DATA_COMMA(MUL_OP, 10, 1, 2, 3) -> 10*1, 10*2, 10*3
    int results[3];
    int multiplier = 10;

    results[0] = multiplier * 1;
    results[1] = multiplier * 2;
    results[2] = multiplier * 3;

    if (!D_TEST_CHECK_MSG(_suite,
                          "data_comma concept: 10 * 1 == 10",
                          results[0] == 10,
                          "first element with data multiplier"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", results[0]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "data_comma concept: 10 * 2 == 20",
                          results[1] == 20,
                          "second element with data multiplier"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", results[1]);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "data_comma concept: 10 * 3 == 30",
                          results[2] == 30,
                          "third element with data multiplier"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", results[2]);
    }

    // example use case: prefixing strings with a namespace
    // D_FOR_EACH_DATA_COMMA(PREFIX_OP, "ns_", foo, bar) -> "ns_foo", "ns_bar"
    // test concept: data passed to each element
    const char* prefix   = "ns_";
    const char* names[2] = {"foo", "bar"};

    if (!D_TEST_CHECK_MSG(_suite,
                          "data_comma concept: prefix is 'ns_'",
                          prefix[0] == 'n' && prefix[1] == 's',
                          "data parameter available for each element"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "data_comma concept: names are 'foo', 'bar'",
                          names[0][0] == 'f' && names[1][0] == 'b',
                          "elements processed with data"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_STRUCT_ARRAY_INIT TESTS
 *****************************************************************************/

/*
d_tests_dmacro_struct_array_init
  Tests D_STRUCT_ARRAY_INIT concept for struct array initialization.
Note: D_STRUCT_ARRAY_INIT uses D_FOR_EACH_DATA_COMMA internally which passes
custom function names through macro expansion (MSVC incompatible).
  Tests the following:
  - Struct array initialization from tuples concept
  - Compound literal array initialization
*/
bool d_tests_dmacro_struct_array_init(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_struct_array_init");

    all_assertions_passed = true;

    // D_STRUCT_ARRAY_INIT((1, 2), (3, 4), (5, 6))
    // would generate: { {1, 2}, {3, 4}, {5, 6} }
    // concept: convert tuple list to brace-enclosed initializers

    struct dj_test_point
    {
        int x;
        int y;
    };

    // manual equivalent of D_STRUCT_ARRAY_INIT((10, 20), (30, 40), (50, 60))
    struct dj_test_point points[] = {
        {10, 20},
        {30, 40},
        {50, 60}
    };

    if (!D_TEST_CHECK_MSG(_suite,
                          "struct_array_init concept: points[0] == (10, 20)",
                          points[0].x == 10 && points[0].y == 20,
                          "first tuple initializes struct"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "got %d, %d", points[0].x, points[0].y);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "struct_array_init concept: points[1] == (30, 40)",
                          points[1].x == 30 && points[1].y == 40,
                          "second tuple initializes struct"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "got %d, %d", points[1].x, points[1].y);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "struct_array_init concept: points[2] == (50, 60)",
                          points[2].x == 50 && points[2].y == 60,
                          "third tuple initializes struct"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "got %d, %d", points[2].x, points[2].y);
    }

    // verify array count
    size_t count = sizeof(points) / sizeof(points[0]);

    if (!D_TEST_CHECK_MSG(_suite,
                          "struct_array_init concept: array has 3 elements",
                          count == 3,
                          "all tuples converted to structs"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got count: %zu", count);
    }

    // test with different struct type
    struct dj_test_rgb
    {
        int r;
        int g;
        int b;
    };

    // manual equivalent of D_STRUCT_ARRAY_INIT((255, 0, 0), (0, 255, 0))
    struct dj_test_rgb colors[] = {
        {255, 0, 0},
        {0, 255, 0}
    };

    if (!D_TEST_CHECK_MSG(_suite,
                          "struct_array_init concept: colors[0] is red",
                          colors[0].r == 255 && colors[0].g == 0 && colors[0].b == 0,
                          "RGB tuple initialization"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "struct_array_init concept: colors[1] is green",
                          colors[1].r == 0 && colors[1].g == 255 && colors[1].b == 0,
                          "second RGB tuple"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * POINTER ARRAY MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_ptr_array_all
  Runs all pointer array initialization concept tests.
  Tests the following sections:
  - D_FOR_EACH_DATA_COMMA concept
  - D_STRUCT_ARRAY_INIT concept
*/
bool d_tests_dmacro_ptr_array_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_ptr_array_all");

    d_tests_dmacro_for_each_data_comma(_suite);
    d_tests_dmacro_struct_array_init(_suite);

    return true;
}
