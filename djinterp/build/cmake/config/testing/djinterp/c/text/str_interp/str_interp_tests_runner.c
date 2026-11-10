/*******************************************************************************
* djinterp [test]                                      str_interp_tests_runner.c
*
* The runner for c/text/str_interp unit tests: options, then the module, which
* the suite's entry point fills with its blocks (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/text/str_interp/str_interp_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <stdio.h>  // fprintf, printf, stderr
// djinterp
#include "str_interp_tests.h"  // d_tests_str_interp_run_all


/*
main
  The runner: the command line's options, the module, the report.
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
    printf("== c/text/str_interp unit tests ==\n");

    (void)d_test_suite_module(suite, "str_interp");
    (void)d_tests_str_interp_run_all(suite);

    rc = d_test_suite_run(suite, NULL);
    d_test_suite_free(suite);

    return rc;
}
