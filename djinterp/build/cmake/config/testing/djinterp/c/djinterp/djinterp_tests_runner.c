/*******************************************************************************
* djinterp [test]                                        djinterp_tests_runner.c
*
* The runner for djinterp.h's suite: options, the spec, and the sections
* the spec's module goes on to hold.
*   It defines DTEST_SPEC_MODE before the suite header, which then gives it
* the spec and the sections' entry points: it adds the spec, then each entry
* point builds its blocks into the same module (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/djinterp/djinterp_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.06
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face

#include <stdio.h>  // fprintf, printf, stderr

#include "djinterp_tests.h"  // d_tests_djinterp_spec, the entry points


/*
main
  The runner: the spec, then the sections, in one module.
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
    printf("== djinterp.h unit tests ==\n");

    (void)d_test_suite_add_module(suite, d_tests_djinterp_spec());
    (void)d_tests_djinterp_run_all(suite);
    (void)d_tests_djinterp_core_all_extended(suite);

    rc = d_test_suite_run(suite, NULL);
    d_test_suite_free(suite);

    return rc;
}
