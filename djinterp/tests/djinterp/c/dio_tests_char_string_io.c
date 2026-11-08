/*******************************************************************************
* djinterp [test]                                     dio_tests_char_string_io.c
*
*
* path:      /tests/djinterp/c/dio_tests_char_string_io.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dio_tests.h"


/*
d_tests_dio_gets_s
  Tests the d_gets_s secure line input function.
  Tests the following:
  - NULL buffer rejection
  - zero size rejection
  - successful read from redirected stdin
  - buffer size enforcement
  - newline removal behavior

  Note: Since gets_s reads from stdin, these tests use d_file_reopen_stream_s to redirect
  stdin to a temporary file for automated testing.
*/
bool d_tests_dio_gets_s(
    struct dj_test_suite* _suite
)
{
    bool  result;
    char  buffer[128];
    char* read_result;
    FILE* redirected;
    char  temp_filename[256];

    d_test_suite_test(_suite, "dio_gets_s");

    result = true;

    // test 1: NULL buffer should fail
    read_result = d_gets_s(NULL, 10);
    result      = D_TEST_CHECK_MSG(_suite,
                                   "gets_s_null_buffer",
                                   read_result == NULL,
                                   "NULL buffer should return NULL") && result;

    // test 2: zero size should fail
    read_result = d_gets_s(buffer, 0);
    result      = D_TEST_CHECK_MSG(_suite,
                                   "gets_s_zero_size",
                                   read_result == NULL,
                                   "Zero size should return NULL") && result;

    // test 3: successful read (simulated via stdin redirection using d_file_reopen_stream_s)
    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_gets_s_temp.txt");

    // create test file
    redirected = d_file_open_stream(temp_filename, "w");
    if (redirected)
    {
        fprintf(redirected, "test line\n");
        fclose(redirected);
    }

    // redirect stdin to temp file
    redirected = NULL;
    if (d_file_reopen_stream_s(&redirected, temp_filename, "r", stdin) == 0 && redirected != NULL)
    {
        d_memset(buffer, 0, sizeof(buffer));
        read_result = d_gets_s(buffer, sizeof(buffer));

        result = D_TEST_CHECK_MSG(_suite,
                                  "gets_s_success_ptr",
                                  read_result != NULL,
                                  "Successful read should return non-NULL") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "gets_s_success_value",
                                  strcmp(buffer, "test line") == 0,
                                  "Buffer should contain 'test line' without newline") && result;
    }

    // test 4: small buffer truncation
    // create new test file
    redirected = d_file_open_stream(temp_filename, "w");
    if (redirected)
    {
        fprintf(redirected, "This is a very long line that will be truncated\n");
        fclose(redirected);
    }

    // redirect stdin again
    redirected = NULL;
    if (d_file_reopen_stream_s(&redirected, temp_filename, "r", stdin) == 0 && redirected != NULL)
    {
        char small_buffer[10];

        d_memset(small_buffer, 0, sizeof(small_buffer));
        read_result = d_gets_s(small_buffer, sizeof(small_buffer));

        // should either succeed with truncation or fail
        result = D_TEST_CHECK_MSG(_suite,
                                  "gets_s_truncate",
                                  (read_result == NULL) || (strlen(small_buffer) < sizeof(small_buffer)),
                                  "Small buffer should truncate or fail") && result;
    }

    // Note: we don't restore stdin as it's not portable to do so reliably
    // The test framework will handle stdin state?
    // CORRECTION: restore stdin to console
#if defined(D_ENV_PLATFORM_WINDOWS)
    {
        FILE* restored = NULL;
        d_file_reopen_stream_s(&restored, "CON", "r", stdin);
    }
#else
    {
        FILE* restored = NULL;
        d_file_reopen_stream_s(&restored, "/dev/tty", "r", stdin);
    }
#endif

    // cleanup
    d_file_remove(temp_filename);

    return result;
}


/*
d_tests_dio_fputs
  Tests the d_fputs string output function.
  Tests the following:
  - successful write to file
  - write without newline appending
  - return value correctness

  Note: NULL stream and NULL string tests are skipped as they cause assertions in debug builds.
*/
bool d_tests_dio_fputs(
    struct dj_test_suite* _suite
)
{
    bool  result;
    FILE* temp_file;
    int   write_result;
    char  temp_filename[256];
    char  read_buffer[128];

    d_test_suite_test(_suite, "dio_fputs");

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fputs_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: NULL string test skipped (causes assertion in debug builds)

    // test 3: successful write
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        write_result = d_fputs("Hello, fputs!", temp_file);
        result       = D_TEST_CHECK_MSG(_suite,
                                        "fputs_success_return",
                                        write_result >= 0,
                                        "Successful write should return non-negative") && result;

        fclose(temp_file);

        // verify written content
        temp_file = d_file_open_stream(temp_filename, "r");
        if (temp_file)
        {
            d_memset(read_buffer, 0, sizeof(read_buffer));
            d_fgets(read_buffer, sizeof(read_buffer), temp_file);

            result = D_TEST_CHECK_MSG(_suite,
                                      "fputs_success_content",
                                      strcmp(read_buffer, "Hello, fputs!") == 0,
                                      "File should contain 'Hello, fputs!'") && result;

            fclose(temp_file);
        }
    }

    // test 4: empty string
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        write_result = d_fputs("", temp_file);
        result       = D_TEST_CHECK_MSG(_suite,
                                        "fputs_empty_return",
                                        write_result >= 0,
                                        "Empty string should return non-negative") && result;

        fclose(temp_file);
    }

    // test 5: multiple writes
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        d_fputs("First ", temp_file);
        d_fputs("Second ", temp_file);
        d_fputs("Third", temp_file);
        fclose(temp_file);

        // verify concatenated content
        temp_file = d_file_open_stream(temp_filename, "r");
        if (temp_file)
        {
            d_memset(read_buffer, 0, sizeof(read_buffer));
            d_fgets(read_buffer, sizeof(read_buffer), temp_file);

            result = D_TEST_CHECK_MSG(_suite,
                                      "fputs_multi_content",
                                      strcmp(read_buffer, "First Second Third") == 0,
                                      "File should contain 'First Second Third'") && result;

            fclose(temp_file);
        }
    }

    // cleanup
    d_file_remove(temp_filename);

    return result;
}


/*
d_tests_dio_fgets
  Tests the d_fgets line input function.
  Tests the following:
  - successful line read
  - newline preservation
  - partial line read with buffer limit
  - EOF handling

  Note: NULL buffer and NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool d_tests_dio_fgets(
    struct dj_test_suite* _suite
)
{
    bool  result;
    FILE* temp_file;
    char  buffer[128];
    char* read_result;
    char  temp_filename[256];

    d_test_suite_test(_suite, "dio_fgets");

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fgets_temp.txt");

    // test 1: NULL buffer test skipped (causes assertion in debug builds)

    // test 2: NULL stream test skipped (causes assertion in debug builds)

    // test 3: successful line read with newline
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "test line\n");
        fclose(temp_file);

        temp_file = d_file_open_stream(temp_filename, "r");
        if (temp_file)
        {
            d_memset(buffer, 0, sizeof(buffer));
            read_result = d_fgets(buffer, sizeof(buffer), temp_file);

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_success_ptr",
                                      read_result != NULL,
                                      "Successful read should return non-NULL") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_success_value",
                                      strcmp(buffer, "test line\n") == 0,
                                      "Buffer should contain 'test line\\n'") && result;

            fclose(temp_file);
        }
    }

    // test 4: partial read with buffer limit
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "This is a very long line\n");
        fclose(temp_file);

        temp_file = d_file_open_stream(temp_filename, "r");
        if (temp_file)
        {
            char small_buffer[10];

            d_memset(small_buffer, 0, sizeof(small_buffer));
            read_result = d_fgets(small_buffer, sizeof(small_buffer), temp_file);

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_partial_ptr",
                                      read_result != NULL,
                                      "Partial read should return non-NULL") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_partial_length",
                                      strlen(small_buffer) == 9,
                                      "Partial read should be 9 chars") && result;

            fclose(temp_file);
        }
    }

    // test 5: EOF handling
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        fclose(temp_file);  // empty file

        temp_file = d_file_open_stream(temp_filename, "r");
        if (temp_file)
        {
            d_memset(buffer, 0, sizeof(buffer));
            read_result = d_fgets(buffer, sizeof(buffer), temp_file);

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_eof",
                                      read_result == NULL,
                                      "EOF should return NULL") && result;

            fclose(temp_file);
        }
    }

    // test 6: line without newline
    temp_file = d_file_open_stream(temp_filename, "w");
    if (temp_file)
    {
        fprintf(temp_file, "no newline");
        fclose(temp_file);

        temp_file = d_file_open_stream(temp_filename, "r");
        if (temp_file)
        {
            d_memset(buffer, 0, sizeof(buffer));
            read_result = d_fgets(buffer, sizeof(buffer), temp_file);

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_no_newline_ptr",
                                      read_result != NULL,
                                      "Line without newline should return non-NULL") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "fgets_no_newline_value",
                                      strcmp(buffer, "no newline") == 0,
                                      "Buffer should contain 'no newline'") && result;

            fclose(temp_file);
        }
    }

    // cleanup
    d_file_remove(temp_filename);

    return result;
}


/*
d_tests_dio_char_string_io_all
  Aggregation function that runs all character and string I/O tests.
*/
bool d_tests_dio_char_string_io_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "dio_char_string_io_all");

    result = true;

    result = d_tests_dio_gets_s(_suite) && result;
    result = d_tests_dio_fputs(_suite)  && result;
    result = d_tests_dio_fgets(_suite)  && result;

    return result;
}
