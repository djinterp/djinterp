#include ".\dio_tests_sa.h"


/*
d_tests_sa_dio_perror
  Tests the d_perror error message printing function.
  Tests the following:
  - successful error message printing with prefix
  - error message printing with NULL prefix
  - error message based on current errno
  - no crash on various inputs
  
  Note: d_perror writes to stderr, so these tests mainly verify
  it doesn't crash and accepts various inputs.
*/
bool
d_tests_sa_dio_perror
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: perror with valid prefix (should not crash)
    errno = ENOENT;  // set a known error
    d_perror("test_prefix");
    result = d_assert_standalone(
        true,
        "perror_valid_prefix",
        "perror with valid prefix should not crash",
        _counter) && result;

    // test 2: perror with NULL prefix (should not crash)
    errno = EINVAL;
    d_perror(NULL);
    result = d_assert_standalone(
        true,
        "perror_null_prefix",
        "perror with NULL prefix should not crash",
        _counter) && result;

    // test 3: perror with empty string prefix
    errno = ERANGE;
    d_perror("");
    result = d_assert_standalone(
        true,
        "perror_empty_prefix",
        "perror with empty prefix should not crash",
        _counter) && result;

    // test 4: perror with various errno values
    errno = 0;
    d_perror("zero_errno");
    result = d_assert_standalone(
        true,
        "perror_zero_errno",
        "perror with errno=0 should not crash",
        _counter) && result;

    // test 5: perror with long prefix
    errno = ENOENT;
    d_perror("This is a very long error message prefix that tests perror behavior");
    result = d_assert_standalone(
        true,
        "perror_long_prefix",
        "perror with long prefix should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_dio_feof
  Tests the d_feof end-of-file indicator testing function.
  Tests the following:
  - EOF not set initially
  - EOF set after reading past end
  - EOF cleared by rewind
  - EOF cleared by clearerr
  
  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_feof
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    int   eof_status;
    char  buffer[16];
    char  temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_feof_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: EOF not set initially
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "content");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        eof_status = d_feof(temp_file);
        result     = d_assert_standalone(
            eof_status == 0,
            "feof_not_set_initially",
            "EOF should not be set initially",
            _counter) && result;

        fclose(temp_file);
    }

    // test 3: EOF set after reading past end
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "AB");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        // read all content
        fread(buffer, 1, 2, temp_file);
        
        // try to read more (should set EOF)
        fgetc(temp_file);

        eof_status = d_feof(temp_file);
        result     = d_assert_standalone(
            eof_status != 0,
            "feof_set_after_read_past_end",
            "EOF should be set after reading past end",
            _counter) && result;

        fclose(temp_file);
    }

    // test 4: EOF cleared by rewind
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "XY");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        // set EOF
        fread(buffer, 1, 2, temp_file);
        fgetc(temp_file);

        // rewind should clear EOF
        d_rewind(temp_file);

        eof_status = d_feof(temp_file);
        result     = d_assert_standalone(
            eof_status == 0,
            "feof_cleared_by_rewind",
            "EOF should be cleared by rewind",
            _counter) && result;

        fclose(temp_file);
    }

    // test 5: EOF cleared by clearerr
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "12");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        // set EOF
        fread(buffer, 1, 2, temp_file);
        fgetc(temp_file);

        // clearerr should clear EOF
        d_clearerr(temp_file);

        eof_status = d_feof(temp_file);
        result     = d_assert_standalone(
            eof_status == 0,
            "feof_cleared_by_clearerr",
            "EOF should be cleared by clearerr",
            _counter) && result;

        fclose(temp_file);
    }

    // test 6: EOF not set on successful read
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "ABCDEF");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        fread(buffer, 1, 3, temp_file);  // read only part

        eof_status = d_feof(temp_file);
        result     = d_assert_standalone(
            eof_status == 0,
            "feof_not_set_partial_read",
            "EOF should not be set on partial read",
            _counter) && result;

        fclose(temp_file);
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_ferror
  Tests the d_ferror error indicator testing function.
  Tests the following:
  - error not set initially
  - error set after invalid operation
  - error cleared by clearerr
  - error cleared by rewind
  
  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_ferror
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    int   error_status;
    char  temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_ferror_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: error not set initially
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        error_status = d_ferror(temp_file);
        result       = d_assert_standalone(
            error_status == 0,
            "ferror_not_set_initially",
            "Error should not be set initially",
            _counter) && result;

        fclose(temp_file);
    }

    // test 3: error set after invalid write (write to read-only file)
    temp_file = d_fopen(temp_filename, "r");
    if (temp_file)
    {
        // attempt to write to read-only file
        fputc('X', temp_file);

        error_status = d_ferror(temp_file);
        result       = d_assert_standalone(
            error_status != 0,
            "ferror_set_invalid_write",
            "Error should be set after invalid write",
            _counter) && result;

        fclose(temp_file);
    }

    // test 4: error cleared by clearerr
    temp_file = d_fopen(temp_filename, "r");
    if (temp_file)
    {
        // set error
        fputc('X', temp_file);

        // clear error
        d_clearerr(temp_file);

        error_status = d_ferror(temp_file);
        result       = d_assert_standalone(
            error_status == 0,
            "ferror_cleared_by_clearerr",
            "Error should be cleared by clearerr",
            _counter) && result;

        fclose(temp_file);
    }

    // test 5: error cleared by rewind
    temp_file = d_fopen(temp_filename, "r");
    if (temp_file)
    {
        // set error
        fputc('X', temp_file);

        // rewind should clear error
        d_rewind(temp_file);

        error_status = d_ferror(temp_file);
        result       = d_assert_standalone(
            error_status == 0,
            "ferror_cleared_by_rewind",
            "Error should be cleared by rewind",
            _counter) && result;

        fclose(temp_file);
    }

    // test 6: error not set on valid operations
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "valid data");
        fflush(temp_file);

        error_status = d_ferror(temp_file);
        result       = d_assert_standalone(
            error_status == 0,
            "ferror_not_set_valid_ops",
            "Error should not be set on valid operations",
            _counter) && result;

        fclose(temp_file);
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_clearerr
  Tests the d_clearerr error indicator clearing function.
  Tests the following:
  - clearing EOF indicator
  - clearing error indicator
  - clearing both indicators simultaneously
  - no effect when no indicators set
  
  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_clearerr
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    char  buffer[16];
    char  temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_clearerr_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: clearing EOF indicator
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "AB");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        // set EOF
        fread(buffer, 1, 2, temp_file);
        fgetc(temp_file);

        // verify EOF is set
        result = d_assert_standalone(
            d_feof(temp_file) != 0,
            "clearerr_eof_set_before",
            "EOF should be set before clearerr",
            _counter) && result;

        // clear indicators
        d_clearerr(temp_file);

        result = d_assert_standalone(
            d_feof(temp_file) == 0,
            "clearerr_eof_cleared",
            "EOF should be cleared after clearerr",
            _counter) && result;

        fclose(temp_file);
    }

    // test 3: clearing error indicator
    temp_file = d_fopen(temp_filename, "r");
    if (temp_file)
    {
        // set error
        fputc('X', temp_file);

        // verify error is set
        result = d_assert_standalone(
            d_ferror(temp_file) != 0,
            "clearerr_error_set_before",
            "Error should be set before clearerr",
            _counter) && result;

        // clear indicators
        d_clearerr(temp_file);

        result = d_assert_standalone(
            d_ferror(temp_file) == 0,
            "clearerr_error_cleared",
            "Error should be cleared after clearerr",
            _counter) && result;

        fclose(temp_file);
    }

    // test 4: clearing both indicators
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "X");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        // set EOF
        fread(buffer, 1, 1, temp_file);
        fgetc(temp_file);

        // close and reopen as read-only to set error too
        fclose(temp_file);
        temp_file = d_fopen(temp_filename, "r");
        if (temp_file)
        {
            // set both EOF and error
            fseek(temp_file, 0, SEEK_END);
            fgetc(temp_file);  // EOF
            fputc('Y', temp_file);  // error

            // clear both
            d_clearerr(temp_file);

            result = d_assert_standalone(
                (d_feof(temp_file) == 0) && (d_ferror(temp_file) == 0),
                "clearerr_both_cleared",
                "Both EOF and error should be cleared",
                _counter) && result;

            fclose(temp_file);
        }
    }

    // test 5: clearerr on stream with no indicators set
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "normal");
        fflush(temp_file);

        // no indicators set
        d_clearerr(temp_file);

        result = d_assert_standalone(
            (d_feof(temp_file) == 0) && (d_ferror(temp_file) == 0),
            "clearerr_no_effect",
            "clearerr should have no effect when no indicators set",
            _counter) && result;

        fclose(temp_file);
    }

    // test 6: multiple clearerr calls
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "T");
        fflush(temp_file);
        fseek(temp_file, 0, SEEK_SET);

        // set EOF
        fread(buffer, 1, 1, temp_file);
        fgetc(temp_file);

        // multiple clears
        d_clearerr(temp_file);
        d_clearerr(temp_file);
        d_clearerr(temp_file);

        result = d_assert_standalone(
            d_feof(temp_file) == 0,
            "clearerr_multiple",
            "Multiple clearerr calls should keep indicators clear",
            _counter) && result;

        fclose(temp_file);
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_error_handling_all
  Aggregation function that runs all error handling tests.
*/
bool
d_tests_sa_dio_error_handling_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Error Handling Functions\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_dio_perror(_counter) && result;
    result = d_tests_sa_dio_feof(_counter) && result;
    result = d_tests_sa_dio_ferror(_counter) && result;
    result = d_tests_sa_dio_clearerr(_counter) && result;

    return result;
}
