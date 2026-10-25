/*******************************************************************************
* djinterp [net]                                        smtp_tests_sa_loopback.c
*
* djinterp SMTP kernel tests -- over loopback TCP.
*   Live sessions: the client connects with d_smtp_client_connect to a
* server engine running on a thread of its own behind a tcp.h listener on
* 127.0.0.1. Plaintext first; then, with the OpenSSL-family engine and the
* certificates net_tests_pki.c makes for the run, STARTTLS verified against
* a name, implicit TLS verified against an address, and a certificate for
* another name refused. Then what connect refuses before it opens anything,
* how it fails when it cannot open or upgrade, and the EHLO identity it
* finds when it is given none. Without a TCP backend, or without the engine
* or the certificates, the tests that need them pass as skipped.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_loopback.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
// enable POSIX.1-2008 (pthreads) under a strict C compiler; a feature-test
// macro must precede every include, so it sits ahead of the suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./smtp_tests_sa.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <string.h>   // memcpy, strchr, strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"  // the engine
#include "../../../../inc/djinterp/net/tcp/tcp.h"          // TCP
#include "../net_tests_pki.h"                              // certificates
#include "../net_tests_sa_support.h"                       // d_tests_net_mock
// posix
#include <pthread.h>  // pthread_create, pthread_join, pthread_t


// g_pki, g_pki_made
//   variable: the run's certificates, and whether they and the engine are
// there to test with.
static struct d_tests_net_pki g_pki;
static bool                   g_pki_made = false;

/*
d_tests_sa_smtp_loopback_setup
  A TCP backend and the engine first, then the certificates the TLS tests
use.
*/
bool
d_tests_sa_smtp_loopback_setup(
    void
)
{
    g_pki_made = ( (d_tcp_available())              &&
                   (d_ssl_engine_openssl() != NULL) &&
                   (d_tests_net_pki_make(&g_pki)) );

    return g_pki_made;
}

/*
d_tests_sa_smtp_loopback_teardown
  Removes the certificates, if they were made.
*/
void
d_tests_sa_smtp_loopback_teardown(
    void
)
{
    if (g_pki_made)
    {
        d_tests_net_pki_remove(&g_pki);
        g_pki_made = false;
    }

    return;
}

// d_tests_smtp_server
//   struct: a server thread's setup and outcome: its listener, settings
// and handlers, the context it makes implicit TLS with, if any, what it
// says in place of a session, if anything, what it saw, and how its
// session ended.
struct d_tests_smtp_server
{
    struct d_tcp_listener        listener;
    struct d_smtp_server_config  config;
    struct d_smtp_server_handler handler;
    struct d_tests_smtp_inbox    inbox;
    const struct d_ssl_context*  implicit;
    const char*                  refusal;
    unsigned int                 port;
    enum d_smtp_error            result;
};

/*
d_tests_smtp_serve_implicit
  Implicit TLS: the handshake comes before the greeting (RFC 8314), and the
session runs over the TLS session's view, which says close_notify as the
session ends, where it still can.
*/
static enum d_smtp_error
d_tests_smtp_serve_implicit(
    struct d_tests_smtp_server* _server,
    struct d_tcp_connection*    _connection
)
{
    const struct d_pack_text none   = { "",
                                        0u };
    struct d_ssl_session     tls    = { .base = { .vtable = NULL } };
    enum d_smtp_error        result = D_SMTP_ERROR_TLS;

    // a session not made holds nothing to release
    if (d_ssl_session_init(&tls,
                           _server->implicit,
                           &_connection->base,
                           none) != D_SSL_OK)
    {
        return D_SMTP_ERROR_TLS;
    }

    // a session only once TLS is up
    if (d_ssl_session_handshake(&tls) == D_SSL_OK)
    {
        result = d_smtp_server_session(&tls.base,
                                       &_server->config,
                                       &_server->handler);
    }

    (void)d_ssl_session_shutdown(&tls);
    d_ssl_session_destroy(&tls);

    return result;
}

/*
d_tests_smtp_serve_thread
  Server thread: accepts one connection, and runs one session over it --
in TLS from the first byte when asked, or none at all, only a refusal --
then closes it.
*/
static void*
d_tests_smtp_serve_thread(
    void* _server
)
{
    struct d_tests_smtp_server* server     = _server;
    struct d_tcp_connection     connection = { .base = { .vtable = NULL } };

    d_tcp_connection_init(&connection);

    const enum d_net_error accepted = d_tcp_accept_wait(&server->listener,
                                                        &connection,
                                                        10000L);

    // a connection that never came is the session's end
    if (accepted != D_NET_OK)
    {
        server->result = d_smtp_error_from_net(accepted);

        return NULL;
    }

    // a refusal in place of a greeting, or a session
    if (server->refusal)
    {
        (void)d_net_write_all(&connection.base,
                              server->refusal,
                              strlen(server->refusal),
                              NULL);
        server->result = D_SMTP_ERROR_REJECTED;
    }
    else
    {
        server->result = (server->implicit)
                             ? d_tests_smtp_serve_implicit(server,
                                                           &connection)
                             : d_smtp_server_session(&connection.base,
                                                     &server->config,
                                                     &server->handler);
    }

    d_tcp_connection_close(&connection);

    return NULL;
}

/*
d_tests_smtp_server_start
  Listens on a port of 127.0.0.1 the system chooses, and starts the server
thread; false, with nothing left open, when either fails.
*/
static bool
d_tests_smtp_server_start(
    struct d_tests_smtp_server* _server,
    pthread_t*                  _thread
)
{
    struct d_net_endpoint endpoint = { .port = 0 };
    struct d_net_endpoint bound    = { .port = 0 };
    struct d_tcp_options  options  = { .io_timeout_ms = 0 };

    d_net_endpoint_init(&endpoint);
    d_net_endpoint_init(&bound);
    d_tcp_options_init(&options);
    memcpy(endpoint.host,
           "127.0.0.1",
           sizeof("127.0.0.1"));
    endpoint.protocol     = D_NET_PROTOCOL_TCP;
    options.io_timeout_ms = 10000L;
    d_tcp_listener_init(&_server->listener);
    d_tests_smtp_handler(&_server->handler,
                         &_server->inbox,
                         true);
    _server->result = D_SMTP_ERROR_STATE;

    // a listener, its port, and the thread that serves it
    if ( (d_tcp_listen(&_server->listener,
                       &endpoint,
                       &options) == D_NET_OK)    &&
         (d_tcp_listener_local_endpoint(&_server->listener,
                                        &bound)) &&
         (pthread_create(_thread,
                         NULL,
                         d_tests_smtp_serve_thread,
                         _server) == 0) )
    {
        _server->port = bound.port;

        return true;
    }

    d_tcp_listener_close(&_server->listener);

    return false;
}

/*
d_tests_smtp_server_stop
  Waits for the server thread, then closes its listener.
*/
static void
d_tests_smtp_server_stop(
    struct d_tests_smtp_server* _server,
    pthread_t                   _thread
)
{
    (void)pthread_join(_thread,
                       NULL);
    d_tcp_listener_close(&_server->listener);

    return;
}

/*
d_tests_smtp_deliver
  One client session against a started server: connect, authenticate when
asked, send to two recipients, and quit. Returns the first error; `_client`
is left closed, its reply and causes kept -- by connect itself, when that
is what fails, so callers see that it does.
*/
static enum d_smtp_error
d_tests_smtp_deliver(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options,
    bool                                _authenticate,
    const char*                         _message
)
{
    static const char* const        RECIPIENTS[] = { "bob@example.com",
                                                     "carol@example.com" };
    const struct d_smtp_credentials credentials  = { .user   = "alice",
                                                     .secret = "s3cret" };
    const struct d_smtp_envelope    envelope     =
    {
        .sender          = "alice@example.org",
        .recipients      = RECIPIENTS,
        .recipient_count = 2u
    };
    enum d_smtp_error result = d_smtp_client_connect(_client,
                                                     _options);

    // a failed connect leaves the client closed, or nothing does
    if (result != D_SMTP_OK)
    {
        return result;
    }

    if (_authenticate)
    {
        result = d_smtp_client_authenticate(_client,
                                            &credentials);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_send(_client,
                                    &envelope,
                                    _message,
                                    strlen(_message),
                                    NULL);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_quit(_client);
    }

    d_smtp_client_close(_client);

    return result;
}

/*
d_tests_smtp_options
  Options for the server on `_port`, as c.example, with ten-second limits.
*/
static void
d_tests_smtp_options(
    struct d_smtp_client_options* _options,
    unsigned int                  _port,
    enum d_smtp_security          _security,
    const struct d_ssl_context*   _tls
)
{
    d_smtp_client_options_init(_options);
    _options->host               = "127.0.0.1";
    _options->port               = _port;
    _options->security           = _security;
    _options->tls                = _tls;
    _options->helo_domain        = "client.example";
    _options->connect_timeout_ms = 10000L;
    _options->io_timeout_ms      = 10000L;

    return;
}

/*
d_tests_sa_smtp_loopback
  Tests the following:
  - a real client and server over loopback TCP, in plaintext
  - the message arrives with transparency undone and CRLF line endings
  - the server's session ends at QUIT
*/
bool
d_tests_sa_smtp_loopback(
    struct d_test_counter* _counter
)
{
    const char* const            TEST    = "loopback (plaintext)";
    struct d_tests_smtp_server   server  = { .implicit = NULL };
    struct d_smtp_client_options options = { .host = NULL };
    struct d_smtp_client         client  = { .utf8 = false };
    pthread_t                    thread;

    if (!d_tcp_available())
    {
        return d_tests_net_assert(true,
                                  TEST,
                                  "skipped: no TCP backend",
                                  _counter);
    }

    d_smtp_server_config_init(&server.config);

    if (!d_tests_smtp_server_start(&server,
                                   &thread))
    {
        return d_tests_net_assert(false,
                                  TEST,
                                  "the loopback server starts",
                                  _counter);
    }

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         server.port,
                         D_SMTP_SECURITY_NONE,
                         NULL);

    const enum d_smtp_error sent = d_tests_smtp_deliver(
                                       &client,
                                       &options,
                                       false,
                                       "Subject: loop\r\n\r\n.dot\nend");

    d_tests_smtp_server_stop(&server,
                             thread);

    return d_tests_net_assert(
               ( (sent == D_SMTP_OK)             &&
                 (server.result == D_SMTP_OK)    &&
                 (server.inbox.messages == 1u)   &&
                 (server.inbox.recipients == 2u) &&
                 (strcmp(server.inbox.domain,
                         "client.example") == 0) &&
                 (strcmp(server.inbox.data,
                         "Subject: loop\r\n\r\n.dot\r\nend\r\n") == 0) ),
               TEST,
               "a message crosses loopback intact",
               _counter);
}

/*
d_tests_smtp_contexts
  The run's two OpenSSL-family contexts: a server presenting the PKI's
certificate for localhost and 127.0.0.1, and a client trusting the PKI's CA
alone and checking names. False, with neither left to destroy, when either
cannot be made.
*/
static bool
d_tests_smtp_contexts(
    struct d_ssl_context* _server,
    struct d_ssl_context* _client
)
{
    struct d_ssl_config server = { .verify_hostname = false };
    struct d_ssl_config client = { .verify_hostname = false };

    d_ssl_config_init(&server,
                      D_SSL_ROLE_SERVER);
    d_ssl_config_init(&client,
                      D_SSL_ROLE_CLIENT);
    server.certificate_file = g_pki.server_cert;
    server.private_key_file = g_pki.server_key;
    server.verify           = D_SSL_VERIFY_NONE;
    client.use_system_trust = false;
    client.ca_file          = g_pki.ca;
    client.verify           = D_SSL_VERIFY_REQUIRED;
    client.verify_hostname  = true;

    // the server's first, and only with it the client's
    if (d_ssl_context_init(_server,
                           d_ssl_engine_openssl(),
                           &server) != D_SSL_OK)
    {
        return false;
    }

    if (d_ssl_context_init(_client,
                           d_ssl_engine_openssl(),
                           &client) != D_SSL_OK)
    {
        d_ssl_context_destroy(_server);

        return false;
    }

    return true;
}

/*
d_tests_smtp_tls_round
  One loopback session, by `_security` with the run's client context and
`_name` to check, against a server that requires TLS and AUTH: in TLS from
the first byte for implicit TLS, offering STARTTLS otherwise. The server's
outcome is left in `_server`, and the client's in `_client`.
*/
static enum d_smtp_error
d_tests_smtp_tls_round(
    struct d_tests_smtp_server* _server,
    struct d_smtp_client*       _client,
    const struct d_ssl_context* _contexts,
    enum d_smtp_security        _security,
    const char*                 _name
)
{
    const bool                   implicit = (_security ==
                                             D_SMTP_SECURITY_TLS);
    struct d_smtp_client_options options  = { .host = NULL };
    pthread_t                    thread;

    memset(&_server->inbox,
           0,
           sizeof(_server->inbox));
    d_smtp_server_config_init(&_server->config);
    _server->config.tls          = (implicit) ? NULL : &_contexts[0];
    _server->config.secure       = implicit;
    _server->config.require_tls  = true;
    _server->config.require_auth = true;
    _server->implicit            = (implicit) ? &_contexts[0] : NULL;

    if (!d_tests_smtp_server_start(_server,
                                   &thread))
    {
        return D_SMTP_ERROR_TRANSPORT;
    }

    d_tests_smtp_options(&options,
                         _server->port,
                         _security,
                         &_contexts[1]);
    options.server_name = _name;

    const enum d_smtp_error sent = d_tests_smtp_deliver(_client,
                                                        &options,
                                                        true,
                                                        "Subject: tls\r\n"
                                                        "\r\nsecret\r\n");

    d_tests_smtp_server_stop(_server,
                             thread);

    return sent;
}

/*
d_tests_sa_smtp_loopback_tls
  Tests the following:
  - STARTTLS checked against a name, then AUTH PLAIN, then delivery, to a
    server that requires both TLS and authentication
  - implicit TLS checked against an IP address
*/
bool
d_tests_sa_smtp_loopback_tls(
    struct d_test_counter* _counter
)
{
    const char* const          TEST        = "loopback (TLS)";
    struct d_tests_smtp_server server      = { .implicit = NULL };
    struct d_ssl_context       contexts[2] = { { .engine = NULL } };
    struct d_smtp_client       client      = { .utf8 = false };
    bool                       result      = true;

    if (!g_pki_made)
    {
        return d_tests_net_assert(true,
                                  TEST,
                                  "skipped: no engine, or no certificates",
                                  _counter);
    }

    if (!d_tests_net_assert(d_tests_smtp_contexts(&contexts[0],
                                                  &contexts[1]),
                            TEST,
                            "a server's context and a client's",
                            _counter))
    {
        return false;
    }

    d_smtp_client_init(&client);

    const enum d_smtp_error starttls = d_tests_smtp_tls_round(
                                           &server,
                                           &client,
                                           contexts,
                                           D_SMTP_SECURITY_STARTTLS,
                                           "localhost");

    result = d_tests_net_assert(
                 ( (starttls == D_SMTP_OK)       &&
                   (server.result == D_SMTP_OK)  &&
                   (server.inbox.messages == 1u) &&
                   (strcmp(server.inbox.user,
                           "alice") == 0)        &&
                   (strcmp(server.inbox.data,
                           "Subject: tls\r\n\r\nsecret\r\n") == 0) ),
                 TEST,
                 "STARTTLS, AUTH PLAIN and delivery",
                 _counter) && result;

    const enum d_smtp_error implicit = d_tests_smtp_tls_round(
                                           &server,
                                           &client,
                                           contexts,
                                           D_SMTP_SECURITY_TLS,
                                           NULL);

    result = d_tests_net_assert( ( (implicit == D_SMTP_OK)      &&
                                   (server.result == D_SMTP_OK) &&
                                   (server.inbox.messages == 1u) ),
                                 TEST,
                                 "implicit TLS checked against 127.0.0.1",
                                 _counter) && result;
    d_ssl_context_destroy(&contexts[1]);
    d_ssl_context_destroy(&contexts[0]);

    return result;
}

/*
d_tests_sa_smtp_loopback_mismatch
  Tests the following:
  - a certificate that does not carry the expected name fails the
    handshake, by STARTTLS and in TLS from the first byte alike: TLS,
    VERIFY kept, the client closed, and the server's session failed with
    nothing delivered
*/
bool
d_tests_sa_smtp_loopback_mismatch(
    struct d_test_counter* _counter
)
{
    const char* const          TEST        = "loopback (wrong name)";
    struct d_tests_smtp_server server      = { .implicit = NULL };
    struct d_ssl_context       contexts[2] = { { .engine = NULL } };
    struct d_smtp_client       client      = { .utf8 = false };
    bool                       refused     = true;

    if (!g_pki_made)
    {
        return d_tests_net_assert(true,
                                  TEST,
                                  "skipped: no engine, or no certificates",
                                  _counter);
    }

    if (!d_tests_net_assert(d_tests_smtp_contexts(&contexts[0],
                                                  &contexts[1]),
                            TEST,
                            "a server's context and a client's",
                            _counter))
    {
        return false;
    }

    d_smtp_client_init(&client);

    // by STARTTLS, then in TLS from the first byte
    for (size_t i = 0; i < 2u; ++i)
    {
        const enum d_smtp_error sent = d_tests_smtp_tls_round(
                                           &server,
                                           &client,
                                           contexts,
                                           (i == 1u) ? D_SMTP_SECURITY_TLS
                                                     : D_SMTP_SECURITY_STARTTLS,
                                           "wrong.example");

        refused = ( (sent == D_SMTP_ERROR_TLS)                &&
                    (client.tls_status == D_SSL_ERROR_VERIFY) &&
                    (client.state == D_SMTP_CLIENT_CLOSED)    &&
                    (server.result != D_SMTP_OK)              &&
                    (server.inbox.messages == 0u)             &&
                    (refused) );
    }

    d_ssl_context_destroy(&contexts[1]);
    d_ssl_context_destroy(&contexts[0]);

    return d_tests_net_assert(refused,
                              TEST,
                              "a certificate for another name is refused",
                              _counter);
}

/*
d_tests_sa_smtp_loopback_policy
  Tests the following:
  - STARTTLS optional upgrades when the server offers it, then
    authenticates and delivers to a server that requires both
  - plaintext never upgrades, even with a context and the offer: the
    client will not send a secret in the clear, INSECURE, and nothing is
    delivered
*/
bool
d_tests_sa_smtp_loopback_policy(
    struct d_test_counter* _counter
)
{
    const char* const          TEST        = "loopback (TLS policy)";
    struct d_tests_smtp_server server      = { .implicit = NULL };
    struct d_ssl_context       contexts[2] = { { .engine = NULL } };
    struct d_smtp_client       client      = { .utf8 = false };
    bool                       result      = true;

    if (!g_pki_made)
    {
        return d_tests_net_assert(true,
                                  TEST,
                                  "skipped: no engine, or no certificates",
                                  _counter);
    }

    if (!d_tests_net_assert(d_tests_smtp_contexts(&contexts[0],
                                                  &contexts[1]),
                            TEST,
                            "a server's context and a client's",
                            _counter))
    {
        return false;
    }

    d_smtp_client_init(&client);

    const enum d_smtp_error optional = d_tests_smtp_tls_round(
                                           &server,
                                           &client,
                                           contexts,
                                           D_SMTP_SECURITY_STARTTLS_OPTIONAL,
                                           "localhost");

    result = d_tests_net_assert( ( (optional == D_SMTP_OK)      &&
                                   (server.result == D_SMTP_OK) &&
                                   (server.inbox.messages == 1u) ),
                                 TEST,
                                 "optional, and offered: upgraded",
                                 _counter) && result;

    const enum d_smtp_error plain = d_tests_smtp_tls_round(
                                        &server,
                                        &client,
                                        contexts,
                                        D_SMTP_SECURITY_NONE,
                                        "localhost");

    result = d_tests_net_assert( ( (plain == D_SMTP_ERROR_INSECURE) &&
                                   (!client.secure)                 &&
                                   (server.inbox.messages == 0u) ),
                                 TEST,
                                 "plaintext never upgrades: INSECURE",
                                 _counter) && result;
    d_ssl_context_destroy(&contexts[1]);
    d_ssl_context_destroy(&contexts[0]);

    return result;
}

/*
d_tests_smtp_refused_connect
  Whether connect refuses `_options` as an invalid argument before opening
anything, leaving the client closed.
*/
static bool
d_tests_smtp_refused_connect(
    const struct d_smtp_client_options* _options
)
{
    struct d_smtp_client client = { .utf8 = false };

    d_smtp_client_init(&client);

    return ( (d_smtp_client_connect(&client,
                                    _options) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)        &&
             (client.state == D_SMTP_CLIENT_CLOSED) &&
             (!client.owns_tcp) );
}

// d_tests_smtp_bad_options
//   struct: what one set of options connect must refuse differs in.
struct d_tests_smtp_bad_options
{
    const char*                 host;
    unsigned int                port;
    enum d_smtp_security        security;
    const struct d_ssl_context* tls;
};

/*
d_tests_smtp_refused_options
  Every option connect refuses, one at a time: no host, an empty one, one
too long; a port past 65535; a mode past TLS; modes that need TLS without
a context, or with one never made; a server's context; and a context that
checks names with only a malformed host to check. And no client, or no
options.
*/
static bool
d_tests_smtp_refused_options(
    const struct d_ssl_context* _server,
    const struct d_ssl_context* _client
)
{
    static char                           long_host[D_NET_HOST_MAX + 2u] =
        { 0 };
    const struct d_ssl_context            empty   = { .engine = NULL };
    const struct d_tests_smtp_bad_options BAD[]   =
    {
        { NULL,         25u,    D_SMTP_SECURITY_NONE,              NULL    },
        { "",           25u,    D_SMTP_SECURITY_NONE,              NULL    },
        { long_host,    25u,    D_SMTP_SECURITY_NONE,              NULL    },
        { "127.0.0.1",  65536u, D_SMTP_SECURITY_NONE,              NULL    },
        { "127.0.0.1",  25u,    (enum d_smtp_security)9,           NULL    },
        { "127.0.0.1",  25u,    D_SMTP_SECURITY_STARTTLS,          NULL    },
        { "127.0.0.1",  25u,    D_SMTP_SECURITY_TLS,               NULL    },
        { "127.0.0.1",  25u,    D_SMTP_SECURITY_TLS,               &empty  },
        { "127.0.0.1",  25u,    D_SMTP_SECURITY_STARTTLS,          _server },
        { "127.0.0.1",  25u,    D_SMTP_SECURITY_STARTTLS_OPTIONAL, _server },
        { "not a host", 25u,    D_SMTP_SECURITY_STARTTLS,          _client }
    };
    struct d_smtp_client_options          options = { .host = NULL };
    bool                                  refused = true;

    memset(long_host,
           'a',
           D_NET_HOST_MAX + 1u);
    long_host[D_NET_HOST_MAX + 1u] = '\0';

    // each set, from the same good defaults
    for (size_t i = 0; i < sizeof(BAD) / sizeof(BAD[0]); ++i)
    {
        d_tests_smtp_options(&options,
                             BAD[i].port,
                             BAD[i].security,
                             BAD[i].tls);
        options.host = BAD[i].host;
        refused      = ( (d_tests_smtp_refused_connect(&options)) &&
                         (refused) );
    }

    return ( (refused)                            &&
             (d_tests_smtp_refused_connect(NULL)) &&
             (d_smtp_client_connect(NULL,
                                    &options) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)      &&
             (d_smtp_client_connect(NULL,
                                    NULL) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) );
}

/*
d_tests_sa_smtp_connect_arguments
  Tests the following:
  - every bad option is refused before anything is opened, one at a time,
    and so are a NULL client and NULL options
  - a client that is not closed is out of sequence
*/
bool
d_tests_sa_smtp_connect_arguments(
    struct d_test_counter* _counter
)
{
    const char* const            TEST        =
        "d_smtp_client_connect (arguments)";
    struct d_tests_net_toy       toys[2]     = { { .alerts = false } };
    struct d_ssl_context         contexts[2] = { { .engine = NULL } };
    struct d_tests_net_peer*     peer        = d_tests_smtp_peer("220 x\r\n");
    struct d_smtp_client_options options     = { .host = NULL };
    struct d_smtp_client         client      = { .utf8 = false };
    bool                         result      = true;
    const bool                   made        =
        ( (peer) &&
          (d_tests_smtp_toy_pair(contexts,
                                 toys,
                                 true)) );

    if (!d_tests_net_assert(made,
                            TEST,
                            "a peer and two toy contexts",
                            _counter))
    {
        d_tests_net_peer_free(peer);

        return false;
    }

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         25u,
                         D_SMTP_SECURITY_NONE,
                         NULL);
    result = d_tests_net_assert(d_tests_smtp_refused_options(&contexts[0],
                                                             &contexts[1]),
                                TEST,
                                "every bad option, before opening anything",
                                _counter) && result;
    result = d_tests_net_assert(
                 ( (d_smtp_client_attach(&client,
                                         d_tests_net_peer_connection(peer,
                                                                     true),
                                         false) == D_SMTP_OK)              &&
                   (d_smtp_client_connect(&client,
                                          &options) == D_SMTP_ERROR_STATE) &&
                   (client.state == D_SMTP_CLIENT_GREETING) ),
                 TEST,
                 "a client already attached is out of sequence",
                 _counter) && result;
    d_smtp_client_close(&client);
    d_ssl_context_destroy(&contexts[1]);
    d_ssl_context_destroy(&contexts[0]);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_smtp_free_port
  A port on 127.0.0.1 nothing listens on: one the system chose for a
listener, which is then closed. 0 when none could be had.
*/
static unsigned int
d_tests_smtp_free_port(
    void
)
{
    struct d_tcp_listener listener = { .protocol = D_NET_PROTOCOL_TCP };
    struct d_net_endpoint endpoint = { .port = 0 };
    unsigned int          port     = 0u;

    d_net_endpoint_init(&endpoint);
    memcpy(endpoint.host,
           "127.0.0.1",
           sizeof("127.0.0.1"));
    endpoint.protocol = D_NET_PROTOCOL_TCP;
    d_tcp_listener_init(&listener);

    // the port the system gave the listener, once it is gone
    if ( (d_tcp_listen(&listener,
                       &endpoint,
                       NULL) == D_NET_OK) &&
         (d_tcp_listener_local_endpoint(&listener,
                                        &endpoint)) )
    {
        port = endpoint.port;
    }

    d_tcp_listener_close(&listener);

    return port;
}

/*
d_tests_smtp_refused_port
  A client whose last session was refused, and whose TLS had failed, then
connects where nothing listens: CONNECT, the refusal kept as the cause,
and the earlier session's reply and status forgotten.
*/
static bool
d_tests_smtp_refused_port(
    void
)
{
    struct d_tests_net_peer*     peer = d_tests_smtp_peer("554 no\r\n");
    struct d_smtp_client_options options   = { .host = NULL };
    struct d_smtp_client         client    = { .utf8 = false };
    bool                         forgotten = false;

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         d_tests_smtp_free_port(),
                         D_SMTP_SECURITY_NONE,
                         NULL);

    // an earlier session's end, explained
    if ( (peer)                                     &&
         (options.port != 0u)                       &&
         (d_smtp_client_attach(&client,
                               d_tests_net_peer_connection(peer,
                                                           true),
                               false) == D_SMTP_OK) &&
         (d_smtp_client_greeting(&client) == D_SMTP_ERROR_REJECTED) )
    {
        d_smtp_client_close(&client);
        client.tls_status = D_SSL_ERROR_VERIFY;
        forgotten = ( (d_smtp_client_connect(&client,
                                             &options) ==
                       D_SMTP_ERROR_CONNECT)                 &&
                      (client.reader.net_error ==
                       D_NET_ERROR_CONNECTION_REFUSED)       &&
                      (client.reply.code == 0u)              &&
                      (client.tls_status == D_SSL_OK)        &&
                      (client.state == D_SMTP_CLIENT_CLOSED) &&
                      (!client.owns_tcp) );
    }

    d_tests_net_peer_free(peer);

    return forgotten;
}

/*
d_tests_smtp_policy_round
  One session against a plaintext server, by `_security` with a toy client
context: STARTTLS required must fail closed, UNSUPPORTED, while STARTTLS
optional goes on in plaintext and delivers.
*/
static bool
d_tests_smtp_policy_round(
    enum d_smtp_security        _security,
    const struct d_ssl_context* _tls,
    enum d_smtp_error           _expected
)
{
    struct d_tests_smtp_server   server  = { .implicit = NULL };
    struct d_smtp_client_options options = { .host = NULL };
    struct d_smtp_client         client  = { .utf8 = false };
    pthread_t                    thread;

    d_smtp_server_config_init(&server.config);
    server.config.allow_insecure_auth = true;

    if (!d_tests_smtp_server_start(&server,
                                   &thread))
    {
        return false;
    }

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         server.port,
                         _security,
                         _tls);

    const enum d_smtp_error sent = d_tests_smtp_deliver(&client,
                                                        &options,
                                                        false,
                                                        "x\r\n");

    d_tests_smtp_server_stop(&server,
                             thread);

    return ( (sent == _expected)                                   &&
             (client.state == D_SMTP_CLIENT_CLOSED)                &&
             (server.inbox.messages == ((sent == D_SMTP_OK) ? 1u
                                        : 0u)) &&
             (server.result == ((sent == D_SMTP_OK)
                                ? D_SMTP_OK
                                : D_SMTP_ERROR_CLOSED)) );
}

/*
d_tests_smtp_refused_greeting
  A server that answers a connection with 554 and nothing more: connect,
by STARTTLS with `_tls`, must report REJECTED, the 554 kept and the client
closed, before anything is made of STARTTLS.
*/
static bool
d_tests_smtp_refused_greeting(
    const struct d_ssl_context* _tls
)
{
    struct d_tests_smtp_server   server  = { .implicit = NULL };
    struct d_smtp_client_options options = { .host = NULL };
    struct d_smtp_client         client  = { .utf8 = false };
    pthread_t                    thread;

    d_smtp_server_config_init(&server.config);
    server.refusal = "554 5.3.2 Not now\r\n";

    if (!d_tests_smtp_server_start(&server,
                                   &thread))
    {
        return false;
    }

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         server.port,
                         D_SMTP_SECURITY_STARTTLS,
                         _tls);

    const enum d_smtp_error sent = d_tests_smtp_deliver(&client,
                                                        &options,
                                                        false,
                                                        "x\r\n");

    d_tests_smtp_server_stop(&server,
                             thread);

    return ( (sent == D_SMTP_ERROR_REJECTED)        &&
             (client.reply.code == 554u)            &&
             (client.state == D_SMTP_CLIENT_CLOSED) &&
             (!client.owns_tcp) );
}

/*
d_tests_sa_smtp_connect_failures
  Tests the following:
  - a port nothing listens on: CONNECT, the refusal kept as the cause, and
    an earlier session's reply and TLS status forgotten
  - STARTTLS required but not offered fails closed: UNSUPPORTED, the
    client closed, and the server sees it hang up
  - STARTTLS optional and not offered goes on in plaintext, and delivers
  - a greeting refused is REJECTED, the client closed, whatever the mode
*/
bool
d_tests_sa_smtp_connect_failures(
    struct d_test_counter* _counter
)
{
    const char* const      TEST   = "d_smtp_client_connect (failures)";
    struct d_tests_net_toy toy    = { .create_status = D_SSL_OK };
    struct d_ssl_context   tls    = { .engine = NULL };
    bool                   result = true;

    if (!d_tcp_available())
    {
        return d_tests_net_assert(true,
                                  TEST,
                                  "skipped: no TCP backend",
                                  _counter);
    }

    if (!d_tests_net_assert(d_tests_net_toy_context(&tls,
                                                    &toy,
                                                    D_SSL_ROLE_CLIENT,
                                                    true),
                            TEST,
                            "a toy client context",
                            _counter))
    {
        return false;
    }

    result = d_tests_net_assert(d_tests_smtp_refused_port(),
                                TEST,
                                "nothing listening: CONNECT, afresh",
                                _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_policy_round(D_SMTP_SECURITY_STARTTLS,
                                           &tls,
                                           D_SMTP_ERROR_UNSUPPORTED),
                 TEST,
                 "STARTTLS required, not offered: fails closed",
                 _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_policy_round(D_SMTP_SECURITY_STARTTLS_OPTIONAL,
                                           &tls,
                                           D_SMTP_OK),
                 TEST,
                 "STARTTLS optional, not offered: plaintext",
                 _counter) && result;
    result = d_tests_net_assert(d_tests_smtp_refused_greeting(&tls),
                                TEST,
                                "a refused greeting: REJECTED, closed",
                                _counter) && result;
    result = d_tests_net_assert( (toy.creates == 0),
                                 TEST,
                                 "no TLS session made for either",
                                 _counter) && result;
    d_ssl_context_destroy(&tls);

    return result;
}

/*
d_tests_smtp_taken
  Whether connect takes `_options`, whatever then becomes of the connection,
rather than refusing them; the client is closed after.
*/
static bool
d_tests_smtp_taken(
    const struct d_smtp_client_options* _options
)
{
    struct d_smtp_client client = { .utf8 = false };

    d_smtp_client_init(&client);

    const enum d_smtp_error result = d_smtp_client_connect(&client,
                                                           _options);

    d_smtp_client_close(&client);

    return (result != D_SMTP_ERROR_INVALID_ARGUMENT);
}

/*
d_tests_smtp_lenient
  Whether connect takes four sets of options it might be thought to refuse:
plaintext with a server's context, `_server`, which it ignores; STARTTLS
optional with none; port 65535; and a host of D_NET_HOST_MAX characters,
which no one can find. All but the third go to `_port`, where nothing
listens.
*/
static bool
d_tests_smtp_lenient(
    unsigned int                _port,
    const struct d_ssl_context* _server
)
{
    static char                  longest[D_NET_HOST_MAX + 1u] = { 0 };
    struct d_smtp_client_options options = { .host = NULL };

    memset(longest,
           'a',
           D_NET_HOST_MAX);
    d_tests_smtp_options(&options,
                         _port,
                         D_SMTP_SECURITY_NONE,
                         _server);

    const bool plain = d_tests_smtp_taken(&options);

    options.security = D_SMTP_SECURITY_STARTTLS_OPTIONAL;
    options.tls      = NULL;

    const bool optional = d_tests_smtp_taken(&options);

    options.port = 65535u;

    const bool highest = d_tests_smtp_taken(&options);

    options.port = _port;
    options.host = longest;

    return ( (plain)    &&
             (optional) &&
             (highest)  &&
             (d_tests_smtp_taken(&options)) );
}

/*
d_tests_smtp_insecure_round
  One session against a plaintext server that allows AUTH in the clear,
from a client whose options allow it, or not: allowed, alice signs in and
delivers; not, the client will not send her secret, INSECURE.
*/
static bool
d_tests_smtp_insecure_round(
    bool              _allowed,
    enum d_smtp_error _expected
)
{
    struct d_tests_smtp_server   server  = { .implicit = NULL };
    struct d_smtp_client_options options = { .host = NULL };
    struct d_smtp_client         client  = { .utf8 = false };
    pthread_t                    thread;

    d_smtp_server_config_init(&server.config);
    server.config.allow_insecure_auth = true;

    if (!d_tests_smtp_server_start(&server,
                                   &thread))
    {
        return false;
    }

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         server.port,
                         D_SMTP_SECURITY_NONE,
                         NULL);
    options.allow_insecure_auth = _allowed;

    const enum d_smtp_error sent = d_tests_smtp_deliver(&client,
                                                        &options,
                                                        true,
                                                        "x\r\n");

    d_tests_smtp_server_stop(&server,
                             thread);

    return ( (sent == _expected) &&
             (server.inbox.messages == ((sent == D_SMTP_OK) ? 1u
                                        : 0u)) );
}

/*
d_tests_smtp_defaults_are
  Whether `_options` hold d_smtp_client_options_init's defaults.
*/
static bool
d_tests_smtp_defaults_are(
    const struct d_smtp_client_options* _options
)
{
    return ( (_options->host == NULL)                                 &&
             (_options->port == 0u)                                   &&
             (_options->security == D_SMTP_SECURITY_STARTTLS)         &&
             (_options->tls == NULL)                                  &&
             (_options->server_name == NULL)                          &&
             (_options->helo_domain == NULL)                          &&
             (_options->connect_timeout_ms ==
              D_SMTP_CLIENT_CONNECT_TIMEOUT_MS)                       &&
             (_options->io_timeout_ms == D_SMTP_CLIENT_IO_TIMEOUT_MS) &&
             (!_options->allow_insecure_auth) );
}

/*
d_tests_sa_smtp_connect_options
  Tests the following:
  - the defaults: no host, the port by mode, STARTTLS required, no context
    or names, the D_SMTP_CLIENT_* timeouts, and no AUTH in the clear; NULL
    is ignored
  - a plaintext session takes a context, even a server's, and ignores it;
    STARTTLS optional needs none; 65535 is a port; and a host of
    D_NET_HOST_MAX characters is a host, which no one can find
  - AUTH goes in the clear only when the options allow it
*/
bool
d_tests_sa_smtp_connect_options(
    struct d_test_counter* _counter
)
{
    const char* const            TEST     = "d_smtp_client_connect (options)";
    struct d_tests_net_toy       toy      = { .create_status = D_SSL_OK };
    struct d_ssl_context         server   = { .engine = NULL };
    struct d_smtp_client_options defaults = { .host = "set" };
    const unsigned int           port     = d_tests_smtp_free_port();

    d_smtp_client_options_init(&defaults);
    d_smtp_client_options_init(NULL);

    bool result = d_tests_net_assert(d_tests_smtp_defaults_are(&defaults),
                                     TEST,
                                     "the defaults",
                                     _counter);

    // nowhere to connect to: the options cannot be tried
    if (port == 0u)
    {
        return d_tests_net_assert(true,
                                  TEST,
                                  "skipped: no TCP backend",
                                  _counter) && result;
    }

    if (!d_tests_net_assert(d_tests_net_toy_context(&server,
                                                    &toy,
                                                    D_SSL_ROLE_SERVER,
                                                    false),
                            TEST,
                            "a toy server context",
                            _counter))
    {
        return false;
    }

    result = d_tests_net_assert(d_tests_smtp_lenient(port,
                                                     &server),
                                TEST,
                                "a context ignored, none needed, the highest "
                                "port, the longest host",
                                _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_smtp_insecure_round(true,
                                                D_SMTP_OK)) &&
                   (d_tests_smtp_insecure_round(false,
                                                D_SMTP_ERROR_INSECURE)) ),
                 TEST,
                 "AUTH in the clear only when the options allow it",
                 _counter) && result;
    d_ssl_context_destroy(&server);

    return result;
}

// d_tests_smtp_placed
//   struct: a connection that knows only where it is: `local` is its local
// endpoint's host. Its reads and writes fail; identity never makes them.
struct d_tests_smtp_placed
{
    struct d_net_connection base;   // must stay first
    const char*             local;  // its address
};

/*
d_tests_smtp_placed_io
  Reads and writes alike: the connection is closed.
*/
static struct d_net_io_result
d_tests_smtp_placed_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    (void)_connection;
    (void)_buffer;
    (void)_size;

    return d_net_io_result_make(0u,
                                D_NET_ERROR_CLOSED);
}

static struct d_net_io_result
d_tests_smtp_placed_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    (void)_connection;
    (void)_data;
    (void)_size;

    return d_net_io_result_make(0u,
                                D_NET_ERROR_CLOSED);
}

/*
d_tests_smtp_placed_open, d_tests_smtp_placed_close
  Always open; closing does nothing.
*/
static bool
d_tests_smtp_placed_open(
    const struct d_net_connection* _connection
)
{
    (void)_connection;

    return true;
}

static void
d_tests_smtp_placed_close(
    struct d_net_connection* _connection
)
{
    (void)_connection;

    return;
}

/*
d_tests_smtp_placed_local
  The connection's address, on port 25.
*/
static bool
d_tests_smtp_placed_local(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    const struct d_tests_smtp_placed* placed =
        (const struct d_tests_smtp_placed*)_connection;

    (void)snprintf(_out->host,
                   sizeof(_out->host),
                   "%s",
                   placed->local);
    _out->port     = 25u;
    _out->protocol = D_NET_PROTOCOL_TCP;

    return true;
}

/*
PLACED_OPERATIONS
  The placed connection's table: the four required operations and its
local endpoint.
*/
static const struct d_net_connection_vtable PLACED_OPERATIONS =
{
    d_tests_smtp_placed_read,
    d_tests_smtp_placed_write,
    d_tests_smtp_placed_open,
    d_tests_smtp_placed_close,
    NULL,
    NULL,
    d_tests_smtp_placed_local,
    NULL,
    NULL
};

/*
d_tests_smtp_fqdn
  This machine's name when it is fully qualified and valid in EHLO, which
d_smtp_client_identity prefers to any address; "" otherwise.
*/
static void
d_tests_smtp_fqdn(
    char*  _out,
    size_t _capacity
)
{
    char name[D_SMTP_DOMAIN_MAX + 1u] = { 0 };

    _out[0] = '\0';

    // a name that is fully qualified, and valid as EHLO's argument
    if ( (d_tcp_host_name(name,
                          sizeof(name)) == D_NET_OK) &&
         (strchr(name,
                 '.') != NULL)                       &&
         (d_smtp_domain_is_valid(name,
                                 strlen(name),
                                 false)) )
    {
        (void)snprintf(_out,
                       _capacity,
                       "%s",
                       name);
    }

    return;
}

/*
d_tests_smtp_identity_is
  The identity found for a connection placed at `_local` -- or one that
knows no address, for NULL -- must be this machine's fully qualified name
if it has one, and otherwise `_literal`, or UNSUPPORTED for NULL.
*/
static bool
d_tests_smtp_identity_is(
    const char* _local,
    const char* _literal
)
{
    char                       fqdn[D_SMTP_DOMAIN_MAX + 1u] = { 0 };
    char                       text[D_SMTP_DOMAIN_MAX + 1u] = { 0 };
    struct d_smtp_buffer       found  = { .data     = text,
                                          .capacity = sizeof(text),
                                          .length   = 0 };
    struct d_tests_smtp_placed placed = { .local = _local };
    struct d_tests_net_mock    mock   = { .input = NULL };
    const char*                wanted = NULL;

    d_tests_smtp_fqdn(fqdn,
                      sizeof(fqdn));
    d_tests_net_mock_init(&mock,
                          true);
    (void)d_net_connection_init(&placed.base,
                                &PLACED_OPERATIONS);
    wanted = (fqdn[0] != '\0') ? fqdn : _literal;

    const enum d_smtp_error result =
        d_smtp_client_identity((_local) ? &placed.base : &mock.base,
                               &found);

    return (wanted) ? ( (result == D_SMTP_OK) &&
                        (strcmp(text,
                                wanted) == 0) )
                    : ( (result == D_SMTP_ERROR_UNSUPPORTED) &&
                        (text[0] == '\0') );
}

/*
d_tests_smtp_identity_said
  A session connected without an identity says the one found for its end
of the connection, 127.0.0.1, to the server.
*/
static bool
d_tests_smtp_identity_said(
    void
)
{
    char                         fqdn[D_SMTP_DOMAIN_MAX + 1u] = { 0 };
    struct d_tests_smtp_server   server  = { .implicit = NULL };
    struct d_smtp_client_options options = { .host = NULL };
    struct d_smtp_client         client  = { .utf8 = false };
    pthread_t                    thread;

    d_tests_smtp_fqdn(fqdn,
                      sizeof(fqdn));
    d_smtp_server_config_init(&server.config);

    if (!d_tests_smtp_server_start(&server,
                                   &thread))
    {
        return false;
    }

    d_smtp_client_init(&client);
    d_tests_smtp_options(&options,
                         server.port,
                         D_SMTP_SECURITY_NONE,
                         NULL);
    options.helo_domain = NULL;

    const enum d_smtp_error sent = d_tests_smtp_deliver(&client,
                                                        &options,
                                                        false,
                                                        "x\r\n");

    d_tests_smtp_server_stop(&server,
                             thread);

    return ( (sent == D_SMTP_OK) &&
             (strcmp(server.inbox.domain,
                     (fqdn[0] != '\0') ? fqdn
                     : "[127.0.0.1]") == 0) );
}

/*
d_tests_sa_smtp_identity
  Tests the following:
  - a fully qualified host name is preferred; failing it, an IPv4 address
    is "[a.b.c.d]" and an IPv6 one "[IPv6:...]"
  - a connection that knows no address, or one whose address makes no
    literal, leaves nothing to offer: UNSUPPORTED
  - a buffer too small is LINE_TOO_LONG, and NULL arguments are refused
  - connect without an identity says the one found to the server
*/
bool
d_tests_sa_smtp_identity(
    struct d_test_counter* _counter
)
{
    const char* const    TEST     = "d_smtp_client_identity";
    char                 tiny[4]  = { 0 };
    struct d_smtp_buffer small    = { .data     = tiny,
                                      .capacity = sizeof(tiny),
                                      .length   = 0 };
    struct d_tests_smtp_placed placed = { .local = "192.0.2.7" };
    bool                 result   = true;

    (void)d_net_connection_init(&placed.base,
                                &PLACED_OPERATIONS);
    result = d_tests_net_assert(
                 ( (d_tests_smtp_identity_is("192.0.2.7",
                                             "[192.0.2.7]"))       &&
                   (d_tests_smtp_identity_is("2001:db8::7",
                                             "[IPv6:2001:db8::7]")) ),
                 TEST,
                 "the name, or else an address literal",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_smtp_identity_is(NULL,
                                             NULL))       &&
                   (d_tests_smtp_identity_is("nowhere",
                                             NULL)) ),
                 TEST,
                 "no address, or no literal of it: UNSUPPORTED",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_smtp_client_identity(&placed.base,
                                           &small) ==
                    D_SMTP_ERROR_LINE_TOO_LONG)    &&
                   (d_smtp_client_identity(NULL,
                                           &small) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) &&
                   (d_smtp_client_identity(&placed.base,
                                           NULL) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) ),
                 TEST,
                 "too small a buffer, and NULL arguments",
                 _counter) && result;
    result = d_tests_net_assert( ( (!d_tcp_available()) ||
                                   (d_tests_smtp_identity_said()) ),
                                 TEST,
                                 "connect says the identity it found",
                                 _counter) && result;

    return result;
}
