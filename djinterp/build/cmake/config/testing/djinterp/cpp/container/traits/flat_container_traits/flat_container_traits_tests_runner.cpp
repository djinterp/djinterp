/******************************************************************************
* djinterp [testing]        flat_container_traits_tests_runner.cpp
*
*   Entry point for the flat_container_traits.hpp suite.  Lives in the config tree
* (next to its CMakeLists.txt), includes the test header, and calls every tests_*
* function the section translation units define.  Linking this runner with those
* sections produces the flat_container_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/flat_container_traits/flat_container_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.17
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "flat_container_traits_tests.hpp"


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

// unique letters per suite: FC = Flat Container traits
#define D_FC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("flat_container_traits.hpp unit tests\n");

    // -- I.  Flat classification --
    D_FC_RUN(tests_is_flat_container);
    D_FC_RUN(tests_is_strictly_flat_container);

    // -- II. Aggregate snapshot + variable-template companions --
    D_FC_RUN(tests_flat_container_class);
    D_FC_RUN(tests_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_FC_RUN
