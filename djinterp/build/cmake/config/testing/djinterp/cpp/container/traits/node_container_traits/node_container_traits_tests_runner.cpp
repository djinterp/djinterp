/******************************************************************************
* djinterp [testing]                     node_container_traits_tests_runner.cpp
*
* Node Container Traits Test Runner:
*   Entry point for the node_container_traits.hpp unit test suite.  Drives
* every test function the section translation units define, in the order of
* the sections of the header under test.
*
*   The suite is standalone: it includes node_container_traits.hpp and nothing
* else from djinterp.  node_container.hpp is deliberately NOT linked in, which
* leaves the three ownership policies incomplete and is what
* tests_is_owning_container_completeness records.  Every section TU must agree
* on that, so no section may include it either.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/node_container_traits/node_container_traits_tests_runner.cpp
*                                       node_container_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "node_container_traits_tests.hpp"


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

// D_NC_RUN
//   macro: names the test in the report and calls it.  The D_NC_ letters are
// unique to this suite so a co-compiled suite's runner cannot collide.
#define D_NC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("node_container_traits.hpp unit tests\n");

    section("I.   ownership policy detection");
    D_NC_RUN(tests_has_ownership_policy);
    D_NC_RUN(tests_has_entry_storage_type);
    D_NC_RUN(tests_has_entry_owns_constant);
    D_NC_RUN(tests_ownership_detect_cv_ref);
    D_NC_RUN(tests_ownership_detect_companions);

    section("II.  entry point detection");
    D_NC_RUN(tests_has_entry_point_method);
    D_NC_RUN(tests_has_root_method);
    D_NC_RUN(tests_has_root_member_method);
    D_NC_RUN(tests_has_head_method);
    D_NC_RUN(tests_has_tail_method);
    D_NC_RUN(tests_has_entries_method);
    D_NC_RUN(tests_has_entry_count_method);
    D_NC_RUN(tests_has_has_entry_method);
    D_NC_RUN(tests_has_release_entry_method);
    D_NC_RUN(tests_entry_point_method_shapes);
    D_NC_RUN(tests_has_any_entry_point);
    D_NC_RUN(tests_entry_point_probe_qualification);
    D_NC_RUN(tests_entry_point_companions);

    section("III. node type extraction");
    D_NC_RUN(tests_has_node_type);
    D_NC_RUN(tests_node_type_of_t);
    D_NC_RUN(tests_node_type_of_t_fallback);
    D_NC_RUN(tests_node_type_cv_ref);

    section("IV.  ownership model classification");
    D_NC_RUN(tests_ownership_model_enum);
    D_NC_RUN(tests_ownership_model_of);
    D_NC_RUN(tests_ownership_model_absent_policy);
    D_NC_RUN(tests_is_owning_container);
    D_NC_RUN(tests_is_owning_container_completeness);
    D_NC_RUN(tests_is_unique_owning_container);
    D_NC_RUN(tests_is_shared_owning_container);
    D_NC_RUN(tests_ownership_model_orthogonality);
    D_NC_RUN(tests_ownership_model_cv_ref);
    D_NC_RUN(tests_ownership_model_companions);

    section("V.   node container identity");
    D_NC_RUN(tests_is_node_container);
    D_NC_RUN(tests_is_tree_shaped_container);
    D_NC_RUN(tests_is_list_shaped_container);
    D_NC_RUN(tests_is_doubly_linked_container);
    D_NC_RUN(tests_is_graph_shaped_container);
    D_NC_RUN(tests_identity_composition_shape);
    D_NC_RUN(tests_identity_overlap);
    D_NC_RUN(tests_node_container_cv_ref_divergence);
    D_NC_RUN(tests_identity_companions);

    section("VI.  combined classification");
    D_NC_RUN(tests_node_container_shape_enum);
    D_NC_RUN(tests_node_container_shape_of);
    D_NC_RUN(tests_node_container_shape_precedence);
    D_NC_RUN(tests_node_container_shape_companions);
    D_NC_RUN(tests_node_container_class_identity);
    D_NC_RUN(tests_node_container_class_ownership);
    D_NC_RUN(tests_node_container_class_entry);
    D_NC_RUN(tests_node_container_class_node);
    D_NC_RUN(tests_node_container_class_agreement);
    D_NC_RUN(tests_node_container_class_edge_types);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_NC_RUN
