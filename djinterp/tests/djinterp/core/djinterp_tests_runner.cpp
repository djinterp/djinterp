/*******************************************************************************
* djinterp [core/testing]                              djinterp_tests_runner.cpp
*
*   Entry point that drives the djinterp.hpp unit-test suite.  It includes the
* test header and calls every flat tests_* function in djinterp::testing,
* reporting per-test PASS/FAIL and a final tally.  Exit code is 0 when every
* test passed, 1 otherwise.
*
*
* path:      /tests/djinterp/core/djinterp_tests_runner.cpp
* link(s):   TBA
* author(s): Claude                                          created: 2026.06.24
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "djinterp_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void run(const char* _name, bool (*_test)())
    {
        const bool ok = _test();

        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);

        if (ok)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
        }

        return;
    }
}

// D_DJ_RUN
//   macro: invokes a single test by name, resolving it in djinterp::testing.
// Unique suite letters ("DJ") so co-compiled suites never collide on the macro.
#define D_DJ_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int
main()
{
    std::printf("djinterp.hpp unit tests\n");

    // I & II -- keyword, namespace, and qualifier macros
    D_DJ_RUN(tests_macros_keywords);
    D_DJ_RUN(tests_macros_namespaces);
    D_DJ_RUN(tests_macros_constexpr_qualifiers);
    D_DJ_RUN(tests_macros_noexcept);

    // III.i -- void_t
    D_DJ_RUN(tests_void_t_aliases_to_void);
    D_DJ_RUN(tests_void_t_detection_idiom);

    // III.ii -- abs_value / abs_value_v / abs_value_to_size_t
    D_DJ_RUN(tests_abs_value_struct);
    D_DJ_RUN(tests_abs_value_v);
    D_DJ_RUN(tests_abs_value_to_size_t);

    // III.iii -- clean / clean_t
    D_DJ_RUN(tests_clean_basic);
    D_DJ_RUN(tests_clean_edges);
    D_DJ_RUN(tests_clean_t_alias);

    // III.iv -- constexpr_swap
    D_DJ_RUN(tests_constexpr_swap_runtime);
    D_DJ_RUN(tests_constexpr_swap_constexpr);
    D_DJ_RUN(tests_constexpr_swap_noexcept);

    // III.v -- repeat / repeat_t
    D_DJ_RUN(tests_repeat_zero);
    D_DJ_RUN(tests_repeat_counts);
    D_DJ_RUN(tests_repeat_t_alias_and_types);

    // III.vi.a-d -- self / is_self / is_self_v
    D_DJ_RUN(tests_self_is_self_positive);
    D_DJ_RUN(tests_self_is_self_negative);

    // III.vi.e-f -- resolve_self / resolve_self_t
    D_DJ_RUN(tests_resolve_self_passthrough);
    D_DJ_RUN(tests_resolve_self_marker);
    D_DJ_RUN(tests_resolve_self_smart_pointers);
    D_DJ_RUN(tests_resolve_self_raw_pointer);
    D_DJ_RUN(tests_resolve_self_variadic_templates);
    D_DJ_RUN(tests_resolve_self_repeat);
    D_DJ_RUN(tests_resolve_self_limitations);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_DJ_RUN
