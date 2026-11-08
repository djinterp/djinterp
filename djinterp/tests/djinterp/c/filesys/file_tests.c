/*******************************************************************************
* djinterp [test]                                                   file_tests.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.21
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./file_tests.h"


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_file_run_all
  Runs every fs module suite and returns the result tree.
  The tree's shape follows the BUILD, not the platform. A module the config
compiled out contributes nothing -- not an empty group, not a green "skipped"
assertion. So the count is derived from what actually ran, and the suite
cannot claim to have tested something it did not.

Parameter(s):
  none.
Return:
  The root of the result tree, or NULL on failure.
*/
bool d_tests_file_run_all(struct dj_test_suite* _suite)
{
    if (!d_tests_file_setup())
    {
        return false;
    }

    d_tests_file_common_all(_suite);
    d_tests_file_open_all(_suite);
    d_tests_file_desc_all(_suite);
    d_tests_file_io_all(_suite);
    d_tests_file_seek_all(_suite);
    d_tests_file_sync_all(_suite);
    d_tests_file_lock_all(_suite);
    d_tests_file_stat_all(_suite);
    d_tests_file_path_all(_suite);
    d_tests_file_dir_all(_suite);
    d_tests_file_ops_all(_suite);
    d_tests_file_temp_all(_suite);

    // the gated modules. Absent means absent: no group, no assertion, no
    // claim. D_FILE_*_IS_AVAILABLE is a compile-time fact the config already
    // resolved, so there is nothing to probe.
#if D_FILE_LINK_IS_AVAILABLE
    d_tests_file_link_all(_suite);
#endif
#if D_FILE_PIPE_IS_AVAILABLE
    d_tests_file_pipe_all(_suite);
#endif
#if D_FILE_SPACE_IS_AVAILABLE
    d_tests_file_space_all(_suite);
#endif


    d_tests_file_teardown();

    return true;
}
