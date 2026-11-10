/*******************************************************************************
* djinterp [functional]                                   predicate_tests_eval.c
*
*
* path:      /tests/djinterp/c/functional/predicate_tests_eval.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./predicate_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/**
 * @brief Part 1 of d_tests_predicate_and_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL combo should return false
 *   - test 2: NULL predicate1 should return false
 *   - test 3: NULL predicate2 should return false
 *   - test 4: both NULL predicates should return false
 *   - test 5: true AND true = true
 *   - test 6: true AND false = false
 */
static bool
d_internal_tests_predicate_and_eval_part1(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_and* combo;
    int                      value;

    result = true;

    // test 1: NULL combo should return false
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "and_eval_null_combo",
                              d_predicate_and_eval(NULL, &value) == false,
                              "NULL combo should return false") && result;

    // test 2: NULL predicate1 should return false
    combo = d_predicate_and_new(NULL, NULL, pred_always_true, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_null_predicate1",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "NULL predicate1 should return false") && result;

        free(combo);
    }

    // test 3: NULL predicate2 should return false
    combo = d_predicate_and_new(pred_always_true, NULL, NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_null_predicate2",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "NULL predicate2 should return false") && result;

        free(combo);
    }

    // test 4: both NULL predicates should return false
    combo = d_predicate_and_new(NULL, NULL, NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_both_null_predicates",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "both NULL predicates should return false") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_true_and_true",
                                  d_predicate_and_eval(combo, &value) == true,
                                  "true AND true should return true") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_true_and_false",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "true AND false should return false") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_predicate_and_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 7: false AND true = false (short-circuit test)
 *   - test 8: false AND false = false
 *   - test 9: even AND positive (both true)
 */
static bool
d_internal_tests_predicate_and_eval_part2(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_and* combo;
    int                      value;

    result = true;

    // test 7: false AND true = false (short-circuit test)
    combo = d_predicate_and_new(pred_always_false,
                                 NULL,
                                 pred_always_true,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_false_and_true",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "false AND true should return false (short-circuit)") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_false_and_false",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "false AND false should return false") && result;

        free(combo);
    }

    // test 9: even AND positive (both true)
    combo = d_predicate_and_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        // 4 is even and positive = true
        value  = 4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_even_and_positive_4",
                                  d_predicate_and_eval(combo, &value) == true,
                                  "4 is even AND positive = true") && result;

        // 3 is odd but positive (even fails)
        value  = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_even_and_positive_3",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "3 is odd, so even AND positive = false") && result;

        // -4 is even but negative (positive fails)
        value  = -4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_even_and_positive_neg4",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "-4 is negative, so even AND positive = false") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 3 of d_tests_predicate_and_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 10: context usage
 *   - test 11: NULL element handling
 *   - test 12: zero boundary value (0 is even but not positive)
 */
static bool
d_internal_tests_predicate_and_eval_part3(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_and* combo;
    int                      value;
    int                      threshold;

    result    = true;
    threshold = 5;

    // test 10: context usage
    combo = d_predicate_and_new(pred_greater_than_threshold,
                                 &threshold,
                                 pred_is_even,
                                 NULL);

    if (combo)
    {
        // 10 > 5 and 10 is even = true
        value  = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_context_10",
                                  d_predicate_and_eval(combo, &value) == true,
                                  "10 > 5 AND 10 is even = true") && result;

        // 7 > 5 but 7 is odd = false
        value  = 7;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_context_7",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "7 > 5 but 7 is odd = false") && result;

        // 2 is even but 2 <= 5 = false
        value  = 2;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_context_2",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "2 is even but 2 <= 5 = false") && result;

        free(combo);
    }

    // test 11: NULL element handling
    combo = d_predicate_and_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_null_element",
                                  d_predicate_and_eval(combo, NULL) == false,
                                  "NULL element should return false (predicates check for NULL)") && result;

        free(combo);
    }

    // test 12: zero boundary value (0 is even but not positive)
    combo = d_predicate_and_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        value  = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "and_eval_zero_boundary",
                                  d_predicate_and_eval(combo, &value) == false,
                                  "0 is even but not positive, so AND = false") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_and_eval
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
bool d_tests_predicate_and_eval(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "predicate_and_eval");

    result = true;
    result &= d_internal_tests_predicate_and_eval_part1(_suite);
    result &= d_internal_tests_predicate_and_eval_part2(_suite);
    result &= d_internal_tests_predicate_and_eval_part3(_suite);

    return result;
}


/**
 * @brief Part 1 of d_tests_predicate_or_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL combo should return false
 *   - test 2: NULL predicate1 should return false
 *   - test 3: NULL predicate2 should return false
 *   - test 4: both NULL predicates should return false
 *   - test 5: true OR true = true
 *   - test 6: true OR false = true (short-circuit test)
 */
static bool
d_internal_tests_predicate_or_eval_part1(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_predicate_or* combo;
    int                     value;

    result = true;

    // test 1: NULL combo should return false
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "or_eval_null_combo",
                              d_predicate_or_eval(NULL, &value) == false,
                              "NULL combo should return false") && result;

    // test 2: NULL predicate1 should return false
    combo = d_predicate_or_new(NULL, NULL, pred_always_true, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_null_predicate1",
                                  d_predicate_or_eval(combo, &value) == false,
                                  "NULL predicate1 should return false") && result;

        free(combo);
    }

    // test 3: NULL predicate2 should return false
    combo = d_predicate_or_new(pred_always_true, NULL, NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_null_predicate2",
                                  d_predicate_or_eval(combo, &value) == false,
                                  "NULL predicate2 should return false") && result;

        free(combo);
    }

    // test 4: both NULL predicates should return false
    combo = d_predicate_or_new(NULL, NULL, NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_both_null_predicates",
                                  d_predicate_or_eval(combo, &value) == false,
                                  "both NULL predicates should return false") && result;

        free(combo);
    }

    // test 5: true OR true = true
    combo = d_predicate_or_new(pred_always_true, NULL, pred_always_true, NULL);

    if (combo)
    {
        value  = 42;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_true_or_true",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "true OR true should return true") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_true_or_false",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "true OR false should return true (short-circuit)") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_predicate_or_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 7: false OR true = true
 *   - test 8: false OR false = false
 *   - test 9: even OR positive
 */
static bool
d_internal_tests_predicate_or_eval_part2(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_predicate_or* combo;
    int                     value;

    result = true;

    // test 7: false OR true = true
    combo = d_predicate_or_new(pred_always_false,
                                NULL,
                                pred_always_true,
                                NULL);

    if (combo)
    {
        value  = 42;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_false_or_true",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "false OR true should return true") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_false_or_false",
                                  d_predicate_or_eval(combo, &value) == false,
                                  "false OR false should return false") && result;

        free(combo);
    }

    // test 9: even OR positive
    combo = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        // 4 is even and positive = true
        value  = 4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_even_or_positive_4",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "4 is even OR positive = true") && result;

        // 3 is odd but positive = true
        value  = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_even_or_positive_3",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "3 is positive, so even OR positive = true") && result;

        // -4 is even but negative = true
        value  = -4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_even_or_positive_neg4",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "-4 is even, so even OR positive = true") && result;

        // -3 is odd and negative = false
        value  = -3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_even_or_positive_neg3",
                                  d_predicate_or_eval(combo, &value) == false,
                                  "-3 is odd and negative, so even OR positive = false") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 3 of d_tests_predicate_or_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 10: context usage
 *   - test 11: NULL element handling
 *   - test 12: zero boundary value (0 is even but not positive)
 */
static bool
d_internal_tests_predicate_or_eval_part3(
    struct dj_test_suite* _suite
)
{
    bool                    result;
    struct dj_predicate_or* combo;
    int                     value;
    int                     threshold;

    result    = true;
    threshold = 5;

    // test 10: context usage
    combo = d_predicate_or_new(pred_greater_than_threshold,
                                &threshold,
                                pred_is_negative,
                                NULL);

    if (combo)
    {
        // 10 > 5 = true
        value  = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_context_10",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "10 > 5, so greater_than OR negative = true") && result;

        // -2 is negative = true
        value  = -2;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_context_neg2",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "-2 is negative, so greater_than OR negative = true") && result;

        // 3 <= 5 and not negative = false
        value  = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_context_3",
                                  d_predicate_or_eval(combo, &value) == false,
                                  "3 <= 5 and not negative = false") && result;

        free(combo);
    }

    // test 11: NULL element handling
    combo = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_null_element",
                                  d_predicate_or_eval(combo, NULL) == false,
                                  "NULL element should return false (both predicates return false)") && result;

        free(combo);
    }

    // test 12: zero boundary value (0 is even but not positive)
    combo = d_predicate_or_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        value  = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "or_eval_zero_boundary",
                                  d_predicate_or_eval(combo, &value) == true,
                                  "0 is even, so even OR positive = true") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_or_eval
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
bool d_tests_predicate_or_eval(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "predicate_or_eval");

    result = true;
    result &= d_internal_tests_predicate_or_eval_part1(_suite);
    result &= d_internal_tests_predicate_or_eval_part2(_suite);
    result &= d_internal_tests_predicate_or_eval_part3(_suite);

    return result;
}


/**
 * @brief Part 1 of d_tests_predicate_xor_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL combo should return false
 *   - test 2: NULL predicate1 should return false
 *   - test 3: NULL predicate2 should return false
 *   - test 4: both NULL predicates should return false
 *   - test 5: true XOR true = false
 *   - test 6: true XOR false = true
 */
static bool
d_internal_tests_predicate_xor_eval_part1(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_xor* combo;
    int                      value;

    result = true;

    // test 1: NULL combo should return false
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "xor_eval_null_combo",
                              d_predicate_xor_eval(NULL, &value) == false,
                              "NULL combo should return false") && result;

    // test 2: NULL predicate1 should return false
    combo = d_predicate_xor_new(NULL, NULL, pred_always_true, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_null_predicate1",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "NULL predicate1 should return false") && result;

        free(combo);
    }

    // test 3: NULL predicate2 should return false
    combo = d_predicate_xor_new(pred_always_true, NULL, NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_null_predicate2",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "NULL predicate2 should return false") && result;

        free(combo);
    }

    // test 4: both NULL predicates should return false
    combo = d_predicate_xor_new(NULL, NULL, NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_both_null_predicates",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "both NULL predicates should return false") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_true_xor_true",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "true XOR true should return false") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_true_xor_false",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "true XOR false should return true") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_predicate_xor_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 7: false XOR true = true
 *   - test 8: false XOR false = false
 *   - test 9: even XOR odd (mutually exclusive)
 */
static bool
d_internal_tests_predicate_xor_eval_part2(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_xor* combo;
    int                      value;

    result = true;

    // test 7: false XOR true = true
    combo = d_predicate_xor_new(pred_always_false,
                                 NULL,
                                 pred_always_true,
                                 NULL);

    if (combo)
    {
        value  = 42;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_false_xor_true",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "false XOR true should return true") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_false_xor_false",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "false XOR false should return false") && result;

        free(combo);
    }

    // test 9: even XOR odd (mutually exclusive)
    combo = d_predicate_xor_new(pred_is_even, NULL, pred_is_odd, NULL);

    if (combo)
    {
        // even number (4): even=true, odd=false -> XOR=true
        value  = 4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_even_xor_odd_4",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "4 is even XOR odd = true") && result;

        // odd number (5): even=false, odd=true -> XOR=true
        value  = 5;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_even_xor_odd_5",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "5 is even XOR odd = true") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 3 of d_tests_predicate_xor_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 10: even XOR positive (can both be true)
 *   - test 11: context usage (greater_than_threshold XOR is_even)
 */
static bool
d_internal_tests_predicate_xor_eval_part3(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_xor* combo;
    int                      value;
    int                      threshold;

    result    = true;
    threshold = 5;

    // test 10: even XOR positive (can both be true)
    combo = d_predicate_xor_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        // 4 is both even and positive = false
        value  = 4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_even_xor_positive_4",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "4 is both even and positive, XOR = false") && result;

        // 3 is odd but positive = true
        value  = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_even_xor_positive_3",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "3 is only positive, XOR = true") && result;

        // -4 is even but negative = true
        value  = -4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_even_xor_positive_neg4",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "-4 is only even, XOR = true") && result;

        // -3 is neither = false
        value  = -3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_even_xor_positive_neg3",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "-3 is neither even nor positive, XOR = false") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_context_10",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "10 > 5 and even, XOR = false") && result;

        // 7 > 5 and 7 is odd -> only first true, XOR = true
        value  = 7;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_context_7",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "7 > 5 but odd, XOR = true") && result;

        // 2 <= 5 and 2 is even -> only second true, XOR = true
        value  = 2;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_context_2",
                                  d_predicate_xor_eval(combo, &value) == true,
                                  "2 <= 5 but even, XOR = true") && result;

        // 3 <= 5 and 3 is odd -> both false, XOR = false
        value  = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_context_3",
                                  d_predicate_xor_eval(combo, &value) == false,
                                  "3 <= 5 and odd, XOR = false") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 4 of d_tests_predicate_xor_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 12: NULL element handling
 */
static bool
d_internal_tests_predicate_xor_eval_part4(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_xor* combo;

    result = true;

    // test 12: NULL element handling
    combo = d_predicate_xor_new(pred_is_even, NULL, pred_is_positive, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "xor_eval_null_element",
                                  d_predicate_xor_eval(combo, NULL) == false,
                                  "NULL element: both return false, XOR = false") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_xor_eval
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
bool d_tests_predicate_xor_eval(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "predicate_xor_eval");

    result = true;
    result &= d_internal_tests_predicate_xor_eval_part1(_suite);
    result &= d_internal_tests_predicate_xor_eval_part2(_suite);
    result &= d_internal_tests_predicate_xor_eval_part3(_suite);
    result &= d_internal_tests_predicate_xor_eval_part4(_suite);

    return result;
}


/**
 * @brief Part 1 of d_tests_predicate_not_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL combo should return false
 *   - test 2: NULL predicate should return false
 *   - test 3: NOT true = false
 *   - test 4: NOT false = true
 *   - test 5: NOT is_even
 */
static bool
d_internal_tests_predicate_not_eval_part1(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_not* combo;
    int                      value;

    result = true;

    // test 1: NULL combo should return false
    value  = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "not_eval_null_combo",
                              d_predicate_not_eval(NULL, &value) == false,
                              "NULL combo should return false") && result;

    // test 2: NULL predicate should return false
    combo = d_predicate_not_new(NULL, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_null_predicate",
                                  d_predicate_not_eval(combo, &value) == false,
                                  "NULL predicate should return false") && result;

        free(combo);
    }

    // test 3: NOT true = false
    combo = d_predicate_not_new(pred_always_true, NULL);

    if (combo)
    {
        value  = 42;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_true",
                                  d_predicate_not_eval(combo, &value) == false,
                                  "NOT true should return false") && result;

        free(combo);
    }

    // test 4: NOT false = true
    combo = d_predicate_not_new(pred_always_false, NULL);

    if (combo)
    {
        value  = 42;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_false",
                                  d_predicate_not_eval(combo, &value) == true,
                                  "NOT false should return true") && result;

        free(combo);
    }

    // test 5: NOT is_even
    combo = d_predicate_not_new(pred_is_even, NULL);

    if (combo)
    {
        // even number (4) -> NOT even = false
        value  = 4;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_even_4",
                                  d_predicate_not_eval(combo, &value) == false,
                                  "NOT even(4) should return false") && result;

        // odd number (5) -> NOT even = true
        value  = 5;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_even_5",
                                  d_predicate_not_eval(combo, &value) == true,
                                  "NOT even(5) should return true") && result;

        free(combo);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_predicate_not_eval.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 6: NOT is_positive
 *   - test 7: context usage
 *   - test 8: NULL element handling
 */
static bool
d_internal_tests_predicate_not_eval_part2(
    struct dj_test_suite* _suite
)
{
    bool                     result;
    struct dj_predicate_not* combo;
    int                      value;
    int                      threshold;

    result    = true;
    threshold = 5;

    // test 6: NOT is_positive
    combo = d_predicate_not_new(pred_is_positive, NULL);

    if (combo)
    {
        // positive (10) -> NOT positive = false
        value  = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_positive_10",
                                  d_predicate_not_eval(combo, &value) == false,
                                  "NOT positive(10) should return false") && result;

        // negative (-5) -> NOT positive = true
        value  = -5;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_positive_neg5",
                                  d_predicate_not_eval(combo, &value) == true,
                                  "NOT positive(-5) should return true") && result;

        // zero -> NOT positive = true
        value  = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_not_positive_0",
                                  d_predicate_not_eval(combo, &value) == true,
                                  "NOT positive(0) should return true") && result;

        free(combo);
    }

    // test 7: context usage
    combo = d_predicate_not_new(pred_greater_than_threshold, &threshold);

    if (combo)
    {
        // 10 > 5 -> NOT (10 > 5) = false
        value  = 10;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_context_10",
                                  d_predicate_not_eval(combo, &value) == false,
                                  "NOT (10 > 5) should return false") && result;

        // 3 <= 5 -> NOT (3 > 5) = true
        value  = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_context_3",
                                  d_predicate_not_eval(combo, &value) == true,
                                  "NOT (3 > 5) should return true") && result;

        // 5 <= 5 -> NOT (5 > 5) = true
        value  = 5;
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_context_5",
                                  d_predicate_not_eval(combo, &value) == true,
                                  "NOT (5 > 5) should return true") && result;

        free(combo);
    }

    // test 8: NULL element handling
    combo = d_predicate_not_new(pred_is_even, NULL);

    if (combo)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "not_eval_null_element",
                                  d_predicate_not_eval(combo, NULL) == true,
                                  "NOT (pred returns false for NULL) should return true") && result;

        free(combo);
    }

    return result;
}


/*
d_tests_predicate_not_eval
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
bool d_tests_predicate_not_eval(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "predicate_not_eval");

    result = true;
    result &= d_internal_tests_predicate_not_eval_part1(_suite);
    result &= d_internal_tests_predicate_not_eval_part2(_suite);

    return result;
}


/*
d_tests_predicate_eval_all
  Aggregation function that runs all evaluation tests.
*/
bool d_tests_predicate_eval_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "predicate_eval_all");

    result = true;

    result = d_tests_predicate_and_eval(_suite) && result;
    result = d_tests_predicate_or_eval(_suite) && result;
    result = d_tests_predicate_xor_eval(_suite) && result;
    result = d_tests_predicate_not_eval(_suite) && result;

    return result;
}
