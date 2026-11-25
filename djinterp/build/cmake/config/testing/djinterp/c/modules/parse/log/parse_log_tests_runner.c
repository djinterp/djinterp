/*******************************************************************************
* djinterp [c][parse]                                   parse_log_tests_runner.c
*
* The runner for parse's log suite: its options and its spec.
*   It defines DTEST_SPEC_MODE before the suite header, which then gives it
* the tests and the spec provider (test_suite.h 3.2).
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/parse/log/parse_log_tests_runner.c
* link(s):   /docs/djinterp/log/log_design.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./parse_log_tests.h"  // d_tests_parse_log_spec


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

    modules[0] = d_tests_parse_log_spec();

    return d_test_run_modules(_argc, _argv,
                              "parse/parse_log.h: parse's records in the log",
                              modules, 1u);
}
