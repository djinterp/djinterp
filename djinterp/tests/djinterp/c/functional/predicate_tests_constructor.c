/*******************************************************************************
* djinterp [test]                                  predicate_tests_constructor.c
*
*   Standalone tests of predicate.h's constructors: d_predicate_and_new,
* _or_new, _xor_new and _not_new, including their refusal of NULL operands.
*
*
* path:      /tests/djinterp/c/functional/predicate_tests_constructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.09
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./predicate_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_predicate_and_new
  Tests the d_predicate_and_new constructor function.
  Tests the following:
  - successful creation with valid predicates and no context
  - predicate and context field initialization
  - successful creation with contexts
  - context pointer verification
  - NULL predicate handling (passed through, validated in eval)
*/
bool d_tests_predicate_and_new(
    struct dj_test_suite* _suite
)
{
    bool                        result;
    struct dj_predicate_and*     combo;
    int                         context_val;

    d_test_suite_test(_suite, "predicate_and_new");

    result      = true;
    context_val = 42;

    // test 1: successful creation with both predicates and no context
    combo  = d_predicate_and_new(pred_always_true,
                                  NULL,
                                  pred_always_false,
                                  NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "and_new_success_no_context",
                              combo != NULL,
                              "d_predicate_and_new should succeed with NULL contexts") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_new_predicate1_set",
                                  combo->predicate1 == pred_always_true,
                                  "predicate1 should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "and_new_predicate2_set",
                                  combo->predicate2 == pred_always_false,
                                  "predicate2 should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "and_new_contexts_null",
                                  combo->context1 == NULL && combo->context2 == NULL,
                                  "contexts should be NULL") && result;

        free(combo);
    }

    // test 2: successful creation with contexts
    combo  = d_predicate_and_new(pred_greater_than_threshold,
                                  &context_val,
                                  pred_is_even,
                                  NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "and_new_success_with_context",
                              combo != NULL,
                              "d_predicate_and_new should succeed with context") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_new_context_predicate1_set",
                                  combo->predicate1 == pred_greater_than_threshold,
                                  "predicate1 should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "and_new_context1_set",
                                  combo->context1 == &context_val,
                                  "context1 should point to context_val") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "and_new_context2_null",
                                  combo->context2 == NULL,
                                  "context2 should be NULL") && result;

        free(combo);
    }

    //   test 3: creation with NULL predicates is refused, as
    // d_predicate_and_new requires both operands
    combo  = d_predicate_and_new(NULL, NULL, NULL, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "and_new_null_predicates",
                              combo == NULL,
                              "d_predicate_and_new should refuse NULL predicates") && result;

    if (combo)
    {
        free(combo);
    }

    return result;
}


/*
d_tests_predicate_or_new
  Tests the d_predicate_or_new constructor function.
  Tests the following:
  - successful creation with valid predicates and no context
  - predicate and context field initialization
  - successful creation with context
  - context pointer verification
*/
bool d_tests_predicate_or_new(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_predicate_or*  combo;
    int                     threshold;

    d_test_suite_test(_suite, "predicate_or_new");

    result    = true;
    threshold = 10;

    // test 1: successful creation with no context
    combo  = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "or_new_success_no_context",
                              combo != NULL,
                              "d_predicate_or_new should succeed with NULL contexts") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_new_predicate1_set",
                                  combo->predicate1 == pred_is_even,
                                  "predicate1 should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "or_new_predicate2_set",
                                  combo->predicate2 == pred_is_positive,
                                  "predicate2 should be set correctly") && result;

        free(combo);
    }

    // test 2: successful creation with context
    combo  = d_predicate_or_new(pred_greater_than_threshold,
                                 &threshold,
                                 pred_always_true,
                                 NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "or_new_success_with_context",
                              combo != NULL,
                              "d_predicate_or_new should succeed with context") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_new_context1_set",
                                  combo->context1 == &threshold,
                                  "context1 should point to threshold") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "or_new_context2_null",
                                  combo->context2 == NULL,
                                  "context2 should be NULL") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_xor_new
  Tests the d_predicate_xor_new constructor function.
  Tests the following:
  - successful creation with valid predicates
  - predicate and context field initialization
  - creation with mixed predicate types
*/
bool d_tests_predicate_xor_new(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_xor*  combo;

    d_test_suite_test(_suite, "predicate_xor_new");

    result = true;

    // test 1: successful creation
    combo  = d_predicate_xor_new(pred_is_even,
                                  NULL,
                                  pred_is_positive,
                                  NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "xor_new_success",
                              combo != NULL,
                              "d_predicate_xor_new should succeed") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_new_predicate1_set",
                                  combo->predicate1 == pred_is_even,
                                  "predicate1 should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_new_predicate2_set",
                                  combo->predicate2 == pred_is_positive,
                                  "predicate2 should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_new_contexts_null",
                                  combo->context1 == NULL && combo->context2 == NULL,
                                  "contexts should be NULL") && result;

        free(combo);
    }

    // test 2: creation with mixed predicates
    combo  = d_predicate_xor_new(pred_always_true,
                                  NULL,
                                  pred_always_false,
                                  NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "xor_new_mixed_predicates",
                              combo != NULL,
                              "d_predicate_xor_new with mixed predicates should succeed") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_new_mixed_pred1",
                                  combo->predicate1 == pred_always_true,
                                  "predicate1 should be always_true") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_new_mixed_pred2",
                                  combo->predicate2 == pred_always_false,
                                  "predicate2 should be always_false") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_not_new
  Tests the d_predicate_not_new constructor function.
  Tests the following:
  - successful creation with valid predicate and no context
  - predicate and context field initialization
  - successful creation with context
  - creation with always_false predicate
*/
bool d_tests_predicate_not_new(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_not*  combo;
    int                      context_val;

    d_test_suite_test(_suite, "predicate_not_new");

    result      = true;
    context_val = 100;

    // test 1: successful creation with no context
    combo  = d_predicate_not_new(pred_is_even, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "not_new_success_no_context",
                              combo != NULL,
                              "d_predicate_not_new should succeed with NULL context") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_new_predicate_set",
                                  combo->predicate == pred_is_even,
                                  "predicate should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "not_new_context_null",
                                  combo->context == NULL,
                                  "context should be NULL") && result;

        free(combo);
    }

    // test 2: successful creation with context
    combo  = d_predicate_not_new(pred_greater_than_threshold, &context_val);
    result = D_TEST_CHECK_MSG(_suite,
                              "not_new_success_with_context",
                              combo != NULL,
                              "d_predicate_not_new should succeed with context") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_new_context_predicate_set",
                                  combo->predicate == pred_greater_than_threshold,
                                  "predicate should be set correctly") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "not_new_context_set",
                                  combo->context == &context_val,
                                  "context should point to context_val") && result;

        free(combo);
    }

    // test 3: creation with always_false predicate
    combo  = d_predicate_not_new(pred_always_false, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "not_new_always_false",
                              combo != NULL,
                              "d_predicate_not_new with always_false should succeed") && result;

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_new_always_false_set",
                                  combo->predicate == pred_always_false,
                                  "predicate should be always_false") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool d_tests_predicate_constructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "predicate_constructor_all");

    result = true;

    result = d_tests_predicate_and_new(_suite) && result;
    result = d_tests_predicate_or_new(_suite) && result;
    result = d_tests_predicate_xor_new(_suite) && result;
    result = d_tests_predicate_not_new(_suite) && result;

    return result;
}
