/*******************************************************************************
* djinterp [test]                                       text_edit_tests_runner.c
*
* The runner for c/text/text_edit's suite: one module spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/text/text_edit/text_edit_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "text_edit_tests.h"  // d_tests_text_edit_spec


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
        { d_tests_text_edit_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "text_edit unit tests",
                              modules,
                              1);
}
