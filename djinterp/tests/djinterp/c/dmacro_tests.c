/*******************************************************************************
* djinterp [test]                                                 dmacro_tests.c
*
*   Master test suite for `dmacro.h` unit tests.
*   This file provides the entry point functions that run all dmacro tests
*   across all test modules.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * MASTER TEST SUITE FUNCTIONS
 *****************************************************************************/

/*
d_tests_dmacro_all
  Runs all dmacro tests and updates the counter.
  Tests all sections:
  - 0.   Configuration System
  - I.   Token Manipulation
  - II.  Array Utilities
  - III. Argument Counting
  - IV.  Macro Expansion and Evaluation
  - V.   Boolean and Conditional Logic
  - VI.  Argument Selection
  - VII. Core Iteration Infrastructure
  - VIII. FOR_EACH Implementations
  - IX.  Pair and Indexed Iteration
  - X.   Member Access Iteration
  - XI.  Advanced Iteration Patterns
  - XII. Pointer Array Initialization
  - XIII. Utility Operators
  - XIV. Compile-Time Assertions
*/
bool d_tests_dmacro_all(
    struct dj_test_suite* _suite
)
{
    d_tests_dmacro_cfg_all(_suite);
    d_tests_dmacro_token_all(_suite);
    d_tests_dmacro_array_all(_suite);
    d_tests_dmacro_arg_count_all(_suite);
    d_tests_dmacro_eval_all(_suite);
    d_tests_dmacro_boolean_all(_suite);
    d_tests_dmacro_selection_all(_suite);
    d_tests_dmacro_iteration_core_all(_suite);
    d_tests_dmacro_for_each_all(_suite);
    d_tests_dmacro_pair_indexed_all(_suite);
    d_tests_dmacro_member_access_all(_suite);
    d_tests_dmacro_advanced_iter_all(_suite);
    d_tests_dmacro_ptr_array_all(_suite);
    d_tests_dmacro_utility_all(_suite);
    d_tests_dmacro_static_assert_all(_suite);

    return true;
}


/*
d_tests_dmacro_run_all
  Runs all dmacro tests and returns a test object with results.
  This is the primary entry point for running the dmacro test suite.
*/
bool d_tests_dmacro_run_all(struct dj_test_suite* _suite)
{
    d_tests_dmacro_all(_suite);

    return true;
}
