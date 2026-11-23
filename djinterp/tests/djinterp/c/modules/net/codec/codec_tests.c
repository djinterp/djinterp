/*******************************************************************************
* djinterp [c][net]                                                codec_tests.c
*
* The section of net's codec suite, in DTest's C face: every test, in the
* order run, each a test of its own in module codec.
*
*
* path:      /tests/djinterp/c/modules/net/codec/codec_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./codec_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


// D_TESTS_CODEC_CASES
//   constant: every test, in the order run.
static const struct dj_tests_codec_case D_TESTS_CODEC_CASES[] =
{
    { "digest sizes",             d_tests_codec_digest_sizes },
    { "MD5 known answers",        d_tests_codec_md5_known },
    { "SHA-1 known answers",      d_tests_codec_sha1_known },
    { "SHA-256 known answers",    d_tests_codec_sha256_known },
    { "a million octets",         d_tests_codec_digest_million },
    { "digests in pieces",        d_tests_codec_digest_pieces },
    { "digest refusals",          d_tests_codec_digest_refusals },
    { "HMAC-MD5 known answers",   d_tests_codec_hmac_md5_known },
    { "HMAC-SHA1 known answers",  d_tests_codec_hmac_sha1_known },
    { "HMAC-SHA-256 answers",     d_tests_codec_hmac_sha256_known },
    { "HMAC keys",                d_tests_codec_hmac_keys },
    { "HMAC refusals",            d_tests_codec_hmac_refusals },
    { "outputs over inputs",      d_tests_codec_digest_overlap },
    { "constant-time comparison", d_tests_codec_digest_equal },
    { "wiping",                   d_tests_codec_wipe },
    { "base64 known answers",     d_tests_codec_base64_known },
    { "base64 refusals",          d_tests_codec_base64_strict },
    { "base64, canonical",        d_tests_codec_base64_canonical },
    { "base64, blanks",           d_tests_codec_base64_blanks },
    { "base64, IMAP's alphabet",  d_tests_codec_base64_modified },
    { "base64 lengths",           d_tests_codec_base64_lengths },
    { "base64 buffers",           d_tests_codec_base64_buffers },
    { "base64 round trips",       d_tests_codec_base64_round_trip },
    { "hex digits",               d_tests_codec_hex_digits },
    { "hex strings",              d_tests_codec_hex_strings },
    { "hex buffers",              d_tests_codec_hex_buffers },
    { "hex edges",                d_tests_codec_hex_edges },
    { "ASCII classes",            d_tests_codec_ascii_classes },
    { "ASCII comparison",         d_tests_codec_ascii_compare },
    { "ASCII trimming",           d_tests_codec_ascii_trim },
    { "ASCII edges",              d_tests_codec_ascii_edges },
    { "line finding",             d_tests_codec_line_find },
    { "line splitting",           d_tests_codec_line_next },
    { "line safety",              d_tests_codec_line_checks },
    { "line buffer",              d_tests_codec_line_buffer },
    { "line buffer room",         d_tests_codec_line_room },
    { "line edges",               d_tests_codec_line_edges },
    { "line buffer held",         d_tests_codec_line_held },
    { "line spills",              d_tests_codec_line_spill },
    { "lines in pieces",          d_tests_codec_line_stream },
    { "errors onto net",          d_tests_codec_errors_to_net }
};

// D_TESTS_CODEC_CASE_COUNT
//   constant: how many tests D_TESTS_CODEC_CASES lists.
#define D_TESTS_CODEC_CASE_COUNT                                               \
    (sizeof(D_TESTS_CODEC_CASES) / sizeof(D_TESTS_CODEC_CASES[0]))

/*
d_tests_codec
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_codec(
    struct dj_test_suite* _suite
)
{
    bool all = true;

    (void)d_test_suite_module(_suite, "codec");
    (void)d_test_suite_describe(_suite,
                                "net's codecs: ASCII, base64, hex, digests "
                                "and lines");

    // each case in order, in a test of its own
    for (size_t i = 0u; i < D_TESTS_CODEC_CASE_COUNT; ++i)
    {
        (void)d_test_suite_test(_suite,
                                D_TESTS_CODEC_CASES[i].name);

        const bool passed = D_TESTS_CODEC_CASES[i].run(_suite);

        all = (passed) && all;
    }

    return all;
}
