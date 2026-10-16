/******************************************************************************
* djinterp [testing]        container_overlay_traits_tests_runner.cpp
*
*   Entry point for the container_overlay_traits.hpp suite.  Lives in the config
* tree (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner
* with those sections produces the container_overlay_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/container_overlay_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.16
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_overlay_traits_tests.hpp"


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

// unique letters per suite: OVL = OVerLay traits
#define D_OVL_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("container_overlay_traits.hpp unit tests\n");

    // -- I.   Restriction vocabulary --
    D_OVL_RUN(tests_restriction_scope_name);
    D_OVL_RUN(tests_restriction_kind_name);
    D_OVL_RUN(tests_scope_of_restriction);

    // -- II.  Overlay-specific signals (eta) --
    D_OVL_RUN(tests_is_keyed_container);

    // -- III. Overlay descriptor + algebra --
    D_OVL_RUN(tests_overlay_top);
    D_OVL_RUN(tests_is_order_blind);
    D_OVL_RUN(tests_overlay_subsumes);
    D_OVL_RUN(tests_overlay_meet);

    // -- IV.  The overlay a container wears --
    D_OVL_RUN(tests_overlay_of);
    D_OVL_RUN(tests_is_order_blind_overlay);
    D_OVL_RUN(tests_wears_overlay);

    // -- V.   Canonical overlays --
    D_OVL_RUN(tests_canonical_overlay_name);
    D_OVL_RUN(tests_classify_canonical);
    D_OVL_RUN(tests_canonical_overlay_of);

    // -- VI.  Aggregate snapshot + variable-template companions --
    D_OVL_RUN(tests_overlay_container_class);
    D_OVL_RUN(tests_overlay_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_OVL_RUN
