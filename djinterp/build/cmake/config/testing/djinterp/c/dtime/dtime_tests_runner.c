/*******************************************************************************
* djinterp [test]                                           dtime_tests_runner.c
*
* The runner for dtime.h's suite: options and nothing else.
*
*
* path:      /build/cmake/config/testing/djinterp/c/dtime/dtime_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face

#include "dtime_tests.h"


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

    modules[0] = d_tests_dtime_spec();

    return d_test_run_modules(_argc,
                              _argv,
                              "dtime.h unit tests",
                              modules,
                              1);
}
