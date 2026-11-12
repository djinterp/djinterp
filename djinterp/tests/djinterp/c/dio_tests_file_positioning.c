/*******************************************************************************
* djinterp [c]                                      dio_tests_file_positioning.c
*
*
* path:      /tests/djinterp/c/dio_tests_file_positioning.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dio_tests.h"


/*
d_tests_dio_rewind
  Tests the d_file_rewind_stream file rewind function.
  Tests the following:
  - successful rewind to start
  - rewind after write
  - rewind clears error indicators
  - rewind followed by read verification

  Note: NULL stream tests are skipped as they cause assertions in debug builds.
*/
bool d_tests_dio_rewind(
    struct dj_test_suite* _suite
)
{
    bool  result;
    FILE* temp_file;
    char  buffer[16];
    char  temp_filename[256];

    d_test_suite_test(_suite, "dio_rewind");

    result = true;

    d_strcpy_s(temp_filename, sizeof(temp_filename), "test_rewind_temp.txt");

    // test 1: NULL stream test skipped (causes assertion in debug builds)

    // test 2: successful rewind after write
    temp_file = d_file_open_stream(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "REWINDTEST");
        fflush(temp_file);

        d_file_rewind_stream(temp_file);

        // verify position by reading
        d_memset(buffer, 0, sizeof(buffer));
        result = D_TEST_CHECK_MSG(_suite,
                                  "setup_read",
                                  fread(buffer, 1, 6, temp_file) == 6,
                                  "the setup read should get 6 bytes") && result;
        result = D_TEST_CHECK_MSG(_suite,
                                  "rewind_after_write",
                                  strncmp(buffer, "REWIND", 6) == 0,
                                  "After rewind, should read 'REWIND'") && result;

        fclose(temp_file);
    }

    // test 3: rewind from middle position
    temp_file = d_file_open_stream(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "0123456789");
        fflush(temp_file);
        fseek(temp_file, 5, SEEK_SET);

        d_file_rewind_stream(temp_file);

        // verify position
        d_memset(buffer, 0, sizeof(buffer));
        result = D_TEST_CHECK_MSG(_suite,
                                  "setup_read",
                                  fread(buffer, 1, 1, temp_file) == 1,
                                  "the setup read should get 1 byte") && result;
        result = D_TEST_CHECK_MSG(_suite,
                                  "rewind_from_middle",
                                  buffer[0] == '0',
                                  "After rewind from middle, should read '0'") && result;

        fclose(temp_file);
    }

    // test 4: rewind clears error indicators
    temp_file = d_file_open_stream(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "TEST");
        fflush(temp_file);

        // try to read past EOF to set error indicator
        fseek(temp_file, 0, SEEK_END);
        fgetc(temp_file);  // should set EOF indicator

        d_file_rewind_stream(temp_file);

        result = D_TEST_CHECK_MSG(_suite,
                                  "rewind_clears_eof",
                                  d_feof(temp_file) == 0,
                                  "Rewind should clear EOF indicator") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "rewind_clears_error",
                                  d_ferror(temp_file) == 0,
                                  "Rewind should clear error indicator") && result;

        fclose(temp_file);
    }

    // test 5: multiple rewinds
    temp_file = d_file_open_stream(temp_filename, "w+");
    if (temp_file)
    {
        fprintf(temp_file, "ABC");
        fflush(temp_file);

        d_file_rewind_stream(temp_file);
        d_file_rewind_stream(temp_file);
        d_file_rewind_stream(temp_file);

        // verify position
        d_memset(buffer, 0, sizeof(buffer));
        result = D_TEST_CHECK_MSG(_suite,
                                  "setup_read",
                                  fread(buffer, 1, 1, temp_file) == 1,
                                  "the setup read should get 1 byte") && result;
        result = D_TEST_CHECK_MSG(_suite,
                                  "rewind_multiple",
                                  buffer[0] == 'A',
                                  "Multiple rewinds should still read 'A'") && result;

        fclose(temp_file);
    }

    // cleanup
    d_file_remove(temp_filename);

    return result;
}


/*
d_tests_dio_file_positioning_all
  Aggregation function that runs all file positioning tests.
*/
bool d_tests_dio_file_positioning_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "dio_file_positioning_all");

    result = true;

    result = d_tests_dio_rewind(_suite) && result;

    return result;
}
