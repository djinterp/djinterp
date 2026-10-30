/******************************************************************************
* djinterp [test]                                      archive_tests_runner.cpp
*
*   Entry point (main) for the archive.hpp / archive.cpp unit-test suite.  It
* includes the suite's test header and calls every tests_* function the section
* translation units define, flat, in djinterp::testing.  Linking this runner
* with the section .cpp files (and the module's production sources) produces the
* archive_tests executable that CTest runs.
*
*   The runner owns the only main() for the suite; the section TUs contribute
* test functions only.  Each call reports PASS/FAIL and the process exits non-
* zero if any test failed.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/util/archive/archive_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.21
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "archive_tests.hpp"


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

        return;
    }
}

// unique letters per suite (D_AR_) so co-compiled suites never collide on the
// macro name
#define D_AR_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int main()
{
    std::printf("archive.hpp unit tests\n");

    // entry model
    D_AR_RUN(tests_entry_defaults);
    D_AR_RUN(tests_entry_list_basics);
    D_AR_RUN(tests_entry_helpers);

    // format tags / traits / capability queries
    D_AR_RUN(tests_format_traits_identity);
    D_AR_RUN(tests_format_traits_capability_flags);
    D_AR_RUN(tests_format_capability_queries);

    // tar (ustar) writer / reader
    D_AR_RUN(tests_tar_roundtrip_files);
    D_AR_RUN(tests_tar_directories);
    D_AR_RUN(tests_tar_metadata);
    D_AR_RUN(tests_tar_name_truncation);
    D_AR_RUN(tests_tar_checksum);
    D_AR_RUN(tests_tar_extract_truncated);
    D_AR_RUN(tests_tar_empty_archive);

    // zip writer / reader
    D_AR_RUN(tests_zip_roundtrip_files);
    D_AR_RUN(tests_zip_store_mode);
    D_AR_RUN(tests_zip_deflate_mode);
    D_AR_RUN(tests_zip_directories);
    D_AR_RUN(tests_zip_empty_archive);
    D_AR_RUN(tests_zip_unsupported_method);
    D_AR_RUN(tests_zip_extract_truncated);
    D_AR_RUN(tests_zip_extract_corrupt_deflate);

    // gz and tar.gz
    D_AR_RUN(tests_gz_requires_single_entry);
    D_AR_RUN(tests_gz_roundtrip);
    D_AR_RUN(tests_gz_extract_corrupt);
    D_AR_RUN(tests_targz_roundtrip_files);
    D_AR_RUN(tests_targz_empty_archive);
    D_AR_RUN(tests_targz_extract_corrupt);

    // 7z / rar backends
    D_AR_RUN(tests_sevenzip_capability_consistency);
    D_AR_RUN(tests_rar_capability_consistency);

    // dispatch leaves + argument guards
    D_AR_RUN(tests_dispatch_null_base_guard);
    D_AR_RUN(tests_dispatch_unknown_format);
    D_AR_RUN(tests_dispatch_capability_switches);

    // non-throwing edges + throwing convenience API
    D_AR_RUN(tests_try_api_empty_list);
    D_AR_RUN(tests_throwing_roundtrip);
    D_AR_RUN(tests_throwing_error_on_create);
    D_AR_RUN(tests_throwing_error_on_extract);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_AR_RUN
