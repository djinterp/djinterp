/*******************************************************************************
* djinterp [c/parse]                                     overflow_tests_runner.c
*
* The runner for the overflow suite of parse and parsegen: its options and
* its spec.
*   It defines DTEST_SPEC_MODE before the suite header, which then gives it
* the tests and the spec provider (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/parse/overflow/overflow_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./overflow_tests.h"  // d_tests_parse_overflow_spec


/*
main
  The runner: the spec's one module, run as the command line asks.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    const struct dj_test_module_spec* modules[1];

    modules[0] = d_tests_parse_overflow_spec();

    return d_test_run_modules(_argc, _argv,
                              "parse and parsegen: overflow tests",
                              modules, 1u);
}
