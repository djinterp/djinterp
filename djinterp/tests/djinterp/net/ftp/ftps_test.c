/*******************************************************************************
* djinterp [net]                                                     ftps_test.c
*
* FTPS tests: the net/ftp/ client over real loopback sockets, secured through
* the SSL kernel with a deterministic test engine, against a threaded server
* that the same kernel secures in the server role. Both live in
* ftps_tests_support.c, which the C++ suite shares.
*   The engine ("toy") frames every message as a TLS record, so the kernel's
* sniffing, pumping, and identity checks all run for real; its "encryption"
* is the identity. Its client contexts cache one session per host and offer
* it again, which is how a real engine resumes the control session on FTPS
* data connections.
*
*
* path:      /tests/djinterp/net/ftp/ftps_test.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "./ftps_tests_support.h"  // the suite: the server, the toy engine
// std
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf
#include <string.h>   // memcpy, memcmp
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp.h"  // d_ftp_client_*
#include "../../../../inc/djinterp/net/ssl/ssl.h"  // d_ssl_session_info


static int g_fail   = 0;
static int g_checks = 0;

/*
d_tests_ftps_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_ftps_check(
    bool        _ok,
    const char* _text
)
{
    g_checks++;

    // a failed check says which
    if (!_ok)
    {
        g_fail++;
        printf("FAIL: %s\n",
               _text);
    }

    return;
}

// D_TESTS_FTPS_CHECK
//   macro: one check, its condition's text naming it when it fails.
#define D_TESTS_FTPS_CHECK(c) d_tests_ftps_check((c), #c)

// ---------------------------------------------------------------------------
// the scenarios
// ---------------------------------------------------------------------------
static struct
{
    char   data[300000];
    size_t len;
} g_sink;
static struct d_ftp_client  g_client;
static struct d_ssl_context g_ctx;

static enum d_ftp_error
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

// d_tests_ftps_source
//   struct: an upload's bytes, and how far the client has read them.
struct d_tests_ftps_source
{
    const char* data;
    size_t      len, pos;
};

static enum d_ftp_error
source_mem(
    void*   _c,
    void*   _b,
    size_t  _cap,
    size_t* _out
)
{
    struct d_tests_ftps_source* s = _c;
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
    struct d_ftp_options* _o,
    enum d_ftp_security   _security
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

static int
setup(
    struct d_tests_ftps_server*                 _s,
    struct d_tests_ftps_server_config              _cfg,
    const struct d_ftp_options* _o
)
{
    struct d_ssl_config c = { 0 };
    d_ftp_client_tls_config(_o,
                            &c);
    enum d_ssl_error st = d_ssl_context_init(&g_ctx,
                                             &TOY,
                                             &c);
    if (st != D_SSL_OK)
    {
        printf("client context: %s\n",
               d_ssl_error_name(st));

        return 0;
    }

    if (!d_tests_ftps_server_start(_s,
                                   _cfg))
    {
        d_ssl_context_destroy(&g_ctx);

        return 0;
    }

    d_ftp_client_init(&g_client,
                      _o,
                      &g_ctx);

    return 1;
}

static void
teardown(
    struct d_tests_ftps_server* _s
)
{
    d_ftp_client_close(&g_client);
    d_tests_ftps_server_stop(_s);
    d_ssl_context_destroy(&g_ctx);

    return;
}

static int
fetch_hello(
    void
)
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
static void
t_explicit_listing(
    void
)
{
    enum d_ftp_listing_format fmt = D_FTP_LISTING_UNIX;

    g_sink.len = 0;
    D_TESTS_FTPS_CHECK( (d_ftp_client_list(&g_client,
                                           NULL,
                                           sink_mem,
                                           NULL,
                                           &fmt) == D_FTP_OK) &&
                        (fmt == D_FTP_LISTING_MLSX) );
    struct d_pack_text cur   = { g_sink.data, g_sink.len };
    struct d_pack_text line  = { 0 };
    struct d_ftp_entry e     = { 0 };
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

static void
t_explicit(
    void
)
{
    struct d_tests_ftps_server        s    = { 0 };
    struct d_tests_ftps_server_config cfg  = { 0 };
    struct d_ftp_options              o    = { 0 };
    struct d_tests_ftps_source        up   = { "upload body\r\n", 13, 0 };
    struct d_ssl_info                 info = { 0 };
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
    D_TESTS_FTPS_CHECK( (d_ssl_session_info(&g_client.control.tls,
                                            &info))            &&
                        (info.version == D_SSL_VERSION_TLS1_3) &&
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

static void
t_implicit(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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

static void
t_downgrade(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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

static void
t_injection(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
    enum d_ftp_error                  err = D_FTP_OK;
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
                         D_SSL_ERROR_NOT_TLS) );
    D_TESTS_FTPS_CHECK(!g_client.connected);
    teardown(&s);
    D_TESTS_FTPS_CHECK( (s.logins == 0) &&
                        (!s.saw_user_in_clear) );

    return;
}

static void
t_reuse_required(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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

static void
t_truncation(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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
    D_TESTS_FTPS_CHECK( (g_client.tls_status == D_SSL_ERROR_TRUNCATED) &&
                        (g_client.reply.code == 226)                   &&
                        (g_sink.len == 24) );
    D_TESTS_FTPS_CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);

    return;
}

static void
t_pasv_fallback(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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

static void
t_active(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
    struct d_tests_ftps_source        up  = { "active\r\n", 8, 0 };
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

static void
t_ccc(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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

static void
t_verify(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
    struct d_ssl_config               c   = { 0 };
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
                        (g_client.tls_status == D_SSL_ERROR_VERIFY) );
    teardown(&s);
    g_toy.cert_name = "localhost";
    D_TESTS_FTPS_CHECK( (s.logins == 0) &&
                        (!s.saw_user_in_clear) );

    g_toy.chain_verdict =
        D_SSL_VERIFY_FLAG_UNTRUSTED; // a chain that does not validate
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
                        (g_client.tls_status == D_SSL_ERROR_VERIFY) );
    teardown(&s);
    g_toy.chain_verdict = 0;

    d_ssl_config_init(&c,
                      D_SSL_ROLE_CLIENT);
    c.verify = D_SSL_VERIFY_NONE; // a context that checks less than asked
    D_TESTS_FTPS_CHECK(d_ssl_context_init(&g_ctx,
                                          &TOY,
                                          &c) == D_SSL_OK);
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
    d_ssl_context_destroy(&g_ctx);

    return;
}

static void
t_ascii(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
    struct d_tests_ftps_source        up  = { "a\nb\n", 4, 0 };
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

static void
t_plain_and_login(
    void
)
{
    struct d_tests_ftps_server        s   = { 0 };
    struct d_tests_ftps_server_config cfg = { 0 };
    struct d_ftp_options              o   = { 0 };
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

/*
main
  Fills the big file's pattern, then runs each scenario in turn; the exit
status is whether any check failed.
*/
int
main(
    void
)
{
    for (size_t i = 0; i < sizeof g_big; i++)
    {
        g_big[i] = (char)(i * 31u + 7u);
    }

    t_explicit();
    t_implicit();
    t_downgrade();
    t_injection();
    t_reuse_required();
    t_truncation();
    t_pasv_fallback();
    t_active();
    t_ccc();
    t_verify();
    t_ascii();
    t_plain_and_login();
    printf("%d checks, %d failures\n",
           g_checks,
           g_fail);

    return g_fail != 0;
}
