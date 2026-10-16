/******************************************************************************
* djinterp [testing]           read_write_container_traits_tests_runner.cpp
*
*   Entry point for the read_write_container_traits.hpp unit suite.  Calls
* every test function in the order the header declares its sections, prints a
* per-test verdict, and reports a non-zero exit status if any test failed - so
* CTest reads the result without any further reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/core/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
*   Section I of the header carries three like-groups behind one heading - the
* opt-in tag, the read surface and the write surface - and each has its own
* section TU, so the banners below read I-a, I-b and I-c.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/
*                                read_write_container_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "read_write_container_traits_tests.hpp"


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
        std::printf("\n%s\n", _title);

        return;
    }

}  // namespace

// D_RW_RUN
//   macro: names a test once, as both the label and the callee, so the two
// cannot drift apart.
#define D_RW_RUN(_fn)  run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("read_write_container_traits.hpp - unit suite\n");

    section("I-a. Read surface");
    D_RW_RUN(tests_has_const_index_helper);
    D_RW_RUN(tests_has_const_front_helper);
    D_RW_RUN(tests_has_const_data_helper);
    D_RW_RUN(tests_has_const_begin_helper);
    D_RW_RUN(tests_has_read_surface);
    D_RW_RUN(tests_read_surface_excludes_metadata);
    D_RW_RUN(tests_read_surface_std_types);

    section("I-b. Write surface");
    D_RW_RUN(tests_has_push_back_helper);
    D_RW_RUN(tests_has_emplace_back_helper);
    D_RW_RUN(tests_has_push_front_helper);
    D_RW_RUN(tests_has_clear_helper);
    D_RW_RUN(tests_has_assignable_index_helper);
    D_RW_RUN(tests_has_write_surface);
    D_RW_RUN(tests_write_surface_value_type_asymmetry);
    D_RW_RUN(tests_write_surface_std_types);

    section("I-c. Capability tag and bits resolution");
    D_RW_RUN(tests_has_capability_bits_helper);
    D_RW_RUN(tests_capability_tag_shapes);
    D_RW_RUN(tests_access_bits_structural);
    D_RW_RUN(tests_access_bits_tagged);
    D_RW_RUN(tests_access_bits_precedence);
    D_RW_RUN(tests_capability_bits_cv_ref);

    section("II.  Capability classification");
    D_RW_RUN(tests_access_capability_enum);
    D_RW_RUN(tests_access_capability_name);
    D_RW_RUN(tests_access_capability_name_total);
    D_RW_RUN(tests_access_capability_from_bits);
    D_RW_RUN(tests_access_capability_of);
    D_RW_RUN(tests_access_capability_of_members);
    D_RW_RUN(tests_access_capability_cv_ref);
    D_RW_RUN(tests_access_capability_companions);

    section("III. Predicate and tag re-emission");
    D_RW_RUN(tests_is_read_write_container);
    D_RW_RUN(tests_is_read_write_std_types);
    D_RW_RUN(tests_access_tag_of);
    D_RW_RUN(tests_access_tag_of_none_is_void);
    D_RW_RUN(tests_access_tag_round_trip);
    D_RW_RUN(tests_predicate_cv_ref);
    D_RW_RUN(tests_predicate_companions);

    section("IV.  Aggregate snapshot");
    D_RW_RUN(tests_access_container_class_bits);
    D_RW_RUN(tests_access_container_class_kind);
    D_RW_RUN(tests_access_container_class_predicates);
    D_RW_RUN(tests_access_container_class_agreement);
    D_RW_RUN(tests_access_container_class_cv_ref);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_RW_RUN
