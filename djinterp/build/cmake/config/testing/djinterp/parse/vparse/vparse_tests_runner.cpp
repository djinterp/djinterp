/******************************************************************************
* djinterp [vparse]                                   vparse_tests_runner.cpp
*
*   Entry point for the vparse test suite.  Lives in the config tree (next to
* the CMakeLists), includes the test header, and drives the tests_* section
* functions.  Linking this with the section TUs and the production sources
* yields the vparse_tests executable.
*
* path:      /build/cmake/config/testing/djinterp/parse/vparse/vparse_tests_runner.cpp
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "../../../../../../../tests/djinterp/parse/vparse/vparse_tests.hpp"

namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void run(const char* _name, bool (*_test)())
    {
        const bool ok = _test();
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);
        if (ok) { ++g_passed; } else { ++g_failed; }
    }
}

// unique letters per suite so co-compiled runners never collide on the macro
#define D_VP_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int
main()
{
    std::printf("vparse.hpp unit tests\n");

    D_VP_RUN(tests_captures_backtrack);
    D_VP_RUN(tests_captures_generated);
    D_VP_RUN(tests_generate_anchor);
    D_VP_RUN(tests_generate_multirule);
    D_VP_RUN(tests_notation_anchor);
    D_VP_RUN(tests_notation_alternation);
    D_VP_RUN(tests_notation_plus_literal);
    D_VP_RUN(tests_rebase_handle);
    D_VP_RUN(tests_rebase_compose);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_VP_RUN
