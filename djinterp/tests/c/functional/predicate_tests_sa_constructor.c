#include ".\predicate_tests_sa.h"
#include ".\predicate_tests_sa_helpers.h"


/*
d_tests_sa_predicate_and_new
  Tests the d_predicate_and_new constructor function.
  Tests the following:
  - successful creation with valid predicates and no context
  - predicate and context field initialization
  - successful creation with contexts
  - context pointer verification
  - NULL predicate handling (passed through, validated in eval)
*/
bool
d_tests_sa_predicate_and_new
(
    struct d_test_counter* _counter
)
{
    bool                        result;
    struct d_predicate_and*     combo;
    int                         context_val;

    result      = true;
    context_val = 42;

    // test 1: successful creation with both predicates and no context
    combo  = d_predicate_and_new(pred_always_true,
                                  NULL,
                                  pred_always_false,
                                  NULL);
    result = d_assert_standalone(
        combo != NULL,
        "and_new_success_no_context",
        "d_predicate_and_new should succeed with NULL contexts",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate1 == pred_always_true,
            "and_new_predicate1_set",
            "predicate1 should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->predicate2 == pred_always_false,
            "and_new_predicate2_set",
            "predicate2 should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->context1 == NULL && combo->context2 == NULL,
            "and_new_contexts_null",
            "contexts should be NULL",
            _counter) && result;

        free(combo);
    }

    // test 2: successful creation with contexts
    combo  = d_predicate_and_new(pred_greater_than_threshold,
                                  &context_val,
                                  pred_is_even,
                                  NULL);
    result = d_assert_standalone(
        combo != NULL,
        "and_new_success_with_context",
        "d_predicate_and_new should succeed with context",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate1 == pred_greater_than_threshold,
            "and_new_context_predicate1_set",
            "predicate1 should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->context1 == &context_val,
            "and_new_context1_set",
            "context1 should point to context_val",
            _counter) && result;

        result = d_assert_standalone(
            combo->context2 == NULL,
            "and_new_context2_null",
            "context2 should be NULL",
            _counter) && result;

        free(combo);
    }

    // test 3: creation with NULL predicates (allowed, fails in eval)
    combo  = d_predicate_and_new(NULL, NULL, NULL, NULL);
    result = d_assert_standalone(
        combo != NULL,
        "and_new_null_predicates",
        "d_predicate_and_new should succeed even with NULL predicates",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate1 == NULL && combo->predicate2 == NULL,
            "and_new_null_predicates_stored",
            "NULL predicates should be stored",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_or_new
  Tests the d_predicate_or_new constructor function.
  Tests the following:
  - successful creation with valid predicates and no context
  - predicate and context field initialization
  - successful creation with context
  - context pointer verification
*/
bool
d_tests_sa_predicate_or_new
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_predicate_or*  combo;
    int                     threshold;

    result    = true;
    threshold = 10;

    // test 1: successful creation with no context
    combo  = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);
    result = d_assert_standalone(
        combo != NULL,
        "or_new_success_no_context",
        "d_predicate_or_new should succeed with NULL contexts",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate1 == pred_is_even,
            "or_new_predicate1_set",
            "predicate1 should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->predicate2 == pred_is_positive,
            "or_new_predicate2_set",
            "predicate2 should be set correctly",
            _counter) && result;

        free(combo);
    }

    // test 2: successful creation with context
    combo  = d_predicate_or_new(pred_greater_than_threshold,
                                 &threshold,
                                 pred_always_true,
                                 NULL);
    result = d_assert_standalone(
        combo != NULL,
        "or_new_success_with_context",
        "d_predicate_or_new should succeed with context",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->context1 == &threshold,
            "or_new_context1_set",
            "context1 should point to threshold",
            _counter) && result;

        result = d_assert_standalone(
            combo->context2 == NULL,
            "or_new_context2_null",
            "context2 should be NULL",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_xor_new
  Tests the d_predicate_xor_new constructor function.
  Tests the following:
  - successful creation with valid predicates
  - predicate and context field initialization
  - creation with mixed predicate types
*/
bool
d_tests_sa_predicate_xor_new
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_predicate_xor*  combo;

    result = true;

    // test 1: successful creation
    combo  = d_predicate_xor_new(pred_is_even,
                                  NULL,
                                  pred_is_positive,
                                  NULL);
    result = d_assert_standalone(
        combo != NULL,
        "xor_new_success",
        "d_predicate_xor_new should succeed",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate1 == pred_is_even,
            "xor_new_predicate1_set",
            "predicate1 should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->predicate2 == pred_is_positive,
            "xor_new_predicate2_set",
            "predicate2 should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->context1 == NULL && combo->context2 == NULL,
            "xor_new_contexts_null",
            "contexts should be NULL",
            _counter) && result;

        free(combo);
    }

    // test 2: creation with mixed predicates
    combo  = d_predicate_xor_new(pred_always_true,
                                  NULL,
                                  pred_always_false,
                                  NULL);
    result = d_assert_standalone(
        combo != NULL,
        "xor_new_mixed_predicates",
        "d_predicate_xor_new with mixed predicates should succeed",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate1 == pred_always_true,
            "xor_new_mixed_pred1",
            "predicate1 should be always_true",
            _counter) && result;

        result = d_assert_standalone(
            combo->predicate2 == pred_always_false,
            "xor_new_mixed_pred2",
            "predicate2 should be always_false",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_not_new
  Tests the d_predicate_not_new constructor function.
  Tests the following:
  - successful creation with valid predicate and no context
  - predicate and context field initialization
  - successful creation with context
  - creation with always_false predicate
*/
bool
d_tests_sa_predicate_not_new
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_predicate_not*  combo;
    int                      context_val;

    result      = true;
    context_val = 100;

    // test 1: successful creation with no context
    combo  = d_predicate_not_new(pred_is_even, NULL);
    result = d_assert_standalone(
        combo != NULL,
        "not_new_success_no_context",
        "d_predicate_not_new should succeed with NULL context",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate == pred_is_even,
            "not_new_predicate_set",
            "predicate should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->context == NULL,
            "not_new_context_null",
            "context should be NULL",
            _counter) && result;

        free(combo);
    }

    // test 2: successful creation with context
    combo  = d_predicate_not_new(pred_greater_than_threshold, &context_val);
    result = d_assert_standalone(
        combo != NULL,
        "not_new_success_with_context",
        "d_predicate_not_new should succeed with context",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate == pred_greater_than_threshold,
            "not_new_context_predicate_set",
            "predicate should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            combo->context == &context_val,
            "not_new_context_set",
            "context should point to context_val",
            _counter) && result;

        free(combo);
    }

    // test 3: creation with always_false predicate
    combo  = d_predicate_not_new(pred_always_false, NULL);
    result = d_assert_standalone(
        combo != NULL,
        "not_new_always_false",
        "d_predicate_not_new with always_false should succeed",
        _counter) && result;

    if (combo)
    {
        result = d_assert_standalone(
            combo->predicate == pred_always_false,
            "not_new_always_false_set",
            "predicate should be always_false",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool
d_tests_sa_predicate_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_predicate_and_new(_counter) && result;
    result = d_tests_sa_predicate_or_new(_counter) && result;
    result = d_tests_sa_predicate_xor_new(_counter) && result;
    result = d_tests_sa_predicate_not_new(_counter) && result;

    return result;
}
