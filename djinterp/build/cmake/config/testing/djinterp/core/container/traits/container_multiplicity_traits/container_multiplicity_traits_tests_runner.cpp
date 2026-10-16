/******************************************************************************
* djinterp [testing]        container_multiplicity_traits_tests_runner.cpp
*
*   Entry point for the container_multiplicity_traits.hpp suite.  Lives in the
* config tree (next to its CMakeLists.txt), includes the test header, and calls
* every tests_* function the section translation units define.  Linking this
* runner with those sections produces the container_multiplicity_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/container_multiplicity_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.16
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_multiplicity_traits_tests.hpp"


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

// unique letters per suite: CMP = Container MultiPlicity traits
#define D_CMP_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("container_multiplicity_traits.hpp unit tests\n");

    // -- I.   Structural signals --
    D_CMP_RUN(tests_is_countable_container);
    D_CMP_RUN(tests_has_interval_bounds_signal);
    D_CMP_RUN(tests_has_unique_insert);
    D_CMP_RUN(tests_has_multiplicity_bound);

    // -- II.  Verdict (kind + numeric bound) --
    D_CMP_RUN(tests_multiplicity_kind_of);
    D_CMP_RUN(tests_multiplicity_kind_companions);
    D_CMP_RUN(tests_multiplicity_bound_of);

    // -- III. Classification predicates --
    D_CMP_RUN(tests_is_sequence_container);
    D_CMP_RUN(tests_is_unique_container);
    D_CMP_RUN(tests_is_multiset_container);

    // -- IV.  Aggregate snapshot + variable-template companions --
    D_CMP_RUN(tests_multiplicity_container_class);
    D_CMP_RUN(tests_multiplicity_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_CMP_RUN
