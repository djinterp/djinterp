/*******************************************************************************
* djinterp [c][net]                                                  net_tests.c
*
* The section of net's foundation suite, in DTest's C face: every test, in
* the order run, each a test of its own in module net. The tests are in
* net_tests_vocabulary.c, net_tests_memory.c, net_tests_stream.c and
* net_tests_url.c; the doubles they run against in net_tests_support.c.
*
*
* path:      /tests/djinterp/c/modules/net/net_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./net_tests.h"  // the suite


// dj_tests_net_case
//   struct: a named test, as the section's table lists it.
struct dj_tests_net_case
{
    const char* name;
    bool        (*run)(struct dj_test_suite* _suite);
};


/*
d_tests_net
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_net(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_net_case CASES[] =
    {
        { "errors",           d_tests_net_errors },
        { "results",          d_tests_net_results },
        { "ports",            d_tests_net_ports },
        { "endpoint records", d_tests_net_endpoint_records },
        { "endpoint parsing", d_tests_net_endpoint_parse },
        { "endpoint text",    d_tests_net_endpoint_format },
        { "frame codec",      d_tests_net_frame_codec },
        { "dispatch",         d_tests_net_dispatch },
        { "completeness",     d_tests_net_complete },
        { "lifetime",         d_tests_net_lifetime },
        { "available",        d_tests_net_available },
        { "pipes",            d_tests_net_pipes },
        { "pipe ends",        d_tests_net_pipe_ends },
        { "memory streams",   d_tests_net_memory },
        { "exact transfers",  d_tests_net_exact },
        { "bulk transfers",   d_tests_net_bulk },
        { "url components",   d_tests_net_url_components },
        { "url refusals",     d_tests_net_url_refusals },
        { "url authorities",  d_tests_net_url_authority },
        { "url hosts",        d_tests_net_url_hosts },
        { "url format",       d_tests_net_url_format },
        { "url resolution",   d_tests_net_url_resolve },
        { "url normal forms", d_tests_net_url_normalize },
        { "url escapes",      d_tests_net_url_percent },
        { "url endpoints",    d_tests_net_url_endpoints },
        { "url net errors",   d_tests_net_url_errors_to_net },
        { "frames",           d_tests_net_frames },
        { "frames to a sink", d_tests_net_frames_sink }
    };
    bool all = true;

    (void)d_test_suite_module(_suite, "net");
    (void)d_test_suite_describe(_suite,
                                "net's foundation: errors, results, "
                                "endpoints, connections, the algorithms over "
                                "them, framing and URLs");

    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        (void)d_test_suite_test(_suite,
                                CASES[i].name);

        const bool passed = CASES[i].run(_suite);

        all = (passed) && all;
    }

    return all;
}
