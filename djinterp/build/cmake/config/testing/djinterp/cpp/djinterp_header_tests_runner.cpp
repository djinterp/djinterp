/*******************************************************************************
* djinterp [cpp]                                djinterp_header_tests_runner.cpp
*
* The runner for djinterp.hpp's suite: one module spec, the suite's own, run
* by DTest's C face.
*   The suite's tests are flat `bool tests_*()` functions, which a C spec row
* takes as they are, so the suite runs without the C++ framework (C++17 and
* up) and reports in the same `|...|` table as every other suite.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/djinterp_header_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.11.11
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "djinterp_tests.hpp"  // djinterp::testing::djinterp_tests_spec


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
        { ::djinterp::testing::djinterp_tests_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "djinterp.hpp unit tests",
                              modules,
                              1);
}
