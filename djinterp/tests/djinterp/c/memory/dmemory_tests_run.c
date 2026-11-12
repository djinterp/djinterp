/*******************************************************************************
* djinterp [c][memory]                                       dmemory_tests_run.c
*
*
* path:      /tests/djinterp/c/memory/dmemory_tests_run.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmemory_tests.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_dmemory_setup
  Sets up test environment for memory tests.
*/
bool d_tests_dmemory_setup(void)
{
    // no specific setup needed for memory tests
    // this function is here for consistency and potential future use

    return true;
}


/*
d_tests_dmemory_teardown
  Cleans up test environment.
*/
bool d_tests_dmemory_teardown(void)
{
    // no specific teardown needed for memory tests
    // this function is here for consistency and potential future use

    return true;
}


/*
d_tests_dmemory_fill_pattern
  Fills a buffer with a repeating byte pattern.
*/
void d_tests_dmemory_fill_pattern(
    void*       _buffer,
    size_t      _size,
    const void* _pattern,
    size_t      _pattern_size
)
{
    unsigned char* buf;
    unsigned char* pat;
    size_t         i;
    size_t         j;

    if ( (!_buffer) ||
         (!_pattern) ||
         (_size == 0) ||
         (_pattern_size == 0) )
    {
        return;
    }

    buf = (unsigned char*)_buffer;
    pat = (unsigned char*)_pattern;

    for (i = 0; i < _size; i++)
    {
        j = i % _pattern_size;
        buf[i] = pat[j];
    }

    return;
}


/*
d_tests_dmemory_verify_pattern
  Verifies a buffer contains expected pattern.
*/
bool d_tests_dmemory_verify_pattern(
    const void*   _buffer,
    size_t        _size,
    unsigned char _pattern
)
{
    const unsigned char* buf;
    size_t               i;

    if ( (!_buffer) ||
         (_size == 0) )
    {
        return false;
    }

    buf = (const unsigned char*)_buffer;

    for (i = 0; i < _size; i++)
    {
        if (buf[i] != _pattern)
        {
            return false;
        }
    }

    return true;
}


/*
d_tests_dmemory_compare_buffers
  Compares two memory buffers byte-by-byte.
*/
bool d_tests_dmemory_compare_buffers(
    const void* _buf1,
    const void* _buf2,
    size_t      _size
)
{
    const unsigned char* b1;
    const unsigned char* b2;
    size_t               i;

    if ( (!_buf1) ||
         (!_buf2) )
    {
        return (_buf1 == _buf2);
    }

    if (_size == 0)
    {
        return true;
    }

    b1 = (const unsigned char*)_buf1;
    b2 = (const unsigned char*)_buf2;

    for (i = 0; i < _size; i++)
    {
        if (b1[i] != b2[i])
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
d_tests_dmemory_run_all
  Master test runner for all dmemory tests.
  Tests the following:
  - Memory copy operations
  - Memory duplication
  - Memory set operations
  - NULL parameter handling
  - Boundary conditions
  - Alignment tests
  - Overlapping memory
  - Performance tests
*/
bool d_tests_dmemory_run_all(struct dj_test_suite* _suite)
{
    bool                  setup_success;

    // setup test environment
    setup_success = d_tests_dmemory_setup();

    if (!setup_success)
    {
        // a failed setup is a failed test, not a line on stdout
        (void)d_test_suite_test(_suite, "setup");
        (void)D_TEST_CHECK_MSG(_suite,
                               "setup",
                               false,
                               "the dmemory test environment could not "
                               "be set up");

        return false;
    }

    // run all test categories
    d_tests_dmemory_copy_all(_suite);
    d_tests_dmemory_duplication_all(_suite);
    d_tests_dmemory_set_all(_suite);
    d_tests_dmemory_null_params_all(_suite);
    d_tests_dmemory_boundary_conditions_all(_suite);
    d_tests_dmemory_alignment_all(_suite);
    d_tests_dmemory_overlap_all(_suite);
    d_tests_dmemory_performance_all(_suite);

    // cleanup
    d_tests_dmemory_teardown();

    return true;
}
