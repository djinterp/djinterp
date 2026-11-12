/*******************************************************************************
* djinterp [c]                                                       dio_tests.c
*
*
* path:      /tests/djinterp/c/dio_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dio_tests.h"


/*
d_tests_dio_run_all
  Module-level aggregation function that runs all dio tests.
  Executes tests for all categories:
  - Formatted input functions
  - Formatted output functions
  - Character and string I/O functions
  - Large file stream positioning functions
  - Error handling functions
*/
bool d_tests_dio_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_dio_formatted_input_all(_suite) && result;
    result = d_tests_dio_formatted_output_all(_suite) && result;
    result = d_tests_dio_char_string_io_all(_suite) && result;
    result = d_tests_dio_file_positioning_all(_suite) && result;
    result = d_tests_dio_error_handling_all(_suite) && result;

    return result;
}
