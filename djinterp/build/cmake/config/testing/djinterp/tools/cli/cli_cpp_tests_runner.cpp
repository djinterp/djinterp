/*******************************************************************************
* djinterp [cpp][tools]                                 cli_cpp_tests_runner.cpp
*
* Entry point for the command-line kit's C++ suite.
*   Built as C++ and linked against the kit and the argument parser, built as
* C at the C floor, across a real language boundary: the suite calls the C++
* face, and the face calls the C libraries. DTest's C face, built as C, runs
* it from C++. The suite is one block of DTest's spec form, defined in its
* own source -- which a language level or exceptions turned off leave
* sections out of -- run as the module `cli_cpp`, so the count of tests
* differs by level, by design.
*   NO_COLOR is set before the first test, as the C runner sets it. The
* first argument, when it is no option, is the directory the probe modules
* were built in; every other is DTest's.
*
*
* path:      /build/cmake/config/testing/djinterp/tools/cli/cli_cpp_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.18
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
// djinterp
#include "../../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_tests_support.h"  // d_tests_cli_setenv
#include "../../../../../../../tests/djinterp/tools/cli/cli_tests.hpp"  // the C++ face


/*
main
  Runs the C++ suite's sections, as one module of one block.
*/
int main(
    int    _argc,
    char** _argv
)
{
    const dj_test_block_spec blocks[] =
    {
        d_tests_cli_cpp_block
    };
    const dj_test_module_spec module =
    {
        "cli_cpp",
        "the command-line kit's C++ face, called from C++",
        blocks,
        sizeof(blocks) / sizeof(blocks[0])
    };
    const dj_test_module_spec* const modules[] = { &module };
    const char* const                probes    =
        d_tests_cli_take_modules(&_argc,
                                 _argv);

    // nothing coloured unless a case asks; the probe modules' directory
    (void)d_tests_cli_setenv("NO_COLOR",
                             "1");
    d_tests_cli_cpp_modules(probes);

    return d_test_run_modules(_argc,
                              _argv,
                              "djinterp command-line kit: C++ tests",
                              modules,
                              1u);
}
