/*******************************************************************************
* djinterp [functional]                      functional_common_tests_predicate.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_predicate.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_functional_is_null
  Tests d_functional_is_null for correctness.
  Tests the following:
  - NULL element pointer returns true (element itself is NULL)
  - pointer-to-NULL-pointer returns true (stored pointer is NULL)
  - pointer-to-non-NULL-pointer returns false
  - context parameter is ignored
  - various non-NULL stored pointer types return false
*/
bool d_tests_functional_is_null(
    struct dj_test_suite* _suite
)
{
    bool        all_passed;
    const void* stored_ptr;
    int         value;
    int         dummy_context;
    bool        result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_is_null");

    all_passed = true;

    // --- test: NULL element pointer returns true ---
    result = d_functional_is_null(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_null: NULL element pointer returns true",
                                   result == true,
                                   "expected true when element itself is NULL");

    // --- test: pointer to NULL stored pointer returns true ---
    stored_ptr = NULL;
    result     = d_functional_is_null(&stored_ptr, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_null: stored NULL pointer returns true",
                                   result == true,
                                   "expected true when *element is NULL");

    // --- test: pointer to non-NULL stored pointer returns false ---
    value      = 42;
    stored_ptr = &value;
    result     = d_functional_is_null(&stored_ptr, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_null: stored non-NULL pointer returns false",
                                   result == false,
                                   "expected false when *element points to valid data");

    // --- test: context parameter is ignored ---
    stored_ptr    = NULL;
    dummy_context = 99;
    result        = d_functional_is_null(&stored_ptr,
                                         &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_null: context is ignored (stored NULL)",
                                   result == true,
                                   "expected true; context should not affect result");

    // --- test: non-NULL stored with context still returns false ---
    stored_ptr = &value;
    result     = d_functional_is_null(&stored_ptr,
                                      &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_null: context is ignored (stored non-NULL)",
                                   result == false,
                                   "expected false; context should not affect result");

    return all_passed;
}


/*
d_tests_functional_is_not_null
  Tests d_functional_is_not_null for correctness.
  Tests the following:
  - NULL element pointer returns false (element itself is NULL)
  - pointer-to-NULL-pointer returns false (stored pointer is NULL)
  - pointer-to-non-NULL-pointer returns true
  - context parameter is ignored
  - complementary behavior to d_functional_is_null
*/
bool d_tests_functional_is_not_null(
    struct dj_test_suite* _suite
)
{
    bool        all_passed;
    const void* stored_ptr;
    int         value;
    int         dummy_context;
    bool        result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_is_not_null");

    all_passed = true;

    // --- test: NULL element pointer returns false ---
    result = d_functional_is_not_null(NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: NULL element pointer returns false",
                                   result == false,
                                   "expected false when element itself is NULL");

    // --- test: pointer to NULL stored pointer returns false ---
    stored_ptr = NULL;
    result     = d_functional_is_not_null(&stored_ptr, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: stored NULL pointer returns false",
                                   result == false,
                                   "expected false when *element is NULL");

    // --- test: pointer to non-NULL stored pointer returns true ---
    value      = 42;
    stored_ptr = &value;
    result     = d_functional_is_not_null(&stored_ptr, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: stored non-NULL pointer returns true",
                                   result == true,
                                   "expected true when *element points to valid data");

    // --- test: context parameter is ignored ---
    stored_ptr    = &value;
    dummy_context = 99;
    result        = d_functional_is_not_null(&stored_ptr,
                                             &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: context is ignored (stored non-NULL)",
                                   result == true,
                                   "expected true; context should not affect result");

    // --- test: complementary to is_null (NULL stored) ---
    stored_ptr = NULL;
    result     = d_functional_is_not_null(&stored_ptr, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: complementary to is_null for NULL stored",
                                   result == false,
                                   "expected false; complement of is_null's true");

    // --- test: complementary to is_null (non-NULL stored) ---
    stored_ptr = &value;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: is_null and is_not_null are complementary",
                                   d_functional_is_null(&stored_ptr, NULL) != d_functional_is_not_null(&stored_ptr, NULL),
                                   "expected opposite results for same non-NULL stored ptr");

    // --- test: complementary to is_null (NULL element) ---
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "is_not_null: complementary for NULL element",
                                   d_functional_is_null(NULL, NULL) != d_functional_is_not_null(NULL, NULL),
                                   "expected opposite results for NULL element");

    return all_passed;
}
