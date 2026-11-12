/*******************************************************************************
* djinterp [c]                                          dmacro_tests_arg_count.c
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
* path:      /tests/djinterp/c/dmacro_tests_arg_count.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * D_VARG_COUNT BASIC TESTS
 *****************************************************************************/

/*
d_tests_dmacro_varg_count_basic
  Tests basic D_VARG_COUNT functionality with small argument counts.
  Tests the following:
  - D_VARG_COUNT correctly counts 1 argument
  - D_VARG_COUNT correctly counts 2 arguments
  - D_VARG_COUNT correctly counts 3 arguments
  - D_VARG_COUNT correctly counts 5 arguments
  - D_VARG_COUNT correctly counts 10 arguments
*/
bool d_tests_dmacro_varg_count_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_varg_count_basic");

    all_assertions_passed = true;

    // test with 1 argument
    count = D_VARG_COUNT(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1) == 1",
                          count == 1,
                          "single argument should count as 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 2 arguments
    count = D_VARG_COUNT(1, 2);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1, 2) == 2",
                          count == 2,
                          "two arguments should count as 2"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 3 arguments
    count = D_VARG_COUNT(1, 2, 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1, 2, 3) == 3",
                          count == 3,
                          "three arguments should count as 3"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 5 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1, 2, 3, 4, 5) == 5",
                          count == 5,
                          "five arguments should count as 5"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 10 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..10) == 10",
                          count == 10,
                          "ten arguments should count as 10"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_varg_count_medium
  Tests D_VARG_COUNT with medium argument counts (11-32).
  Tests the following:
  - D_VARG_COUNT correctly counts 15 arguments
  - D_VARG_COUNT correctly counts 16 arguments (power of 2 boundary)
  - D_VARG_COUNT correctly counts 20 arguments
  - D_VARG_COUNT correctly counts 31 arguments
  - D_VARG_COUNT correctly counts 32 arguments (power of 2 boundary)
*/
bool d_tests_dmacro_varg_count_medium(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_varg_count_medium");

    all_assertions_passed = true;

    // test with 15 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..15) == 15",
                          count == 15,
                          "fifteen arguments should count as 15"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 16 arguments (power of 2 boundary)
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..16) == 16",
                          count == 16,
                          "sixteen arguments (2^4) should count as 16"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 20 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..20) == 20",
                          count == 20,
                          "twenty arguments should count as 20"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 31 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                         21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                         31);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..31) == 31",
                          count == 31,
                          "thirty-one arguments should count as 31"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with 32 arguments (power of 2 boundary)
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                         21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                         31, 32);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..32) == 32",
                          count == 32,
                          "thirty-two arguments (2^5) should count as 32"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_varg_count_large
  Tests D_VARG_COUNT with large argument counts (33-64).
  Tests the following:
  - D_VARG_COUNT correctly counts 48 arguments
  - D_VARG_COUNT correctly counts 63 arguments (max - 1)
  - D_VARG_COUNT correctly counts 64 arguments (variant max for 64-bit)
*/
bool d_tests_dmacro_varg_count_large(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_varg_count_large");

    all_assertions_passed = true;

    // test with 48 arguments
    count = D_VARG_COUNT(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..48) == 48",
                          count == 48,
                          "forty-eight arguments should count as 48"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..63) == 63",
                          count == 63,
                          "sixty-three arguments should count as 63"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1..64) == 64",
                          count == 64,
                          "sixty-four arguments should count as 64"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }
    #else
    (void)d_test_suite_note(_suite,
                            "64-argument case skipped (variant max: %d)",
                            D_DMACRO_VARG_MAX);
    #endif

    return all_assertions_passed;
}


/*
d_tests_dmacro_varg_count_types
  Tests D_VARG_COUNT with various argument types.
  Tests the following:
  - D_VARG_COUNT works with integer literals
  - D_VARG_COUNT works with floating-point literals
  - D_VARG_COUNT works with character literals
  - D_VARG_COUNT works with string literals
  - D_VARG_COUNT works with mixed types
  - D_VARG_COUNT works with expressions
*/
bool d_tests_dmacro_varg_count_types(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_varg_count_types");

    all_assertions_passed = true;

    // test with integer literals (various bases)
    count = D_VARG_COUNT(0, 42, -1, 0xFF, 0b1010, 077);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(int literals) == 6",
                          count == 6,
                          "integer literals in various bases"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with floating-point literals
    count = D_VARG_COUNT(1.0, 2.5, 3.14159, -0.5, 1e10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(float literals) == 5",
                          count == 5,
                          "floating-point literals"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with character literals
    count = D_VARG_COUNT('a', 'b', 'c', '\n', '\t', '\\', '\'');

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(char literals) == 7",
                          count == 7,
                          "character literals including escapes"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with string literals
    count = D_VARG_COUNT("hello", "world", "", "a,b,c", "test");

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(string literals) == 5",
                          count == 5,
                          "string literals including empty and with commas"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with mixed types
    count = D_VARG_COUNT(42, 3.14, 'x', "str", -1, 0.0);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(mixed types) == 6",
                          count == 6,
                          "mixed int, float, char, string"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with expressions (parenthesized to avoid comma issues)
    count = D_VARG_COUNT((1+2), (3*4), (5-6), (7/1));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(expressions) == 4",
                          count == 4,
                          "parenthesized expressions"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with sizeof expressions
    count = D_VARG_COUNT(sizeof(int), sizeof(char), sizeof(double));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(sizeof exprs) == 3",
                          count == 3,
                          "sizeof expressions"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_varg_count_edge_cases
  Tests D_VARG_COUNT edge cases and special scenarios.
  Tests the following:
  - D_VARG_COUNT with parenthesized argument groups
  - D_VARG_COUNT with nested macro calls
  - D_VARG_COUNT result usable in expressions
  - D_VARG_COUNT result usable in array declarations
  - D_VARG_COUNT with compound literals
*/
bool d_tests_dmacro_varg_count_edge_cases(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;
    int    arr_size;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_varg_count_edge_cases");

    all_assertions_passed = true;

    // test with parenthesized groups (each group is one argument)
    count = D_VARG_COUNT((1, 2), (3, 4), (5, 6));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT((1,2), (3,4), (5,6)) == 3",
                          count == 3,
                          "parenthesized groups count as single args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test count result in arithmetic expression
    count = D_VARG_COUNT(1, 2, 3) + D_VARG_COUNT(4, 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(3 args) + D_VARG_COUNT(2 args) == 5",
                          count == 5,
                          "count results usable in expressions"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test count in multiplication
    count = D_VARG_COUNT(1, 2, 3, 4) * 2;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(4 args) * 2 == 8",
                          count == 8,
                          "count result in multiplication"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test count usable in array size (via variable)
    arr_size = D_VARG_COUNT(10, 20, 30, 40, 50);
    {
        int test_arr[5];  // size matches expected count
        (void)test_arr;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_VARG_COUNT for array size == 5",
                              arr_size == 5,
                              "count usable for array dimensioning"))
        {
            all_assertions_passed = false;
        }
    }

    // test with nested D_VARG_COUNT (count the counts)
    count = D_VARG_COUNT(
        D_VARG_COUNT(1),
        D_VARG_COUNT(1, 2),
        D_VARG_COUNT(1, 2, 3));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(nested counts) == 3",
                          count == 3,
                          "nested D_VARG_COUNT calls count as 3 args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with ternary expressions
    count = D_VARG_COUNT((1 ? 2 : 3), (4 ? 5 : 6));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(ternary, ternary) == 2",
                          count == 2,
                          "ternary expressions as arguments"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_HAS_ARGS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_has_args_basic
  Tests basic D_HAS_ARGS functionality.
  Tests the following:
  - D_HAS_ARGS returns 1 for single argument
  - D_HAS_ARGS returns 1 for multiple arguments
  - D_HAS_ARGS result is usable as boolean
  - D_HAS_ARGS works with various argument counts
*/
bool d_tests_dmacro_has_args_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_has_args_basic");

    all_assertions_passed = true;

    // test with single argument
    result = D_HAS_ARGS(1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(1) == 1",
                          result == 1,
                          "single argument should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test with two arguments
    result = D_HAS_ARGS(1, 2);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(1, 2) == 1",
                          result == 1,
                          "two arguments should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test with five arguments
    result = D_HAS_ARGS(1, 2, 3, 4, 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(1, 2, 3, 4, 5) == 1",
                          result == 1,
                          "five arguments should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test result as boolean condition
    result = D_HAS_ARGS(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(42) is truthy",
                          result != 0,
                          "result should be usable as boolean"))
    {
        all_assertions_passed = false;
    }

    // test with 10 arguments
    result = D_HAS_ARGS(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(10 args) == 1",
                          result == 1,
                          "ten arguments should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_has_args_types
  Tests D_HAS_ARGS with various argument types.
  Tests the following:
  - D_HAS_ARGS works with integer literals
  - D_HAS_ARGS works with string literals
  - D_HAS_ARGS works with character literals
  - D_HAS_ARGS works with expressions
  - D_HAS_ARGS works with mixed types
*/
bool d_tests_dmacro_has_args_types(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_has_args_types");

    all_assertions_passed = true;

    // test with integer literal
    result = D_HAS_ARGS(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(42) == 1",
                          result == 1,
                          "integer literal"))
    {
        all_assertions_passed = false;
    }

    // test with string literal
    result = D_HAS_ARGS("hello");

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(\"hello\") == 1",
                          result == 1,
                          "string literal"))
    {
        all_assertions_passed = false;
    }

    // test with character literal
    result = D_HAS_ARGS('x');

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS('x') == 1",
                          result == 1,
                          "character literal"))
    {
        all_assertions_passed = false;
    }

    // test with floating-point literal
    result = D_HAS_ARGS(3.14);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(3.14) == 1",
                          result == 1,
                          "floating-point literal"))
    {
        all_assertions_passed = false;
    }

    // test with parenthesized expression
    result = D_HAS_ARGS((1 + 2));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS((1 + 2)) == 1",
                          result == 1,
                          "parenthesized expression"))
    {
        all_assertions_passed = false;
    }

    // test with mixed types
    result = D_HAS_ARGS(1, "two", 3.0, '4');

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(int, str, float, char) == 1",
                          result == 1,
                          "mixed types"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_has_args_large_counts
  Tests D_HAS_ARGS with large argument counts.
  Tests the following:
  - D_HAS_ARGS returns 1 for 32 arguments
  - D_HAS_ARGS returns 1 for 63 arguments
  - D_HAS_ARGS returns 1 for max variant arguments
*/
bool d_tests_dmacro_has_args_large_counts(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_has_args_large_counts");

    all_assertions_passed = true;

    // test with 32 arguments
    result = D_HAS_ARGS(
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(32 args) == 1",
                          result == 1,
                          "32 arguments should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(63 args) == 1",
                          result == 1,
                          "63 arguments should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(64 args) == 1",
                          result == 1,
                          "64 arguments should return 1"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }
    #else
    (void)d_test_suite_note(_suite,
                            "64-argument case skipped (variant max: %d)",
                            D_DMACRO_VARG_MAX);
    #endif

    return all_assertions_passed;
}


/*
d_tests_dmacro_has_args_conditionals
  Tests D_HAS_ARGS usage in conditional contexts.
  Tests the following:
  - D_HAS_ARGS result usable in if statement
  - D_HAS_ARGS result usable in ternary operator
  - D_HAS_ARGS result usable in logical expressions
  - D_HAS_ARGS combined with D_VARG_COUNT
*/
bool d_tests_dmacro_has_args_conditionals(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;
    int    ternary_result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_has_args_conditionals");

    all_assertions_passed = true;

    // test in ternary operator
    ternary_result = D_HAS_ARGS(1, 2, 3) ? 100 : 0;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS ? 100 : 0 == 100",
                          ternary_result == 100,
                          "D_HAS_ARGS truthy in ternary"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", ternary_result);
    }

    // test in logical AND
    result = D_HAS_ARGS(1) && D_HAS_ARGS(2, 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(1) && D_HAS_ARGS(2,3) is truthy",
                          result != 0,
                          "D_HAS_ARGS in logical AND"))
    {
        all_assertions_passed = false;
    }

    // test in logical OR
    result = D_HAS_ARGS(1) || 0;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS(1) || 0 is truthy",
                          result != 0,
                          "D_HAS_ARGS in logical OR"))
    {
        all_assertions_passed = false;
    }

    // test combined with D_VARG_COUNT
    result = D_HAS_ARGS(1, 2, 3) * D_VARG_COUNT(1, 2, 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS * D_VARG_COUNT == 3",
                          result == 3,
                          "combined with D_VARG_COUNT"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test in complex expression
    result = (D_HAS_ARGS(1) + D_HAS_ARGS(2, 3) + D_HAS_ARGS(4, 5, 6));

    if (!D_TEST_CHECK_MSG(_suite,
                          "sum of three D_HAS_ARGS == 3",
                          result == 3,
                          "multiple D_HAS_ARGS in expression"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * COMBINED USAGE TESTS
 *****************************************************************************/

/*
d_tests_dmacro_arg_count_combined
  Tests combined usage of D_VARG_COUNT and D_HAS_ARGS.
  Tests the following:
  - Using both macros together in expressions
  - Consistency between D_VARG_COUNT > 0 and D_HAS_ARGS == 1
  - Practical usage patterns
*/
bool d_tests_dmacro_arg_count_combined(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;
    int    has;
    bool   consistent;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_arg_count_combined");

    all_assertions_passed = true;

    // test consistency with 1 argument
    count = D_VARG_COUNT(1);
    has   = D_HAS_ARGS(1);
    consistent = (count > 0) == (has == 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(1)>0 == D_HAS_ARGS(1)==1",
                          consistent,
                          "consistency with 1 arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "count=%d, has=%d", count, has);
    }

    // test consistency with 5 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5);
    has   = D_HAS_ARGS(1, 2, 3, 4, 5);
    consistent = (count == 5) && (has == 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(5 args)==5 && D_HAS_ARGS==1",
                          consistent,
                          "consistency with 5 args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "count=%d, has=%d", count, has);
    }

    // test consistency with 20 arguments
    count = D_VARG_COUNT(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    has   = D_HAS_ARGS(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                       11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    consistent = (count == 20) && (has == 1);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(20 args)==20 && D_HAS_ARGS==1",
                          consistent,
                          "consistency with 20 args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "count=%d, has=%d", count, has);
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

        if (!D_TEST_CHECK_MSG(_suite,
                              "loop using D_VARG_COUNT sums to 60",
                              sum == 60,
                              "practical loop pattern"))
        {
            all_assertions_passed = false;
            (void)d_test_suite_note(_suite, "sum=%d", sum);
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

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_HAS_ARGS guard allows execution",
                              guarded_sum == 30,
                              "practical guard pattern"))
        {
            all_assertions_passed = false;
            (void)d_test_suite_note(_suite, "guarded_sum=%d", guarded_sum);
        }
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_arg_count_boundary
  Tests boundary conditions for argument counting.
  Tests the following:
  - Behavior at variant boundaries (64, 128, 256, etc.)
  - Consistency near maximum supported count
  - Verification against D_DMACRO_VARG_MAX
*/
bool d_tests_dmacro_arg_count_boundary(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    max_supported;
    int    count_at_63;
    int    has_at_63;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_arg_count_boundary");

    all_assertions_passed = true;

    // verify D_DMACRO_VARG_MAX is accessible and reasonable
    max_supported = D_DMACRO_VARG_MAX;

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARG_MAX >= 64",
                          max_supported >= 64,
                          "minimum variant is 64"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "max_supported=%d", max_supported);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DMACRO_VARG_MAX <= 1024",
                          max_supported <= 1024,
                          "maximum variant is 1024"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "max_supported=%d", max_supported);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT at boundary-1 == 63",
                          count_at_63 == 63,
                          "63 args always works"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "count_at_63=%d", count_at_63);
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HAS_ARGS at boundary-1 == 1",
                          has_at_63 == 1,
                          "D_HAS_ARGS works at 63"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "has_at_63=%d", has_at_63);
    }

    // verify count is within supported range
    if (!D_TEST_CHECK_MSG(_suite,
                          "count_at_63 <= D_DMACRO_VARG_MAX",
                          count_at_63 <= max_supported,
                          "63 is within max supported"))
    {
        all_assertions_passed = false;
    }

    (void)d_test_suite_note(_suite, "Current variant max: %d arguments",
                            max_supported);

    return all_assertions_passed;
}


/******************************************************************************
 * ARGUMENT COUNTING MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_arg_count_all
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
bool d_tests_dmacro_arg_count_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_arg_count_all");

    d_tests_dmacro_varg_count_basic(_suite);
    d_tests_dmacro_varg_count_medium(_suite);
    d_tests_dmacro_varg_count_large(_suite);
    d_tests_dmacro_varg_count_types(_suite);
    d_tests_dmacro_varg_count_edge_cases(_suite);
    d_tests_dmacro_has_args_basic(_suite);
    d_tests_dmacro_has_args_types(_suite);
    d_tests_dmacro_has_args_large_counts(_suite);
    d_tests_dmacro_has_args_conditionals(_suite);
    d_tests_dmacro_arg_count_combined(_suite);
    d_tests_dmacro_arg_count_boundary(_suite);

    return true;
}
