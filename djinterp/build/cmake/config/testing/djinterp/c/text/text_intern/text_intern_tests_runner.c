/*******************************************************************************
* djinterp [c][text]                                  text_intern_tests_runner.c
*
* The runner for c/text/text_intern's suite: one module spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/text/text_intern/text_intern_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "text_intern_tests.h"  // d_tests_text_intern_spec


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
        { d_tests_text_intern_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "text_intern unit tests",
                              modules,
                              1);
}
