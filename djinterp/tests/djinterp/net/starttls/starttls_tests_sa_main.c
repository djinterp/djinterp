/*******************************************************************************
* djinterp [net]                                        starttls_tests_sa_main.c
*
* The STARTTLS module's standalone test runner.
*   Runs every test, reports each, and counts tests and assertions. The
* scripted and TCP tests always run; the OpenSSL-family engine's run only
* when the engine is compiled and the certificates they use could be made,
* which are made before them and removed after. A watchdog alarm ends a run
* that hangs instead of letting it wait forever.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/
// enable POSIX.1-2008 (alarm) under a strict C compiler; a feature-test
// macro must precede every include, so it sits ahead of the suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./starttls_tests_sa.h"  // the suite
// std
#include <stdio.h>   // printf
// posix
#include <unistd.h>  // alarm


// D_TESTS_STARTTLS_CASES
//   constant: the tests every run makes, in the order run.
static const struct d_tests_starttls_case D_TESTS_STARTTLS_CASES[] =
{
    { "defaults",             d_tests_sa_starttls_defaults },
    { "init arguments",       d_tests_sa_starttls_init_arguments },
    { "init values",          d_tests_sa_starttls_init_values },
    { "init contexts",        d_tests_sa_starttls_init_contexts },
    { "new session",          d_tests_sa_starttls_new_session },
    { "close",                d_tests_sa_starttls_close },
    { "ownership",            d_tests_sa_starttls_ownership },
    { "NULL queries",         d_tests_sa_starttls_null_queries },
    { "names",                d_tests_sa_starttls_names },
    { "errors to net",        d_tests_sa_starttls_errors_to_net },
    { "errors from net",      d_tests_sa_starttls_errors_from_net },
    { "lines",                d_tests_sa_starttls_lines },
    { "lines trickled",       d_tests_sa_starttls_lines_trickled },
    { "line endings",         d_tests_sa_starttls_line_endings },
    { "failed session",       d_tests_sa_starttls_failed_session },
    { "line limit",           d_tests_sa_starttls_line_limit },
    { "literal",              d_tests_sa_starttls_literal },
    { "byte refusals",        d_tests_sa_starttls_byte_refusals },
    { "stream ends",          d_tests_sa_starttls_stream_ends },
    { "send line",            d_tests_sa_starttls_send_line },
    { "send limit",           d_tests_sa_starttls_send_limit },
    { "send long",            d_tests_sa_starttls_send_long },
    { "broken writes",        d_tests_sa_starttls_broken_writes },
    { "broken reads",         d_tests_sa_starttls_broken_reads },
    { "policy and offer",     d_tests_sa_starttls_policy_offer },
    { "policy and reply",     d_tests_sa_starttls_policy_reply },
    { "policy and callbacks", d_tests_sa_starttls_policy_callbacks },
    { "negotiate refusals",   d_tests_sa_starttls_negotiate_refusals },
    { "client upgrade",       d_tests_sa_starttls_upgrade_client },
    { "secure traffic",       d_tests_sa_starttls_secure_traffic },
    { "server upgrade",       d_tests_sa_starttls_upgrade_server },
    { "implicit upgrade",     d_tests_sa_starttls_upgrade_implicit },
    { "client injection",     d_tests_sa_starttls_injection_client },
    { "server injection",     d_tests_sa_starttls_injection_server },
    { "leftover dropped",     d_tests_sa_starttls_leftover_dropped },
    { "engine failures",      d_tests_sa_starttls_engine_failures },
    { "failure details",      d_tests_sa_starttls_failure_details },
    { "upgrade disabled",     d_tests_sa_starttls_upgrade_disabled },
    { "secure end",           d_tests_sa_starttls_secure_end },
    { "secure failures",      d_tests_sa_starttls_secure_failures },
    { "optional client cert", d_tests_sa_starttls_optional_client },
    { "TCP exchange",         d_tests_sa_starttls_tcp_exchange },
    { "TCP ownership",        d_tests_sa_starttls_tcp_ownership },
    { "TCP deadline",         d_tests_sa_starttls_tcp_deadline },
    { "TCP peer vanished",    d_tests_sa_starttls_tcp_vanished }
};

// D_TESTS_STARTTLS_OPENSSL_CASES
//   constant: the tests run against the OpenSSL-family engine, when it and
// the run's certificates are there, in the order run.
static const struct d_tests_starttls_case D_TESTS_STARTTLS_OPENSSL_CASES[] =
{
    { "OpenSSL STARTTLS",            d_tests_sa_starttls_openssl_starttls },
    { "OpenSSL implicit TLS",        d_tests_sa_starttls_openssl_implicit },
    { "OpenSSL verification",        d_tests_sa_starttls_openssl_verify },
    { "OpenSSL unverified",          d_tests_sa_starttls_openssl_unverified },
    { "OpenSSL truncation",          d_tests_sa_starttls_openssl_truncation },
    { "OpenSSL injection",           d_tests_sa_starttls_openssl_injection },
    { "OpenSSL pipelined command",   d_tests_sa_starttls_openssl_pipelined },
    { "OpenSSL client certificates",
      d_tests_sa_starttls_openssl_client_cert }
};

/*
d_tests_starttls_run_cases
  Runs the `_count` tests of `_cases` in order, reporting each, and counts
tests into `_counter`. True if every one passed.
*/
static bool
d_tests_starttls_run_cases(
    const struct d_tests_starttls_case* _cases,
    size_t                              _count,
    struct d_test_counter*              _counter
)
{
    bool all = true;

    // each case in order, counted and reported
    for (size_t i = 0u; i < _count; ++i)
    {
        const bool passed = _cases[i].run(_counter);

        _counter->tests_total  += 1u;
        _counter->tests_passed += (passed) ? 1u : 0u;
        printf("  %s  %s\n",
               (passed) ? "pass" : "FAIL",
               _cases[i].name);
        all = (passed) && all;
    }

    return all;
}

/*
d_tests_sa_starttls_run_all
  Runs every test, reporting each, and counts tests into `_counter`; the
OpenSSL-family engine's tests run only once their setup succeeds, and are
reported as skipped otherwise.
*/
bool
d_tests_sa_starttls_run_all(
    struct d_test_counter* _counter
)
{
    const size_t COUNT   = sizeof(D_TESTS_STARTTLS_CASES) /
                           sizeof(D_TESTS_STARTTLS_CASES[0]);
    const size_t OPENSSL = sizeof(D_TESTS_STARTTLS_OPENSSL_CASES) /
                           sizeof(D_TESTS_STARTTLS_OPENSSL_CASES[0]);
    bool         all     = d_tests_starttls_run_cases(D_TESTS_STARTTLS_CASES,
                                                      COUNT,
                                                      _counter);

    // no engine, or no certificates: the engine's tests cannot run
    if (!d_tests_sa_starttls_openssl_setup())
    {
        printf("  skip  OpenSSL tests: no OpenSSL-family engine, or no "
               "certificates\n");
        d_tests_sa_starttls_openssl_teardown();

        return all;
    }

    all = d_tests_starttls_run_cases(D_TESTS_STARTTLS_OPENSSL_CASES,
                                     OPENSSL,
                                     _counter) && all;
    d_tests_sa_starttls_openssl_teardown();

    return all;
}

/*
main
  Arms the watchdog, counts from zero, runs every test, and prints tests
before assertions, as the other net suites do.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    (void)alarm(120u);
    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] starttls standalone tests\n");

    const bool passed = d_tests_sa_starttls_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
