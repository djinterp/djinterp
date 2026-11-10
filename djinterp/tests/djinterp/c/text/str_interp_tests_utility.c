/*******************************************************************************
* djinterp [text]                                     str_interp_tests_utility.c
*
*
* path:      /tests/djinterp/c/text/str_interp_tests_utility.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./str_interp_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's
                                                                // C face


/******************************************************************************
 * UTILITY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_str_interp_error_string
  Tests d_str_interp_error_string for error code descriptions.
  Tests the following:
  - returns non-NULL for D_STR_INTERP_SUCCESS
  - returns distinct strings for each error code
  - SUCCESS string contains "success"
  - NULL_PARAM string contains "null"
  - BUFFER_TOO_SMALL string contains "buffer"
  - SPECIFIER_NOT_FOUND string contains "not found"
  - TOO_MANY_SPECIFIERS string contains "too many"
  - ALLOCATION_FAILED string contains "allocation"
  - returns non-NULL for unknown/invalid error code
*/
bool d_tests_str_interp_error_string(struct dj_test_suite* _suite)
{
    const char*           s_success;
    const char*           s_null;
    const char*           s_buffer;
    const char*           s_not_found;
    const char*           s_too_many;
    const char*           s_alloc;
    const char*           s_unknown;
    bool                  test_not_null;
    bool                  test_distinct;
    bool                  test_success_str;
    bool                  test_null_str;
    bool                  test_buffer_str;
    bool                  test_not_found_str;
    bool                  test_too_many_str;
    bool                  test_alloc_str;
    bool                  test_unknown_str;

    // gather all error strings
    s_success   = d_str_interp_error_string(D_STR_INTERP_SUCCESS);
    s_null      = d_str_interp_error_string(D_STR_INTERP_ERROR_NULL_PARAM);
    s_buffer    = d_str_interp_error_string(
                      D_STR_INTERP_ERROR_BUFFER_TOO_SMALL);
    s_not_found = d_str_interp_error_string(
                      D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND);
    s_too_many  = d_str_interp_error_string(
                      D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS);
    s_alloc     = d_str_interp_error_string(
                      D_STR_INTERP_ERROR_ALLOCATION_FAILED);
    s_unknown   = d_str_interp_error_string((enum dj_str_interp_error)999);

    // test 1: all non-NULL
    test_not_null = ( (s_success != NULL)   &&
                      (s_null != NULL)      &&
                      (s_buffer != NULL)    &&
                      (s_not_found != NULL) &&
                      (s_too_many != NULL)  &&
                      (s_alloc != NULL)     &&
                      (s_unknown != NULL) );

    // test 2: distinct strings (no two pointers are the same)
    test_distinct = ( (s_success != s_null)       &&
                      (s_success != s_buffer)     &&
                      (s_success != s_not_found)  &&
                      (s_success != s_too_many)   &&
                      (s_success != s_alloc)      &&
                      (s_null != s_buffer)        &&
                      (s_null != s_not_found)     &&
                      (s_null != s_too_many)      &&
                      (s_null != s_alloc) );

    // test 3: SUCCESS contains "success"
    test_success_str = false;
    if (s_success)
    {
        test_success_str = d_strcontains(s_success,
                                         d_strnlen(s_success, 128),
                                         "success");
    }

    // test 4: NULL_PARAM contains "null"
    test_null_str = false;
    if (s_null)
    {
        test_null_str = d_strcontains(s_null,
                                      d_strnlen(s_null, 128),
                                      "null");
    }

    // test 5: BUFFER_TOO_SMALL contains "buffer"
    test_buffer_str = false;
    if (s_buffer)
    {
        test_buffer_str = d_strcontains(s_buffer,
                                        d_strnlen(s_buffer, 128),
                                        "buffer");
    }

    // test 6: SPECIFIER_NOT_FOUND contains "not found"
    test_not_found_str = false;
    if (s_not_found)
    {
        test_not_found_str = d_strcontains(s_not_found,
                                           d_strnlen(s_not_found, 128),
                                           "not found");
    }

    // test 7: TOO_MANY contains "too many"
    test_too_many_str = false;
    if (s_too_many)
    {
        test_too_many_str = d_strcontains(s_too_many,
                                          d_strnlen(s_too_many, 128),
                                          "too many");
    }

    // test 8: ALLOCATION_FAILED contains "allocation"
    test_alloc_str = false;
    if (s_alloc)
    {
        test_alloc_str = d_strcontains(s_alloc,
                                       d_strnlen(s_alloc, 128),
                                       "allocation");
    }

    // test 9: unknown error returns a non-empty string
    test_unknown_str = false;
    if (s_unknown)
    {
        test_unknown_str = (d_strnlen(s_unknown, 128) > 0);
    }

    // build result tree
    d_test_suite_test(_suite, "d_str_interp_error_string");

    D_TEST_CHECK_MSG(_suite,
                     "not_null",
                     test_not_null,
                     "all error strings are non-NULL");
    D_TEST_CHECK_MSG(_suite,
                     "distinct",
                     test_distinct,
                     "each error code has distinct string");
    D_TEST_CHECK_MSG(_suite,
                     "success_str",
                     test_success_str,
                     "SUCCESS string contains 'success'");
    D_TEST_CHECK_MSG(_suite,
                     "null_str",
                     test_null_str,
                     "NULL_PARAM string contains 'null'");
    D_TEST_CHECK_MSG(_suite,
                     "buffer_str",
                     test_buffer_str,
                     "BUFFER_TOO_SMALL contains 'buffer'");
    D_TEST_CHECK_MSG(_suite,
                     "not_found_str",
                     test_not_found_str,
                     "SPECIFIER_NOT_FOUND contains text");
    D_TEST_CHECK_MSG(_suite,
                     "too_many_str",
                     test_too_many_str,
                     "TOO_MANY contains 'too many'");
    D_TEST_CHECK_MSG(_suite,
                     "alloc_str",
                     test_alloc_str,
                     "ALLOCATION_FAILED contains text");
    D_TEST_CHECK_MSG(_suite,
                     "unknown_str",
                     test_unknown_str,
                     "unknown code returns non-empty");

    return true;
}

/*
d_tests_str_interp_utility_all
  Runs all utility function tests.
  Tests the following:
  - d_str_interp_error_string
*/
bool d_tests_str_interp_utility_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Utility Functions");

    d_tests_str_interp_error_string(_suite);

    return true;
}
