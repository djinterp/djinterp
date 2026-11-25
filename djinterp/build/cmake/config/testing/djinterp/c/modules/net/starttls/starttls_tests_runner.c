/*******************************************************************************
* djinterp [c][net]                                      starttls_tests_runner.c
*
* The runner for net's STARTTLS: options, then its one section,
* d_tests_starttls, which opens its module and builds it call by call
* (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/net/starttls/starttls_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.19
*                                                            revised: 2026.11.19
*******************************************************************************/
// std
#include <stdio.h>  // fprintf, stderr
// djinterp
#include "starttls_tests.h"  // d_tests_starttls, d_test_suite_*


/*
main
  The runner: options, the suite, its section, the report.
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
    (void)d_tests_starttls(suite);
    rc = d_test_suite_run(suite, NULL);
    d_test_suite_free(suite);

    return rc;
}
