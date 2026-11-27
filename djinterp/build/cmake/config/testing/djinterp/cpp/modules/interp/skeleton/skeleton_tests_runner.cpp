/*******************************************************************************
* djinterp [cpp][interp]                               skeleton_tests_runner.cpp
*
* The runner of the interpreter's skeleton's suite: its module handed to
* DTest's C face, and nothing else.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/interp/skeleton/skeleton_tests_runner.cpp
* link(s):   /docs/djinterp/cpp/modules/interp/DESIGN.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/
// djinterp
#include "../../../../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
#include "../../../../../../../../../tests/djinterp/cpp/modules/interp/skeleton_tests.hpp"  // the suite


/*
main
  The suite's one module, run as the command line asks.
*/
int main(
    int    _argc,
    char** _argv
)
{
    static const struct dj_test_module_spec* const MODULES[] =
    {
        &d_tests_interp_skeleton_module
    };

    return d_test_run_modules(_argc, _argv, "djinterp interpreter skeleton",
                              MODULES, 1u);
}
