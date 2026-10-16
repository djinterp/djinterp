/******************************************************************************
* djinterp [testing]        container_filter_traits_tests_runner.cpp
*
*   Entry point for the container_filter_traits.hpp suite.  Lives in the config
* tree (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner
* with those sections produces the container_filter_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/container_filter_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.15
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_filter_traits_tests.hpp"


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

// unique letters per suite: CF = Container Filter traits
#define D_CF_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("container_filter_traits.hpp unit tests\n");

    // -- I.   Capability detection --
    D_CF_RUN(tests_is_filter_source);
    D_CF_RUN(tests_is_container_filterable);
    D_CF_RUN(tests_is_filter_input_only);
    D_CF_RUN(tests_has_native_filter);

    // -- II.  Strategy --
    D_CF_RUN(tests_filter_strategy_name);
    D_CF_RUN(tests_container_filter_strategy);

    // -- III. Stage --
    D_CF_RUN(tests_filter_stage_name);
    D_CF_RUN(tests_filter_stage_of);

    // -- IV.  Preservation --
    D_CF_RUN(tests_filter_preserves_order);
    D_CF_RUN(tests_filter_preserves_sortedness);
    D_CF_RUN(tests_filter_preserves_uniqueness);
    D_CF_RUN(tests_filter_preserves_capacity_bounds);

    // -- V.   Result type --
    D_CF_RUN(tests_filter_result_type);

    // -- VI.  Combined classification + variable-template companions --
    D_CF_RUN(tests_container_filter_class);
    D_CF_RUN(tests_filter_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_CF_RUN
