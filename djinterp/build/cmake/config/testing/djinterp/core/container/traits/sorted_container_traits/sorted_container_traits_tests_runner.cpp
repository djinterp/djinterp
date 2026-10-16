/******************************************************************************
* djinterp [testing]               sorted_container_traits_tests_runner.cpp
*
*   Entry point for the sorted_container_traits.hpp unit suite.  Calls every
* test function in the order the header declares its sections, prints a
* per-test verdict, and reports a non-zero exit status if any test failed - so
* CTest reads the result without any further reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/core/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
*   The section numbering follows the header, which labels its sections I, II,
* III and V - there is no section IV.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/
*                                    sorted_container_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "sorted_container_traits_tests.hpp"


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

// D_SR_RUN
//   macro: names the test in the report and calls it.  The D_SR_ letters are
// unique to this suite so a co-compiled suite's runner cannot collide.
#define D_SR_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("sorted_container_traits.hpp unit tests\n");

    section("I.   sortedness signals");
    D_SR_RUN(tests_has_key_compare_helper);
    D_SR_RUN(tests_has_interval_bounds_helper);
    D_SR_RUN(tests_interval_bounds_shapes);
    D_SR_RUN(tests_sorted_invariant_helper);
    D_SR_RUN(tests_sorted_invariant_shapes);
    D_SR_RUN(tests_signals_cv_ref);

    section("II.  sortedness classification");
    D_SR_RUN(tests_sortedness_enum);
    D_SR_RUN(tests_sortedness_name);
    D_SR_RUN(tests_sortedness_of_cascade);
    D_SR_RUN(tests_sortedness_of_precedence);
    D_SR_RUN(tests_sortedness_of_std_types);
    D_SR_RUN(tests_sortedness_out_of_range);
    D_SR_RUN(tests_sortedness_cv_ref);
    D_SR_RUN(tests_sortedness_companions);

    section("III. classification predicates");
    D_SR_RUN(tests_is_sorted_container);
    D_SR_RUN(tests_is_unsorted_container);
    D_SR_RUN(tests_is_unsorted_not_complement);
    D_SR_RUN(tests_is_monotone_container);
    D_SR_RUN(tests_admits_sorted_enumeration);
    D_SR_RUN(tests_predicates_monotone_paradox);
    D_SR_RUN(tests_predicates_partition);
    D_SR_RUN(tests_predicates_cv_ref);
    D_SR_RUN(tests_predicates_companions);

    section("V.   aggregate snapshot");
    D_SR_RUN(tests_sorted_container_class_signals);
    D_SR_RUN(tests_sorted_container_class_kind);
    D_SR_RUN(tests_sorted_container_class_predicates);
    D_SR_RUN(tests_sorted_container_class_agreement);
    D_SR_RUN(tests_sorted_container_class_cv_ref);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_SR_RUN
