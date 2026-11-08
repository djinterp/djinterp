/*******************************************************************************
* djinterp [test]                                                  dfile_tests.c
*
*   Master test runner for the dfile module (cross-platform file I/O).
*   Aggregates all test categories and manages test environment lifecycle.
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "dfile_tests.h"


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_dfile_run_all
  Runs all dfile tests and returns the root test object tree.
  Tests the following:
  - sets up test environment
  - runs all test categories
  - tears down test environment
  - returns comprehensive test results

Return:
  pointer to root test object containing all test results, or NULL on failure.
*/
bool d_tests_dfile_run_all(struct dj_test_suite* _suite)
{
    // setup test environment
    if (!d_tests_dfile_setup())
    {
        return false;
    }

    // run all test categories
    d_tests_dfile_secure_file_opening_all(_suite);
    d_tests_dfile_large_file_support_all(_suite);
    d_tests_dfile_descriptor_operations_all(_suite);
    d_tests_dfile_synchronization_all(_suite);
    d_tests_dfile_locking_all(_suite);
    d_tests_dfile_temporary_files_all(_suite);
    d_tests_dfile_metadata_all(_suite);
    d_tests_dfile_directory_operations_all(_suite);
    d_tests_dfile_file_operations_all(_suite);
    d_tests_dfile_path_utilities_all(_suite);

#if D_FILE_LINK_IS_AVAILABLE
    d_tests_dfile_symbolic_links_all(_suite);
#endif

    d_tests_dfile_pipe_operations_all(_suite);
    d_tests_dfile_binary_io_all(_suite);
    d_tests_dfile_null_params_all(_suite);

    // teardown test environment
    d_tests_dfile_teardown();

    return true;
}
