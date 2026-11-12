/*******************************************************************************
* djinterp [c][filesys]                                       dfile_tests_null.c
*
*   Tests for NULL parameter handling across all functions.
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_null.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * NULL PARAMETER TESTS
 *****************************************************************************/

/*
d_tests_dfile_null_params_all
  Tests that all functions handle NULL parameters gracefully.
  Tests the following:
  - various functions don't crash with NULL input
  - proper error codes or NULL returns for NULL params
*/
bool d_tests_dfile_null_params_all(struct dj_test_suite* _suite)
{
    bool                  test_fopen;
    bool                  test_stat;
    bool                  test_mkdir;
    bool                  test_getcwd;
    bool                  test_path_join;

    // test various NULL parameter handling
    test_fopen = (d_file_open_stream(NULL, "r") == NULL);
    test_stat = (d_file_stat(NULL, NULL) != 0);
    test_mkdir = (d_dir_create(NULL, 0) != 0);
    test_getcwd = (d_dir_get_current(NULL, 0) == NULL);
    test_path_join = (d_path_join(NULL, 0, "a", "b") == NULL);

    // build result tree
    d_test_suite_test(_suite, "NULL Parameter Handling");

    D_TEST_CHECK_MSG(_suite,
                     "fopen",
                     test_fopen,
                     "d_file_open_stream handles NULL gracefully");
    D_TEST_CHECK_MSG(_suite,
                     "stat",
                     test_stat,
                     "d_file_stat handles NULL gracefully");
    D_TEST_CHECK_MSG(_suite,
                     "mkdir",
                     test_mkdir,
                     "d_dir_create handles NULL gracefully");
    D_TEST_CHECK_MSG(_suite,
                     "getcwd",
                     test_getcwd,
                     "d_dir_get_current handles NULL gracefully");
    D_TEST_CHECK_MSG(_suite,
                     "path_join",
                     test_path_join,
                     "d_path_join handles NULL gracefully");

    return true;
}
