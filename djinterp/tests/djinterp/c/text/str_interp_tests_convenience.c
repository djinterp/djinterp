/*******************************************************************************
* djinterp [text]                                 str_interp_tests_convenience.c
*
*
* path:      /tests/djinterp/c/text/str_interp_tests_convenience.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./str_interp_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * CONVENIENCE FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_str_interp_quick
  Tests d_str_interp_quick for single-call interpolation.
  Tests the following:
  - replaces specifiers from parallel name/value arrays
  - handles multiple name-value pairs
  - handles empty name/value arrays (null-terminated at index 0)
  - returns ERROR_NULL_PARAM for NULL buffer
  - returns ERROR_NULL_PARAM for NULL input
  - returns ERROR_NULL_PARAM for NULL names
  - returns ERROR_NULL_PARAM for NULL values
  - returns ERROR_BUFFER_TOO_SMALL when buffer is insufficient
  - handles no match (arrays have entries but nothing matches)
  - result matches expected string
*/
bool d_tests_str_interp_quick(struct dj_test_suite* _suite)
{
    enum dj_str_interp_error err;
    char                    buf[D_TEST_INTERP_BUFFER_SIZE];
    bool                    test_single_pair;
    bool                    test_multi_pair;
    bool                    test_empty_arrays;
    bool                    test_null_buf;
    bool                    test_null_input;
    bool                    test_null_names;
    bool                    test_null_values;
    bool                    test_buf_small;
    bool                    test_no_match;
    bool                    test_result;

    /* --- single name-value pair --- */
    {
        const char* names[]  = { "NAME", NULL };
        const char* values[] = { "Bob",  NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "Hi NAME",
                                 names,
                                 values);
        test_single_pair = ( (err == D_STR_INTERP_SUCCESS) &&
                             (d_strequals(buf,
                                          d_strnlen(buf, sizeof(buf)),
                                          "Hi Bob",
                                          6)) );
    }

    /* --- multiple pairs --- */
    {
        const char* names[]  = { "CITY", "YEAR", NULL };
        const char* values[] = { "Paris", "2026", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "CITY in YEAR",
                                 names,
                                 values);
        test_multi_pair = ( (err == D_STR_INTERP_SUCCESS) &&
                            (d_strequals(buf,
                                         d_strnlen(buf, sizeof(buf)),
                                         "Paris in 2026",
                                         13)) );
    }

    /* --- empty arrays (immediate NULL terminator) --- */
    {
        const char* names[]  = { NULL };
        const char* values[] = { NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "nothing changes",
                                 names,
                                 values);
        test_empty_arrays = ( (err == D_STR_INTERP_SUCCESS) &&
                              (d_strequals(buf,
                                           d_strnlen(buf, sizeof(buf)),
                                           "nothing changes",
                                           15)) );
    }

    /* --- NULL buffer --- */
    {
        const char* names[]  = { "K", NULL };
        const char* values[] = { "V", NULL };

        err = d_str_interp_quick(NULL,
                                 100,
                                 "test",
                                 names,
                                 values);
        test_null_buf = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- NULL input --- */
    {
        const char* names[]  = { "K", NULL };
        const char* values[] = { "V", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 NULL,
                                 names,
                                 values);
        test_null_input = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- NULL names array --- */
    {
        const char* values[] = { "V", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "test",
                                 NULL,
                                 values);
        test_null_names = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- NULL values array --- */
    {
        const char* names[] = { "K", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "test",
                                 names,
                                 NULL);
        test_null_values = (err == D_STR_INTERP_ERROR_NULL_PARAM);
    }

    /* --- buffer too small --- */
    {
        const char* names[]  = { "X", NULL };
        const char* values[] = { "a very long replacement string", NULL };

        err = d_str_interp_quick(buf,
                                 5,
                                 "X",
                                 names,
                                 values);
        test_buf_small = (err == D_STR_INTERP_ERROR_BUFFER_TOO_SMALL);
    }

    /* --- no match (keys present but don't appear in input) --- */
    {
        const char* names[]  = { "ZZZ", NULL };
        const char* values[] = { "nope", NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "abc",
                                 names,
                                 values);
        test_no_match = ( (err == D_STR_INTERP_SUCCESS) &&
                          (d_strequals(buf,
                                       d_strnlen(buf, sizeof(buf)),
                                       "abc",
                                       3)) );
    }

    /* --- verify a more complex interpolation result --- */
    {
        const char* names[]  = { "GREETING", "TARGET", NULL };
        const char* values[] = { "Hola",     "Mundo",  NULL };

        err = d_str_interp_quick(buf,
                                 sizeof(buf),
                                 "GREETING TARGET!",
                                 names,
                                 values);
        test_result = ( (err == D_STR_INTERP_SUCCESS) &&
                        (d_strequals(buf,
                                     d_strnlen(buf, sizeof(buf)),
                                     "Hola Mundo!",
                                     11)) );
    }

    /* --- build result tree --- */
    d_test_suite_test(_suite, "d_str_interp_quick");

    D_TEST_CHECK_MSG(_suite,
                     "single_pair",
                     test_single_pair,
                     "single name-value pair works");
    D_TEST_CHECK_MSG(_suite,
                     "multi_pair",
                     test_multi_pair,
                     "multiple pairs work");
    D_TEST_CHECK_MSG(_suite,
                     "empty_arrays",
                     test_empty_arrays,
                     "empty arrays pass through");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "ERROR_NULL_PARAM for NULL buffer");
    D_TEST_CHECK_MSG(_suite,
                     "null_input",
                     test_null_input,
                     "ERROR_NULL_PARAM for NULL input");
    D_TEST_CHECK_MSG(_suite,
                     "null_names",
                     test_null_names,
                     "ERROR_NULL_PARAM for NULL names");
    D_TEST_CHECK_MSG(_suite,
                     "null_values",
                     test_null_values,
                     "ERROR_NULL_PARAM for NULL values");
    D_TEST_CHECK_MSG(_suite,
                     "buf_small",
                     test_buf_small,
                     "ERROR_BUFFER_TOO_SMALL on overflow");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "no match passes through");
    D_TEST_CHECK_MSG(_suite,
                     "result",
                     test_result,
                     "complex interpolation correct");

    return true;
}

/*
d_tests_str_interp_convenience_all
  Runs all convenience function tests.
  Tests the following:
  - d_str_interp_quick
*/
bool d_tests_str_interp_convenience_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Convenience Functions");

    d_tests_str_interp_quick(_suite);

    return true;
}
