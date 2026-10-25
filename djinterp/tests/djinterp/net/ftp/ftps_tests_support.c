/*******************************************************************************
* djinterp [net]                                            ftps_tests_support.c
*
* FTPS test support: a deterministic TLS engine and a threaded FTP and FTPS
* server over the TCP transport, shared by the C suite (ftps_test.c) and the
* C++ suite (ftp_client_tests.cpp).
*   The engine ("toy") frames every message as a TLS record, so the kernel's
* sniffing, pumping, and identity checks all run for real; its "encryption"
* is the identity. Its client contexts cache one session per host and offer
* it again, which is how a real engine resumes the control session on FTPS
* data connections.
*
*
* path:      /tests/djinterp/net/ftp/ftps_tests_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "./ftps_tests_support.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // printf, snprintf
#include <stdlib.h>   // calloc, free
#include <string.h>   // memcpy, memset, strcmp, strncmp, strlen, strchr
#include <time.h>     // nanosleep, struct timespec
// POSIX
#include <pthread.h>     // pthread_create, pthread_join
#include <sys/socket.h>  // recv, MSG_PEEK, MSG_DONTWAIT
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint16_t, uint32_t


// the toy engine
struct d_tests_ftps_toy_settings g_toy = { 1, "localhost", 0u };

struct d_tests_ftps_toy_context
{
    int      server;
    int      cache;
    char     cached_host[256];
    uint32_t cached_id;
    uint32_t next_id;
    uint32_t issued[256];
    size_t   issued_n;
};

struct d_tests_ftps_toy_session
{
    struct d_tests_ftps_toy_context* ctx;
    int                              server;
    int                              state;
    int                              resumed;
    int                              peer_closed;
    int                              sent_close;
    char                             host[256];
    uint32_t                         offered;
    uint32_t                         id;
    size_t                           in_len;
    size_t                           out_len;
    size_t                           app_off;
    unsigned char                    in[1 << 17];
    unsigned char                    out[1 << 17];
};

/*
be32
  Reads a big-endian 32-bit integer from four octets.
*/
static uint32_t
be32(
    const unsigned char* _p
)
{
    return ((uint32_t)_p[0] << 24) | ((uint32_t)_p[1] << 16) |
           ((uint32_t)_p[2] << 8) | _p[3];
}

/*
toy_queue
  Appends one toy record -- a five-octet header with the payload's length,
then the payload -- to the session's outgoing octets; 0 when it does not
fit.
*/
static int
toy_queue(
    struct d_tests_ftps_toy_session* _s,
    unsigned char                    _type,
    const void*                      _p,
    size_t                           _n,
    unsigned char                    _minor
)
{
    if (_s->out_len + 5 + _n > sizeof _s->out)
    {
        return 0;
    }

    unsigned char* o = _s->out + _s->out_len;
    o[0]             = _type;
    o[1]             = 3;
    o[2]             = _minor;
    o[3]             = (unsigned char)(_n >> 8);
    o[4]             = (unsigned char)_n;
    memcpy(o + 5,
           _p,
           _n);
    _s->out_len += 5 + _n;

    return 1;
}

/*
toy_peek
  Whether a whole record has arrived: the header's length against the octets
buffered.
*/
static int
toy_peek(
    struct d_tests_ftps_toy_session* _s,
    unsigned char*                   _type,
    const unsigned char**            _p,
    size_t*                          _n
)
{
    if (_s->in_len < 5)
    {
        return 0;
    }

    size_t len = ((size_t)_s->in[3] << 8) | _s->in[4];
    if (_s->in_len < 5 + len)
    {
        return 0;
    }

    *_type = _s->in[0];
    *_p    = _s->in + 5;
    *_n    = len;

    return 1;
}

/*
toy_pop
  Drops the record at the front of the incoming octets.
*/
static void
toy_pop(
    struct d_tests_ftps_toy_session* _s
)
{
    size_t len = 5 + (((size_t)_s->in[3] << 8) | _s->in[4]);
    memmove(_s->in,
            _s->in + len,
            _s->in_len - len);
    _s->in_len -= len;
    _s->app_off = 0;

    return;
}

/*
toy_context_create
  A context of the toy engine: its role, and the session cache a client
context keeps.
*/
static enum d_ssl_error
toy_context_create(
    const struct d_ssl_config* _c,
    const struct d_ssl_plan*   _plan,
    void**                     _out
)
{
    struct d_tests_ftps_toy_context* x = calloc(1,
                                                sizeof *x);
    (void)_c;
    if (!x)
    {
        return D_SSL_ERROR_OUT_OF_MEMORY;
    }

    x->server  = (_plan->role == D_SSL_ROLE_SERVER);
    x->cache   = g_toy.client_cache;
    x->next_id = 1000;
    *_out       = x;

    return D_SSL_OK;
}

/*
toy_context_destroy
  Frees a context.
*/
static void
toy_context_destroy(
    void* _x
)
{
    free(_x);

    return;
}

/*
toy_session_create
  A session over a context, at the start of its handshake.
*/
static enum d_ssl_error
toy_session_create(
    void*       _ctx,
    const char* _host,
    const char* _sni,
    void**      _out
)
{
    struct d_tests_ftps_toy_session* s = calloc(1,
                                                sizeof *s);
    (void)_sni;
    if (!s)
    {
        return D_SSL_ERROR_OUT_OF_MEMORY;
    }

    s->ctx    = _ctx;
    s->server = s->ctx->server;
    if (_host)
    {
        snprintf(s->host,
                 sizeof s->host,
                 "%s",
                 _host);
    }

    *_out = s;

    return D_SSL_OK;
}

/*
toy_session_destroy
  Frees a session.
*/
static void
toy_session_destroy(
    void* _s
)
{
    free(_s);

    return;
}

/*
toy_feed
  Takes octets the kernel read from the transport into the session's incoming
buffer.
*/
static enum d_ssl_error
toy_feed(
    void*       _v,
    const void* _d,
    size_t      _n
)
{
    struct d_tests_ftps_toy_session* s = _v;
    if (s->in_len + _n > sizeof s->in)
    {
        return D_SSL_ERROR_OUT_OF_MEMORY;
    }

    memcpy(s->in + s->in_len,
           _d,
           _n);
    s->in_len += _n;

    return D_SSL_OK;
}

/*
toy_drain
  Hands the kernel the octets the session has queued for the transport.
*/
static enum d_ssl_error
toy_drain(
    void*   _v,
    void*   _b,
    size_t  _cap,
    size_t* _out
)
{
    struct d_tests_ftps_toy_session* s = _v;
    size_t                           n = s->out_len < _cap ? s->out_len : _cap;
    memcpy(_b,
           s->out,
           n);
    memmove(s->out,
            s->out + n,
            s->out_len - n);
    s->out_len -= n;
    *_out = n;

    return D_SSL_OK;
}

/*
toy_handshake
  The handshake, as a small state machine over toy records; a client context
offers the session it cached for the host, which is how resumption is seen.
It runs past 75 lines because each state is a case of its own, as the guides
allow a state machine.
*/
static enum d_ssl_error
toy_handshake(
    void* _v
)
{
    struct d_tests_ftps_toy_session* s = _v;
    unsigned char                    t = 0;
    const unsigned char*             p = NULL;
    size_t                           n;
    if (s->state == 2)
    {
        return D_SSL_OK;
    }

    if (!s->server)
    {
        if (s->state == 0)
        {
            uint32_t      id   = (s->ctx->cache && s->ctx->cached_id &&
                                  !strcmp(s->ctx->cached_host,
                                          s->host))
                                     ? s->ctx->cached_id
                                     : 0u;
            unsigned char m[6] = { 'C',
                                   'H',
                                   (unsigned char)(id >> 24),
                                   (unsigned char)(id >> 16),
                                   (unsigned char)(id >> 8),
                                   (unsigned char)id };
            s->offered         = id;
            toy_queue(s,
                      22,
                      m,
                      6,
                      1);
            s->state = 1;

            return D_SSL_ERROR_WANT_READ;
        }

        if (!toy_peek(s,
                      &t,
                      &p,
                      &n))
        {
            return D_SSL_ERROR_WANT_READ;
        }

        if (t != 22 || n != 6 || p[0] != 'S' || p[1] != 'H')
        {
            return D_SSL_ERROR_HANDSHAKE;
        }

        s->id      = be32(p + 2);
        s->resumed = (s->offered && s->offered == s->id);
        toy_pop(s);
        if (s->ctx->cache)
        {
            snprintf(s->ctx->cached_host,
                     sizeof s->ctx->cached_host,
                     "%s",
                     s->host);
            s->ctx->cached_id = s->id;
        }

        s->state = 2;

        return D_SSL_OK;
    }

    if (!toy_peek(s,
                  &t,
                  &p,
                  &n))
    {
        return D_SSL_ERROR_WANT_READ;
    }

    if (t != 22 || n != 6 || p[0] != 'C' || p[1] != 'H')
    {
        return D_SSL_ERROR_HANDSHAKE;
    }

    uint32_t offered = be32(p + 2);
    int      known   = 0;
    toy_pop(s);
    for (size_t i = 0; i < s->ctx->issued_n; i++)
    {
        known |= (s->ctx->issued[i] == offered);
    }

    s->resumed = (offered && known);
    s->id      = s->resumed ? offered : ++s->ctx->next_id;
    if (!s->resumed && s->ctx->issued_n < 256)
    {
        s->ctx->issued[s->ctx->issued_n++] = s->id;
    }

    unsigned char m[6] = { 'S',
                           'H',
                           (unsigned char)(s->id >> 24),
                           (unsigned char)(s->id >> 16),
                           (unsigned char)(s->id >> 8),
                           (unsigned char)s->id };
    toy_queue(s,
              22,
              m,
              6,
              3);
    s->state = 2;

    return D_SSL_OK;
}

/*
toy_read
  Opens the next application record into the caller's buffer.
*/
static enum d_ssl_error
toy_read(
    void*   _v,
    void*   _b,
    size_t  _cap,
    size_t* _out
)
{
    struct d_tests_ftps_toy_session* s = _v;
    unsigned char                    t = 0;
    const unsigned char*             p = NULL;
    size_t                           n;
    *_out = 0;
    for (;;)
    {
        if (s->peer_closed)
        {
            return D_SSL_ERROR_CLOSED;
        }

        if (!toy_peek(s,
                      &t,
                      &p,
                      &n))
        {
            return D_SSL_ERROR_WANT_READ;
        }

        if (t == 23)
        {
            size_t k = n - s->app_off;
            if (k > _cap)
            {
                k = _cap;
            }

            memcpy(_b,
                   p + s->app_off,
                   k);
            s->app_off += k;
            if (s->app_off == n)
            {
                toy_pop(s);
            }

            if (k)
            {
                *_out = k;

                return D_SSL_OK;
            }

            continue;
        }

        if (t == 21)
        {
            toy_pop(s);
            s->peer_closed = 1;

            return D_SSL_ERROR_CLOSED;
        }

        if (t == 22)
        {
            toy_pop(s);
            continue;
        }

        return D_SSL_ERROR_PROTOCOL;
    }
}

/*
toy_write
  Queues up to 16384 octets as one application record.
*/
static enum d_ssl_error
toy_write(
    void*       _v,
    const void* _d,
    size_t      _n,
    size_t*     _out
)
{
    struct d_tests_ftps_toy_session* s = _v;
    size_t                           k = _n > 16384 ? 16384 : _n;
    if (!toy_queue(s,
                   23,
                   _d,
                   k,
                   3))
    {
        return D_SSL_ERROR_WANT_READ;
    }

    *_out = k;

    return D_SSL_OK;
}

/*
toy_close
  Queues a close notify, so the peer sees an orderly end.
*/
static enum d_ssl_error
toy_close(
    void* _v
)
{
    struct d_tests_ftps_toy_session* s    = _v;
    unsigned char                    a[2] = { 1, 0 };
    if (!s->sent_close)
    {
        toy_queue(s,
                  21,
                  a,
                  2,
                  3);
        s->sent_close = 1;
    }

    return D_SSL_OK;
}

/*
toy_describe
  The session's protocol and suite, once the handshake is done.
*/
static void
toy_describe(
    void*              _v,
    struct d_ssl_info* _i
)
{
    struct d_tests_ftps_toy_session* s = _v;
    if (s->state != 2)
    {
        return;
    }

    _i->version   = D_SSL_VERSION_TLS1_3;
    _i->cipher_id = 0x1301;
    snprintf(_i->cipher,
             sizeof _i->cipher,
             "TLS_AES_128_GCM_SHA256");
    _i->verify  = s->server ? (uint32_t)D_SSL_VERIFY_FLAG_NOT_PERFORMED
                           : g_toy.chain_verdict;
    _i->resumed = (s->resumed != 0);

    return;
}

/*
toy_peer_name
  The name in the server's certificate, for a client, once the handshake is
done.
*/
static bool
toy_peer_name(
    void*                   _v,
    size_t                  _index,
    struct d_ssl_peer_name* _out
)
{
    struct d_tests_ftps_toy_session* s = _v;
    if (s->server || _index > 0 || s->state != 2)
    {
        return false;
    }

    _out->kind       = D_SSL_NAME_DNS;
    _out->value.data = g_toy.cert_name;
    _out->value.size = strlen(g_toy.cert_name);

    return true;
}

/*
toy_peer_certificate
  A fixed five-octet DER value, so the kernel has a certificate to
fingerprint.
*/
static bool
toy_peer_certificate(
    void*                _v,
    struct d_pack_bytes* _der
)
{
    static const unsigned char       cert[] = { 0x30, 0x03, 0x02, 0x01, 0x01 };
    struct d_tests_ftps_toy_session* s      = _v;
    if (s->server || s->state != 2)
    {
        return false;
    }

    _der->data = cert;
    _der->size = sizeof cert;

    return true;
}
const struct d_ssl_engine TOY = { "toy",
                                  D_SSL_BACKEND_OTHER,
                                  D_SSL_ENGINE_TLS1_3       |
                                  D_SSL_ENGINE_SYSTEM_TRUST |
                                  D_SSL_ENGINE_MEMORY_PEM,
                                  toy_context_create,
                                  toy_context_destroy,
                                  toy_session_create,
                                  toy_session_destroy,
                                  toy_feed,
                                  toy_drain,
                                  toy_handshake,
                                  toy_read,
                                  toy_write,
                                  toy_close,
                                  toy_describe,
                                  toy_peer_name,
                                  toy_peer_certificate,
                                  NULL };
static const char         CERT_PEM[] =
    "-----BEGIN CERTIFICATE-----\nMAMCAQE=\n-----END CERTIFICATE-----\n";
static const char KEY_PEM[] =
    "-----BEGIN PRIVATE KEY-----\nMAMCAQE=\n-----END PRIVATE KEY-----\n";

// the test server
struct d_tests_ftps_connection
{
    struct d_tcp_connection tcp;
    struct d_ssl_session    tls;
    int                     secured;
    char                    buf[4096];
    size_t                  start;
    size_t                  end;
};

char       g_big[200000];
const char HELLO[] = "Hello, FTPS!\r\nLine two\r\n";

/*
sc_tread
  The server's plain reads, over net.h, in the SSL statuses its TLS reads
give; a TLS session reads and writes the same connection itself.
*/
static enum d_ssl_error
sc_tread(
    struct d_tests_ftps_connection* _c,
    void*                           _b,
    size_t                          _cap,
    size_t*                         _n
)
{
    struct d_net_io_result r = d_net_connection_read(&_c->tcp.base,
                                                     _b,
                                                     _cap);
    *_n                       = r.count;
    if (r.error != D_NET_OK)
    {
        return D_SSL_ERROR_TRANSPORT;
    }

    return r.count ? D_SSL_OK : D_SSL_ERROR_CLOSED;
}

/*
sc_read
  A read on the server's connection: TLS's when it is secured, plain
otherwise.
*/
static enum d_ssl_error
sc_read(
    struct d_tests_ftps_connection* _c,
    void*                           _b,
    size_t                          _cap,
    size_t*                         _n
)
{
    if (_c->secured)
    {
        return d_ssl_session_read(&_c->tls,
                                  _b,
                                  _cap,
                                  _n);
    }

    return sc_tread(_c,
                    _b,
                    _cap,
                    _n);
}

/*
sc_write
  Writes the whole buffer, through TLS when the connection is secured.
*/
static int
sc_write(
    struct d_tests_ftps_connection* _c,
    const char*                     _d,
    size_t                          _n
)
{
    size_t w = 0;
    if (_c->secured)
    {
        return d_ssl_session_write_all(&_c->tls,
                                       _d,
                                       _n,
                                       &w) == D_SSL_OK;
    }

    return d_net_write_all(&_c->tcp.base,
                           _d,
                           _n,
                           NULL) == D_NET_OK;
}

/*
sc_say
  Writes one reply line.
*/
static int
sc_say(
    struct d_tests_ftps_connection* _c,
    const char*                     _text
)
{
    return sc_write(_c,
                    _text,
                    strlen(_text));
}

/*
sc_line
  Reads one command line, up to its CRLF.
*/
static int
sc_line(
    struct d_tests_ftps_connection* _c,
    char*                           _line,
    size_t                          _cap
)
{
    size_t len = 0;
    for (;;)
    {
        while (_c->start < _c->end)
        {
            char ch = _c->buf[_c->start++];
            if (ch == '\n')
            {
                if (len && _line[len - 1] == '\r')
                {
                    len--;
                }

                _line[len] = 0;

                return 1;
            }

            if (len + 1 < _cap)
            {
                _line[len++] = ch;
            }
        }

        size_t n = 0;
        if (sc_read(_c,
                    _c->buf,
                    sizeof _c->buf,
                    &n) != D_SSL_OK)
        {
            return 0;
        }

        _c->start = 0;
        _c->end   = n;
    }
}

/*
sc_secure
  Secures a server connection with the server's toy context.
*/
static int
sc_secure(
    struct d_tests_ftps_server*     _s,
    struct d_tests_ftps_connection* _c
)
{
    struct d_pack_text none = { "", 0 };
    if (d_ssl_session_init(&_c->tls,
                           &_s->tls,
                           &_c->tcp.base,
                           none) != D_SSL_OK)
    {
        return 0;
    }

    if (d_ssl_session_handshake(&_c->tls) != D_SSL_OK)
    {
        d_ssl_session_destroy(&_c->tls);

        return 0;
    }

    _c->secured = 1;

    return 1;
}

/*
sc_close
  Closes a server connection, with a close notify first when it is secured and
the end is orderly.
*/
static void
sc_close(
    struct d_tests_ftps_connection* _c,
    int                             _orderly
)
{
    if (_c->secured)
    {
        if (_orderly)
        {
            (void)d_ssl_session_shutdown(&_c->tls);
        }

        d_ssl_session_destroy(&_c->tls);
        _c->secured = 0;
    }

    d_tcp_connection_close(&_c->tcp);

    return;
}

/*
srv_data
  Opens the data connection a transfer command needs; 0 on failure, 2 when TLS
did not resume and the configuration requires it. It runs past 75 lines
because it opens the connection either way -- accepting the client's, or
connecting to its active address -- then secures it and judges resumption.
*/
static int
srv_data(
    struct d_tests_ftps_server*     _s,
    struct d_tests_ftps_connection* _d,
    struct d_tcp_listener*          _pl,
    struct d_ftp_endpoint*          _target,
    char                            _prot
)
{
    int ok = 0;
    memset(_d,
           0,
           sizeof *_d);
    d_tcp_connection_init(&_d->tcp);
    if (_target->family != D_FTP_FAMILY_NONE)
    {
        struct d_net_endpoint e = { 0 };
        struct d_pack_text    h = { _target->address,
                                    strlen(_target->address) };
        d_net_endpoint_init(&e);
        ok = d_net_endpoint_set(&e,
                                h,
                                _target->port,
                                D_NET_PROTOCOL_TCP) ==
                 D_NET_OK &&
             d_tcp_connect(&_d->tcp,
                           &e,
                           NULL) == D_NET_OK;
        _target->family = D_FTP_FAMILY_NONE;
    }
    else
    {
        ok = d_tcp_accept(_pl,
                          &_d->tcp) == D_NET_OK;
        d_tcp_listener_close(_pl);
    }

    if (!ok)
    {
        return 0;
    }

    if (_prot != 'P')
    {
        _s->data_plain++;

        return 1;
    }

    if (!sc_secure(_s,
                   _d))
    {
        sc_close(_d,
                 0);

        return 0;
    }

    struct d_ssl_info info = { 0 };
    d_ssl_session_info(&_d->tls,
                       &info);
    if (info.resumed)
    {
        _s->data_resumed++;
    }
    else
    {
        _s->data_fresh++;
    }

    if (_s->cfg.require_reuse && !info.resumed)
    {
        sc_close(_d,
                 0);

        return 2;
    }

    _s->data_tls++;

    return 1;
}

/*
srv_listen_pasv
  Listens on an ephemeral port of 127.0.0.1 for a passive data connection,
giving the port; 0 on failure.
*/
static int
srv_listen_pasv(
    struct d_tcp_listener* _pl,
    uint16_t*              _port
)
{
    struct d_net_endpoint e = { 0 };
    struct d_pack_text    h = { "127.0.0.1", 9 };
    d_net_endpoint_init(&e);
    d_tcp_listener_close(_pl);
    if (d_net_endpoint_set(&e,
                           h,
                           0,
                           D_NET_PROTOCOL_TCP) != D_NET_OK ||
        d_tcp_listen(_pl,
                     &e,
                     NULL) != D_NET_OK                     ||
        !d_tcp_listener_local_endpoint(_pl,
                                       &e))
    {
        return 0;
    }

    *_port = e.port;

    return 1;
}

/*
srv_send_file
  Sends hello.txt or big.bin over the data connection.
*/
static void
srv_send_file(
    struct d_tests_ftps_server*     _s,
    struct d_tests_ftps_connection* _ctl,
    struct d_tests_ftps_connection* _d,
    const char*                     _name
)
{
    if (!strcmp(_name,
                "big.bin"))
    {
        sc_write(_d,
                 g_big,
                 sizeof g_big);
    }
    else
    {
        sc_write(_d,
                 HELLO,
                 strlen(HELLO));
    }

    sc_close(_d,
             !_s->cfg.truncate);
    sc_say(_ctl,
           "226 Transfer complete\r\n");

    return;
}

/*
srv_receive_file
  Receives an upload, keeping as much of it as fits in the server's record.
*/
static void
srv_receive_file(
    struct d_tests_ftps_server*     _s,
    struct d_tests_ftps_connection* _ctl,
    struct d_tests_ftps_connection* _d
)
{
    char             b[1024];  // filled before it is read
    size_t           n  = 0;
    enum d_ssl_error st = D_SSL_OK;
    _s->stored_len = 0;
    while ((st = sc_read(_d,
                         b,
                         sizeof b,
                         &n)) == D_SSL_OK)
    {
        if (_s->stored_len + n <= sizeof _s->stored)
        {
            memcpy(_s->stored + _s->stored_len,
                   b,
                   n);
            _s->stored_len += n;
        }
    }

    _s->upload_clean = (st == D_SSL_ERROR_CLOSED);
    // like vsftpd, answer close_notify with close_notify -- after a pause that
    // shows whether the client waited for it: one that closed at once has left
    // an end of stream to peek at by now
    if (_d->secured && _s->upload_clean)
    {
        struct timespec pause = { 0, 100000000L };
        char            peek  = 0;
        nanosleep(&pause,
                  NULL);
        _s->upload_waited = (recv((int)d_tcp_connection_native(&_d->tcp),
                                  &peek,
                                  1,
                                  MSG_PEEK | MSG_DONTWAIT) != 0);
    }

    sc_close(_d,
             1);
    sc_say(_ctl,
           _s->upload_clean ? "226 Stored\r\n" : "426 Upload truncated\r\n");

    return;
}

/*
srv_main
  The session, on its own thread: the greeting, then one command after another
until QUIT, answered as the configuration says. It runs past 75 lines
because it dispatches every command the suites send, one branch each, as the
guides allow a dispatch.
*/
static void*
srv_main(
    void* _arg
)
{
    struct d_tests_ftps_server*    s      = _arg;
    struct d_tests_ftps_connection ctl    = { 0 };
    struct d_tests_ftps_connection d      = { 0 };
    struct d_tcp_listener          pl     = { 0 };
    struct d_ftp_endpoint          target = { .family = D_FTP_FAMILY_NONE };
    char                           line[1024];  // filled before it is read
    char                           out[256];    // filled before it is read
    char                           prot   = 'C';
    uint16_t                       port   = 0;
    memset(&ctl,
           0,
           sizeof ctl);
    d_tcp_connection_init(&ctl.tcp);
    d_tcp_listener_init(&pl);
    if (d_tcp_accept(&s->listener,
                     &ctl.tcp) != D_NET_OK)
    {
        return NULL;
    }

    if (s->cfg.implicit && !sc_secure(s,
                                      &ctl))
    {
        sc_close(&ctl,
                 0);

        return NULL;
    }

    sc_say(&ctl,
           "220 Test FTPS server ready\r\n");
    while (sc_line(&ctl,
                   line,
                   sizeof line))
    {
        const char* arg1 = strchr(line,
                                  ' ');
        arg1             = arg1 ? arg1 + 1 : "";
        if (!strncmp(line,
                     "USER ",
                     5))
        {
            if (!ctl.secured)
            {
                s->saw_user_in_clear = 1;
            }

            sc_say(&ctl,
                   "331 Password required\r\n");
        }
        else if (!strncmp(line,
                          "PASS ",
                          5))
        {
            int ok = !strcmp(arg1,
                             "secret") || !strcmp(arg1,
                                                  "anonymous@");
            s->logins += ok;
            sc_say(&ctl,
                   ok ? "230 Logged in\r\n" : "530 Login incorrect\r\n");
        }
        else if (!strcmp(line,
                         "AUTH TLS"))
        {
            if (s->cfg.refuse_auth)
            {
                sc_say(&ctl,
                       "504 AUTH not supported\r\n");
                continue;
            }

            sc_say(&ctl,
                   s->cfg.inject ? "234 Proceed\r\n230 Injected login\r\n"
                   : "234 Proceed\r\n");
            if (!sc_secure(s,
                           &ctl))
            {
                break;
            }
        }
        else if (!strcmp(line,
                         "PBSZ 0"))
        {
            sc_say(&ctl,
                   "200 PBSZ=0\r\n");
        }
        else if (!strncmp(line,
                          "PROT ",
                          5))
        {
            prot = arg1[0];
            sc_say(&ctl,
                   "200 Protection level set\r\n");
        }
        else if (!strcmp(line,
                         "FEAT"))
        {
            sc_say(&ctl,
                   "211-Features:\r\n EPSV\r\n PASV\r\n");
            if (!s->cfg.no_mlst)
            {
                sc_say(&ctl,
                       " MLST type*;size*;modify*;\r\n");
            }

            sc_say(&ctl,
                   " UTF8\r\n AUTH TLS\r\n PBSZ\r\n PROT\r\n211 End\r\n");
        }
        else if (!strcmp(line,
                         "OPTS UTF8 ON"))
        {
            sc_say(&ctl,
                   "200 UTF8 on\r\n");
        }
        else if (!strncmp(line,
                          "TYPE ",
                          5))
        {
            s->type = arg1[0];
            sc_say(&ctl,
                   "200 Type set\r\n");
        }
        else if (!strcmp(line,
                         "EPSV"))
        {
            s->epsv_count++;
            if (s->cfg.refuse_epsv || !srv_listen_pasv(&pl,
                                                       &port))
            {
                sc_say(&ctl,
                       "500 EPSV not understood\r\n");
                continue;
            }

            snprintf(out,
                     sizeof out,
                     "229 Entering Extended Passive Mode (|||%u|)\r\n",
                     port);
            sc_say(&ctl,
                   out);
        }
        else if (!strcmp(line,
                         "PASV"))
        {
            if (!srv_listen_pasv(&pl,
                                 &port))
            {
                sc_say(&ctl,
                       "425 No port\r\n");
                continue;
            }

            snprintf(out,
                     sizeof out,
                     "227 Entering Passive Mode (%s,%u,%u).\r\n",
                     s->cfg.bogus_pasv ? "10,255,255,1" : "127,0,0,1",
                     port >> 8,
                     port & 0xFF);
            sc_say(&ctl,
                   out);
        }
        else if (!strncmp(line,
                          "EPRT ",
                          5))
        {
            sc_say(&ctl,
                   d_ftp_parse_eprt(arg1,
                                    strlen(arg1),
                                    &target) == D_FTP_OK
                   ? "200 EPRT ok\r\n"
                   : "501 Bad EPRT\r\n");
        }
        else if (!strncmp(line,
                          "PORT ",
                          5))
        {
            sc_say(&ctl,
                   d_ftp_parse_port(arg1,
                                    strlen(arg1),
                                    &target) == D_FTP_OK
                   ? "200 PORT ok\r\n"
                   : "501 Bad PORT\r\n");
        }
        else if (!strncmp(line,
                          "RETR ",
                          5) || !strncmp(line,
                                         "STOR ",
                                         5) ||
                 !strcmp(line,
                         "MLSD") || !strcmp(line,
                                            "LIST"))
        {
            sc_say(&ctl,
                   "150 Opening data connection\r\n");
            int r = srv_data(s,
                             &d,
                             &pl,
                             &target,
                             prot);
            if (r == 2)
            {
                sc_say(&ctl,
                       "522 SSL connection failed: session reuse required\r\n");
                continue;
            }

            if (r == 0)
            {
                sc_say(&ctl,
                       "425 Cannot open data connection\r\n");
                continue;
            }

            if (line[0] == 'R')
            {
                srv_send_file(s,
                              &ctl,
                              &d,
                              arg1);
            }
            else if (line[0] == 'S')
            {
                srv_receive_file(s,
                                 &ctl,
                                 &d);
            }
            else
            {
                sc_say(
                    &d,
                    line[0] == 'M'
                        ? "type=file;size=24;modify=20260925123456; "
                          "hello.txt\r\ntype=dir;modify=20260101000000; pub\r\n"
                        : "-rw-r--r-- 1 u g 24 Sep 25 12:34 hello.txt\r\n");
                sc_close(&d,
                         1);
                sc_say(&ctl,
                       "226 Listing sent\r\n");
            }
        }
        else if (!strncmp(line,
                          "SIZE ",
                          5))
        {
            sc_say(&ctl,
                   "213 24\r\n");
        }
        else if (!strcmp(line,
                         "CCC"))
        {
            char   b[16];  // filled before it is read
            size_t n = 0;
            // the client's close_notify first: it sends nothing more until ours
            // arrives, so no plaintext can be read ahead with it
            sc_say(&ctl,
                   "200 CCC ok\r\n");
            s->ccc_done = (d_ssl_session_read(&ctl.tls,
                                              b,
                                              sizeof b,
                                              &n) ==
                           D_SSL_ERROR_CLOSED);
            (void)d_ssl_session_shutdown(&ctl.tls);
            d_ssl_session_destroy(&ctl.tls);
            ctl.secured = 0;
        }
        else if (!strcmp(line,
                         "QUIT"))
        {
            sc_say(&ctl,
                   "221 Goodbye\r\n");
            break;
        }
        else
        {
            sc_say(&ctl,
                   "502 Command not implemented\r\n");
        }
    }

    sc_close(&ctl,
             1);
    d_tcp_listener_close(&pl);

    return NULL;
}

/*
d_tests_ftps_server_start
  Opens the listener on an ephemeral port of 127.0.0.1, secures it with the
toy engine's server context, and serves one session on its own thread.
*/
int
d_tests_ftps_server_start(
    struct d_tests_ftps_server*       _s,
    struct d_tests_ftps_server_config _cfg
)
{
    struct d_ssl_config   c = { 0 };
    struct d_ftp_endpoint e = { .family = D_FTP_FAMILY_IPV4 };
    memset(_s,
           0,
           sizeof *_s);
    _s->cfg = _cfg;
    d_ssl_config_init(&c,
                      D_SSL_ROLE_SERVER);
    c.certificate_pem.data   = CERT_PEM;
    c.certificate_pem.length = strlen(CERT_PEM);
    c.private_key_pem.data   = KEY_PEM;
    c.private_key_pem.length = strlen(KEY_PEM);
    enum d_ssl_error st      = d_ssl_context_init(&_s->tls,
                                                  &TOY,
                                                  &c);
    if (st != D_SSL_OK)
    {
        printf("server context: %s\n",
               d_ssl_error_name(st));

        return 0;
    }

    {
        struct d_net_endpoint ne = { 0 };
        struct d_pack_text    h  = { "127.0.0.1", 9 };
        d_net_endpoint_init(&ne);
        d_tcp_listener_init(&_s->listener);
        if (d_net_endpoint_set(&ne,
                               h,
                               0,
                               D_NET_PROTOCOL_TCP) != D_NET_OK ||
            d_tcp_listen(&_s->listener,
                         &ne,
                         NULL) != D_NET_OK                     ||
            !d_tcp_listener_local_endpoint(&_s->listener,
                                           &ne))
        {
            return 0;
        }

        _s->port = ne.port;
        (void)e;
    }

    return pthread_create(&_s->thread,
                          NULL,
                          srv_main,
                          _s) == 0;
}

/*
d_tests_ftps_server_stop
  Joins the session's thread, then releases the listener and the context.
*/
void
d_tests_ftps_server_stop(
    struct d_tests_ftps_server* _s
)
{
    pthread_join(_s->thread,
                 NULL);
    d_tcp_listener_close(&_s->listener);
    d_ssl_context_destroy(&_s->tls);

    return;
}
