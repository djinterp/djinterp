/*******************************************************************************
* djinterp [c][net]                                                 curl_tests.c
*
* The section of the libcurl binding's suite, in DTest's C face: every test,
* in the order run, each a test of its own in module curl.
*
*
* path:      /tests/djinterp/c/modules/net/curl/curl_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./curl_tests.h"  // the suite


/*
d_tests_curl
  The suite's section: opens its module, then a test per case, which the
case's checks land in. libcurl's global state is set up first, before the
peers' threads start, as libcurl asks where its initialization isn't
thread-safe. Returns whether every case held.
*/
bool d_tests_curl(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_curl_case CASES[] =
    {
        { "the default version",            d_tests_curl_options_default },
        { "an unlisted version",            d_tests_curl_version_refused },
        { "HTTP/2 without its support",     d_tests_curl_without_http2 },
        { "HTTP/2 by prior knowledge",      d_tests_curl_prior_knowledge },
        { "HTTP/2 by an upgrade",           d_tests_curl_upgrade },
        { "an upgrade declined",            d_tests_curl_upgrade_declined },
        { "HTTP/1.1 only",                  d_tests_curl_http1_only },
        { "HTTP/1.0",                       d_tests_curl_http1_0 },
        { "HTTP/2 over TLS, in the clear",  d_tests_curl_tls_only_in_clear },
        { "libcurl's choice, in the clear", d_tests_curl_default_in_clear },
        { "prior knowledge refused",
          d_tests_curl_prior_knowledge_refused },
        { "HTTP/3 without its support",     d_tests_curl_without_http3 },
        { "HTTP/3 to a named server",       d_tests_curl_http3_server },
        { "HTTP/3 in the clear",            d_tests_curl_http3_in_clear }
    };
    bool all = true;

    (void)d_curl_global_init();
    (void)d_test_suite_module(_suite, "curl");
    (void)d_test_suite_describe(_suite,
                                "the libcurl binding: the HTTP version asked "
                                "for, and the one a response came in");

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
