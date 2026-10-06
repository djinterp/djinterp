// FTPS tests: the net/ftp/ client over real loopback sockets, secured through
// the SSL kernel with a deterministic test engine, against a threaded server
// that the same kernel secures in the server role. Both live in
// ftps_tests_support.c, which the C++ suite shares.
//
// The engine ("toy") frames every message as a TLS record, so the kernel's
// sniffing, pumping, and identity checks all run for real; its "encryption"
// is the identity. Its client contexts cache one session per host and offer
// it again, which is how a real engine resumes the control session on FTPS
// data connections.
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../inc/djinterp/net/ftp/ftp.h"
#include "../../../../inc/djinterp/net/ssl/ssl.h"
#include "./ftps_tests_support.h"

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)


// ---------------------------------------------------------------------------
// the scenarios
// ---------------------------------------------------------------------------
static struct { char data[300000]; size_t len; } g_sink;
static struct d_ftp_client  g_client;
static struct d_ssl_context g_ctx;

static enum d_ftp_error sink_mem(void* c, const void* d, size_t n)
{
    (void)c;
    if (g_sink.len + n > sizeof g_sink.data) return D_FTP_ERROR_BUFFER_TOO_SMALL;
    memcpy(g_sink.data + g_sink.len, d, n); g_sink.len += n; return D_FTP_OK;
}
struct src { const char* data; size_t len, pos; };
static enum d_ftp_error source_mem(void* c, void* b, size_t cap, size_t* out)
{
    struct src* s = c; size_t n = s->len - s->pos; if (n > cap) n = cap;
    memcpy(b, s->data + s->pos, n); s->pos += n; *out = n; return D_FTP_OK;
}
static void options(struct d_ftp_options* o, enum d_ftp_security security)
{
    d_ftp_options_init(o); o->security = security; o->user = "alice"; o->password = "secret";
    o->connect_timeout_ms = 5000; o->response_timeout_ms = 5000; o->idle_timeout_ms = 5000;
}
static int setup(struct srv* s, struct srv_cfg cfg, const struct d_ftp_options* o)
{
    struct d_ssl_config c; d_ftp_client_tls_config(o, &c);
    enum d_ssl_status st = d_ssl_context_init(&g_ctx, &TOY, &c);
    if (st != D_SSL_STATUS_OK) { printf("client context: %s\n", d_ssl_status_name(st)); return 0; }
    if (!srv_start(s, cfg)) { d_ssl_context_destroy(&g_ctx); return 0; }
    d_ftp_client_init(&g_client, o, &g_ctx); return 1;
}
static void teardown(struct srv* s) { d_ftp_client_close(&g_client); srv_stop(s); d_ssl_context_destroy(&g_ctx); }
static int fetch_hello(void) { g_sink.len = 0; return d_ftp_client_retrieve(&g_client, "hello.txt", sink_mem, NULL) == D_FTP_OK && g_sink.len == 24 && !memcmp(g_sink.data, HELLO, 24); }

static void t_explicit(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o; struct src up = { "upload body\r\n", 13, 0 };
    struct d_ssl_info info; enum d_ftp_listing_format fmt = D_FTP_LISTING_UNIX;
    cfg.require_reuse = 1; options(&o, D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && g_client.control.secured);
    CHECK((g_client.features & (D_FTP_FEATURE_EPSV | D_FTP_FEATURE_MLST | D_FTP_FEATURE_UTF8)) == (D_FTP_FEATURE_EPSV | D_FTP_FEATURE_MLST | D_FTP_FEATURE_UTF8));
    CHECK(d_ssl_session_info(&g_client.control.tls, &info) && info.version == D_SSL_VERSION_TLS1_3 && !info.resumed && info.has_peer_certificate);
    CHECK(d_ftp_client_login(&g_client) == D_FTP_OK && g_client.protection == D_FTP_PROTECTION_PRIVATE);
    CHECK(fetch_hello());
    g_sink.len = 0;
    CHECK(d_ftp_client_retrieve(&g_client, "big.bin", sink_mem, NULL) == D_FTP_OK && g_sink.len == sizeof g_big && !memcmp(g_sink.data, g_big, sizeof g_big));
    CHECK(d_ftp_client_store(&g_client, "up.txt", source_mem, &up) == D_FTP_OK);
    g_sink.len = 0;
    CHECK(d_ftp_client_list(&g_client, NULL, sink_mem, NULL, &fmt) == D_FTP_OK && fmt == D_FTP_LISTING_MLSX);
    struct d_ftp_span cur = { g_sink.data, g_sink.len }, line; struct d_ftp_entry e; int files = 0, dirs = 0;
    while (d_ftp_span_next_line(&cur, &line))
        if (d_ftp_listing_parse(line.data, line.length, fmt, NULL, &e) == D_FTP_LINE_ENTRY) { files += e.type == D_FTP_ENTRY_FILE; dirs += e.type == D_FTP_ENTRY_DIRECTORY; }
    CHECK(files == 1 && dirs == 1);
    CHECK(d_ftp_client_command(&g_client, D_FTP_COMMAND_SIZE, "hello.txt") == D_FTP_OK && g_client.reply.code == 213);
    CHECK(d_ftp_client_command(&g_client, D_FTP_COMMAND_RETR, "x") == D_FTP_ERROR_INVALID_ARGUMENT);
    CHECK(d_ftp_client_command(&g_client, D_FTP_COMMAND_CCC, NULL) == D_FTP_ERROR_INVALID_ARGUMENT);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK && !g_client.connected && !g_client.control.secured);
    teardown(&s);
    CHECK(!s.saw_user_in_clear && s.logins == 1 && s.type == 'I');
    CHECK(s.data_tls == 4 && s.data_resumed == 4 && s.data_fresh == 0);
    CHECK(s.upload_clean && s.stored_len == 13 && !memcmp(s.stored, "upload body\r\n", 13));
    CHECK(s.upload_waited);  // the client waited for the server's close_notify
}
static void t_implicit(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    cfg.implicit = 1; options(&o, D_FTP_SECURITY_IMPLICIT);
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && g_client.control.secured);
    CHECK(d_ftp_client_login(&g_client) == D_FTP_OK && fetch_hello());
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    CHECK(!s.saw_user_in_clear && s.data_tls == 1 && s.data_resumed == 1);
}
static void t_downgrade(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    cfg.refuse_auth = 1; options(&o, D_FTP_SECURITY_EXPLICIT);   // require_security by default
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_ERROR_SECURITY && !g_client.connected);
    teardown(&s);
    CHECK(!s.saw_user_in_clear && s.logins == 0);

    o.require_security = false;                                   // the caller accepts plaintext
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && !g_client.control.secured);
    CHECK(d_ftp_client_login(&g_client) == D_FTP_OK && g_client.protection == D_FTP_PROTECTION_CLEAR && fetch_hello());
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    CHECK(s.saw_user_in_clear && s.data_plain == 1 && s.data_tls == 0);
}
static void t_injection(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o; enum d_ftp_error err;
    cfg.inject = 1; options(&o, D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    err = d_ftp_client_connect(&g_client, "localhost", s.port);
    CHECK(err == D_FTP_ERROR_SECURITY || (err == D_FTP_ERROR_TLS && g_client.tls_status == D_SSL_STATUS_NOT_TLS));
    CHECK(!g_client.connected);
    teardown(&s);
    CHECK(s.logins == 0 && !s.saw_user_in_clear);
}
static void t_reuse_required(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    cfg.require_reuse = 1; options(&o, D_FTP_SECURITY_EXPLICIT); g_toy.client_cache = 0;   // an engine without resumption
    if (!setup(&s, cfg, &o)) { CHECK(0); g_toy.client_cache = 1; return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && d_ftp_client_login(&g_client) == D_FTP_OK);
    g_sink.len = 0;
    CHECK(d_ftp_client_retrieve(&g_client, "hello.txt", sink_mem, NULL) == D_FTP_ERROR_TLS && g_client.reply.code == 522);
    CHECK(d_ftp_client_command(&g_client, D_FTP_COMMAND_SIZE, "hello.txt") == D_FTP_OK && g_client.reply.code == 213);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s); g_toy.client_cache = 1;
    CHECK(s.data_fresh == 1 && s.data_tls == 0);
}
static void t_truncation(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    cfg.truncate = 1; options(&o, D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && d_ftp_client_login(&g_client) == D_FTP_OK);
    g_sink.len = 0;
    CHECK(d_ftp_client_retrieve(&g_client, "hello.txt", sink_mem, NULL) == D_FTP_ERROR_TLS);
    CHECK(g_client.tls_status == D_SSL_STATUS_UNEXPECTED_EOF && g_client.reply.code == 226 && g_sink.len == 24);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
}
static void t_pasv_fallback(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    cfg.refuse_epsv = 1; cfg.bogus_pasv = 1; options(&o, D_FTP_SECURITY_EXPLICIT);
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && d_ftp_client_login(&g_client) == D_FTP_OK);
    CHECK(fetch_hello() && fetch_hello() && g_client.extended_refused);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    CHECK(s.epsv_count == 1 && s.data_tls == 2);
}
static void t_active(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o; struct src up = { "active\r\n", 8, 0 };
    options(&o, D_FTP_SECURITY_EXPLICIT); o.data_mode = D_FTP_DATA_ACTIVE_AUTO;
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && d_ftp_client_login(&g_client) == D_FTP_OK);
    CHECK(fetch_hello() && d_ftp_client_store(&g_client, "a.txt", source_mem, &up) == D_FTP_OK);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    CHECK(s.data_tls == 2 && s.data_resumed == 2 && s.stored_len == 8 && s.upload_clean);
}
static void t_ccc(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    options(&o, D_FTP_SECURITY_EXPLICIT); o.clear_control = true;
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && d_ftp_client_login(&g_client) == D_FTP_OK);
    CHECK(!g_client.control.secured && g_client.protection == D_FTP_PROTECTION_PRIVATE && fetch_hello());
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    CHECK(s.ccc_done && s.data_tls == 1);
}
static void t_verify(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o; struct d_ssl_config c;
    options(&o, D_FTP_SECURITY_EXPLICIT); g_toy.cert_name = "evil.example";          // a certificate for another host
    if (!setup(&s, cfg, &o)) { CHECK(0); g_toy.cert_name = "localhost"; return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_ERROR_TLS && g_client.tls_status == D_SSL_STATUS_VERIFY_FAILED);
    teardown(&s); g_toy.cert_name = "localhost";
    CHECK(s.logins == 0 && !s.saw_user_in_clear);

    g_toy.chain_verdict = D_SSL_VERIFY_FLAG_UNTRUSTED;                               // a chain that does not validate
    if (!setup(&s, cfg, &o)) { CHECK(0); g_toy.chain_verdict = 0; return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_ERROR_TLS && g_client.tls_status == D_SSL_STATUS_VERIFY_FAILED);
    teardown(&s); g_toy.chain_verdict = 0;

    d_ssl_config_init(&c, D_SSL_ROLE_CLIENT); c.verify = D_SSL_VERIFY_NONE;           // a context that checks less than asked
    CHECK(d_ssl_context_init(&g_ctx, &TOY, &c) == D_SSL_STATUS_OK);
    d_ftp_client_init(&g_client, &o, &g_ctx);
    CHECK(d_ftp_client_connect(&g_client, "localhost", 1) == D_FTP_ERROR_INVALID_ARGUMENT);
    d_ftp_client_init(&g_client, &o, NULL);
    CHECK(d_ftp_client_connect(&g_client, "localhost", 1) == D_FTP_ERROR_INVALID_ARGUMENT);
    d_ssl_context_destroy(&g_ctx);
}
static void t_ascii(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o; struct src up = { "a\nb\n", 4, 0 };
    options(&o, D_FTP_SECURITY_EXPLICIT); o.type.data_type = D_FTP_TYPE_ASCII; o.type.format = D_FTP_FORMAT_NONE;
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "localhost", s.port) == D_FTP_OK && d_ftp_client_login(&g_client) == D_FTP_OK);
    g_sink.len = 0;
    CHECK(d_ftp_client_retrieve(&g_client, "hello.txt", sink_mem, NULL) == D_FTP_OK && g_sink.len == 22 && !memcmp(g_sink.data, "Hello, FTPS!\nLine two\n", 22));
    CHECK(d_ftp_client_store(&g_client, "a.txt", source_mem, &up) == D_FTP_OK);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
    CHECK(s.type == 'A' && s.stored_len == 6 && !memcmp(s.stored, "a\r\nb\r\n", 6));
}
static void t_plain_and_login(void)
{
    struct srv s; struct srv_cfg cfg = { 0 }; struct d_ftp_options o;
    options(&o, D_FTP_SECURITY_NONE);
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "127.0.0.1", s.port) == D_FTP_OK && !g_client.control.secured);
    CHECK(d_ftp_client_retrieve(&g_client, "hello.txt", sink_mem, NULL) == D_FTP_ERROR_BAD_SEQUENCE);
    CHECK(d_ftp_client_login(&g_client) == D_FTP_OK && fetch_hello());
    CHECK(d_ftp_client_login(&g_client) == D_FTP_ERROR_BAD_SEQUENCE);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK && d_ftp_client_quit(&g_client) == D_FTP_ERROR_BAD_SEQUENCE);
    teardown(&s);
    CHECK(s.data_plain == 1);

    o.password = "wrong";
    if (!setup(&s, cfg, &o)) { CHECK(0); return; }
    CHECK(d_ftp_client_connect(&g_client, "127.0.0.1", s.port) == D_FTP_OK);
    CHECK(d_ftp_client_login(&g_client) == D_FTP_ERROR_LOGIN_DENIED && !g_client.logged_in && g_client.reply.code == 530);
    CHECK(d_ftp_client_quit(&g_client) == D_FTP_OK);
    teardown(&s);
}

int main(void)
{
    for (size_t i = 0; i < sizeof g_big; i++) g_big[i] = (char)(i * 31u + 7u);
    t_explicit(); t_implicit(); t_downgrade(); t_injection(); t_reuse_required(); t_truncation();
    t_pasv_fallback(); t_active(); t_ccc(); t_verify(); t_ascii(); t_plain_and_login();
    printf("%d checks, %d failures\n", g_checks, g_fail);
    return g_fail != 0;
}
