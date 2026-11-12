/*******************************************************************************
* djinterp [cpp][memory]                                 memory_tests_runner.cpp
*
* The runner for the C++ memory suite: one module spec, the suite's own,
* run by DTest's C face.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/memory/memory/memory_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face
// djinterp
#include "memory_tests.hpp"  // djinterp::testing::memory_tests_spec


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
        { ::djinterp::testing::memory_tests_spec() };

    return d_test_run_modules(_argc,
                              _argv,
                              "cpp/memory unit tests",
                              modules,
                              1);
}
