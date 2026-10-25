/*******************************************************************************
* djinterp [net]                                             tcp_tests_sa_main.c
*
* The TCP transport's standalone test runner.
*   Runs every test, reports each, and counts tests and assertions. A
* watchdog ends a run that hangs -- a blocked accept that never woke --
* instead of letting it wait forever.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./tcp_tests_sa.h"  // the suite
// std
#include <stdio.h>   // printf


/*
d_tests_sa_tcp_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_tcp_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_tcp_case CASES[] =
    {
        { "options",             d_tests_sa_tcp_options },
        { "closed connections",  d_tests_sa_tcp_closed },
        { "argument refusals",   d_tests_sa_tcp_refusals },
        { "loopback",            d_tests_sa_tcp_loopback },
        { "half close",          d_tests_sa_tcp_half_close },
        { "the net.h view",      d_tests_sa_tcp_net_view },
        { "refused connections", d_tests_sa_tcp_refused },
        { "resolved names",      d_tests_sa_tcp_names },
        { "adopt and release",   d_tests_sa_tcp_adopt },
        { "adopting as UDP",     d_tests_sa_tcp_adopt_udp },
        { "non-blocking reads",  d_tests_sa_tcp_would_block },
        { "Nagle at both ends",  d_tests_sa_tcp_nagle },
        { "native helpers",      d_tests_sa_tcp_native },
        { "listener errors",     d_tests_sa_tcp_listener_errors },
        { "unix-domain",         d_tests_sa_tcp_unix },
        { "unix-domain paths",   d_tests_sa_tcp_unix_paths },
        { "accept wake",         d_tests_sa_tcp_accept_wake },
        { "bulk transfers",      d_tests_sa_tcp_bulk },
        { "frames",              d_tests_sa_tcp_frames },
        { "pumping",             d_tests_sa_tcp_pump },
        { "read timeouts",       d_tests_sa_tcp_read_timeout },
        { "limit values",        d_tests_sa_tcp_limit_values },
        { "write timeouts",      d_tests_sa_tcp_write_timeout },
        { "limited accepts",     d_tests_sa_tcp_accept_wait },
        { "waiting bytes",       d_tests_sa_tcp_available },
        { "the host name",       d_tests_sa_tcp_host_name }
    };
    bool all = true;

    // each case in order, counted and reported
    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        const bool passed = CASES[i].run(_counter);

        _counter->tests_total  += 1u;
        _counter->tests_passed += (passed) ? 1u : 0u;
        printf("  %s  %s\n",
               (passed) ? "pass" : "FAIL",
               CASES[i].name);
        all = (passed) && all;
    }

    return all;
}

/*
main
  Arms the watchdog, counts from zero, runs every test, and prints tests
before assertions, as the net suite does.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_tests_tcp_watchdog(60u);
    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] tcp standalone tests\n");

    const bool passed = d_tests_sa_tcp_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
