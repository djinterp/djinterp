/*******************************************************************************
* djinterp [tools]                                      cli_cpp_tests_runner.cpp
*
* Entry point for the command-line kit's C++ suite.
*   Built as C++ and linked against the kit and the argument parser, built as
* C at the C floor, across a real language boundary: the suite calls the C++
* face, and the face calls the C libraries. The suite is a table of
* sections its own source defines -- which a language level or exceptions
* turned off leave out -- so the count printed here differs by level, by
* design. Each result is flushed as it is printed, so that a section that
* crashes is the one after the last result shown.
*   NO_COLOR is set before the first section, as the C runner sets it. The
* one argument, when given, is the directory the probe modules were built
* in.
*
*
* path:      /build/cmake/config/testing/djinterp/tools/cli/cli_cpp_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.24
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf, std::fflush
// djinterp
#include "../../../../../../../tests/djinterp/tools/cli/c/cli_tests_support.h"  // d_tests_cli_setenv
#include "../../../../../../../tests/djinterp/tools/cli/cli_tests.hpp"  // the C++ face


/*
main
  Runs every section of the C++ suite, reporting each as it finishes.
*/
int main(
    int    _argc,
    char** _argv
)
{
    int passed = 0;
    int failed = 0;

    std::printf("djinterp command-line kit: C++ tests\n");

    // nothing coloured unless a case asks; the probe modules' directory
    (void)d_tests_cli_setenv("NO_COLOR",
                             "1");
    d_tests_cli_cpp_modules((_argc > 1) ? _argv[1] : NULL);

    // each section
    for (std::size_t i = 0u; i < d_tests_cli_cpp_count; ++i)
    {
        const dj_tests_section& section = d_tests_cli_cpp[i];
        const bool             ok      = section.run();

        std::printf("  [%s] %s\n",
                    ok ? "PASS" : "FAIL",
                    section.name);
        (void)std::fflush(stdout);

        // tally for the summary line
        if (ok)
        {
            ++passed;
        }
        else
        {
            ++failed;
        }
    }

    std::printf("passed: %d   failed: %d\n",
                passed,
                failed);

    return (failed == 0) ? 0 : 1;
}
