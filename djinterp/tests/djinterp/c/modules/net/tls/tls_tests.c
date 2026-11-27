/*******************************************************************************
* djinterp [c][net]                                                  tls_tests.c
*
* The section of net's TLS suite, in DTest's C face: every test, in the
* order run, each a test of its own in module tls, once the run's world -
* an engine and certificates - is open.
*
*
* path:      /tests/djinterp/c/modules/net/tls/tls_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "./tls_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_tls_cases
  Every case in order, each in a test of its own. True if every one held.
*/
static
bool d_tests_tls_cases(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_tls_case CASES[] =
    {
        { "contexts",               d_tests_tls_contexts },
        { "session init",           d_tests_tls_init },
        { "handshakes",             d_tests_tls_handshake },
        { "exchanges",              d_tests_tls_exchange },
        { "resumption",             d_tests_tls_resume },
        { "what a session awaits",  d_tests_tls_wants },
        { "close_notify",           d_tests_tls_close_notify },
        { "truncation",             d_tests_tls_truncation },
        { "transport failures",     d_tests_tls_transport_failure },
        { "host names",             d_tests_tls_names },
        { "trust",                  d_tests_tls_trust },
        { "ALPN",                   d_tests_tls_alpn },
        { "plaintext first",        d_tests_tls_plaintext_first },
        { "not TLS",                d_tests_tls_not_tls },
        { "view transfers",         d_tests_tls_view_transfers },
        { "view ends",              d_tests_tls_view_ends },
        { "view failures",          d_tests_tls_view_failures },
        { "view ownership",         d_tests_tls_view_ownership },
        { "view shutdown",          d_tests_tls_view_shutdown },
        { "destroyed views",        d_tests_tls_view_destroyed },
        { "view endpoints",         d_tests_tls_view_endpoints },
        { "TLS over TLS",           d_tests_tls_nested },
        { "names an engine gets",   d_tests_tls_engine_names },
        { "engine session_create",  d_tests_tls_engine_creates },
        { "engine feed and drain",  d_tests_tls_engine_steps }
    };
    bool all = true;

    // each case in order, in a test of its own
    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        (void)d_test_suite_test(_suite,
                                CASES[i].name);

        const bool passed = CASES[i].run(_suite);
        all = (passed) && all;
    }

    return all;
}

/*
d_tests_tls
  The suite's section: opens its module, and the run's world - an
OpenSSL-family engine, and certificates made for the run - then the cases.
Without the world there is nothing to test with: one test says so, skipped,
not passed.
*/
bool d_tests_tls(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_module(_suite, "tls");
    (void)d_test_suite_describe(_suite,
                                "TLS contexts, sessions and the engine's "
                                "view, over an OpenSSL-family engine and "
                                "certificates the run makes");

    // no engine, or no certificates: nothing to test with
    if (!d_tests_tls_world_open())
    {
        (void)d_test_suite_test(_suite, "the run's world");
        (void)d_test_suite_skip(_suite,
                                "no OpenSSL-family engine, or no "
                                "certificates");

        return true;
    }

    const bool all = d_tests_tls_cases(_suite);

    d_tests_tls_world_close();

    return all;
}
