/******************************************************************************
* djinterp [test]                         dtuple_wrap_partition_tests_runner.cpp
*
*   Standalone runner (main) for the dtuple_wrap_partition.hpp suite.  It
* includes the test header and drives every tests_pwp_* predicate the section
* translation units define (flat in djinterp::testing).  Linking this runner
* with those sections yields the `dtuple_wrap_partition_tests` executable.
*
*   The engine is entirely type-level, so the real verification is the
* static_assert wall in each section .cpp: if the suite links, every partition
* resolved to its expected std::tuple.  Each predicate additionally re-checks
* its section's std::is_same conditions at run time and returns the fold, so
* the report shows one PASS/FAIL line per group and the exit code is 0 iff all
* groups pass.
*
*   Placement / build follow AGENTS-cmake.md: this runner lives in the config
* tree next to its CMakeLists.txt, the sections live under tests/, and the
* leaf's INCLUDES carry both inc/djinterp/cpp/meta and tests/djinterp/cpp/meta.
*
* path:      /build/cmake/config/testing/djinterp/cpp/meta/dtuple_wrap_partition/dtuple_wrap_partition_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "dtuple_wrap_partition_tests.hpp"


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


// unique letters per suite (D_WP_ = wrap-partition) so co-compiled suites never
// collide on the macro name.
#define D_WP_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("dtuple_wrap_partition.hpp unit tests\n");

    // ---- I.  pwe_cat ----
    D_WP_RUN(tests_pwp_cat_basic);
    D_WP_RUN(tests_pwp_cat_empty);

    // ---- II. chunking ----
    D_WP_RUN(tests_pwp_chunking_n1);
    D_WP_RUN(tests_pwp_chunking_n2);
    D_WP_RUN(tests_pwp_chunking_n3);
    D_WP_RUN(tests_pwp_chunking_n4);
    D_WP_RUN(tests_pwp_chunking_multi);
    D_WP_RUN(tests_pwp_chunking_order);

    // ---- III. passthrough survival ----
    D_WP_RUN(tests_pwp_passthrough_start);
    D_WP_RUN(tests_pwp_passthrough_end);
    D_WP_RUN(tests_pwp_passthrough_between);
    D_WP_RUN(tests_pwp_passthrough_consecutive);
    D_WP_RUN(tests_pwp_passthrough_only);
    D_WP_RUN(tests_pwp_passthrough_n1);

    // ---- IV. schema input forms ----
    D_WP_RUN(tests_pwp_schema_pack);
    D_WP_RUN(tests_pwp_schema_single_tuple);
    D_WP_RUN(tests_pwp_schema_tuple_precedence);
    D_WP_RUN(tests_pwp_schema_empty_tuple);
    D_WP_RUN(tests_pwp_schema_empty_pack);

    // ---- V. alias + integration ----
    D_WP_RUN(tests_pwp_integration_alias);
    D_WP_RUN(tests_pwp_integration_wrapper_arbitrary);
    D_WP_RUN(tests_pwp_integration_complex);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_WP_RUN
