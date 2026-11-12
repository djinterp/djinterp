/*******************************************************************************
* djinterp [c][test]                               test_options_tests_defaults.c
*
* test_options.h's suite, block "config_default_all": D_TEST_DEFAULT_INDENT,
* _MAX_INDENT, _MAX_FAILURES and _TIMEOUT.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_defaults.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 8.  DEFAULT VALUE TESTS
//==============================================================================

/*
d_tests_options_default_indent
  Tests the D_TEST_DEFAULT_INDENT constant.
  Tests the following:
  - Value is "  " (two spaces)
  - Length is 2
*/
bool
d_tests_options_default_indent(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_default_indent");

    bool result = true;

    // test 1: not NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "default_indent_not_null",
                              D_TEST_DEFAULT_INDENT != NULL,
                              "D_TEST_DEFAULT_INDENT should not be NULL") && result;

    // test 2: is two spaces
    result = D_TEST_CHECK_MSG(_suite,
                              "default_indent_value",
                              d_strcasecmp(D_TEST_DEFAULT_INDENT,
                                           "  ") == 0,
                              "D_TEST_DEFAULT_INDENT should be two spaces") && result;

    // test 3: length is 2
    result = D_TEST_CHECK_MSG(_suite,
                              "default_indent_length",
                              d_strnlen(D_TEST_DEFAULT_INDENT,
                                        16) == 2,
                              "D_TEST_DEFAULT_INDENT should have length 2") && result;

    return result;
}

/*
d_tests_options_default_max_indent
  Tests the D_TEST_DEFAULT_MAX_INDENT constant.
  Tests the following:
  - Value is 10
  - Type is uint16_t-compatible
*/
bool
d_tests_options_default_max_indent(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_default_max_indent");

    bool           result = true;
    const uint16_t val    = D_TEST_DEFAULT_MAX_INDENT;

    // test 1: value is 10
    result = D_TEST_CHECK_MSG(_suite,
                              "default_max_indent_value",
                              val == 10,
                              "D_TEST_DEFAULT_MAX_INDENT should be 10") && result;

    // test 2: fits in uint16_t (no truncation)
    result = D_TEST_CHECK_MSG(_suite,
                              "default_max_indent_range",
                              D_TEST_DEFAULT_MAX_INDENT <= UINT16_MAX,
                              "D_TEST_DEFAULT_MAX_INDENT should fit in uint16_t") && result;

    return result;
}

/*
d_tests_options_default_max_failures
  Tests the D_TEST_DEFAULT_MAX_FAILURES constant.
  Tests the following:
  - Value is 0 (unlimited)
*/
bool
d_tests_options_default_max_failures(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_default_max_failures");

    bool         result = true;
    const size_t val    = D_TEST_DEFAULT_MAX_FAILURES;

    // test 1: value is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "default_max_failures_value",
                              val == 0,
                              "D_TEST_DEFAULT_MAX_FAILURES should be 0 (unlimited)") && result;

    return result;
}

/*
d_tests_options_default_timeout
  Tests the D_TEST_DEFAULT_TIMEOUT constant.
  Tests the following:
  - Value is 1000 (milliseconds)
*/
bool
d_tests_options_default_timeout(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite,
                      "config_default_timeout");

    bool         result = true;
    const size_t val    = D_TEST_DEFAULT_TIMEOUT;

    // test 1: value is 1000
    result = D_TEST_CHECK_MSG(_suite,
                              "default_timeout_value",
                              val == 1000,
                              "D_TEST_DEFAULT_TIMEOUT should be 1000 ms") && result;

    // test 2: positive value
    result = D_TEST_CHECK_MSG(_suite,
                              "default_timeout_positive",
                              val > 0,
                              "D_TEST_DEFAULT_TIMEOUT should be positive") && result;

    return result;
}

/*
d_tests_options_default_all
  Aggregation function that runs all default value tests.
*/
bool
d_tests_options_default_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite,
                       "config_default_all");

    bool result = true;

    result = d_tests_options_default_indent(_suite) && result;
    result = d_tests_options_default_max_indent(_suite) && result;
    result = d_tests_options_default_max_failures(_suite) && result;
    result = d_tests_options_default_timeout(_suite) && result;

    return result;
}
