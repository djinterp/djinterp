/*******************************************************************************
* djinterp [cpp][test]                               test_suite_tests_runner.cpp
*
*   Entry point for the suite of test_suite.h as C++ suites use it. Defining
* DTEST_SPEC_MODE selects the header's provider face; main hands its spec,
* whose rows D_TEST_CASE wrote, to C++'s run_module.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/test_suite/test_suite_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.15
*                                                            revised: 2026.11.15
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite header's provider face

// djinterp
#include "test_suite_tests.hpp"  // djinterp::testing::suite_spec


/*
main
  The runner: one module spec, the suite's own, to the console only.
*/
int
main()
{
    return ::djinterp::test::run_module(::djinterp::testing::suite_spec(),
                                        "test_suite.h from C++",
                                        "DTest's C face, as C++ suites use it");
}
