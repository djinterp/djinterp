#include ".\functional_common_tests_sa.h"


// helper predicates for testing

static bool pred_is_even(const void* _element, void* _context)
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

static bool pred_is_positive(const void* _element, void* _context)
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

static bool pred_is_negative(const void* _element, void* _context)
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

static bool pred_greater_than_10(const void* _element, void* _context)
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
d_tests_sa_fc_any
  Tests the d_functional_any function.
  Tests the following:
  - NULL input rejection
  - NULL test rejection
  - returns true when at least one element matches
  - returns false when no elements match
  - short-circuit behavior
*/
bool
d_tests_sa_fc_any
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  input1[5] = {1, 2, 3, 4, 5};
    int  input2[5] = {1, 3, 5, 7, 9};
    int  input3[5] = {-1, -2, -3, -4, -5};

    result = true;

    // test 1: NULL input should return false
    result = d_assert_standalone(
        d_functional_any(NULL,
                         5,
                         sizeof(int),
                         pred_is_even,
                         NULL) == false,
        "any_null_input",
        "NULL input should return false",
        _counter) && result;

    // test 2: NULL test should return false
    result = d_assert_standalone(
        d_functional_any(input1, 5, sizeof(int), NULL, NULL) == false,
        "any_null_test",
        "NULL test should return false",
        _counter) && result;

    // test 3: any even numbers in [1, 2, 3, 4, 5] = true
    result = d_assert_standalone(
        d_functional_any(input1,
                         5,
                         sizeof(int),
                         pred_is_even,
                         NULL) == true,
        "any_has_even",
        "Should find even numbers (2, 4)",
        _counter) && result;

    // test 4: any even numbers in [1, 3, 5, 7, 9] = false
    result = d_assert_standalone(
        d_functional_any(input2,
                         5,
                         sizeof(int),
                         pred_is_even,
                         NULL) == false,
        "any_no_even",
        "Should not find any even numbers",
        _counter) && result;

    // test 5: any positive in negative array = false
    result = d_assert_standalone(
        d_functional_any(input3,
                         5,
                         sizeof(int),
                         pred_is_positive,
                         NULL) == false,
        "any_no_positive",
        "Should not find any positive numbers",
        _counter) && result;

    // test 6: any negative in negative array = true
    result = d_assert_standalone(
        d_functional_any(input3,
                         5,
                         sizeof(int),
                         pred_is_negative,
                         NULL) == true,
        "any_has_negative",
        "Should find negative numbers",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_all
  Tests the d_functional_all function.
  Tests the following:
  - NULL input rejection
  - NULL test rejection
  - returns true when all elements match
  - returns false when at least one element doesn't match
  - short-circuit behavior
*/
bool
d_tests_sa_fc_all
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  input1[5] = {2, 4, 6, 8, 10};
    int  input2[5] = {1, 2, 3, 4, 5};
    int  input3[5] = {1, 2, 3, 4, 5};

    result = true;

    // test 1: NULL input should return false
    result = d_assert_standalone(
        d_functional_all(NULL,
                         5,
                         sizeof(int),
                         pred_is_even,
                         NULL) == false,
        "all_null_input",
        "NULL input should return false",
        _counter) && result;

    // test 2: all even in [2, 4, 6, 8, 10] = true
    result = d_assert_standalone(
        d_functional_all(input1,
                         5,
                         sizeof(int),
                         pred_is_even,
                         NULL) == true,
        "all_even",
        "All elements are even",
        _counter) && result;

    // test 3: all even in [1, 2, 3, 4, 5] = false
    result = d_assert_standalone(
        d_functional_all(input2,
                         5,
                         sizeof(int),
                         pred_is_even,
                         NULL) == false,
        "all_not_even",
        "Not all elements are even",
        _counter) && result;

    // test 4: all positive in [1, 2, 3, 4, 5] = true
    result = d_assert_standalone(
        d_functional_all(input3,
                         5,
                         sizeof(int),
                         pred_is_positive,
                         NULL) == true,
        "all_positive",
        "All elements are positive",
        _counter) && result;

    // test 5: all greater than 10 in [1, 2, 3, 4, 5] = false
    result = d_assert_standalone(
        d_functional_all(input3,
                         5,
                         sizeof(int),
                         pred_greater_than_10,
                         NULL) == false,
        "all_not_gt_10",
        "No elements are greater than 10",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_none
  Tests the d_functional_none function.
  Tests the following:
  - NULL input rejection
  - returns true when no elements match
  - returns false when at least one element matches
*/
bool
d_tests_sa_fc_none
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  input1[5] = {1, 3, 5, 7, 9};
    int  input2[5] = {1, 2, 3, 4, 5};
    int  input3[5] = {-1, -2, -3, -4, -5};

    result = true;

    // test 1: NULL input should return false
    result = d_assert_standalone(
        d_functional_none(NULL,
                          5,
                          sizeof(int),
                          pred_is_even,
                          NULL) == false,
        "none_null_input",
        "NULL input should return false",
        _counter) && result;

    // test 2: none even in [1, 3, 5, 7, 9] = true
    result = d_assert_standalone(
        d_functional_none(input1,
                          5,
                          sizeof(int),
                          pred_is_even,
                          NULL) == true,
        "none_even",
        "No elements are even",
        _counter) && result;

    // test 3: none even in [1, 2, 3, 4, 5] = false
    result = d_assert_standalone(
        d_functional_none(input2,
                          5,
                          sizeof(int),
                          pred_is_even,
                          NULL) == false,
        "none_has_even",
        "Some elements are even (2, 4)",
        _counter) && result;

    // test 4: none positive in negative array = true
    result = d_assert_standalone(
        d_functional_none(input3,
                          5,
                          sizeof(int),
                          pred_is_positive,
                          NULL) == true,
        "none_positive",
        "No elements are positive",
        _counter) && result;

    // test 5: none greater than 10 in [1, 2, 3, 4, 5] = true
    result = d_assert_standalone(
        d_functional_none(input2,
                          5,
                          sizeof(int),
                          pred_greater_than_10,
                          NULL) == true,
        "none_gt_10",
        "No elements are greater than 10",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_count_if
  Tests the d_functional_count_if function.
  Tests the following:
  - NULL input returns 0
  - correct count when some elements match
  - count 0 when no elements match
  - count all when all elements match
*/
bool
d_tests_sa_fc_count_if
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    input1[5] = {1, 2, 3, 4, 5};
    int    input2[5] = {1, 3, 5, 7, 9};
    int    input3[5] = {2, 4, 6, 8, 10};
    size_t count;

    result = true;

    // test 1: NULL input should return 0
    count  = d_functional_count_if(NULL,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = d_assert_standalone(
        count == 0,
        "count_if_null_input",
        "NULL input should return 0",
        _counter) && result;

    // test 2: count even in [1, 2, 3, 4, 5] = 2
    count  = d_functional_count_if(input1,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = d_assert_standalone(
        count == 2,
        "count_if_some_even",
        "Should count 2 even numbers (2, 4)",
        _counter) && result;

    // test 3: count even in [1, 3, 5, 7, 9] = 0
    count  = d_functional_count_if(input2,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = d_assert_standalone(
        count == 0,
        "count_if_none_even",
        "Should count 0 even numbers",
        _counter) && result;

    // test 4: count even in [2, 4, 6, 8, 10] = 5
    count  = d_functional_count_if(input3,
                                   5,
                                   sizeof(int),
                                   pred_is_even,
                                   NULL);
    result = d_assert_standalone(
        count == 5,
        "count_if_all_even",
        "Should count 5 even numbers (all)",
        _counter) && result;

    // test 5: count positive in [1, 2, 3, 4, 5] = 5
    count  = d_functional_count_if(input1,
                                   5,
                                   sizeof(int),
                                   pred_is_positive,
                                   NULL);
    result = d_assert_standalone(
        count == 5,
        "count_if_all_positive",
        "Should count 5 positive numbers (all)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_find_if
  Tests the d_functional_find_if function.
  Tests the following:
  - NULL input returns NULL
  - finds first matching element
  - returns NULL when no element matches
  - pointer points to correct element in array
*/
bool
d_tests_sa_fc_find_if
(
    struct d_test_counter* _counter
)
{
    bool  result;
    int   input1[5] = {1, 2, 3, 4, 5};
    int   input2[5] = {1, 3, 5, 7, 9};
    int*  found;

    result = true;

    // test 1: NULL input should return NULL
    found  = (int*)d_functional_find_if(NULL,
                                        5,
                                        sizeof(int),
                                        pred_is_even,
                                        NULL);
    result = d_assert_standalone(
        found == NULL,
        "find_if_null_input",
        "NULL input should return NULL",
        _counter) && result;

    // test 2: find first even in [1, 2, 3, 4, 5] = 2
    found  = (int*)d_functional_find_if(input1,
                                        5,
                                        sizeof(int),
                                        pred_is_even,
                                        NULL);
    result = d_assert_standalone(
        found != NULL,
        "find_if_found",
        "Should find an even number",
        _counter) && result;

    if (found)
    {
        result = d_assert_standalone(
            *found == 2,
            "find_if_first_even",
            "First even number should be 2",
            _counter) && result;

        result = d_assert_standalone(
            found == &input1[1],
            "find_if_pointer_location",
            "Pointer should point to input1[1]",
            _counter) && result;
    }

    // test 3: find even in [1, 3, 5, 7, 9] = NULL
    found  = (int*)d_functional_find_if(input2,
                                        5,
                                        sizeof(int),
                                        pred_is_even,
                                        NULL);
    result = d_assert_standalone(
        found == NULL,
        "find_if_not_found",
        "Should not find any even numbers",
        _counter) && result;

    // test 4: find negative in positive array = NULL
    found  = (int*)d_functional_find_if(input1,
                                        5,
                                        sizeof(int),
                                        pred_is_negative,
                                        NULL);
    result = d_assert_standalone(
        found == NULL,
        "find_if_no_negative",
        "Should not find any negative numbers",
        _counter) && result;

    // test 5: find greater than 10 in [1, 2, 3, 4, 5] = NULL
    found  = (int*)d_functional_find_if(input1,
                                        5,
                                        sizeof(int),
                                        pred_greater_than_10,
                                        NULL);
    result = d_assert_standalone(
        found == NULL,
        "find_if_no_gt_10",
        "Should not find any numbers > 10",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_quantifier_all
  Aggregation function that runs all quantifier function tests.
*/
bool
d_tests_sa_fc_quantifier_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Quantifier Functions\n");
    printf("  -------------------------------\n");

    result = d_tests_sa_fc_any(_counter) && result;
    result = d_tests_sa_fc_all(_counter) && result;
    result = d_tests_sa_fc_none(_counter) && result;
    result = d_tests_sa_fc_count_if(_counter) && result;
    result = d_tests_sa_fc_find_if(_counter) && result;

    return result;
}
