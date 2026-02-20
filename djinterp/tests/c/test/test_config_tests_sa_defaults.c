#include ".\test_config_tests_sa.h"


/******************************************************************************
 * VIII. DEFAULT VALUE TESTS
 *****************************************************************************/

/*
d_tests_sa_config_default_indent
  Tests the D_TEST_DEFAULT_INDENT constant.
  Tests the following:
  - Value is "  " (two spaces)
  - Length is 2
*/
bool
d_tests_sa_config_default_indent
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: not NULL
    result = d_assert_standalone(
        D_TEST_DEFAULT_INDENT != NULL,
        "default_indent_not_null",
        "D_TEST_DEFAULT_INDENT should not be NULL",
        _counter) && result;

    // test 2: is two spaces
    result = d_assert_standalone(
        d_strcasecmp(D_TEST_DEFAULT_INDENT, "  ") == 0,
        "default_indent_value",
        "D_TEST_DEFAULT_INDENT should be two spaces",
        _counter) && result;

    // test 3: length is 2
    result = d_assert_standalone(
        d_strnlen(D_TEST_DEFAULT_INDENT, 16) == 2,
        "default_indent_length",
        "D_TEST_DEFAULT_INDENT should have length 2",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_default_max_indent
  Tests the D_TEST_DEFAULT_MAX_INDENT constant.
  Tests the following:
  - Value is 10
  - Type is uint16_t-compatible
*/
bool
d_tests_sa_config_default_max_indent
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t val;

    result = true;
    val    = D_TEST_DEFAULT_MAX_INDENT;

    // test 1: value is 10
    result = d_assert_standalone(
        val == 10,
        "default_max_indent_value",
        "D_TEST_DEFAULT_MAX_INDENT should be 10",
        _counter) && result;

    // test 2: fits in uint16_t (no truncation)
    result = d_assert_standalone(
        D_TEST_DEFAULT_MAX_INDENT <= UINT16_MAX,
        "default_max_indent_range",
        "D_TEST_DEFAULT_MAX_INDENT should fit in uint16_t",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_default_max_failures
  Tests the D_TEST_DEFAULT_MAX_FAILURES constant.
  Tests the following:
  - Value is 0 (unlimited)
*/
bool
d_tests_sa_config_default_max_failures
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t val;

    result = true;
    val    = D_TEST_DEFAULT_MAX_FAILURES;

    // test 1: value is 0
    result = d_assert_standalone(
        val == 0,
        "default_max_failures_value",
        "D_TEST_DEFAULT_MAX_FAILURES should be 0 (unlimited)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_default_timeout
  Tests the D_TEST_DEFAULT_TIMEOUT constant.
  Tests the following:
  - Value is 1000 (milliseconds)
*/
bool
d_tests_sa_config_default_timeout
(
    struct d_test_counter* _counter
)
{
    bool   result;
    size_t val;

    result = true;
    val    = D_TEST_DEFAULT_TIMEOUT;

    // test 1: value is 1000
    result = d_assert_standalone(
        val == 1000,
        "default_timeout_value",
        "D_TEST_DEFAULT_TIMEOUT should be 1000 ms",
        _counter) && result;

    // test 2: positive value
    result = d_assert_standalone(
        val > 0,
        "default_timeout_positive",
        "D_TEST_DEFAULT_TIMEOUT should be positive",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_default_all
  Aggregation function that runs all default value tests.
*/
bool
d_tests_sa_config_default_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Default Values\n");
    printf("  --------------------------\n");

    result = d_tests_sa_config_default_indent(_counter) && result;
    result = d_tests_sa_config_default_max_indent(_counter) && result;
    result = d_tests_sa_config_default_max_failures(_counter) && result;
    result = d_tests_sa_config_default_timeout(_counter) && result;

    return result;
}
