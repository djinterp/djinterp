// FTPS test support: a deterministic TLS engine and a threaded FTP and FTPS
// server over the TCP transport, shared by the C suite (ftps_test.c) and the
// C++ suite (ftp_client_tests.cpp).
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
#include <time.h>
#include <sys/socket.h>
#include "./ftps_tests_support.h"

// ---------------------------------------------------------------------------
// the toy engine
// ---------------------------------------------------------------------------
struct toy_settings g_toy = { 1, "localhost", 0u };

struct toy_ctx { int server, cache; char cached_host[256]; uint32_t cached_id, next_id, issued[256]; size_t issued_n; };
struct toy_sess
{
    struct toy_ctx* ctx; int server, state, resumed, peer_closed, sent_close; char host[256];
    uint32_t offered, id; size_t in_len, out_len, app_off;
    unsigned char in[1 << 17], out[1 << 17];
};

static uint32_t be32(const unsigned char* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static int toy_queue(struct toy_sess* s, unsigned char type, const void* p, size_t n, unsigned char minor)
{
    if (s->out_len + 5 + n > sizeof s->out) return 0;
    unsigned char* o = s->out + s->out_len;
    o[0] = type; o[1] = 3; o[2] = minor; o[3] = (unsigned char)(n >> 8); o[4] = (unsigned char)n;
    memcpy(o + 5, p, n); s->out_len += 5 + n; return 1;
}
static int toy_peek(struct toy_sess* s, unsigned char* type, const unsigned char** p, size_t* n)
{
    if (s->in_len < 5) return 0;
    size_t len = ((size_t)s->in[3] << 8) | s->in[4];
    if (s->in_len < 5 + len) return 0;
    *type = s->in[0]; *p = s->in + 5; *n = len; return 1;
}
static void toy_pop(struct toy_sess* s)
{
    size_t len = 5 + (((size_t)s->in[3] << 8) | s->in[4]);
    memmove(s->in, s->in + len, s->in_len - len); s->in_len -= len; s->app_off = 0;
}
static enum d_ssl_status toy_context_create(const struct d_ssl_config* c, const struct d_ssl_plan* plan, void** out)
{
    struct toy_ctx* x = calloc(1, sizeof *x); (void)c;
    if (!x) return D_SSL_STATUS_OUT_OF_MEMORY;
    x->server = (plan->role == D_SSL_ROLE_SERVER); x->cache = g_toy.client_cache; x->next_id = 1000; *out = x;
    return D_SSL_STATUS_OK;
}
static void toy_context_destroy(void* x) { free(x); }
static enum d_ssl_status toy_session_create(void* ctx, const char* host, const char* sni, void** out)
{
    struct toy_sess* s = calloc(1, sizeof *s); (void)sni;
    if (!s) return D_SSL_STATUS_OUT_OF_MEMORY;
    s->ctx = ctx; s->server = s->ctx->server;
    if (host) snprintf(s->host, sizeof s->host, "%s", host);
    *out = s; return D_SSL_STATUS_OK;
}
static void toy_session_destroy(void* s) { free(s); }
static enum d_ssl_status toy_feed(void* v, const void* d, size_t n)
{
    struct toy_sess* s = v;
    if (s->in_len + n > sizeof s->in) return D_SSL_STATUS_OUT_OF_MEMORY;
    memcpy(s->in + s->in_len, d, n); s->in_len += n; return D_SSL_STATUS_OK;
}
static enum d_ssl_status toy_drain(void* v, void* b, size_t cap, size_t* out)
{
    struct toy_sess* s = v; size_t n = s->out_len < cap ? s->out_len : cap;
    memcpy(b, s->out, n); memmove(s->out, s->out + n, s->out_len - n); s->out_len -= n; *out = n;
    return D_SSL_STATUS_OK;
}
static enum d_ssl_status toy_handshake(void* v)
{
    struct toy_sess* s = v; unsigned char t; const unsigned char* p; size_t n;
    if (s->state == 2) return D_SSL_STATUS_OK;
    if (!s->server)
    {
        if (s->state == 0)
        {
            uint32_t id = (s->ctx->cache && s->ctx->cached_id && !strcmp(s->ctx->cached_host, s->host)) ? s->ctx->cached_id : 0u;
            unsigned char m[6] = { 'C', 'H', (unsigned char)(id >> 24), (unsigned char)(id >> 16), (unsigned char)(id >> 8), (unsigned char)id };
            s->offered = id; toy_queue(s, 22, m, 6, 1); s->state = 1; return D_SSL_STATUS_WANT_READ;
        }
        if (!toy_peek(s, &t, &p, &n)) return D_SSL_STATUS_WANT_READ;
        if (t != 22 || n != 6 || p[0] != 'S' || p[1] != 'H') return D_SSL_STATUS_HANDSHAKE_FAILED;
        s->id = be32(p + 2); s->resumed = (s->offered && s->offered == s->id); toy_pop(s);
        if (s->ctx->cache) { snprintf(s->ctx->cached_host, sizeof s->ctx->cached_host, "%s", s->host); s->ctx->cached_id = s->id; }
        s->state = 2; return D_SSL_STATUS_OK;
    }
    if (!toy_peek(s, &t, &p, &n)) return D_SSL_STATUS_WANT_READ;
    if (t != 22 || n != 6 || p[0] != 'C' || p[1] != 'H') return D_SSL_STATUS_HANDSHAKE_FAILED;
    uint32_t offered = be32(p + 2); int known = 0; toy_pop(s);
    for (size_t i = 0; i < s->ctx->issued_n; i++) known |= (s->ctx->issued[i] == offered);
    s->resumed = (offered && known); s->id = s->resumed ? offered : ++s->ctx->next_id;
    if (!s->resumed && s->ctx->issued_n < 256) s->ctx->issued[s->ctx->issued_n++] = s->id;
    unsigned char m[6] = { 'S', 'H', (unsigned char)(s->id >> 24), (unsigned char)(s->id >> 16), (unsigned char)(s->id >> 8), (unsigned char)s->id };
    toy_queue(s, 22, m, 6, 3); s->state = 2; return D_SSL_STATUS_OK;
}
static enum d_ssl_status toy_read(void* v, void* b, size_t cap, size_t* out)
{
    struct toy_sess* s = v; unsigned char t; const unsigned char* p; size_t n; *out = 0;
    for (;;)
    {
        if (s->peer_closed) return D_SSL_STATUS_CONNECTION_CLOSED;
        if (!toy_peek(s, &t, &p, &n)) return D_SSL_STATUS_WANT_READ;
        if (t == 23)
        {
            size_t k = n - s->app_off; if (k > cap) k = cap;
            memcpy(b, p + s->app_off, k); s->app_off += k;
            if (s->app_off == n) toy_pop(s);
            if (k) { *out = k; return D_SSL_STATUS_OK; }
            continue;
        }
        if (t == 21) { toy_pop(s); s->peer_closed = 1; return D_SSL_STATUS_CONNECTION_CLOSED; }
        if (t == 22) { toy_pop(s); continue; }
        return D_SSL_STATUS_PROTOCOL_ERROR;
    }
}
static enum d_ssl_status toy_write(void* v, const void* d, size_t n, size_t* out)
{
    struct toy_sess* s = v; size_t k = n > 16384 ? 16384 : n;
    if (!toy_queue(s, 23, d, k, 3)) return D_SSL_STATUS_WANT_READ;
    *out = k; return D_SSL_STATUS_OK;
}
static enum d_ssl_status toy_close(void* v)
{
    struct toy_sess* s = v; unsigned char a[2] = { 1, 0 };
    if (!s->sent_close) { toy_queue(s, 21, a, 2, 3); s->sent_close = 1; }
    return D_SSL_STATUS_OK;
}
static void toy_describe(void* v, struct d_ssl_info* i)
{
    struct toy_sess* s = v;
    if (s->state != 2) return;
    i->version = D_SSL_VERSION_TLS1_3; i->cipher_id = 0x1301;
    snprintf(i->cipher, sizeof i->cipher, "TLS_AES_128_GCM_SHA256");
    i->verify = s->server ? (uint32_t)D_SSL_VERIFY_FLAG_NOT_PERFORMED : g_toy.chain_verdict;
    i->resumed = (s->resumed != 0);
}
static bool toy_peer_name(void* v, size_t index, struct d_ssl_peer_name* out)
{
    struct toy_sess* s = v;
    if (s->server || index > 0 || s->state != 2) return false;
    out->kind = D_SSL_NAME_DNS; out->value.data = g_toy.cert_name; out->value.size = strlen(g_toy.cert_name);
    return true;
}
static bool toy_peer_certificate(void* v, struct d_pack_bytes* der)
{
    static const unsigned char cert[] = { 0x30, 0x03, 0x02, 0x01, 0x01 };
    struct toy_sess* s = v;
    if (s->server || s->state != 2) return false;
    der->data = cert; der->size = sizeof cert; return true;
}
const struct d_ssl_engine TOY =
{
    "toy", D_SSL_BACKEND_OTHER, D_SSL_ENGINE_TLS1_3 | D_SSL_ENGINE_SYSTEM_TRUST | D_SSL_ENGINE_MEMORY_PEM,
    toy_context_create, toy_context_destroy, toy_session_create, toy_session_destroy, toy_feed, toy_drain,
    toy_handshake, toy_read, toy_write, toy_close, toy_describe, toy_peer_name, toy_peer_certificate
};
static const char CERT_PEM[] = "-----BEGIN CERTIFICATE-----\nMAMCAQE=\n-----END CERTIFICATE-----\n";
static const char KEY_PEM[] = "-----BEGIN PRIVATE KEY-----\nMAMCAQE=\n-----END PRIVATE KEY-----\n";

// ---------------------------------------------------------------------------
// the test server
// ---------------------------------------------------------------------------
struct sconn { struct d_tcp_connection tcp; struct d_ssl_session tls; int secured; char buf[4096]; size_t start, end; };

char g_big[200000];
const char HELLO[] = "Hello, FTPS!\r\nLine two\r\n";

// the server's transport, over net.h, for plain reads and its TLS sessions alike
static enum d_ssl_status sc_tread(void* ctx, void* b, size_t cap, size_t* n)
{
    struct d_net_io_result r = d_net_connection_read(&((struct sconn*)ctx)->tcp.base, b, cap);
    *n = r.count; if (r.error != D_NET_ERROR_NONE) return D_SSL_STATUS_TRANSPORT_ERROR;
    return r.count ? D_SSL_STATUS_OK : D_SSL_STATUS_CONNECTION_CLOSED;
}
static enum d_ssl_status sc_twrite(void* ctx, const void* d, size_t n, size_t* w)
{
    struct d_net_io_result r = d_net_connection_write(&((struct sconn*)ctx)->tcp.base, d, n);
    *w = r.count; return (r.error == D_NET_ERROR_NONE && r.count) ? D_SSL_STATUS_OK : D_SSL_STATUS_CONNECTION_CLOSED;
}
static struct d_ssl_transport sc_transport(struct sconn* c) { struct d_ssl_transport t = { sc_tread, sc_twrite, c }; return t; }
static enum d_ssl_status sc_read(struct sconn* c, void* b, size_t cap, size_t* n)
{
    if (c->secured) return d_ssl_session_read(&c->tls, b, cap, n);
    return sc_tread(c, b, cap, n);
}
static int sc_write(struct sconn* c, const char* d, size_t n)
{
    size_t w = 0;
    if (c->secured) return d_ssl_session_write_all(&c->tls, d, n, &w) == D_SSL_STATUS_OK;
    return d_ssl_transport_write_all(sc_transport(c), d, n, &w) == D_SSL_STATUS_OK;
}
static int sc_say(struct sconn* c, const char* text) { return sc_write(c, text, strlen(text)); }
static int sc_line(struct sconn* c, char* line, size_t cap)
{
    size_t len = 0;
    for (;;)
    {
        while (c->start < c->end)
        {
            char ch = c->buf[c->start++];
            if (ch == '\n') { if (len && line[len - 1] == '\r') len--; line[len] = 0; return 1; }
            if (len + 1 < cap) line[len++] = ch;
        }
        size_t n = 0;
        if (sc_read(c, c->buf, sizeof c->buf, &n) != D_SSL_STATUS_OK) return 0;
        c->start = 0; c->end = n;
    }
}
static int sc_secure(struct srv* s, struct sconn* c)
{
    struct d_pack_text none = { "", 0 };
    if (d_ssl_session_init(&c->tls, &s->tls, sc_transport(c), none) != D_SSL_STATUS_OK) return 0;
    if (d_ssl_session_handshake(&c->tls) != D_SSL_STATUS_OK) { d_ssl_session_destroy(&c->tls); return 0; }
    c->secured = 1; return 1;
}
static void sc_close(struct sconn* c, int orderly)
{
    if (c->secured) { if (orderly) (void)d_ssl_session_shutdown(&c->tls); d_ssl_session_destroy(&c->tls); c->secured = 0; }
    d_tcp_connection_close(&c->tcp);
}
// opens the data connection a transfer command needs; 0 on failure, 2 when TLS
// did not resume and the configuration requires it
static int srv_data(struct srv* s, struct sconn* d, struct d_tcp_listener* pl, struct d_ftp_endpoint* target, char prot)
{
    int ok;
    memset(d, 0, sizeof *d); d_tcp_connection_init(&d->tcp);
    if (target->family != D_FTP_FAMILY_NONE) { struct d_net_endpoint e; struct d_pack_text h = { target->address, strlen(target->address) }; d_net_endpoint_init(&e);
      ok = d_net_endpoint_set(&e, h, target->port, D_NET_PROTOCOL_TCP) == D_NET_ERROR_NONE && d_tcp_connect(&d->tcp, &e, NULL) == D_NET_ERROR_NONE; target->family = D_FTP_FAMILY_NONE; }
    else { ok = d_tcp_accept(pl, &d->tcp) == D_NET_ERROR_NONE; d_tcp_listener_close(pl); }
    if (!ok) return 0;
    if (prot != 'P') { s->data_plain++; return 1; }
    if (!sc_secure(s, d)) { sc_close(d, 0); return 0; }
    struct d_ssl_info info; d_ssl_session_info(&d->tls, &info);
    if (info.resumed) s->data_resumed++; else s->data_fresh++;
    if (s->cfg.require_reuse && !info.resumed) { sc_close(d, 0); return 2; }
    s->data_tls++; return 1;
}
static int srv_listen_pasv(struct d_tcp_listener* pl, uint16_t* port)
{
    struct d_net_endpoint e; struct d_pack_text h = { "127.0.0.1", 9 };
    d_net_endpoint_init(&e); d_tcp_listener_close(pl);
    if (d_net_endpoint_set(&e, h, 0, D_NET_PROTOCOL_TCP) != D_NET_ERROR_NONE || d_tcp_listen(pl, &e, NULL) != D_NET_ERROR_NONE || !d_tcp_listener_local_endpoint(pl, &e)) return 0;
    *port = e.port; return 1;
}
static void srv_send_file(struct srv* s, struct sconn* ctl, struct sconn* d, const char* name)
{
    if (!strcmp(name, "big.bin")) sc_write(d, g_big, sizeof g_big); else sc_write(d, HELLO, strlen(HELLO));
    sc_close(d, !s->cfg.truncate); sc_say(ctl, "226 Transfer complete\r\n");
}
static void srv_receive_file(struct srv* s, struct sconn* ctl, struct sconn* d)
{
    char b[1024]; size_t n = 0; enum d_ssl_status st;
    s->stored_len = 0;
    while ((st = sc_read(d, b, sizeof b, &n)) == D_SSL_STATUS_OK)
        if (s->stored_len + n <= sizeof s->stored) { memcpy(s->stored + s->stored_len, b, n); s->stored_len += n; }
    s->upload_clean = (st == D_SSL_STATUS_CONNECTION_CLOSED);
    // like vsftpd, answer close_notify with close_notify -- after a pause that
    // shows whether the client waited for it: one that closed at once has left
    // an end of stream to peek at by now
    if (d->secured && s->upload_clean)
    {
        struct timespec pause = { 0, 100000000L };
        char            peek  = 0;
        nanosleep(&pause, NULL);
        s->upload_waited = (recv(d_tcp_connection_native(&d->tcp), &peek, 1, MSG_PEEK | MSG_DONTWAIT) != 0);
    }
    sc_close(d, 1); sc_say(ctl, s->upload_clean ? "226 Stored\r\n" : "426 Upload truncated\r\n");
}
static void* srv_main(void* arg)
{
    struct srv* s = arg; struct sconn ctl, d; struct d_tcp_listener pl; struct d_ftp_endpoint target = { .family = D_FTP_FAMILY_NONE };
    char line[1024], out[256]; char prot = 'C'; uint16_t port = 0;
    memset(&ctl, 0, sizeof ctl); d_tcp_connection_init(&ctl.tcp); d_tcp_listener_init(&pl);
    if (d_tcp_accept(&s->listener, &ctl.tcp) != D_NET_ERROR_NONE) return NULL;
    if (s->cfg.implicit && !sc_secure(s, &ctl)) { sc_close(&ctl, 0); return NULL; }
    sc_say(&ctl, "220 Test FTPS server ready\r\n");
    while (sc_line(&ctl, line, sizeof line))
    {
        const char* arg1 = strchr(line, ' '); arg1 = arg1 ? arg1 + 1 : "";
        if (!strncmp(line, "USER ", 5)) { if (!ctl.secured) s->saw_user_in_clear = 1; sc_say(&ctl, "331 Password required\r\n"); }
        else if (!strncmp(line, "PASS ", 5)) { int ok = !strcmp(arg1, "secret") || !strcmp(arg1, "anonymous@"); s->logins += ok; sc_say(&ctl, ok ? "230 Logged in\r\n" : "530 Login incorrect\r\n"); }
        else if (!strcmp(line, "AUTH TLS"))
        {
            if (s->cfg.refuse_auth) { sc_say(&ctl, "504 AUTH not supported\r\n"); continue; }
            sc_say(&ctl, s->cfg.inject ? "234 Proceed\r\n230 Injected login\r\n" : "234 Proceed\r\n");
            if (!sc_secure(s, &ctl)) break;
        }
        else if (!strcmp(line, "PBSZ 0")) sc_say(&ctl, "200 PBSZ=0\r\n");
        else if (!strncmp(line, "PROT ", 5)) { prot = arg1[0]; sc_say(&ctl, "200 Protection level set\r\n"); }
        else if (!strcmp(line, "FEAT"))
        {
            sc_say(&ctl, "211-Features:\r\n EPSV\r\n PASV\r\n");
            if (!s->cfg.no_mlst) sc_say(&ctl, " MLST type*;size*;modify*;\r\n");
            sc_say(&ctl, " UTF8\r\n AUTH TLS\r\n PBSZ\r\n PROT\r\n211 End\r\n");
        }
        else if (!strcmp(line, "OPTS UTF8 ON")) sc_say(&ctl, "200 UTF8 on\r\n");
        else if (!strncmp(line, "TYPE ", 5)) { s->type = arg1[0]; sc_say(&ctl, "200 Type set\r\n"); }
        else if (!strcmp(line, "EPSV"))
        {
            s->epsv_count++;
            if (s->cfg.refuse_epsv || !srv_listen_pasv(&pl, &port)) { sc_say(&ctl, "500 EPSV not understood\r\n"); continue; }
            snprintf(out, sizeof out, "229 Entering Extended Passive Mode (|||%u|)\r\n", port); sc_say(&ctl, out);
        }
        else if (!strcmp(line, "PASV"))
        {
            if (!srv_listen_pasv(&pl, &port)) { sc_say(&ctl, "425 No port\r\n"); continue; }
            snprintf(out, sizeof out, "227 Entering Passive Mode (%s,%u,%u).\r\n", s->cfg.bogus_pasv ? "10,255,255,1" : "127,0,0,1", port >> 8, port & 0xFF);
            sc_say(&ctl, out);
        }
        else if (!strncmp(line, "EPRT ", 5)) sc_say(&ctl, d_ftp_parse_eprt(arg1, strlen(arg1), &target) == D_FTP_OK ? "200 EPRT ok\r\n" : "501 Bad EPRT\r\n");
        else if (!strncmp(line, "PORT ", 5)) sc_say(&ctl, d_ftp_parse_port(arg1, strlen(arg1), &target) == D_FTP_OK ? "200 PORT ok\r\n" : "501 Bad PORT\r\n");
        else if (!strncmp(line, "RETR ", 5) || !strncmp(line, "STOR ", 5) || !strcmp(line, "MLSD") || !strcmp(line, "LIST"))
        {
            sc_say(&ctl, "150 Opening data connection\r\n");
            int r = srv_data(s, &d, &pl, &target, prot);
            if (r == 2) { sc_say(&ctl, "522 SSL connection failed: session reuse required\r\n"); continue; }
            if (r == 0) { sc_say(&ctl, "425 Cannot open data connection\r\n"); continue; }
            if (line[0] == 'R') srv_send_file(s, &ctl, &d, arg1);
            else if (line[0] == 'S') srv_receive_file(s, &ctl, &d);
            else
            {
                sc_say(&d, line[0] == 'M' ? "type=file;size=24;modify=20260925123456; hello.txt\r\ntype=dir;modify=20260101000000; pub\r\n"
                                          : "-rw-r--r-- 1 u g 24 Sep 25 12:34 hello.txt\r\n");
                sc_close(&d, 1); sc_say(&ctl, "226 Listing sent\r\n");
            }
        }
        else if (!strncmp(line, "SIZE ", 5)) sc_say(&ctl, "213 24\r\n");
        else if (!strcmp(line, "CCC"))
        {
            char b[16]; size_t n = 0;
            // the client's close_notify first: it sends nothing more until ours
            // arrives, so no plaintext can be read ahead with it
            sc_say(&ctl, "200 CCC ok\r\n");
            s->ccc_done = (d_ssl_session_read(&ctl.tls, b, sizeof b, &n) == D_SSL_STATUS_CONNECTION_CLOSED);
            (void)d_ssl_session_shutdown(&ctl.tls); d_ssl_session_destroy(&ctl.tls); ctl.secured = 0;
        }
        else if (!strcmp(line, "QUIT")) { sc_say(&ctl, "221 Goodbye\r\n"); break; }
        else sc_say(&ctl, "502 Command not implemented\r\n");
    }
    sc_close(&ctl, 1); d_tcp_listener_close(&pl);
    return NULL;
}
int srv_start(struct srv* s, struct srv_cfg cfg)
{
    struct d_ssl_config c; struct d_ftp_endpoint e = { .family = D_FTP_FAMILY_IPV4 };
    memset(s, 0, sizeof *s); s->cfg = cfg;
    d_ssl_config_init(&c, D_SSL_ROLE_SERVER);
    c.certificate_pem.data = CERT_PEM; c.certificate_pem.length = strlen(CERT_PEM);
    c.private_key_pem.data = KEY_PEM; c.private_key_pem.length = strlen(KEY_PEM);
    enum d_ssl_status st = d_ssl_context_init(&s->tls, &TOY, &c);
    if (st != D_SSL_STATUS_OK) { printf("server context: %s\n", d_ssl_status_name(st)); return 0; }
    { struct d_net_endpoint ne; struct d_pack_text h = { "127.0.0.1", 9 }; d_net_endpoint_init(&ne); d_tcp_listener_init(&s->listener);
      if (d_net_endpoint_set(&ne, h, 0, D_NET_PROTOCOL_TCP) != D_NET_ERROR_NONE || d_tcp_listen(&s->listener, &ne, NULL) != D_NET_ERROR_NONE || !d_tcp_listener_local_endpoint(&s->listener, &ne)) return 0;
      s->port = ne.port; (void)e; }
    return pthread_create(&s->thread, NULL, srv_main, s) == 0;
}
void srv_stop(struct srv* s) { pthread_join(s->thread, NULL); d_tcp_listener_close(&s->listener); d_ssl_context_destroy(&s->tls); }
