/*******************************************************************************
* djinterp [c][net]                                                 ftps_tests.c
*
* The section of net's FTPS suite, in DTest's C face, module ftps: the TLS
* layer under the FTP client's own helpers, over ftp_tests_fake.h's scripted
* transport and the OpenSSL engine, as block adapter; then the blocks
* loopback (ftps_tests_loopback.c) and interop (ftps_tests_interop.c).
*
*
* path:      /tests/djinterp/c/modules/net/ftps/ftps_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "./ftps_tests.h"  // the suite
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t
#include <string.h>   // strcpy
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"                // dj_pack_text
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp.h"            // d_ftp_*
#include "../../../../../../inc/djinterp/c/modules/net/ftps/ftps.h"          // d_ftps_*
#include "../../../../../../inc/djinterp/c/modules/net/tls/tls_openssl.h"    // the OpenSSL engine
#include "../../../../../../src/djinterp/c/modules/net/ftp/ftp_client_internal.h"  // the client's helpers
#include "../ftp/ftp_tests_fake.h"  // the scripted transport


// g_suite
//   variable: the suite d_tests_ftps records into; the tests' checks reach it
// through D_TESTS_FTPS_CHECK.
static struct dj_test_suite* g_suite = NULL;

// g_failed
//   variable: how many of this file's checks have failed, so its section can
// say whether its tests passed.
static size_t g_failed = 0u;

/*
d_tests_ftps_held
  Counts a check that failed, and gives back whether it held.
*/
static
bool d_tests_ftps_held(
    bool _held
)
{
    // a failure counts toward the section's result
    if (!_held)
    {
        ++g_failed;
    }

    return _held;
}

// D_TESTS_FTPS_CHECK
//   macro: one check of the open test, recorded with its condition's text,
// file and line.
#define D_TESTS_FTPS_CHECK(c)                                                  \
    d_tests_ftps_held(d_test_suite_check(g_suite,                              \
                                         ((c) ? true : false),                 \
                                         #c,                                   \
                                         __FILE__,                             \
                                         __LINE__))


static struct dj_ftps_client g_fc;

/*
fc_tls
  g_fc's kernel status, as d_ftps_client_tls_status gives it.
*/
static
enum dj_tls_error fc_tls(void)
{
    return d_ftps_client_tls_status(&g_fc);
}

/*
d_tests_ftps_session_of
  The session of g_fc's that runs over `_k`.
*/
static
struct dj_tls_session* d_tests_ftps_session_of(
    const struct dj_ftp_connection* _k
)
{
    return (_k == &g_fc.ftp.control) ? &g_fc.control_tls
                                     : &g_fc.data_tls;
}

// a session in force over the scripted transport, as the layer's secure
// leaves one
static
int secured(
    struct dj_ftp_connection*    _k,
    const struct dj_tls_context* _ctx
)
{
    const struct dj_pack_text host = { "ftp.example", 11 };
    _k->secured =
        d_tls_session_init(d_tests_ftps_session_of(_k),
                           _ctx,
                           &_k->tcp.base,
                           host) == D_TLS_OK;

    return _k->secured;
}

static
void unsecured(
    struct dj_ftp_connection* _k
)
{
    d_tls_session_destroy(d_tests_ftps_session_of(_k));
    _k->secured = false;

    return;
}

/*
t_adapter_tls
  Checks d_ftps_client_init, d_ftps_client_tls_config,
d_ftp_internal_client_failure, d_ftp_internal_client_read,
d_ftp_internal_client_secure, d_ftp_internal_client_write,
  It runs past 75 lines because one fixture's state runs through every
check, and no cut leaves both halves whole.
*/
static
void t_adapter_tls(void)
{
    struct dj_ftp_options o = { 0 };
    struct dj_tls_config  c = { 0 };
    struct dj_tls_context ctx = { 0 };
    char                 buf[8];  // filled before it is read
    size_t               n = 0;
    enum dj_ftp_io        st = D_FTP_IO_OK;
    d_ftp_options_init(&o);
    o.verify_peer = false;
    o.verify_host = false;
    d_ftps_client_tls_config(&o,
                             &c);
    if (!d_tls_engine_openssl() ||
        d_tls_context_init(&ctx,
                           d_tls_engine_openssl(),
                           &c) != D_TLS_OK)
    {
        (void)d_test_suite_skip(g_suite,
                                "no TLS engine for the adapter's TLS half");

        return;
    }

    d_ftps_client_init(&g_fc,
                       &o,
                       &ctx);
    strcpy(g_fc.ftp.host,
           "ftp.example");
    // securing: a handshake whose transport times out leaves no session and
    // says why; one whose peer hangs up is TLS's failure
    d_tests_ftp_fake_on(&g_fc.ftp.control,
                        NULL,
                        D_NET_ERROR_TIMED_OUT,
                        0,
                        D_NET_OK);
    D_TESTS_FTPS_CHECK( (d_ftp_internal_client_secure(&g_fc.ftp,
                                                       &g_fc.ftp.control) ==
                         D_FTP_ERROR_TIMED_OUT)     &&
                        (!g_fc.ftp.control.secured) &&
                        (g_ftp_fake.out_len > 0) );
    D_TESTS_FTPS_CHECK( (fc_tls() == D_TLS_ERROR_TRANSPORT)           &&
                        (g_fc.ftp.net_error == D_NET_ERROR_TIMED_OUT) &&
                        (g_fc.ftp.control.failure == D_NET_ERROR_TIMED_OUT) );
    d_tests_ftp_fake_on(&g_fc.ftp.control,
                        NULL,
                        D_NET_OK,
                        0,
                        D_NET_OK);
    D_TESTS_FTPS_CHECK( (d_ftp_internal_client_secure(&g_fc.ftp,
                                                       &g_fc.ftp.control) ==
                         D_FTP_ERROR_TLS)                   &&
                        (!g_fc.ftp.control.secured)         &&
                        (fc_tls() == D_TLS_ERROR_TRUNCATED) &&
                        (g_fc.ftp.net_error == D_NET_OK) );
    // in force: a read's transport error, then a write's, reach the judgment
    d_tests_ftp_fake_on(&g_fc.ftp.control,
                        NULL,
                        D_NET_ERROR_TIMED_OUT,
                        0,
                        D_NET_OK);
    D_TESTS_FTPS_CHECK(secured(&g_fc.ftp.control,
                               &ctx));
    st = d_ftp_internal_client_read(&g_fc.ftp,
                                    &g_fc.ftp.control,
                                    buf,
                                    sizeof buf,
                                    &n);
    D_TESTS_FTPS_CHECK( (st == D_FTP_IO_TRANSPORT) &&
                        (d_ftp_internal_client_failure(&g_fc.ftp,
                                                       &g_fc.ftp.control,
                                                       st,
                                                       D_FTP_ERROR_CLOSED) ==
                         D_FTP_ERROR_TIMED_OUT)    &&
                        (g_fc.ftp.net_error == D_NET_ERROR_TIMED_OUT) );
    unsecured(&g_fc.ftp.control);
    d_tests_ftp_fake_on(&g_fc.ftp.control,
                        NULL,
                        D_NET_OK,
                        0,
                        D_NET_ERROR_CONNECTION_RESET);
    D_TESTS_FTPS_CHECK(secured(&g_fc.ftp.control,
                               &ctx));
    st = d_ftp_internal_client_write(&g_fc.ftp,
                                     &g_fc.ftp.control,
                                     "NOOP\r\n",
                                     6);
    D_TESTS_FTPS_CHECK( (st == D_FTP_IO_TRUNCATED) &&
                        (d_ftp_internal_client_failure(&g_fc.ftp,
                                                       &g_fc.ftp.control,
                                                       st,
                                                       D_FTP_ERROR_CLOSED) ==
                         D_FTP_ERROR_CLOSED)       &&
                        (g_fc.ftp.net_error == D_NET_ERROR_CONNECTION_RESET) );
    unsecured(&g_fc.ftp.control);
    d_tls_context_destroy(&ctx);

    return;
}

/*
d_tests_ftps
  The suite's section: opens module ftps, then block adapter's one test,
then the blocks loopback and interop, which open their own. True if every
check of the three passed.
*/
bool d_tests_ftps(
    struct dj_test_suite* _suite
)
{
    const size_t before   = g_failed;
    bool         loopback = true;
    bool         interop  = true;

    g_suite = _suite;
    (void)d_test_suite_module(_suite, "ftps");
    (void)d_test_suite_describe(_suite,
                                "FTPS: the TLS layer over the FTP client, "
                                "over a scripted transport, over loopback "
                                "against a threaded server, and against a "
                                "real server where one is named");
    (void)d_test_suite_block(_suite, "adapter");
    (void)d_test_suite_test(_suite, "TLS adapter");
    t_adapter_tls();

    loopback = d_tests_ftps_loopback(_suite);
    interop  = d_tests_ftps_interop(_suite);

    return (g_failed == before) && loopback && interop;
}
