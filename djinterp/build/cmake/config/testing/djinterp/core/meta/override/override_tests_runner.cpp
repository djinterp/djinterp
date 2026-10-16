// std
#include <cstdio>
// djinterp
#include "override_tests.hpp"

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

    void section(const char* _title)
    {
        std::printf("\n--- %s ---\n", _title);
    }
}

// D_OV_RUN
//   macro: runs one suite function by name.  The OV letters are unique to
// this suite so co-compiled suites never collide on the macro name.
#define D_OV_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int main()
{
    std::printf("override.hpp unit tests\n");

    section("I.   dropped sentinel");
    D_OV_RUN(tests_override_dropped_shape);
    D_OV_RUN(tests_override_is_dropped_trait);
    D_OV_RUN(tests_override_is_dropped_cvref);
    D_OV_RUN(tests_override_is_dropped_hostile);

    section("II.  OverridePolicy concept");
    D_OV_RUN(tests_override_concept_primitives);
    D_OV_RUN(tests_override_concept_negatives);
    D_OV_RUN(tests_override_concept_laziness);
    D_OV_RUN(tests_override_concept_as_constraint);

    section("III. atomic primitives");
    D_OV_RUN(tests_override_keep_base);
    D_OV_RUN(tests_override_keep_delta);
    D_OV_RUN(tests_override_drop_extras);
    D_OV_RUN(tests_override_strict_subset);
    D_OV_RUN(tests_override_drop_unmatched_base);
    D_OV_RUN(tests_override_primitive_shapes);

    section("IV.  combinators");
    D_OV_RUN(tests_override_with_on_both);
    D_OV_RUN(tests_override_with_on_base_only);
    D_OV_RUN(tests_override_with_on_delta_only);
    D_OV_RUN(tests_override_combinator_stacking);
    D_OV_RUN(tests_override_combinator_equivalences);

    section("V.   misc helpers");
    D_OV_RUN(tests_override_identity_t);
    D_OV_RUN(tests_override_always_drop);
    D_OV_RUN(tests_override_always_keep_left);
    D_OV_RUN(tests_override_always_keep_right);

    section("VI.  integration (policy-driven walk)");
    D_OV_RUN(tests_override_engine_primitives);
    D_OV_RUN(tests_override_engine_edge_cases);
    D_OV_RUN(tests_override_engine_custom_policies);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_OV_RUN
