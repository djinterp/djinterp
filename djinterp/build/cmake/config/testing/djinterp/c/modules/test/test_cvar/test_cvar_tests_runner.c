/*******************************************************************************
* djinterp [test]                                       test_cvar_tests_runner.c
*
* The runner for test_cvar.h's suite: options, then the module, which its
* entry points build call by call (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/test/test_cvar/test_cvar_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.06
*******************************************************************************/
#include <stdio.h>  // fprintf, printf, stderr

#include "test_cvar_tests.h"  // the entry points, d_test_suite_*


/*
main
  The runner: one module, its entry points in order.
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

    suite = d_test_suite_new();

    if (!suite)
    {
        fprintf(stderr, "fatal: could not create the test suite\n");

        return D_TEST_SUITE_EXIT_ERROR;
    }

    d_test_suite_set_options(suite, &options);
    printf("== test_cvar.h unit tests ==\n");

    (void)d_test_suite_module(suite, "test_cvar");
    (void)d_test_suite_describe(
        suite,
        "c/modules/test/test_cvar.h: the DTest variable table");
    (void)d_tests_cvar_run_all(suite);

    rc = d_test_suite_run(suite, NULL);
    d_test_suite_free(suite);

    return rc;
}
