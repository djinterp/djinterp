/*******************************************************************************
* djinterp [net]                                            smtp_tests_sa_main.c
*
* djinterp SMTP kernel tests -- runner.
*   Runs the standalone SMTP suite and exits non-zero if any test failed.
* Build with the four sources under src/djinterp/net/smtp, net.c, tcp.c,
* the SSL kernel and its OpenSSL-family engine (src/djinterp/net/ssl), net's
* codec units, sink_common.c, the suites' shared support
* (tests/djinterp/net/net_tests_sa_assert.c, net_tests_sa_support.c,
* net_tests_peer.c, net_tests_pki.c), -lssl -lcrypto, and -pthread.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./smtp_tests_sa.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf


// CASES
//   constant: every SMTP test, in the order the runner runs them.
static const struct d_tests_smtp_case CASES[] =
{
    { "buffers",                d_tests_sa_smtp_buffer },
    { "replies",                d_tests_sa_smtp_reply },
    { "capabilities",           d_tests_sa_smtp_capabilities },
    { "validation",             d_tests_sa_smtp_validation },
    { "commands",               d_tests_sa_smtp_commands },
    { "base64",                 d_tests_sa_smtp_base64 },
    { "message data",           d_tests_sa_smtp_data },
    { "errors to net",          d_tests_sa_smtp_errors_to_net },
    { "errors from net",        d_tests_sa_smtp_errors_from_net },
    { "errors from SSL",        d_tests_sa_smtp_errors_from_ssl },
    { "reader, lines",          d_tests_sa_smtp_reader_lines },
    { "reader, limits",         d_tests_sa_smtp_reader_limits },
    { "reader, failures",       d_tests_sa_smtp_reader_failures },
    { "client, pipelined",      d_tests_sa_smtp_client_script },
    { "client, CRAM-MD5",       d_tests_sa_smtp_client_cram },
    { "client, serial",         d_tests_sa_smtp_client_serial },
    { "client, guards",         d_tests_sa_smtp_client_guards },
    { "client, attach",         d_tests_sa_smtp_client_attach },
    { "client, attach_with",    d_tests_sa_smtp_client_attach_with },
    { "client, failures",       d_tests_sa_smtp_client_failures },
    { "client, replies",        d_tests_sa_smtp_client_replies },
    { "client, messages",       d_tests_sa_smtp_client_messages },
    { "client, batches",        d_tests_sa_smtp_client_batches },
    { "server, scripted",       d_tests_sa_smtp_server_script },
    { "server, AUTH",           d_tests_sa_smtp_server_auth },
    { "server, SASL ends",      d_tests_sa_smtp_server_sasl_ends },
    { "server, arguments",      d_tests_sa_smtp_server_arguments },
    { "server, contexts",       d_tests_sa_smtp_server_contexts },
    { "server, failures",       d_tests_sa_smtp_server_failures },
    { "STARTTLS, client",       d_tests_sa_smtp_starttls_client },
    { "STARTTLS, unnamed",      d_tests_sa_smtp_starttls_unnamed },
    { "STARTTLS, refusals",     d_tests_sa_smtp_starttls_refusals },
    { "STARTTLS, injection",    d_tests_sa_smtp_starttls_injection },
    { "STARTTLS, failures",     d_tests_sa_smtp_starttls_failures },
    { "STARTTLS, after",        d_tests_sa_smtp_starttls_after },
    { "errors from TLS",        d_tests_sa_smtp_errors_from_tls },
    { "STARTTLS, server",       d_tests_sa_smtp_starttls_server },
    { "STARTTLS, server refusals",
                                d_tests_sa_smtp_starttls_server_refusals },
    { "STARTTLS, server failures",
                                d_tests_sa_smtp_starttls_server_failures },
    { "loopback",               d_tests_sa_smtp_loopback },
    { "loopback, TLS",          d_tests_sa_smtp_loopback_tls },
    { "loopback, wrong name",   d_tests_sa_smtp_loopback_mismatch },
    { "loopback, TLS policy",   d_tests_sa_smtp_loopback_policy },
    { "connect, arguments",     d_tests_sa_smtp_connect_arguments },
    { "connect, failures",      d_tests_sa_smtp_connect_failures },
    { "connect, options",       d_tests_sa_smtp_connect_options },
    { "identity",               d_tests_sa_smtp_identity }
};

/*
d_tests_sa_smtp_run_all
  Runs every SMTP test in order, reporting each, and counts tests into
`_counter`. The loopback setup comes first, so the TLS tests know whether
they can run, and its teardown last.
*/
bool
d_tests_sa_smtp_run_all(
    struct d_test_counter* _counter
)
{
    bool all = true;

    // the certificates the TLS loopback runs on, when they can be made
    if (!d_tests_sa_smtp_loopback_setup())
    {
        printf("  (no engine or certificates: the TLS loopback skips)\n");
    }

    // each test, reported as it finishes
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        const bool passed = CASES[i].run(_counter);

        _counter->tests_total  += 1;
        _counter->tests_passed += (passed) ? 1u : 0u;
        all                     = ( (all) &&
                                    (passed) );
        printf("  [%s] %s\n",
               (passed) ? "PASS" : "FAIL",
               CASES[i].name);
    }

    d_tests_sa_smtp_loopback_teardown();

    return all;
}

/*
main
  Runs the suite, and prints the tally.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] smtp standalone tests\n");

    const bool passed = d_tests_sa_smtp_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
