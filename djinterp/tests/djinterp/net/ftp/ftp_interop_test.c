/*******************************************************************************
* djinterp [net]                                              ftp_interop_test.c
*
* FTP interop tests: the net/ftp/ client against a real FTP server, plain
* and over TLS through the SSL kernel's OpenSSL engine.
*   Opt-in, like the SFTP integration: without FTP_TEST_PORT the suite skips.
*     FTP_TEST_PORT           plain FTP and explicit FTPS (AUTH TLS)
*     FTP_TEST_USER           an account that may write in its directory
*     FTP_TEST_PASSWORD
*     FTP_TEST_ADDRESS        where to connect for plain FTP; 127.0.0.1
*     FTP_TEST_CA             the server's certificate, or its CA: enables FTPS
*     FTP_TEST_TLS_HOST       the name the certificate is checked against;
*                             localhost
*     FTP_TEST_IMPLICIT_PORT  optional: implicit FTPS
*     FTP_TEST_REUSE_PORT     optional: a server requiring TLS session reuse
*                             on data connections, as vsftpd does by default
*   Every session uploads 300 KB, checks its SIZE, downloads and compares it,
* finds it in a listing, and deletes it, in passive and in active mode.
*
*
* path:      /tests/djinterp/net/ftp/ftp_interop_test.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.16
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf, snprintf
#include <stdlib.h>   // getenv, atoi, strtoul
#include <string.h>   // memcpy, memcmp, strstr
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp.h"          // d_ftp_client_*
#include "../../../../inc/djinterp/net/ssl/ssl.h"          // d_ssl_context
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"  // the OpenSSL engine


static int g_fail   = 0;
static int g_checks = 0;
/*
d_tests_ftp_interop_check
  Counts one check, and names it when it fails.
*/
static void
d_tests_ftp_interop_check(
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

// D_TESTS_FTP_INTEROP_CHECK
//   macro: one check, its condition's text naming it when it fails.
#define D_TESTS_FTP_INTEROP_CHECK(c) d_tests_ftp_interop_check((c), #c)

static char g_body[300000];
static char g_back[400000];
static char g_listing[65536];

// d_tests_ftp_interop_buffer
//   struct: where a download or a listing lands, and how much of it is used.
struct d_tests_ftp_interop_buffer
{
    char*  data;
    size_t length;
    size_t capacity;
};

// d_tests_ftp_interop_source
//   struct: an upload's bytes, and how far the client has read them.
struct d_tests_ftp_interop_source
{
    const char* data;
    size_t      length;
    size_t      position;
};

static enum d_ftp_error
sink_mem(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_ftp_interop_buffer* m = _context;
    if (m->length + _size > m->capacity)
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(m->data + m->length,
           _data,
           _size);
    m->length += _size;

    return D_FTP_OK;
}

static enum d_ftp_error
source_mem(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out
)
{
    struct d_tests_ftp_interop_source* s = _context;
    size_t      n = s->length - s->position;
    if (n > _capacity)
    {
        n = _capacity;
    }

    memcpy(_buffer,
           s->data + s->position,
           n);
    s->position += n;
    *_out = n;

    return D_FTP_OK;
}

static const char*
env(
    const char* _name,
    const char* _fallback
)
{
    const char* value = getenv(_name);

    return (value && *value) ? value : _fallback;
}

static void
report(
    const char*                _label,
    const struct d_ftp_client* _c,
    enum d_ftp_error           _e
)
{
    printf("  %s: %s (reply %u, tls %s, net %s)\n",
           _label,
           d_ftp_error_string(_e),
           _c->reply.code,
           d_ssl_error_name(_c->tls_status),
           d_net_error_string(_c->net_error));

    return;
}

// one session: connect, login, and a file's whole life
/*
session_file
  A file's whole life over an open session: stored, sized, retrieved and
compared, found in a listing, and deleted; then QUIT, and a line saying how
it went.
*/
static void
session_file(
    struct d_ftp_client* _c,
    const char*          _label,
    const char*          _name,
    int                  _failures
)
{
    struct d_tests_ftp_interop_source up      = { g_body, sizeof g_body, 0 };
    struct d_tests_ftp_interop_buffer back    = { g_back, 0, sizeof g_back };
    struct d_tests_ftp_interop_buffer listing = { g_listing,
                                                  0,
                                                  sizeof g_listing - 1u };
    enum d_ftp_listing_format         format  = D_FTP_LISTING_AUTO;
    enum d_ftp_error                  e       = D_FTP_OK;

    e = d_ftp_client_store(_c,
                           _name,
                           source_mem,
                           &up);
    D_TESTS_FTP_INTEROP_CHECK(e == D_FTP_OK);
    if (e != D_FTP_OK)
    {
        report(_label,
               _c,
               e);
    }

    D_TESTS_FTP_INTEROP_CHECK( (d_ftp_client_command(_c,
                                                     D_FTP_COMMAND_SIZE,
                                                     _name) == D_FTP_OK) &&
                               (_c->reply.code == 213u)                  &&
                               (strtoul(_c->reply.text.data,
                                        NULL,
                                        10) == sizeof g_body) );
    e = d_ftp_client_retrieve(_c,
                              _name,
                              sink_mem,
                              &back);
    D_TESTS_FTP_INTEROP_CHECK( (e == D_FTP_OK)                &&
                               (back.length == sizeof g_body) &&
                               (memcmp(g_back,
                                       g_body,
                                       sizeof g_body) == 0) );
    if (e != D_FTP_OK)
    {
        report(_label,
               _c,
               e);
    }

    e = d_ftp_client_list(_c,
                          NULL,
                          sink_mem,
                          &listing,
                          &format);
    g_listing[listing.length] = '\0';
    D_TESTS_FTP_INTEROP_CHECK( (e == D_FTP_OK) &&
                               (strstr(g_listing,
                                       _name) != NULL) );
    D_TESTS_FTP_INTEROP_CHECK(d_ftp_client_command(_c,
                                                   D_FTP_COMMAND_DELE,
                                                   _name) == D_FTP_OK);
    const bool control_tls = _c->control.secured;
    const bool data_tls    = (_c->protection == D_FTP_PROTECTION_PRIVATE);
    D_TESTS_FTP_INTEROP_CHECK(d_ftp_client_quit(_c) == D_FTP_OK);
    printf("  %s: %s, listed as %s, control %s, data %s\n",
           _label,
           (g_fail == _failures) ? "300000 bytes up and back" : "FAILED",
           (format == D_FTP_LISTING_MLSX) ? "MLSD" : "LIST",
           control_tls ? "TLS" : "plain",
           data_tls ? "TLS" : "plain");

    return;
}

static void
session(
    const char*                 _label,
    const struct d_ftp_options* _o,
    const struct d_ssl_context* _tls,
    const char*                 _host,
    uint16_t                    _port
)
{
    static struct d_ftp_client c = { 0 };
    char                       name[64];  // filled before it is read
    enum d_ftp_error          e = D_FTP_OK;

    const int failures = g_fail;

    snprintf(name,
             sizeof name,
             "interop_%s.bin",
             _label);
    d_ftp_client_init(&c,
                      _o,
                      _tls);
    e = d_ftp_client_connect(&c,
                             _host,
                             _port);
    D_TESTS_FTP_INTEROP_CHECK(e == D_FTP_OK);
    if (e != D_FTP_OK)
    {
        report(_label,
               &c,
               e);
        d_ftp_client_close(&c);

        return;
    }

    e = d_ftp_client_login(&c);
    D_TESTS_FTP_INTEROP_CHECK(e == D_FTP_OK);
    if (e != D_FTP_OK)
    {
        report(_label,
               &c,
               e);
        d_ftp_client_close(&c);

        return;
    }

    D_TESTS_FTP_INTEROP_CHECK((_o->security == D_FTP_SECURITY_NONE) ==
                              !c.control.secured);
    session_file(&c,
                 _label,
                 name,
                 failures);
    d_ftp_client_close(&c);

    return;
}

static void
options(
    struct d_ftp_options* _o,
    enum d_ftp_security   _security,
    enum d_ftp_data_mode  _mode
)
{
    d_ftp_options_init(_o);
    _o->user               = env("FTP_TEST_USER",
                                 "ftpuser");
    _o->password           = env("FTP_TEST_PASSWORD",
                                 "");
    _o->security           = _security;
    _o->data_mode          = _mode;
    _o->connect_timeout_ms = 10000u;
    if (_security != D_FTP_SECURITY_NONE)
    {
        _o->data_protection  = D_FTP_PROTECTION_PRIVATE;
        _o->require_security = true;
    }

    return;
}

// a context trusting the test CA alone, or the system store alone
static int
context(
    struct d_ssl_context*       _ctx,
    const struct d_ftp_options* _o,
    const char*                 _ca
)
{
    struct d_ssl_config config = { 0 };
    d_ftp_client_tls_config(_o,
                            &config);
    config.ca_file          = _ca;
    config.use_system_trust = (_ca == NULL);

    return d_ssl_context_init(_ctx,
                              d_ssl_engine_openssl(),
                              &config) ==
           D_SSL_OK;
}

/*
run_plain
  Plain FTP, passive and active.
*/
static void
run_plain(
    const char* _address,
    uint16_t    _port
)
{
    struct d_ftp_options o = { 0 };

    printf("plain FTP:\n");
    options(&o,
            D_FTP_SECURITY_NONE,
            D_FTP_DATA_PASSIVE_AUTO);
    session("plain_passive",
            &o,
            NULL,
            _address,
            _port);
    options(&o,
            D_FTP_SECURITY_NONE,
            D_FTP_DATA_ACTIVE_AUTO);
    session("plain_active",
            &o,
            NULL,
            _address,
            _port);

    return;
}

/*
run_ftps
  Explicit FTPS, passive and active, over a context trusting `_ca`; then
implicit FTPS, where the server offers it on `_implicit`.
*/
static void
run_ftps(
    const char* _ca,
    const char* _tls_host,
    uint16_t    _port,
    uint16_t    _implicit
)
{
    struct d_ftp_options o   = { 0 };
    struct d_ssl_context ctx = { 0 };

    printf("explicit FTPS (AUTH TLS, PROT P), OpenSSL engine:\n");
    options(&o,
            D_FTP_SECURITY_EXPLICIT,
            D_FTP_DATA_PASSIVE_AUTO);
    D_TESTS_FTP_INTEROP_CHECK(context(&ctx,
                                      &o,
                                      _ca));
    session("explicit_passive",
            &o,
            &ctx,
            _tls_host,
            _port);
    o.data_mode = D_FTP_DATA_ACTIVE_AUTO;
    session("explicit_active",
            &o,
            &ctx,
            _tls_host,
            _port);
    d_ssl_context_destroy(&ctx);

    if (_implicit)
    {
        printf("implicit FTPS:\n");
        options(&o,
                D_FTP_SECURITY_IMPLICIT,
                D_FTP_DATA_PASSIVE_AUTO);
        D_TESTS_FTP_INTEROP_CHECK(context(&ctx,
                                          &o,
                                          _ca));
        session("implicit_passive",
                &o,
                &ctx,
                _tls_host,
                _implicit);
        d_ssl_context_destroy(&ctx);
    }

    return;
}

/*
run_untrusted
  A certificate the context does not trust ends the session in TLS.
*/
static void
run_untrusted(
    const char* _tls_host,
    uint16_t    _port
)
{
    struct d_ftp_options o   = { 0 };
    struct d_ssl_context ctx = { 0 };
    static struct d_ftp_client c = { 0 };
    options(&o,
            D_FTP_SECURITY_EXPLICIT,
            D_FTP_DATA_PASSIVE_AUTO);
    D_TESTS_FTP_INTEROP_CHECK(context(&ctx,
                                      &o,
                                      NULL));
    d_ftp_client_init(&c,
                      &o,
                      &ctx);
    const enum d_ftp_error e = d_ftp_client_connect(&c,
                                                    _tls_host,
                                                    _port);
    D_TESTS_FTP_INTEROP_CHECK( (e == D_FTP_ERROR_TLS) &&
                               (!c.connected) );
    printf("untrusted certificate:\n");
    report("refused",
           &c,
           e);
    d_ftp_client_close(&c);
    d_ssl_context_destroy(&ctx);

    return;
}

/*
run_reuse
  A server requiring session reuse on data connections, as vsftpd does by
default: the listing must succeed, which it can only by resuming the control
connection's session (d_ssl_session_resume). A failure is still reported
cleanly, and so is the next command after it, to say why.
*/
static void
run_reuse(
    const char* _ca,
    const char* _tls_host,
    uint16_t    _reuse
)
{
    struct d_ftp_options o   = { 0 };
    struct d_ssl_context ctx = { 0 };
    static struct d_ftp_client c = { 0 };
    struct d_tests_ftp_interop_buffer back = { g_back, 0, sizeof g_back };
    options(&o,
            D_FTP_SECURITY_EXPLICIT,
            D_FTP_DATA_PASSIVE_AUTO);
    D_TESTS_FTP_INTEROP_CHECK(context(&ctx,
                                      &o,
                                      _ca));
    d_ftp_client_init(&c,
                      &o,
                      &ctx);
    D_TESTS_FTP_INTEROP_CHECK(d_ftp_client_connect(&c,
                                                   _tls_host,
                                                   _reuse) == D_FTP_OK);
    D_TESTS_FTP_INTEROP_CHECK(d_ftp_client_login(&c) == D_FTP_OK);
    const enum d_ftp_error e =
        d_ftp_client_list(&c,
                          NULL,
                          sink_mem,
                          &back,
                          NULL);
    printf("server requiring session reuse:\n");
    // the server refuses a data connection that does not resume
    D_TESTS_FTP_INTEROP_CHECK(e == D_FTP_OK);

    if (e == D_FTP_OK)
    {
        printf("  listing: resumed, %zu bytes\n",
               back.length);
    }
    else
    {
        // the refusal is reported; vsftpd then ends the session, which
        // the next command must report cleanly as a closed connection
        report("listing",
               &c,
               e);
        const enum d_ftp_error next =
            d_ftp_client_command(&c,
                                 D_FTP_COMMAND_NOOP,
                                 NULL);
        D_TESTS_FTP_INTEROP_CHECK( (next == D_FTP_OK) ||
                                   (next == D_FTP_ERROR_CLOSED) );
        report("next command",
               &c,
               next);
    }

    d_ftp_client_close(&c);
    d_ssl_context_destroy(&ctx);

    return;
}

/*
main
  Reads the server from the environment, skipping without one; runs plain
FTP, then FTPS where a CA is named and the OpenSSL engine is built.
*/
int
main(
    void
)
{
    const char* port_text = getenv("FTP_TEST_PORT");
    if (!port_text)
    {
        printf("interop: skipped (FTP_TEST_* not set)\n");

        return 0;
    }

    const uint16_t port = (uint16_t)atoi(port_text);
    const uint16_t implicit =
        (uint16_t)atoi(env("FTP_TEST_IMPLICIT_PORT",
                           "0"));
    const uint16_t reuse    = (uint16_t)atoi(env("FTP_TEST_REUSE_PORT",
                                                 "0"));
    const char*    address  = env("FTP_TEST_ADDRESS",
                                  "127.0.0.1");
    const char*    tls_host = env("FTP_TEST_TLS_HOST",
                                  "localhost");
    const char*    ca       = getenv("FTP_TEST_CA");

    for (size_t i = 0; i < sizeof g_body; i++)
    {
        g_body[i] = (char)((i * 2654435761u) >> 13);
    }

    run_plain(address,
              port);

    if (!ca)
    {
        printf("FTPS: skipped (FTP_TEST_CA not set)\n");
    }
    else if (!d_ssl_engine_openssl())
    {
        printf("FTPS: skipped (the OpenSSL engine is not built)\n");
    }
    else
    {
        run_ftps(ca,
                 tls_host,
                 port,
                 implicit);
        run_untrusted(tls_host,
                      port);

        // a server requiring session reuse, where the build names one
        if (reuse)
        {
            run_reuse(ca,
                      tls_host,
                      reuse);
        }
    }

    printf("%d checks, %d failures\n",
           g_checks,
           g_fail);

    return g_fail != 0;
}
