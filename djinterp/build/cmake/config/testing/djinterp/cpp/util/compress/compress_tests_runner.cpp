/******************************************************************************
* djinterp [test]                                   compress_tests_runner.cpp
*
*   Entry point for the compress.hpp / compress.cpp unit-test suite.  It
* includes the test header and calls every tests_* function the section
* translation units define (all flat in djinterp::testing); linking this runner
* together with those sections and the production compress.cpp yields the test
* executable.  Exits non-zero if any test fails.
*
* path:      /build/cmake/config/testing/djinterp/cpp/util/compress/compress_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.21
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "compress_tests.hpp"


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


// unique letters per suite (CP == comPress) so co-compiled suites never
// collide on the macro name
#define D_CP_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("compress.hpp / compress.cpp unit tests\n");

    // -- status ---------------------------------------------------------------
    D_CP_RUN(tests_compress_status_messages);
    D_CP_RUN(tests_compress_status_unknown);
    D_CP_RUN(tests_compress_status_enum_values);

    // -- traits ---------------------------------------------------------------
    D_CP_RUN(tests_compress_traits_ids);
    D_CP_RUN(tests_compress_traits_names);
    D_CP_RUN(tests_compress_traits_store_always_available);
    D_CP_RUN(tests_compress_traits_runtime_matches_dispatch);
    D_CP_RUN(tests_compress_traits_out_of_range_unavailable);

    // -- store ----------------------------------------------------------------
    D_CP_RUN(tests_compress_store_roundtrip);
    D_CP_RUN(tests_compress_store_verbatim);
    D_CP_RUN(tests_compress_store_empty);
    D_CP_RUN(tests_compress_store_embedded_nuls);
    D_CP_RUN(tests_compress_store_all_byte_values);
    D_CP_RUN(tests_compress_store_large);
    D_CP_RUN(tests_compress_store_invalid_argument);
    D_CP_RUN(tests_compress_store_null_zero_ok);

    // -- roundtrip ------------------------------------------------------------
    D_CP_RUN(tests_compress_roundtrip_deflate);
    D_CP_RUN(tests_compress_roundtrip_zlib);
    D_CP_RUN(tests_compress_roundtrip_gzip);
    D_CP_RUN(tests_compress_roundtrip_bzip2);
    D_CP_RUN(tests_compress_roundtrip_xz);
    D_CP_RUN(tests_compress_roundtrip_zstd);
    D_CP_RUN(tests_compress_roundtrip_lz4);
    D_CP_RUN(tests_compress_roundtrip_brotli);
    D_CP_RUN(tests_compress_roundtrip_signatures);

    // -- api / dispatch -------------------------------------------------------
    D_CP_RUN(tests_compress_api_template_dispatch);
    D_CP_RUN(tests_compress_api_invalid_argument_compress);
    D_CP_RUN(tests_compress_api_invalid_argument_decompress);
    D_CP_RUN(tests_compress_api_out_of_range_unavailable);
    D_CP_RUN(tests_compress_api_decompress_garbage);
    D_CP_RUN(tests_compress_api_decompress_garbage_brotli);
    D_CP_RUN(tests_compress_api_deflate_bad_level);
    D_CP_RUN(tests_compress_api_zlib_truncated);

    // -- options --------------------------------------------------------------
    D_CP_RUN(tests_compress_options_defaults);
    D_CP_RUN(tests_compress_options_deflate_levels);
    D_CP_RUN(tests_compress_options_bzip2_block_sizes);
    D_CP_RUN(tests_compress_options_xz_levels);
    D_CP_RUN(tests_compress_options_zstd_levels);
    D_CP_RUN(tests_compress_options_brotli_levels);
    D_CP_RUN(tests_compress_options_store_ignores_level);

    // -- throwing -------------------------------------------------------------
    D_CP_RUN(tests_compress_throwing_success_matches_try);
    D_CP_RUN(tests_compress_throwing_roundtrip);
    D_CP_RUN(tests_compress_throwing_throws_on_failure);
    D_CP_RUN(tests_compress_throwing_what_message);
    D_CP_RUN(tests_compress_throwing_error_type);
    D_CP_RUN(tests_compress_throwing_decompress_throws);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_CP_RUN
