/*******************************************************************************
* djinterp [test]                                             compress_tests.hpp
*
*   Unit-test header for compress.hpp / compress.cpp - the djinterp portable
* compression facade.  It declares every test function (all flat in
* djinterp::testing, per the framework convention) and the one shared check
* macro the section translation units use; it defines no test bodies itself.
*
*   The suite is split into like-grouped sections, one .cpp per semantic region
* of the facade, each defining the tests_compress_<section>_* functions
* declared below:
*   - compress_tests_status.cpp     the status enum + status_message()
*   - compress_tests_traits.cpp     codec tags, codec_id, codec_traits<>, and
*                                   the availability query
*   - compress_tests_store.cpp      the always-present store codec and the
*                                   store_copy leaf (invalid-argument, verbatim,
*                                   empty, NUL-bearing, large)
*   - compress_tests_roundtrip.cpp  per-codec compress/decompress reproduction
*                                   over the shared corpus, plus stream framing
*   - compress_tests_api.cpp        the non-throwing template API and the
*                                   dispatch leaves (invalid arguments, the
*                                   unavailable path, garbage / truncation
*                                   decode errors, forced backend error)
*   - compress_tests_options.cpp    generic level plumbing and each codec's
*                                   level / block-size clamp
*   - compress_tests_throwing.cpp   the throwing convenience API and
*                                   compression_error (gated on exceptions)
*
*   BUILD-AGNOSTIC BY DESIGN:
*   The facade compiles every codec but only the ones whose backend this build
* detected actually run; the rest return status_unavailable.  The tests honor
* that: a real-codec assertion is gated on the runtime availability query, and
* the unavailable branch is itself asserted (an absent codec must REPORT itself
* absent).  So the whole suite passes on any codec configuration, while still
* exercising both the available and the unavailable dispatch arms for whatever
* mix of libraries is present.
*
*   A short note on coverage of defensively-unreachable lines lives at the top
* of compress_tests_api.cpp.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_COMPRESS_
#define DJINTERP_TESTS_COMPRESS_ 1

// djinterp
#include "../../../../../inc/djinterp/core/util/compress/compress.hpp"        // the facade under test
#include "../../../../../inc/djinterp/test/compress/test_compress.hpp"   // shared compression-testing helpers


// D_CP_CHECK
//   macro: the section suites' assertion primitive.  Evaluates _cond and, if
// it is false, returns false from the enclosing bool() test function
// immediately (short-circuiting the rest of that test).  Unique two-letter
// prefix (CP) so it never collides with a co-compiled suite's macro.
#define D_CP_CHECK(_cond)                                                       \
    do                                                                         \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (false)


NS_DJINTERP
NS_TESTING


// -- compress_tests_status.cpp ------------------------------------------------
bool tests_compress_status_messages();
bool tests_compress_status_unknown();
bool tests_compress_status_enum_values();

// -- compress_tests_traits.cpp ------------------------------------------------
bool tests_compress_traits_ids();
bool tests_compress_traits_names();
bool tests_compress_traits_store_always_available();
bool tests_compress_traits_runtime_matches_dispatch();
bool tests_compress_traits_out_of_range_unavailable();

// -- compress_tests_store.cpp -------------------------------------------------
bool tests_compress_store_roundtrip();
bool tests_compress_store_verbatim();
bool tests_compress_store_empty();
bool tests_compress_store_embedded_nuls();
bool tests_compress_store_all_byte_values();
bool tests_compress_store_large();
bool tests_compress_store_invalid_argument();
bool tests_compress_store_null_zero_ok();

// -- compress_tests_roundtrip.cpp ---------------------------------------------
bool tests_compress_roundtrip_deflate();
bool tests_compress_roundtrip_zlib();
bool tests_compress_roundtrip_gzip();
bool tests_compress_roundtrip_bzip2();
bool tests_compress_roundtrip_xz();
bool tests_compress_roundtrip_zstd();
bool tests_compress_roundtrip_lz4();
bool tests_compress_roundtrip_brotli();
bool tests_compress_roundtrip_signatures();

// -- compress_tests_api.cpp ---------------------------------------------------
bool tests_compress_api_template_dispatch();
bool tests_compress_api_invalid_argument_compress();
bool tests_compress_api_invalid_argument_decompress();
bool tests_compress_api_out_of_range_unavailable();
bool tests_compress_api_decompress_garbage();
bool tests_compress_api_decompress_garbage_brotli();
bool tests_compress_api_deflate_bad_level();
bool tests_compress_api_zlib_truncated();

// -- compress_tests_options.cpp -----------------------------------------------
bool tests_compress_options_defaults();
bool tests_compress_options_deflate_levels();
bool tests_compress_options_bzip2_block_sizes();
bool tests_compress_options_xz_levels();
bool tests_compress_options_zstd_levels();
bool tests_compress_options_brotli_levels();
bool tests_compress_options_store_ignores_level();

// -- compress_tests_throwing.cpp ----------------------------------------------
bool tests_compress_throwing_success_matches_try();
bool tests_compress_throwing_roundtrip();
bool tests_compress_throwing_throws_on_failure();
bool tests_compress_throwing_what_message();
bool tests_compress_throwing_error_type();
bool tests_compress_throwing_decompress_throws();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_COMPRESS_
