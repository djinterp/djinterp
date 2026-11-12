/*******************************************************************************
* djinterp [test]                            test_counter_machine_tests_runner.c
*
* The runner for the suite of DTest's C counters' opt-in machine: one module
* spec, the suite's own.
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/test/test_counter_machine/test_counter_machine_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "./test_counter_machine_tests.h"  // d_tests_test_counter_machine_spec


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
        { d_tests_test_counter_machine_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "test_counter machine unit tests",
                              modules,
                              1);
}
