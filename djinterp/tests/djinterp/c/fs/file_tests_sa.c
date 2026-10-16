/*******************************************************************************
* djinterp [fs]                                                  file_tests_sa.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"


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
struct d_test_object*
d_tests_file_run_all
(
    void
)
{
    struct d_test_object* items[16];
    struct d_test_object* root;
    size_t                idx;

    if (!d_tests_file_setup())
    {
        return NULL;
    }

    idx = 0;
    items[idx++] = d_tests_file_common_all();
    items[idx++] = d_tests_file_open_all();
    items[idx++] = d_tests_file_desc_all();
    items[idx++] = d_tests_file_io_all();
    items[idx++] = d_tests_file_seek_all();
    items[idx++] = d_tests_file_sync_all();
    items[idx++] = d_tests_file_lock_all();
    items[idx++] = d_tests_file_stat_all();
    items[idx++] = d_tests_file_path_all();
    items[idx++] = d_tests_file_dir_all();
    items[idx++] = d_tests_file_ops_all();
    items[idx++] = d_tests_file_temp_all();

    // the gated modules. Absent means absent: no group, no assertion, no
    // claim. D_FILE_*_IS_AVAILABLE is a compile-time fact the config already
    // resolved, so there is nothing to probe.
#if D_FILE_LINK_IS_AVAILABLE
    items[idx++] = d_tests_file_link_all();
#endif
#if D_FILE_PIPE_IS_AVAILABLE
    items[idx++] = d_tests_file_pipe_all();
#endif
#if D_FILE_SPACE_IS_AVAILABLE
    items[idx++] = d_tests_file_space_all();
#endif

    root = d_tests_file_group("fs Subframework", items, idx);

    d_tests_file_teardown();

    return root;
}
