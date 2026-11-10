/*******************************************************************************
* djinterp [functional]                                  predicate_tests_macro.c
*
*
* path:      /tests/djinterp/c/functional/predicate_tests_macro.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./predicate_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_predicate_macro_and
  Tests the D_PREDICATE_AND compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and contexts
  - field values match macro arguments
  - evaluation produces correct results through compound literal
*/
bool d_tests_predicate_macro_and(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_predicate_and combo;
    int                    value;
    int                    threshold;

    d_test_suite_test(_suite, "predicate_macro_and");

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly with contexts
    combo  = D_PREDICATE_AND(pred_greater_than_threshold,
                             &threshold,
                             pred_is_even,
                             NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_predicate1",
                              combo.predicate1 == pred_greater_than_threshold,
                              "D_PREDICATE_AND should set predicate1 correctly") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_predicate2",
                              combo.predicate2 == pred_is_even,
                              "D_PREDICATE_AND should set predicate2 correctly") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_context1",
                              combo.context1 == &threshold,
                              "D_PREDICATE_AND should set context1 correctly") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_context2",
                              combo.context2 == NULL,
                              "D_PREDICATE_AND should set context2 to NULL") && result;

    // test 2: evaluation through macro-created struct
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_eval_true",
                              d_predicate_and_eval(&combo, &value) == true,
                              "10 > 5 AND 10 is even = true via macro") && result;

    value  = 7;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_eval_false",
                              d_predicate_and_eval(&combo, &value) == false,
                              "7 > 5 but 7 is odd = false via macro") && result;

    return result;
}


/*
d_tests_predicate_macro_or
  Tests the D_PREDICATE_OR compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and contexts
  - evaluation produces correct results through compound literal
*/
bool d_tests_predicate_macro_or(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_predicate_or combo;
    int                   value;
    int                   threshold;

    d_test_suite_test(_suite, "predicate_macro_or");

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly
    combo  = D_PREDICATE_OR(pred_greater_than_threshold,
                             &threshold,
                             pred_is_negative,
                             NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_predicate1",
                              combo.predicate1 == pred_greater_than_threshold,
                              "D_PREDICATE_OR should set predicate1 correctly") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_context1",
                              combo.context1 == &threshold,
                              "D_PREDICATE_OR should set context1 correctly") && result;

    // test 2: evaluation through macro-created struct
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_eval_true",
                              d_predicate_or_eval(&combo, &value) == true,
                              "10 > 5, so greater_than OR negative = true via macro") && result;

    value  = 3;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_eval_false",
                              d_predicate_or_eval(&combo, &value) == false,
                              "3 <= 5 and not negative = false via macro") && result;

    return result;
}


/*
d_tests_predicate_macro_xor
  Tests the D_PREDICATE_XOR compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and contexts
  - evaluation produces correct results through compound literal
*/
bool d_tests_predicate_macro_xor(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_predicate_xor combo;
    int                    value;
    int                    threshold;

    d_test_suite_test(_suite, "predicate_macro_xor");

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly
    combo  = D_PREDICATE_XOR(pred_greater_than_threshold,
                              &threshold,
                              pred_is_even,
                              NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_predicate1",
                              combo.predicate1 == pred_greater_than_threshold,
                              "D_PREDICATE_XOR should set predicate1 correctly") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_context1",
                              combo.context1 == &threshold,
                              "D_PREDICATE_XOR should set context1 correctly") && result;

    // test 2: evaluation through macro-created struct
    // 7 > 5 but 7 is odd -> only first true, XOR = true
    value  = 7;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_eval_true",
                              d_predicate_xor_eval(&combo, &value) == true,
                              "7 > 5 but odd, XOR = true via macro") && result;

    // 10 > 5 and 10 is even -> both true, XOR = false
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_eval_false",
                              d_predicate_xor_eval(&combo, &value) == false,
                              "10 > 5 and even, XOR = false via macro") && result;

    return result;
}


/*
d_tests_predicate_macro_not
  Tests the D_PREDICATE_NOT compound literal macro.
  Tests the following:
  - correct struct initialization with predicate and context
  - evaluation produces correct results through compound literal
*/
bool d_tests_predicate_macro_not(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_predicate_not combo;
    int                    value;
    int                    threshold;

    d_test_suite_test(_suite, "predicate_macro_not");

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly
    combo  = D_PREDICATE_NOT(pred_greater_than_threshold, &threshold);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_predicate",
                              combo.predicate == pred_greater_than_threshold,
                              "D_PREDICATE_NOT should set predicate correctly") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_context",
                              combo.context == &threshold,
                              "D_PREDICATE_NOT should set context correctly") && result;

    // test 2: evaluation through macro-created struct
    // NOT (10 > 5) = false
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_eval_false",
                              d_predicate_not_eval(&combo, &value) == false,
                              "NOT (10 > 5) should return false via macro") && result;

    // NOT (3 > 5) = true
    value  = 3;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_eval_true",
                              d_predicate_not_eval(&combo, &value) == true,
                              "NOT (3 > 5) should return true via macro") && result;

    return result;
}


/*
d_tests_predicate_macro_and_simple
  Tests the D_PREDICATE_AND_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and NULL contexts
  - evaluation produces correct results
*/
bool d_tests_predicate_macro_and_simple(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_predicate_and combo;
    int                    value;

    d_test_suite_test(_suite, "predicate_macro_and_simple");

    result = true;

    // test 1: macro initializes with NULL contexts
    combo  = D_PREDICATE_AND_SIMPLE(pred_is_even, pred_is_positive);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_simple_pred1",
                              combo.predicate1 == pred_is_even,
                              "D_PREDICATE_AND_SIMPLE should set predicate1") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_simple_pred2",
                              combo.predicate2 == pred_is_positive,
                              "D_PREDICATE_AND_SIMPLE should set predicate2") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_simple_contexts_null",
                              combo.context1 == NULL && combo.context2 == NULL,
                              "D_PREDICATE_AND_SIMPLE should set both contexts to NULL") && result;

    // test 2: evaluation
    value  = 4;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_simple_eval_true",
                              d_predicate_and_eval(&combo, &value) == true,
                              "4 is even AND positive = true via SIMPLE macro") && result;

    value  = -4;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_and_simple_eval_false",
                              d_predicate_and_eval(&combo, &value) == false,
                              "-4 is even but not positive = false via SIMPLE macro") && result;

    return result;
}


/*
d_tests_predicate_macro_or_simple
  Tests the D_PREDICATE_OR_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and NULL contexts
  - evaluation produces correct results
*/
bool d_tests_predicate_macro_or_simple(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_predicate_or combo;
    int                   value;

    d_test_suite_test(_suite, "predicate_macro_or_simple");

    result = true;

    // test 1: macro initializes with NULL contexts
    combo  = D_PREDICATE_OR_SIMPLE(pred_is_even, pred_is_positive);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_simple_pred1",
                              combo.predicate1 == pred_is_even,
                              "D_PREDICATE_OR_SIMPLE should set predicate1") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_simple_contexts_null",
                              combo.context1 == NULL && combo.context2 == NULL,
                              "D_PREDICATE_OR_SIMPLE should set both contexts to NULL") && result;

    // test 2: evaluation
    value  = -3;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_simple_eval_false",
                              d_predicate_or_eval(&combo, &value) == false,
                              "-3 is neither even nor positive = false via SIMPLE macro") && result;

    value  = -4;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_or_simple_eval_true",
                              d_predicate_or_eval(&combo, &value) == true,
                              "-4 is even, so even OR positive = true via SIMPLE macro") && result;

    return result;
}


/*
d_tests_predicate_macro_xor_simple
  Tests the D_PREDICATE_XOR_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and NULL contexts
  - evaluation produces correct results
*/
bool d_tests_predicate_macro_xor_simple(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_predicate_xor combo;
    int                    value;

    d_test_suite_test(_suite, "predicate_macro_xor_simple");

    result = true;

    // test 1: macro initializes with NULL contexts
    combo  = D_PREDICATE_XOR_SIMPLE(pred_is_even, pred_is_positive);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_simple_pred1",
                              combo.predicate1 == pred_is_even,
                              "D_PREDICATE_XOR_SIMPLE should set predicate1") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_simple_contexts_null",
                              combo.context1 == NULL && combo.context2 == NULL,
                              "D_PREDICATE_XOR_SIMPLE should set both contexts to NULL") && result;

    // test 2: evaluation
    // 3 is only positive -> XOR = true
    value  = 3;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_simple_eval_true",
                              d_predicate_xor_eval(&combo, &value) == true,
                              "3 is only positive, XOR = true via SIMPLE macro") && result;

    // 4 is both even and positive -> XOR = false
    value  = 4;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_xor_simple_eval_false",
                              d_predicate_xor_eval(&combo, &value) == false,
                              "4 is even and positive, XOR = false via SIMPLE macro") && result;

    return result;
}


/*
d_tests_predicate_macro_not_simple
  Tests the D_PREDICATE_NOT_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicate and NULL context
  - evaluation produces correct results
*/
bool d_tests_predicate_macro_not_simple(
    struct dj_test_suite* _suite
)
{
    bool                   result;
    struct dj_predicate_not combo;
    int                    value;

    d_test_suite_test(_suite, "predicate_macro_not_simple");

    result = true;

    // test 1: macro initializes with NULL context
    combo  = D_PREDICATE_NOT_SIMPLE(pred_is_even);
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_simple_pred",
                              combo.predicate == pred_is_even,
                              "D_PREDICATE_NOT_SIMPLE should set predicate") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_simple_context_null",
                              combo.context == NULL,
                              "D_PREDICATE_NOT_SIMPLE should set context to NULL") && result;

    // test 2: evaluation
    // NOT even(4) = false
    value  = 4;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_simple_eval_false",
                              d_predicate_not_eval(&combo, &value) == false,
                              "NOT even(4) = false via SIMPLE macro") && result;

    // NOT even(5) = true
    value  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "macro_not_simple_eval_true",
                              d_predicate_not_eval(&combo, &value) == true,
                              "NOT even(5) = true via SIMPLE macro") && result;

    return result;
}


/*
d_tests_predicate_macro_all
  Aggregation function that runs all macro tests.
*/
bool d_tests_predicate_macro_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "predicate_macro_all");

    result = true;

    result = d_tests_predicate_macro_and(_suite)        && result;
    result = d_tests_predicate_macro_or(_suite)         && result;
    result = d_tests_predicate_macro_xor(_suite)        && result;
    result = d_tests_predicate_macro_not(_suite)        && result;
    result = d_tests_predicate_macro_and_simple(_suite) && result;
    result = d_tests_predicate_macro_or_simple(_suite)  && result;
    result = d_tests_predicate_macro_xor_simple(_suite) && result;
    result = d_tests_predicate_macro_not_simple(_suite) && result;

    return result;
}
