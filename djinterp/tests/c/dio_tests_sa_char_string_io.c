#include ".\dio_tests_sa.h"


/*
d_tests_sa_dio_gets_s
  Tests the d_gets_s secure line input function.
  Tests the following:
  - NULL buffer rejection
  - zero size rejection
  - successful read from redirected stdin
  - buffer size enforcement
  - newline removal behavior
  
  Note: Since gets_s reads from stdin, these tests use d_freopen_s to redirect
  stdin to a temporary file for automated testing.
*/
bool
d_tests_sa_dio_gets_s
(
    struct d_test_counter* _counter
)
{
    bool  result;
    char  buffer[128];
    char* read_result;
    FILE* redirected;
    char  temp_filename[256];

    result = true;

    // test 1: NULL buffer should fail
    read_result = d_gets_s(NULL, 10);
    result      = d_assert_standalone(
        read_result == NULL,
        "gets_s_null_buffer",
        "NULL buffer should return NULL",
        _counter) && result;

    // test 2: zero size should fail
    read_result = d_gets_s(buffer, 0);
    result      = d_assert_standalone(
        read_result == NULL,
        "gets_s_zero_size",
        "Zero size should return NULL",
        _counter) && result;

    // test 3: successful read (simulated via stdin redirection using d_freopen_s)
    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_gets_s_temp.txt");
    
    // create test file
    redirected = d_fopen(temp_filename, "w");
    if (redirected)
    {
        fprintf(redirected, "test line\n");
        fclose(redirected);
    }

    // redirect stdin to temp file
    redirected = NULL;
    if (d_freopen_s(&redirected, temp_filename, "r", stdin) == 0 && redirected != NULL)
    {
        d_memset(buffer, 0, sizeof(buffer));
        read_result = d_gets_s(buffer, sizeof(buffer));
        
        result = d_assert_standalone(
            read_result != NULL,
            "gets_s_success_ptr",
            "Successful read should return non-NULL",
            _counter) && result;

        result = d_assert_standalone(
            strcmp(buffer, "test line") == 0,
            "gets_s_success_value",
            "Buffer should contain 'test line' without newline",
            _counter) && result;
    }

    // test 4: small buffer truncation
    // create new test file
    redirected = d_fopen(temp_filename, "w");
    if (redirected)
    {
        fprintf(redirected, "This is a very long line that will be truncated\n");
        fclose(redirected);
    }

    // redirect stdin again
    redirected = NULL;
    if (d_freopen_s(&redirected, temp_filename, "r", stdin) == 0 && redirected != NULL)
    {
        char small_buffer[10];
        
        d_memset(small_buffer, 0, sizeof(small_buffer));
        read_result = d_gets_s(small_buffer, sizeof(small_buffer));
        
        // should either succeed with truncation or fail
        result = d_assert_standalone(
            (read_result == NULL) || (strlen(small_buffer) < sizeof(small_buffer)),
            "gets_s_truncate",
            "Small buffer should truncate or fail",
            _counter) && result;
    }

    // Note: We don't restore stdin as it's not portable to do so reliably
    // The test framework will handle stdin state

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_fputs
  Tests the d_fputs string output function.
  Tests the following:
  - successful write to file
  - write without newline appending
  - return value correctness
  
  Note: NULL stream and NULL string tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_fputs
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    int   write_result;
    char  temp_filename[256];
    char  read_buffer[128];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fputs_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: NULL string test skipped (causes assertion in debug builds)

    // test 3: successful write
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        write_result = d_fputs("Hello, fputs!", temp_file);
        result       = d_assert_standalone(
            write_result >= 0,
            "fputs_success_return",
            "Successful write should return non-negative",
            _counter) && result;

        fclose(temp_file);

        // verify written content
        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            d_memset(read_buffer, 0, sizeof(read_buffer));
            d_fgets(read_buffer, sizeof(read_buffer), temp_file);
            
            result = d_assert_standalone(
                strcmp(read_buffer, "Hello, fputs!") == 0,
                "fputs_success_content",
                "File should contain 'Hello, fputs!'",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 4: empty string
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        write_result = d_fputs("", temp_file);
        result       = d_assert_standalone(
            write_result >= 0,
            "fputs_empty_return",
            "Empty string should return non-negative",
            _counter) && result;

        fclose(temp_file);
    }

    // test 5: multiple writes
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        d_fputs("First ", temp_file);
        d_fputs("Second ", temp_file);
        d_fputs("Third", temp_file);
        fclose(temp_file);

        // verify concatenated content
        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            d_memset(read_buffer, 0, sizeof(read_buffer));
            d_fgets(read_buffer, sizeof(read_buffer), temp_file);
            
            result = d_assert_standalone(
                strcmp(read_buffer, "First Second Third") == 0,
                "fputs_multi_content",
                "File should contain 'First Second Third'",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_fgets
  Tests the d_fgets line input function.
  Tests the following:
  - successful line read
  - newline preservation
  - partial line read with buffer limit
  - EOF handling
  
  Note: NULL buffer and NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_fgets
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    char  buffer[128];
    char* read_result;
    char  temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fgets_temp.txt");

    // test 1: NULL buffer test skipped (causes assertion in debug builds)

    // test 2: NULL stream test skipped (causes assertion in debug builds)

    // test 3: successful line read with newline
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "test line\n");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            d_memset(buffer, 0, sizeof(buffer));
            read_result = d_fgets(buffer, sizeof(buffer), temp_file);
            
            result = d_assert_standalone(
                read_result != NULL,
                "fgets_success_ptr",
                "Successful read should return non-NULL",
                _counter) && result;

            result = d_assert_standalone(
                strcmp(buffer, "test line\n") == 0,
                "fgets_success_value",
                "Buffer should contain 'test line\\n'",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 4: partial read with buffer limit
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "This is a very long line\n");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            char small_buffer[10];
            
            d_memset(small_buffer, 0, sizeof(small_buffer));
            read_result = d_fgets(small_buffer, sizeof(small_buffer), temp_file);
            
            result = d_assert_standalone(
                read_result != NULL,
                "fgets_partial_ptr",
                "Partial read should return non-NULL",
                _counter) && result;

            result = d_assert_standalone(
                strlen(small_buffer) == 9,
                "fgets_partial_length",
                "Partial read should be 9 chars",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 5: EOF handling
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fclose(temp_file);  // empty file

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            d_memset(buffer, 0, sizeof(buffer));
            read_result = d_fgets(buffer, sizeof(buffer), temp_file);
            
            result = d_assert_standalone(
                read_result == NULL,
                "fgets_eof",
                "EOF should return NULL",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 6: line without newline
    temp_file = d_fopen(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "no newline");
        fclose(temp_file);

        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            d_memset(buffer, 0, sizeof(buffer));
            read_result = d_fgets(buffer, sizeof(buffer), temp_file);
            
            result = d_assert_standalone(
                read_result != NULL,
                "fgets_no_newline_ptr",
                "Line without newline should return non-NULL",
                _counter) && result;

            result = d_assert_standalone(
                strcmp(buffer, "no newline") == 0,
                "fgets_no_newline_value",
                "Buffer should contain 'no newline'",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_char_string_io_all
  Aggregation function that runs all character and string I/O tests.
*/
bool
d_tests_sa_dio_char_string_io_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Character and String I/O Functions\n");
    printf("  ---------------------------------------------\n");

    result = d_tests_sa_dio_gets_s(_counter) && result;
    result = d_tests_sa_dio_fputs(_counter)  && result;
    result = d_tests_sa_dio_fgets(_counter)  && result;

    return result;
}