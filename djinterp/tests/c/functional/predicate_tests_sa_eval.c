#include ".\predicate_tests_sa.h"
#include ".\predicate_tests_sa_helpers.h"


/*
d_tests_sa_predicate_and_eval
  Tests the d_predicate_and_eval evaluation function.
  Tests the following:
  - NULL combo rejection
  - NULL predicate1 rejection
  - NULL predicate2 rejection
  - both NULL predicates rejection
  - true && true = true
  - true && false = false
  - false && true = false (short-circuit)
  - false && false = false
  - real predicate logic (even AND positive)
  - context passing (greater_than_threshold AND is_even)
  - NULL element handling
  - zero boundary value
*/
bool
d_tests_sa_predicate_and_eval
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_predicate_and* combo;
    int                     value;
    int                     threshold;

    result    = true;
    threshold = 5;

    // test 1: NULL combo should return false
    value  = 10;
    result = d_assert_standalone(
        d_predicate_and_eval(NULL, &value) == false,
        "and_eval_null_combo",
        "NULL combo should return false",
        _counter) && result;

    // test 2: NULL predicate1 should return false
    combo = d_predicate_and_new(NULL, NULL, pred_always_true, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_null_predicate1",
            "NULL predicate1 should return false",
            _counter) && result;

        free(combo);
    }

    // test 3: NULL predicate2 should return false
    combo = d_predicate_and_new(pred_always_true, NULL, NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_null_predicate2",
            "NULL predicate2 should return false",
            _counter) && result;

        free(combo);
    }

    // test 4: both NULL predicates should return false
    combo = d_predicate_and_new(NULL, NULL, NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_both_null_predicates",
            "both NULL predicates should return false",
            _counter) && result;

        free(combo);
    }

    // test 5: true AND true = true
    combo = d_predicate_and_new(pred_always_true,
                                 NULL,
                                 pred_always_true,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == true,
            "and_eval_true_and_true",
            "true AND true should return true",
            _counter) && result;

        free(combo);
    }

    // test 6: true AND false = false
    combo = d_predicate_and_new(pred_always_true,
                                 NULL,
                                 pred_always_false,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_true_and_false",
            "true AND false should return false",
            _counter) && result;

        free(combo);
    }

    // test 7: false AND true = false (short-circuit test)
    combo = d_predicate_and_new(pred_always_false,
                                 NULL,
                                 pred_always_true,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_false_and_true",
            "false AND true should return false (short-circuit)",
            _counter) && result;

        free(combo);
    }

    // test 8: false AND false = false
    combo = d_predicate_and_new(pred_always_false,
                                 NULL,
                                 pred_always_false,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_false_and_false",
            "false AND false should return false",
            _counter) && result;

        free(combo);
    }

    // test 9: even AND positive (both true)
    combo = d_predicate_and_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        // 4 is even and positive = true
        value  = 4;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == true,
            "and_eval_even_and_positive_4",
            "4 is even AND positive = true",
            _counter) && result;

        // 3 is odd but positive (even fails)
        value  = 3;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_even_and_positive_3",
            "3 is odd, so even AND positive = false",
            _counter) && result;

        // -4 is even but negative (positive fails)
        value  = -4;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_even_and_positive_neg4",
            "-4 is negative, so even AND positive = false",
            _counter) && result;

        free(combo);
    }

    // test 10: context usage
    combo = d_predicate_and_new(pred_greater_than_threshold,
                                 &threshold,
                                 pred_is_even,
                                 NULL);

    if (combo)
    {
        // 10 > 5 and 10 is even = true
        value  = 10;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == true,
            "and_eval_context_10",
            "10 > 5 AND 10 is even = true",
            _counter) && result;

        // 7 > 5 but 7 is odd = false
        value  = 7;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_context_7",
            "7 > 5 but 7 is odd = false",
            _counter) && result;

        // 2 is even but 2 <= 5 = false
        value  = 2;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_context_2",
            "2 is even but 2 <= 5 = false",
            _counter) && result;

        free(combo);
    }

    // test 11: NULL element handling
    combo = d_predicate_and_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_and_eval(combo, NULL) == false,
            "and_eval_null_element",
            "NULL element should return false (predicates check for NULL)",
            _counter) && result;

        free(combo);
    }

    // test 12: zero boundary value (0 is even but not positive)
    combo = d_predicate_and_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        value  = 0;
        result = d_assert_standalone(
            d_predicate_and_eval(combo, &value) == false,
            "and_eval_zero_boundary",
            "0 is even but not positive, so AND = false",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_or_eval
  Tests the d_predicate_or_eval evaluation function.
  Tests the following:
  - NULL combo rejection
  - NULL predicate1 rejection
  - NULL predicate2 rejection
  - both NULL predicates rejection
  - true || true = true
  - true || false = true (short-circuit)
  - false || true = true
  - false || false = false
  - real predicate logic (even OR positive)
  - context passing (greater_than_threshold OR is_negative)
  - NULL element handling
  - zero boundary value
*/
bool
d_tests_sa_predicate_or_eval
(
    struct d_test_counter* _counter
)
{
    bool                   result;
    struct d_predicate_or* combo;
    int                    value;
    int                    threshold;

    result    = true;
    threshold = 5;

    // test 1: NULL combo should return false
    value  = 10;
    result = d_assert_standalone(
        d_predicate_or_eval(NULL, &value) == false,
        "or_eval_null_combo",
        "NULL combo should return false",
        _counter) && result;

    // test 2: NULL predicate1 should return false
    combo = d_predicate_or_new(NULL, NULL, pred_always_true, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == false,
            "or_eval_null_predicate1",
            "NULL predicate1 should return false",
            _counter) && result;

        free(combo);
    }

    // test 3: NULL predicate2 should return false
    combo = d_predicate_or_new(pred_always_true, NULL, NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == false,
            "or_eval_null_predicate2",
            "NULL predicate2 should return false",
            _counter) && result;

        free(combo);
    }

    // test 4: both NULL predicates should return false
    combo = d_predicate_or_new(NULL, NULL, NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == false,
            "or_eval_both_null_predicates",
            "both NULL predicates should return false",
            _counter) && result;

        free(combo);
    }

    // test 5: true OR true = true
    combo = d_predicate_or_new(pred_always_true, NULL, pred_always_true, NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_true_or_true",
            "true OR true should return true",
            _counter) && result;

        free(combo);
    }

    // test 6: true OR false = true (short-circuit test)
    combo = d_predicate_or_new(pred_always_true,
                                NULL,
                                pred_always_false,
                                NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_true_or_false",
            "true OR false should return true (short-circuit)",
            _counter) && result;

        free(combo);
    }

    // test 7: false OR true = true
    combo = d_predicate_or_new(pred_always_false,
                                NULL,
                                pred_always_true,
                                NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_false_or_true",
            "false OR true should return true",
            _counter) && result;

        free(combo);
    }

    // test 8: false OR false = false
    combo = d_predicate_or_new(pred_always_false,
                                NULL,
                                pred_always_false,
                                NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == false,
            "or_eval_false_or_false",
            "false OR false should return false",
            _counter) && result;

        free(combo);
    }

    // test 9: even OR positive
    combo = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        // 4 is even and positive = true
        value  = 4;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_even_or_positive_4",
            "4 is even OR positive = true",
            _counter) && result;

        // 3 is odd but positive = true
        value  = 3;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_even_or_positive_3",
            "3 is positive, so even OR positive = true",
            _counter) && result;

        // -4 is even but negative = true
        value  = -4;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_even_or_positive_neg4",
            "-4 is even, so even OR positive = true",
            _counter) && result;

        // -3 is odd and negative = false
        value  = -3;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == false,
            "or_eval_even_or_positive_neg3",
            "-3 is odd and negative, so even OR positive = false",
            _counter) && result;

        free(combo);
    }

    // test 10: context usage
    combo = d_predicate_or_new(pred_greater_than_threshold,
                                &threshold,
                                pred_is_negative,
                                NULL);

    if (combo)
    {
        // 10 > 5 = true
        value  = 10;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_context_10",
            "10 > 5, so greater_than OR negative = true",
            _counter) && result;

        // -2 is negative = true
        value  = -2;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_context_neg2",
            "-2 is negative, so greater_than OR negative = true",
            _counter) && result;

        // 3 <= 5 and not negative = false
        value  = 3;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == false,
            "or_eval_context_3",
            "3 <= 5 and not negative = false",
            _counter) && result;

        free(combo);
    }

    // test 11: NULL element handling
    combo = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_or_eval(combo, NULL) == false,
            "or_eval_null_element",
            "NULL element should return false (both predicates return false)",
            _counter) && result;

        free(combo);
    }

    // test 12: zero boundary value (0 is even but not positive)
    combo = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        value  = 0;
        result = d_assert_standalone(
            d_predicate_or_eval(combo, &value) == true,
            "or_eval_zero_boundary",
            "0 is even, so even OR positive = true",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_xor_eval
  Tests the d_predicate_xor_eval evaluation function.
  Tests the following:
  - NULL combo rejection
  - NULL predicate1 rejection
  - NULL predicate2 rejection
  - both NULL predicates rejection
  - true XOR true = false
  - true XOR false = true
  - false XOR true = true
  - false XOR false = false
  - mutually exclusive predicates (even XOR odd)
  - overlapping predicates (even XOR positive)
  - context passing (greater_than_threshold XOR is_even)
  - NULL element handling
*/
bool
d_tests_sa_predicate_xor_eval
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_predicate_xor* combo;
    int                     value;
    int                     threshold;

    result    = true;
    threshold = 5;

    // test 1: NULL combo should return false
    value  = 10;
    result = d_assert_standalone(
        d_predicate_xor_eval(NULL, &value) == false,
        "xor_eval_null_combo",
        "NULL combo should return false",
        _counter) && result;

    // test 2: NULL predicate1 should return false
    combo = d_predicate_xor_new(NULL, NULL, pred_always_true, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_null_predicate1",
            "NULL predicate1 should return false",
            _counter) && result;

        free(combo);
    }

    // test 3: NULL predicate2 should return false
    combo = d_predicate_xor_new(pred_always_true, NULL, NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_null_predicate2",
            "NULL predicate2 should return false",
            _counter) && result;

        free(combo);
    }

    // test 4: both NULL predicates should return false
    combo = d_predicate_xor_new(NULL, NULL, NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_both_null_predicates",
            "both NULL predicates should return false",
            _counter) && result;

        free(combo);
    }

    // test 5: true XOR true = false
    combo = d_predicate_xor_new(pred_always_true,
                                 NULL,
                                 pred_always_true,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_true_xor_true",
            "true XOR true should return false",
            _counter) && result;

        free(combo);
    }

    // test 6: true XOR false = true
    combo = d_predicate_xor_new(pred_always_true,
                                 NULL,
                                 pred_always_false,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_true_xor_false",
            "true XOR false should return true",
            _counter) && result;

        free(combo);
    }

    // test 7: false XOR true = true
    combo = d_predicate_xor_new(pred_always_false,
                                 NULL,
                                 pred_always_true,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_false_xor_true",
            "false XOR true should return true",
            _counter) && result;

        free(combo);
    }

    // test 8: false XOR false = false
    combo = d_predicate_xor_new(pred_always_false,
                                 NULL,
                                 pred_always_false,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_false_xor_false",
            "false XOR false should return false",
            _counter) && result;

        free(combo);
    }

    // test 9: even XOR odd (mutually exclusive)
    combo = d_predicate_xor_new(pred_is_even, NULL, pred_is_odd, NULL);

    if (combo)
    {
        // even number (4): even=true, odd=false -> XOR=true
        value  = 4;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_even_xor_odd_4",
            "4 is even XOR odd = true",
            _counter) && result;

        // odd number (5): even=false, odd=true -> XOR=true
        value  = 5;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_even_xor_odd_5",
            "5 is even XOR odd = true",
            _counter) && result;

        free(combo);
    }

    // test 10: even XOR positive (can both be true)
    combo = d_predicate_xor_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        // 4 is both even and positive = false
        value  = 4;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_even_xor_positive_4",
            "4 is both even and positive, XOR = false",
            _counter) && result;

        // 3 is odd but positive = true
        value  = 3;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_even_xor_positive_3",
            "3 is only positive, XOR = true",
            _counter) && result;

        // -4 is even but negative = true
        value  = -4;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_even_xor_positive_neg4",
            "-4 is only even, XOR = true",
            _counter) && result;

        // -3 is neither = false
        value  = -3;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_even_xor_positive_neg3",
            "-3 is neither even nor positive, XOR = false",
            _counter) && result;

        free(combo);
    }

    // test 11: context usage (greater_than_threshold XOR is_even)
    combo = d_predicate_xor_new(pred_greater_than_threshold,
                                 &threshold,
                                 pred_is_even,
                                 NULL);

    if (combo)
    {
        // 10 > 5 and 10 is even -> both true, XOR = false
        value  = 10;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_context_10",
            "10 > 5 and even, XOR = false",
            _counter) && result;

        // 7 > 5 and 7 is odd -> only first true, XOR = true
        value  = 7;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_context_7",
            "7 > 5 but odd, XOR = true",
            _counter) && result;

        // 2 <= 5 and 2 is even -> only second true, XOR = true
        value  = 2;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == true,
            "xor_eval_context_2",
            "2 <= 5 but even, XOR = true",
            _counter) && result;

        // 3 <= 5 and 3 is odd -> both false, XOR = false
        value  = 3;
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, &value) == false,
            "xor_eval_context_3",
            "3 <= 5 and odd, XOR = false",
            _counter) && result;

        free(combo);
    }

    // test 12: NULL element handling
    combo = d_predicate_xor_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_xor_eval(combo, NULL) == false,
            "xor_eval_null_element",
            "NULL element: both return false, XOR = false",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_not_eval
  Tests the d_predicate_not_eval evaluation function.
  Tests the following:
  - NULL combo rejection
  - NULL predicate rejection
  - NOT true = false
  - NOT false = true
  - real predicate logic (NOT is_even, NOT is_positive)
  - context passing (NOT greater_than_threshold)
  - NULL element handling
*/
bool
d_tests_sa_predicate_not_eval
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_predicate_not* combo;
    int                     value;
    int                     threshold;

    result    = true;
    threshold = 5;

    // test 1: NULL combo should return false
    value  = 10;
    result = d_assert_standalone(
        d_predicate_not_eval(NULL, &value) == false,
        "not_eval_null_combo",
        "NULL combo should return false",
        _counter) && result;

    // test 2: NULL predicate should return false
    combo = d_predicate_not_new(NULL, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == false,
            "not_eval_null_predicate",
            "NULL predicate should return false",
            _counter) && result;

        free(combo);
    }

    // test 3: NOT true = false
    combo = d_predicate_not_new(pred_always_true, NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == false,
            "not_eval_not_true",
            "NOT true should return false",
            _counter) && result;

        free(combo);
    }

    // test 4: NOT false = true
    combo = d_predicate_not_new(pred_always_false, NULL);

    if (combo)
    {
        value  = 42;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == true,
            "not_eval_not_false",
            "NOT false should return true",
            _counter) && result;

        free(combo);
    }

    // test 5: NOT is_even
    combo = d_predicate_not_new(pred_is_even, NULL);

    if (combo)
    {
        // even number (4) -> NOT even = false
        value  = 4;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == false,
            "not_eval_not_even_4",
            "NOT even(4) should return false",
            _counter) && result;

        // odd number (5) -> NOT even = true
        value  = 5;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == true,
            "not_eval_not_even_5",
            "NOT even(5) should return true",
            _counter) && result;

        free(combo);
    }

    // test 6: NOT is_positive
    combo = d_predicate_not_new(pred_is_positive, NULL);

    if (combo)
    {
        // positive (10) -> NOT positive = false
        value  = 10;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == false,
            "not_eval_not_positive_10",
            "NOT positive(10) should return false",
            _counter) && result;

        // negative (-5) -> NOT positive = true
        value  = -5;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == true,
            "not_eval_not_positive_neg5",
            "NOT positive(-5) should return true",
            _counter) && result;

        // zero -> NOT positive = true
        value  = 0;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == true,
            "not_eval_not_positive_0",
            "NOT positive(0) should return true",
            _counter) && result;

        free(combo);
    }

    // test 7: context usage
    combo = d_predicate_not_new(pred_greater_than_threshold, &threshold);

    if (combo)
    {
        // 10 > 5 -> NOT (10 > 5) = false
        value  = 10;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == false,
            "not_eval_context_10",
            "NOT (10 > 5) should return false",
            _counter) && result;

        // 3 <= 5 -> NOT (3 > 5) = true
        value  = 3;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == true,
            "not_eval_context_3",
            "NOT (3 > 5) should return true",
            _counter) && result;

        // 5 <= 5 -> NOT (5 > 5) = true
        value  = 5;
        result = d_assert_standalone(
            d_predicate_not_eval(combo, &value) == true,
            "not_eval_context_5",
            "NOT (5 > 5) should return true",
            _counter) && result;

        free(combo);
    }

    // test 8: NULL element handling
    combo = d_predicate_not_new(pred_is_even, NULL);

    if (combo)
    {
        result = d_assert_standalone(
            d_predicate_not_eval(combo, NULL) == true,
            "not_eval_null_element",
            "NOT (pred returns false for NULL) should return true",
            _counter) && result;

        free(combo);
    }

    return result;
}


/*
d_tests_sa_predicate_eval_all
  Aggregation function that runs all evaluation tests.
*/
bool
d_tests_sa_predicate_eval_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Evaluation Functions\n");
    printf("  -------------------------------\n");

    result = d_tests_sa_predicate_and_eval(_counter) && result;
    result = d_tests_sa_predicate_or_eval(_counter) && result;
    result = d_tests_sa_predicate_xor_eval(_counter) && result;
    result = d_tests_sa_predicate_not_eval(_counter) && result;

    return result;
}
