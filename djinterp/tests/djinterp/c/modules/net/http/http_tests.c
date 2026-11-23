/*******************************************************************************
* djinterp [c][net]                                                 http_tests.c
*
* The section of net's HTTP suite, in DTest's C face: every test, in the
* order run, each a test of its own in module http.
*
*
* path:      /tests/djinterp/c/modules/net/http/http_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./http_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_http
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_http(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_http_case CASES[] =
    {
        { "methods",        d_tests_http_methods },
        { "versions",       d_tests_http_versions },
        { "status codes",   d_tests_http_status },
        { "reason phrases", d_tests_http_reasons },
        { "field syntax",   d_tests_http_syntax },
        { "common names",   d_tests_http_names },
        { "error names",    d_tests_http_errors },
        { "errors to net",  d_tests_http_errors_to_net },
        { "http/1.1 heads", d_tests_http1_heads },
        { "head parts",     d_tests_http1_parts },
        { "byte at a time", d_tests_http1_incremental },
        { "head limits",    d_tests_http1_limits },
        { "field lookup",   d_tests_http1_lookup }
    };
    bool all = true;

    (void)d_test_suite_module(_suite, "http");
    (void)d_test_suite_describe(_suite,
                                "HTTP's vocabulary and HTTP/1.1's parser and "
                                "writer");

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
