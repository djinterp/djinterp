/*******************************************************************************
* djinterp [test]                                              string_fn_tests.c
*
*
* path:      /tests/djinterp/c/string_fn_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./string_fn_tests.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_string_fn_setup
  Sets up test environment for string tests.
*/
bool d_tests_string_fn_setup(void)
{
    // no specific setup needed for string tests
    // this function is here for consistency and potential future use

    return true;
}

/*
d_tests_string_fn_teardown
  Cleans up test environment.
*/
bool d_tests_string_fn_teardown(void)
{
    // no specific teardown needed for string tests
    // this function is here for consistency and potential future use

    return true;
}

/*
d_tests_string_fn_fill_buffer
  Fills a buffer with a specific character pattern.
*/
void d_tests_string_fn_fill_buffer(
    char*  _buffer,
    size_t _size,
    char   _pattern
)
{
    size_t i;

    if ( (!_buffer) ||
         (_size == 0) )
    {
        return;
    }

    for (i = 0; i < _size - 1; i++)
    {
        _buffer[i] = _pattern;
    }

    _buffer[_size - 1] = '\0';

    return;
}

/*
d_tests_string_fn_compare_buffers
  Compares two buffers byte-by-byte.
*/
bool d_tests_string_fn_compare_buffers(
    const char* _buf1,
    const char* _buf2,
    size_t      _size
)
{
    size_t i;

    if ( (!_buf1) ||
         (!_buf2) )
    {
        return (_buf1 == _buf2);
    }

    if (_size == 0)
    {
        return true;
    }

    for (i = 0; i < _size; i++)
    {
        if (_buf1[i] != _buf2[i])
        {
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_string_fn_run_all
  Master test runner for all dstring tests.
  Tests the following:
  - Safe string copy operations
  - String duplication
  - Case-insensitive comparison
  - String tokenization
  - String length operations
  - String search
  - Case conversion
  - String manipulation
  - Error handling
  - NULL parameter handling
  - Boundary conditions
*/
bool d_tests_string_fn_run_all(struct dj_test_suite* _suite)
{
    bool                  setup_success;

    // setup test environment
    setup_success = d_tests_string_fn_setup();

    if (!setup_success)
    {
        // a failed setup is a failed test, not a line on stdout
        (void)d_test_suite_test(_suite, "setup");
        (void)D_TEST_CHECK_MSG(_suite,
                               "setup",
                               false,
                               "the string_fn test environment could not "
                               "be set up");

        return false;
    }

    // run all test categories
    d_tests_string_fn_safe_copy_all(_suite);
    d_tests_string_fn_duplication_all(_suite);
    d_tests_string_fn_case_comparison_all(_suite);
    d_tests_string_fn_tokenization_all(_suite);
    d_tests_string_fn_length_all(_suite);
    d_tests_string_fn_search_all(_suite);
    d_tests_string_fn_case_conversion_all(_suite);
    d_tests_string_fn_manipulation_all(_suite);
    d_tests_string_fn_error_handling_all(_suite);
    d_tests_string_fn_null_params_all(_suite);
    d_tests_string_fn_boundary_conditions_all(_suite);

    // cleanup
    d_tests_string_fn_teardown();

    return true;
}
