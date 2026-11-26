/*******************************************************************************
* djinterp [c][net]                                                 lmtp_tests.c
*
* The section of net's LMTP tests: one DTest module, lmtp, with a test for
* each case, in the order they build on each other.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "./lmtp_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


// CASES
//   constant: every LMTP test, in the order the section runs them.
static const struct dj_tests_lmtp_case CASES[] =
{
    { "client, pipelined",          d_tests_lmtp_client_pipelined },
    { "client, every one refused",  d_tests_lmtp_client_refused },
    { "client, hung up on",         d_tests_lmtp_client_hang_up },
    { "client, no HELO",            d_tests_lmtp_client_no_helo },
    { "client, serial",             d_tests_lmtp_client_serial },
    { "client, data",               d_tests_lmtp_client_data },
    { "client, guards",             d_tests_lmtp_client_guards },
    { "SMTP's deliveries",          d_tests_lmtp_smtp_deliveries },
    { "server, each recipient",     d_tests_lmtp_server_each },
    { "server, hellos",             d_tests_lmtp_server_hellos },
    { "server, no recipient",       d_tests_lmtp_server_no_recipient },
    { "server, one verdict",        d_tests_lmtp_server_one_verdict },
    { "server, oversize",           d_tests_lmtp_server_oversize },
    { "server, SMTP's",             d_tests_lmtp_server_smtp },
    { "loopback, TCP",              d_tests_lmtp_loopback_tcp },
    { "loopback, unix-domain",      d_tests_lmtp_loopback_unix },
    { "loopback, an SMTP server",   d_tests_lmtp_loopback_smtp_server },
    { "a real server",              d_tests_lmtp_live }
};

/*
d_tests_lmtp
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_lmtp(
    struct dj_test_suite* _suite
)
{
    bool all = true;

    (void)d_test_suite_module(_suite, "lmtp");
    (void)d_test_suite_describe(_suite,
                                "LMTP's client and server, over a scripted "
                                "peer, loopback, and a real server where one "
                                "is named");

    // each case in order, in a test of its own
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        (void)d_test_suite_test(_suite,
                                CASES[i].name);

        const bool passed = CASES[i].run(_suite);

        all = (passed) && all;
    }

    return all;
}
