/*******************************************************************************
* djinterp [c][net]                                                  tcp_tests.c
*
* The section of net's TCP suite, in DTest's C face: every test, in the
* order run, each a test of its own in module tcp. A watchdog ends a run
* that hangs -- a blocked accept that never woke -- instead of letting it
* wait forever.
*
*
* path:      /tests/djinterp/c/modules/net/tcp/tcp_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./tcp_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_tcp
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_tcp(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_tcp_case CASES[] =
    {
        { "options",             d_tests_tcp_options },
        { "closed connections",  d_tests_tcp_closed },
        { "argument refusals",   d_tests_tcp_refusals },
        { "loopback",            d_tests_tcp_loopback },
        { "half close",          d_tests_tcp_half_close },
        { "the net.h view",      d_tests_tcp_net_view },
        { "refused connections", d_tests_tcp_refused },
        { "resolved names",      d_tests_tcp_names },
        { "adopt and release",   d_tests_tcp_adopt },
        { "adopting as UDP",     d_tests_tcp_adopt_udp },
        { "non-blocking reads",  d_tests_tcp_would_block },
        { "Nagle at both ends",  d_tests_tcp_nagle },
        { "native helpers",      d_tests_tcp_native },
        { "listener errors",     d_tests_tcp_listener_errors },
        { "unix-domain",         d_tests_tcp_unix },
        { "unix-domain paths",   d_tests_tcp_unix_paths },
        { "accept wake",         d_tests_tcp_accept_wake },
        { "bulk transfers",      d_tests_tcp_bulk },
        { "frames",              d_tests_tcp_frames },
        { "pumping",             d_tests_tcp_pump },
        { "read timeouts",       d_tests_tcp_read_timeout },
        { "limit values",        d_tests_tcp_limit_values },
        { "write timeouts",      d_tests_tcp_write_timeout },
        { "limited accepts",     d_tests_tcp_accept_wait },
        { "waiting bytes",       d_tests_tcp_available },
        { "the host name",       d_tests_tcp_host_name }
    };
    bool all = true;

    // a run that hangs ends instead of waiting forever
    d_tests_tcp_watchdog(60u);

    (void)d_test_suite_module(_suite, "tcp");
    (void)d_test_suite_describe(_suite,
                                "TCP and unix-domain transports, listeners "
                                "and their limits");

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
