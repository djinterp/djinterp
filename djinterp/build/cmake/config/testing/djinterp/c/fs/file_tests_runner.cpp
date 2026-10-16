/******************************************************************************
* djinterp [test]                                     file_tests_runner.cpp
*
*   Entry point for the fs subframework's C unit tests, driven through the
* C-C++ DTest bridge. It creates one suite, installs the console reporter,
* splices every section builder into it, evaluates the forest once, and reports
* the tally as its process exit code (0 all-passed, 1 a failure, 2 an internal
* error) so CTest can gate on it.
*
*   ORDER MATTERS. The bridge's imperative checks evaluate at the moment a
* section builder runs -- that is when d_fopen, d_stat and the rest are
* actually called -- not later during dtest_run, which only walks the finished
* tree and reports it. So the scratch environment is stood up BEFORE any
* builder runs and torn down after. The link and pipe builders exist only where
* those capabilities are compiled in, so their calls are guarded to match.
*
* path:      \build\cmake\config\testing\djinterp\c\fs\file_tests_runner.cpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.21
******************************************************************************/

#include <cstdio>

#include "file_tests.h"


int
main
(
    void
)
{
    dtest_suite*   suite;
    dtest_summary  summary;
    int            rc;

    std::printf(
        "==================================================================\n"
        " djinterp fs subframework (c/fs) -- C unit tests via DTest bridge\n"
        " backend: %s\n"
        "==================================================================\n",
        d_file_backend_name());

    // the environment must exist before any section runs, because the bridge
    // evaluates each check as its builder executes -- not during dtest_run
    if (!d_fs_test_setup())
    {
        std::fprintf(stderr,
                     "fatal: could not create the test environment\n");

        return 2;
    }

    suite = dtest_create();

    if (!suite)
    {
        std::fprintf(stderr, "fatal: could not create the test suite\n");
        d_fs_test_teardown();

        return 2;
    }

    // a per-leaf line plus a closing summary
    dtest_use_console(suite, DTEST_SHOW_ALL);

    // build (and thereby evaluate) every section, in narrative order
    d_fs_tests_common(suite);
    d_fs_tests_open(suite);
    d_fs_tests_desc(suite);
    d_fs_tests_io(suite);
    d_fs_tests_seek(suite);
    d_fs_tests_sync(suite);
    d_fs_tests_lock(suite);
    d_fs_tests_stat(suite);
    d_fs_tests_path(suite);
    d_fs_tests_dir(suite);
    d_fs_tests_ops(suite);
    d_fs_tests_temp(suite);

#if D_FILE_LINK_IS_AVAILABLE
    // defined only where symbolic links are compiled in
    d_fs_tests_link(suite);
#endif

#if D_FILE_PIPE_IS_AVAILABLE
    // defined only where command pipes are compiled in
    d_fs_tests_pipe(suite);
#endif

    d_fs_tests_space(suite);

    // walk the finished tree once and report; rc is the process-level verdict
    rc = dtest_run(suite, &summary);

    d_fs_test_teardown();
    dtest_destroy(suite);

    return rc;
}
