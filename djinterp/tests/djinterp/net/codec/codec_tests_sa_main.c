/*******************************************************************************
* djinterp [net]                                           codec_tests_sa_main.c
*
* The codec's standalone test runner.
*   Runs every test, reports each, and counts tests and assertions. Build with
* the sources under src/djinterp/net/codec and the suites' assertion
* (tests/djinterp/net/net_tests_sa_assert.c); nothing else is linked.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


// D_TESTS_CODEC_CASES
//   constant: every test, in the order run.
static const struct d_tests_codec_case D_TESTS_CODEC_CASES[] =
{
    { "digest sizes",             d_tests_sa_codec_digest_sizes },
    { "MD5 known answers",        d_tests_sa_codec_md5_known },
    { "SHA-1 known answers",      d_tests_sa_codec_sha1_known },
    { "SHA-256 known answers",    d_tests_sa_codec_sha256_known },
    { "a million octets",         d_tests_sa_codec_digest_million },
    { "digests in pieces",        d_tests_sa_codec_digest_pieces },
    { "digest refusals",          d_tests_sa_codec_digest_refusals },
    { "HMAC-MD5 known answers",   d_tests_sa_codec_hmac_md5_known },
    { "HMAC-SHA1 known answers",  d_tests_sa_codec_hmac_sha1_known },
    { "HMAC-SHA-256 answers",     d_tests_sa_codec_hmac_sha256_known },
    { "HMAC keys",                d_tests_sa_codec_hmac_keys },
    { "HMAC refusals",            d_tests_sa_codec_hmac_refusals },
    { "outputs over inputs",      d_tests_sa_codec_digest_overlap },
    { "constant-time comparison", d_tests_sa_codec_digest_equal },
    { "wiping",                   d_tests_sa_codec_wipe },
    { "base64 known answers",     d_tests_sa_codec_base64_known },
    { "base64 refusals",          d_tests_sa_codec_base64_strict },
    { "base64, canonical",        d_tests_sa_codec_base64_canonical },
    { "base64, blanks",           d_tests_sa_codec_base64_blanks },
    { "base64, IMAP's alphabet",  d_tests_sa_codec_base64_modified },
    { "base64 lengths",           d_tests_sa_codec_base64_lengths },
    { "base64 buffers",           d_tests_sa_codec_base64_buffers },
    { "base64 round trips",       d_tests_sa_codec_base64_round_trip },
    { "hex digits",               d_tests_sa_codec_hex_digits },
    { "hex strings",              d_tests_sa_codec_hex_strings },
    { "hex buffers",              d_tests_sa_codec_hex_buffers },
    { "hex edges",                d_tests_sa_codec_hex_edges },
    { "ASCII classes",            d_tests_sa_codec_ascii_classes },
    { "ASCII comparison",         d_tests_sa_codec_ascii_compare },
    { "ASCII trimming",           d_tests_sa_codec_ascii_trim },
    { "ASCII edges",              d_tests_sa_codec_ascii_edges },
    { "line finding",             d_tests_sa_codec_line_find },
    { "line splitting",           d_tests_sa_codec_line_next },
    { "line safety",              d_tests_sa_codec_line_checks },
    { "line buffer",              d_tests_sa_codec_line_buffer },
    { "line buffer room",         d_tests_sa_codec_line_room },
    { "line edges",               d_tests_sa_codec_line_edges },
    { "line buffer held",         d_tests_sa_codec_line_held },
    { "line spills",              d_tests_sa_codec_line_spill },
    { "lines in pieces",          d_tests_sa_codec_line_stream },
    { "errors onto net",          d_tests_sa_codec_errors_to_net }
};

// D_TESTS_CODEC_CASE_COUNT
//   constant: how many tests D_TESTS_CODEC_CASES lists.
#define D_TESTS_CODEC_CASE_COUNT                                               \
    (sizeof(D_TESTS_CODEC_CASES) / sizeof(D_TESTS_CODEC_CASES[0]))

/*
d_tests_sa_codec_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_codec_run_all(
    struct d_test_counter* _counter
)
{
    bool all = true;

    // each case in order, counted and reported
    for (size_t i = 0u; i < D_TESTS_CODEC_CASE_COUNT; ++i)
    {
        const bool passed = D_TESTS_CODEC_CASES[i].run(_counter);

        _counter->tests_total  += 1u;
        _counter->tests_passed += (passed) ? 1u : 0u;
        printf("  %s  %s\n",
               (passed) ? "pass" : "FAIL",
               D_TESTS_CODEC_CASES[i].name);
        all = (passed) && all;
    }

    return all;
}

/*
main
  Counts from zero, runs every test, and prints tests before assertions, as
the other net suites do.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] codec standalone tests\n");

    const bool passed = d_tests_sa_codec_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
