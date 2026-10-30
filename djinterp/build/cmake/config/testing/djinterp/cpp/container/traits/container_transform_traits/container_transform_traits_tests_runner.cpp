/******************************************************************************
* djinterp [testing]        container_transform_traits_tests_runner.cpp
*
*   Entry point for the container_transform_traits.hpp suite.  Lives in the config
* tree (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner with
* those sections produces the container_transform_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/container_transform_traits/container_transform_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.17
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_transform_traits_tests.hpp"


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

// unique letters per suite: TX = TransXform traits
#define D_TX_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("container_transform_traits.hpp unit tests\n");

    // -- I.   Capability detection --
    D_TX_RUN(tests_is_transform_source);
    D_TX_RUN(tests_is_container_transformable);
    D_TX_RUN(tests_is_transform_input_only);
    D_TX_RUN(tests_has_native_transform);

    // -- II.  Strategy --
    D_TX_RUN(tests_transform_strategy_name);
    D_TX_RUN(tests_container_transform_strategy);

    // -- III. Stage --
    D_TX_RUN(tests_transform_stage_name);
    D_TX_RUN(tests_transform_stage_of);

    // -- IV.  Preservation --
    D_TX_RUN(tests_transform_preserves_order);
    D_TX_RUN(tests_transform_preserves_size_and_capacity);
    D_TX_RUN(tests_transform_preserves_sortedness_and_uniqueness);

    // -- V.   Result type and element rebind --
    D_TX_RUN(tests_rebind_element);
    D_TX_RUN(tests_transform_result_type);

    // -- VI.  Combined classification + variable-template companions --
    D_TX_RUN(tests_container_transform_class);
    D_TX_RUN(tests_transform_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_TX_RUN
