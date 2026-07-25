/******************************************************************************
* djinterp [tests]                                      test_compress_tests.hpp
*
*   Suite header for test_compress.hpp - the shared verification surface any
* suite reaches for when it needs to check compressed output.  Declares every
* test body, supplies the fixtures the bodies need, and (in DTEST_SPEC_MODE)
* exposes the spec provider the runner hands to run_module.
*
*   BUILD PREREQUISITE - THE RUNNER NEEDS D_TEST_REPORT_ENABLE_ARCHIVE.
*   This is a property of the framework, not of the module under test, and it
* applies to EVERY DTest runner rather than to this suite alone.  A runner
* includes test_defaults.hpp to reach run_module; test_defaults.hpp includes
* output/test_report_runner.hpp; and that header spells `byte_buffer` in two
* places - pending_doc::bytes and write_bytes_to_file's parameter - which is a
* typedef reachable only through the archive include guarded by
* D_TEST_REPORT_ENABLE_ARCHIVE.  Without the macro the runner does not compile
* at C++17 or higher (eight errors).  The leaf CMakeLists therefore defines it
* on the target.  The SECTION translation units do not need it: they include
* the module under test directly and never reach the report machinery.
*
*   The fix belongs in test_report_runner.hpp - spell those two uses
* std::string, or include compress.hpp unconditionally - not here; per the
* authoring guide a broken dependency is reported rather than patched.
*
*   BUILD-AGNOSTIC BY CONSTRUCTION.
*   Which codecs exist is a property of the build, not of the module, and a
* suite that asserted "gzip round-trips" would pass or fail depending on what
* was linked.  So every assertion here is written against the contract the
* module itself documents:
*
*     available   -> the cycle completes and reproduces the input
*     unavailable -> compression SAYS status_unavailable and emits nothing
*
*   That is exactly facade_roundtrip_ok's promise, and it holds on every
* build.  Where a body needs a codec that genuinely works, it uses
* codecs::store - the identity codec, which is always compiled in - so the
* round-trip machinery is exercised for real rather than only in its
* unavailable branch.  Bodies that would otherwise be vacuous on a bare build
* assert BOTH branches explicitly, gated on is_available<_Codec>().
*
*   The signature predicates need no backend at all: they read leading bytes,
* so the suite hands them hand-built buffers and covers every branch - correct
* magic, truncated input, and a wrong byte at each position.
*
*   PORTABILITY:
*   C++11 minimum, matching the header under test.  Built at C++20 by the
* DTest CMake helper.
*
*
* TABLE OF CONTENTS
* =================
* I.    PAYLOADS      (test_compress_tests_payloads.cpp)
* II.   SIGNATURES    (test_compress_tests_signatures.cpp)
* III.  ROUNDTRIP     (test_compress_tests_roundtrip.cpp)
* IV.   VERIFICATION  (test_compress_tests_verification.cpp)
*
*
* path:      /tests/djinterp/test/test_compress_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_COMPRESS_TESTS_
#define DJINTERP_TESTS_TEST_COMPRESS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "test_compress.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"
#endif


NS_DJINTERP
NS_TESTING

// dt
//   type: names the entities under test (djinterp::test).  Unconditional,
// because the spec provider needs dt::module_spec in spec mode.
namespace dt = ::djinterp::test;


// compress_check
//   function: reports a failed assertion and returns the condition, so the
// D_TC_CHECK macro can early-return on the first failure.
//
// Parameter(s):
//   _cond: the asserted condition.
//   _expr: the stringized expression.
//   _file: the source file of the assertion.
//   _line: the source line of the assertion.
// Return:
//   _cond, unchanged.
inline bool
compress_check(
    bool        _cond,
    const char* _expr,
    const char* _file,
    int         _line
)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

// D_TC_CHECK
//   macro: assert _cond, printing the expression and its location and
// returning false from the enclosing test body on failure.  Variadic so a
// top-level comma inside the expression passes through whole.
#define D_TC_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::compress_check(                             \
                (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))             \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// dj
//   type: names the foundation namespace the codec vocabulary lives in.
namespace dj = ::djinterp;


// tc_bytes
//   function: builds a buffer from an explicit unsigned byte list, so a test
// can hand a signature predicate an exact header without escaping games.
inline dj::byte_buffer
tc_bytes(
    const unsigned char* _p,
    std::size_t          _n
)
{
    dj::byte_buffer out;
    std::size_t     i = 0;

    out.reserve(_n);

    for (i = 0; i < _n; ++i)
    {
        out.push_back(static_cast<char>(_p[i]));
    }

    return out;
}

// tc_prefix
//   function: _magic followed by _tail_n filler bytes.  Signature predicates
// are documented to be lenient about everything past the header, so the
// filler proves they really do stop reading where they say they do.
inline dj::byte_buffer
tc_prefix(
    const unsigned char* _magic,
    std::size_t          _magic_n,
    std::size_t          _tail_n
)
{
    dj::byte_buffer out = tc_bytes(_magic, _magic_n);
    std::size_t     i   = 0;

    for (i = 0; i < _tail_n; ++i)
    {
        out.push_back(static_cast<char>(0xA5u));
    }

    return out;
}

// tc_truncations
//   function: every proper prefix of _b, shortest first - the input a
// length-guard test needs to prove a predicate rejects short buffers rather
// than reading past the end.
inline std::vector<dj::byte_buffer>
tc_truncations(
    const dj::byte_buffer& _b
)
{
    std::vector<dj::byte_buffer> out;
    std::size_t                  i = 0;

    for (i = 0; i < _b.size(); ++i)
    {
        out.push_back(_b.substr(0, i));
    }

    return out;
}

// tc_corruptions
//   function: _b repeated once per leading byte, each copy with one byte of
// the first _n flipped - so a magic test can prove EVERY byte of the
// signature is checked, not just the first.
inline std::vector<dj::byte_buffer>
tc_corruptions(
    const dj::byte_buffer& _b,
    std::size_t            _n
)
{
    std::vector<dj::byte_buffer> out;
    std::size_t                  i = 0;

    for (i = 0; (i < _n) && (i < _b.size()); ++i)
    {
        dj::byte_buffer c = _b;

        c[i] = static_cast<char>(
            static_cast<unsigned char>(c[i]) ^ 0xFFu);
        out.push_back(c);
    }

    return out;
}


// tc_zlib_header
//   function: a well-formed two-byte zlib header - DEFLATE in the low nibble
// of CMF, and a FLG chosen so the pair is a multiple of 31.
inline dj::byte_buffer
tc_zlib_header(
    unsigned int _cmf = 0x78u
)
{
    unsigned int flg = 0;

    // find the FLG that satisfies the FCHECK constraint
    for (flg = 0; flg < 256u; ++flg)
    {
        if ((((_cmf << 8) | flg) % 31u) == 0u)
        {
            break;
        }
    }

    dj::byte_buffer out;

    out.push_back(static_cast<char>(static_cast<unsigned char>(_cmf)));
    out.push_back(static_cast<char>(static_cast<unsigned char>(flg)));

    return out;
}


// tc_is_high_entropy
//   function: a coarse spread check - how many distinct byte values appear in
// _b.  Used to distinguish the pseudo-random corpus from the patterned ones
// without asserting on specific generator output.
inline std::size_t
tc_distinct_bytes(
    const dj::byte_buffer& _b
)
{
    bool        seen[256];
    std::size_t n = 0;
    std::size_t i = 0;

    for (i = 0; i < 256u; ++i)
    {
        seen[i] = false;
    }

    for (i = 0; i < _b.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(_b[i]);

        if (!seen[c])
        {
            seen[c] = true;
            ++n;
        }
    }

    return n;
}

// tc_count_byte
//   function: how many bytes of _b equal _c.
inline std::size_t
tc_count_byte(
    const dj::byte_buffer& _b,
    char                   _c
)
{
    std::size_t n = 0;
    std::size_t i = 0;

    for (i = 0; i < _b.size(); ++i)
    {
        if (_b[i] == _c)
        {
            ++n;
        }
    }

    return n;
}


// tc_facade_contract_holds
//   function: the build-agnostic correctness check, spelled out rather than
// delegated, so a failing sweep can say WHICH half of the contract broke.
// Mirrors facade_roundtrip_ok and is asserted to agree with it.
template<typename _Codec>
inline bool
tc_facade_contract_holds(
    const dj::byte_buffer& _in,
    const char*            _name
)
{
    const dt::roundtrip_report r = dt::roundtrip<_Codec>(_in);

    if (r.available)
    {
        if ( (r.compress_status != dj::status_ok)   ||
             (r.decompress_status != dj::status_ok) ||
             (!r.restored_equals_input) )
        {
            std::printf("    [FAIL] %s available but did not round-trip "
                        "(cs=%d ds=%d eq=%d)\n",
                        _name,
                        static_cast<int>(r.compress_status),
                        static_cast<int>(r.decompress_status),
                        static_cast<int>(r.restored_equals_input));

            return false;
        }
    }
    else if (r.compress_status != dj::status_unavailable)
    {
        std::printf("    [FAIL] %s unavailable but reported cs=%d\n",
                    _name, static_cast<int>(r.compress_status));

        return false;
    }

    // and the module's own roll-up must agree with the longhand
    if (dt::facade_roundtrip_ok<_Codec>(_in) != true)
    {
        std::printf("    [FAIL] %s: facade_roundtrip_ok disagreed\n", _name);

        return false;
    }

    return true;
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   PAYLOADS   (test_compress_tests_payloads.cpp)
bool tests_compress_make_repeated_size_and_content();
bool tests_compress_make_repeated_degenerate();
bool tests_compress_make_pattern_cycles_byte_range();
bool tests_compress_make_pattern_degenerate();
bool tests_compress_make_text_length_and_tiling();
bool tests_compress_make_text_degenerate();
bool tests_compress_make_incompressible_is_deterministic();
bool tests_compress_make_incompressible_seed_behaviour();
bool tests_compress_make_incompressible_spread();
bool tests_compress_make_with_nuls_layout();
bool tests_compress_standard_corpus_shape();
bool tests_compress_standard_corpus_is_deterministic();

// II.  SIGNATURES   (test_compress_tests_signatures.cpp)
bool tests_compress_gzip_magic();
bool tests_compress_zlib_magic_accepts_valid_header();
bool tests_compress_zlib_magic_rejects_bad_header();
bool tests_compress_bzip2_magic();
bool tests_compress_xz_magic();
bool tests_compress_zstd_magic();
bool tests_compress_lz4_frame_magic();
bool tests_compress_magic_predicates_reject_empty();
bool tests_compress_expected_signature_dispatch();
bool tests_compress_expected_signature_unframed_codecs();

// III. ROUNDTRIP   (test_compress_tests_roundtrip.cpp)
bool tests_compress_roundtrip_report_is_fully_populated();
bool tests_compress_roundtrip_store_reproduces_input();
bool tests_compress_roundtrip_unavailable_codec_contract();
bool tests_compress_roundtrip_empty_input();
bool tests_compress_roundtrip_large_input();
bool tests_compress_roundtrip_accepts_options();
bool tests_compress_roundtrips_is_strict();
bool tests_compress_facade_roundtrip_ok_is_build_agnostic();
bool tests_compress_facade_roundtrip_ok_over_corpus();
bool tests_compress_roundtrip_never_throws_on_garbage();

// IV.  VERIFICATION   (test_compress_tests_verification.cpp)
bool tests_compress_decompresses_to_matching_payload();
bool tests_compress_decompresses_to_rejects_mismatch();
bool tests_compress_decompresses_to_empty_cases();
bool tests_compress_is_valid_stream_accepts_own_output();
bool tests_compress_is_valid_stream_rejects_undecodable();
bool tests_compress_is_available_matches_facade();
bool tests_compress_is_available_agrees_with_report();
bool tests_compress_verification_helpers_agree();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE

// compress_spec
//   function: the suite's authoritative description - one block per section
// TU, one row per test body, each carrying the descriptor the report renders.
inline dt::module_spec
compress_spec()
{
    return dt::module_spec{
        "test_compress",
        "The shared verification surface for compressed output: deterministic "
        "payload corpora, magic-byte predicates for every framed codec, and "
        "the round-trip and decode checks - written so a suite can assert "
        "them on any build, whatever codecs were actually compiled.",
        {
            dt::block_spec{
                "payloads",
                "The deterministic input builders: their exact lengths, their "
                "documented content, their degenerate cases, and the "
                "reproducibility the whole corpus rests on.",
                {
                    { "tests_compress_make_repeated_size_and_content",
                      "a repeated buffer is exactly the requested length and "
                      "every byte is the requested value",
                      &tests_compress_make_repeated_size_and_content },
                    { "tests_compress_make_repeated_degenerate",
                      "zero length yields an empty buffer, and NUL is a "
                      "usable fill byte",
                      &tests_compress_make_repeated_degenerate },
                    { "tests_compress_make_pattern_cycles_byte_range",
                      "the pattern walks 0x00 to 0xFF and wraps, so it "
                      "exercises the whole byte range including NUL",
                      &tests_compress_make_pattern_cycles_byte_range },
                    { "tests_compress_make_pattern_degenerate",
                      "zero length is empty and a single byte is 0x00",
                      &tests_compress_make_pattern_degenerate },
                    { "tests_compress_make_text_length_and_tiling",
                      "prose is trimmed to the exact requested length and "
                      "tiles the seed phrase to get there",
                      &tests_compress_make_text_length_and_tiling },
                    { "tests_compress_make_text_degenerate",
                      "zero length is empty, and a length shorter than the "
                      "seed is a prefix of it",
                      &tests_compress_make_text_degenerate },
                    { "tests_compress_make_incompressible_is_deterministic",
                      "the same seed reproduces the same bytes on every "
                      "call, which is what makes a failure reproducible",
                      &tests_compress_make_incompressible_is_deterministic },
                    { "tests_compress_make_incompressible_seed_behaviour",
                      "different seeds diverge, and a zero seed falls back to "
                      "the documented non-zero state rather than sticking",
                      &tests_compress_make_incompressible_seed_behaviour },
                    { "tests_compress_make_incompressible_spread",
                      "the generator's output spans the byte range far more "
                      "widely than the patterned corpora do",
                      &tests_compress_make_incompressible_spread },
                    { "tests_compress_make_with_nuls_layout",
                      "every third byte is NUL and the rest walk the "
                      "printable alphabet, so a round-trip proves the path is "
                      "length-carrying",
                      &tests_compress_make_with_nuls_layout },
                    { "tests_compress_standard_corpus_shape",
                      "the corpus carries the ten documented payloads, "
                      "including the empty one and two that cross the 64 KiB "
                      "chunk boundary",
                      &tests_compress_standard_corpus_shape },
                    { "tests_compress_standard_corpus_is_deterministic",
                      "two calls produce byte-identical corpora, so a sweep "
                      "compares like with like",
                      &tests_compress_standard_corpus_is_deterministic },
                }
            },
            dt::block_spec{
                "signatures",
                "The magic-byte predicates: each accepts its own framing, "
                "rejects a wrong byte at every position, and rejects any "
                "buffer too short to carry the header.",
                {
                    { "tests_compress_gzip_magic",
                      "the two gzip identification bytes are required, and "
                      "trailing content is ignored",
                      &tests_compress_gzip_magic },
                    { "tests_compress_zlib_magic_accepts_valid_header",
                      "a header with DEFLATE in the low nibble and a "
                      "31-divisible pair is accepted",
                      &tests_compress_zlib_magic_accepts_valid_header },
                    { "tests_compress_zlib_magic_rejects_bad_header",
                      "a non-DEFLATE method or a failed FCHECK constraint is "
                      "rejected, unlike the fixed-magic codecs",
                      &tests_compress_zlib_magic_rejects_bad_header },
                    { "tests_compress_bzip2_magic",
                      "the three-byte BZh signature is required in full",
                      &tests_compress_bzip2_magic },
                    { "tests_compress_xz_magic",
                      "all six xz signature bytes are checked, so a wrong "
                      "byte anywhere is rejected",
                      &tests_compress_xz_magic },
                    { "tests_compress_zstd_magic",
                      "the four-byte little-endian Zstandard frame magic is "
                      "checked in full",
                      &tests_compress_zstd_magic },
                    { "tests_compress_lz4_frame_magic",
                      "the four-byte little-endian LZ4 frame magic is checked "
                      "in full, and is distinct from Zstandard's",
                      &tests_compress_lz4_frame_magic },
                    { "tests_compress_magic_predicates_reject_empty",
                      "every framed predicate rejects an empty buffer without "
                      "reading past its end",
                      &tests_compress_magic_predicates_reject_empty },
                    { "tests_compress_expected_signature_dispatch",
                      "each codec tag routes to the framing predicate it is "
                      "required to satisfy",
                      &tests_compress_expected_signature_dispatch },
                    { "tests_compress_expected_signature_unframed_codecs",
                      "store, raw DEFLATE and brotli carry no fixed prefix "
                      "and pass unconditionally, even on an empty buffer",
                      &tests_compress_expected_signature_unframed_codecs },
                }
            },
            dt::block_spec{
                "roundtrip",
                "The compress/decompress cycle and its report - covered on "
                "both sides of the availability split, so the assertions hold "
                "whatever this build compiled.",
                {
                    { "tests_compress_roundtrip_report_is_fully_populated",
                      "every field of the report is filled in, including the "
                      "sizes and the availability flag",
                      &tests_compress_roundtrip_report_is_fully_populated },
                    { "tests_compress_roundtrip_store_reproduces_input",
                      "the always-present identity codec completes the cycle "
                      "and reproduces the input exactly",
                      &tests_compress_roundtrip_store_reproduces_input },
                    { "tests_compress_roundtrip_unavailable_codec_contract",
                      "an unavailable codec reports status_unavailable, "
                      "emits no bytes, and does not claim a match",
                      &tests_compress_roundtrip_unavailable_codec_contract },
                    { "tests_compress_roundtrip_empty_input",
                      "an empty payload round-trips to an empty payload "
                      "rather than being treated as a failure",
                      &tests_compress_roundtrip_empty_input },
                    { "tests_compress_roundtrip_large_input",
                      "a payload past the 64 KiB streaming chunk round-trips "
                      "whole, exercising the multi-chunk path",
                      &tests_compress_roundtrip_large_input },
                    { "tests_compress_roundtrip_accepts_options",
                      "an explicit option set is accepted and does not change "
                      "the correctness of the cycle",
                      &tests_compress_roundtrip_accepts_options },
                    { "tests_compress_roundtrips_is_strict",
                      "the strict check requires the codec to be present, so "
                      "it reports false on an unavailable one",
                      &tests_compress_roundtrips_is_strict },
                    { "tests_compress_facade_roundtrip_ok_is_build_agnostic",
                      "the build-agnostic check passes for every codec tag on "
                      "this build, whichever side of the split it falls",
                      &tests_compress_facade_roundtrip_ok_is_build_agnostic },
                    { "tests_compress_facade_roundtrip_ok_over_corpus",
                      "the contract holds for every payload of the standard "
                      "corpus, across every codec",
                      &tests_compress_facade_roundtrip_ok_over_corpus },
                    { "tests_compress_roundtrip_never_throws_on_garbage",
                      "decoding arbitrary non-stream bytes returns a status "
                      "rather than raising, since the helpers are built on "
                      "the non-throwing API",
                      &tests_compress_roundtrip_never_throws_on_garbage },
                }
            },
            dt::block_spec{
                "verification",
                "The helpers a suite points at another module's emitted "
                "bytes: decode-and-compare, well-formedness, and the "
                "availability re-export that gates them.",
                {
                    { "tests_compress_decompresses_to_matching_payload",
                      "a stream that decodes to the expected payload is "
                      "accepted",
                      &tests_compress_decompresses_to_matching_payload },
                    { "tests_compress_decompresses_to_rejects_mismatch",
                      "a stream that decodes to different bytes is rejected, "
                      "including a one-byte difference",
                      &tests_compress_decompresses_to_rejects_mismatch },
                    { "tests_compress_decompresses_to_empty_cases",
                      "the empty payload is handled on both sides of the "
                      "comparison without a false match",
                      &tests_compress_decompresses_to_empty_cases },
                    { "tests_compress_is_valid_stream_accepts_own_output",
                      "a stream the facade itself produced is reported "
                      "well-formed, disregarding its decoded value",
                      &tests_compress_is_valid_stream_accepts_own_output },
                    { "tests_compress_is_valid_stream_rejects_undecodable",
                      "bytes that do not decode under a codec are reported "
                      "malformed rather than silently accepted",
                      &tests_compress_is_valid_stream_rejects_undecodable },
                    { "tests_compress_is_available_matches_facade",
                      "the re-export returns exactly what the facade's own "
                      "availability query does, for every codec tag",
                      &tests_compress_is_available_matches_facade },
                    { "tests_compress_is_available_agrees_with_report",
                      "the standalone query and the report's flag never "
                      "disagree about a codec",
                      &tests_compress_is_available_agrees_with_report },
                    { "tests_compress_verification_helpers_agree",
                      "decode-and-compare implies well-formed, and the two "
                      "helpers agree with the round-trip report",
                      &tests_compress_verification_helpers_agree },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_TEST_COMPRESS_TESTS_
