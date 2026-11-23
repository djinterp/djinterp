/*******************************************************************************
* djinterp [c][net]                                                 smtp_tests.c
*
* The section of net's SMTP suite, in DTest's C face: every test, in the
* order run, each a test of its own in module smtp, between the TLS
* loopback's setup and its teardown.
*
*
* path:      /tests/djinterp/c/modules/net/smtp/smtp_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./smtp_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
// std
#include <stdbool.h>  // bool


// CASES
//   constant: every SMTP test, in the order the section runs them.
static const struct dj_tests_smtp_case CASES[] =
{
    { "buffers",                d_tests_smtp_buffer },
    { "replies",                d_tests_smtp_reply },
    { "capabilities",           d_tests_smtp_capabilities },
    { "validation",             d_tests_smtp_validation },
    { "commands",               d_tests_smtp_commands },
    { "base64",                 d_tests_smtp_base64 },
    { "message data",           d_tests_smtp_data },
    { "errors to net",          d_tests_smtp_errors_to_net },
    { "errors from net",        d_tests_smtp_errors_from_net },
    { "errors from SSL",        d_tests_smtp_errors_from_ssl },
    { "reader, lines",          d_tests_smtp_reader_lines },
    { "reader, limits",         d_tests_smtp_reader_limits },
    { "reader, failures",       d_tests_smtp_reader_failures },
    { "client, pipelined",      d_tests_smtp_client_script },
    { "client, CRAM-MD5",       d_tests_smtp_client_cram },
    { "client, serial",         d_tests_smtp_client_serial },
    { "client, guards",         d_tests_smtp_client_guards },
    { "client, attach",         d_tests_smtp_client_attach },
    { "client, attach_with",    d_tests_smtp_client_attach_with },
    { "client, failures",       d_tests_smtp_client_failures },
    { "client, replies",        d_tests_smtp_client_replies },
    { "client, messages",       d_tests_smtp_client_messages },
    { "client, batches",        d_tests_smtp_client_batches },
    { "server, scripted",       d_tests_smtp_server_script },
    { "server, AUTH",           d_tests_smtp_server_auth },
    { "server, SASL ends",      d_tests_smtp_server_sasl_ends },
    { "server, arguments",      d_tests_smtp_server_arguments },
    { "server, contexts",       d_tests_smtp_server_contexts },
    { "server, failures",       d_tests_smtp_server_failures },
    { "STARTTLS, client",       d_tests_smtp_starttls_client },
    { "STARTTLS, unnamed",      d_tests_smtp_starttls_unnamed },
    { "STARTTLS, refusals",     d_tests_smtp_starttls_refusals },
    { "STARTTLS, injection",    d_tests_smtp_starttls_injection },
    { "STARTTLS, failures",     d_tests_smtp_starttls_failures },
    { "STARTTLS, after",        d_tests_smtp_starttls_after },
    { "errors from TLS",        d_tests_smtp_errors_from_tls },
    { "STARTTLS, server",       d_tests_smtp_starttls_server },
    { "STARTTLS, server refusals",
                                d_tests_smtp_starttls_server_refusals },
    { "STARTTLS, server failures",
                                d_tests_smtp_starttls_server_failures },
    { "loopback",               d_tests_smtp_loopback },
    { "loopback, TLS",          d_tests_smtp_loopback_tls },
    { "loopback, wrong name",   d_tests_smtp_loopback_mismatch },
    { "loopback, TLS policy",   d_tests_smtp_loopback_policy },
    { "connect, arguments",     d_tests_smtp_connect_arguments },
    { "connect, failures",      d_tests_smtp_connect_failures },
    { "connect, options",       d_tests_smtp_connect_options },
    { "identity",               d_tests_smtp_identity }
};

/*
d_tests_smtp
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_smtp(
    struct dj_test_suite* _suite
)
{
    bool all = true;

    (void)d_test_suite_module(_suite, "smtp");
    (void)d_test_suite_describe(_suite,
                                "SMTP's client, server and reader, over a "
                                "scripted peer and a TLS loopback");

    // the certificates the TLS loopback runs on, when they can be made
    if (!d_tests_smtp_loopback_setup())
    {
        (void)d_test_suite_note(_suite,
                                "no engine or certificates: the TLS loopback "
                                "skips");
    }

    // each case in order, in a test of its own
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        (void)d_test_suite_test(_suite,
                                CASES[i].name);

        const bool passed = CASES[i].run(_suite);

        all = (passed) && all;
    }

    d_tests_smtp_loopback_teardown();

    return all;
}
