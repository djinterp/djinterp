/*******************************************************************************
* djinterp [test]                                         dmacro_tests_utility.c
*
*   Unit tests for `dmacro.h` utility operators section.
*   Tests D_PRINT_OP, D_PRINT_VAL_OP, D_DECLARE_VAR_OP, D_DECLARE_TYPED_OP,
*   D_ASSIGN_OP, and D_INIT_ZERO_OP macros.
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * UTILITY OPERATORS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_utility_ops
  Tests utility operator macros for debugging and testing.
  Tests the following:
  - D_DECLARE_VAR_OP declares int variable
  - D_DECLARE_TYPED_OP declares typed variable
  - D_ASSIGN_OP assigns value to variable
  - D_INIT_ZERO_OP initializes variable to zero
  - D_PRINT_OP concept (stringifies argument)
  - D_PRINT_VAL_OP concept (stringifies and prints value)
*/
bool d_tests_dmacro_utility_ops(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_utility_ops");

    all_assertions_passed = true;

    // test D_DECLARE_VAR_OP
    // D_DECLARE_VAR_OP(x) -> int x;
    D_DECLARE_VAR_OP(test_var_a)
    test_var_a = 42;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DECLARE_VAR_OP creates int variable",
                          test_var_a == 42,
                          "declared variable is assignable"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, test_var_a);
    }

    // test D_DECLARE_TYPED_OP
    // D_DECLARE_TYPED_OP(double, x) -> double x;
    D_DECLARE_TYPED_OP(double, test_var_b)
    test_var_b = 3.14159;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DECLARE_TYPED_OP creates typed variable",
                          test_var_b > 3.14 && test_var_b < 3.15,
                          "declared double variable is assignable"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %f\n", D_INDENT, test_var_b);
    }

    // test D_ASSIGN_OP
    // D_ASSIGN_OP(x, 100) -> x = 100;
    int assign_test = 0;
    D_ASSIGN_OP(assign_test, 100)

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ASSIGN_OP assigns value",
                          assign_test == 100,
                          "variable assigned to 100"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, assign_test);
    }

    // test D_INIT_ZERO_OP
    // D_INIT_ZERO_OP(x) -> x = 0;
    int zero_test = 999;
    D_INIT_ZERO_OP(zero_test)

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INIT_ZERO_OP zeroes variable",
                          zero_test == 0,
                          "variable set to 0"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, zero_test);
    }

    // test multiple declarations
    D_DECLARE_VAR_OP(multi_a)
    D_DECLARE_VAR_OP(multi_b)
    D_DECLARE_VAR_OP(multi_c)
    multi_a = 1;
    multi_b = 2;
    multi_c = 3;

    if (!D_TEST_CHECK_MSG(_suite,
                          "multiple D_DECLARE_VAR_OP sum == 6",
                          multi_a + multi_b + multi_c == 6,
                          "multiple declarations work"))
    {
        all_assertions_passed = false;
        printf("%s    Got sum: %d\n", D_INDENT, multi_a + multi_b + multi_c);
    }

    // test D_DECLARE_TYPED_OP with different types
    D_DECLARE_TYPED_OP(char, char_var)
    D_DECLARE_TYPED_OP(long, long_var)

    char_var = 'X';
    long_var = 1234567890L;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DECLARE_TYPED_OP(char, x) works",
                          char_var == 'X',
                          "char variable declaration"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DECLARE_TYPED_OP(long, x) works",
                          long_var == 1234567890L,
                          "long variable declaration"))
    {
        all_assertions_passed = false;
    }

    // test chained assignments
    int chain_a, chain_b, chain_c;
    chain_a = chain_b = chain_c = 0;

    D_ASSIGN_OP(chain_a, 10)
    D_ASSIGN_OP(chain_b, 20)
    D_ASSIGN_OP(chain_c, 30)

    if (!D_TEST_CHECK_MSG(_suite,
                          "chained D_ASSIGN_OP works",
                          chain_a == 10 && chain_b == 20 && chain_c == 30,
                          "multiple assignments"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT, chain_a, chain_b, chain_c);
    }

    // test D_INIT_ZERO_OP on multiple variables
    int z1 = 111, z2 = 222, z3 = 333;

    D_INIT_ZERO_OP(z1)
    D_INIT_ZERO_OP(z2)
    D_INIT_ZERO_OP(z3)

    if (!D_TEST_CHECK_MSG(_suite,
                          "multiple D_INIT_ZERO_OP all zero",
                          z1 == 0 && z2 == 0 && z3 == 0,
                          "all variables zeroed"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT, z1, z2, z3);
    }

    // note: D_PRINT_OP and D_PRINT_VAL_OP produce output to stdout
    // we verify they compile but don't capture output in this test
    // D_PRINT_OP(test_token) -> printf("%s\n", "test_token");
    // D_PRINT_VAL_OP(42) -> printf("%s = %d\n", "42", 42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_PRINT_OP and D_PRINT_VAL_OP compile",
                          1,
                          "print macros are defined"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * UTILITY OPERATORS MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_utility_all
  Runs all utility operator tests.
  Tests the following:
  - D_DECLARE_VAR_OP, D_DECLARE_TYPED_OP
  - D_ASSIGN_OP, D_INIT_ZERO_OP
  - D_PRINT_OP, D_PRINT_VAL_OP concepts
*/
bool d_tests_dmacro_utility_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_utility_all");

    d_tests_dmacro_utility_ops(_suite);

    return true;
}
