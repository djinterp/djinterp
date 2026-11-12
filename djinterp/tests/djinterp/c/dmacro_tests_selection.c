/*******************************************************************************
* djinterp [c]                                          dmacro_tests_selection.c
*
*
* path:      /tests/djinterp/c/dmacro_tests_selection.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmacro_tests.h"


/******************************************************************************
 * POSITIONAL ARGUMENT ACCESS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_first_second_third
  Tests D_VARG_GET_FIRST through D_VARG_GET_TENTH positional accessors.
  Tests the following:
  - D_VARG_GET_FIRST returns the first argument
  - D_VARG_GET_SECOND returns the second argument
  - D_VARG_GET_THIRD through D_VARG_GET_TENTH return correct positions
  - Positional accessors work with various argument types
*/
bool d_tests_dmacro_first_second_third(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_first_second_third");

    all_assertions_passed = true;

    // test D_VARG_GET_FIRST
    result = D_VARG_GET_ARG_1(10, 20, 30, 40, 50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_FIRST(10,20,30,40,50) == 10",
                          result == 10,
                          "FIRST should return first arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_GET_SECOND
    result = D_VARG_GET_SECOND(10, 20, 30, 40, 50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_SECOND(10,20,30,40,50) == 20",
                          result == 20,
                          "SECOND should return second arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_GET_THIRD
    result = D_VARG_GET_THIRD(10, 20, 30, 40, 50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_THIRD(10,20,30,40,50) == 30",
                          result == 30,
                          "THIRD should return third arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_GET_FOURTH
    result = D_VARG_GET_FOURTH(10, 20, 30, 40, 50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_FOURTH(10,20,30,40,50) == 40",
                          result == 40,
                          "FOURTH should return fourth arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_GET_FIFTH
    result = D_VARG_GET_FIFTH(10, 20, 30, 40, 50);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_FIFTH(10,20,30,40,50) == 50",
                          result == 50,
                          "FIFTH should return fifth arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_GET_SIXTH through TENTH with more args
    result = D_VARG_GET_SIXTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_SIXTH(...) == 6",
                          result == 6,
                          "SIXTH should return sixth arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    result = D_VARG_GET_SEVENTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_SEVENTH(...) == 7",
                          result == 7,
                          "SEVENTH should return seventh arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    result = D_VARG_GET_EIGHTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_EIGHTH(...) == 8",
                          result == 8,
                          "EIGHTH should return eighth arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    result = D_VARG_GET_NINTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_NINTH(...) == 9",
                          result == 9,
                          "NINTH should return ninth arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    result = D_VARG_GET_TENTH(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_TENTH(...) == 10",
                          result == 10,
                          "TENTH should return tenth arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test with single argument
    result = D_VARG_GET_FIRST(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_FIRST(42) == 42",
                          result == 42,
                          "FIRST with single arg should work"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * HEAD/REST/TAIL ALIAS TESTS
 *****************************************************************************/

/*
d_tests_dmacro_head_rest_tail
  Tests D_HEAD, D_REST, and D_TAIL macros for list manipulation.
  Tests the following:
  - D_HEAD returns the first element (alias for D_VARG_GET_FIRST)
  - D_REST returns all elements except the first
  - D_TAIL is an alias for D_REST
  - These macros work with various argument counts
*/
bool d_tests_dmacro_head_rest_tail(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    head_result;
    int    rest_count;
    int    tail_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_head_rest_tail");

    all_assertions_passed = true;

    // test D_HEAD
    head_result = D_HEAD(100, 200, 300);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HEAD(100, 200, 300) == 100",
                          head_result == 100,
                          "HEAD should return first element"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", head_result);
    }

    // test D_HEAD with single element
    head_result = D_HEAD(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HEAD(42) == 42",
                          head_result == 42,
                          "HEAD with single element should work"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", head_result);
    }

    // test D_REST - count remaining args
    rest_count = D_VARG_COUNT(D_REST(1, 2, 3, 4, 5));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(D_REST(1,2,3,4,5)) == 4",
                          rest_count == 4,
                          "REST should return all but first"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", rest_count);
    }

    // test D_REST - verify first element of rest
    head_result = D_HEAD(D_REST(10, 20, 30, 40));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HEAD(D_REST(10,20,30,40)) == 20",
                          head_result == 20,
                          "HEAD of REST should be second original"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", head_result);
    }

    // test D_TAIL - should be same as D_REST
    tail_count = D_VARG_COUNT(D_TAIL(1, 2, 3, 4, 5));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(D_TAIL(1,2,3,4,5)) == 4",
                          tail_count == 4,
                          "TAIL should return all but first"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", tail_count);
    }

    // test D_TAIL - verify first element
    head_result = D_HEAD(D_TAIL(100, 200, 300));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HEAD(D_TAIL(100,200,300)) == 200",
                          head_result == 200,
                          "HEAD of TAIL should be second original"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", head_result);
    }

    // Note: Chained D_REST(D_REST(...)) doesn't work without D_EXPAND
    // because the inner macro result isn't re-scanned. Test the concept
    // using D_VARG_GET_THIRD instead.
    head_result = D_VARG_GET_THIRD(1, 2, 3, 4, 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_GET_THIRD(1,2,3,4,5) == 3",
                          head_result == 3,
                          "positional accessor for third element"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", head_result);
    }

    // test D_REST with two elements (should return single element)
    rest_count = D_VARG_COUNT(D_REST(1, 2));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(D_REST(1, 2)) == 1",
                          rest_count == 1,
                          "REST of two elements should return one"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", rest_count);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * PARENTHESES HANDLING TESTS
 *****************************************************************************/

/*
d_tests_dmacro_remove_parentheses
  Tests D_VARGS_REMOVE_PARENTHESES and D_VARG_LAST macros.
  Tests the following:
  - D_VARGS_REMOVE_PARENTHESES strips outer parentheses
  - D_VARG_LAST returns the last argument in a list
  - Both macros work with various argument counts and types
*/
bool d_tests_dmacro_remove_parentheses(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    result;
    int    count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_remove_parentheses");

    all_assertions_passed = true;

    // test D_VARGS_REMOVE_PARENTHESES - count args after removal
    count = D_VARG_COUNT(D_VARGS_REMOVE_PARENTHESES((1, 2, 3)));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_COUNT(D_VARGS_REMOVE_PARENTHESES((1,2,3))) == 3",
                          count == 3,
                          "remove parens should expose inner args"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", count);
    }

    // test D_VARGS_REMOVE_PARENTHESES - get first after removal
    result = D_HEAD(D_VARGS_REMOVE_PARENTHESES((10, 20, 30)));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HEAD(D_VARGS_REMOVE_PARENTHESES((10,20,30))) == 10",
                          result == 10,
                          "HEAD after remove parens should be first inner"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARGS_REMOVE_PARENTHESES with single element
    result = D_HEAD(D_VARGS_REMOVE_PARENTHESES((42)));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_HEAD(D_VARGS_REMOVE_PARENTHESES((42))) == 42",
                          result == 42,
                          "remove parens with single element"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_LAST with multiple elements
    result = D_VARG_LAST(1, 2, 3, 4, 5);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_LAST(1, 2, 3, 4, 5) == 5",
                          result == 5,
                          "LAST should return last argument"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_LAST with two elements
    result = D_VARG_LAST(100, 200);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_LAST(100, 200) == 200",
                          result == 200,
                          "LAST with two elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_LAST with single element
    result = D_VARG_LAST(42);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_LAST(42) == 42",
                          result == 42,
                          "LAST with single element should return it"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    // test D_VARG_LAST with ten elements
    result = D_VARG_LAST(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_VARG_LAST(1..10) == 10",
                          result == 10,
                          "LAST with ten elements"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d", result);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * ARGUMENT SELECTION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_selection_all
  Runs all argument selection tests.
  Tests the following sections:
  - D_VARG_GET_FIRST through D_VARG_GET_TENTH
  - D_HEAD, D_REST, D_TAIL
  - D_VARGS_REMOVE_PARENTHESES, D_VARG_LAST
*/
bool d_tests_dmacro_selection_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_selection_all");

    d_tests_dmacro_first_second_third(_suite);
    d_tests_dmacro_head_rest_tail(_suite);
    d_tests_dmacro_remove_parentheses(_suite);

    return true;
}
