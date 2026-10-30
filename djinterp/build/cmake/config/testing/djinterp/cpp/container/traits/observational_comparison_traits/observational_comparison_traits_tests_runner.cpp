/******************************************************************************
* djinterp [testing]         observational_comparison_traits_tests_runner.cpp
*
*   Entry point for the observational_comparison_traits.hpp unit suite.  Calls
* every test function in the order the header declares its sections, prints a
* per-test verdict, and reports a non-zero exit status if any test failed - so
* CTest reads the result without any further reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/cpp/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/observational_comparison_traits/observational_comparison_traits_tests_runner.cpp
*                              observational_comparison_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "observational_comparison_traits_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   function: invokes one test function and records its verdict.
    void run(
        const char* _name,
        bool (*_test)()
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
    void section(
        const char* _title
    )
    {
        std::printf("\n-- %s\n", _title);

        return;
    }
}

// D_OC_RUN
//   macro: names the test in the report and calls it.  The D_OC_ letters are
// unique to this suite so a co-compiled suite's runner cannot collide.
#define D_OC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("observational_comparison_traits.hpp unit tests\n");

    section("I.   interface-operation probes");
    D_OC_RUN(tests_has_size_member);
    D_OC_RUN(tests_has_subscript_index);
    D_OC_RUN(tests_has_at_index);
    D_OC_RUN(tests_has_indexed_access);
    D_OC_RUN(tests_has_count_member);
    D_OC_RUN(tests_has_contains_member);
    D_OC_RUN(tests_probe_method_shapes);
    D_OC_RUN(tests_probe_const_observation);
    D_OC_RUN(tests_probe_void_element_safety);
    D_OC_RUN(tests_probe_cv_ref);
    D_OC_RUN(tests_probe_companions);

    section("II.  the observational level");
    D_OC_RUN(tests_observational_level_enum);
    D_OC_RUN(tests_observational_level_name);
    D_OC_RUN(tests_observational_level_rank);
    D_OC_RUN(tests_observational_level_coarser);
    D_OC_RUN(tests_observational_level_finer);
    D_OC_RUN(tests_observational_level_subsumes);
    D_OC_RUN(tests_level_algebra_lattice_laws);
    D_OC_RUN(tests_level_algebra_out_of_range);

    section("III. the level one type resolves");
    D_OC_RUN(tests_observational_level_of_cascade);
    D_OC_RUN(tests_observational_level_of_precedence);
    D_OC_RUN(tests_observational_level_of_std_types);
    D_OC_RUN(tests_observational_level_of_set_gap);
    D_OC_RUN(tests_observational_level_of_cv_ref);
    D_OC_RUN(tests_observational_level_of_companions);

    section("IV.  the shared-interface principle");
    D_OC_RUN(tests_shared_observational_level);
    D_OC_RUN(tests_shared_level_is_meet);
    D_OC_RUN(tests_shared_level_principle);
    D_OC_RUN(tests_shared_level_cv_ref);
    D_OC_RUN(tests_shared_level_companions);

    section("V.   stage of a comparison");
    D_OC_RUN(tests_has_static_size_helper);
    D_OC_RUN(tests_observed_access_stage_of);
    D_OC_RUN(tests_observational_stage_of);
    D_OC_RUN(tests_is_compile_time_comparison);
    D_OC_RUN(tests_stage_range);
    D_OC_RUN(tests_stage_level_orthogonality);
    D_OC_RUN(tests_stage_cv_ref);
    D_OC_RUN(tests_stage_companions);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_OC_RUN
