/******************************************************************************
* djinterp [testing]              ordered_container_traits_tests_runner.cpp
*
*   Entry point for the ordered_container_traits.hpp unit suite.  Calls every
* test function in the order the header declares its sections, prints a
* per-test verdict, and reports a non-zero exit status if any test failed - so
* CTest reads the result without any further reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/core/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/
*                                   ordered_container_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "ordered_container_traits_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   function: invokes one test function and records its verdict.
    void
    run(
        const char* _name,
        bool      (*_test)()
    )
    {
        const bool ok = _test();

        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);

        if (ok)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
        }

        return;
    }

    // section
    //   function: prints a section banner.
    void
    section(
        const char* _title
    )
    {
        std::printf("\n-- %s\n", _title);

        return;
    }
}

// D_OR_RUN
//   macro: names the test in the report and calls it.  The D_OR_ letters are
// unique to this suite so a co-compiled suite's runner cannot collide.
#define D_OR_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("ordered_container_traits.hpp unit tests\n");

    section("I.   order-axis predicates");
    D_OR_RUN(tests_has_key_type_helper);
    D_OR_RUN(tests_is_ordered_container);
    D_OR_RUN(tests_is_unordered_container);
    D_OR_RUN(tests_order_axis_exclusivity);
    D_OR_RUN(tests_order_axis_keyed_gap);
    D_OR_RUN(tests_order_axis_cv_ref);
    D_OR_RUN(tests_order_axis_companions);

    section("II.  the ordering kind");
    D_OR_RUN(tests_ordering_kind_enum);
    D_OR_RUN(tests_ordering_kind_name);
    D_OR_RUN(tests_ordering_kind_of);
    D_OR_RUN(tests_ordering_kind_of_agreement);
    D_OR_RUN(tests_ordering_kind_out_of_range);
    D_OR_RUN(tests_ordering_kind_cv_ref);
    D_OR_RUN(tests_ordering_kind_companions);

    section("III. the sequential layout");
    D_OR_RUN(tests_has_c_str_helper);
    D_OR_RUN(tests_sequential_layout_enum);
    D_OR_RUN(tests_sequential_layout_name);
    D_OR_RUN(tests_sequential_layout_of_std_types);
    D_OR_RUN(tests_sequential_layout_of_precedence);
    D_OR_RUN(tests_sequential_layout_of_gate);
    D_OR_RUN(tests_sequential_layout_of_fallthrough);
    D_OR_RUN(tests_sequential_layout_out_of_range);
    D_OR_RUN(tests_sequential_layout_cv_ref);
    D_OR_RUN(tests_sequential_layout_companions);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_OR_RUN
