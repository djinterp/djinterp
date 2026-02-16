/******************************************************************************
* djinterp [test]                                              dfile_tests_sa.c
*
*   Master test runner for the dfile module (cross-platform file I/O).
*   Aggregates all test categories and manages test environment lifecycle.
*
*
* path:      \src\test\dfile_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/

#include "dfile_tests_sa.h"


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
struct d_test_object*
d_tests_dfile_run_all
(
    void
)
{
    struct d_test_object* root;
    size_t                idx;
    size_t                total_tests;

    // setup test environment
    if (!d_tests_dfile_setup())
    {
        return NULL;
    }

    // determine total test count based on available features
#if D_FILE_HAS_SYMLINKS
    total_tests = 14;
#else
    total_tests = 13;
#endif

    // create root test group
    root = d_test_object_new_interior("dfile Module Tests", total_tests);

    if (!root)
    {
        d_tests_dfile_teardown();

        return NULL;
    }

    // run all test categories
    idx = 0;
    root->elements[idx++] = d_tests_dfile_secure_file_opening_all();
    root->elements[idx++] = d_tests_dfile_large_file_support_all();
    root->elements[idx++] = d_tests_dfile_descriptor_operations_all();
    root->elements[idx++] = d_tests_dfile_synchronization_all();
    root->elements[idx++] = d_tests_dfile_locking_all();
    root->elements[idx++] = d_tests_dfile_temporary_files_all();
    root->elements[idx++] = d_tests_dfile_metadata_all();
    root->elements[idx++] = d_tests_dfile_directory_operations_all();
    root->elements[idx++] = d_tests_dfile_file_operations_all();
    root->elements[idx++] = d_tests_dfile_path_utilities_all();

#if D_FILE_HAS_SYMLINKS
    root->elements[idx++] = d_tests_dfile_symbolic_links_all();
#endif

    root->elements[idx++] = d_tests_dfile_pipe_operations_all();
    root->elements[idx++] = d_tests_dfile_binary_io_all();
    root->elements[idx++] = d_tests_dfile_null_params_all();

    // teardown test environment
    d_tests_dfile_teardown();

    return root;
}
