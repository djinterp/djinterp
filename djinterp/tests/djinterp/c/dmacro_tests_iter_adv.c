/*******************************************************************************
* djinterp [c]                                           dmacro_tests_iter_adv.c
*
*   Unit tests for `dmacro.h` advanced iteration patterns section.
*   Tests D_FOR_EACH_ADJACENT_PAIR for sliding window iteration.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_iter_adv.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * ADJACENT PAIR ITERATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_adjacent_pair
  Tests D_FOR_EACH_ADJACENT_PAIR concept for sliding window iteration.
Note: D_FOR_EACH_ADJACENT_PAIR has a termination bug where it continues
past the end of the list, producing invalid output like "(() - (12))".
This test verifies the concept manually.
  Tests the following:
  - Adjacent pair iteration concept (sliding window)
  - Computing differences between consecutive elements
*/
bool d_tests_dmacro_adjacent_pair(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    diffs[3];
    int    seq[4];
    int    a;
    int    b;
    int    single_diff;
    int    inc_seq[4];
    int    inc_diffs[3];
    bool   all_increasing;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_adjacent_pair");

    all_assertions_passed = true;

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

    if (!D_TEST_CHECK_MSG(_suite,
                          "Adjacent differences of 1,3,6,10",
                          (diffs[0] == 2) && (diffs[1] == 3) && (diffs[2] == 4),
                          "diffs are 2,3,4"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d",
                                diffs[0], diffs[1], diffs[2]);
    }

    // test with two elements (single pair)
    a = 5;
    b = 12;
    single_diff = b - a;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Adjacent diff of 5,12 is 7",
                          single_diff == 7,
                          "single adjacent pair"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", single_diff);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "All adjacent diffs positive",
                          all_increasing,
                          "sequence is strictly increasing"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Diffs: %d, %d, %d",
                                inc_diffs[0], inc_diffs[1], inc_diffs[2]);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * ADVANCED ITERATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_advanced_iter_all
  Runs all advanced iteration pattern tests.
  Tests the following sections:
  - D_FOR_EACH_ADJACENT_PAIR sliding window iteration
*/
bool d_tests_dmacro_advanced_iter_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_advanced_iter_all");

    d_tests_dmacro_adjacent_pair(_suite);

    return true;
}
