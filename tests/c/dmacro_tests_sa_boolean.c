#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * PROBE MECHANISM TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_probe_check
  Tests D_PROBE, D_CHECK_IMPL, and D_CHECK macros for boolean detection.
  Tests the following:
  - D_CHECK returns 0 for non-probe arguments
  - D_CHECK returns 1 for probe arguments
  - D_PROBE creates detectable probe pattern
  - D_CHECK_IMPL correctly extracts the second argument
*/
bool
d_tests_sa_dmacro_probe_check
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_PROBE and D_CHECK ---\n", D_INDENT);

    // test D_CHECK with non-probe value returns 0
    result = D_CHECK(something);

    if (!d_assert_standalone(result == 0,
                             "D_CHECK(something) == 0",
                             "non-probe should return 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_CHECK with simple token returns 0
    result = D_CHECK(42);

    if (!d_assert_standalone(result == 0,
                             "D_CHECK(42) == 0",
                             "number should return 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_CHECK with D_PROBE returns 1
    result = D_CHECK(D_PROBE(~));

    if (!d_assert_standalone(result == 1,
                             "D_CHECK(D_PROBE(~)) == 1",
                             "probe should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_CHECK_IMPL directly
    result = D_CHECK_IMPL(a, 0, );

    if (!d_assert_standalone(result == 0,
                             "D_CHECK_IMPL(a, 0, ) == 0",
                             "CHECK_IMPL extracts second arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_CHECK_IMPL(a, 1, );

    if (!d_assert_standalone(result == 1,
                             "D_CHECK_IMPL(a, 1, ) == 1",
                             "CHECK_IMPL extracts second arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_PROBE and D_CHECK test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_PROBE and D_CHECK test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * PARENTHESES DETECTION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_is_paren
  Tests D_IS_PAREN macro for detecting parenthesized arguments.
  Tests the following:
  - D_IS_PAREN returns 1 for parenthesized content
  - D_IS_PAREN returns 0 for non-parenthesized content
  - D_IS_PAREN works with empty parentheses
  - D_IS_PAREN works with nested parentheses
*/
bool
d_tests_sa_dmacro_is_paren
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_IS_PAREN ---\n", D_INDENT);

    // test D_IS_PAREN with parenthesized content
    result = D_IS_PAREN((hello));

    if (!d_assert_standalone(result == 1,
                             "D_IS_PAREN((hello)) == 1",
                             "parenthesized content should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with non-parenthesized content
    result = D_IS_PAREN(hello);

    if (!d_assert_standalone(result == 0,
                             "D_IS_PAREN(hello) == 0",
                             "non-parenthesized should return 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with empty parentheses
    result = D_IS_PAREN(());

    if (!d_assert_standalone(result == 1,
                             "D_IS_PAREN(()) == 1",
                             "empty parentheses should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with number
    result = D_IS_PAREN(42);

    if (!d_assert_standalone(result == 0,
                             "D_IS_PAREN(42) == 0",
                             "number should return 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with parenthesized number
    result = D_IS_PAREN((42));

    if (!d_assert_standalone(result == 1,
                             "D_IS_PAREN((42)) == 1",
                             "parenthesized number should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with multiple items in parentheses
    result = D_IS_PAREN((a, b, c));

    if (!d_assert_standalone(result == 1,
                             "D_IS_PAREN((a, b, c)) == 1",
                             "parenthesized list should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_IS_PAREN test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_IS_PAREN test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * CONDITIONAL EXPANSION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_if_macros
  Tests D_IF, D_IF_0, D_IF_1 conditional expansion macros.
  Tests the following:
  - D_IF(1) expands the true case
  - D_IF(0) skips the true case
  - D_IF_1_ELSE discards the else clause
  - D_IF_0_ELSE expands the else clause
*/
bool
d_tests_sa_dmacro_if_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_IF Macros ---\n", D_INDENT);

    // test D_IF(1) with true case
    result = D_IF(1)(100)(200);

    if (!d_assert_standalone(result == 100,
                             "D_IF(1)(100)(200) == 100",
                             "D_IF(1) should select true case",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IF(0) with else case
    result = D_IF(0)(100)(200);

    if (!d_assert_standalone(result == 200,
                             "D_IF(0)(100)(200) == 200",
                             "D_IF(0) should select else case",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IF_1 directly
    result = D_IF_1(42)(99);

    if (!d_assert_standalone(result == 42,
                             "D_IF_1(42)(99) == 42",
                             "D_IF_1 should return true case",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IF_0 directly
    result = D_IF_0(42)(99);

    if (!d_assert_standalone(result == 99,
                             "D_IF_0(42)(99) == 99",
                             "D_IF_0 should return else case",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_IF macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_IF macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_iif_macros
  Tests D_IIF immediate if macros.
  Tests the following:
  - D_IIF(1) selects the first argument
  - D_IIF(0) selects the second argument
  - D_IIF_0 and D_IIF_1 work directly
*/
bool
d_tests_sa_dmacro_iif_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_IIF Macros ---\n", D_INDENT);

    // test D_IIF(1) selects first argument
    result = D_IIF(1)(100, 200);

    if (!d_assert_standalone(result == 100,
                             "D_IIF(1)(100, 200) == 100",
                             "D_IIF(1) should select first arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IIF(0) selects second argument
    result = D_IIF(0)(100, 200);

    if (!d_assert_standalone(result == 200,
                             "D_IIF(0)(100, 200) == 200",
                             "D_IIF(0) should select second arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IIF_1 directly
    result = D_IIF_1(42, 99);

    if (!d_assert_standalone(result == 42,
                             "D_IIF_1(42, 99) == 42",
                             "D_IIF_1 should return first arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IIF_0 directly
    result = D_IIF_0(42, 99);

    if (!d_assert_standalone(result == 99,
                             "D_IIF_0(42, 99) == 99",
                             "D_IIF_0 should return second arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with expressions
    result = D_IIF(1)(10 + 5, 20 + 5);

    if (!d_assert_standalone(result == 15,
                             "D_IIF(1)(10+5, 20+5) == 15",
                             "D_IIF should work with expressions",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_IIF macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_IIF macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * BOOLEAN LOGIC TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_not_compl
  Tests D_NOT and D_COMPL macros.
  Tests the following:
  - D_NOT(0) returns 1
  - D_NOT(non-zero) returns 0
  - D_COMPL(0) returns 1
  - D_COMPL(1) returns 0
*/
bool
d_tests_sa_dmacro_not_compl
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_NOT and D_COMPL ---\n", D_INDENT);

    // test D_NOT(0) returns 1
    result = D_NOT(0);

    if (!d_assert_standalone(result == 1,
                             "D_NOT(0) == 1",
                             "NOT of 0 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_NOT(1) returns 0
    result = D_NOT(1);

    if (!d_assert_standalone(result == 0,
                             "D_NOT(1) == 0",
                             "NOT of 1 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_NOT with non-zero returns 0
    result = D_NOT(42);

    if (!d_assert_standalone(result == 0,
                             "D_NOT(42) == 0",
                             "NOT of non-zero should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_COMPL(0) returns 1
    result = D_COMPL(0);

    if (!d_assert_standalone(result == 1,
                             "D_COMPL(0) == 1",
                             "complement of 0 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_COMPL(1) returns 0
    result = D_COMPL(1);

    if (!d_assert_standalone(result == 0,
                             "D_COMPL(1) == 0",
                             "complement of 1 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test double NOT
    result = D_NOT(D_NOT(0));

    if (!d_assert_standalone(result == 0,
                             "D_NOT(D_NOT(0)) == 0",
                             "double NOT of 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test double COMPL
    result = D_COMPL(D_COMPL(0));

    if (!d_assert_standalone(result == 0,
                             "D_COMPL(D_COMPL(0)) == 0",
                             "double complement of 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_NOT and D_COMPL test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_NOT and D_COMPL test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_bool_macro
  Tests D_BOOL macro for normalizing values to 0 or 1.
  Tests the following:
  - D_BOOL(0) returns 0
  - D_BOOL(1) returns 1
  - D_BOOL(non-zero) returns 1
  - D_BOOL normalizes any truthy value to 1
*/
bool
d_tests_sa_dmacro_bool_macro
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_BOOL ---\n", D_INDENT);

    // test D_BOOL(0) returns 0
    result = D_BOOL(0);

    if (!d_assert_standalone(result == 0,
                             "D_BOOL(0) == 0",
                             "BOOL of 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_BOOL(1) returns 1
    result = D_BOOL(1);

    if (!d_assert_standalone(result == 1,
                             "D_BOOL(1) == 1",
                             "BOOL of 1 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_BOOL with various non-zero values
    result = D_BOOL(42);

    if (!d_assert_standalone(result == 1,
                             "D_BOOL(42) == 1",
                             "BOOL of 42 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_BOOL(100);

    if (!d_assert_standalone(result == 1,
                             "D_BOOL(100) == 1",
                             "BOOL of 100 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test idempotency: D_BOOL(D_BOOL(x)) == D_BOOL(x)
    result = D_BOOL(D_BOOL(42));

    if (!d_assert_standalone(result == 1,
                             "D_BOOL(D_BOOL(42)) == 1",
                             "BOOL is idempotent for non-zero",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_BOOL(D_BOOL(0));

    if (!d_assert_standalone(result == 0,
                             "D_BOOL(D_BOOL(0)) == 0",
                             "BOOL is idempotent for zero",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_BOOL test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_BOOL test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_and_or
  Tests D_AND and D_OR logical operation macros.
  Tests the following:
  - D_AND truth table (00->0, 01->0, 10->0, 11->1)
  - D_OR truth table (00->0, 01->1, 10->1, 11->1)
  - D_AND and D_OR work with normalized inputs
*/
bool
d_tests_sa_dmacro_and_or
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_AND and D_OR ---\n", D_INDENT);

    // test D_AND truth table
    result = D_AND(0, 0);

    if (!d_assert_standalone(result == 0,
                             "D_AND(0, 0) == 0",
                             "0 AND 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(0, 1);

    if (!d_assert_standalone(result == 0,
                             "D_AND(0, 1) == 0",
                             "0 AND 1 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(1, 0);

    if (!d_assert_standalone(result == 0,
                             "D_AND(1, 0) == 0",
                             "1 AND 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(1, 1);

    if (!d_assert_standalone(result == 1,
                             "D_AND(1, 1) == 1",
                             "1 AND 1 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_OR truth table
    result = D_OR(0, 0);

    if (!d_assert_standalone(result == 0,
                             "D_OR(0, 0) == 0",
                             "0 OR 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_OR(0, 1);

    if (!d_assert_standalone(result == 1,
                             "D_OR(0, 1) == 1",
                             "0 OR 1 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_OR(1, 0);

    if (!d_assert_standalone(result == 1,
                             "D_OR(1, 0) == 1",
                             "1 OR 0 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_OR(1, 1);

    if (!d_assert_standalone(result == 1,
                             "D_OR(1, 1) == 1",
                             "1 OR 1 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_AND with non-normalized inputs (uses D_BOOL internally)
    result = D_AND(42, 100);

    if (!d_assert_standalone(result == 1,
                             "D_AND(42, 100) == 1",
                             "non-zero AND non-zero should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(42, 0);

    if (!d_assert_standalone(result == 0,
                             "D_AND(42, 0) == 0",
                             "non-zero AND 0 should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_OR with non-normalized inputs
    result = D_OR(0, 42);

    if (!d_assert_standalone(result == 1,
                             "D_OR(0, 42) == 1",
                             "0 OR non-zero should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_AND and D_OR test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_AND and D_OR test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * BOOLEAN LOGIC MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_boolean_all
  Runs all boolean and conditional logic tests.
  Tests the following sections:
  - D_PROBE and D_CHECK
  - D_IS_PAREN
  - D_IF macros
  - D_IIF macros
  - D_NOT and D_COMPL
  - D_BOOL
  - D_AND and D_OR
*/
bool
d_tests_sa_dmacro_boolean_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  probe_check_result;
    bool                  is_paren_result;
    bool                  if_macros_result;
    bool                  iif_macros_result;
    bool                  not_compl_result;
    bool                  bool_macro_result;
    bool                  and_or_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Boolean and Conditional Logic (Section V)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all boolean logic tests
    probe_check_result = d_tests_sa_dmacro_probe_check(&module_counter);
    is_paren_result    = d_tests_sa_dmacro_is_paren(&module_counter);
    if_macros_result   = d_tests_sa_dmacro_if_macros(&module_counter);
    iif_macros_result  = d_tests_sa_dmacro_iif_macros(&module_counter);
    not_compl_result   = d_tests_sa_dmacro_not_compl(&module_counter);
    bool_macro_result  = d_tests_sa_dmacro_bool_macro(&module_counter);
    and_or_result      = d_tests_sa_dmacro_and_or(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( probe_check_result &&
                       is_paren_result    &&
                       if_macros_result   &&
                       iif_macros_result  &&
                       not_compl_result   &&
                       bool_macro_result  &&
                       and_or_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Boolean/Conditional Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Boolean/Conditional Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - D_PROBE/D_CHECK:  %s\n",
               probe_check_result ? "PASSED" : "FAILED");
        printf("  - D_IS_PAREN:       %s\n",
               is_paren_result ? "PASSED" : "FAILED");
        printf("  - D_IF Macros:      %s\n",
               if_macros_result ? "PASSED" : "FAILED");
        printf("  - D_IIF Macros:     %s\n",
               iif_macros_result ? "PASSED" : "FAILED");
        printf("  - D_NOT/D_COMPL:    %s\n",
               not_compl_result ? "PASSED" : "FAILED");
        printf("  - D_BOOL:           %s\n",
               bool_macro_result ? "PASSED" : "FAILED");
        printf("  - D_AND/D_OR:       %s\n",
               and_or_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
