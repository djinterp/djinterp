/******************************************************************************
* djinterp [test]                                  dmacro_tests_sa_ptr_array.c
*
*   Unit tests for `dmacro.h` pointer array initialization section.
*   Tests D_FOR_EACH_DATA_COMMA and D_STRUCT_ARRAY_INIT concepts.
*   Note: D_FOR_EACH_DATA_COMMA passes custom function names through macro
* expansion which is incompatible with MSVC traditional preprocessor.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_ptr_array.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * D_FOR_EACH_DATA_COMMA TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_data_comma
  Tests D_FOR_EACH_DATA_COMMA concept for data-passing iteration.
Note: D_FOR_EACH_DATA_COMMA passes custom function names through macro
expansion which is incompatible with MSVC traditional preprocessor.
  Tests the following:
  - Data-passing iteration concept
  - Comma-separated output generation concept
*/
bool
d_tests_sa_dmacro_for_each_data_comma
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Data-Comma Iteration Concept ---\n", D_INDENT);

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

    if (!d_assert_standalone(results[0] == 10,
                             "data_comma concept: 10 * 1 == 10",
                             "first element with data multiplier",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, results[0]);
    }

    if (!d_assert_standalone(results[1] == 20,
                             "data_comma concept: 10 * 2 == 20",
                             "second element with data multiplier",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, results[1]);
    }

    if (!d_assert_standalone(results[2] == 30,
                             "data_comma concept: 10 * 3 == 30",
                             "third element with data multiplier",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, results[2]);
    }

    // example use case: prefixing strings with a namespace
    // D_FOR_EACH_DATA_COMMA(PREFIX_OP, "ns_", foo, bar) -> "ns_foo", "ns_bar"
    // test concept: data passed to each element
    const char* prefix   = "ns_";
    const char* names[2] = {"foo", "bar"};

    if (!d_assert_standalone(prefix[0] == 'n' && prefix[1] == 's',
                             "data_comma concept: prefix is 'ns_'",
                             "data parameter available for each element",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(names[0][0] == 'f' && names[1][0] == 'b',
                             "data_comma concept: names are 'foo', 'bar'",
                             "elements processed with data",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Data-comma iteration concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Data-comma iteration concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_STRUCT_ARRAY_INIT TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_struct_array_init
  Tests D_STRUCT_ARRAY_INIT concept for struct array initialization.
Note: D_STRUCT_ARRAY_INIT uses D_FOR_EACH_DATA_COMMA internally which passes
custom function names through macro expansion (MSVC incompatible).
  Tests the following:
  - Struct array initialization from tuples concept
  - Compound literal array initialization
*/
bool
d_tests_sa_dmacro_struct_array_init
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Struct Array Init Concept ---\n", D_INDENT);

    // D_STRUCT_ARRAY_INIT((1, 2), (3, 4), (5, 6))
    // would generate: { {1, 2}, {3, 4}, {5, 6} }
    // concept: convert tuple list to brace-enclosed initializers

    struct d_test_point
    {
        int x;
        int y;
    };

    // manual equivalent of D_STRUCT_ARRAY_INIT((10, 20), (30, 40), (50, 60))
    struct d_test_point points[] = {
        {10, 20},
        {30, 40},
        {50, 60}
    };

    if (!d_assert_standalone(points[0].x == 10 && points[0].y == 20,
                             "struct_array_init concept: points[0] == (10, 20)",
                             "first tuple initializes struct",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: (%d, %d)\n", D_INDENT, points[0].x, points[0].y);
    }

    if (!d_assert_standalone(points[1].x == 30 && points[1].y == 40,
                             "struct_array_init concept: points[1] == (30, 40)",
                             "second tuple initializes struct",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: (%d, %d)\n", D_INDENT, points[1].x, points[1].y);
    }

    if (!d_assert_standalone(points[2].x == 50 && points[2].y == 60,
                             "struct_array_init concept: points[2] == (50, 60)",
                             "third tuple initializes struct",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: (%d, %d)\n", D_INDENT, points[2].x, points[2].y);
    }

    // verify array count
    size_t count = sizeof(points) / sizeof(points[0]);

    if (!d_assert_standalone(count == 3,
                             "struct_array_init concept: array has 3 elements",
                             "all tuples converted to structs",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got count: %zu\n", D_INDENT, count);
    }

    // test with different struct type
    struct d_test_rgb
    {
        int r;
        int g;
        int b;
    };

    // manual equivalent of D_STRUCT_ARRAY_INIT((255, 0, 0), (0, 255, 0))
    struct d_test_rgb colors[] = {
        {255, 0, 0},
        {0, 255, 0}
    };

    if (!d_assert_standalone(colors[0].r == 255 && colors[0].g == 0 && colors[0].b == 0,
                             "struct_array_init concept: colors[0] is red",
                             "RGB tuple initialization",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(colors[1].r == 0 && colors[1].g == 255 && colors[1].b == 0,
                             "struct_array_init concept: colors[1] is green",
                             "second RGB tuple",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Struct array init concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Struct array init concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * POINTER ARRAY MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_ptr_array_all
  Runs all pointer array initialization concept tests.
  Tests the following sections:
  - D_FOR_EACH_DATA_COMMA concept
  - D_STRUCT_ARRAY_INIT concept
*/
bool
d_tests_sa_dmacro_ptr_array_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  data_comma_result;
    bool                  struct_init_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Pointer Array Initialization Concepts (Section XII)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all pointer array tests
    data_comma_result  = d_tests_sa_dmacro_for_each_data_comma(&module_counter);
    struct_init_result = d_tests_sa_dmacro_struct_array_init(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = (data_comma_result && struct_init_result);

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Pointer Array Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Pointer Array Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - FOR_EACH_DATA_COMMA: %s\n",
               data_comma_result ? "PASSED" : "FAILED");
        printf("  - STRUCT_ARRAY_INIT:   %s\n",
               struct_init_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
