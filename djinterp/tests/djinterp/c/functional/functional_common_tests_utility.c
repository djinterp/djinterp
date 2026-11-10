/*******************************************************************************
* djinterp [functional]                        functional_common_tests_utility.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_fc_identity_functions
  Tests identity transformer and predicate functions.
  Tests the following:
  - d_functional_identity_transformer copies an element of the size its
    context holds, and refuses NULL input, output or context
  - d_functional_identity_predicate reads a bool element; NULL is false
*/
bool d_tests_fc_identity_functions(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    value;
    int    output;
    size_t size;
    bool   flag;

    d_test_suite_test(_suite, "fc_identity_functions");

    result = true;
    value  = 42;
    output = 0;
    size   = sizeof(value);
    flag   = true;

    // test 1: identity transformer with valid input/output and size
    result = D_TEST_CHECK_MSG(_suite,
                              "identity_transformer_valid",
                              d_functional_identity_transformer(&value,
                                                                &output,
                                                                &size),
                              "should succeed given a size") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "identity_transformer_copy",
                              output == value,
                              "Output should hold the copied value") && result;

    // test 2: identity transformer with NULL input, output or size
    result = D_TEST_CHECK_MSG(_suite,
                              "identity_transformer_null_input",
                              !d_functional_identity_transformer(NULL,
                                                                 &output,
                                                                 &size),
                              "NULL input should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "identity_transformer_null_output",
                              !d_functional_identity_transformer(&value,
                                                                 NULL,
                                                                 &size),
                              "NULL output should return false") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "identity_transformer_null_size",
                              !d_functional_identity_transformer(&value,
                                                                 &output,
                                                                 NULL),
                              "no size should return false") && result;

    // test 3: identity predicate with a true element
    result = D_TEST_CHECK_MSG(_suite,
                              "identity_predicate_true",
                              d_functional_identity_predicate(&flag, NULL),
                              "A true element should return true") && result;

    // test 4: identity predicate with NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "identity_predicate_null",
                              !d_functional_identity_predicate(NULL, NULL),
                              "NULL element should return false") && result;

    return result;
}


/*
d_tests_fc_constant_functions
  Tests constant true/false functions.
  Tests the following:
  - d_functional_constant_true always returns true
  - d_functional_constant_false always returns false
  - NULL parameters handled correctly
*/
bool d_tests_fc_constant_functions(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  value;

    d_test_suite_test(_suite, "fc_constant_functions");

    result = true;
    value  = 42;

    // test 1: constant_true with non-NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "constant_true_non_null",
                              d_functional_constant_true(&value, NULL) == true,
                              "constant_true should return true with non-NULL") && result;

    // test 2: constant_true with NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "constant_true_null",
                              d_functional_constant_true(NULL, NULL) == true,
                              "constant_true should return true with NULL") && result;

    // test 3: constant_false with non-NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "constant_false_non_null",
                              d_functional_constant_false(&value, NULL) == false,
                              "constant_false should return false with non-NULL") && result;

    // test 4: constant_false with NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "constant_false_null",
                              d_functional_constant_false(NULL, NULL) == false,
                              "constant_false should return false with NULL") && result;

    return result;
}


/**
 * @brief Part 1 of d_tests_fc_comparison_utilities.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: compare_int - less than
 *   - test 2: compare_int - equal
 *   - test 3: compare_int - greater than
 *   - test 4: compare_int with NULL: a missing operand sorts first
 *   - test 5: compare_size_t - less than
 *   - test 6: compare_size_t - equal
 *   - test 7: compare_double - greater than
 *   - test 8: compare_double - less than
 *   - test 9: equal_int - true
 *   - test 10: equal_int - false
 *   - test 11: equal_int with NULL
 */
static bool
d_internal_tests_fc_comparison_utilities_part1(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    int_a;
    int    int_b;
    size_t size_a;
    size_t size_b;
    double double_a;
    double double_b;

    result   = true;
    int_a    = 5;
    int_b    = 10;
    size_a   = 100;
    size_b   = 200;
    double_a = 3.14;
    double_b = 2.71;

    // test 1: compare_int - less than
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_int_less_than",
                              d_functional_compare_int(&int_a, &int_b, NULL) < 0,
                              "5 < 10 should return negative") && result;

    // test 2: compare_int - equal
    int_b  = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_int_equal",
                              d_functional_compare_int(&int_a, &int_b, NULL) == 0,
                              "5 == 5 should return 0") && result;

    // test 3: compare_int - greater than
    int_b  = 3;
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_int_greater_than",
                              d_functional_compare_int(&int_a, &int_b, NULL) > 0,
                              "5 > 3 should return positive") && result;

    // test 4: compare_int with NULL: a missing operand sorts first
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_int_null_a",
                              d_functional_compare_int(NULL, &int_b, NULL) < 0,
                              "NULL first param should sort first") && result;

    // test 5: compare_size_t - less than
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_size_t_less_than",
                              d_functional_compare_size_t(&size_a, &size_b, NULL) < 0,
                              "100 < 200 should return negative") && result;

    // test 6: compare_size_t - equal
    size_b = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_size_t_equal",
                              d_functional_compare_size_t(&size_a, &size_b, NULL) == 0,
                              "100 == 100 should return 0") && result;

    // test 7: compare_double - greater than
    result = D_TEST_CHECK_MSG(_suite,
                              "compare_double_greater_than",
                              d_functional_compare_double(&double_a, &double_b, NULL) > 0,
                              "3.14 > 2.71 should return positive") && result;

    // test 8: compare_double - less than
    double_a = 1.5;
    double_b = 2.71;
    result   = D_TEST_CHECK_MSG(_suite,
                                "compare_double_less_than",
                                d_functional_compare_double(&double_a, &double_b, NULL) < 0,
                                "1.5 < 2.71 should return negative") && result;

    // test 9: equal_int - true
    int_a  = 42;
    int_b  = 42;
    result = D_TEST_CHECK_MSG(_suite,
                              "equal_int_true",
                              d_functional_equal_int(&int_a, &int_b, NULL) == true,
                              "42 == 42 should return true") && result;

    // test 10: equal_int - false
    int_b  = 43;
    result = D_TEST_CHECK_MSG(_suite,
                              "equal_int_false",
                              d_functional_equal_int(&int_a, &int_b, NULL) == false,
                              "42 != 43 should return false") && result;

    // test 11: equal_int with NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "equal_int_null",
                              d_functional_equal_int(NULL, &int_b, NULL) == false,
                              "NULL param should return false") && result;

    return result;
}


/**
 * @brief Part 2 of d_tests_fc_comparison_utilities.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 12: equal_size_t - true
 *   - test 13: equal_size_t - false
 */
static bool
d_internal_tests_fc_comparison_utilities_part2(
    struct dj_test_suite* _suite
)
{
    bool   result;
    size_t size_a;
    size_t size_b;

    result = true;
    size_a = 100;
    size_b = 200;

    // test 12: equal_size_t - true
    size_a = 1000;
    size_b = 1000;
    result = D_TEST_CHECK_MSG(_suite,
                              "equal_size_t_true",
                              d_functional_equal_size_t(&size_a, &size_b, NULL) == true,
                              "1000 == 1000 should return true") && result;

    // test 13: equal_size_t - false
    size_b = 999;
    result = D_TEST_CHECK_MSG(_suite,
                              "equal_size_t_false",
                              d_functional_equal_size_t(&size_a, &size_b, NULL) == false,
                              "1000 != 999 should return false") && result;

    return result;
}


/*
d_tests_fc_comparison_utilities
  Tests comparison and equality functions.
  Tests the following:
  - int comparison (less than, equal, greater than)
  - size_t comparison
  - double comparison
  - int equality
  - size_t equality
  - NULL parameter handling
*/
bool d_tests_fc_comparison_utilities(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "fc_comparison_utilities");

    result = true;
    result &= d_internal_tests_fc_comparison_utilities_part1(_suite);
    result &= d_internal_tests_fc_comparison_utilities_part2(_suite);

    return result;
}


/*
d_tests_fc_predicate_utilities
  Tests null-checking predicates.
  Tests the following:
  - d_functional_is_null with NULL pointer
  - d_functional_is_null with non-NULL pointer
  - d_functional_is_null with NULL element
  - d_functional_is_not_null with non-NULL pointer
  - d_functional_is_not_null with NULL pointer
  - d_functional_is_not_null with NULL element
*/
bool d_tests_fc_predicate_utilities(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  null_ptr;
    void*  non_null_ptr;
    int    value;

    d_test_suite_test(_suite, "fc_predicate_utilities");

    result      = true;
    null_ptr    = NULL;
    value       = 42;
    non_null_ptr = &value;

    // test 1: is_null with NULL pointer
    result = D_TEST_CHECK_MSG(_suite,
                              "is_null_with_null_ptr",
                              d_functional_is_null(&null_ptr, NULL) == true,
                              "Pointer to NULL should return true") && result;

    // test 2: is_null with non-NULL pointer
    result = D_TEST_CHECK_MSG(_suite,
                              "is_null_with_non_null_ptr",
                              d_functional_is_null(&non_null_ptr, NULL) == false,
                              "Pointer to non-NULL should return false") && result;

    // test 3: is_null with NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "is_null_with_null_element",
                              d_functional_is_null(NULL, NULL) == true,
                              "NULL element should return true") && result;

    // test 4: is_not_null with non-NULL pointer
    result = D_TEST_CHECK_MSG(_suite,
                              "is_not_null_with_non_null_ptr",
                              d_functional_is_not_null(&non_null_ptr, NULL) == true,
                              "Pointer to non-NULL should return true") && result;

    // test 5: is_not_null with NULL pointer
    result = D_TEST_CHECK_MSG(_suite,
                              "is_not_null_with_null_ptr",
                              d_functional_is_not_null(&null_ptr, NULL) == false,
                              "Pointer to NULL should return false") && result;

    // test 6: is_not_null with NULL element
    result = D_TEST_CHECK_MSG(_suite,
                              "is_not_null_with_null_element",
                              d_functional_is_not_null(NULL, NULL) == false,
                              "NULL element should return false") && result;

    return result;
}


/*
d_tests_fc_utility_all
  Aggregation function that runs all utility function tests.
*/
bool d_tests_fc_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "fc_utility_all");

    result = true;

    result = d_tests_fc_identity_functions(_suite) && result;
    result = d_tests_fc_constant_functions(_suite) && result;
    result = d_tests_fc_comparison_utilities(_suite) && result;
    result = d_tests_fc_predicate_utilities(_suite) && result;

    return result;
}
