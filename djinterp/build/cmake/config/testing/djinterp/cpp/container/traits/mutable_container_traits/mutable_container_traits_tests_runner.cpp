// std
#include <cstdio>
// djinterp
#include "mutable_container_traits_tests.hpp"


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

    void section(
        const char* _title
    )
    {
        std::printf("\n-- %s\n", _title);

        return;
    }
}

// D_MC_RUN
//   macro: runs one test function of the mutable_container_traits suite,
// naming it from its own identifier.  The MC letters are unique to this suite
// so a co-compiled runner cannot collide on the macro name.
#define D_MC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int main()
{
    std::printf("mutable_container_traits.hpp unit tests\n");

    section("I.   SFINAE mutator detection");
    D_MC_RUN(tests_has_push_back_signal);
    D_MC_RUN(tests_has_push_front_signal);
    D_MC_RUN(tests_has_insert_signal);
    D_MC_RUN(tests_has_erase_signal);
    D_MC_RUN(tests_has_clear_signal);
    D_MC_RUN(tests_has_resize_signal);
    D_MC_RUN(tests_has_mutable_subscript_signal);
    D_MC_RUN(tests_has_mutable_data_signal);
    D_MC_RUN(tests_signal_value_type_dependency);
    D_MC_RUN(tests_signal_cv_ref_invariance);
    D_MC_RUN(tests_signal_value_companions);

    section("II.  Capability umbrellas");
    D_MC_RUN(tests_is_element_mutable_container);
    D_MC_RUN(tests_is_structurally_mutable_container);
    D_MC_RUN(tests_is_mutable_container);
    D_MC_RUN(tests_is_immutable_container);
    D_MC_RUN(tests_umbrella_signal_composition);
    D_MC_RUN(tests_umbrella_cv_ref_invariance);
    D_MC_RUN(tests_umbrella_value_companions);

    section("III. The mutability grade");
    D_MC_RUN(tests_mutability_enum);
    D_MC_RUN(tests_mutability_name);
    D_MC_RUN(tests_mutability_grade_predicates);
    D_MC_RUN(tests_mutability_grade_admits);
    D_MC_RUN(tests_mutability_of);
    D_MC_RUN(tests_mutability_of_companions);
    D_MC_RUN(tests_mutability_grade_aliases);
    D_MC_RUN(tests_mutability_of_umbrella_agreement);
    D_MC_RUN(tests_mutability_of_cv_ref_invariance);

    section("IV.  The access dimension");
    D_MC_RUN(tests_access_internal_marks);
    D_MC_RUN(tests_access_restriction_enum);
    D_MC_RUN(tests_access_restriction_name);
    D_MC_RUN(tests_access_restriction_of);
    D_MC_RUN(tests_access_restriction_of_companions);
    D_MC_RUN(tests_constrains_element);
    D_MC_RUN(tests_constrains_key);
    D_MC_RUN(tests_access_restriction_cv_ref_invariance);
    D_MC_RUN(tests_access_grade_orthogonality);

    section("V.   Aggregate snapshot");
    D_MC_RUN(tests_mutable_container_class_signals);
    D_MC_RUN(tests_mutable_container_class_capabilities);
    D_MC_RUN(tests_mutable_container_class_grade);
    D_MC_RUN(tests_mutable_container_class_access);
    D_MC_RUN(tests_mutable_container_class_agreement);
    D_MC_RUN(tests_mutable_container_class_cv_ref_invariance);
    D_MC_RUN(tests_mutable_container_class_edge_types);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_MC_RUN
