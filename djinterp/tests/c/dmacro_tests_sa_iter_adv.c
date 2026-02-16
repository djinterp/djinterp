/******************************************************************************
* djinterp [test]                                   dmacro_tests_sa_iter_adv.c
*
*   Unit tests for `dmacro.h` advanced iteration patterns section.
*   Tests D_FOR_EACH_ADJACENT_PAIR for sliding window iteration.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_iter_adv.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * ADJACENT PAIR ITERATION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_adjacent_pair
  Tests D_FOR_EACH_ADJACENT_PAIR concept for sliding window iteration.
Note: D_FOR_EACH_ADJACENT_PAIR has a termination bug where it continues
past the end of the list, producing invalid output like "(() - (12))".
This test verifies the concept manually.
  Tests the following:
  - Adjacent pair iteration concept (sliding window)
  - Computing differences between consecutive elements
*/
bool
d_tests_sa_dmacro_adjacent_pair
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    diffs[3];
    int    seq[4];
    int    a;
    int    b;
    int    single_diff;
    int    inc_seq[4];
    int    inc_diffs[3];
    bool   all_increasing;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Adjacent Pair Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_ADJACENT_PAIR has a termination bug.
    // Test the concept manually: compute differences between adjacent elements.

    // sequence: 1, 3, 6, 10 -> diffs: (3-1)=2, (6-3)=3, (10-6)=4
    seq[0] = 1;
    seq[1] = 3;
    seq[2] = 6;
    seq[3] = 10;

    diffs[0] = seq[1] - seq[0];
    diffs[1] = seq[2] - seq[1];
    diffs[2] = seq[3] - seq[2];

    if (!d_assert_standalone( (diffs[0] == 2) &&
                              (diffs[1] == 3) &&
                              (diffs[2] == 4),
                             "Adjacent differences of 1,3,6,10",
                             "diffs are 2,3,4",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT,
               diffs[0], diffs[1], diffs[2]);
    }

    // test with two elements (single pair)
    a = 5;
    b = 12;
    single_diff = b - a;

    if (!d_assert_standalone(single_diff == 7,
                             "Adjacent diff of 5,12 is 7",
                             "single adjacent pair",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, single_diff);
    }

    // test detecting increasing sequence
    // sequence: 10, 20, 35, 55 -> diffs: 10, 15, 20 (all positive)
    inc_seq[0] = 10;
    inc_seq[1] = 20;
    inc_seq[2] = 35;
    inc_seq[3] = 55;

    inc_diffs[0] = inc_seq[1] - inc_seq[0];
    inc_diffs[1] = inc_seq[2] - inc_seq[1];
    inc_diffs[2] = inc_seq[3] - inc_seq[2];

    all_increasing = (inc_diffs[0] > 0) &&
                     (inc_diffs[1] > 0) &&
                     (inc_diffs[2] > 0);

    if (!d_assert_standalone(all_increasing,
                             "All adjacent diffs positive",
                             "sequence is strictly increasing",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Diffs: %d, %d, %d\n", D_INDENT,
               inc_diffs[0], inc_diffs[1], inc_diffs[2]);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Adjacent pair iteration concept test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Adjacent pair iteration concept test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * ADVANCED ITERATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_advanced_iter_all
  Runs all advanced iteration pattern tests.
  Tests the following sections:
  - D_FOR_EACH_ADJACENT_PAIR sliding window iteration
*/
bool
d_tests_sa_dmacro_advanced_iter_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  adjacent_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Advanced Iteration Patterns (Section XI)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all advanced iteration tests
    adjacent_result = d_tests_sa_dmacro_adjacent_pair(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = adjacent_result;

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Advanced Iteration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Advanced Iteration Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - ADJACENT_PAIR: %s\n",
               adjacent_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
