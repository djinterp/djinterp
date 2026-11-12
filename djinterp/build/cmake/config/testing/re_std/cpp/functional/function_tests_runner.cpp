/*******************************************************************************
* djinterp [re_std]                                    function_tests_runner.cpp
*
* The runner for re_std::function's suite: one module spec, the suite's own,
* run by DTest's C face.
*
*
* path:      /build/cmake/config/testing/re_std/cpp/functional/function_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "function_tests.hpp"  // djinterp::testing::function_tests_spec


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
        { ::djinterp::testing::function_tests_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "re_std::function unit tests",
                              modules,
                              1);
}
