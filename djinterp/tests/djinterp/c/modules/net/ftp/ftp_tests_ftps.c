/*******************************************************************************
* djinterp [c][net]                                             ftp_tests_ftps.c
*
* FTPS tests: the net/ftp/ client over real loopback sockets, secured through
* the TLS kernel with a deterministic test engine, against a threaded server
* that the same kernel secures in the server role. Both live in
* ftps_tests_support.c, which the C++ suite shares.
*   The engine ("toy") frames every message as a TLS record, so the kernel's
* sniffing, pumping, and identity checks all run for real; its "encryption"
* is the identity. Its client contexts cache one session per host and offer
* it again, which is how a real engine resumes the control session on FTPS
* data connections.
*   Block ftps of the suite's module ftp: d_tests_ftp_ftps runs each
* scenario as a test of its own.
*
*
* path:      /tests/djinterp/c/modules/net/ftp/ftp_tests_ftps.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "./ftp_tests.h"           // the suite
#include "./ftps_tests_support.h"  // the server, the toy engine
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t
#include <string.h>   // memcpy, memcmp
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp.h"  // d_ftp_client_*
#include "../../../../../../inc/djinterp/c/modules/net/tls/tls.h"  // d_tls_session_info


// g_suite
//   variable: the suite d_tests_ftp_ftps records into; the tests' checks reach it
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

// ---------------------------------------------------------------------------
// the scenarios
// ---------------------------------------------------------------------------
static struct
{
    char   data[300000];
    size_t len;
} g_sink;
static struct dj_ftp_client  g_client;
static struct dj_tls_context g_ctx;

static enum dj_ftp_error
sink_mem(
    void*       _c,
    const void* _d,
    size_t      _n
)
{
    (void)_c;
    if (g_sink.len + _n > sizeof g_sink.data)
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(g_sink.data + g_sink.len,
           _d,
           _n);
    g_sink.len += _n;

    return D_FTP_OK;
}

// dj_tests_ftps_source
//   struct: an upload's bytes, and how far the client has read them.
struct dj_tests_ftps_source
{
    const char* data;
    size_t      len, pos;
};

static enum dj_ftp_error
source_mem(
    void*   _c,
    void*   _b,
    size_t  _cap,
    size_t* _out
)
{
    struct dj_tests_ftps_source* s = _c;
    size_t                      n = s->len - s->pos;
    if (n > _cap)
    {
        n = _cap;
    }

    memcpy(_b,
           s->data + s->pos,
           n);
    s->pos += n;
    *_out = n;

    return D_FTP_OK;
}

static void
options(
    struct dj_ftp_options* _o,
    enum dj_ftp_security   _security
)
{
    d_ftp_options_init(_o);
    _o->security            = _security;
    _o->user                = "alice";
    _o->password            = "secret";
    _o->connect_timeout_ms  = 5000;
    _o->response_timeout_ms = 5000;
    _o->idle_timeout_ms     = 5000;

    return;
}

static
int setup(
    struct dj_tests_ftps_server*       _s,
    struct dj_tests_ftps_server_config _cfg,
    const struct dj_ftp_options*       _o
)
{
    struct dj_tls_config c = { 0 };
    d_ftp_client_tls_config(_o,
                            &c);
    enum dj_tls_error st = d_tls_context_init(&g_ctx,
                                             &TOY,
                                             &c);
    if (st != D_TLS_OK)
    {
        (void)d_test_suite_note(g_suite,
                                "client context: %s",
                                d_tls_error_name(st));

        return 0;
    }

    if (!d_tests_ftps_server_start(_s,
                                   _cfg))
    {
        d_tls_context_destroy(&g_ctx);

        return 0;
    }

    d_ftp_client_init(&g_client,
                      _o,
                      &g_ctx);

    return 1;
}

static
void teardown(
    struct dj_tests_ftps_server* _s
)
{
    d_ftp_client_close(&g_client);
    d_tests_ftps_server_stop(_s);
    d_tls_context_destroy(&g_ctx);

    return;
}

static
int fetch_hello(void)
{
    g_sink.len = 0;

    return d_ftp_client_retrieve(&g_client,
                                 "hello.txt",
                                 sink_mem,
                                 NULL) ==
               D_FTP_OK &&
           g_sink.len == 24 && !memcmp(g_sink.data,
                                       HELLO,
                                       24);
}

/*
t_explicit_listing
  t_explicit's second half, over its secured session: an MLSD listing with
one file and one directory, then SIZE, and the refusals of a transfer or a
CCC sent as a plain command.
*/
static
void t_explicit_listing(void)
{
    enum dj_ftp_listing_format fmt = D_FTP_LISTING_UNIX;

    g_sink.len = 0;
    D_TESTS_FTPS_CHECK( (d_ftp_client_list(&g_client,
                                           NULL,
                                           sink_mem,
                                           NULL,
                                           &fmt) == D_FTP_OK) &&
                        (fmt == D_FTP_LISTING_MLSX) );
    struct dj_pack_text cur   = { g_sink.data, g_sink.len };
    struct dj_pack_text line  = { 0 };
    struct dj_ftp_entry e     = { 0 };
    int                files = 0;
    int                dirs  = 0;
    while (d_net_line_next(&cur,
                           &line))
    {
        if (d_ftp_listing_parse(line.data,
                                line.length,
                                fmt,
                                NULL,
                                &e) ==
            D_FTP_LINE_ENTRY)
        {
            files += e.type == D_FTP_ENTRY_FILE;
            dirs += e.type == D_FTP_ENTRY_DIRECTORY;
        }
    }

    D_TESTS_FTPS_CHECK( (files == 1) &&
                        (dirs == 1) );
    D_TESTS_FTPS_CHECK( (d_ftp_client_command(&g_client,
                                              D_FTP_COMMAND_SIZE,
                                              "hello.txt") == D_FTP_OK) &&
                        (g_client.reply.code == 213) );
    D_TESTS_FTPS_CHECK(
        d_ftp_client_command(&g_client,
                             D_FTP_COMMAND_RETR,
                             "x") ==
        D_FTP_ERROR_INVALID_ARGUMENT);
    D_TESTS_FTPS_CHECK(
        d_ftp_client_command(&g_client,
                             D_FTP_COMMAND_CCC,
                             NULL) ==
        D_FTP_ERROR_INVALID_ARGUMENT);

    return;
}

static
void t_explicit(void)
{
    struct dj_tests_ftps_server        s    = { 0 };
    struct dj_tests_ftps_server_config cfg  = { 0 };
    struct dj_ftp_options              o    = { 0 };
    struct dj_tests_ftps_source        up   = { "upload body\r\n", 13, 0 };
    struct dj_tls_info                 info = { 0 };
    cfg.require_reuse               = 1;
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (g_client.control.secured) );
    D_TESTS_FTPS_CHECK(
        (g_client.features &
         (D_FTP_FEATURE_EPSV | D_FTP_FEATURE_MLST | D_FTP_FEATURE_UTF8)) ==
        (D_FTP_FEATURE_EPSV | D_FTP_FEATURE_MLST | D_FTP_FEATURE_UTF8));
    D_TESTS_FTPS_CHECK( (d_tls_session_info(&g_client.control.tls,
                                            &info))            &&
                        (info.version == D_TLS_VERSION_TLS1_3) &&
                        (!info.resumed)                        &&
                        (info.has_peer_certificate) );
    D_TESTS_FTPS_CHECK( (d_ftp_client_login(&g_client) == D_FTP_OK) &&
                        (g_client.protection == D_FTP_PROTECTION_PRIVATE) );
    D_TESTS_FTPS_CHECK(fetch_hello());
    g_sink.len = 0;
    D_TESTS_FTPS_CHECK( (d_ftp_client_retrieve(&g_client,
                                               "big.bin",
                                               sink_mem,
                                               NULL) == D_FTP_OK) &&
                        (g_sink.len == sizeof g_big)              &&
                        (!memcmp(g_sink.data,
                                 g_big,
                                 sizeof g_big)) );
    D_TESTS_FTPS_CHECK(
        d_ftp_client_store(&g_client,
                           "up.txt",
                           source_mem,
                           &up) == D_FTP_OK);
    t_explicit_listing();
    D_TESTS_FTPS_CHECK( (d_ftp_client_quit(&g_client) == D_FTP_OK) &&
                        (!g_client.connected)                      &&
                        (!g_client.control.secured) );
    teardown(&s);
    D_TESTS_FTPS_CHECK( (!s.saw_user_in_clear) &&
                        (s.logins == 1)        &&
                        (s.type == 'I') );
    D_TESTS_FTPS_CHECK( (s.data_tls == 4)     &&
                        (s.data_resumed == 4) &&
                        (s.data_fresh == 0) );
    D_TESTS_FTPS_CHECK( (s.upload_clean)     &&
                        (s.stored_len == 13) &&
                        (!memcmp(s.stored,
                                 "upload body\r\n",
                                 13)) );
    D_TESTS_FTPS_CHECK(
        s.upload_waited); // the client waited for the server's close_notify

    return;
}

static
void t_implicit(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    cfg.implicit             = 1;
    options(&o,
            D_FTP_SECURITY_IMPLICIT);
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (g_client.control.secured) );
    D_TESTS_FTPS_CHECK( (d_ftp_client_login(&g_client) == D_FTP_OK) &&
                        (fetch_hello()) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (!s.saw_user_in_clear) &&
                        (s.data_tls == 1)      &&
                        (s.data_resumed == 1) );

    return;
}

static
void t_downgrade(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    cfg.refuse_auth          = 1;
    options(&o,
            D_FTP_SECURITY_EXPLICIT); // require_security by default
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) ==
                         D_FTP_ERROR_SECURITY) &&
                        (!g_client.connected) );
    teardown(&s);
    D_TESTS_FTPS_CHECK( (!s.saw_user_in_clear) &&
                        (s.logins == 0) );

    o.require_security = false; // the caller accepts plaintext
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (!g_client.control.secured) );
    D_TESTS_FTPS_CHECK( (d_ftp_client_login(&g_client) == D_FTP_OK)     &&
                        (g_client.protection == D_FTP_PROTECTION_CLEAR) &&
                        (fetch_hello()) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.saw_user_in_clear) &&
                        (s.data_plain == 1)   &&
                        (s.data_tls == 0) );

    return;
}

static
void t_injection(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    enum dj_ftp_error                  err = D_FTP_OK;
    cfg.inject               = 1;
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    err = d_ftp_client_connect(&g_client,
                               "localhost",
                               s.port);
    D_TESTS_FTPS_CHECK( (err == D_FTP_ERROR_SECURITY) ||
                        (err == D_FTP_ERROR_TLS && g_client.tls_status ==
                         D_TLS_ERROR_NOT_TLS) );
    D_TESTS_FTPS_CHECK(!g_client.connected);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.logins == 0) &&
                        (!s.saw_user_in_clear) );

    return;
}

static
void t_reuse_required(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    cfg.require_reuse        = 1;
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    g_toy.client_cache = 0; // an engine without resumption
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);
        g_toy.client_cache = 1;

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (d_ftp_client_login(&g_client) == D_FTP_OK) );
    g_sink.len = 0;
    D_TESTS_FTPS_CHECK( (d_ftp_client_retrieve(&g_client,
                                               "hello.txt",
                                               sink_mem,
                                               NULL) == D_FTP_ERROR_TLS) &&
                        (g_client.reply.code == 522) );
    D_TESTS_FTPS_CHECK( (d_ftp_client_command(&g_client,
                                              D_FTP_COMMAND_SIZE,
                                              "hello.txt") == D_FTP_OK) &&
                        (g_client.reply.code == 213) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    g_toy.client_cache = 1;
    D_TESTS_FTPS_CHECK( (s.data_fresh == 1) &&
                        (s.data_tls == 0) );

    return;
}

static
void t_truncation(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    cfg.truncate             = 1;
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (d_ftp_client_login(&g_client) == D_FTP_OK) );
    g_sink.len = 0;
    D_TESTS_FTPS_CHECK(
        d_ftp_client_retrieve(&g_client,
                              "hello.txt",
                              sink_mem,
                              NULL) ==
        D_FTP_ERROR_TLS);
    D_TESTS_FTPS_CHECK( (g_client.tls_status == D_TLS_ERROR_TRUNCATED) &&
                        (g_client.reply.code == 226)                   &&
                        (g_sink.len == 24) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);

    return;
}

static
void t_pasv_fallback(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    cfg.refuse_epsv          = 1;
    cfg.bogus_pasv           = 1;
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (d_ftp_client_login(&g_client) == D_FTP_OK) );
    D_TESTS_FTPS_CHECK( (fetch_hello()) &&
                        (fetch_hello()) &&
                        (g_client.extended_refused) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.epsv_count == 1) &&
                        (s.data_tls == 2) );

    return;
}

static
void t_active(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    struct dj_tests_ftps_source        up  = { "active\r\n", 8, 0 };
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    o.data_mode = D_FTP_DATA_ACTIVE_AUTO;
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (d_ftp_client_login(&g_client) == D_FTP_OK) );
    D_TESTS_FTPS_CHECK( (fetch_hello()) &&
                        (d_ftp_client_store(&g_client,
                                            "a.txt",
                                            source_mem,
                                            &up) == D_FTP_OK) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.data_tls == 2)     &&
                        (s.data_resumed == 2) &&
                        (s.stored_len == 8)   &&
                        (s.upload_clean) );

    return;
}

static
void t_ccc(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    o.clear_control = true;
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (d_ftp_client_login(&g_client) == D_FTP_OK) );
    D_TESTS_FTPS_CHECK( (!g_client.control.secured)                       &&
                        (g_client.protection == D_FTP_PROTECTION_PRIVATE) &&
                        (fetch_hello()) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.ccc_done) &&
                        (s.data_tls == 1) );

    return;
}

static
void t_verify(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    struct dj_tls_config               c   = { 0 };
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    g_toy.cert_name = "evil.example"; // a certificate for another host
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);
        g_toy.cert_name = "localhost";

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_ERROR_TLS) &&
                        (g_client.tls_status == D_TLS_ERROR_VERIFY) );
    teardown(&s);
    g_toy.cert_name = "localhost";
    D_TESTS_FTPS_CHECK( (s.logins == 0) &&
                        (!s.saw_user_in_clear) );

    g_toy.chain_verdict =
        D_TLS_VERIFY_FLAG_UNTRUSTED; // a chain that does not validate
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);
        g_toy.chain_verdict = 0;

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_ERROR_TLS) &&
                        (g_client.tls_status == D_TLS_ERROR_VERIFY) );
    teardown(&s);
    g_toy.chain_verdict = 0;

    d_tls_config_init(&c,
                      D_TLS_ROLE_CLIENT);
    c.verify = D_TLS_VERIFY_NONE; // a context that checks less than asked
    D_TESTS_FTPS_CHECK(d_tls_context_init(&g_ctx,
                                          &TOY,
                                          &c) == D_TLS_OK);
    d_ftp_client_init(&g_client,
                      &o,
                      &g_ctx);
    D_TESTS_FTPS_CHECK(d_ftp_client_connect(&g_client,
                                            "localhost",
                                            1) ==
                       D_FTP_ERROR_INVALID_ARGUMENT);
    d_ftp_client_init(&g_client,
                      &o,
                      NULL);
    D_TESTS_FTPS_CHECK(d_ftp_client_connect(&g_client,
                                            "localhost",
                                            1) ==
                       D_FTP_ERROR_INVALID_ARGUMENT);
    d_tls_context_destroy(&g_ctx);

    return;
}

static
void t_ascii(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    struct dj_tests_ftps_source        up  = { "a\nb\n", 4, 0 };
    options(&o,
            D_FTP_SECURITY_EXPLICIT);
    o.type.data_type = D_FTP_TYPE_ASCII;
    o.type.format    = D_FTP_FORMAT_NONE;
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "localhost",
                                              s.port) == D_FTP_OK) &&
                        (d_ftp_client_login(&g_client) == D_FTP_OK) );
    g_sink.len = 0;
    D_TESTS_FTPS_CHECK( (d_ftp_client_retrieve(&g_client,
                                               "hello.txt",
                                               sink_mem,
                                               NULL) == D_FTP_OK) &&
                        (g_sink.len == 22)                        &&
                        (!memcmp(g_sink.data,
                                 "Hello, FTPS!\nLine two\n",
                                 22)) );
    D_TESTS_FTPS_CHECK(
        d_ftp_client_store(&g_client,
                           "a.txt",
                           source_mem,
                           &up) == D_FTP_OK);
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.type == 'A')     &&
                        (s.stored_len == 6) &&
                        (!memcmp(s.stored,
                                 "a\r\nb\r\n",
                                 6)) );

    return;
}

static
void t_plain_and_login(void)
{
    struct dj_tests_ftps_server        s   = { 0 };
    struct dj_tests_ftps_server_config cfg = { 0 };
    struct dj_ftp_options              o   = { 0 };
    options(&o,
            D_FTP_SECURITY_NONE);
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK( (d_ftp_client_connect(&g_client,
                                              "127.0.0.1",
                                              s.port) == D_FTP_OK) &&
                        (!g_client.control.secured) );
    D_TESTS_FTPS_CHECK(
        d_ftp_client_retrieve(&g_client,
                              "hello.txt",
                              sink_mem,
                              NULL) ==
        D_FTP_ERROR_BAD_SEQUENCE);
    D_TESTS_FTPS_CHECK( (d_ftp_client_login(&g_client) == D_FTP_OK) &&
                        (fetch_hello()) );
    D_TESTS_FTPS_CHECK(d_ftp_client_login(&g_client) ==
                       D_FTP_ERROR_BAD_SEQUENCE);
    D_TESTS_FTPS_CHECK( (d_ftp_client_quit(&g_client) == D_FTP_OK) &&
                        (d_ftp_client_quit(&g_client) ==
                         D_FTP_ERROR_BAD_SEQUENCE) );
    teardown(&s);
    D_TESTS_FTPS_CHECK(s.data_plain == 1);

    o.password = "wrong";
    if (!setup(&s,
               cfg,
               &o))
    {
        D_TESTS_FTPS_CHECK(0);

        return;
    }

    D_TESTS_FTPS_CHECK(d_ftp_client_connect(&g_client,
                                            "127.0.0.1",
                                            s.port) ==
                       D_FTP_OK);
    D_TESTS_FTPS_CHECK( (d_ftp_client_login(&g_client) ==
                         D_FTP_ERROR_LOGIN_DENIED) &&
                        (!g_client.logged_in)      &&
                        (g_client.reply.code == 530) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);

    return;
}

// dj_tests_ftps_case
//   struct: a row of D_TESTS_FTPS_CASES: a test's name, and the function that
// makes its checks.
struct dj_tests_ftps_case
{
    const char* name;
    void        (*run)(void);
};

// D_TESTS_FTPS_CASES
//   constant: block ftps's tests, in the order run.
static const struct dj_tests_ftps_case D_TESTS_FTPS_CASES[] =
{
    { "explicit FTPS",   t_explicit },
    { "implicit FTPS",   t_implicit },
    { "downgrade",       t_downgrade },
    { "injection",       t_injection },
    { "reuse required",  t_reuse_required },
    { "truncation",      t_truncation },
    { "PASV fallback",   t_pasv_fallback },
    { "active mode",     t_active },
    { "CCC",             t_ccc },
    { "verification",    t_verify },
    { "ASCII",           t_ascii },
    { "plain and login", t_plain_and_login }
};

/*
d_tests_ftp_ftps
  Block ftps: fills the big file's pattern, then runs each scenario as a test
of its own. True if every check passed.
*/
bool d_tests_ftp_ftps(
    struct dj_test_suite* _suite
)
{
    const size_t COUNT  = sizeof(D_TESTS_FTPS_CASES) /
                          sizeof(D_TESTS_FTPS_CASES[0]);
    const size_t before = g_failed;

    g_suite = _suite;
    (void)d_test_suite_block(_suite, "ftps");
    (void)d_test_suite_describe(_suite,
                                "the client over loopback, secured by the "
                                "toy engine, against a threaded server");

    for (size_t i = 0; i < sizeof g_big; i++)
    {
        g_big[i] = (char)(i * 31u + 7u);
    }

    // each scenario in order, a test of its own
    for (size_t i = 0u; i < COUNT; ++i)
    {
        (void)d_test_suite_test(_suite,
                                D_TESTS_FTPS_CASES[i].name);
        D_TESTS_FTPS_CASES[i].run();
    }

    return (g_failed == before);
}
