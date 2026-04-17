#include ".\dio_tests_sa.h"


/*
d_tests_sa_dio_run_all
  Module-level aggregation function that runs all dio tests.
  Executes tests for all categories:
  - Formatted input functions
  - Formatted output functions
  - Character and string I/O functions
  - Large file stream positioning functions
  - Error handling functions
*/
bool
d_tests_sa_dio_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_dio_formatted_input_all(_counter) && result;
    result = d_tests_sa_dio_formatted_output_all(_counter) && result;
    result = d_tests_sa_dio_char_string_io_all(_counter) && result;
    result = d_tests_sa_dio_file_positioning_all(_counter) && result;
    result = d_tests_sa_dio_error_handling_all(_counter) && result;

    return result;
}
