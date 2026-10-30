/******************************************************************************
* djinterp [test]                          compress_options_tests_runner.cpp
*
*   Entry point for the compress_options.hpp unit-test suite.  It includes the
* test header and calls every tests_* function the section translation units
* define (all flat in djinterp::testing); linking this runner with those
* sections yields the test executable.  The suite is header-only - the
* vocabulary under test pulls in no backend - so nothing else is linked.  Exits
* non-zero if any test fails.
*
* path:      /build/cmake/config/testing/djinterp/cpp/util/compress_options/compress_options_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.21
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "compress_options_tests.hpp"


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


// unique letters per suite (CO == Compress Options) so co-compiled suites never
// collide on the macro name
#define D_CO_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("compress_options.hpp unit tests\n");

    // -- enums ----------------------------------------------------------------
    D_CO_RUN(tests_compress_options_enum_deflate_strategy);
    D_CO_RUN(tests_compress_options_enum_lzma_check);
    D_CO_RUN(tests_compress_options_enum_lzma_mode);
    D_CO_RUN(tests_compress_options_enum_lzma_match_finder);
    D_CO_RUN(tests_compress_options_enum_zstd_strategy);
    D_CO_RUN(tests_compress_options_enum_lz4_block_size);
    D_CO_RUN(tests_compress_options_enum_lz4_block_mode);
    D_CO_RUN(tests_compress_options_enum_brotli_mode);
    D_CO_RUN(tests_compress_options_enum_neutral_defaults_are_zero);
    D_CO_RUN(tests_compress_options_enum_are_plain);

    // -- DDeflateOptions ------------------------------------------------------
    D_CO_RUN(tests_compress_options_deflate_defaults);
    D_CO_RUN(tests_compress_options_deflate_fields);
    D_CO_RUN(tests_compress_options_deflate_layout);

    // -- DBzip2Options --------------------------------------------------------
    D_CO_RUN(tests_compress_options_bzip2_defaults);
    D_CO_RUN(tests_compress_options_bzip2_fields);
    D_CO_RUN(tests_compress_options_bzip2_layout);

    // -- DLzmaOptions ---------------------------------------------------------
    D_CO_RUN(tests_compress_options_lzma_defaults);
    D_CO_RUN(tests_compress_options_lzma_sentinels);
    D_CO_RUN(tests_compress_options_lzma_fields);
    D_CO_RUN(tests_compress_options_lzma_layout);

    // -- DZstdOptions ---------------------------------------------------------
    D_CO_RUN(tests_compress_options_zstd_defaults);
    D_CO_RUN(tests_compress_options_zstd_flag_defaults);
    D_CO_RUN(tests_compress_options_zstd_fields);
    D_CO_RUN(tests_compress_options_zstd_layout);

    // -- DLz4Options ----------------------------------------------------------
    D_CO_RUN(tests_compress_options_lz4_defaults);
    D_CO_RUN(tests_compress_options_lz4_fields);
    D_CO_RUN(tests_compress_options_lz4_layout);

    // -- DBrotliOptions -------------------------------------------------------
    D_CO_RUN(tests_compress_options_brotli_defaults);
    D_CO_RUN(tests_compress_options_brotli_fields);
    D_CO_RUN(tests_compress_options_brotli_layout);

    // -- compress_options (aggregate) -----------------------------------------
    D_CO_RUN(tests_compress_options_aggregate_level_default);
    D_CO_RUN(tests_compress_options_aggregate_subobjects_defaulted);
    D_CO_RUN(tests_compress_options_aggregate_independence);
    D_CO_RUN(tests_compress_options_aggregate_value_semantics);
    D_CO_RUN(tests_compress_options_aggregate_layout);
    D_CO_RUN(tests_compress_options_aggregate_minus_one_sentinels);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_CO_RUN
