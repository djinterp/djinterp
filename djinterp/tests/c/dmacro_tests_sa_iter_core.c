#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * MAP TERMINATION TESTS
 *****************************************************************************/

// helper macro for testing map termination
#define D_TEST_MAP_ITEM(x) x

/*
d_tests_sa_dmacro_map_end_detection
  Tests D_INTERNAL_MAP_END, D_INTERNAL_MAP_OUT, D_INTERNAL_MAP_GET_END,
  and D_INTERNAL_MAP_NEXT macros for iteration termination.
  Tests the following:
  - D_INTERNAL_MAP_END consumes arguments and produces nothing
  - D_INTERNAL_MAP_GET_END produces termination signal
  - D_INTERNAL_MAP_NEXT correctly detects end vs continuation
  - Map termination works with empty parentheses sentinel
*/
bool
d_tests_sa_dmacro_map_end_detection
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Map End Detection ---\n", D_INDENT);

    // test that D_FOR_EACH terminates properly (uses MAP_END internally)
    // this indirectly tests the termination mechanism
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 1, 2, 3));

    if (!d_assert_standalone(count == 3,
                             "D_FOR_EACH_COMMA produces correct count",
                             "map should terminate after all items",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with single element (edge case for termination)
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 42));

    if (!d_assert_standalone(count == 1,
                             "D_FOR_EACH_COMMA with single element",
                             "map should handle single element",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with many elements to verify repeated termination checks
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_MAP_ITEM,
                                          1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    if (!d_assert_standalone(count == 10,
                             "D_FOR_EACH_COMMA with 10 elements",
                             "map should handle many elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Map end detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Map end detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * CORE MAPPING MECHANISM TESTS
 *****************************************************************************/

// helper macros for testing data-passing and indexed iteration
#define D_TEST_DATA_OP(data, x)  ((data) + (x))
#define D_TEST_IDX_OP(idx, x)    ((idx) * 10 + (x))

/*
d_tests_sa_dmacro_map_internals
  Tests the core mapping mechanism concepts.
Note: D_FOR_EACH with statement arguments may not work correctly.
  Tests the following:
  - Core mapping concept verification
  - D_FOR_EACH_COMMA works correctly
  - Iteration produces expected results
*/
bool
d_tests_sa_dmacro_map_internals
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

    printf("%s--- Testing Map Internals ---\n", D_INDENT);

    // test D_FOR_EACH_COMMA with transformation
    // this exercises the core mapping mechanism
    int transformed[] = { D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 10, 20, 30) };

    if (!d_assert_standalone( (transformed[0] == 10) &&
                              (transformed[1] == 20) &&
                              (transformed[2] == 30),
                             "D_FOR_EACH_COMMA produces correct values",
                             "map should process each element",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT,
               transformed[0], transformed[1], transformed[2]);
    }

    // test that map produces correct count
    int five_items[] = { D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 1, 2, 3, 4, 5) };

    if (!d_assert_standalone( (five_items[0] == 1) &&
                              (five_items[1] == 2) &&
                              (five_items[2] == 3) &&
                              (five_items[3] == 4) &&
                              (five_items[4] == 5),
                             "D_FOR_EACH_COMMA with 5 elements",
                             "map should handle multiple elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d, %d, %d\n", D_INDENT,
               five_items[0], five_items[1], five_items[2],
               five_items[3], five_items[4]);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Map internals test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Map internals test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * CORE ITERATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_iteration_core_all
  Runs all core iteration infrastructure tests.
  Tests the following sections:
  - Map termination detection
  - Core mapping mechanisms
*/
bool
d_tests_sa_dmacro_iteration_core_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  map_end_result;
    bool                  map_internals_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Core Iteration Infrastructure (Section VII)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all core iteration tests
    map_end_result       = d_tests_sa_dmacro_map_end_detection(&module_counter);
    map_internals_result = d_tests_sa_dmacro_map_internals(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = (map_end_result && map_internals_result);

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Core Iteration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Core Iteration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Map End Detection: %s\n",
               map_end_result ? "PASSED" : "FAILED");
        printf("  - Map Internals:     %s\n",
               map_internals_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}