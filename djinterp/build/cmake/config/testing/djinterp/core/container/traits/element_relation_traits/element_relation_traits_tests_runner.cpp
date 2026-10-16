/******************************************************************************
* djinterp [testing]        element_relation_traits_tests_runner.cpp
*
*   Entry point for the element_relation_traits.hpp suite.  Lives in the config
* tree (next to its CMakeLists.txt), includes the test header, and calls every
* tests_* function the section translation units define.  Linking this runner with
* those sections produces the element_relation_traits_tests exe.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/element_relation_traits_tests_runner.cpp
* author(s): unit-test suite                               created: 2026.07.17
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "element_relation_traits_tests.hpp"


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

// unique letters per suite: ER = Element Relation traits
#define D_ER_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("element_relation_traits.hpp unit tests\n");

    // -- II.  Element relation (on the value_type) --
    D_ER_RUN(tests_element_type_of);
    D_ER_RUN(tests_has_equality_comparable_elements);
    D_ER_RUN(tests_has_less_than_comparable_elements);
    D_ER_RUN(tests_has_totally_ordered_elements);
    D_ER_RUN(tests_has_three_way_comparable_elements);

    // -- III. Comparison degree --
    D_ER_RUN(tests_comparison_degree_name);
    D_ER_RUN(tests_comparison_degree_rank_and_weaker);
    D_ER_RUN(tests_element_comparison_degree_of);

    // -- IV.  Container-level comparison --
    D_ER_RUN(tests_has_container_equality_and_less);
    D_ER_RUN(tests_has_three_way_comparable_container);
    D_ER_RUN(tests_container_comparison_degree_of);
    D_ER_RUN(tests_is_equality_comparable_container);
    D_ER_RUN(tests_is_ordered_comparable_container);

    // -- V.   Cross-container element compatibility --
    D_ER_RUN(tests_elements_same_type);
    D_ER_RUN(tests_elements_convertible);
    D_ER_RUN(tests_elements_mutually_convertible);
    D_ER_RUN(tests_cross_elements_equality_comparable);
    D_ER_RUN(tests_cross_elements_ordered);
    D_ER_RUN(tests_variable_templates);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_ER_RUN
