/*******************************************************************************
* djinterp [test]                                     test_common_tests_runner.c
*
* The runner for test_common.h's suite: options and nothing else.
*   It defines DTEST_SPEC_MODE before the suite header, which then gives it the
* spec provider, and hands the spec to d_test_run_modules, which reads the
* command line, runs every test and returns the exit code.
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/test/test_common/test_common_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.06
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face

#include "test_common_tests.h"  // d_tests_test_common_spec


/*
main
  The runner: one module spec, the suite's own.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    const struct dj_test_module_spec* modules[1];

    modules[0] = d_tests_test_common_spec();

    return d_test_run_modules(_argc,
                              _argv,
                              "test_common.h unit tests",
                              modules,
                              1);
}
