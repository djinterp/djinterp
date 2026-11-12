/*******************************************************************************
* djinterp [test]                                    test_counter_tests_runner.c
*
* The runner for DTest's C test counter suite: one module spec, the suite's
* own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/test/test_counter/test_counter_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./test_counter_tests.h"  // d_tests_test_counter_spec


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
        { d_tests_test_counter_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "test_counter unit tests",
                              modules,
                              1);
}
