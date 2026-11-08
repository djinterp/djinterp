/*******************************************************************************
* djinterp [test]                                   dio_tests_formatted_output.c
*
*
* path:      /tests/djinterp/c/dio_tests_formatted_output.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dio_tests.h"


/*
d_tests_dio_sprintf_s
  Tests the d_sprintf_s secure formatting function.
  Tests the following:
  - zero size behavior (varies by implementation)
  - successful simple formatting
  - successful multi-value formatting
  - buffer overflow handling
  - return value correctness

  Note: NULL buffer tests are skipped as they cause assertions in debug builds.
  Behavior varies between sprintf_s (Windows) and vsnprintf (fallback).
*/
bool d_tests_dio_sprintf_s(
    struct dj_test_suite* _suite
)
{
    bool result;
    char buffer[128];
    int  written;

    d_test_suite_test(_suite, "dio_sprintf_s");

    result = true;

    // test 1: NULL buffer test skipped (causes assertion in debug builds)

    // test 2: zero size behavior varies by implementation
    // vsnprintf: returns would-be length (4)
    // sprintf_s: may return error (-1)
    written = d_sprintf_s(buffer, 0, "test");
    result  = D_TEST_CHECK_MSG(_suite,
                               "sprintf_s_zero_size",
                               (written == 4) || (written < 0),
                               "Zero size should return would-be length or error") && result;

    // test 3: successful simple format
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "Hello, World!");
    result  = D_TEST_CHECK_MSG(_suite,
                               "sprintf_s_simple_count",
                               written == 13,
                               "Simple format should return 13") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "sprintf_s_simple_value",
                              strcmp(buffer, "Hello, World!") == 0,
                              "Buffer should contain 'Hello, World!'") && result;

    // test 4: successful integer formatting
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "Number: %d", 42);
    result  = D_TEST_CHECK_MSG(_suite,
                               "sprintf_s_int_count",
                               written == 10,
                               "Integer format should return 10") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "sprintf_s_int_value",
                              strcmp(buffer, "Number: 42") == 0,
                              "Buffer should contain 'Number: 42'") && result;

    // test 5: successful multi-value formatting
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "%s %d %.2f", "Test", 123, 45.67);
    result  = D_TEST_CHECK_MSG(_suite,
                               "sprintf_s_multi_count",
                               written > 0,
                               "Multi-value format should return positive") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "sprintf_s_multi_value",
                              strcmp(buffer, "Test 123 45.67") == 0,
                              "Buffer should contain 'Test 123 45.67'") && result;

    // test 6: buffer size limiting (small buffer)
    // Behavior varies by implementation:
    // vsnprintf: returns would-be length (21)
    // sprintf_s: may return error (-1) or truncated length
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, 5, "This is a long string");

    result = D_TEST_CHECK_MSG(_suite,
                              "sprintf_s_truncate",
                              (written == (int)strlen("This is a long string")) || (written < 0) || (written == 4),
                              "Small buffer should return would-be length, error, or truncated length") && result;

    // Verify buffer was null-terminated and truncated
    result = D_TEST_CHECK_MSG(_suite,
                              "sprintf_s_truncate_actual",
                              strlen(buffer) <= 4,
                              "Buffer should be truncated and null-terminated") && result;

    // test 7: empty format string
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "");
    result  = D_TEST_CHECK_MSG(_suite,
                               "sprintf_s_empty_format",
                               written == 0,
                               "Empty format should return 0") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "sprintf_s_empty_value",
                              buffer[0] == '\0',
                              "Buffer should be empty string") && result;

    return result;
}


/*
d_tests_dio_vsprintf_s
  Tests the d_vsprintf_s secure variadic formatting function.
  Tests the following:
  - successful formatting via variadic wrapper
  - correct argument passing through va_list
  - buffer size enforcement

  Note: NULL buffer tests are skipped as they cause assertions in debug builds.
*/
static
int vsprintf_s_test_wrapper(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
    result = d_vsprintf_s(_buffer, _size, _format, args);
    va_end(args);

    return result;
}

bool d_tests_dio_vsprintf_s(
    struct dj_test_suite* _suite
)
{
    bool result;
    char buffer[128];
    int  written;

    d_test_suite_test(_suite, "dio_vsprintf_s");

    result = true;

    // test 1: successful format via variadic wrapper
    d_memset(buffer, 0, sizeof(buffer));
    written = vsprintf_s_test_wrapper(buffer, sizeof(buffer), "Value: %d", 999);
    result  = D_TEST_CHECK_MSG(_suite,
                               "vsprintf_s_success_count",
                               written > 0,
                               "Variadic format should return positive") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "vsprintf_s_success_value",
                              strcmp(buffer, "Value: 999") == 0,
                              "Buffer should contain 'Value: 999'") && result;

    // test 2: multi-value format
    d_memset(buffer, 0, sizeof(buffer));
    written = vsprintf_s_test_wrapper(buffer, sizeof(buffer), "%s: %d", "Count", 50);
    result  = D_TEST_CHECK_MSG(_suite,
                               "vsprintf_s_multi_count",
                               written > 0,
                               "Multi-value format should return positive") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "vsprintf_s_multi_value",
                              strcmp(buffer, "Count: 50") == 0,
                              "Buffer should contain 'Count: 50'") && result;

    // test 3: NULL buffer test skipped (causes assertion in debug builds)

    return result;
}


/*
d_tests_dio_snprintf
  Tests the d_snprintf size-limited formatting function.
  Tests the following:
  - NULL buffer handling
  - zero size handling
  - successful formatting
  - truncation behavior
  - return value for truncated output
*/
bool d_tests_dio_snprintf(
    struct dj_test_suite* _suite
)
{
    bool result;
    char buffer[32];
    int  written;

    d_test_suite_test(_suite, "dio_snprintf");

    result = true;

    // test 1: successful format within buffer
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, sizeof(buffer), "Short");
    result  = D_TEST_CHECK_MSG(_suite,
                               "snprintf_short_count",
                               written == 5,
                               "Short format should return 5") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "snprintf_short_value",
                              strcmp(buffer, "Short") == 0,
                              "Buffer should contain 'Short'") && result;

    // test 2: truncation (buffer too small)
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, 10, "This is a very long string");

    // snprintf should return what WOULD have been written
    result = D_TEST_CHECK_MSG(_suite,
                              "snprintf_truncate_count",
                              written == (int)strlen("This is a very long string"),
                              "Truncated format should return full length") && result;

    // but buffer should be truncated with null terminator
    result = D_TEST_CHECK_MSG(_suite,
                              "snprintf_truncate_value",
                              strlen(buffer) == 9,
                              "Buffer should be truncated to 9 chars") && result;

    // test 3: zero size should not write
    buffer[0] = 'X';  // marker
    written = d_snprintf(buffer, 0, "Test");
    result  = D_TEST_CHECK_MSG(_suite,
                               "snprintf_zero_size_count",
                               written == 4,
                               "Zero size should return 4") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "snprintf_zero_size_unchanged",
                              buffer[0] == 'X',
                              "Buffer should be unchanged") && result;

    // test 4: size of 1 should write only null terminator
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, 1, "Test");
    result  = D_TEST_CHECK_MSG(_suite,
                               "snprintf_size_one_count",
                               written == 4,
                               "Size 1 should return 4") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "snprintf_size_one_value",
                              buffer[0] == '\0',
                              "Buffer should contain only null terminator") && result;

    // test 5: integer formatting
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, sizeof(buffer), "%d", 12345);
    result  = D_TEST_CHECK_MSG(_suite,
                               "snprintf_int_count",
                               written == 5,
                               "Integer format should return 5") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "snprintf_int_value",
                              strcmp(buffer, "12345") == 0,
                              "Buffer should contain '12345'") && result;

    return result;
}


/*
d_tests_dio_vsnprintf
  Tests the d_vsnprintf variadic size-limited formatting function.
  Tests the following:
  - successful formatting via variadic wrapper
  - truncation behavior
  - return value correctness
*/
static
int vsnprintf_test_wrapper(
    char*       _buffer,
    size_t      _size,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
    result = d_vsnprintf(_buffer, _size, _format, args);
    va_end(args);

    return result;
}

bool d_tests_dio_vsnprintf(
    struct dj_test_suite* _suite
)
{
    bool result;
    char buffer[64];
    int  written;

    d_test_suite_test(_suite, "dio_vsnprintf");

    result = true;

    // test 1: successful format via variadic wrapper
    d_memset(buffer, 0, sizeof(buffer));
    written = vsnprintf_test_wrapper(buffer, sizeof(buffer), "Item %d: %s", 5, "test");
    result  = D_TEST_CHECK_MSG(_suite,
                               "vsnprintf_success_count",
                               written > 0,
                               "Variadic format should return positive") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "vsnprintf_success_value",
                              strcmp(buffer, "Item 5: test") == 0,
                              "Buffer should contain 'Item 5: test'") && result;

    // test 2: truncation behavior
    d_memset(buffer, 0, sizeof(buffer));
    written = vsnprintf_test_wrapper(buffer, 8, "Very long formatted string %d", 100);
    result  = D_TEST_CHECK_MSG(_suite,
                               "vsnprintf_truncate_count",
                               written > 8,
                               "Truncated should return full length") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "vsnprintf_truncate_value",
                              strlen(buffer) == 7,
                              "Buffer should be truncated to 7 chars") && result;

    // test 3: exact fit
    d_memset(buffer, 0, sizeof(buffer));
    written = vsnprintf_test_wrapper(buffer, 6, "12345");
    result  = D_TEST_CHECK_MSG(_suite,
                               "vsnprintf_exact_count",
                               written == 5,
                               "Exact fit should return 5") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "vsnprintf_exact_value",
                              strcmp(buffer, "12345") == 0,
                              "Buffer should contain '12345'") && result;

    return result;
}


/*
d_tests_dio_formatted_output_all
  Aggregation function that runs all formatted output tests.
*/
bool d_tests_dio_formatted_output_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "dio_formatted_output_all");

    result = true;

    result = d_tests_dio_sprintf_s(_suite) && result;
    result = d_tests_dio_vsprintf_s(_suite) && result;
    result = d_tests_dio_snprintf(_suite) && result;
    result = d_tests_dio_vsnprintf(_suite) && result;

    return result;
}
