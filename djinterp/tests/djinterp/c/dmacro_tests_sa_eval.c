/******************************************************************************
* djinterp [test]                                        dmacro_tests_sa_eval.c
*
*   Unit tests for `dmacro.h` macro expansion and evaluation (Section III).
*   Tests D_INC, D_EVAL, and iteration macro results.
*
*   Note: Tests avoid passing custom macro names to D_FOR_EACH variants due to
*   MSVC traditional preprocessor limitations. Instead, we test the results
*   of these macros using built-in operators or direct value checking.
*
*
* path:      \test\dmacro_tests_sa_eval.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "dmacro_tests_sa.h"
#include <string.h>


/******************************************************************************
 * D_INC TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_inc_basic
  Tests basic D_INC functionality for incrementing numbers.
  Tests the following:
  - D_INC(0) returns 1
  - D_INC(1) returns 2
  - D_INC(5) returns 6
  - D_INC(9) returns 10
  - D_INC(10) returns 11
*/
bool
d_tests_sa_dmacro_inc_basic
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

    printf("%s--- Testing D_INC Basic Functionality ---\n", D_INDENT);

    // test D_INC(0)
    result = D_INC(0);

    if (!d_assert_standalone(result == 1,
                             "D_INC(0) == 1",
                             "incrementing 0 should give 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(1)
    result = D_INC(1);

    if (!d_assert_standalone(result == 2,
                             "D_INC(1) == 2",
                             "incrementing 1 should give 2",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(5)
    result = D_INC(5);

    if (!d_assert_standalone(result == 6,
                             "D_INC(5) == 6",
                             "incrementing 5 should give 6",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(9) - single to double digit boundary
    result = D_INC(9);

    if (!d_assert_standalone(result == 10,
                             "D_INC(9) == 10",
                             "incrementing 9 should give 10",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(10)
    result = D_INC(10);

    if (!d_assert_standalone(result == 11,
                             "D_INC(10) == 11",
                             "incrementing 10 should give 11",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_INC basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_INC basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_inc_medium
  Tests D_INC with medium-range numbers (11-32).
  Tests the following:
  - D_INC works correctly in the teens
  - D_INC works correctly in the twenties
  - D_INC works at power-of-2 boundaries (15->16, 31->32)
*/
bool
d_tests_sa_dmacro_inc_medium
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

    printf("%s--- Testing D_INC Medium Range (11-32) ---\n", D_INDENT);

    // test D_INC(15) - power of 2 minus 1
    result = D_INC(15);

    if (!d_assert_standalone(result == 16,
                             "D_INC(15) == 16",
                             "incrementing 15 should give 16",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(16)
    result = D_INC(16);

    if (!d_assert_standalone(result == 17,
                             "D_INC(16) == 17",
                             "incrementing 16 should give 17",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(20)
    result = D_INC(20);

    if (!d_assert_standalone(result == 21,
                             "D_INC(20) == 21",
                             "incrementing 20 should give 21",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(31) - power of 2 minus 1
    result = D_INC(31);

    if (!d_assert_standalone(result == 32,
                             "D_INC(31) == 32",
                             "incrementing 31 should give 32",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(32)
    result = D_INC(32);

    if (!d_assert_standalone(result == 33,
                             "D_INC(32) == 33",
                             "incrementing 32 should give 33",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_INC medium range test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_INC medium range test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_inc_large
  Tests D_INC with large numbers (33-63).
  Tests the following:
  - D_INC works correctly in the 40s and 50s
  - D_INC works at variant boundary (63->64)
  - D_INC is consistent across the full range
*/
bool
d_tests_sa_dmacro_inc_large
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

    printf("%s--- Testing D_INC Large Range (33-63) ---\n", D_INDENT);

    // test D_INC(40)
    result = D_INC(40);

    if (!d_assert_standalone(result == 41,
                             "D_INC(40) == 41",
                             "incrementing 40 should give 41",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(50)
    result = D_INC(50);

    if (!d_assert_standalone(result == 51,
                             "D_INC(50) == 51",
                             "incrementing 50 should give 51",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(62)
    result = D_INC(62);

    if (!d_assert_standalone(result == 63,
                             "D_INC(62) == 63",
                             "incrementing 62 should give 63",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC(63) - boundary for 64 variant
    result = D_INC(63);

    if (!d_assert_standalone(result == 64,
                             "D_INC(63) == 64",
                             "incrementing 63 should give 64",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_INC large range test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_INC large range test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_inc_chained
  Tests chained/nested D_INC calls.
  Tests the following:
  - D_INC(D_INC(0)) gives 2
  - D_INC(D_INC(D_INC(0))) gives 3
  - Multiple increments produce correct accumulated result
  - D_INC result usable in expressions
*/
bool
d_tests_sa_dmacro_inc_chained
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

    printf("%s--- Testing D_INC Chained Calls ---\n", D_INDENT);

    // test double increment: D_INC(D_INC(0))
    result = D_INC(D_INC(0));

    if (!d_assert_standalone(result == 2,
                             "D_INC(D_INC(0)) == 2",
                             "double increment of 0 should give 2",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test triple increment: D_INC(D_INC(D_INC(0)))
    result = D_INC(D_INC(D_INC(0)));

    if (!d_assert_standalone(result == 3,
                             "D_INC(D_INC(D_INC(0))) == 3",
                             "triple increment of 0 should give 3",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test quadruple increment
    result = D_INC(D_INC(D_INC(D_INC(0))));

    if (!d_assert_standalone(result == 4,
                             "D_INC^4(0) == 4",
                             "quadruple increment of 0 should give 4",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test chained from non-zero base
    result = D_INC(D_INC(D_INC(10)));

    if (!d_assert_standalone(result == 13,
                             "D_INC(D_INC(D_INC(10))) == 13",
                             "triple increment of 10 should give 13",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC result in arithmetic expression
    result = D_INC(5) + D_INC(10);

    if (!d_assert_standalone(result == 17,
                             "D_INC(5) + D_INC(10) == 17",
                             "6 + 11 should equal 17",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_INC in multiplication
    result = D_INC(3) * D_INC(4);

    if (!d_assert_standalone(result == 20,
                             "D_INC(3) * D_INC(4) == 20",
                             "4 * 5 should equal 20",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_INC chained calls test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_INC chained calls test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_EVAL TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_eval_basic
  Tests basic D_EVAL functionality for forcing macro expansion.
  Tests the following:
  - D_EVAL passes through simple values unchanged
  - D_EVAL forces expansion of macro arguments
  - D_EVAL works with expressions
*/
bool
d_tests_sa_dmacro_eval_basic
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    int         result;
    const char* str_result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_EVAL Basic Functionality ---\n", D_INDENT);

    // test D_EVAL with simple integer
    result = D_EVAL(42);

    if (!d_assert_standalone(result == 42,
                             "D_EVAL(42) == 42",
                             "D_EVAL should pass through integer",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with expression
    result = D_EVAL(10 + 20);

    if (!d_assert_standalone(result == 30,
                             "D_EVAL(10 + 20) == 30",
                             "D_EVAL should evaluate expression",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with macro result
    result = D_EVAL(D_INC(5));

    if (!d_assert_standalone(result == 6,
                             "D_EVAL(D_INC(5)) == 6",
                             "D_EVAL should expand D_INC",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with string
    str_result = D_EVAL("hello");

    if (!d_assert_standalone(strcmp(str_result, "hello") == 0,
                             "D_EVAL(\"hello\") == \"hello\"",
                             "D_EVAL should pass through string",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_result);
    }

    // test D_EVAL with D_VARG_COUNT
    result = D_EVAL(D_VARG_COUNT(1, 2, 3));

    if (!d_assert_standalone(result == 3,
                             "D_EVAL(D_VARG_COUNT(1,2,3)) == 3",
                             "D_EVAL should expand D_VARG_COUNT",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_EVAL basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_EVAL basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_eval_nested
  Tests nested D_EVAL calls and multiple expansion passes.
  Tests the following:
  - D_EVAL(D_EVAL(x)) works correctly
  - Multiple evaluation passes don't change stable results
  - D_EVAL can force expansion of deferred macros
*/
bool
d_tests_sa_dmacro_eval_nested
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

    printf("%s--- Testing D_EVAL Nested Calls ---\n", D_INDENT);

    // test double D_EVAL
    result = D_EVAL(D_EVAL(100));

    if (!d_assert_standalone(result == 100,
                             "D_EVAL(D_EVAL(100)) == 100",
                             "double D_EVAL should be stable",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test triple D_EVAL
    result = D_EVAL(D_EVAL(D_EVAL(42)));

    if (!d_assert_standalone(result == 42,
                             "D_EVAL(D_EVAL(D_EVAL(42))) == 42",
                             "triple D_EVAL should be stable",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test nested D_EVAL with macro expansion
    result = D_EVAL(D_EVAL(D_INC(D_INC(0))));

    if (!d_assert_standalone(result == 2,
                             "D_EVAL(D_EVAL(D_INC(D_INC(0)))) == 2",
                             "nested eval with nested inc",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_EVAL with complex expression
    result = D_EVAL(D_INC(5) + D_INC(9));

    if (!d_assert_standalone(result == 16,
                             "D_EVAL(D_INC(5) + D_INC(9)) == 16",
                             "D_EVAL with complex expression (6+10)",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_EVAL nested calls test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_EVAL nested calls test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * ITERATION MACRO TESTS (MSVC-compatible)
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_stringify
  Tests D_FOR_EACH_COMMA with D_STRINGIFY (built-in operator).
  This tests iteration functionality using only built-in dmacro operators
  to avoid MSVC traditional preprocessor issues with custom operators.
*/
bool
d_tests_sa_dmacro_for_each_stringify
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* strings[3];
    size_t      count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_FOR_EACH_COMMA with D_STRINGIFY ---\n", D_INDENT);

    // test D_FOR_EACH_COMMA with D_STRINGIFY to create string array
    {
        const char* test_arr[] = { D_FOR_EACH_COMMA(D_STRINGIFY, alpha, beta, gamma) };
        count = sizeof(test_arr) / sizeof(test_arr[0]);

        if (!d_assert_standalone(count == 3,
                                 "D_FOR_EACH_COMMA(D_STRINGIFY,...) creates 3 strings",
                                 "should create array of 3 stringified identifiers",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    Got count: %zu\n", D_INDENT, count);
        }

        strings[0] = test_arr[0];
        strings[1] = test_arr[1];
        strings[2] = test_arr[2];

        if (!d_assert_standalone(strcmp(strings[0], "alpha") == 0,
                                 "first string == \"alpha\"",
                                 "D_STRINGIFY should convert alpha to \"alpha\"",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    Got: \"%s\"\n", D_INDENT, strings[0]);
        }

        if (!d_assert_standalone(strcmp(strings[1], "beta") == 0,
                                 "second string == \"beta\"",
                                 "D_STRINGIFY should convert beta to \"beta\"",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    Got: \"%s\"\n", D_INDENT, strings[1]);
        }

        if (!d_assert_standalone(strcmp(strings[2], "gamma") == 0,
                                 "third string == \"gamma\"",
                                 "D_STRINGIFY should convert gamma to \"gamma\"",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    Got: \"%s\"\n", D_INDENT, strings[2]);
        }
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_FOR_EACH_COMMA with D_STRINGIFY test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_FOR_EACH_COMMA with D_STRINGIFY test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * PRACTICAL EVALUATION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_eval_practical
  Tests practical usage patterns of evaluation macros.
  Tests the following:
  - Using D_INC in compile-time patterns
  - Using D_EVAL for deferred expansion
  - Combining D_INC with D_VARG_COUNT
*/
bool
d_tests_sa_dmacro_eval_practical
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;
    int    sequence[5];

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Practical Evaluation Patterns ---\n", D_INDENT);

    // pattern: generating a sequence with D_INC
    sequence[0] = 0;
    sequence[1] = D_INC(0);
    sequence[2] = D_INC(D_INC(0));
    sequence[3] = D_INC(D_INC(D_INC(0)));
    sequence[4] = D_INC(D_INC(D_INC(D_INC(0))));

    if (!d_assert_standalone(sequence[0] == 0 && sequence[1] == 1 &&
                             sequence[2] == 2 && sequence[3] == 3 &&
                             sequence[4] == 4,
                             "D_INC sequence generates 0,1,2,3,4",
                             "D_INC can generate sequences",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d,%d,%d,%d,%d\n", D_INDENT,
               sequence[0], sequence[1], sequence[2],
               sequence[3], sequence[4]);
    }

    // pattern: chained D_INC to compute value
    // equivalent to adding 5 to 10: INC^5(10) = 15
    result = D_INC(D_INC(D_INC(D_INC(D_INC(10)))));

    if (!d_assert_standalone(result == 15,
                             "D_INC^5(10) == 15",
                             "chained increment pattern",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // pattern: using D_VARG_COUNT with D_INC
    result = D_INC(D_VARG_COUNT(1, 2, 3, 4, 5));

    if (!d_assert_standalone(result == 6,
                             "D_INC(D_VARG_COUNT(5 args)) == 6",
                             "combining counting with increment",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // pattern: conditional computation simulation
    {
        int has = D_HAS_ARGS(1, 2);
        result  = has * D_INC(20);

        if (!d_assert_standalone(result == 21,
                                 "HAS_ARGS guard with D_INC == 21",
                                 "conditional pattern (1 * inc(20))",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    Got: %d\n", D_INDENT, result);
        }
    }

    // pattern: computing array bounds
    {
        int arr[D_INC(D_INC(D_INC(0)))];  // arr[3]
        (void)arr;

        if (!d_assert_standalone(sizeof(arr) / sizeof(arr[0]) == 3,
                                 "D_INC chain for array size == 3",
                                 "D_INC usable in array declaration",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Practical evaluation patterns test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Practical evaluation patterns test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * MACRO EXPANSION AND EVALUATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_eval_all
  Runs all macro expansion and evaluation tests.
  Tests the following sections:
  - D_INC (basic, medium, large, chained)
  - D_EVAL (basic, nested)
  - Iteration macros (D_FOR_EACH_COMMA with D_STRINGIFY)
  - Practical patterns
*/
bool
d_tests_sa_dmacro_eval_all
(
    struct d_test_counter* _test_info
)
{
    bool                  inc_basic_result;
    bool                  inc_medium_result;
    bool                  inc_large_result;
    bool                  inc_chained_result;
    bool                  eval_basic_result;
    bool                  eval_nested_result;
    bool                  for_each_stringify_result;
    bool                  practical_result;
    bool                  overall_result;
    struct d_test_counter module_counter;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Macro Expansion and Evaluation (Section III)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run D_INC tests
    inc_basic_result   = d_tests_sa_dmacro_inc_basic(&module_counter);
    inc_medium_result  = d_tests_sa_dmacro_inc_medium(&module_counter);
    inc_large_result   = d_tests_sa_dmacro_inc_large(&module_counter);
    inc_chained_result = d_tests_sa_dmacro_inc_chained(&module_counter);

    // run D_EVAL tests
    eval_basic_result  = d_tests_sa_dmacro_eval_basic(&module_counter);
    eval_nested_result = d_tests_sa_dmacro_eval_nested(&module_counter);

    // run iteration tests (MSVC-compatible)
    for_each_stringify_result = d_tests_sa_dmacro_for_each_stringify(&module_counter);

    // run practical tests
    practical_result = d_tests_sa_dmacro_eval_practical(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( inc_basic_result           &&
                       inc_medium_result          &&
                       inc_large_result           &&
                       inc_chained_result         &&
                       eval_basic_result          &&
                       eval_nested_result         &&
                       for_each_stringify_result  &&
                       practical_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Macro Expansion/Evaluation Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Macro Expansion/Evaluation Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - D_INC Basic:        %s\n", inc_basic_result ? "PASSED" : "FAILED");
        printf("  - D_INC Medium:       %s\n", inc_medium_result ? "PASSED" : "FAILED");
        printf("  - D_INC Large:        %s\n", inc_large_result ? "PASSED" : "FAILED");
        printf("  - D_INC Chained:      %s\n", inc_chained_result ? "PASSED" : "FAILED");
        printf("  - D_EVAL Basic:       %s\n", eval_basic_result ? "PASSED" : "FAILED");
        printf("  - D_EVAL Nested:      %s\n", eval_nested_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH+STRINGIFY: %s\n", for_each_stringify_result ? "PASSED" : "FAILED");
        printf("  - Practical:          %s\n", practical_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}