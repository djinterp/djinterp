/*******************************************************************************
* djinterp [test]                                         counter_tests_runner.c
*
* The runner for the counter family's suite: one module spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/util/counter/counter_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./counter_tests.h"  // d_tests_counter_spec


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
        { d_tests_counter_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "counter unit tests",
                              modules,
                              1);
}
