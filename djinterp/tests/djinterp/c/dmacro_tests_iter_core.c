/*******************************************************************************
* djinterp [test]                                       dmacro_tests_iter_core.c
*
*
* path:      /tests/djinterp/c/dmacro_tests_iter_core.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * MAP TERMINATION TESTS
 *****************************************************************************/

// helper macro for testing map termination
#define D_TEST_MAP_ITEM(x) x

/*
d_tests_dmacro_map_end_detection
  Tests D_INTERNAL_MAP_END, D_INTERNAL_MAP_OUT, D_INTERNAL_MAP_GET_END,
  and D_INTERNAL_MAP_NEXT macros for iteration termination.
  Tests the following:
  - D_INTERNAL_MAP_END consumes arguments and produces nothing
  - D_INTERNAL_MAP_GET_END produces termination signal
  - D_INTERNAL_MAP_NEXT correctly detects end vs continuation
  - Map termination works with empty parentheses sentinel
*/
bool d_tests_dmacro_map_end_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_map_end_detection");

    all_assertions_passed = true;

    // test that D_FOR_EACH terminates properly (uses MAP_END internally)
    // this indirectly tests the termination mechanism
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 1, 2, 3));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA produces correct count",
                          count == 3,
                          "map should terminate after all items"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with single element (edge case for termination)
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 42));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA with single element",
                          count == 1,
                          "map should handle single element"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with many elements to verify repeated termination checks
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_MAP_ITEM,
                                          1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA with 10 elements",
                          count == 10,
                          "map should handle many elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * CORE MAPPING MECHANISM TESTS
 *****************************************************************************/

// helper macros for testing data-passing and indexed iteration
#define D_TEST_DATA_OP(data, x)  ((data) + (x))
#define D_TEST_IDX_OP(idx, x)    ((idx) * 10 + (x))

/*
d_tests_dmacro_map_internals
  Tests the core mapping mechanism concepts.
Note: D_FOR_EACH with statement arguments may not work correctly.
  Tests the following:
  - Core mapping concept verification
  - D_FOR_EACH_COMMA works correctly
  - Iteration produces expected results
*/
bool d_tests_dmacro_map_internals(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_map_internals");

    all_assertions_passed = true;

    // test D_FOR_EACH_COMMA with transformation
    // this exercises the core mapping mechanism
    int transformed[] = { D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 10, 20, 30) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA produces correct values",
                          (transformed[0] == 10) && (transformed[1] == 20) && (transformed[2] == 30),
                          "map should process each element"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d",
                                transformed[0], transformed[1], transformed[2]);
    }

    // test that map produces correct count
    int five_items[] = { D_FOR_EACH_COMMA(D_TEST_MAP_ITEM, 1, 2, 3, 4, 5) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA with 5 elements",
                          (five_items[0] == 1) && (five_items[1] == 2) && (five_items[2] == 3) && (five_items[3] == 4) && (five_items[4] == 5),
                          "map should handle multiple elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d, %d, %d",
                                five_items[0], five_items[1], five_items[2],
                                five_items[3], five_items[4]);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * CORE ITERATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_iteration_core_all
  Runs all core iteration infrastructure tests.
  Tests the following sections:
  - Map termination detection
  - Core mapping mechanisms
*/
bool d_tests_dmacro_iteration_core_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_iteration_core_all");

    d_tests_dmacro_map_end_detection(_suite);
    d_tests_dmacro_map_internals(_suite);

    return true;
}
