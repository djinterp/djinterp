/******************************************************************************
* djinterp [test]                                           dmacro_tests_sa.c
*
*   Master test suite for `dmacro.h` unit tests.
*   This file provides the entry point functions that run all dmacro tests
*   across all test modules.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * MASTER TEST SUITE FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_dmacro_all
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
bool
d_tests_sa_dmacro_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter master_counter;
    bool                  cfg_result;
    bool                  token_result;
    bool                  array_result;
    bool                  argcount_result;
    bool                  eval_result;
    bool                  boolean_result;
    bool                  selection_result;
    bool                  iter_core_result;
    bool                  for_each_result;
    bool                  pair_idx_result;
    bool                  member_result;
    bool                  iter_adv_result;
    bool                  ptr_array_result;
    bool                  utility_result;
    bool                  static_assert_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    master_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("###########################################################################\n");
    printf("###                    DMACRO.H MASTER TEST SUITE                       ###\n");
    printf("###########################################################################\n");

    // run all module tests
    cfg_result           = d_tests_sa_dmacro_cfg_all(&master_counter);
    token_result         = d_tests_sa_dmacro_token_all(&master_counter);
    array_result         = d_tests_sa_dmacro_array_all(&master_counter);
    argcount_result      = d_tests_sa_dmacro_arg_count_all(&master_counter);
    eval_result          = d_tests_sa_dmacro_eval_all(&master_counter);
    boolean_result       = d_tests_sa_dmacro_boolean_all(&master_counter);
    selection_result     = d_tests_sa_dmacro_selection_all(&master_counter);
    iter_core_result     = d_tests_sa_dmacro_iteration_core_all(&master_counter);
    for_each_result      = d_tests_sa_dmacro_for_each_all(&master_counter);
    pair_idx_result      = d_tests_sa_dmacro_pair_indexed_all(&master_counter);
    member_result        = d_tests_sa_dmacro_member_access_all(&master_counter);
    iter_adv_result      = d_tests_sa_dmacro_advanced_iter_all(&master_counter);
    ptr_array_result     = d_tests_sa_dmacro_ptr_array_all(&master_counter);
    utility_result       = d_tests_sa_dmacro_utility_all(&master_counter);
    static_assert_result = d_tests_sa_dmacro_static_assert_all(&master_counter);

    // update caller's totals
    _test_info->assertions_total  += master_counter.assertions_total;
    _test_info->assertions_passed += master_counter.assertions_passed;
    _test_info->tests_total       += master_counter.tests_total;
    _test_info->tests_passed      += master_counter.tests_passed;

    overall_result = ( cfg_result           &&
                       token_result         &&
                       array_result         &&
                       argcount_result      &&
                       eval_result          &&
                       boolean_result       &&
                       selection_result     &&
                       iter_core_result     &&
                       for_each_result      &&
                       pair_idx_result      &&
                       member_result        &&
                       iter_adv_result      &&
                       ptr_array_result     &&
                       utility_result       &&
                       static_assert_result );

    printf("\n");
    printf("###########################################################################\n");
    printf("###                       MASTER SUITE SUMMARY                          ###\n");
    printf("###########################################################################\n");
    printf("\n");

    if (overall_result)
    {
        printf("[PASS] DMACRO.H MASTER SUITE: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               master_counter.assertions_passed,
               master_counter.assertions_total,
               master_counter.tests_passed,
               master_counter.tests_total);
    }
    else
    {
        printf("[FAIL] DMACRO.H MASTER SUITE: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               master_counter.assertions_passed,
               master_counter.assertions_total,
               master_counter.tests_passed,
               master_counter.tests_total);

        printf("\nModule Results:\n");
        printf("  - 0.    Configuration:        %s\n",
               cfg_result ? "PASSED" : "FAILED");
        printf("  - I.    Token Manipulation:   %s\n",
               token_result ? "PASSED" : "FAILED");
        printf("  - II.   Array Utilities:      %s\n",
               array_result ? "PASSED" : "FAILED");
        printf("  - III.  Argument Counting:    %s\n",
               argcount_result ? "PASSED" : "FAILED");
        printf("  - IV.   Eval/Expansion:       %s\n",
               eval_result ? "PASSED" : "FAILED");
        printf("  - V.    Boolean Logic:        %s\n",
               boolean_result ? "PASSED" : "FAILED");
        printf("  - VI.   Argument Selection:   %s\n",
               selection_result ? "PASSED" : "FAILED");
        printf("  - VII.  Core Iteration:       %s\n",
               iter_core_result ? "PASSED" : "FAILED");
        printf("  - VIII. FOR_EACH:             %s\n",
               for_each_result ? "PASSED" : "FAILED");
        printf("  - IX.   Pair/Indexed:         %s\n",
               pair_idx_result ? "PASSED" : "FAILED");
        printf("  - X.    Member Access:        %s\n",
               member_result ? "PASSED" : "FAILED");
        printf("  - XI.   Advanced Iteration:   %s\n",
               iter_adv_result ? "PASSED" : "FAILED");
        printf("  - XII.  Pointer Array:        %s\n",
               ptr_array_result ? "PASSED" : "FAILED");
        printf("  - XIII. Utility Operators:    %s\n",
               utility_result ? "PASSED" : "FAILED");
        printf("  - XIV.  Compile-Time Assert:  %s\n",
               static_assert_result ? "PASSED" : "FAILED");
    }

    printf("\n");

    return overall_result;
}


/*
d_tests_dmacro_run_all
  Runs all dmacro tests and returns a test object with results.
  This is the primary entry point for running the dmacro test suite.
*/
struct d_test_object*
d_tests_dmacro_run_all
(
    void
)
{
    struct d_test_object*  root;
    struct d_test_counter  counter;
    bool                   success;
    static char            message_buf[256];

    // initialize counter
    counter = (struct d_test_counter){0, 0, 0, 0};

    // run all tests
    success = d_tests_sa_dmacro_all(&counter);

    // create result message
    snprintf(message_buf, sizeof(message_buf),
             "%zu/%zu tests, %zu/%zu assertions",
             counter.tests_passed, counter.tests_total,
             counter.assertions_passed, counter.assertions_total);

    // create and return test object as leaf node
    root = d_test_object_new_leaf("dmacro.h", message_buf, success);

    return root;
}
