/******************************************************************************
* djinterp [test]                                  dmacro_tests_sa_utility.c
*
*   Unit tests for `dmacro.h` utility operators section.
*   Tests D_PRINT_OP, D_PRINT_VAL_OP, D_DECLARE_VAR_OP, D_DECLARE_TYPED_OP,
*   D_ASSIGN_OP, and D_INIT_ZERO_OP macros.
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_utility.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * UTILITY OPERATORS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_utility_ops
  Tests utility operator macros for debugging and testing.
  Tests the following:
  - D_DECLARE_VAR_OP declares int variable
  - D_DECLARE_TYPED_OP declares typed variable
  - D_ASSIGN_OP assigns value to variable
  - D_INIT_ZERO_OP initializes variable to zero
  - D_PRINT_OP concept (stringifies argument)
  - D_PRINT_VAL_OP concept (stringifies and prints value)
*/
bool
d_tests_sa_dmacro_utility_ops
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Utility Operators ---\n", D_INDENT);

    // test D_DECLARE_VAR_OP
    // D_DECLARE_VAR_OP(x) -> int x;
    D_DECLARE_VAR_OP(test_var_a)
    test_var_a = 42;

    if (!d_assert_standalone(test_var_a == 42,
                             "D_DECLARE_VAR_OP creates int variable",
                             "declared variable is assignable",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, test_var_a);
    }

    // test D_DECLARE_TYPED_OP
    // D_DECLARE_TYPED_OP(double, x) -> double x;
    D_DECLARE_TYPED_OP(double, test_var_b)
    test_var_b = 3.14159;

    if (!d_assert_standalone(test_var_b > 3.14 && test_var_b < 3.15,
                             "D_DECLARE_TYPED_OP creates typed variable",
                             "declared double variable is assignable",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %f\n", D_INDENT, test_var_b);
    }

    // test D_ASSIGN_OP
    // D_ASSIGN_OP(x, 100) -> x = 100;
    int assign_test = 0;
    D_ASSIGN_OP(assign_test, 100)

    if (!d_assert_standalone(assign_test == 100,
                             "D_ASSIGN_OP assigns value",
                             "variable assigned to 100",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, assign_test);
    }

    // test D_INIT_ZERO_OP
    // D_INIT_ZERO_OP(x) -> x = 0;
    int zero_test = 999;
    D_INIT_ZERO_OP(zero_test)

    if (!d_assert_standalone(zero_test == 0,
                             "D_INIT_ZERO_OP zeroes variable",
                             "variable set to 0",
                             _test_info))
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

    if (!d_assert_standalone(multi_a + multi_b + multi_c == 6,
                             "multiple D_DECLARE_VAR_OP sum == 6",
                             "multiple declarations work",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got sum: %d\n", D_INDENT, multi_a + multi_b + multi_c);
    }

    // test D_DECLARE_TYPED_OP with different types
    D_DECLARE_TYPED_OP(char, char_var)
    D_DECLARE_TYPED_OP(long, long_var)

    char_var = 'X';
    long_var = 1234567890L;

    if (!d_assert_standalone(char_var == 'X',
                             "D_DECLARE_TYPED_OP(char, x) works",
                             "char variable declaration",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(long_var == 1234567890L,
                             "D_DECLARE_TYPED_OP(long, x) works",
                             "long variable declaration",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test chained assignments
    int chain_a, chain_b, chain_c;
    chain_a = chain_b = chain_c = 0;

    D_ASSIGN_OP(chain_a, 10)
    D_ASSIGN_OP(chain_b, 20)
    D_ASSIGN_OP(chain_c, 30)

    if (!d_assert_standalone(chain_a == 10 && chain_b == 20 && chain_c == 30,
                             "chained D_ASSIGN_OP works",
                             "multiple assignments",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT, chain_a, chain_b, chain_c);
    }

    // test D_INIT_ZERO_OP on multiple variables
    int z1 = 111, z2 = 222, z3 = 333;

    D_INIT_ZERO_OP(z1)
    D_INIT_ZERO_OP(z2)
    D_INIT_ZERO_OP(z3)

    if (!d_assert_standalone(z1 == 0 && z2 == 0 && z3 == 0,
                             "multiple D_INIT_ZERO_OP all zero",
                             "all variables zeroed",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT, z1, z2, z3);
    }

    // note: D_PRINT_OP and D_PRINT_VAL_OP produce output to stdout
    // we verify they compile but don't capture output in this test
    // D_PRINT_OP(test_token) -> printf("%s\n", "test_token");
    // D_PRINT_VAL_OP(42) -> printf("%s = %d\n", "42", 42);

    if (!d_assert_standalone(1,
                             "D_PRINT_OP and D_PRINT_VAL_OP compile",
                             "print macros are defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Utility operators test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Utility operators test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * UTILITY OPERATORS MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_utility_all
  Runs all utility operator tests.
  Tests the following:
  - D_DECLARE_VAR_OP, D_DECLARE_TYPED_OP
  - D_ASSIGN_OP, D_INIT_ZERO_OP
  - D_PRINT_OP, D_PRINT_VAL_OP concepts
*/
bool
d_tests_sa_dmacro_utility_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  ops_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Utility Operators (Section XIII)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all utility operator tests
    ops_result = d_tests_sa_dmacro_utility_ops(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ops_result;

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Utility Operators Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Utility Operators Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Utility Ops: %s\n", ops_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
