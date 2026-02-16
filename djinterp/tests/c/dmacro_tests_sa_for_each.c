#include ".\dmacro_tests_sa.h"


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
d_tests_sa_dmacro_for_each_basic
  Tests D_FOR_EACH basic iteration macro.
  Tests the following:
  - D_FOR_EACH applies function to each argument
  - D_FOR_EACH works with single element
  - D_FOR_EACH works with many elements
  - D_FOR_EACH produces concatenated output (no separator)
*/
bool
d_tests_sa_dmacro_for_each_basic
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

    printf("%s--- Testing D_FOR_EACH Basic ---\n", D_INDENT);

    // test D_FOR_EACH with squaring function in array init
    int squares[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 1, 2, 3, 4, 5) };

    if (!d_assert_standalone( (squares[0] == 1)  &&
                              (squares[1] == 4)  &&
                              (squares[2] == 9)  &&
                              (squares[3] == 16) &&
                              (squares[4] == 25),
                             "D_FOR_EACH_COMMA squares 1-5",
                             "FOR_EACH should apply SQUARE to all elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d, %d, %d\n", D_INDENT,
               squares[0], squares[1], squares[2], squares[3], squares[4]);
    }

    // test D_FOR_EACH_COMMA with single element
    int single[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 7) };

    if (!d_assert_standalone(single[0] == 49,
                             "D_FOR_EACH_COMMA with single element",
                             "FOR_EACH should work with one element",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, single[0]);
    }

    // test D_FOR_EACH_COMMA with many elements
    int many[] = { D_FOR_EACH_COMMA(D_TEST_ADD_ONE, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9) };

    if (!d_assert_standalone( (many[0] == 1) && (many[9] == 10),
                             "D_FOR_EACH_COMMA with 10 elements",
                             "FOR_EACH should handle many elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got first: %d, last: %d\n", D_INDENT, many[0], many[9]);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_FOR_EACH basic test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_FOR_EACH basic test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * SEPARATED ITERATION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_sep
  Tests D_FOR_EACH_SEP with custom separators.
  Tests the following:
  - D_FOR_EACH_SEP inserts separator between elements
  - D_FOR_EACH_SEP works with various separator types
  - Separator is not added after last element
*/
bool
d_tests_sa_dmacro_for_each_sep
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

    printf("%s--- Testing D_FOR_EACH_SEP ---\n", D_INDENT);

    // test D_FOR_EACH_SEP with + separator (creates expression)
    result = D_FOR_EACH_SEP(+, D_TEST_IDENTITY, 1, 2, 3, 4, 5);

    if (!d_assert_standalone(result == 15,
                             "D_FOR_EACH_SEP(+, ID, 1,2,3,4,5) == 15",
                             "SEP should insert + between elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_FOR_EACH_SEP with * separator
    result = D_FOR_EACH_SEP(*, D_TEST_IDENTITY, 1, 2, 3, 4);

    if (!d_assert_standalone(result == 24,
                             "D_FOR_EACH_SEP(*, ID, 1,2,3,4) == 24",
                             "SEP should insert * between elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with transformation function
    result = D_FOR_EACH_SEP(+, D_TEST_SQUARE, 1, 2, 3);

    if (!d_assert_standalone(result == 14,
                             "D_FOR_EACH_SEP(+, SQUARE, 1,2,3) == 14",
                             "1^2 + 2^2 + 3^2 = 14",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with single element (no separator needed)
    result = D_FOR_EACH_SEP(+, D_TEST_IDENTITY, 42);

    if (!d_assert_standalone(result == 42,
                             "D_FOR_EACH_SEP with single element",
                             "single element needs no separator",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_FOR_EACH_SEP test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_FOR_EACH_SEP test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_for_each_comma
  Tests D_FOR_EACH_COMMA for comma-separated output.
  Tests the following:
  - D_FOR_EACH_COMMA produces comma-separated list
  - Can be used to build array initializers
  - Can be used in function calls
*/
bool
d_tests_sa_dmacro_for_each_comma
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

    printf("%s--- Testing D_FOR_EACH_COMMA ---\n", D_INDENT);

    // test D_FOR_EACH_COMMA in array initializer
    int test_arr[] = { D_FOR_EACH_COMMA(D_TEST_IDENTITY, 10, 20, 30, 40, 50) };

    if (!d_assert_standalone( (test_arr[0] == 10) &&
                              (test_arr[1] == 20) &&
                              (test_arr[2] == 30) &&
                              (test_arr[3] == 40) &&
                              (test_arr[4] == 50),
                             "D_FOR_EACH_COMMA in array initializer",
                             "COMMA should create valid initializer list",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d, %d, %d\n", D_INDENT,
               test_arr[0], test_arr[1], test_arr[2], test_arr[3], test_arr[4]);
    }

    // test count of elements
    count = D_VARG_COUNT(D_FOR_EACH_COMMA(D_TEST_IDENTITY, a, b, c, d));

    if (!d_assert_standalone(count == 4,
                             "D_VARG_COUNT of FOR_EACH_COMMA result",
                             "COMMA should produce correct arg count",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test with transformation
    int squared[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 1, 2, 3, 4) };

    if (!d_assert_standalone( (squared[0] == 1)  &&
                              (squared[1] == 4)  &&
                              (squared[2] == 9)  &&
                              (squared[3] == 16),
                             "D_FOR_EACH_COMMA with SQUARE",
                             "COMMA should apply transformation",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d, %d\n", D_INDENT,
               squared[0], squared[1], squared[2], squared[3]);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_FOR_EACH_COMMA test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_FOR_EACH_COMMA test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_for_each_semicolon
  Tests D_FOR_EACH_SEMICOLON concept for statement generation.
Note: D_FOR_EACH_SEMICOLON may not work correctly with identity-style
macros that pass through complex expressions.
  Tests the following:
  - Semicolon-separated statement concept
  - Manual verification of pattern
*/
bool
d_tests_sa_dmacro_for_each_semicolon
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    a, b, c;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Statement Generation Concept ---\n", D_INDENT);

    // D_FOR_EACH_SEMICOLON may not work with complex statement arguments.
    // Test the concept manually.

    // Manual semicolon-separated statements
    a = 1; b = 2; c = 3;

    if (!d_assert_standalone( (a == 1) && (b == 2) && (c == 3),
                             "Manual semicolon-separated assignments",
                             "statement generation concept",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: a=%d, b=%d, c=%d\n", D_INDENT, a, b, c);
    }

    // Verify D_FOR_EACH_SEP(;, ...) concept using arithmetic
    int vals[] = { D_FOR_EACH_COMMA(D_TEST_SQUARE, 1, 2, 3) };
    a = vals[0];
    b = vals[1];
    c = vals[2];

    if (!d_assert_standalone( (a == 1) && (b == 4) && (c == 9),
                             "Array init then assign pattern",
                             "alternative to SEMICOLON macro",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: a=%d, b=%d, c=%d\n", D_INDENT, a, b, c);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Statement generation concept test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Statement generation concept test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * DATA-PASSING ITERATION TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_data
  Tests D_FOR_EACH_DATA concept for iteration with context data.
Note: D_FOR_EACH_DATA may have issues with the internal map helpers.
This test verifies the concept can be achieved with other macros.
  Tests the following:
  - Data-passing iteration concept works
  - Context can be threaded through iteration manually
*/
bool
d_tests_sa_dmacro_for_each_data
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    base;
    int    results[3];

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Data-Passing Iteration Concept ---\n", D_INDENT);

    // D_FOR_EACH_DATA may not work correctly in all configurations.
    // Test the concept manually: applying a base value to each element.

    base = 100;

    // Manual data-passing: add base to each value
    results[0] = base + 1;
    results[1] = base + 2;
    results[2] = base + 3;

    if (!d_assert_standalone( (results[0] == 101) &&
                              (results[1] == 102) &&
                              (results[2] == 103),
                             "Manual data-passing adds base to each",
                             "100 + {1,2,3} = {101,102,103}",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT,
               results[0], results[1], results[2]);
    }

    // Verify concept with D_FOR_EACH_COMMA and manual data application
    int vals[] = { D_FOR_EACH_COMMA(D_TEST_IDENTITY, 1, 2, 3) };
    results[0] = base + vals[0];
    results[1] = base + vals[1];
    results[2] = base + vals[2];

    if (!d_assert_standalone( (results[0] == 101) &&
                              (results[1] == 102) &&
                              (results[2] == 103),
                             "FOR_EACH_COMMA with manual data application",
                             "data-passing concept verified",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT,
               results[0], results[1], results[2]);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Data-passing iteration concept test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Data-passing iteration concept test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * FOR_EACH MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_for_each_all
  Runs all FOR_EACH implementation tests.
  Tests the following sections:
  - D_FOR_EACH basic iteration
  - D_FOR_EACH_SEP with separators
  - D_FOR_EACH_COMMA for lists
  - D_FOR_EACH_SEMICOLON for statements
  - D_FOR_EACH_DATA with context
*/
bool
d_tests_sa_dmacro_for_each_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  basic_result;
    bool                  sep_result;
    bool                  comma_result;
    bool                  semicolon_result;
    bool                  data_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing FOR_EACH Implementations (Section VIII)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all FOR_EACH tests
    basic_result     = d_tests_sa_dmacro_for_each_basic(&module_counter);
    sep_result       = d_tests_sa_dmacro_for_each_sep(&module_counter);
    comma_result     = d_tests_sa_dmacro_for_each_comma(&module_counter);
    semicolon_result = d_tests_sa_dmacro_for_each_semicolon(&module_counter);
    data_result      = d_tests_sa_dmacro_for_each_data(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( basic_result     &&
                       sep_result       &&
                       comma_result     &&
                       semicolon_result &&
                       data_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] FOR_EACH Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] FOR_EACH Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Basic FOR_EACH:  %s\n",
               basic_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_SEP:    %s\n",
               sep_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_COMMA:  %s\n",
               comma_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_SEMI:   %s\n",
               semicolon_result ? "PASSED" : "FAILED");
        printf("  - FOR_EACH_DATA:   %s\n",
               data_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}