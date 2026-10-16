/******************************************************************************
* djinterp [testing]        container_storage_traits_tests_runner.cpp
*
*   Entry point for the container_storage_traits.hpp suite.  Lives in the config
* tree (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner
* with those sections produces the container_storage_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/container_storage_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.16
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_storage_traits_tests.hpp"


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

// unique letters per suite: STG = SToraGe traits
#define D_STG_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("container_storage_traits.hpp unit tests\n");

    // -- I.   Storage-shape signals --
    D_STG_RUN(tests_has_static_storage_signal);
    D_STG_RUN(tests_has_dynamic_storage_signal);

    // -- II.  The container siting classifier --
    D_STG_RUN(tests_storage_duration_of);
    D_STG_RUN(tests_storage_duration_of_companions);

    // -- III. Container siting predicates --
    D_STG_RUN(tests_is_static_storage_container);
    D_STG_RUN(tests_is_dynamic_storage_container);
    D_STG_RUN(tests_is_hybrid_storage_container);
    D_STG_RUN(tests_has_static_storage_component_container);
    D_STG_RUN(tests_has_dynamic_storage_component_container);

    // -- IV.  Cross-axis entailment --
    D_STG_RUN(tests_static_storage_implies_constexpr_size);

    // -- V.   Aggregate snapshot + variable-template companions --
    D_STG_RUN(tests_container_storage_class);
    D_STG_RUN(tests_storage_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_STG_RUN
