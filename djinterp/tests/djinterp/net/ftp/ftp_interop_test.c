// FTP interop tests: the net/ftp/ client against a real FTP server, plain
// and over TLS through the SSL kernel's OpenSSL engine.
//
// Opt-in, like the SFTP integration: without FTP_TEST_PORT the suite skips.
//   FTP_TEST_PORT           plain FTP and explicit FTPS (AUTH TLS)
//   FTP_TEST_USER           an account that may write in its directory
//   FTP_TEST_PASSWORD
//   FTP_TEST_ADDRESS        where to connect for plain FTP; 127.0.0.1
//   FTP_TEST_CA             the server's certificate, or its CA: enables FTPS
//   FTP_TEST_TLS_HOST       the name the certificate is checked against;
//                           localhost
//   FTP_TEST_IMPLICIT_PORT  optional: implicit FTPS
//   FTP_TEST_REUSE_PORT     optional: a server requiring TLS session reuse on
//                           data connections, as vsftpd does by default
//
// Every session uploads 300 KB, checks its SIZE, downloads and compares it,
// finds it in a listing, and deletes it, in passive and in active mode.
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../inc/djinterp/net/ftp/ftp.h"
#include "../../../../inc/djinterp/net/ssl/ssl.h"
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"

static int g_fail   = 0;
static int g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)

static char g_body[300000];
static char g_back[400000];
static char g_listing[65536];

struct mem { char* data; size_t length, capacity; };
struct src { const char* data; size_t length, position; };

static enum d_ftp_error sink_mem(void* _context, const void* _data, size_t _size)
{
    struct mem* m = _context;
    if (m->length + _size > m->capacity) return D_FTP_ERROR_BUFFER_TOO_SMALL;
    memcpy(m->data + m->length, _data, _size);
    m->length += _size;
    return D_FTP_OK;
}

static enum d_ftp_error source_mem(void* _context, void* _buffer, size_t _capacity, size_t* _out)
{
    struct src* s = _context;
    size_t      n = s->length - s->position;
    if (n > _capacity) n = _capacity;
    memcpy(_buffer, s->data + s->position, n);
    s->position += n;
    *_out = n;
    return D_FTP_OK;
}

static const char* env(const char* _name, const char* _fallback)
{
    const char* value = getenv(_name);
    return (value && *value) ? value : _fallback;
}

static void report(const char* _label, const struct d_ftp_client* _c, enum d_ftp_error _e)
{
    printf("  %s: %s (reply %u, tls %s, net %s)\n", _label, d_ftp_error_string(_e), _c->reply.code,
           d_ssl_status_name(_c->tls_status), d_net_error_string(_c->net_error));
}

// one session: connect, login, and a file's whole life
static void session(const char* _label, const struct d_ftp_options* _o,
                    const struct d_ssl_context* _tls, const char* _host, uint16_t _port)
{
    static struct d_ftp_client c;
    char                  name[64];
    struct src            up      = { g_body, sizeof g_body, 0 };
    struct mem            back    = { g_back, 0, sizeof g_back };
    struct mem            listing = { g_listing, 0, sizeof g_listing - 1u };
    enum d_ftp_listing_format format = D_FTP_LISTING_AUTO;
    enum d_ftp_error      e;

    const int             failures = g_fail;

    snprintf(name, sizeof name, "interop_%s.bin", _label);
    d_ftp_client_init(&c, _o, _tls);
    e = d_ftp_client_connect(&c, _host, _port);
    CHECK(e == D_FTP_OK);
    if (e != D_FTP_OK) { report(_label, &c, e); d_ftp_client_close(&c); return; }
    e = d_ftp_client_login(&c);
    CHECK(e == D_FTP_OK);
    if (e != D_FTP_OK) { report(_label, &c, e); d_ftp_client_close(&c); return; }
    CHECK((_o->security == D_FTP_SECURITY_NONE) == !c.control.secured);

    e = d_ftp_client_store(&c, name, source_mem, &up);
    CHECK(e == D_FTP_OK);
    if (e != D_FTP_OK) report(_label, &c, e);
    CHECK(d_ftp_client_command(&c, D_FTP_COMMAND_SIZE, name) == D_FTP_OK &&
          c.reply.code == 213u && strtoul(c.reply.text.data, NULL, 10) == sizeof g_body);
    e = d_ftp_client_retrieve(&c, name, sink_mem, &back);
    CHECK(e == D_FTP_OK && back.length == sizeof g_body && memcmp(g_back, g_body, sizeof g_body) == 0);
    if (e != D_FTP_OK) report(_label, &c, e);
    e = d_ftp_client_list(&c, NULL, sink_mem, &listing, &format);
    g_listing[listing.length] = '\0';
    CHECK(e == D_FTP_OK && strstr(g_listing, name) != NULL);
    CHECK(d_ftp_client_command(&c, D_FTP_COMMAND_DELE, name) == D_FTP_OK);
    const bool control_tls = c.control.secured;
    const bool data_tls    = (c.protection == D_FTP_PROTECTION_PRIVATE);
    CHECK(d_ftp_client_quit(&c) == D_FTP_OK);
    printf("  %s: %s, listed as %s, control %s, data %s\n", _label,
           (g_fail == failures) ? "300000 bytes up and back" : "FAILED",
           (format == D_FTP_LISTING_MLSX) ? "MLSD" : "LIST",
           control_tls ? "TLS" : "plain", data_tls ? "TLS" : "plain");
    d_ftp_client_close(&c);
}

static void options(struct d_ftp_options* _o, enum d_ftp_security _security, enum d_ftp_data_mode _mode)
{
    d_ftp_options_init(_o);
    _o->user               = env("FTP_TEST_USER", "ftpuser");
    _o->password           = env("FTP_TEST_PASSWORD", "");
    _o->security           = _security;
    _o->data_mode          = _mode;
    _o->connect_timeout_ms = 10000u;
    if (_security != D_FTP_SECURITY_NONE)
    {
        _o->data_protection  = D_FTP_PROTECTION_PRIVATE;
        _o->require_security = true;
    }
}

// a context trusting the test CA alone, or the system store alone
static int context(struct d_ssl_context* _ctx, const struct d_ftp_options* _o, const char* _ca)
{
    struct d_ssl_config config;
    d_ftp_client_tls_config(_o, &config);
    config.ca_file          = _ca;
    config.use_system_trust = (_ca == NULL);
    return d_ssl_context_init(_ctx, d_ssl_engine_openssl(), &config) == D_SSL_STATUS_OK;
}

int main(void)
{
    const char* port_text = getenv("FTP_TEST_PORT");
    if (!port_text)
    {
        printf("interop: skipped (FTP_TEST_* not set)\n");
        return 0;
    }
    const uint16_t port     = (uint16_t)atoi(port_text);
    const uint16_t implicit = (uint16_t)atoi(env("FTP_TEST_IMPLICIT_PORT", "0"));
    const uint16_t reuse    = (uint16_t)atoi(env("FTP_TEST_REUSE_PORT", "0"));
    const char*    address  = env("FTP_TEST_ADDRESS", "127.0.0.1");
    const char*    tls_host = env("FTP_TEST_TLS_HOST", "localhost");
    const char*    ca       = getenv("FTP_TEST_CA");
    struct d_ftp_options o;
    struct d_ssl_context ctx;

    for (size_t i = 0; i < sizeof g_body; i++) g_body[i] = (char)((i * 2654435761u) >> 13);

    printf("plain FTP:\n");
    options(&o, D_FTP_SECURITY_NONE, D_FTP_DATA_PASSIVE_AUTO);
    session("plain_passive", &o, NULL, address, port);
    options(&o, D_FTP_SECURITY_NONE, D_FTP_DATA_ACTIVE_AUTO);
    session("plain_active", &o, NULL, address, port);

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
        printf("explicit FTPS (AUTH TLS, PROT P), OpenSSL engine:\n");
        options(&o, D_FTP_SECURITY_EXPLICIT, D_FTP_DATA_PASSIVE_AUTO);
        CHECK(context(&ctx, &o, ca));
        session("explicit_passive", &o, &ctx, tls_host, port);
        o.data_mode = D_FTP_DATA_ACTIVE_AUTO;
        session("explicit_active", &o, &ctx, tls_host, port);
        d_ssl_context_destroy(&ctx);

        if (implicit)
        {
            printf("implicit FTPS:\n");
            options(&o, D_FTP_SECURITY_IMPLICIT, D_FTP_DATA_PASSIVE_AUTO);
            CHECK(context(&ctx, &o, ca));
            session("implicit_passive", &o, &ctx, tls_host, implicit);
            d_ssl_context_destroy(&ctx);
        }

        // a certificate the context does not trust ends the session in TLS
        {
            static struct d_ftp_client c;
            options(&o, D_FTP_SECURITY_EXPLICIT, D_FTP_DATA_PASSIVE_AUTO);
            CHECK(context(&ctx, &o, NULL));
            d_ftp_client_init(&c, &o, &ctx);
            const enum d_ftp_error e = d_ftp_client_connect(&c, tls_host, port);
            CHECK(e == D_FTP_ERROR_TLS && !c.connected);
            printf("untrusted certificate:\n");
            report("refused", &c, e);
            d_ftp_client_close(&c);
            d_ssl_context_destroy(&ctx);
        }

        // a server requiring session reuse: whatever the outcome, it is clean
        if (reuse)
        {
            static struct d_ftp_client c;
            struct mem back = { g_back, 0, sizeof g_back };
            options(&o, D_FTP_SECURITY_EXPLICIT, D_FTP_DATA_PASSIVE_AUTO);
            CHECK(context(&ctx, &o, ca));
            d_ftp_client_init(&c, &o, &ctx);
            CHECK(d_ftp_client_connect(&c, tls_host, reuse) == D_FTP_OK);
            CHECK(d_ftp_client_login(&c) == D_FTP_OK);
            const enum d_ftp_error e = d_ftp_client_list(&c, NULL, sink_mem, &back, NULL);
            printf("server requiring session reuse:\n");
            if (e == D_FTP_OK)
            {
                printf("  listing: resumed, %zu bytes\n", back.length);
            }
            else
            {
                // the refusal is reported; vsftpd then ends the session, which
                // the next command must report cleanly as a closed connection
                report("listing", &c, e);
                const enum d_ftp_error next = d_ftp_client_command(&c, D_FTP_COMMAND_NOOP, NULL);
                CHECK(next == D_FTP_OK || next == D_FTP_ERROR_CONNECTION_CLOSED);
                report("next command", &c, next);
            }
            d_ftp_client_close(&c);
            d_ssl_context_destroy(&ctx);
        }
    }

    printf("%d checks, %d failures\n", g_checks, g_fail);
    return g_fail != 0;
}
