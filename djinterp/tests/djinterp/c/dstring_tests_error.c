/*******************************************************************************
* djinterp [c]                                             dstring_tests_error.c
*
*   Unit tests for dj_string error functions:
*     - d_string_error
*     - d_string_error_r
*
*
* path:      /tests/djinterp/c/dstring_tests_error.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <errno.h>
// djinterp
#include "dstring_tests.h"


/******************************************************************************
* d_tests_dstring_error
******************************************************************************/

/*
d_tests_dstring_error
  Tests d_string_error() which returns a dj_string containing the error message
  for the given error number.

Test cases:
  1.  Known error code (EINVAL)
  2.  Known error code (ENOMEM)
  3.  Known error code (ENOENT)
  4.  Error code 0 (success)
  5.  Unknown/invalid error code
  6.  Returned string is non-empty for known errors
  7.  Multiple calls return independent strings

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_error(struct dj_test_suite* _suite)
{
    struct dj_string*      error_str;
    struct dj_string*      error_str2;

    d_test_suite_test(_suite, "d_string_error");

    // test 1: EINVAL - known error code
    error_str = d_string_error(EINVAL);

    D_TEST_CHECK_MSG(_suite,
                     "einval_not_null",
                     (error_str) != NULL,
                     "d_string_error(EINVAL) should return non-NULL");

    if (error_str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "einval_not_empty",
                         error_str->size > 0,
                         "error message for EINVAL should not be empty");

        d_string_free(error_str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "einval_skip", false, "skip");
    }

    // test 2: ENOMEM - another known error
    error_str = d_string_error(ENOMEM);

    D_TEST_CHECK_MSG(_suite,
                     "enomem_not_null",
                     (error_str) != NULL,
                     "d_string_error(ENOMEM) should return non-NULL");

    if (error_str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "enomem_not_empty",
                         error_str->size > 0,
                         "error message for ENOMEM should not be empty");

        d_string_free(error_str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "enomem_skip", false, "skip");
    }

    // test 3: ENOENT - file not found
    error_str = d_string_error(ENOENT);

    D_TEST_CHECK_MSG(_suite,
                     "enoent_not_null",
                     (error_str) != NULL,
                     "d_string_error(ENOENT) should return non-NULL");

    if (error_str)
    {
        d_string_free(error_str);
    }

    // test 4: error code 0 (success - may or may not have message)
    error_str = d_string_error(0);

    D_TEST_CHECK_MSG(_suite,
                     "zero_not_null",
                     (error_str) != NULL,
                     "d_string_error(0) should return non-NULL");

    if (error_str)
    {
        d_string_free(error_str);
    }

    // test 5: unknown error code
    error_str = d_string_error(99999);

    D_TEST_CHECK_MSG(_suite,
                     "unknown_not_null",
                     (error_str) != NULL,
                     "unknown error should still return a string");

    if (error_str)
    {
        // should have some kind of message, even if generic
        D_TEST_CHECK_MSG(_suite,
                         "unknown_has_message",
                         error_str->size > 0,
                         "unknown error should have some message");

        d_string_free(error_str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "unknown_skip", false, "skip");
    }

    // test 6: multiple calls return independent strings
    error_str  = d_string_error(EINVAL);
    error_str2 = d_string_error(ENOMEM);

    if (error_str && error_str2)
    {
        D_TEST_CHECK_MSG(_suite,
                         "independent_strings",
                         error_str->text != error_str2->text,
                         "multiple calls should return independent strings");

        d_string_free(error_str);
        d_string_free(error_str2);
    }
    else
    {
        if (error_str) d_string_free(error_str);
        if (error_str2) d_string_free(error_str2);
        D_TEST_CHECK_MSG(_suite, "ind_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_error_r
******************************************************************************/

/*
d_tests_dstring_error_r
  Tests d_string_error_r() which is the reentrant/thread-safe version that
  copies the error message to a provided dj_string.

Test cases:
  1.  NULL destination returns error
  2.  Known error code (EINVAL)
  3.  Known error code (ENOMEM)
  4.  Error code 0
  5.  Unknown error code
  6.  Destination is modified in place
  7.  Previous content replaced

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_error_r(struct dj_test_suite* _suite)
{
    struct dj_string*      dest;
    int                   result;

    d_test_suite_test(_suite, "d_string_error_r");

    // test 1: NULL destination returns error
    result = d_string_error_r(EINVAL, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_dest_error",
                     result != 0,
                     "d_string_error_r(..., NULL) should return error");

    // test 2: EINVAL
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(EINVAL, dest);

        D_TEST_CHECK_MSG(_suite,
                         "einval_succeeds",
                         result == 0,
                         "d_string_error_r(EINVAL, ...) should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "einval_not_empty",
                         dest->size > 0,
                         "error message should not be empty");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "einval_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "einval_ne_skip", false, "skip");
    }

    // test 3: ENOMEM
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(ENOMEM, dest);

        D_TEST_CHECK_MSG(_suite,
                         "enomem_succeeds",
                         result == 0,
                         "d_string_error_r(ENOMEM, ...) should succeed");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "enomem_skip", false, "skip");
    }

    // test 4: error code 0
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(0, dest);

        D_TEST_CHECK_MSG(_suite,
                         "zero_succeeds",
                         result == 0,
                         "d_string_error_r(0, ...) should succeed");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "zero_skip", false, "skip");
    }

    // test 5: unknown error code
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(99999, dest);

        // should still succeed and provide some message
        D_TEST_CHECK_MSG(_suite,
                         "unknown_succeeds",
                         result == 0,
                         "unknown error should still succeed");

        D_TEST_CHECK_MSG(_suite,
                         "unknown_has_message",
                         dest->size > 0,
                         "unknown error should have some message");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "unknown_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "unknown_msg_skip", false, "skip");
    }

    // test 6: previous content replaced
    dest = d_string_new_from_cstr("Previous content that should be replaced");

    if (dest)
    {
        result = d_string_error_r(EINVAL, dest);

        D_TEST_CHECK_MSG(_suite,
                         "content_replaced",
                         strstr(dest->text, "Previous") == NULL,
                         "previous content should be replaced");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "replace_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_error_all
******************************************************************************/

/*
d_tests_dstring_error_all
  Runs all error string tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all error string test results.
*/
bool d_tests_dstring_error_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "dj_string Error Functions");

    // run all error string tests
    d_tests_dstring_error(_suite);
    d_tests_dstring_error_r(_suite);

    return true;
}
