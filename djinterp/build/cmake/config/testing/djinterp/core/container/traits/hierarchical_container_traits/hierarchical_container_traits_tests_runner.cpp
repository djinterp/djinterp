/******************************************************************************
* djinterp [testing]        hierarchical_container_traits_tests_runner.cpp
*
*   Entry point for the hierarchical_container_traits.hpp suite.  Lives in the
* config tree (next to its CMakeLists.txt), includes the test header, and calls
* every tests_* function the section translation units define.  Linking this runner
* with those sections produces the hierarchical_container_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/structure/hierarchical_container_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.17
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "hierarchical_container_traits_tests.hpp"


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

// unique letters per suite: HC = Hierarchical Container traits
#define D_HC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("hierarchical_container_traits.hpp unit tests\n");

    // -- I.   Structural detection signals --
    D_HC_RUN(tests_is_container_shape);
    D_HC_RUN(tests_has_node_summand);
    D_HC_RUN(tests_has_structure_category);

    // -- II.  Recursive depth --
    D_HC_RUN(tests_container_depth);
    D_HC_RUN(tests_container_depth_maxdepth_cap);
    D_HC_RUN(tests_max_depth_of);

    // -- III. Tag reading + verdict --
    D_HC_RUN(tests_is_hierarchical_container);
    D_HC_RUN(tests_is_depth_bounded_container);

    // -- IV.  Structure-kind summary --
    D_HC_RUN(tests_structure_kind_name);
    D_HC_RUN(tests_structure_kind_of);

    // -- V.   Aggregate snapshot + variable-template companions --
    D_HC_RUN(tests_hierarchical_container_class);
    D_HC_RUN(tests_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_HC_RUN
