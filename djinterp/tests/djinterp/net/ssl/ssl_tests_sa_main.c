/*******************************************************************************
* djinterp [net]                                             ssl_tests_sa_main.c
*
* The SSL kernel's standalone suite: its runner and entry point.
*   Makes the run's certificates and contexts, runs every test, and exits 0
* only if each passed. Without the OpenSSL-family engine, or without the
* means to make certificates, it says so and exits 0 having run nothing.
*
*
* path:      /tests/djinterp/net/ssl/ssl_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssl_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


/*
d_tests_sa_ssl_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_ssl_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_ssl_case CASES[] =
    {
        { "contexts",               d_tests_sa_ssl_contexts },
        { "session init",           d_tests_sa_ssl_init },
        { "handshakes",             d_tests_sa_ssl_handshake },
        { "exchanges",              d_tests_sa_ssl_exchange },
        { "resumption",             d_tests_sa_ssl_resume },
        { "what a session awaits",  d_tests_sa_ssl_wants },
        { "close_notify",           d_tests_sa_ssl_close_notify },
        { "truncation",             d_tests_sa_ssl_truncation },
        { "transport failures",     d_tests_sa_ssl_transport_failure },
        { "host names",             d_tests_sa_ssl_names },
        { "trust",                  d_tests_sa_ssl_trust },
        { "ALPN",                   d_tests_sa_ssl_alpn },
        { "plaintext first",        d_tests_sa_ssl_plaintext_first },
        { "not TLS",                d_tests_sa_ssl_not_tls },
        { "view transfers",         d_tests_sa_ssl_view_transfers },
        { "view ends",              d_tests_sa_ssl_view_ends },
        { "view failures",          d_tests_sa_ssl_view_failures },
        { "view ownership",         d_tests_sa_ssl_view_ownership },
        { "view shutdown",          d_tests_sa_ssl_view_shutdown },
        { "destroyed views",        d_tests_sa_ssl_view_destroyed },
        { "view endpoints",         d_tests_sa_ssl_view_endpoints },
        { "TLS over TLS",           d_tests_sa_ssl_nested },
        { "names an engine gets",   d_tests_sa_ssl_engine_names },
        { "engine session_create",  d_tests_sa_ssl_engine_creates },
        { "engine feed and drain",  d_tests_sa_ssl_engine_steps }
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
  Opens the run's world, or reports why it could not, then runs every test
and prints tests before assertions, as the other suites do.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] ssl standalone tests\n");

    // no engine, or no certificates: nothing to test with
    if (!d_tests_ssl_world_open())
    {
        printf("skipped: no OpenSSL-family engine, or no certificates\n");

        return 0;
    }

    const bool passed = d_tests_sa_ssl_run_all(&counter);

    d_tests_ssl_world_close();
    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
