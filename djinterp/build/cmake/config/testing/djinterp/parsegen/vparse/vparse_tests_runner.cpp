/*******************************************************************************
* djinterp [parsegen]                                    vparse_tests_runner.cpp
*
* Entry point for the vparse test suite.  Lives in the config tree (next to
* the CMakeLists), includes the test header, and drives the tests_* section
* functions.  Linking this with the section TUs and the production sources
* yields the vparse_tests executable.
*   The test header is reached by a path relative to this file, so the leaf
* needs no include directory for it.
*
*
* path:      /build/cmake/config/testing/djinterp/parsegen/vparse/vparse_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.06
*******************************************************************************/
// std
#include <cstdio>  // std::printf
// djinterp
#include "../../../../../../../tests/djinterp/parsegen/vparse/vparse_tests.hpp"  // the sections
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor the suite has


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
    D_VP_RUN(tests_captures_trace);
    D_VP_RUN(tests_generate_anchor);
    D_VP_RUN(tests_generate_multirule);
    D_VP_RUN(tests_notation_anchor);
    D_VP_RUN(tests_notation_alternation);
    D_VP_RUN(tests_notation_plus_literal);
    D_VP_RUN(tests_rebase_handle);
    D_VP_RUN(tests_rebase_compose);
    D_VP_RUN(tests_lr_left_recursive);
    D_VP_RUN(tests_lr_trace_and_limits);
    D_VP_RUN(tests_ebnf_arithmetic);
    D_VP_RUN(tests_ebnf_grouping);
    D_VP_RUN(tests_ebnf_errors);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_VP_RUN

#endif  // floor, for now
