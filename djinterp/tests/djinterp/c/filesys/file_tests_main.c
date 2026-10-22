/*******************************************************************************
* djinterp [test]                                              file_tests_main.c
*
*   Driver for the dtest fs suite: builds one suite, hands it to each of the
* sixteen section entry points in order, runs it.
*   The sections take a dtest_suite* and chain onto it (dtest_module ->
* dtest_block -> dtest_test -> D_FS_CHECK), so the driver's only jobs are
* creating the suite, installing a reporter, and returning the run's verdict as
* the process exit code.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_main.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/

// djinterp
#include "file_tests.h"

int
main
(
    int    _argc,
    char** _argv
)
{
    dtest_suite* suite;
    int          rc;

    (void)_argc;
    (void)_argv;

    suite = dtest_create();

    if (!suite)
    {
        return 2;
    }

    dtest_use_console(suite, DTEST_SHOW_FAILURES_ONLY);

    //   The scratch tree comes FIRST. Most sections open a file that is
    // supposed to already exist, so without this every one of them fails on
    // its first check and the run reports the library as broken when what is
    // actually missing is the fixture.
    if (!d_fs_test_setup())
    {
        dtest_destroy(suite);

        return 2;
    }

    d_fs_tests_common(suite);
    d_fs_tests_open(suite);
    d_fs_tests_desc(suite);
    d_fs_tests_io(suite);
    d_fs_tests_seek(suite);
    d_fs_tests_sync(suite);
    d_fs_tests_lock(suite);
    d_fs_tests_stat(suite);
    d_fs_tests_dir(suite);
    d_fs_tests_ops(suite);
    d_fs_tests_path(suite);
    //   Spliced in only where the platform has them, exactly as file_tests.h
    // specifies: the sections are always DECLARED, their definitions compile
    // out with D_FILE_LINK_IS_AVAILABLE / D_FILE_PIPE_IS_AVAILABLE, and the
    // runner is the piece that decides whether to call them. Calling them
    // unconditionally is a link error, not a skipped test.
#if D_FILE_LINK_IS_AVAILABLE
    d_fs_tests_link(suite);
#endif
    d_fs_tests_temp(suite);
#if D_FILE_PIPE_IS_AVAILABLE
    d_fs_tests_pipe(suite);
#endif
    d_fs_tests_space(suite);

    rc = dtest_run(suite, NULL);

    //   Teardown after the run, not after registration: the sections build the
    // tree, dtest_run executes it, and anything removed in between is removed
    // before the tests that need it have run.
    (void)d_fs_test_teardown();

    dtest_destroy(suite);

    return rc;
}
