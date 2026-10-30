/******************************************************************************
* djinterp [testing]        container_traits_tests_runner.cpp
*
*   Entry point for the container_traits.hpp suite.  Lives in the config tree
* (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner
* with those sections produces the container_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/container_traits/container_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.16
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_traits_tests.hpp"


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

// unique letters per suite: CT = Container Traits (the legacy umbrella)
#define D_CT_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("container_traits.hpp unit tests\n");

    // -- I.   Method detection --
    D_CT_RUN(tests_size_capacity_detection);
    D_CT_RUN(tests_type_member_detection);
    D_CT_RUN(tests_hierarchical_detection);
    D_CT_RUN(tests_mutation_detection);
    D_CT_RUN(tests_iteration_detection);
    D_CT_RUN(tests_invariant_detection);

    // -- III. Public constexpr traits --
    D_CT_RUN(tests_constexpr_size_traits);
    D_CT_RUN(tests_size_shape_traits);
    D_CT_RUN(tests_interval_validity);

    // -- IV.  Array detection --
    D_CT_RUN(tests_array_detection);

    // -- V.   Lifetime classification --
    D_CT_RUN(tests_lifetime_classification);

    // -- VI.  Bounds classification --
    D_CT_RUN(tests_is_lower_bounded);
    D_CT_RUN(tests_is_upper_bounded);
    D_CT_RUN(tests_is_unbounded_and_interval_bounded);

    // -- VII. Storage classification --
    D_CT_RUN(tests_is_static_storage);
    D_CT_RUN(tests_is_dynamic_storage);

    // -- XII. Uniqueness and multiplicity --
    D_CT_RUN(tests_enforces_uniqueness);
    D_CT_RUN(tests_allows_duplicates);
    D_CT_RUN(tests_has_bounded_multiplicity);

    // -- XIII-XV. Underlying + extraction --
    D_CT_RUN(tests_is_underlying_container);
    D_CT_RUN(tests_is_fundamental_container);
    D_CT_RUN(tests_underlying_container_type_of);
    D_CT_RUN(tests_interval_extraction);

    // -- XVI. Combined classification + variable-template companions --
    D_CT_RUN(tests_container_class);
    D_CT_RUN(tests_container_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_CT_RUN
