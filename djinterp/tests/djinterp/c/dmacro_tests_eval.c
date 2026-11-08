/*******************************************************************************
* djinterp [test]                                            dmacro_tests_eval.c
*
*   Unit tests for `dmacro.h` macro expansion and evaluation (Section III).
*   Tests D_INC, D_EVAL, and iteration macro results.
*
*   Note: Tests avoid passing custom macro names to D_FOR_EACH variants due to
*   MSVC traditional preprocessor limitations. Instead, we test the results
*   of these macros using built-in operators or direct value checking.
*
*
* path:      /tests/djinterp/c/dmacro_tests_eval.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <string.h>
// djinterp
#include "dmacro_tests.h"


/******************************************************************************
 * D_INC TESTS
 *****************************************************************************/

/*
d_tests_dmacro_inc_basic
  Tests basic D_INC functionality for incrementing numbers.
  Tests the following:
  - D_INC(0) returns 1
  - D_INC(1) returns 2
  - D_INC(5) returns 6
  - D_INC(9) returns 10
  - D_INC(10) returns 11
*/
bool d_tests_dmacro_inc_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_inc_basic");

    all_assertions_passed = true;

    // test D_INC(0)
    result = D_INC(0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(0) == 1",
                          result == 1,
                          "incrementing 0 should give 1"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(1)
    result = D_INC(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(1) == 2",
                          result == 2,
                          "incrementing 1 should give 2"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(5)
    result = D_INC(5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(5) == 6",
                          result == 6,
                          "incrementing 5 should give 6"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(9) - single to double digit boundary
    result = D_INC(9);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(9) == 10",
                          result == 10,
                          "incrementing 9 should give 10"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(10)
    result = D_INC(10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(10) == 11",
                          result == 11,
                          "incrementing 10 should give 11"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_inc_medium
  Tests D_INC with medium-range numbers (11-32).
  Tests the following:
  - D_INC works correctly in the teens
  - D_INC works correctly in the twenties
  - D_INC works at power-of-2 boundaries (15->16, 31->32)
*/
bool d_tests_dmacro_inc_medium(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_inc_medium");

    all_assertions_passed = true;

    // test D_INC(15) - power of 2 minus 1
    result = D_INC(15);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(15) == 16",
                          result == 16,
                          "incrementing 15 should give 16"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(16)
    result = D_INC(16);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(16) == 17",
                          result == 17,
                          "incrementing 16 should give 17"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(20)
    result = D_INC(20);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(20) == 21",
                          result == 21,
                          "incrementing 20 should give 21"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(31) - power of 2 minus 1
    result = D_INC(31);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(31) == 32",
                          result == 32,
                          "incrementing 31 should give 32"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(32)
    result = D_INC(32);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(32) == 33",
                          result == 33,
                          "incrementing 32 should give 33"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_inc_large
  Tests D_INC with large numbers (33-63).
  Tests the following:
  - D_INC works correctly in the 40s and 50s
  - D_INC works at variant boundary (63->64)
  - D_INC is consistent across the full range
*/
bool d_tests_dmacro_inc_large(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_inc_large");

    all_assertions_passed = true;

    // test D_INC(40)
    result = D_INC(40);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(40) == 41",
                          result == 41,
                          "incrementing 40 should give 41"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(50)
    result = D_INC(50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(50) == 51",
                          result == 51,
                          "incrementing 50 should give 51"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(62)
    result = D_INC(62);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(62) == 63",
                          result == 63,
                          "incrementing 62 should give 63"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(63) - boundary for 64 variant
    result = D_INC(63);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(63) == 64",
                          result == 64,
                          "incrementing 63 should give 64"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_inc_chained
  Tests chained/nested D_INC calls.
  Tests the following:
  - D_INC(D_INC(0)) gives 2
  - D_INC(D_INC(D_INC(0))) gives 3
  - Multiple increments produce correct accumulated result
  - D_INC result usable in expressions
*/
bool d_tests_dmacro_inc_chained(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_inc_chained");

    all_assertions_passed = true;

    // test double increment: D_INC(D_INC(0))
    result = D_INC(D_INC(0));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(D_INC(0)) == 2",
                          result == 2,
                          "double increment of 0 should give 2"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test triple increment: D_INC(D_INC(D_INC(0)))
    result = D_INC(D_INC(D_INC(0)));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(D_INC(D_INC(0))) == 3",
                          result == 3,
                          "triple increment of 0 should give 3"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test quadruple increment
    result = D_INC(D_INC(D_INC(D_INC(0))));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC^4(0) == 4",
                          result == 4,
                          "quadruple increment of 0 should give 4"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test chained from non-zero base
    result = D_INC(D_INC(D_INC(10)));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(D_INC(D_INC(10))) == 13",
                          result == 13,
                          "triple increment of 10 should give 13"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC result in arithmetic expression
    result = D_INC(5) + D_INC(10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(5) + D_INC(10) == 17",
                          result == 17,
                          "6 + 11 should equal 17"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC in multiplication
    result = D_INC(3) * D_INC(4);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(3) * D_INC(4) == 20",
                          result == 20,
                          "4 * 5 should equal 20"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_EVAL TESTS
 *****************************************************************************/

/*
d_tests_dmacro_eval_basic
  Tests basic D_EVAL functionality for forcing macro expansion.
  Tests the following:
  - D_EVAL passes through simple values unchanged
  - D_EVAL forces expansion of macro arguments
  - D_EVAL works with expressions
*/
bool d_tests_dmacro_eval_basic(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    int         result;
    const char* str_result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_eval_basic");

    all_assertions_passed = true;

    // test D_EVAL with simple integer
    result = D_EVAL(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(42) == 42",
                          result == 42,
                          "D_EVAL should pass through integer"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with expression
    result = D_EVAL(10 + 20);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(10 + 20) == 30",
                          result == 30,
                          "D_EVAL should evaluate expression"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with macro result
    result = D_EVAL(D_INC(5));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(D_INC(5)) == 6",
                          result == 6,
                          "D_EVAL should expand D_INC"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with string
    str_result = D_EVAL("hello");

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(\"hello\") == \"hello\"",
                          strcmp(str_result, "hello") == 0,
                          "D_EVAL should pass through string"))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_result);
    }

    // test D_EVAL with D_VARG_COUNT
    result = D_EVAL(D_VARG_COUNT(1, 2, 3));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(D_VARG_COUNT(1,2,3)) == 3",
                          result == 3,
                          "D_EVAL should expand D_VARG_COUNT"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_eval_nested
  Tests nested D_EVAL calls and multiple expansion passes.
  Tests the following:
  - D_EVAL(D_EVAL(x)) works correctly
  - Multiple evaluation passes don't change stable results
  - D_EVAL can force expansion of deferred macros
*/
bool d_tests_dmacro_eval_nested(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_eval_nested");

    all_assertions_passed = true;

    // test double D_EVAL
    result = D_EVAL(D_EVAL(100));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(D_EVAL(100)) == 100",
                          result == 100,
                          "double D_EVAL should be stable"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test triple D_EVAL
    result = D_EVAL(D_EVAL(D_EVAL(42)));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(D_EVAL(D_EVAL(42))) == 42",
                          result == 42,
                          "triple D_EVAL should be stable"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test nested D_EVAL with macro expansion
    result = D_EVAL(D_EVAL(D_INC(D_INC(0))));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(D_EVAL(D_INC(D_INC(0)))) == 2",
                          result == 2,
                          "nested eval with nested inc"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with complex expression
    result = D_EVAL(D_INC(5) + D_INC(9));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EVAL(D_INC(5) + D_INC(9)) == 16",
                          result == 16,
                          "D_EVAL with complex expression (6+10)"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * ITERATION MACRO TESTS (MSVC-compatible)
 *****************************************************************************/

/*
d_tests_dmacro_for_each_stringify
  Tests D_FOR_EACH_COMMA with D_STRINGIFY (built-in operator).
  This tests iteration functionality using only built-in dmacro operators
  to avoid MSVC traditional preprocessor issues with custom operators.
*/
bool d_tests_dmacro_for_each_stringify(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* strings[3];
    size_t      count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_stringify");

    all_assertions_passed = true;

    // test D_FOR_EACH_COMMA with D_STRINGIFY to create string array
    {
        const char* test_arr[] = { D_FOR_EACH_COMMA(D_STRINGIFY, alpha, beta, gamma) };
        count = sizeof(test_arr) / sizeof(test_arr[0]);

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_FOR_EACH_COMMA(D_STRINGIFY,...) creates 3 strings",
                              count == 3,
                              "should create array of 3 stringified identifiers"))
        {
            all_assertions_passed = false;
            printf("%s    Got count: %zu\n", D_INDENT, count);
        }

        strings[0] = test_arr[0];
        strings[1] = test_arr[1];
        strings[2] = test_arr[2];

        if (!D_TEST_CHECK_MSG(_suite,
                              "first string == \"alpha\"",
                              strcmp(strings[0], "alpha") == 0,
                              "D_STRINGIFY should convert alpha to \"alpha\""))
        {
            all_assertions_passed = false;
            printf("%s    Got: \"%s\"\n", D_INDENT, strings[0]);
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "second string == \"beta\"",
                              strcmp(strings[1], "beta") == 0,
                              "D_STRINGIFY should convert beta to \"beta\""))
        {
            all_assertions_passed = false;
            printf("%s    Got: \"%s\"\n", D_INDENT, strings[1]);
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "third string == \"gamma\"",
                              strcmp(strings[2], "gamma") == 0,
                              "D_STRINGIFY should convert gamma to \"gamma\""))
        {
            all_assertions_passed = false;
            printf("%s    Got: \"%s\"\n", D_INDENT, strings[2]);
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
 * PRACTICAL EVALUATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_eval_practical
  Tests practical usage patterns of evaluation macros.
  Tests the following:
  - Using D_INC in compile-time patterns
  - Using D_EVAL for deferred expansion
  - Combining D_INC with D_VARG_COUNT
*/
bool d_tests_dmacro_eval_practical(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;
    int    sequence[5];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_eval_practical");

    all_assertions_passed = true;

    // pattern: generating a sequence with D_INC
    sequence[0] = 0;
    sequence[1] = D_INC(0);
    sequence[2] = D_INC(D_INC(0));
    sequence[3] = D_INC(D_INC(D_INC(0)));
    sequence[4] = D_INC(D_INC(D_INC(D_INC(0))));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC sequence generates 0,1,2,3,4",
                          sequence[0] == 0 && sequence[1] == 1 && sequence[2] == 2 && sequence[3] == 3 && sequence[4] == 4,
                          "D_INC can generate sequences"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d,%d,%d,%d,%d\n", D_INDENT,
               sequence[0], sequence[1], sequence[2],
               sequence[3], sequence[4]);
    }

    // pattern: chained D_INC to compute value
    // equivalent to adding 5 to 10: INC^5(10) = 15
    result = D_INC(D_INC(D_INC(D_INC(D_INC(10)))));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC^5(10) == 15",
                          result == 15,
                          "chained increment pattern"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // pattern: using D_VARG_COUNT with D_INC
    result = D_INC(D_VARG_COUNT(1, 2, 3, 4, 5));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INC(D_VARG_COUNT(5 args)) == 6",
                          result == 6,
                          "combining counting with increment"))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // pattern: conditional computation simulation
    {
        int has = D_HAS_ARGS(1, 2);
        result  = has * D_INC(20);

        if (!D_TEST_CHECK_MSG(_suite,
                              "HAS_ARGS guard with D_INC == 21",
                              result == 21,
                              "conditional pattern (1 * inc(20))"))
        {
            all_assertions_passed = false;
            printf("%s    Got: %d\n", D_INDENT, result);
        }
    }

    // pattern: computing array bounds
    {
        int arr[D_INC(D_INC(D_INC(0)))];  // arr[3]
        (void)arr;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_INC chain for array size == 3",
                              sizeof(arr) / sizeof(arr[0]) == 3,
                              "D_INC usable in array declaration"))
        {
            all_assertions_passed = false;
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
 * MACRO EXPANSION AND EVALUATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_eval_all
  Runs all macro expansion and evaluation tests.
  Tests the following sections:
  - D_INC (basic, medium, large, chained)
  - D_EVAL (basic, nested)
  - Iteration macros (D_FOR_EACH_COMMA with D_STRINGIFY)
  - Practical patterns
*/
bool d_tests_dmacro_eval_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_eval_all");

    d_tests_dmacro_inc_basic(_suite);
    d_tests_dmacro_inc_medium(_suite);
    d_tests_dmacro_inc_large(_suite);
    d_tests_dmacro_inc_chained(_suite);
    d_tests_dmacro_eval_basic(_suite);
    d_tests_dmacro_eval_nested(_suite);
    d_tests_dmacro_for_each_stringify(_suite);
    d_tests_dmacro_eval_practical(_suite);

    return true;
}
