#include ".\dmacro_tests_sa.h"


/******************************************************************************
 * POSITIONAL ARGUMENT ACCESS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_first_second_third
  Tests D_VARG_GET_FIRST through D_VARG_GET_TENTH positional accessors.
  Tests the following:
  - D_VARG_GET_FIRST returns the first argument
  - D_VARG_GET_SECOND returns the second argument
  - D_VARG_GET_THIRD through D_VARG_GET_TENTH return correct positions
  - Positional accessors work with various argument types
*/
bool
d_tests_sa_dmacro_first_second_third
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

    printf("%s--- Testing D_VARG_GET_FIRST through D_VARG_GET_TENTH ---\n",
           D_INDENT);

    // test D_VARG_GET_FIRST
    result = D_VARG_GET_FIRST(10, 20, 30, 40, 50);

    if (!d_assert_standalone(result == 10,
                             "D_VARG_GET_FIRST(10,20,30,40,50) == 10",
                             "FIRST should return first arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_GET_SECOND
    result = D_VARG_GET_SECOND(10, 20, 30, 40, 50);

    if (!d_assert_standalone(result == 20,
                             "D_VARG_GET_SECOND(10,20,30,40,50) == 20",
                             "SECOND should return second arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_GET_THIRD
    result = D_VARG_GET_THIRD(10, 20, 30, 40, 50);

    if (!d_assert_standalone(result == 30,
                             "D_VARG_GET_THIRD(10,20,30,40,50) == 30",
                             "THIRD should return third arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_GET_FOURTH
    result = D_VARG_GET_FOURTH(10, 20, 30, 40, 50);

    if (!d_assert_standalone(result == 40,
                             "D_VARG_GET_FOURTH(10,20,30,40,50) == 40",
                             "FOURTH should return fourth arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_GET_FIFTH
    result = D_VARG_GET_FIFTH(10, 20, 30, 40, 50);

    if (!d_assert_standalone(result == 50,
                             "D_VARG_GET_FIFTH(10,20,30,40,50) == 50",
                             "FIFTH should return fifth arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_GET_SIXTH through TENTH with more args
    result = D_VARG_GET_SIXTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 6,
                             "D_VARG_GET_SIXTH(...) == 6",
                             "SIXTH should return sixth arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_VARG_GET_SEVENTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 7,
                             "D_VARG_GET_SEVENTH(...) == 7",
                             "SEVENTH should return seventh arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_VARG_GET_EIGHTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 8,
                             "D_VARG_GET_EIGHTH(...) == 8",
                             "EIGHTH should return eighth arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_VARG_GET_NINTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 9,
                             "D_VARG_GET_NINTH(...) == 9",
                             "NINTH should return ninth arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    result = D_VARG_GET_TENTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 10,
                             "D_VARG_GET_TENTH(...) == 10",
                             "TENTH should return tenth arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test with single argument
    result = D_VARG_GET_FIRST(42);

    if (!d_assert_standalone(result == 42,
                             "D_VARG_GET_FIRST(42) == 42",
                             "FIRST with single arg should work",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Positional accessor test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Positional accessor test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * HEAD/REST/TAIL ALIAS TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_head_rest_tail
  Tests D_HEAD, D_REST, and D_TAIL macros for list manipulation.
  Tests the following:
  - D_HEAD returns the first element (alias for D_VARG_GET_FIRST)
  - D_REST returns all elements except the first
  - D_TAIL is an alias for D_REST
  - These macros work with various argument counts
*/
bool
d_tests_sa_dmacro_head_rest_tail
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    head_result;
    int    rest_count;
    int    tail_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_HEAD, D_REST, D_TAIL ---\n", D_INDENT);

    // test D_HEAD
    head_result = D_HEAD(100, 200, 300);

    if (!d_assert_standalone(head_result == 100,
                             "D_HEAD(100, 200, 300) == 100",
                             "HEAD should return first element",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, head_result);
    }

    // test D_HEAD with single element
    head_result = D_HEAD(42);

    if (!d_assert_standalone(head_result == 42,
                             "D_HEAD(42) == 42",
                             "HEAD with single element should work",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, head_result);
    }

    // test D_REST - count remaining args
    rest_count = D_VARG_COUNT(D_REST(1, 2, 3, 4, 5));

    if (!d_assert_standalone(rest_count == 4,
                             "D_VARG_COUNT(D_REST(1,2,3,4,5)) == 4",
                             "REST should return all but first",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, rest_count);
    }

    // test D_REST - verify first element of rest
    head_result = D_HEAD(D_REST(10, 20, 30, 40));

    if (!d_assert_standalone(head_result == 20,
                             "D_HEAD(D_REST(10,20,30,40)) == 20",
                             "HEAD of REST should be second original",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, head_result);
    }

    // test D_TAIL - should be same as D_REST
    tail_count = D_VARG_COUNT(D_TAIL(1, 2, 3, 4, 5));

    if (!d_assert_standalone(tail_count == 4,
                             "D_VARG_COUNT(D_TAIL(1,2,3,4,5)) == 4",
                             "TAIL should return all but first",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, tail_count);
    }

    // test D_TAIL - verify first element
    head_result = D_HEAD(D_TAIL(100, 200, 300));

    if (!d_assert_standalone(head_result == 200,
                             "D_HEAD(D_TAIL(100,200,300)) == 200",
                             "HEAD of TAIL should be second original",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, head_result);
    }

    // Note: Chained D_REST(D_REST(...)) doesn't work without D_EXPAND
    // because the inner macro result isn't re-scanned. Test the concept
    // using D_VARG_GET_THIRD instead.
    head_result = D_VARG_GET_THIRD(1, 2, 3, 4, 5);

    if (!d_assert_standalone(head_result == 3,
                             "D_VARG_GET_THIRD(1,2,3,4,5) == 3",
                             "positional accessor for third element",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, head_result);
    }

    // test D_REST with two elements (should return single element)
    rest_count = D_VARG_COUNT(D_REST(1, 2));

    if (!d_assert_standalone(rest_count == 1,
                             "D_VARG_COUNT(D_REST(1, 2)) == 1",
                             "REST of two elements should return one",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, rest_count);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_HEAD/D_REST/D_TAIL test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_HEAD/D_REST/D_TAIL test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * PARENTHESES HANDLING TESTS
 *****************************************************************************/

/*
d_tests_sa_dmacro_remove_parentheses
  Tests D_VARGS_REMOVE_PARENTHESES and D_VARG_LAST macros.
  Tests the following:
  - D_VARGS_REMOVE_PARENTHESES strips outer parentheses
  - D_VARG_LAST returns the last argument in a list
  - Both macros work with various argument counts and types
*/
bool
d_tests_sa_dmacro_remove_parentheses
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    result;
    int    count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_VARGS_REMOVE_PARENTHESES and D_VARG_LAST ---\n",
           D_INDENT);

    // test D_VARGS_REMOVE_PARENTHESES - count args after removal
    count = D_VARG_COUNT(D_VARGS_REMOVE_PARENTHESES((1, 2, 3)));

    if (!d_assert_standalone(count == 3,
                             "D_VARG_COUNT(D_VARGS_REMOVE_PARENTHESES((1,2,3))) == 3",
                             "remove parens should expose inner args",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, count);
    }

    // test D_VARGS_REMOVE_PARENTHESES - get first after removal
    result = D_HEAD(D_VARGS_REMOVE_PARENTHESES((10, 20, 30)));

    if (!d_assert_standalone(result == 10,
                             "D_HEAD(D_VARGS_REMOVE_PARENTHESES((10,20,30))) == 10",
                             "HEAD after remove parens should be first inner",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARGS_REMOVE_PARENTHESES with single element
    result = D_HEAD(D_VARGS_REMOVE_PARENTHESES((42)));

    if (!d_assert_standalone(result == 42,
                             "D_HEAD(D_VARGS_REMOVE_PARENTHESES((42))) == 42",
                             "remove parens with single element",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_LAST with multiple elements
    result = D_VARG_LAST(1, 2, 3, 4, 5);

    if (!d_assert_standalone(result == 5,
                             "D_VARG_LAST(1, 2, 3, 4, 5) == 5",
                             "LAST should return last argument",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_LAST with two elements
    result = D_VARG_LAST(100, 200);

    if (!d_assert_standalone(result == 200,
                             "D_VARG_LAST(100, 200) == 200",
                             "LAST with two elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_LAST with single element
    result = D_VARG_LAST(42);

    if (!d_assert_standalone(result == 42,
                             "D_VARG_LAST(42) == 42",
                             "LAST with single element should return it",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // test D_VARG_LAST with ten elements
    result = D_VARG_LAST(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!d_assert_standalone(result == 10,
                             "D_VARG_LAST(1..10) == 10",
                             "LAST with ten elements",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d\n", D_INDENT, result);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Parentheses handling test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Parentheses handling test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * ARGUMENT SELECTION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_selection_all
  Runs all argument selection tests.
  Tests the following sections:
  - D_VARG_GET_FIRST through D_VARG_GET_TENTH
  - D_HEAD, D_REST, D_TAIL
  - D_VARGS_REMOVE_PARENTHESES, D_VARG_LAST
*/
bool
d_tests_sa_dmacro_selection_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  positional_result;
    bool                  head_rest_tail_result;
    bool                  paren_handling_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Argument Selection (Section VI)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all argument selection tests
    positional_result     = d_tests_sa_dmacro_first_second_third(&module_counter);
    head_rest_tail_result = d_tests_sa_dmacro_head_rest_tail(&module_counter);
    paren_handling_result = d_tests_sa_dmacro_remove_parentheses(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( positional_result     &&
                       head_rest_tail_result &&
                       paren_handling_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Argument Selection Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Argument Selection Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Positional Accessors: %s\n",
               positional_result ? "PASSED" : "FAILED");
        printf("  - HEAD/REST/TAIL:       %s\n",
               head_rest_tail_result ? "PASSED" : "FAILED");
        printf("  - Paren Handling:       %s\n",
               paren_handling_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
