/******************************************************************************
* djinterp [testing]           container_conversion_traits_tests_runner.cpp
*
*   Entry point for the container_conversion_traits.hpp suite.  Lives in the
* config tree (next to its CMakeLists.txt), includes the test header, and calls
* every tests_* function the section translation units define.  Linking this
* runner with those sections produces the container_conversion_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/container_conversion_traits/container_conversion_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.15
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_conversion_traits_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void run(
        const char* _name,
        bool (*_test)()
    )
    {
        const bool ok = _test();
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);
        if (ok) { ++g_passed; } else { ++g_failed; }
    }
}

// unique letters per suite: CCV = Container ConVersion traits
#define D_CCV_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("container_conversion_traits.hpp unit tests\n");

    // -- I.   Tier vocabulary --
    D_CCV_RUN(tests_conversion_tier_vocabulary);

    // -- II.  Construction-path detection --
    D_CCV_RUN(tests_range_constructible);
    D_CCV_RUN(tests_range_insertable);
    D_CCV_RUN(tests_has_constructive_path);

    // -- III. Per-concern analysis --
    D_CCV_RUN(tests_elements_compatible);
    D_CCV_RUN(tests_conversion_forgets_content);
    D_CCV_RUN(tests_conversion_clamps_capacity);
    D_CCV_RUN(tests_conversion_changes_topology);

    // -- IV.  Classification --
    D_CCV_RUN(tests_conversion_tier_of_view);
    D_CCV_RUN(tests_conversion_tier_of_constructive);
    D_CCV_RUN(tests_conversion_tier_of_lossy);
    D_CCV_RUN(tests_conversion_tier_of_structural);
    D_CCV_RUN(tests_conversion_tier_of_none);
    D_CCV_RUN(tests_conversion_tier_of_cvref);
    D_CCV_RUN(tests_conversion_predicates);

    // -- IV.  Aggregate + variable-template companions --
    D_CCV_RUN(tests_conversion_class);
    D_CCV_RUN(tests_conversion_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_CCV_RUN
