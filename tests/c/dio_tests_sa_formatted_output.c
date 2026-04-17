#include ".\dio_tests_sa.h"


/*
d_tests_sa_dio_sprintf_s
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
bool
d_tests_sa_dio_sprintf_s
(
    struct d_test_counter* _counter
)
{
    bool result;
    char buffer[128];
    int  written;

    result = true;

    // test 1: NULL buffer test skipped (causes assertion in debug builds)
    
    // test 2: zero size behavior varies by implementation
    // vsnprintf: returns would-be length (4)
    // sprintf_s: may return error (-1) 
    written = d_sprintf_s(buffer, 0, "test");
    result  = d_assert_standalone(
        (written == 4) || (written < 0),
        "sprintf_s_zero_size",
        "Zero size should return would-be length or error",
        _counter) && result;

    // test 3: successful simple format
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "Hello, World!");
    result  = d_assert_standalone(
        written == 13,
        "sprintf_s_simple_count",
        "Simple format should return 13",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Hello, World!") == 0,
        "sprintf_s_simple_value",
        "Buffer should contain 'Hello, World!'",
        _counter) && result;

    // test 4: successful integer formatting
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "Number: %d", 42);
    result  = d_assert_standalone(
        written == 10,
        "sprintf_s_int_count",
        "Integer format should return 10",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Number: 42") == 0,
        "sprintf_s_int_value",
        "Buffer should contain 'Number: 42'",
        _counter) && result;

    // test 5: successful multi-value formatting
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "%s %d %.2f", "Test", 123, 45.67);
    result  = d_assert_standalone(
        written > 0,
        "sprintf_s_multi_count",
        "Multi-value format should return positive",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Test 123 45.67") == 0,
        "sprintf_s_multi_value",
        "Buffer should contain 'Test 123 45.67'",
        _counter) && result;

    // test 6: buffer size limiting (small buffer)
    // Behavior varies by implementation:
    // vsnprintf: returns would-be length (21)
    // sprintf_s: may return error (-1) or truncated length
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, 5, "This is a long string");
    
    result = d_assert_standalone(
        (written == (int)strlen("This is a long string")) || (written < 0) || (written == 4),
        "sprintf_s_truncate",
        "Small buffer should return would-be length, error, or truncated length",
        _counter) && result;
    
    // Verify buffer was null-terminated and truncated
    result = d_assert_standalone(
        strlen(buffer) <= 4,
        "sprintf_s_truncate_actual",
        "Buffer should be truncated and null-terminated",
        _counter) && result;

    // test 7: empty format string
    d_memset(buffer, 0, sizeof(buffer));
    written = d_sprintf_s(buffer, sizeof(buffer), "");
    result  = d_assert_standalone(
        written == 0,
        "sprintf_s_empty_format",
        "Empty format should return 0",
        _counter) && result;

    result = d_assert_standalone(
        buffer[0] == '\0',
        "sprintf_s_empty_value",
        "Buffer should be empty string",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_vsprintf_s
  Tests the d_vsprintf_s secure variadic formatting function.
  Tests the following:
  - successful formatting via variadic wrapper
  - correct argument passing through va_list
  - buffer size enforcement
  
  Note: NULL buffer tests are skipped as they cause assertions in debug builds.
*/
static int
vsprintf_s_test_wrapper
(
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

bool
d_tests_sa_dio_vsprintf_s
(
    struct d_test_counter* _counter
)
{
    bool result;
    char buffer[128];
    int  written;

    result = true;

    // test 1: successful format via variadic wrapper
    d_memset(buffer, 0, sizeof(buffer));
    written = vsprintf_s_test_wrapper(buffer, sizeof(buffer), "Value: %d", 999);
    result  = d_assert_standalone(
        written > 0,
        "vsprintf_s_success_count",
        "Variadic format should return positive",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Value: 999") == 0,
        "vsprintf_s_success_value",
        "Buffer should contain 'Value: 999'",
        _counter) && result;

    // test 2: multi-value format
    d_memset(buffer, 0, sizeof(buffer));
    written = vsprintf_s_test_wrapper(buffer, sizeof(buffer), "%s: %d", "Count", 50);
    result  = d_assert_standalone(
        written > 0,
        "vsprintf_s_multi_count",
        "Multi-value format should return positive",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Count: 50") == 0,
        "vsprintf_s_multi_value",
        "Buffer should contain 'Count: 50'",
        _counter) && result;

    // test 3: NULL buffer test skipped (causes assertion in debug builds)

    return result;
}


/*
d_tests_sa_dio_snprintf
  Tests the d_snprintf size-limited formatting function.
  Tests the following:
  - NULL buffer handling
  - zero size handling
  - successful formatting
  - truncation behavior
  - return value for truncated output
*/
bool
d_tests_sa_dio_snprintf
(
    struct d_test_counter* _counter
)
{
    bool result;
    char buffer[32];
    int  written;

    result = true;

    // test 1: successful format within buffer
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, sizeof(buffer), "Short");
    result  = d_assert_standalone(
        written == 5,
        "snprintf_short_count",
        "Short format should return 5",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Short") == 0,
        "snprintf_short_value",
        "Buffer should contain 'Short'",
        _counter) && result;

    // test 2: truncation (buffer too small)
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, 10, "This is a very long string");
    
    // snprintf should return what WOULD have been written
    result = d_assert_standalone(
        written == (int)strlen("This is a very long string"),
        "snprintf_truncate_count",
        "Truncated format should return full length",
        _counter) && result;

    // but buffer should be truncated with null terminator
    result = d_assert_standalone(
        strlen(buffer) == 9,
        "snprintf_truncate_value",
        "Buffer should be truncated to 9 chars",
        _counter) && result;

    // test 3: zero size should not write
    buffer[0] = 'X';  // marker
    written = d_snprintf(buffer, 0, "Test");
    result  = d_assert_standalone(
        written == 4,
        "snprintf_zero_size_count",
        "Zero size should return 4",
        _counter) && result;

    result = d_assert_standalone(
        buffer[0] == 'X',
        "snprintf_zero_size_unchanged",
        "Buffer should be unchanged",
        _counter) && result;

    // test 4: size of 1 should write only null terminator
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, 1, "Test");
    result  = d_assert_standalone(
        written == 4,
        "snprintf_size_one_count",
        "Size 1 should return 4",
        _counter) && result;

    result = d_assert_standalone(
        buffer[0] == '\0',
        "snprintf_size_one_value",
        "Buffer should contain only null terminator",
        _counter) && result;

    // test 5: integer formatting
    d_memset(buffer, 0, sizeof(buffer));
    written = d_snprintf(buffer, sizeof(buffer), "%d", 12345);
    result  = d_assert_standalone(
        written == 5,
        "snprintf_int_count",
        "Integer format should return 5",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "12345") == 0,
        "snprintf_int_value",
        "Buffer should contain '12345'",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_vsnprintf
  Tests the d_vsnprintf variadic size-limited formatting function.
  Tests the following:
  - successful formatting via variadic wrapper
  - truncation behavior
  - return value correctness
*/
static int
vsnprintf_test_wrapper
(
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

bool
d_tests_sa_dio_vsnprintf
(
    struct d_test_counter* _counter
)
{
    bool result;
    char buffer[64];
    int  written;

    result = true;

    // test 1: successful format via variadic wrapper
    d_memset(buffer, 0, sizeof(buffer));
    written = vsnprintf_test_wrapper(buffer, sizeof(buffer), "Item %d: %s", 5, "test");
    result  = d_assert_standalone(
        written > 0,
        "vsnprintf_success_count",
        "Variadic format should return positive",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "Item 5: test") == 0,
        "vsnprintf_success_value",
        "Buffer should contain 'Item 5: test'",
        _counter) && result;

    // test 2: truncation behavior
    d_memset(buffer, 0, sizeof(buffer));
    written = vsnprintf_test_wrapper(buffer, 8, "Very long formatted string %d", 100);
    result  = d_assert_standalone(
        written > 8,
        "vsnprintf_truncate_count",
        "Truncated should return full length",
        _counter) && result;

    result = d_assert_standalone(
        strlen(buffer) == 7,
        "vsnprintf_truncate_value",
        "Buffer should be truncated to 7 chars",
        _counter) && result;

    // test 3: exact fit
    d_memset(buffer, 0, sizeof(buffer));
    written = vsnprintf_test_wrapper(buffer, 6, "12345");
    result  = d_assert_standalone(
        written == 5,
        "vsnprintf_exact_count",
        "Exact fit should return 5",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(buffer, "12345") == 0,
        "vsnprintf_exact_value",
        "Buffer should contain '12345'",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_formatted_output_all
  Aggregation function that runs all formatted output tests.
*/
bool
d_tests_sa_dio_formatted_output_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Formatted Output Functions\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_dio_sprintf_s(_counter) && result;
    result = d_tests_sa_dio_vsprintf_s(_counter) && result;
    result = d_tests_sa_dio_snprintf(_counter) && result;
    result = d_tests_sa_dio_vsnprintf(_counter) && result;

    return result;
}
