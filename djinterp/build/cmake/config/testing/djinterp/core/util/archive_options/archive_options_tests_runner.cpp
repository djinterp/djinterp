/******************************************************************************
* djinterp [test]                             archive_options_tests_runner.cpp
*
*   Entry point (main) for the archive_options.hpp unit-test suite.  It includes
* the suite's test header and calls every tests_* function the section
* translation units define, flat, in djinterp::testing.  Linking this runner
* with the section .cpp files (and the module's production sources, which the
* behavioural section drives) produces the archive_options_tests executable that
* CTest runs.
*
*   The runner owns the only main() for the suite; the section TUs contribute
* test functions only.  Each call reports PASS/FAIL and the process exits non-
* zero if any test failed.
*
*
* path:      /build/cmake/config/testing/djinterp/core/util/archive_options_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "archive_options_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void
    run(
        const char* _name,
        bool        (*_test)()
    )
    {
        const bool ok = _test();
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);

        if (ok) { ++g_passed; } else { ++g_failed; }

        return;
    }
}

// unique letters per suite (D_AO_) so co-compiled suites never collide on the
// macro name
#define D_AO_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int
main()
{
    std::printf("archive_options.hpp unit tests\n");

    // I. categorical enums
    D_AO_RUN(tests_enum_neutral_defaults);
    D_AO_RUN(tests_enum_distinct_values);
    D_AO_RUN(tests_enum_sequential_values);
    D_AO_RUN(tests_enum_plain_not_scoped);

    // II.i zip_options
    D_AO_RUN(tests_zip_options_defaults);
    D_AO_RUN(tests_zip_options_fields);

    // II.ii-v tar / gz / 7z / rar options
    D_AO_RUN(tests_tar_options_defaults);
    D_AO_RUN(tests_gz_options_defaults);
    D_AO_RUN(tests_sevenzip_options_defaults);
    D_AO_RUN(tests_rar_options_defaults);

    // III. the archive_options aggregate
    D_AO_RUN(tests_archive_options_defaults);
    D_AO_RUN(tests_archive_options_stub_superset);
    D_AO_RUN(tests_archive_options_nested_blocks_default);

    // the embedded codec
    D_AO_RUN(tests_codec_block_defaults);
    D_AO_RUN(tests_codec_block_independence);

    // value semantics + the shared comparators
    D_AO_RUN(tests_options_copy_construction);
    D_AO_RUN(tests_options_assignment);
    D_AO_RUN(tests_options_deep_string_independence);
    D_AO_RUN(tests_comparator_detects_archive_fields);
    D_AO_RUN(tests_comparator_detects_format_fields);
    D_AO_RUN(tests_comparator_detects_codec_fields);
    D_AO_RUN(tests_option_diff_description);
    D_AO_RUN(tests_option_builders);

    // what the facade actually honours
    D_AO_RUN(tests_facade_honours_store_only);
    D_AO_RUN(tests_facade_honours_level);
    D_AO_RUN(tests_facade_codec_block_is_inert);
    D_AO_RUN(tests_facade_container_knobs_do_not_corrupt);
    D_AO_RUN(tests_facade_does_not_mutate_options);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_AO_RUN
