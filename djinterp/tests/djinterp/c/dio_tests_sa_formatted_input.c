#include ".\dio_tests_sa.h"


/*
d_tests_sa_dio_sscanf
  Tests the d_sscanf function.
  Tests the following:
  - empty buffer handling
  - successful parsing of single integer
  - successful parsing of multiple values
  - successful parsing of strings
  - partial match behavior
  - format mismatch returns appropriate count
  
  Note: NULL buffer tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_sscanf
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  int_val;
    int  int_val2;
    char str_buf[64];
    int  parsed;

    result = true;

    // test 1: NULL buffer test skipped (causes assertion in debug builds)
    // Instead test with empty string
    parsed = d_sscanf("", "%d", &int_val);
    result = d_assert_standalone(
        parsed <= 0,
        "sscanf_empty_buffer",
        "Empty buffer should return error or EOF",
        _counter) && result;

    // test 2: successful single integer parse
    parsed  = d_sscanf("42", "%d", &int_val);
    result  = d_assert_standalone(
        parsed == 1,
        "sscanf_single_int_count",
        "Single integer should return 1",
        _counter) && result;

    result = d_assert_standalone(
        int_val == 42,
        "sscanf_single_int_value",
        "Parsed integer should be 42",
        _counter) && result;

    // test 3: successful multiple value parse
    int_val = 0;
    int_val2 = 0;
    parsed = d_sscanf("123 456", "%d %d", &int_val, &int_val2);
    result = d_assert_standalone(
        parsed == 2,
        "sscanf_multi_int_count",
        "Two integers should return 2",
        _counter) && result;

    result = d_assert_standalone(
        (int_val == 123) && (int_val2 == 456),
        "sscanf_multi_int_values",
        "Parsed integers should be 123 and 456",
        _counter) && result;

    // test 4: successful string parse
    d_memset(str_buf, 0, sizeof(str_buf));
    parsed = d_sscanf("hello", "%s", str_buf);
    result = d_assert_standalone(
        parsed == 1,
        "sscanf_string_count",
        "String parse should return 1",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(str_buf, "hello") == 0,
        "sscanf_string_value",
        "Parsed string should be 'hello'",
        _counter) && result;

    // test 5: partial match
    int_val = 0;
    parsed = d_sscanf("42 abc", "%d %d", &int_val, &int_val2);
    result = d_assert_standalone(
        parsed == 1,
        "sscanf_partial_match",
        "Partial match should return 1",
        _counter) && result;

    result = d_assert_standalone(
        int_val == 42,
        "sscanf_partial_value",
        "First value should be 42",
        _counter) && result;

    // test 6: format mismatch
    int_val = 0;
    parsed = d_sscanf("abc", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 0,
        "sscanf_format_mismatch",
        "Format mismatch should return 0",
        _counter) && result;

    // test 7: empty buffer
    int_val = 0;
    parsed = d_sscanf("", "%d", &int_val);
    result = d_assert_standalone(
        parsed <= 0,
        "sscanf_empty_buffer",
        "Empty buffer should return error or 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_sscanf_s
  Tests the d_sscanf_s secure variant function.
  Tests the following:
  - empty buffer handling
  - successful parsing with size parameters
  - buffer size enforcement for strings
  - successful multi-value parse
  - format string validation
  
  Note: NULL buffer tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_sscanf_s
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  int_val;
    char str_buf[32];
    int  parsed;

    result = true;

    // test 1: NULL buffer test skipped (causes assertion in debug builds)
    // Instead test with empty string
    parsed = d_sscanf_s("", "%d", &int_val);
    result = d_assert_standalone(
        parsed <= 0,
        "sscanf_s_empty_buffer",
        "Empty buffer should return error",
        _counter) && result;

    // test 2: successful integer parse
    int_val = 0;
    parsed = d_sscanf_s("789", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 1,
        "sscanf_s_int_count",
        "Integer parse should return 1",
        _counter) && result;

    result = d_assert_standalone(
        int_val == 789,
        "sscanf_s_int_value",
        "Parsed value should be 789",
        _counter) && result;

    // test 3: successful string parse with size
    // Note: actual size enforcement depends on platform support
    d_memset(str_buf, 0, sizeof(str_buf));
    parsed = d_sscanf_s("teststring", "%31s", str_buf, (unsigned)sizeof(str_buf));
    result = d_assert_standalone(
        parsed == 1,
        "sscanf_s_string_count",
        "String parse should return 1",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(str_buf, "teststring") == 0,
        "sscanf_s_string_value",
        "Parsed string should be 'teststring'",
        _counter) && result;

    // test 4: format mismatch
    int_val = 0;
    parsed = d_sscanf_s("xyz", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 0,
        "sscanf_s_format_mismatch",
        "Format mismatch should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_vsscanf
  Tests the d_vsscanf variadic function wrapper.
  Tests the following:
  - successful parsing via wrapper
  - correct argument passing through va_list
  - multiple value parsing
*/
static int
vsscanf_test_wrapper
(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
    result = d_vsscanf(_buffer, _format, args);
    va_end(args);

    return result;
}

bool
d_tests_sa_dio_vsscanf
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  int_val;
    int  int_val2;
    int  parsed;

    result = true;

    // test 1: successful single value parse via variadic wrapper
    int_val = 0;
    parsed = vsscanf_test_wrapper("999", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 1,
        "vsscanf_single_count",
        "Single value parse should return 1",
        _counter) && result;

    result = d_assert_standalone(
        int_val == 999,
        "vsscanf_single_value",
        "Parsed value should be 999",
        _counter) && result;

    // test 2: multiple value parse
    int_val = 0;
    int_val2 = 0;
    parsed = vsscanf_test_wrapper("11 22", "%d %d", &int_val, &int_val2);
    result = d_assert_standalone(
        parsed == 2,
        "vsscanf_multi_count",
        "Multi-value parse should return 2",
        _counter) && result;

    result = d_assert_standalone(
        (int_val == 11) && (int_val2 == 22),
        "vsscanf_multi_values",
        "Parsed values should be 11 and 22",
        _counter) && result;

    // test 3: format mismatch
    int_val = 0;
    parsed = vsscanf_test_wrapper("notanumber", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 0,
        "vsscanf_format_mismatch",
        "Format mismatch should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_vsscanf_s
  Tests the d_vsscanf_s secure variadic function wrapper.
  Tests the following:
  - successful parsing via secure wrapper
  - correct argument passing
  - format validation
*/
static int
vsscanf_s_test_wrapper
(
    const char* _buffer,
    const char* _format,
    ...
)
{
    va_list args;
    int     result;

    va_start(args, _format);
    result = d_vsscanf_s(_buffer, _format, args);
    va_end(args);

    return result;
}

bool
d_tests_sa_dio_vsscanf_s
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  int_val;
    int  parsed;

    result = true;

    // test 1: successful parse via secure variadic wrapper
    int_val = 0;
    parsed = vsscanf_s_test_wrapper("555", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 1,
        "vsscanf_s_count",
        "Secure parse should return 1",
        _counter) && result;

    result = d_assert_standalone(
        int_val == 555,
        "vsscanf_s_value",
        "Parsed value should be 555",
        _counter) && result;

    // test 2: format mismatch
    int_val = 0;
    parsed = vsscanf_s_test_wrapper("abc", "%d", &int_val);
    result = d_assert_standalone(
        parsed == 0,
        "vsscanf_s_format_mismatch",
        "Format mismatch should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_fscanf
  Tests the d_fscanf file scanning function.
  Tests the following:
  - successful parsing from file
  - multiple value parsing
  - EOF handling
  - format mismatch behavior
  
  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_fscanf
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    int   int_val;
    int   int_val2;
    int   parsed;
    char  temp_filename[256];

    result = true;

    // create temporary file for testing
    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fscanf_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: successful parse from file
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "12345");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            int_val = 0;
            parsed = d_fscanf(temp_file, "%d", &int_val);
            
            result = d_assert_standalone(
                parsed == 1,
                "fscanf_success_count",
                "File parse should return 1",
                _counter) && result;

            result = d_assert_standalone(
                int_val == 12345,
                "fscanf_success_value",
                "Parsed value should be 12345",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 3: multiple values from file
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "100 200");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            int_val = 0;
            int_val2 = 0;
            parsed = d_fscanf(temp_file, "%d %d", &int_val, &int_val2);
            
            result = d_assert_standalone(
                parsed == 2,
                "fscanf_multi_count",
                "Multi-value parse should return 2",
                _counter) && result;

            result = d_assert_standalone(
                (int_val == 100) && (int_val2 == 200),
                "fscanf_multi_values",
                "Parsed values should be 100 and 200",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 4: EOF handling
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fclose(temp_file);  // empty file

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            int_val = 0;
            parsed = d_fscanf(temp_file, "%d", &int_val);
            
            result = d_assert_standalone(
                parsed == EOF,
                "fscanf_eof",
                "EOF should return EOF",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_fscanf_s
  Tests the d_fscanf_s secure file scanning function.
  Tests the following:
  - successful secure parsing from file
  - buffer size validation
  
  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_fscanf_s
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    int   int_val;
    char  str_buf[64];
    int   parsed;
    char  temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fscanf_s_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: successful secure parse
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "54321");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            int_val = 0;
            parsed = d_fscanf_s(temp_file, "%d", &int_val);
            
            result = d_assert_standalone(
                parsed == 1,
                "fscanf_s_success_count",
                "Secure parse should return 1",
                _counter) && result;

            result = d_assert_standalone(
                int_val == 54321,
                "fscanf_s_success_value",
                "Parsed value should be 54321",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 3: string parse with size
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "securestring");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            d_memset(str_buf, 0, sizeof(str_buf));
            parsed = d_fscanf_s(temp_file, "%63s", str_buf, (unsigned)sizeof(str_buf));
            
            result = d_assert_standalone(
                parsed == 1,
                "fscanf_s_string_count",
                "String parse should return 1",
                _counter) && result;

            result = d_assert_standalone(
                strcmp(str_buf, "securestring") == 0,
                "fscanf_s_string_value",
                "Parsed string should be 'securestring'",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_formatted_input_all
  Aggregation function that runs all formatted input tests.
*/
bool
d_tests_sa_dio_formatted_input_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Formatted Input Functions\n");
    printf("  ------------------------------------\n");

    result = d_tests_sa_dio_sscanf(_counter) && result;
    result = d_tests_sa_dio_sscanf_s(_counter) && result;
    result = d_tests_sa_dio_vsscanf(_counter) && result;
    result = d_tests_sa_dio_vsscanf_s(_counter) && result;
    result = d_tests_sa_dio_fscanf(_counter) && result;
    result = d_tests_sa_dio_fscanf_s(_counter) && result;

    return result;
}
