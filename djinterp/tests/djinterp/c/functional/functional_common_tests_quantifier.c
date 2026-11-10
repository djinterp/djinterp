/*******************************************************************************
* djinterp [functional]                     functional_common_tests_quantifier.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_quantifier.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// helper predicates for testing

static
bool pred_is_even(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value % 2 == 0);
}

static
bool pred_is_positive(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value > 0);
}

static
bool pred_is_negative(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value < 0);
}

static
bool pred_greater_than_10(
    const void* _element,
    void*       _context
)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value > 10);
}


/*
d_tests_fc_any
  Tests the d_functional_any function.
  Tests the following:
  - NULL input rejection
  - NULL test rejection
  - returns true when at least one element matches
  - returns false when no elements match
  - short-circuit behavior
*/
bool d_tests_fc_any(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input1[5] = {1, 2, 3, 4, 5};
    int  input2[5] = {1, 3, 5, 7, 9};
    int  input3[5] = {-1, -2, -3, -4, -5};

    d_test_suite_test(_suite, "fc_any");

    result = true;

    // test 1: NULL input should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "any_null_input",
                              d_functional_any(NULL, 5, sizeof(int), pred_is_even, NULL) == false,
                              "NULL input should return false") && result;

    // test 2: NULL test should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "any_null_test",
                              d_functional_any(input1, 5, sizeof(int), NULL, NULL) == false,
                              "NULL test should return false") && result;

    // test 3: any even numbers in [1, 2, 3, 4, 5] = true
    result = D_TEST_CHECK_MSG(_suite,
                              "any_has_even",
                              d_functional_any(input1, 5, sizeof(int), pred_is_even, NULL) == true,
                              "Should find even numbers (2, 4)") && result;

    // test 4: any even numbers in [1, 3, 5, 7, 9] = false
    result = D_TEST_CHECK_MSG(_suite,
                              "any_no_even",
                              d_functional_any(input2, 5, sizeof(int), pred_is_even, NULL) == false,
                              "Should not find any even numbers") && result;

    // test 5: any positive in negative array = false
    result = D_TEST_CHECK_MSG(_suite,
                              "any_no_positive",
                              d_functional_any(input3, 5, sizeof(int), pred_is_positive, NULL) == false,
                              "Should not find any positive numbers") && result;

    // test 6: any negative in negative array = true
    result = D_TEST_CHECK_MSG(_suite,
                              "any_has_negative",
                              d_functional_any(input3, 5, sizeof(int), pred_is_negative, NULL) == true,
                              "Should find negative numbers") && result;

    return result;
}


/*
d_tests_fc_all
  Tests the d_functional_all function.
  Tests the following:
  - NULL input rejection
  - NULL test rejection
  - returns true when all elements match
  - returns false when at least one element doesn't match
  - short-circuit behavior
*/
bool d_tests_fc_all(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input1[5] = {2, 4, 6, 8, 10};
    int  input2[5] = {1, 2, 3, 4, 5};
    int  input3[5] = {1, 2, 3, 4, 5};

    d_test_suite_test(_suite, "fc_all");

    result = true;

    // test 1: NULL input should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "all_null_input",
                              d_functional_all(NULL, 5, sizeof(int), pred_is_even, NULL) == false,
                              "NULL input should return false") && result;

    // test 2: all even in [2, 4, 6, 8, 10] = true
    result = D_TEST_CHECK_MSG(_suite,
                              "all_even",
                              d_functional_all(input1, 5, sizeof(int), pred_is_even, NULL) == true,
                              "All elements are even") && result;

    // test 3: all even in [1, 2, 3, 4, 5] = false
    result = D_TEST_CHECK_MSG(_suite,
                              "all_not_even",
                              d_functional_all(input2, 5, sizeof(int), pred_is_even, NULL) == false,
                              "Not all elements are even") && result;

    // test 4: all positive in [1, 2, 3, 4, 5] = true
    result = D_TEST_CHECK_MSG(_suite,
                              "all_positive",
                              d_functional_all(input3, 5, sizeof(int), pred_is_positive, NULL) == true,
                              "All elements are positive") && result;

    // test 5: all greater than 10 in [1, 2, 3, 4, 5] = false
    result = D_TEST_CHECK_MSG(_suite,
                              "all_not_gt_10",
                              d_functional_all(input3, 5, sizeof(int), pred_greater_than_10, NULL) == false,
                              "No elements are greater than 10") && result;

    return result;
}


/*
d_tests_fc_none
  Tests the d_functional_none function.
  Tests the following:
  - NULL input rejection
  - returns true when no elements match
  - returns false when at least one element matches
*/
bool d_tests_fc_none(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input1[5] = {1, 3, 5, 7, 9};
    int  input2[5] = {1, 2, 3, 4, 5};
    int  input3[5] = {-1, -2, -3, -4, -5};

    d_test_suite_test(_suite, "fc_none");

    result = true;

    // test 1: NULL input should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "none_null_input",
                              d_functional_none(NULL, 5, sizeof(int), pred_is_even, NULL) == false,
                              "NULL input should return false") && result;

    // test 2: none even in [1, 3, 5, 7, 9] = true
    result = D_TEST_CHECK_MSG(_suite,
                              "none_even",
                              d_functional_none(input1, 5, sizeof(int), pred_is_even, NULL) == true,
                              "No elements are even") && result;

    // test 3: none even in [1, 2, 3, 4, 5] = false
    result = D_TEST_CHECK_MSG(_suite,
                              "none_has_even",
                              d_functional_none(input2, 5, sizeof(int), pred_is_even, NULL) == false,
                              "Some elements are even (2, 4)") && result;

    // test 4: none positive in negative array = true
    result = D_TEST_CHECK_MSG(_suite,
                              "none_positive",
                              d_functional_none(input3, 5, sizeof(int), pred_is_positive, NULL) == true,
                              "No elements are positive") && result;

    // test 5: none greater than 10 in [1, 2, 3, 4, 5] = true
    result = D_TEST_CHECK_MSG(_suite,
                              "none_gt_10",
                              d_functional_none(input2, 5, sizeof(int), pred_greater_than_10, NULL) == true,
                              "No elements are greater than 10") && result;

    return result;
}


/*
d_tests_fc_count_if
  Tests the d_functional_count_if function.
  Tests the following:
  - NULL input returns 0
  - correct count when some elements match
  - count 0 when no elements match
  - count all when all elements match
*/
bool d_tests_fc_count_if(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    input1[5] = {1, 2, 3, 4, 5};
    int    input2[5] = {1, 3, 5, 7, 9};
    int    input3[5] = {2, 4, 6, 8, 10};
    size_t count;

    d_test_suite_test(_suite, "fc_count_if");

    result = true;

    // test 1: NULL input should return 0
    count  = d_functional_count_if(NULL,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "count_if_null_input",
                              count == 0,
                              "NULL input should return 0") && result;

    // test 2: count even in [1, 2, 3, 4, 5] = 2
    count  = d_functional_count_if(input1,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "count_if_some_even",
                              count == 2,
                              "Should count 2 even numbers (2, 4)") && result;

    // test 3: count even in [1, 3, 5, 7, 9] = 0
    count  = d_functional_count_if(input2,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "count_if_none_even",
                              count == 0,
                              "Should count 0 even numbers") && result;

    // test 4: count even in [2, 4, 6, 8, 10] = 5
    count  = d_functional_count_if(input3,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "count_if_all_even",
                              count == 5,
                              "Should count 5 even numbers (all)") && result;

    // test 5: count positive in [1, 2, 3, 4, 5] = 5
    count  = d_functional_count_if(input1,
                                   5,
                                   sizeof(int),
                                   pred_is_positive,
                                   NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "count_if_all_positive",
                              count == 5,
                              "Should count 5 positive numbers (all)") && result;

    return result;
}


/*
d_tests_fc_find_if
  Tests the d_functional_find_if function.
  Tests the following:
  - NULL input returns NULL
  - finds first matching element
  - returns NULL when no element matches
  - pointer points to correct element in array
*/
bool d_tests_fc_find_if(
    struct dj_test_suite* _suite
)
{
    bool  result;
    int   input1[5] = {1, 2, 3, 4, 5};
    int   input2[5] = {1, 3, 5, 7, 9};
    int*  found;

    d_test_suite_test(_suite, "fc_find_if");

    result = true;

    // test 1: NULL input should return NULL
    found  = (int*)d_functional_find_if(NULL,
                                        5,
                                        sizeof(int),
                                        pred_is_even,
                                        NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "find_if_null_input",
                              found == NULL,
                              "NULL input should return NULL") && result;

    // test 2: find first even in [1, 2, 3, 4, 5] = 2
    found  = (int*)d_functional_find_if(input1,
                                        5,
                                        sizeof(int),
                                        pred_is_even,
                                        NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "find_if_found",
                              found != NULL,
                              "Should find an even number") && result;

    if (found)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "find_if_first_even",
                                  *found == 2,
                                  "First even number should be 2") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "find_if_pointer_location",
                                  found == &input1[1],
                                  "Pointer should point to input1[1]") && result;
    }

    // test 3: find even in [1, 3, 5, 7, 9] = NULL
    found  = (int*)d_functional_find_if(input2,
                                        5,
                                        sizeof(int),
                                        pred_is_even,
                                        NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "find_if_not_found",
                              found == NULL,
                              "Should not find any even numbers") && result;

    // test 4: find negative in positive array = NULL
    found  = (int*)d_functional_find_if(input1,
                                        5,
                                        sizeof(int),
                                        pred_is_negative,
                                        NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "find_if_no_negative",
                              found == NULL,
                              "Should not find any negative numbers") && result;

    // test 5: find greater than 10 in [1, 2, 3, 4, 5] = NULL
    found  = (int*)d_functional_find_if(input1,
                                        5,
                                        sizeof(int),
                                        pred_greater_than_10,
                                        NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "find_if_no_gt_10",
                              found == NULL,
                              "Should not find any numbers > 10") && result;

    return result;
}


/*
d_tests_fc_quantifier_all
  Aggregation function that runs all quantifier function tests.
*/
bool d_tests_fc_quantifier_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "fc_quantifier_all");

    result = true;

    result = d_tests_fc_any(_suite) && result;
    result = d_tests_fc_all(_suite) && result;
    result = d_tests_fc_none(_suite) && result;
    result = d_tests_fc_count_if(_suite) && result;
    result = d_tests_fc_find_if(_suite) && result;

    return result;
}
