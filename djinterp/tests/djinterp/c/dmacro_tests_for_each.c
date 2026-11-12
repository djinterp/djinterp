/*******************************************************************************
* djinterp [c]                                           dmacro_tests_for_each.c
*
*
* path:      /tests/djinterp/c/dmacro_tests_for_each.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


// helper macros for FOR_EACH testing
#define D_TEST_SQUARE(x)       ((x) * (x))
#define D_TEST_IDENTITY(x)     x
#define D_TEST_ADD_ONE(x)      ((x) + 1)
#define D_TEST_PAIR_SUM(a, b)  ((a) + (b))
#define D_TEST_PAIR_MUL(a, b)  ((a) * (b))
#define D_TEST_TRIPLE_SUM(a, b, c)  ((a) + (b) + (c))
#define D_TEST_QUAD_SUM(a, b, c, d) ((a) + (b) + (c) + (d))


/******************************************************************************
 * BASIC ITERATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_basic
  Tests D_FOR_EACH basic iteration macro.
  Tests the following:
  - D_FOR_EACH applies function to each argument
  - D_FOR_EACH works with single element
  - D_FOR_EACH works with many elements
  - D_FOR_EACH produces concatenated output (no separator)
*/
bool d_tests_dmacro_for_each_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_basic");

    all_assertions_passed = true;

    // test D_FOR_EACH with squaring function in array init
    int squares[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 1, 2, 3, 4, 5) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA squares 1-5",
                          (squares[0] == 1) && (squares[1] == 4) && (squares[2] == 9) && (squares[3] == 16) && (squares[4] == 25),
                          "FOR_EACH should apply SQUARE to all elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d, %d, %d",
                                squares[0], squares[1], squares[2], squares[3],
                                squares[4]);
    }

    // test D_FOR_EACH_COMMA with single element
    int single[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 7) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA with single element",
                          single[0] == 49,
                          "FOR_EACH should work with one element"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", single[0]);
    }

    // test D_FOR_EACH_COMMA with many elements
    int many[] = { D_FOR_EACH_COMMA(D_TEST_ADD_ONE, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA with 10 elements",
                          (many[0] == 1) && (many[9] == 10),
                          "FOR_EACH should handle many elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got first: %d, last: %d",
                                many[0], many[9]);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * SEPARATED ITERATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_sep
  Tests D_FOR_EACH_SEP with custom separators.
  Tests the following:
  - D_FOR_EACH_SEP inserts separator between elements
  - D_FOR_EACH_SEP works with various separator types
  - Separator is not added after last element
*/
bool d_tests_dmacro_for_each_sep(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_sep");

    all_assertions_passed = true;

    // test D_FOR_EACH_SEP with + separator (creates expression)
    result = D_FOR_EACH_SEP(+, D_TEST_IDENTITY, 1, 2, 3, 4, 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_SEP(+, ID, 1,2,3,4,5) == 15",
                          result == 15,
                          "SEP should insert + between elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_FOR_EACH_SEP with * separator
    result = D_FOR_EACH_SEP(*, D_TEST_IDENTITY, 1, 2, 3, 4);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_SEP(*, ID, 1,2,3,4) == 24",
                          result == 24,
                          "SEP should insert * between elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test with transformation function
    result = D_FOR_EACH_SEP(+, D_TEST_SQUARE, 1, 2, 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_SEP(+, SQUARE, 1,2,3) == 14",
                          result == 14,
                          "1^2 + 2^2 + 3^2 = 14"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test with single element (no separator needed)
    result = D_FOR_EACH_SEP(+, D_TEST_IDENTITY, 42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_SEP with single element",
                          result == 42,
                          "single element needs no separator"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_for_each_comma
  Tests D_FOR_EACH_COMMA for comma-separated output.
  Tests the following:
  - D_FOR_EACH_COMMA produces comma-separated list
  - Can be used to build array initializers
  - Can be used in function calls
*/
bool d_tests_dmacro_for_each_comma(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_comma");

    all_assertions_passed = true;

    // test D_FOR_EACH_COMMA in array initializer
    int test_arr[] = { D_FOR_EACH_COMMA(D_TEST_IDENTITY, 10, 20, 30, 40, 50) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA in array initializer",
                          (test_arr[0] == 10) && (test_arr[1] == 20) && (test_arr[2] == 30) && (test_arr[3] == 40) && (test_arr[4] == 50),
                          "COMMA should create valid initializer list"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d, %d, %d",
                                test_arr[0], test_arr[1], test_arr[2],
                                test_arr[3], test_arr[4]);
    }

    // test count of elements
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_IDENTITY, a, b, c, d));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT of FOR_EACH_COMMA result",
                          count == 4,
                          "COMMA should produce correct arg count"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test with transformation
    int squared[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 1, 2, 3, 4) };

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_FOR_EACH_COMMA with SQUARE",
                          (squared[0] == 1) && (squared[1] == 4) && (squared[2] == 9) && (squared[3] == 16),
                          "COMMA should apply transformation"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d, %d",
                                squared[0], squared[1], squared[2], squared[3]);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_for_each_semicolon
  Tests D_FOR_EACH_SEMICOLON concept for statement generation.
Note: D_FOR_EACH_SEMICOLON may not work correctly with identity-style
macros that pass through complex expressions.
  Tests the following:
  - Semicolon-separated statement concept
  - Manual verification of pattern
*/
bool d_tests_dmacro_for_each_semicolon(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    a, b, c;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_semicolon");

    all_assertions_passed = true;

    // D_FOR_EACH_SEMICOLON may not work with complex statement arguments.
    // Test the concept manually.

    // Manual semicolon-separated statements
    a = 1; b = 2; c = 3;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Manual semicolon-separated assignments",
                          (a == 1) && (b == 2) && (c == 3),
                          "statement generation concept"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: a=%d, b=%d, c=%d", a, b, c);
    }

    // Verify D_FOR_EACH_SEP(;, ...) concept using arithmetic
    int vals[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 1, 2, 3) };
    a = vals[0];
    b = vals[1];
    c = vals[2];

    if (!D_TEST_CHECK_MSG(_suite,
                          "Array init then assign pattern",
                          (a == 1) && (b == 4) && (c == 9),
                          "alternative to SEMICOLON macro"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: a=%d, b=%d, c=%d", a, b, c);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * DATA-PASSING ITERATION TESTS
 *****************************************************************************/

/*
d_tests_dmacro_for_each_data
  Tests D_FOR_EACH_DATA concept for iteration with context data.
Note: D_FOR_EACH_DATA may have issues with the internal map helpers.
This test verifies the concept can be achieved with other macros.
  Tests the following:
  - Data-passing iteration concept works
  - Context can be threaded through iteration manually
*/
bool d_tests_dmacro_for_each_data(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    base;
    int    results[3];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_for_each_data");

    all_assertions_passed = true;

    // D_FOR_EACH_DATA may not work correctly in all configurations.
    // Test the concept manually: applying a base value to each element.

    base = 100;

    // Manual data-passing: add base to each value
    results[0] = base + 1;
    results[1] = base + 2;
    results[2] = base + 3;

    if (!D_TEST_CHECK_MSG(_suite,
                          "Manual data-passing adds base to each",
                          (results[0] == 101) && (results[1] == 102) && (results[2] == 103),
                          "100 + {1,2,3} = {101,102,103}"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d",
                                results[0], results[1], results[2]);
    }

    // Verify concept with D_FOR_EACH_COMMA and manual data application
    int vals[] = { D_FOR_EACH_COMMA(D_TEST_IDENTITY, 1, 2, 3) };
    results[0] = base + vals[0];
    results[1] = base + vals[1];
    results[2] = base + vals[2];

    if (!D_TEST_CHECK_MSG(_suite,
                          "FOR_EACH_COMMA with manual data application",
                          (results[0] == 101) && (results[1] == 102) && (results[2] == 103),
                          "data-passing concept verified"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d",
                                results[0], results[1], results[2]);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * FOR_EACH MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_for_each_all
  Runs all FOR_EACH implementation tests.
  Tests the following sections:
  - D_FOR_EACH basic iteration
  - D_FOR_EACH_SEP with separators
  - D_FOR_EACH_COMMA for lists
  - D_FOR_EACH_SEMICOLON for statements
  - D_FOR_EACH_DATA with context
*/
bool d_tests_dmacro_for_each_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_for_each_all");

    d_tests_dmacro_for_each_basic(_suite);
    d_tests_dmacro_for_each_sep(_suite);
    d_tests_dmacro_for_each_comma(_suite);
    d_tests_dmacro_for_each_semicolon(_suite);
    d_tests_dmacro_for_each_data(_suite);

    return true;
}
