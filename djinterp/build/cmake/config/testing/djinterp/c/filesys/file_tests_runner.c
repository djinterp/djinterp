/*******************************************************************************
* djinterp [test]                                            file_tests_runner.c
*
* Entry point for the fs subframework's C unit tests, in DTest's C face.
*   It reads the command line into the suite's options, creates one suite,
* splices every section builder into it, and reports the result as its process
* exit code (0 all passed, 1 a failure, 2 an internal error) so CTest can gate
* on it.
*
*   ORDER MATTERS. Each check is evaluated the moment a section builder makes
* it -- that is when the file functions are actually called -- not later when
* d_test_suite_run reports. So the scratch environment is stood up before any
* builder runs and torn down after. The link and pipe builders exist only where
* those capabilities are compiled in, so their calls are guarded to match.
*
*   It replaces file_tests_runner.cpp: the suite is C, and a C runner lets a
* C-only build run it.
*
*
* path:      /build/cmake/config/testing/djinterp/c/filesys/file_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.05
*******************************************************************************/
#include <stdio.h>

#include "file_tests.h"


/*
main
  Options, environment, sections, report.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    struct dj_test_suite_options options;
    struct dj_test_suite*        suite;
    int                          rc;

    d_test_suite_options_init(&options);
    rc = d_test_suite_options_parse(&options, _argc, _argv);

    if (rc != 0)
    {
        return (rc > 0) ? D_TEST_SUITE_EXIT_PASSED : D_TEST_SUITE_EXIT_ERROR;
    }

    printf("=================================================================="
           "\n djinterp fs subframework (c/filesys) -- C unit tests\n"
           " backend: %s\n"
           "=================================================================="
           "\n",
           d_file_backend_name());

    // the environment must exist before any section runs, because each check
    // is evaluated as its builder executes
    if (!d_fs_test_setup())
    {
        fprintf(stderr, "fatal: could not create the test environment\n");

        return D_TEST_SUITE_EXIT_ERROR;
    }

    suite = d_test_suite_new();

    if (!suite)
    {
        fprintf(stderr, "fatal: could not create the test suite\n");
        d_fs_test_teardown();

        return D_TEST_SUITE_EXIT_ERROR;
    }

    d_test_suite_set_options(suite, &options);

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

    rc = d_test_suite_run(suite, NULL);

    d_fs_test_teardown();
    d_test_suite_free(suite);

    return rc;
}
