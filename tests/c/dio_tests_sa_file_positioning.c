#include ".\dio_tests_sa.h"


/*
d_tests_sa_dio_fgetpos
  Tests the d_fgetpos file position retrieval function.
  Tests the following:
  - successful position retrieval at start
  - successful position retrieval after write
  - position retrieval after seek
  
  Note: NULL stream and NULL pointer tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_fgetpos
(
    struct d_test_counter* _counter
)
{
    bool    result;
    FILE*   temp_file;
    d_off_t pos;
    d_off_t pos2;
    int     get_result;
    char    temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fgetpos_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: NULL position pointer test skipped (causes assertion in debug builds)

    // test 3: successful position get at start
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        get_result = d_fgetpos(temp_file, &pos);
        result     = d_assert_standalone(
            get_result == 0,
            "fgetpos_start_return",
            "Position get at start should return 0",
            _counter) && result;

        result = d_assert_standalone(
            pos == 0,
            "fgetpos_start_value",
            "Position at start should be 0",
            _counter) && result;

        fclose(temp_file);
    }

    // test 4: position after write
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "1234567890");
        fflush(temp_file);

        get_result = d_fgetpos(temp_file, &pos);
        result     = d_assert_standalone(
            get_result == 0,
            "fgetpos_after_write_return",
            "Position get after write should return 0",
            _counter) && result;

        result = d_assert_standalone(
            pos == 10,
            "fgetpos_after_write_value",
            "Position after writing 10 bytes should be 10",
            _counter) && result;

        fclose(temp_file);
    }

    // test 5: position after seek
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "0123456789");
        fflush(temp_file);
        fseek(temp_file, 5, SEEK_SET);

        get_result = d_fgetpos(temp_file, &pos);
        result     = d_assert_standalone(
            get_result == 0,
            "fgetpos_after_seek_return",
            "Position get after seek should return 0",
            _counter) && result;

        result = d_assert_standalone(
            pos == 5,
            "fgetpos_after_seek_value",
            "Position after seek to 5 should be 5",
            _counter) && result;

        fclose(temp_file);
    }

    // test 6: multiple position gets
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "ABC");
        fflush(temp_file);

        d_fgetpos(temp_file, &pos);
        
        fprintf(temp_file, "DEF");
        fflush(temp_file);
        
        d_fgetpos(temp_file, &pos2);

        result = d_assert_standalone(
            pos2 > pos,
            "fgetpos_sequential",
            "Second position should be greater than first",
            _counter) && result;

        result = d_assert_standalone(
            (pos2 - pos) == 3,
            "fgetpos_sequential_diff",
            "Position difference should be 3",
            _counter) && result;

        fclose(temp_file);
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_fsetpos
  Tests the d_fsetpos file position setting function.
  Tests the following:
  - successful position set to start
  - successful position set to middle
  - position set followed by read verification
  - position set followed by write verification
  
  Note: NULL stream and NULL pointer tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_fsetpos
(
    struct d_test_counter* _counter
)
{
    bool    result;
    FILE*   temp_file;
    d_off_t pos;
    int     set_result;
    char    buffer[16];
    char    temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_fsetpos_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: NULL position pointer test skipped (causes assertion in debug builds)

    // test 3: successful position set to start
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "0123456789");
        fflush(temp_file);

        pos = 0;
        set_result = d_fsetpos(temp_file, &pos);
        result     = d_assert_standalone(
            set_result == 0,
            "fsetpos_start_return",
            "Position set to start should return 0",
            _counter) && result;

        // verify position by reading
        d_memset(buffer, 0, sizeof(buffer));
        fread(buffer, 1, 1, temp_file);
        result = d_assert_standalone(
            buffer[0] == '0',
            "fsetpos_start_verify",
            "After set to start, should read '0'",
            _counter) && result;

        fclose(temp_file);
    }

    // test 4: position set to middle
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "ABCDEFGHIJ");
        fflush(temp_file);

        pos = 5;
        set_result = d_fsetpos(temp_file, &pos);
        result     = d_assert_standalone(
            set_result == 0,
            "fsetpos_middle_return",
            "Position set to middle should return 0",
            _counter) && result;

        // verify position by reading
        d_memset(buffer, 0, sizeof(buffer));
        fread(buffer, 1, 1, temp_file);
        result = d_assert_standalone(
            buffer[0] == 'F',
            "fsetpos_middle_verify",
            "After set to 5, should read 'F'",
            _counter) && result;

        fclose(temp_file);
    }

    // test 5: get position, write, then restore position
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "1234567890");
        fflush(temp_file);
        
        fseek(temp_file, 3, SEEK_SET);
        d_fgetpos(temp_file, &pos);  // save position at 3

        fprintf(temp_file, "XYZ");
        fflush(temp_file);

        // restore to saved position
        set_result = d_fsetpos(temp_file, &pos);
        result     = d_assert_standalone(
            set_result == 0,
            "fsetpos_restore_return",
            "Position restore should return 0",
            _counter) && result;

        // verify position
        d_memset(buffer, 0, sizeof(buffer));
        fread(buffer, 1, 3, temp_file);
        result = d_assert_standalone(
            strncmp(buffer, "XYZ", 3) == 0,
            "fsetpos_restore_verify",
            "After restore, should read 'XYZ'",
            _counter) && result;

        fclose(temp_file);
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_rewind
  Tests the d_rewind file rewind function.
  Tests the following:
  - successful rewind to start
  - rewind after write
  - rewind clears error indicators
  - rewind followed by read verification
  
  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool
d_tests_sa_dio_rewind
(
    struct d_test_counter* _counter
)
{
    bool  result;
    FILE* temp_file;
    char  buffer[16];
    char  temp_filename[256];

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_rewind_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: successful rewind after write
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "REWINDTEST");
        fflush(temp_file);

        d_rewind(temp_file);

        // verify position by reading
        d_memset(buffer, 0, sizeof(buffer));
        fread(buffer, 1, 6, temp_file);
        result = d_assert_standalone(
            strncmp(buffer, "REWIND", 6) == 0,
            "rewind_after_write",
            "After rewind, should read 'REWIND'",
            _counter) && result;

        fclose(temp_file);
    }

    // test 3: rewind from middle position
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "0123456789");
        fflush(temp_file);
        fseek(temp_file, 5, SEEK_SET);

        d_rewind(temp_file);

        // verify position
        d_memset(buffer, 0, sizeof(buffer));
        fread(buffer, 1, 1, temp_file);
        result = d_assert_standalone(
            buffer[0] == '0',
            "rewind_from_middle",
            "After rewind from middle, should read '0'",
            _counter) && result;

        fclose(temp_file);
    }

    // test 4: rewind clears error indicators
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "TEST");
        fflush(temp_file);

        // try to read past EOF to set error indicator
        fseek(temp_file, 0, SEEK_END);
        fgetc(temp_file);  // should set EOF indicator

        d_rewind(temp_file);

        result = d_assert_standalone(
            d_feof(temp_file) == 0,
            "rewind_clears_eof",
            "Rewind should clear EOF indicator",
            _counter) && result;

        result = d_assert_standalone(
            d_ferror(temp_file) == 0,
            "rewind_clears_error",
            "Rewind should clear error indicator",
            _counter) && result;

        fclose(temp_file);
    }

    // test 5: multiple rewinds
    temp_file = d_fopen(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "ABC");
        fflush(temp_file);

        d_rewind(temp_file);
        d_rewind(temp_file);
        d_rewind(temp_file);

        // verify position
        d_memset(buffer, 0, sizeof(buffer));
        fread(buffer, 1, 1, temp_file);
        result = d_assert_standalone(
            buffer[0] == 'A',
            "rewind_multiple",
            "Multiple rewinds should still read 'A'",
            _counter) && result;

        fclose(temp_file);
    }

    // cleanup
    d_remove(temp_filename);

    return result;
}


/*
d_tests_sa_dio_file_positioning_all
  Aggregation function that runs all file positioning tests.
*/
bool
d_tests_sa_dio_file_positioning_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Large File Stream Positioning Functions\n");
    printf("  --------------------------------------------------\n");

    result = d_tests_sa_dio_fgetpos(_counter) && result;
    result = d_tests_sa_dio_fsetpos(_counter) && result;
    result = d_tests_sa_dio_rewind(_counter) && result;

    return result;
}
