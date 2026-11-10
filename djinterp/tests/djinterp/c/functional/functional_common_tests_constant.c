/*******************************************************************************
* djinterp [functional]                       functional_common_tests_constant.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_constant.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_functional_constant_true
  Tests d_functional_constant_true for correctness.
  Tests the following:
  - returns true with non-NULL element and NULL context
  - returns true with NULL element and NULL context
  - returns true with non-NULL element and non-NULL context
  - returns true with NULL element and non-NULL context
  - always returns true regardless of any argument combination
*/
bool d_tests_functional_constant_true(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  value;
    int  dummy_context;
    bool result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_constant_true");

    all_passed    = true;
    value         = 42;
    dummy_context = 99;

    // --- test: non-NULL element, NULL context ---
    result = d_functional_constant_true(&value, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_true: non-NULL element, NULL context",
                                   result == true,
                                   "expected true regardless of arguments");

    // --- test: NULL element, NULL context ---
    result = d_functional_constant_true(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_true: NULL element, NULL context",
                                   result == true,
                                   "expected true even with NULL element");

    // --- test: non-NULL element, non-NULL context ---
    result = d_functional_constant_true(&value, &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_true: non-NULL element, non-NULL context",
                                   result == true,
                                   "expected true regardless of context");

    // --- test: NULL element, non-NULL context ---
    result = d_functional_constant_true(NULL, &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_true: NULL element, non-NULL context",
                                   result == true,
                                   "expected true regardless of all arguments");

    return all_passed;
}


/*
d_tests_functional_constant_false
  Tests d_functional_constant_false for correctness.
  Tests the following:
  - returns false with non-NULL element and NULL context
  - returns false with NULL element and NULL context
  - returns false with non-NULL element and non-NULL context
  - returns false with NULL element and non-NULL context
  - always returns false regardless of any argument combination
*/
bool d_tests_functional_constant_false(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  value;
    int  dummy_context;
    bool result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_constant_false");

    all_passed    = true;
    value         = 42;
    dummy_context = 99;

    // --- test: non-NULL element, NULL context ---
    result = d_functional_constant_false(&value, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_false: non-NULL element, NULL context",
                                   result == false,
                                   "expected false regardless of arguments");

    // --- test: NULL element, NULL context ---
    result = d_functional_constant_false(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_false: NULL element, NULL context",
                                   result == false,
                                   "expected false even with NULL element");

    // --- test: non-NULL element, non-NULL context ---
    result = d_functional_constant_false(&value, &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_false: non-NULL element, non-NULL context",
                                   result == false,
                                   "expected false regardless of context");

    // --- test: NULL element, non-NULL context ---
    result = d_functional_constant_false(NULL, &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "constant_false: NULL element, non-NULL context",
                                   result == false,
                                   "expected false regardless of all arguments");

    return all_passed;
}
