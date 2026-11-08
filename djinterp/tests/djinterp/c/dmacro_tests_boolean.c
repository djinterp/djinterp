/*******************************************************************************
* djinterp [test]                                         dmacro_tests_boolean.c
*
*
* path:      /tests/djinterp/c/dmacro_tests_boolean.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * PROBE MECHANISM TESTS
 *****************************************************************************/

/*
d_tests_dmacro_probe_check
  Tests D_PROBE, D_CHECK_IMPL, and D_CHECK macros for boolean detection.
  Tests the following:
  - D_CHECK returns 0 for non-probe arguments
  - D_CHECK returns 1 for probe arguments
  - D_PROBE creates detectable probe pattern
  - D_CHECK_IMPL correctly extracts the second argument
*/
bool d_tests_dmacro_probe_check(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_probe_check");

    all_assertions_passed = true;

    // test D_CHECK with non-probe value returns 0
    result = D_CHECK(something);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CHECK(something) == 0",
                          result == 0,
                          "non-probe should return 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_CHECK with simple token returns 0
    result = D_CHECK(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CHECK(42) == 0",
                          result == 0,
                          "number should return 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_CHECK with D_PROBE returns 1
    result = D_CHECK(D_PROBE(~));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CHECK(D_PROBE(~)) == 1",
                          result == 1,
                          "probe should return 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_CHECK_IMPL directly
    result = D_CHECK_IMPL(a, 0, );

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CHECK_IMPL(a, 0, ) == 0",
                          result == 0,
                          "CHECK_IMPL extracts second arg"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_CHECK_IMPL(a, 1, );

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CHECK_IMPL(a, 1, ) == 1",
                          result == 1,
                          "CHECK_IMPL extracts second arg"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * PARENTHESES DETECTION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_is_paren
  Tests D_IS_PAREN macro for detecting parenthesized arguments.
  Tests the following:
  - D_IS_PAREN returns 1 for parenthesized content
  - D_IS_PAREN returns 0 for non-parenthesized content
  - D_IS_PAREN works with empty parentheses
  - D_IS_PAREN works with nested parentheses
*/
bool d_tests_dmacro_is_paren(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_is_paren");

    all_assertions_passed = true;

    // test D_IS_PAREN with parenthesized content
    result = D_IS_PAREN((hello));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_PAREN((hello)) == 1",
                          result == 1,
                          "parenthesized content should return 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with non-parenthesized content
    result = D_IS_PAREN(hello);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_PAREN(hello) == 0",
                          result == 0,
                          "non-parenthesized should return 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with empty parentheses
    result = D_IS_PAREN(());

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_PAREN(()) == 1",
                          result == 1,
                          "empty parentheses should return 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with number
    result = D_IS_PAREN(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_PAREN(42) == 0",
                          result == 0,
                          "number should return 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with parenthesized number
    result = D_IS_PAREN((42));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_PAREN((42)) == 1",
                          result == 1,
                          "parenthesized number should return 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IS_PAREN with multiple items in parentheses
    result = D_IS_PAREN((a, b, c));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_PAREN((a, b, c)) == 1",
                          result == 1,
                          "parenthesized list should return 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * CONDITIONAL EXPANSION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_if_macros
  Tests D_IF, D_IF_0, D_IF_1 conditional expansion macros.
  Tests the following:
  - D_IF(1) expands the true case
  - D_IF(0) skips the true case
  - D_IF_1_ELSE discards the else clause
  - D_IF_0_ELSE expands the else clause
*/
bool d_tests_dmacro_if_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_if_macros");

    all_assertions_passed = true;

    // test D_IF(1) with true case
    result = D_IF(1)(100)(200);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IF(1)(100)(200) == 100",
                          result == 100,
                          "D_IF(1) should select true case"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IF(0) with else case
    result = D_IF(0)(100)(200);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IF(0)(100)(200) == 200",
                          result == 200,
                          "D_IF(0) should select else case"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IF_1 directly
    result = D_IF_1(42)(99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IF_1(42)(99) == 42",
                          result == 42,
                          "D_IF_1 should return true case"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IF_0 directly
    result = D_IF_0(42)(99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IF_0(42)(99) == 99",
                          result == 99,
                          "D_IF_0 should return else case"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_iif_macros
  Tests D_IIF immediate if macros.
  Tests the following:
  - D_IIF(1) selects the first argument
  - D_IIF(0) selects the second argument
  - D_IIF_0 and D_IIF_1 work directly
*/
bool d_tests_dmacro_iif_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_iif_macros");

    all_assertions_passed = true;

    // test D_IIF(1) selects first argument
    result = D_IIF(1)(100, 200);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IIF(1)(100, 200) == 100",
                          result == 100,
                          "D_IIF(1) should select first arg"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IIF(0) selects second argument
    result = D_IIF(0)(100, 200);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IIF(0)(100, 200) == 200",
                          result == 200,
                          "D_IIF(0) should select second arg"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IIF_1 directly
    result = D_IIF_1(42, 99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IIF_1(42, 99) == 42",
                          result == 42,
                          "D_IIF_1 should return first arg"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_IIF_0 directly
    result = D_IIF_0(42, 99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IIF_0(42, 99) == 99",
                          result == 99,
                          "D_IIF_0 should return second arg"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with expressions
    result = D_IIF(1)(10 + 5, 20 + 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IIF(1)(10+5, 20+5) == 15",
                          result == 15,
                          "D_IIF should work with expressions"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * BOOLEAN LOGIC TESTS
 *****************************************************************************/

/*
d_tests_dmacro_not_compl
  Tests D_NOT and D_COMPL macros.
  Tests the following:
  - D_NOT(0) returns 1
  - D_NOT(non-zero) returns 0
  - D_COMPL(0) returns 1
  - D_COMPL(1) returns 0
*/
bool d_tests_dmacro_not_compl(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_not_compl");

    all_assertions_passed = true;

    // test D_NOT(0) returns 1
    result = D_NOT(0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NOT(0) == 1",
                          result == 1,
                          "NOT of 0 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_NOT(1) returns 0
    result = D_NOT(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NOT(1) == 0",
                          result == 0,
                          "NOT of 1 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_NOT with non-zero returns 0
    result = D_NOT(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NOT(42) == 0",
                          result == 0,
                          "NOT of non-zero should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_COMPL(0) returns 1
    result = D_COMPL(0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_COMPL(0) == 1",
                          result == 1,
                          "complement of 0 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_COMPL(1) returns 0
    result = D_COMPL(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_COMPL(1) == 0",
                          result == 0,
                          "complement of 1 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test double NOT
    result = D_NOT(D_NOT(0));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NOT(D_NOT(0)) == 0",
                          result == 0,
                          "double NOT of 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test double COMPL
    result = D_COMPL(D_COMPL(0));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_COMPL(D_COMPL(0)) == 0",
                          result == 0,
                          "double complement of 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_bool_macro
  Tests D_BOOL macro for normalizing values to 0 or 1.
  Tests the following:
  - D_BOOL(0) returns 0
  - D_BOOL(1) returns 1
  - D_BOOL(non-zero) returns 1
  - D_BOOL normalizes any truthy value to 1
*/
bool d_tests_dmacro_bool_macro(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_bool_macro");

    all_assertions_passed = true;

    // test D_BOOL(0) returns 0
    result = D_BOOL(0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_BOOL(0) == 0",
                          result == 0,
                          "BOOL of 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_BOOL(1) returns 1
    result = D_BOOL(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_BOOL(1) == 1",
                          result == 1,
                          "BOOL of 1 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_BOOL with various non-zero values
    result = D_BOOL(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_BOOL(42) == 1",
                          result == 1,
                          "BOOL of 42 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_BOOL(100);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_BOOL(100) == 1",
                          result == 1,
                          "BOOL of 100 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test idempotency: D_BOOL(D_BOOL(x)) == D_BOOL(x)
    result = D_BOOL(D_BOOL(42));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_BOOL(D_BOOL(42)) == 1",
                          result == 1,
                          "BOOL is idempotent for non-zero"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_BOOL(D_BOOL(0));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_BOOL(D_BOOL(0)) == 0",
                          result == 0,
                          "BOOL is idempotent for zero"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_and_or
  Tests D_AND and D_OR logical operation macros.
  Tests the following:
  - D_AND truth table (00->0, 01->0, 10->0, 11->1)
  - D_OR truth table (00->0, 01->1, 10->1, 11->1)
  - D_AND and D_OR work with normalized inputs
*/
bool d_tests_dmacro_and_or(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_and_or");

    all_assertions_passed = true;

    // test D_AND truth table
    result = D_AND(0, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_AND(0, 0) == 0",
                          result == 0,
                          "0 AND 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(0, 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_AND(0, 1) == 0",
                          result == 0,
                          "0 AND 1 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(1, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_AND(1, 0) == 0",
                          result == 0,
                          "1 AND 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(1, 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_AND(1, 1) == 1",
                          result == 1,
                          "1 AND 1 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_OR truth table
    result = D_OR(0, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_OR(0, 0) == 0",
                          result == 0,
                          "0 OR 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_OR(0, 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_OR(0, 1) == 1",
                          result == 1,
                          "0 OR 1 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_OR(1, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_OR(1, 0) == 1",
                          result == 1,
                          "1 OR 0 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_OR(1, 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_OR(1, 1) == 1",
                          result == 1,
                          "1 OR 1 should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_AND with non-normalized inputs (uses D_BOOL internally)
    result = D_AND(42, 100);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_AND(42, 100) == 1",
                          result == 1,
                          "non-zero AND non-zero should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_AND(42, 0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_AND(42, 0) == 0",
                          result == 0,
                          "non-zero AND 0 should be 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_OR with non-normalized inputs
    result = D_OR(0, 42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_OR(0, 42) == 1",
                          result == 1,
                          "0 OR non-zero should be 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * BOOLEAN LOGIC MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_boolean_all
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
bool d_tests_dmacro_boolean_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_boolean_all");

    d_tests_dmacro_probe_check(_suite);
    d_tests_dmacro_is_paren(_suite);
    d_tests_dmacro_if_macros(_suite);
    d_tests_dmacro_iif_macros(_suite);
    d_tests_dmacro_not_compl(_suite);
    d_tests_dmacro_bool_macro(_suite);
    d_tests_dmacro_and_or(_suite);

    return true;
}
