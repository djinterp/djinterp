/******************************************************************************
* djinterp [testing]        iterable_container_traits_tests_runner.cpp
*
*   Entry point for the iterable_container_traits.hpp suite.  Lives in the config
* tree (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner with
* those sections produces the iterable_container_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/iterable_container_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.18
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "iterable_container_traits_tests.hpp"


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

// unique letters per suite: IC = Iterable Container traits
#define D_IC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("iterable_container_traits.hpp unit tests\n");

    // -- I.   SFINAE method / alias detection --
    D_IC_RUN(tests_has_begin_end_methods);
    D_IC_RUN(tests_alias_detection);

    // -- II.  Base classification --
    D_IC_RUN(tests_is_iterable_container);
    D_IC_RUN(tests_is_non_iterable_container);

    // -- III. Mode sub-axis, value-level algebra --
    D_IC_RUN(tests_iteration_mode_name);
    D_IC_RUN(tests_mode_value_predicates);
    D_IC_RUN(tests_mode_grants);
    D_IC_RUN(tests_iteration_mode_rank);
    D_IC_RUN(tests_iteration_mode_subsumes);

    // -- III. Mode sub-axis, type-level traits --
    D_IC_RUN(tests_provides_const_iteration);
    D_IC_RUN(tests_provides_mutable_iteration);
    D_IC_RUN(tests_iteration_mode_of);
    D_IC_RUN(tests_derived_mode_predicates);

    // -- V.   Aggregate snapshot + variable-template companions --
    D_IC_RUN(tests_iterable_container_class);
    D_IC_RUN(tests_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_IC_RUN
