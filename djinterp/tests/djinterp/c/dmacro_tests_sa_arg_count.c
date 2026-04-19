/******************************************************************************
* djinterp [test]                                     dmacro_tests_sa_argcount.c
*
*   Unit tests for `dmacro.h` argument counting utilities (Section II).
*   Tests D_VARG_COUNT, D_HAS_ARGS, and related argument inspection macros.
*   These are foundational macros used throughout the djinterp framework for
*   variadic argument handling.
*
*   Note: All tests use numeric literals instead of bare identifiers to avoid
*   MSVC traditional preprocessor issues where identifiers would be evaluated
*   as expressions.
*
*
* path:      \test\dmacro_tests_sa_argcount.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * D_VARG_COUNT BASIC TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_varg_count_basic
  Tests basic D_VARG_COUNT functionality with small argument counts.
  Tests the following:
  - D_VARG_COUNT correctly counts 1 argument
  - D_VARG_COUNT correctly counts 2 arguments
  - D_VARG_COUNT correctly counts 3 arguments
  - D_VARG_COUNT correctly counts 5 arguments
  - D_VARG_COUNT correctly counts 10 arguments
*/
bool
d_tests_sa_dmacro_varg_count_basic
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_VARG_COUNT Basic Functionality ---\n", D_INDENT);

    // test with 1 argument
    count = D_VARG_COUNT(1);

    if (!d_assert_standalone(count == 1,
                             "D_VARG_COUNT(1) == 1",
                             "single argument should count as 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 2 arguments
    count = D_VARG_COUNT(1, 2);

    if (!d_assert_standalone(count == 2,
                             "D_VARG_COUNT(1, 2) == 2",
                             "two arguments should count as 2",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 3 arguments
    count = D_VARG_COUNT(1, 2, 3);

    if (!d_assert_standalone(count == 3,
                             "D_VARG_COUNT(1, 2, 3) == 3",
                             "three arguments should count as 3",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 5 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5);

    if (!d_assert_standalone(count == 5,
                             "D_VARG_COUNT(1, 2, 3, 4, 5) == 5",
                             "five arguments should count as 5",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 10 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(count == 10,
                             "D_VARG_COUNT(1..10) == 10",
                             "ten arguments should count as 10",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_VARG_COUNT basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_VARG_COUNT basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_varg_count_medium
  Tests D_VARG_COUNT with medium argument counts (11-32).
  Tests the following:
  - D_VARG_COUNT correctly counts 15 arguments
  - D_VARG_COUNT correctly counts 16 arguments (power of 2 boundary)
  - D_VARG_COUNT correctly counts 20 arguments
  - D_VARG_COUNT correctly counts 31 arguments
  - D_VARG_COUNT correctly counts 32 arguments (power of 2 boundary)
*/
bool
d_tests_sa_dmacro_varg_count_medium
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_VARG_COUNT Medium Counts (11-32) ---\n", D_INDENT);

    // test with 15 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15);

    if (!d_assert_standalone(count == 15,
                             "D_VARG_COUNT(1..15) == 15",
                             "fifteen arguments should count as 15",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 16 arguments (power of 2 boundary)
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16);

    if (!d_assert_standalone(count == 16,
                             "D_VARG_COUNT(1..16) == 16",
                             "sixteen arguments (2^4) should count as 16",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 20 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20);

    if (!d_assert_standalone(count == 20,
                             "D_VARG_COUNT(1..20) == 20",
                             "twenty arguments should count as 20",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 31 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                         21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                         31);

    if (!d_assert_standalone(count == 31,
                             "D_VARG_COUNT(1..31) == 31",
                             "thirty-one arguments should count as 31",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 32 arguments (power of 2 boundary)
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                         21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                         31, 32);

    if (!d_assert_standalone(count == 32,
                             "D_VARG_COUNT(1..32) == 32",
                             "thirty-two arguments (2^5) should count as 32",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_VARG_COUNT medium counts test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_VARG_COUNT medium counts test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_varg_count_large
  Tests D_VARG_COUNT with large argument counts (33-64).
  Tests the following:
  - D_VARG_COUNT correctly counts 48 arguments
  - D_VARG_COUNT correctly counts 63 arguments (max - 1)
  - D_VARG_COUNT correctly counts 64 arguments (variant max for 64-bit)
*/
bool
d_tests_sa_dmacro_varg_count_large
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_VARG_COUNT Large Counts (33-64) ---\n", D_INDENT);

    // test with 48 arguments
    count = D_VARG_COUNT(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48);

    if (!d_assert_standalone(count == 48,
                             "D_VARG_COUNT(1..48) == 48",
                             "forty-eight arguments should count as 48",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 63 arguments (max - 1 for 64 variant)
    count = D_VARG_COUNT(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63);

    if (!d_assert_standalone(count == 63,
                             "D_VARG_COUNT(1..63) == 63",
                             "sixty-three arguments should count as 63",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with 64 arguments (max for 64 variant)
    // only run this if variant supports 64+
    #if (D_DMACRO_VARG_MAX >= 64)
    count = D_VARG_COUNT(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63, 64);

    if (!d_assert_standalone(count == 64,
                             "D_VARG_COUNT(1..64) == 64",
                             "sixty-four arguments should count as 64",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }
    #else
    printf("%s    [SKIP] 64-argument test (variant max: %d)\n",
           D_INDENT, D_DMACRO_VARG_MAX);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_VARG_COUNT large counts test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_VARG_COUNT large counts test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_varg_count_types
  Tests D_VARG_COUNT with various argument types.
  Tests the following:
  - D_VARG_COUNT works with integer literals
  - D_VARG_COUNT works with floating-point literals
  - D_VARG_COUNT works with character literals
  - D_VARG_COUNT works with string literals
  - D_VARG_COUNT works with mixed types
  - D_VARG_COUNT works with expressions
*/
bool
d_tests_sa_dmacro_varg_count_types
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_VARG_COUNT with Various Types ---\n", D_INDENT);

    // test with integer literals (various bases)
    count = D_VARG_COUNT(0, 42, -1, 0xFF, 0b1010, 077);

    if (!d_assert_standalone(count == 6,
                             "D_VARG_COUNT(int literals) == 6",
                             "integer literals in various bases",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with floating-point literals
    count = D_VARG_COUNT(1.0, 2.5, 3.14159, -0.5, 1e10);

    if (!d_assert_standalone(count == 5,
                             "D_VARG_COUNT(float literals) == 5",
                             "floating-point literals",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with character literals
    count = D_VARG_COUNT('a', 'b', 'c', '\n', '\t', '\\', '\'');

    if (!d_assert_standalone(count == 7,
                             "D_VARG_COUNT(char literals) == 7",
                             "character literals including escapes",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with string literals
    count = D_VARG_COUNT("hello", "world", "", "a,b,c", "test");

    if (!d_assert_standalone(count == 5,
                             "D_VARG_COUNT(string literals) == 5",
                             "string literals including empty and with commas",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with mixed types
    count = D_VARG_COUNT(42, 3.14, 'x', "str", -1, 0.0);

    if (!d_assert_standalone(count == 6,
                             "D_VARG_COUNT(mixed types) == 6",
                             "mixed int, float, char, string",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with expressions (parenthesized to avoid comma issues)
    count = D_VARG_COUNT((1+2), (3*4), (5-6), (7/1));

    if (!d_assert_standalone(count == 4,
                             "D_VARG_COUNT(expressions) == 4",
                             "parenthesized expressions",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with sizeof expressions
    count = D_VARG_COUNT(sizeof(int), sizeof(char), sizeof(double));

    if (!d_assert_standalone(count == 3,
                             "D_VARG_COUNT(sizeof exprs) == 3",
                             "sizeof expressions",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_VARG_COUNT various types test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_VARG_COUNT various types test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_varg_count_edge_cases
  Tests D_VARG_COUNT edge cases and special scenarios.
  Tests the following:
  - D_VARG_COUNT with parenthesized argument groups
  - D_VARG_COUNT with nested macro calls
  - D_VARG_COUNT result usable in expressions
  - D_VARG_COUNT result usable in array declarations
  - D_VARG_COUNT with compound literals
*/
bool
d_tests_sa_dmacro_varg_count_edge_cases
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;
    int    arr_size;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_VARG_COUNT Edge Cases ---\n", D_INDENT);

    // test with parenthesized groups (each group is one argument)
    count = D_VARG_COUNT((1, 2), (3, 4), (5, 6));

    if (!d_assert_standalone(count == 3,
                             "D_VARG_COUNT((1,2), (3,4), (5,6)) == 3",
                             "parenthesized groups count as single args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test count result in arithmetic expression
    count = D_VARG_COUNT(1, 2, 3) + D_VARG_COUNT(4, 5);

    if (!d_assert_standalone(count == 5,
                             "D_VARG_COUNT(3 args) + D_VARG_COUNT(2 args) == 5",
                             "count results usable in expressions",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test count in multiplication
    count = D_VARG_COUNT(1, 2, 3, 4) * 2;

    if (!d_assert_standalone(count == 8,
                             "D_VARG_COUNT(4 args) * 2 == 8",
                             "count result in multiplication",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test count usable in array size (via variable)
    arr_size = D_VARG_COUNT(10, 20, 30, 40, 50);
    {
        int test_arr[5];  // size matches expected count
        (void)test_arr;

        if (!d_assert_standalone(arr_size == 5,
                                 "D_VARG_COUNT for array size == 5",
                                 "count usable for array dimensioning",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test with nested D_VARG_COUNT (count the counts)
    count = D_VARG_COUNT(
        D_VARG_COUNT(1),
        D_VARG_COUNT(1, 2),
        D_VARG_COUNT(1, 2, 3));

    if (!d_assert_standalone(count == 3,
                             "D_VARG_COUNT(nested counts) == 3",
                             "nested D_VARG_COUNT calls count as 3 args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with ternary expressions
    count = D_VARG_COUNT((1 ? 2 : 3), (4 ? 5 : 6));

    if (!d_assert_standalone(count == 2,
                             "D_VARG_COUNT(ternary, ternary) == 2",
                             "ternary expressions as arguments",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_VARG_COUNT edge cases test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_VARG_COUNT edge cases test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_HAS_ARGS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_has_args_basic
  Tests basic D_HAS_ARGS functionality.
  Tests the following:
  - D_HAS_ARGS returns 1 for single argument
  - D_HAS_ARGS returns 1 for multiple arguments
  - D_HAS_ARGS result is usable as boolean
  - D_HAS_ARGS works with various argument counts
*/
bool
d_tests_sa_dmacro_has_args_basic
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

    printf("%s--- Testing D_HAS_ARGS Basic Functionality ---\n", D_INDENT);

    // test with single argument
    result = D_HAS_ARGS(1);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(1) == 1",
                             "single argument should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with two arguments
    result = D_HAS_ARGS(1, 2);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(1, 2) == 1",
                             "two arguments should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with five arguments
    result = D_HAS_ARGS(1, 2, 3, 4, 5);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(1, 2, 3, 4, 5) == 1",
                             "five arguments should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test result as boolean condition
    result = D_HAS_ARGS(42);

    if (!d_assert_standalone(result != 0,
                             "D_HAS_ARGS(42) is truthy",
                             "result should be usable as boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with 10 arguments
    result = D_HAS_ARGS(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(10 args) == 1",
                             "ten arguments should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_HAS_ARGS basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_HAS_ARGS basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_has_args_types
  Tests D_HAS_ARGS with various argument types.
  Tests the following:
  - D_HAS_ARGS works with integer literals
  - D_HAS_ARGS works with string literals
  - D_HAS_ARGS works with character literals
  - D_HAS_ARGS works with expressions
  - D_HAS_ARGS works with mixed types
*/
bool
d_tests_sa_dmacro_has_args_types
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

    printf("%s--- Testing D_HAS_ARGS with Various Types ---\n", D_INDENT);

    // test with integer literal
    result = D_HAS_ARGS(42);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(42) == 1",
                             "integer literal",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with string literal
    result = D_HAS_ARGS("hello");

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(\"hello\") == 1",
                             "string literal",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with character literal
    result = D_HAS_ARGS('x');

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS('x') == 1",
                             "character literal",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with floating-point literal
    result = D_HAS_ARGS(3.14);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(3.14) == 1",
                             "floating-point literal",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with parenthesized expression
    result = D_HAS_ARGS((1 + 2));

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS((1 + 2)) == 1",
                             "parenthesized expression",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test with mixed types
    result = D_HAS_ARGS(1, "two", 3.0, '4');

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(int, str, float, char) == 1",
                             "mixed types",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_HAS_ARGS various types test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_HAS_ARGS various types test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_has_args_large_counts
  Tests D_HAS_ARGS with large argument counts.
  Tests the following:
  - D_HAS_ARGS returns 1 for 32 arguments
  - D_HAS_ARGS returns 1 for 63 arguments
  - D_HAS_ARGS returns 1 for max variant arguments
*/
bool
d_tests_sa_dmacro_has_args_large_counts
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

    printf("%s--- Testing D_HAS_ARGS with Large Counts ---\n", D_INDENT);

    // test with 32 arguments
    result = D_HAS_ARGS(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(32 args) == 1",
                             "32 arguments should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with 63 arguments
    result = D_HAS_ARGS(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(63 args) == 1",
                             "63 arguments should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    #if (D_DMACRO_VARG_MAX >= 64)
    // test with 64 arguments
    result = D_HAS_ARGS(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63, 64);

    if (!d_assert_standalone(result == 1,
                             "D_HAS_ARGS(64 args) == 1",
                             "64 arguments should return 1",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }
    #else
    printf("%s    [SKIP] 64-argument test (variant max: %d)\n",
           D_INDENT, D_DMACRO_VARG_MAX);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_HAS_ARGS large counts test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_HAS_ARGS large counts test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_has_args_conditionals
  Tests D_HAS_ARGS usage in conditional contexts.
  Tests the following:
  - D_HAS_ARGS result usable in if statement
  - D_HAS_ARGS result usable in ternary operator
  - D_HAS_ARGS result usable in logical expressions
  - D_HAS_ARGS combined with D_VARG_COUNT
*/
bool
d_tests_sa_dmacro_has_args_conditionals
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;
    int    ternary_result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_HAS_ARGS in Conditionals ---\n", D_INDENT);

    // test in ternary operator
    ternary_result = D_HAS_ARGS(1, 2, 3) ? 100 : 0;

    if (!d_assert_standalone(ternary_result == 100,
                             "D_HAS_ARGS ? 100 : 0 == 100",
                             "D_HAS_ARGS truthy in ternary",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, ternary_result);
    }

    // test in logical AND
    result = D_HAS_ARGS(1) && D_HAS_ARGS(2, 3);

    if (!d_assert_standalone(result != 0,
                             "D_HAS_ARGS(1) && D_HAS_ARGS(2,3) is truthy",
                             "D_HAS_ARGS in logical AND",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test in logical OR
    result = D_HAS_ARGS(1) || 0;

    if (!d_assert_standalone(result != 0,
                             "D_HAS_ARGS(1) || 0 is truthy",
                             "D_HAS_ARGS in logical OR",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test combined with D_VARG_COUNT
    result = D_HAS_ARGS(1, 2, 3) * D_VARG_COUNT(1, 2, 3);

    if (!d_assert_standalone(result == 3,
                             "D_HAS_ARGS * D_VARG_COUNT == 3",
                             "combined with D_VARG_COUNT",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test in complex expression
    result = (D_HAS_ARGS(1) + D_HAS_ARGS(2, 3) + D_HAS_ARGS(4, 5, 6));

    if (!d_assert_standalone(result == 3,
                             "sum of three D_HAS_ARGS == 3",
                             "multiple D_HAS_ARGS in expression",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_HAS_ARGS conditionals test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_HAS_ARGS conditionals test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * COMBINED USAGE TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_arg_count_combined
  Tests combined usage of D_VARG_COUNT and D_HAS_ARGS.
  Tests the following:
  - Using both macros together in expressions
  - Consistency between D_VARG_COUNT > 0 and D_HAS_ARGS == 1
  - Practical usage patterns
*/
bool
d_tests_sa_dmacro_arg_count_combined
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    count;
    int    has;
    bool   consistent;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Combined D_VARG_COUNT and D_HAS_ARGS ---\n", D_INDENT);

    // test consistency with 1 argument
    count = D_VARG_COUNT(1);
    has   = D_HAS_ARGS(1);
    consistent = (count > 0) == (has == 1);

    if (!d_assert_standalone(consistent,
                             "D_VARG_COUNT(1)>0 == D_HAS_ARGS(1)==1",
                             "consistency with 1 arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    count=%d, has=%d\n", D_INDENT, count, has);
    }

    // test consistency with 5 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5);
    has   = D_HAS_ARGS(1, 2, 3, 4, 5);
    consistent = (count == 5) && (has == 1);

    if (!d_assert_standalone(consistent,
                             "D_VARG_COUNT(5 args)==5 && D_HAS_ARGS==1",
                             "consistency with 5 args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    count=%d, has=%d\n", D_INDENT, count, has);
    }

    // test consistency with 20 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    has   = D_HAS_ARGS(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                       11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    consistent = (count == 20) && (has == 1);

    if (!d_assert_standalone(consistent,
                             "D_VARG_COUNT(20 args)==20 && D_HAS_ARGS==1",
                             "consistency with 20 args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    count=%d, has=%d\n", D_INDENT, count, has);
    }

    // test practical pattern: conditional based on count
    {
        int arr_count = D_VARG_COUNT(10, 20, 30);
        int values[]  = {10, 20, 30};
        int sum       = 0;
        int i;

        for (i = 0; i < arr_count; i++)
        {
            sum += values[i];
        }

        if (!d_assert_standalone(sum == 60,
                                 "loop using D_VARG_COUNT sums to 60",
                                 "practical loop pattern",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    sum=%d\n", D_INDENT, sum);
        }
    }

    // test practical pattern: guard with D_HAS_ARGS
    {
        int guarded_sum = 0;

        if (D_HAS_ARGS(5, 10, 15))
        {
            int arr[]   = {5, 10, 15};
            guarded_sum = arr[0] + arr[1] + arr[2];
        }

        if (!d_assert_standalone(guarded_sum == 30,
                                 "D_HAS_ARGS guard allows execution",
                                 "practical guard pattern",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    guarded_sum=%d\n", D_INDENT, guarded_sum);
        }
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Combined usage test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Combined usage test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_arg_count_boundary
  Tests boundary conditions for argument counting.
  Tests the following:
  - Behavior at variant boundaries (64, 128, 256, etc.)
  - Consistency near maximum supported count
  - Verification against D_DMACRO_VARG_MAX
*/
bool
d_tests_sa_dmacro_arg_count_boundary
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    max_supported;
    int    count_at_63;
    int    has_at_63;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Argument Count Boundaries ---\n", D_INDENT);

    // verify D_DMACRO_VARG_MAX is accessible and reasonable
    max_supported = D_DMACRO_VARG_MAX;

    if (!d_assert_standalone(max_supported >= 64,
                             "D_DMACRO_VARG_MAX >= 64",
                             "minimum variant is 64",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    max_supported=%d\n", D_INDENT, max_supported);
    }

    if (!d_assert_standalone(max_supported <= 1024,
                             "D_DMACRO_VARG_MAX <= 1024",
                             "maximum variant is 1024",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    max_supported=%d\n", D_INDENT, max_supported);
    }

    // test at 63 (always within range)
    count_at_63 = D_VARG_COUNT(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63);

    has_at_63 = D_HAS_ARGS(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63);

    if (!d_assert_standalone(count_at_63 == 63,
                             "D_VARG_COUNT at boundary-1 == 63",
                             "63 args always works",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    count_at_63=%d\n", D_INDENT, count_at_63);
    }

    if (!d_assert_standalone(has_at_63 == 1,
                             "D_HAS_ARGS at boundary-1 == 1",
                             "D_HAS_ARGS works at 63",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    has_at_63=%d\n", D_INDENT, has_at_63);
    }

    // verify count is within supported range
    if (!d_assert_standalone(count_at_63 <= max_supported,
                             "count_at_63 <= D_DMACRO_VARG_MAX",
                             "63 is within max supported",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Current variant max: %d arguments\n", D_INDENT, max_supported);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Boundary conditions test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Boundary conditions test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * ARGUMENT COUNTING MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_arg_count_all
  Runs all argument counting utilities tests.
  Tests the following sections:
  - D_VARG_COUNT basic functionality
  - D_VARG_COUNT medium argument counts (11-32)
  - D_VARG_COUNT large argument counts (33-64)
  - D_VARG_COUNT with various types
  - D_VARG_COUNT edge cases
  - D_HAS_ARGS basic functionality
  - D_HAS_ARGS with various types
  - D_HAS_ARGS with large counts
  - D_HAS_ARGS in conditionals
  - Combined usage patterns
  - Boundary conditions
*/
bool
d_tests_sa_dmacro_arg_count_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  varg_count_basic_result;
    bool                  varg_count_medium_result;
    bool                  varg_count_large_result;
    bool                  varg_count_types_result;
    bool                  varg_count_edge_result;
    bool                  has_args_basic_result;
    bool                  has_args_types_result;
    bool                  has_args_large_result;
    bool                  has_args_cond_result;
    bool                  combined_result;
    bool                  boundary_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Argument Counting Utilities (Section II)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run D_VARG_COUNT tests
    varg_count_basic_result  = d_tests_sa_dmacro_varg_count_basic(&module_counter);
    varg_count_medium_result = d_tests_sa_dmacro_varg_count_medium(&module_counter);
    varg_count_large_result  = d_tests_sa_dmacro_varg_count_large(&module_counter);
    varg_count_types_result  = d_tests_sa_dmacro_varg_count_types(&module_counter);
    varg_count_edge_result   = d_tests_sa_dmacro_varg_count_edge_cases(&module_counter);

    // run D_HAS_ARGS tests
    has_args_basic_result = d_tests_sa_dmacro_has_args_basic(&module_counter);
    has_args_types_result = d_tests_sa_dmacro_has_args_types(&module_counter);
    has_args_large_result = d_tests_sa_dmacro_has_args_large_counts(&module_counter);
    has_args_cond_result  = d_tests_sa_dmacro_has_args_conditionals(&module_counter);

    // run combined and boundary tests
    combined_result  = d_tests_sa_dmacro_arg_count_combined(&module_counter);
    boundary_result  = d_tests_sa_dmacro_arg_count_boundary(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( varg_count_basic_result  &&
                       varg_count_medium_result &&
                       varg_count_large_result  &&
                       varg_count_types_result  &&
                       varg_count_edge_result   &&
                       has_args_basic_result    &&
                       has_args_types_result    &&
                       has_args_large_result    &&
                       has_args_cond_result     &&
                       combined_result          &&
                       boundary_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Argument Counting Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Argument Counting Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - D_VARG_COUNT Basic:    %s\n",
               varg_count_basic_result ? "PASSED" : "FAILED");
        printf("  - D_VARG_COUNT Medium:   %s\n",
               varg_count_medium_result ? "PASSED" : "FAILED");
        printf("  - D_VARG_COUNT Large:    %s\n",
               varg_count_large_result ? "PASSED" : "FAILED");
        printf("  - D_VARG_COUNT Types:    %s\n",
               varg_count_types_result ? "PASSED" : "FAILED");
        printf("  - D_VARG_COUNT Edge:     %s\n",
               varg_count_edge_result ? "PASSED" : "FAILED");
        printf("  - D_HAS_ARGS Basic:      %s\n",
               has_args_basic_result ? "PASSED" : "FAILED");
        printf("  - D_HAS_ARGS Types:      %s\n",
               has_args_types_result ? "PASSED" : "FAILED");
        printf("  - D_HAS_ARGS Large:      %s\n",
               has_args_large_result ? "PASSED" : "FAILED");
        printf("  - D_HAS_ARGS Cond:       %s\n",
               has_args_cond_result ? "PASSED" : "FAILED");
        printf("  - Combined Usage:        %s\n",
               combined_result ? "PASSED" : "FAILED");
        printf("  - Boundary Conditions:   %s\n",
               boundary_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}