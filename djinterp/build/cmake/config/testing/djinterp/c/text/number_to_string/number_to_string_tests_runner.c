/*******************************************************************************
* djinterp [c][text]                             number_to_string_tests_runner.c
*
* The runner for c/text's numbers in words: one module spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/text/number_to_string/number_to_string_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "number_to_string_tests.h"  // d_tests_number_to_string_spec


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
        { d_tests_number_to_string_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "number_to_string unit tests",
                              modules,
                              1);
}
