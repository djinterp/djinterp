#include ".\predicate_tests_sa.h"
#include ".\predicate_tests_sa_helpers.h"


/*
d_tests_sa_predicate_macro_and
  Tests the D_PREDICATE_AND compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and contexts
  - field values match macro arguments
  - evaluation produces correct results through compound literal
*/
bool
d_tests_sa_predicate_macro_and
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_and combo;
    int                    value;
    int                    threshold;

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly with contexts
    combo  = D_PREDICATE_AND(pred_greater_than_threshold,
                             &threshold,
                             pred_is_even,
                             NULL);
    result = d_assert_standalone(
        combo.predicate1 == pred_greater_than_threshold,
        "macro_and_predicate1",
        "D_PREDICATE_AND should set predicate1 correctly",
        _counter) && result;

    result = d_assert_standalone(
        combo.predicate2 == pred_is_even,
        "macro_and_predicate2",
        "D_PREDICATE_AND should set predicate2 correctly",
        _counter) && result;

    result = d_assert_standalone(
        combo.context1 == &threshold,
        "macro_and_context1",
        "D_PREDICATE_AND should set context1 correctly",
        _counter) && result;

    result = d_assert_standalone(
        combo.context2 == NULL,
        "macro_and_context2",
        "D_PREDICATE_AND should set context2 to NULL",
        _counter) && result;

    // test 2: evaluation through macro-created struct
    value  = 10;
    result = d_assert_standalone(
        d_predicate_and_eval(&combo, &value) == true,
        "macro_and_eval_true",
        "10 > 5 AND 10 is even = true via macro",
        _counter) && result;

    value  = 7;
    result = d_assert_standalone(
        d_predicate_and_eval(&combo, &value) == false,
        "macro_and_eval_false",
        "7 > 5 but 7 is odd = false via macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_or
  Tests the D_PREDICATE_OR compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and contexts
  - evaluation produces correct results through compound literal
*/
bool
d_tests_sa_predicate_macro_or
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_predicate_or combo;
    int                   value;
    int                   threshold;

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly
    combo  = D_PREDICATE_OR(pred_greater_than_threshold,
                             &threshold,
                             pred_is_negative,
                             NULL);
    result = d_assert_standalone(
        combo.predicate1 == pred_greater_than_threshold,
        "macro_or_predicate1",
        "D_PREDICATE_OR should set predicate1 correctly",
        _counter) && result;

    result = d_assert_standalone(
        combo.context1 == &threshold,
        "macro_or_context1",
        "D_PREDICATE_OR should set context1 correctly",
        _counter) && result;

    // test 2: evaluation through macro-created struct
    value  = 10;
    result = d_assert_standalone(
        d_predicate_or_eval(&combo, &value) == true,
        "macro_or_eval_true",
        "10 > 5, so greater_than OR negative = true via macro",
        _counter) && result;

    value  = 3;
    result = d_assert_standalone(
        d_predicate_or_eval(&combo, &value) == false,
        "macro_or_eval_false",
        "3 <= 5 and not negative = false via macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_xor
  Tests the D_PREDICATE_XOR compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and contexts
  - evaluation produces correct results through compound literal
*/
bool
d_tests_sa_predicate_macro_xor
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_xor combo;
    int                    value;
    int                    threshold;

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly
    combo  = D_PREDICATE_XOR(pred_greater_than_threshold,
                              &threshold,
                              pred_is_even,
                              NULL);
    result = d_assert_standalone(
        combo.predicate1 == pred_greater_than_threshold,
        "macro_xor_predicate1",
        "D_PREDICATE_XOR should set predicate1 correctly",
        _counter) && result;

    result = d_assert_standalone(
        combo.context1 == &threshold,
        "macro_xor_context1",
        "D_PREDICATE_XOR should set context1 correctly",
        _counter) && result;

    // test 2: evaluation through macro-created struct
    // 7 > 5 but 7 is odd -> only first true, XOR = true
    value  = 7;
    result = d_assert_standalone(
        d_predicate_xor_eval(&combo, &value) == true,
        "macro_xor_eval_true",
        "7 > 5 but odd, XOR = true via macro",
        _counter) && result;

    // 10 > 5 and 10 is even -> both true, XOR = false
    value  = 10;
    result = d_assert_standalone(
        d_predicate_xor_eval(&combo, &value) == false,
        "macro_xor_eval_false",
        "10 > 5 and even, XOR = false via macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_not
  Tests the D_PREDICATE_NOT compound literal macro.
  Tests the following:
  - correct struct initialization with predicate and context
  - evaluation produces correct results through compound literal
*/
bool
d_tests_sa_predicate_macro_not
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_not combo;
    int                    value;
    int                    threshold;

    result    = true;
    threshold = 5;

    // test 1: macro initializes fields correctly
    combo  = D_PREDICATE_NOT(pred_greater_than_threshold, &threshold);
    result = d_assert_standalone(
        combo.predicate == pred_greater_than_threshold,
        "macro_not_predicate",
        "D_PREDICATE_NOT should set predicate correctly",
        _counter) && result;

    result = d_assert_standalone(
        combo.context == &threshold,
        "macro_not_context",
        "D_PREDICATE_NOT should set context correctly",
        _counter) && result;

    // test 2: evaluation through macro-created struct
    // NOT (10 > 5) = false
    value  = 10;
    result = d_assert_standalone(
        d_predicate_not_eval(&combo, &value) == false,
        "macro_not_eval_false",
        "NOT (10 > 5) should return false via macro",
        _counter) && result;

    // NOT (3 > 5) = true
    value  = 3;
    result = d_assert_standalone(
        d_predicate_not_eval(&combo, &value) == true,
        "macro_not_eval_true",
        "NOT (3 > 5) should return true via macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_and_simple
  Tests the D_PREDICATE_AND_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and NULL contexts
  - evaluation produces correct results
*/
bool
d_tests_sa_predicate_macro_and_simple
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_and combo;
    int                    value;

    result = true;

    // test 1: macro initializes with NULL contexts
    combo  = D_PREDICATE_AND_SIMPLE(pred_is_even, pred_is_positive);
    result = d_assert_standalone(
        combo.predicate1 == pred_is_even,
        "macro_and_simple_pred1",
        "D_PREDICATE_AND_SIMPLE should set predicate1",
        _counter) && result;

    result = d_assert_standalone(
        combo.predicate2 == pred_is_positive,
        "macro_and_simple_pred2",
        "D_PREDICATE_AND_SIMPLE should set predicate2",
        _counter) && result;

    result = d_assert_standalone(
        combo.context1 == NULL && combo.context2 == NULL,
        "macro_and_simple_contexts_null",
        "D_PREDICATE_AND_SIMPLE should set both contexts to NULL",
        _counter) && result;

    // test 2: evaluation
    value  = 4;
    result = d_assert_standalone(
        d_predicate_and_eval(&combo, &value) == true,
        "macro_and_simple_eval_true",
        "4 is even AND positive = true via SIMPLE macro",
        _counter) && result;

    value  = -4;
    result = d_assert_standalone(
        d_predicate_and_eval(&combo, &value) == false,
        "macro_and_simple_eval_false",
        "-4 is even but not positive = false via SIMPLE macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_or_simple
  Tests the D_PREDICATE_OR_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and NULL contexts
  - evaluation produces correct results
*/
bool
d_tests_sa_predicate_macro_or_simple
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_predicate_or combo;
    int                   value;

    result = true;

    // test 1: macro initializes with NULL contexts
    combo  = D_PREDICATE_OR_SIMPLE(pred_is_even, pred_is_positive);
    result = d_assert_standalone(
        combo.predicate1 == pred_is_even,
        "macro_or_simple_pred1",
        "D_PREDICATE_OR_SIMPLE should set predicate1",
        _counter) && result;

    result = d_assert_standalone(
        combo.context1 == NULL && combo.context2 == NULL,
        "macro_or_simple_contexts_null",
        "D_PREDICATE_OR_SIMPLE should set both contexts to NULL",
        _counter) && result;

    // test 2: evaluation
    value  = -3;
    result = d_assert_standalone(
        d_predicate_or_eval(&combo, &value) == false,
        "macro_or_simple_eval_false",
        "-3 is neither even nor positive = false via SIMPLE macro",
        _counter) && result;

    value  = -4;
    result = d_assert_standalone(
        d_predicate_or_eval(&combo, &value) == true,
        "macro_or_simple_eval_true",
        "-4 is even, so even OR positive = true via SIMPLE macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_xor_simple
  Tests the D_PREDICATE_XOR_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicates and NULL contexts
  - evaluation produces correct results
*/
bool
d_tests_sa_predicate_macro_xor_simple
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_xor combo;
    int                    value;

    result = true;

    // test 1: macro initializes with NULL contexts
    combo  = D_PREDICATE_XOR_SIMPLE(pred_is_even, pred_is_positive);
    result = d_assert_standalone(
        combo.predicate1 == pred_is_even,
        "macro_xor_simple_pred1",
        "D_PREDICATE_XOR_SIMPLE should set predicate1",
        _counter) && result;

    result = d_assert_standalone(
        combo.context1 == NULL && combo.context2 == NULL,
        "macro_xor_simple_contexts_null",
        "D_PREDICATE_XOR_SIMPLE should set both contexts to NULL",
        _counter) && result;

    // test 2: evaluation
    // 3 is only positive -> XOR = true
    value  = 3;
    result = d_assert_standalone(
        d_predicate_xor_eval(&combo, &value) == true,
        "macro_xor_simple_eval_true",
        "3 is only positive, XOR = true via SIMPLE macro",
        _counter) && result;

    // 4 is both even and positive -> XOR = false
    value  = 4;
    result = d_assert_standalone(
        d_predicate_xor_eval(&combo, &value) == false,
        "macro_xor_simple_eval_false",
        "4 is even and positive, XOR = false via SIMPLE macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_not_simple
  Tests the D_PREDICATE_NOT_SIMPLE compound literal macro.
  Tests the following:
  - correct struct initialization with predicate and NULL context
  - evaluation produces correct results
*/
bool
d_tests_sa_predicate_macro_not_simple
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_not combo;
    int                    value;

    result = true;

    // test 1: macro initializes with NULL context
    combo  = D_PREDICATE_NOT_SIMPLE(pred_is_even);
    result = d_assert_standalone(
        combo.predicate == pred_is_even,
        "macro_not_simple_pred",
        "D_PREDICATE_NOT_SIMPLE should set predicate",
        _counter) && result;

    result = d_assert_standalone(
        combo.context == NULL,
        "macro_not_simple_context_null",
        "D_PREDICATE_NOT_SIMPLE should set context to NULL",
        _counter) && result;

    // test 2: evaluation
    // NOT even(4) = false
    value  = 4;
    result = d_assert_standalone(
        d_predicate_not_eval(&combo, &value) == false,
        "macro_not_simple_eval_false",
        "NOT even(4) = false via SIMPLE macro",
        _counter) && result;

    // NOT even(5) = true
    value  = 5;
    result = d_assert_standalone(
        d_predicate_not_eval(&combo, &value) == true,
        "macro_not_simple_eval_true",
        "NOT even(5) = true via SIMPLE macro",
        _counter) && result;

    return result;
}


/*
d_tests_sa_predicate_macro_all
  Aggregation function that runs all macro tests.
*/
bool
d_tests_sa_predicate_macro_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Compound Literal Macros\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_predicate_macro_and(_counter)        && result;
    result = d_tests_sa_predicate_macro_or(_counter)         && result;
    result = d_tests_sa_predicate_macro_xor(_counter)        && result;
    result = d_tests_sa_predicate_macro_not(_counter)        && result;
    result = d_tests_sa_predicate_macro_and_simple(_counter) && result;
    result = d_tests_sa_predicate_macro_or_simple(_counter)  && result;
    result = d_tests_sa_predicate_macro_xor_simple(_counter) && result;
    result = d_tests_sa_predicate_macro_not_simple(_counter) && result;

    return result;
}
