/******************************************************************************
* djinterp [testing]              container_copy_merge_traits_tests_runner.cpp
*
*   Entry point for the container_copy_merge_traits.hpp unit suite.
* Calls every test function in the order the test header declares its
* sections, prints a per-test verdict, and reports a non-zero exit status
* if any test failed - so CTest reads the result without any further
* reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/cpp/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
*   The header divides into six numbered sections and the banners below follow
* them exactly: the two operand prerequisites, copy, the local discipline
* reading, the combining kind built on it, the three provisos, and the
* aggregate that restates the lot.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/container_copy_merge_traits/container_copy_merge_traits_tests_runner.cpp
*                                container_copy_merge_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "container_copy_merge_traits_tests.hpp"


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
        std::printf("\n%s\n", _title);

        return;
    }

}  // namespace

// D_CM_RUN
//   macro: names a test once, as both the label and the callee, so the two
// cannot drift apart.
#define D_CM_RUN(_fn)  run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("container_copy_merge_traits.hpp - unit suite\n");

    section("I.   Operand prerequisites");
    D_CM_RUN(tests_merge_elements_compatible);
    D_CM_RUN(tests_merge_elements_needs_containers);
    D_CM_RUN(tests_merge_elements_convertibility_is_either_way);
    D_CM_RUN(tests_merge_elements_cv_ref);
    D_CM_RUN(tests_merge_elements_companions);

    section("II.  Copy");
    D_CM_RUN(tests_is_copyable_container);
    D_CM_RUN(tests_copy_needs_both_conjuncts);
    D_CM_RUN(tests_copy_capability_false_positive);
    D_CM_RUN(tests_copy_preserves_all_axes);
    D_CM_RUN(tests_copy_cv_ref);
    D_CM_RUN(tests_copy_companions);

    section("III. Discipline signals");
    D_CM_RUN(tests_discipline_signal_probes);
    D_CM_RUN(tests_discipline_boundedness_probes);
    D_CM_RUN(tests_merge_discipline_enum);
    D_CM_RUN(tests_merge_discipline_name);
    D_CM_RUN(tests_discipline_family_predicates);
    D_CM_RUN(tests_merge_discipline_of);
    D_CM_RUN(tests_merge_discipline_of_std_types);
    D_CM_RUN(tests_discipline_pair_sequence_misread);
    D_CM_RUN(tests_discipline_signal_order);
    D_CM_RUN(tests_merge_discipline_cv_ref);
    D_CM_RUN(tests_merge_discipline_companions);

    section("IV.  Merge kind");
    D_CM_RUN(tests_merge_kind_enum);
    D_CM_RUN(tests_merge_kind_name);
    D_CM_RUN(tests_enum_names_fall_through);
    D_CM_RUN(tests_merge_kind_from_disciplines);
    D_CM_RUN(tests_merge_kind_from_disciplines_total);
    D_CM_RUN(tests_merge_kind_of);
    D_CM_RUN(tests_merge_kind_of_element_guard);
    D_CM_RUN(tests_merge_kind_of_std_types);
    D_CM_RUN(tests_is_mergeable);
    D_CM_RUN(tests_merge_kind_cv_ref);
    D_CM_RUN(tests_merge_kind_companions);

    section("V.   Merge provisos");
    D_CM_RUN(tests_merge_result_type);
    D_CM_RUN(tests_merge_has_key_conflict);
    D_CM_RUN(tests_merge_has_key_conflict_needs_keyed_merge);
    D_CM_RUN(tests_merge_may_overflow);
    D_CM_RUN(tests_merge_may_overflow_routes);
    D_CM_RUN(tests_merge_may_overflow_is_left_biased);
    D_CM_RUN(tests_provisos_cv_ref);
    D_CM_RUN(tests_provisos_companions);

    section("VI.  Aggregate snapshot");
    D_CM_RUN(tests_merge_class_disciplines);
    D_CM_RUN(tests_merge_class_kind);
    D_CM_RUN(tests_merge_class_provisos);
    D_CM_RUN(tests_merge_class_agreement);
    D_CM_RUN(tests_merge_class_cv_ref);
    D_CM_RUN(tests_traits_metafunction_shape);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_CM_RUN
