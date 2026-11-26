/*******************************************************************************
* djinterp [c][text]                                    path_span_tests_runner.c
*
* The runner for c/text/path_span.h: one module spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/text/path_span/path_span_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "path_span_tests.h"  // d_tests_path_span_spec


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
    const struct dj_test_module_spec* modules[1] =
        { d_tests_path_span_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "path_span unit tests",
                              modules,
                              1);
}
