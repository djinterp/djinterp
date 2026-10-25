/*******************************************************************************
* djinterp [net]                                     starttls_tests_sa_openssl.c
*
* Standalone tests of STARTTLS with the SSL kernel's OpenSSL-family engine.
*   A server thread and the test's own thread each hold one end of an
* AF_UNIX stream pair, adopted by tcp.c and owned by each side's session,
* and hold an SMTP-like conversation: a greeting, STARTTLS and its go-ahead,
* then one line each way under TLS. The certificates are net_tests_pki.c's,
* made once for the run by d_tests_sa_starttls_openssl_setup; without the
* engine, or without them, the runner skips these tests.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_openssl.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/
// enable POSIX.1-2008 (pthreads, socketpair) under a strict C compiler; a
// feature-test macro must precede every include, so it sits ahead of the
// suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./starttls_tests_sa.h"  // the suite
// std
#include <ctype.h>   // tolower
#include <stdio.h>   // snprintf
#include <string.h>  // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"  // the engine
#include "../net_tests_pki.h"                              // certificates
// posix
#include <pthread.h>  // pthread_create, pthread_join, pthread_t
#include <unistd.h>   // close, read, write


// g_pki, g_ready
//   variable: the run's certificates, and whether they and the engine are
// there to test with.
static struct d_tests_net_pki g_pki;
static bool                   g_pki_made = false;
static bool                   g_ready    = false;

/*
d_tests_sa_starttls_openssl_setup
  The engine first, then the certificates every test below uses.
*/
bool
d_tests_sa_starttls_openssl_setup(
    void
)
{
    g_pki_made = ( (d_ssl_engine_openssl() != NULL) &&
                   (d_tests_net_pki_make(&g_pki)) );
    g_ready    = g_pki_made;

    return g_ready;
}

/*
d_tests_sa_starttls_openssl_teardown
  Removes the certificates, if they were made.
*/
void
d_tests_sa_starttls_openssl_teardown(
    void
)
{
    if (g_pki_made)
    {
        d_tests_net_pki_remove(&g_pki);
        g_pki_made = false;
    }

    g_ready = false;

    return;
}

// d_tests_starttls_side
//   struct: what one context is made from: its role, whether it verifies its
// peer, the files it loads, each of which may be NULL, and its version
// floor, D_SSL_VERSION_NONE for the default.
struct d_tests_starttls_side
{
    enum d_starttls_role role;
    bool                 verify;
    const char*          ca_file;
    const char*          cert_file;
    const char*          key_file;
    enum d_ssl_version   min_version;
};

/*
d_tests_starttls_context
  An OpenSSL-family context made from `_side`. Only the run's CA is trusted:
the system store is left out, so a certificate from anywhere else fails.
*/
static bool
d_tests_starttls_context(
    struct d_ssl_context*               _out,
    const struct d_tests_starttls_side* _side
)
{
    const bool          server = (_side->role == D_STARTTLS_ROLE_SERVER);
    struct d_ssl_config config = { .verify_hostname = false };

    d_ssl_config_init(&config,
                      (server) ? D_SSL_ROLE_SERVER
                      : D_SSL_ROLE_CLIENT);
    config.use_system_trust = false;
    config.ca_file          = _side->ca_file;
    config.certificate_file = _side->cert_file;
    config.private_key_file = _side->key_file;
    config.verify           = (_side->verify) ? D_SSL_VERIFY_REQUIRED
                                              : D_SSL_VERIFY_NONE;
    config.verify_hostname  = ( (!server) &&
                                (_side->verify) );

    // a floor of its own, or the configured default
    if (_side->min_version != D_SSL_VERSION_NONE)
    {
        config.min_version = _side->min_version;
    }

    return (d_ssl_context_init(_out,
                               d_ssl_engine_openssl(),
                               &config) == D_SSL_OK);
}

/*
d_tests_starttls_verifier
  A client that verifies against the run's CA.
*/
static struct d_tests_starttls_side
d_tests_starttls_verifier(
    void
)
{
    const struct d_tests_starttls_side side =
    {
        .role        = D_STARTTLS_ROLE_CLIENT,
        .verify      = true,
        .ca_file     = g_pki.ca,
        .cert_file   = NULL,
        .key_file    = NULL,
        .min_version = D_SSL_VERSION_NONE
    };

    return side;
}

/*
d_tests_starttls_presenter
  A server that presents `_cert_file` and `_key_file` and asks for no client
certificate.
*/
static struct d_tests_starttls_side
d_tests_starttls_presenter(
    const char* _cert_file,
    const char* _key_file
)
{
    const struct d_tests_starttls_side side =
    {
        .role        = D_STARTTLS_ROLE_SERVER,
        .verify      = false,
        .ca_file     = NULL,
        .cert_file   = _cert_file,
        .key_file    = _key_file,
        .min_version = D_SSL_VERSION_NONE
    };

    return side;
}

/*
d_tests_starttls_mentions
  Whether `_text` contains `_word`, ignoring case; false for NULL text.
*/
static bool
d_tests_starttls_mentions(
    const char* _text,
    const char* _word
)
{
    const size_t length = strlen(_word);

    if (!_text)
    {
        return false;
    }

    // every position the word could start at
    for (size_t at = 0; _text[at] != '\0'; ++at)
    {
        size_t i = 0;

        while ( (i < length)            &&
                (_text[at + i] != '\0') &&
                (tolower((unsigned char)_text[at + i]) ==
                 tolower((unsigned char)_word[i])) )
        {
            ++i;
        }

        if (i == length)
        {
            return true;
        }
    }

    return false;
}

// d_tests_starttls_server
//   struct: the server thread's orders, and what it saw.
struct d_tests_starttls_server
{
    struct d_tcp_connection     connection;  // its end; its session's
    int                         socket;      // that end's socket
    const struct d_ssl_context* tls;         // its context
    bool                        implicit;    // TLS from the first byte
    const char*                 go_ahead;    // sent as is after STARTTLS
    bool                        truncate;    // end without close_notify
    enum d_starttls_error       upgrade;     // what its upgrade reported
    enum d_starttls_error       read;        // what its secure read reported
    char                        line[128];   // the line that read returned
};

// d_tests_starttls_client
//   struct: the client's orders, and what it saw.
struct d_tests_starttls_client
{
    struct d_tcp_connection     connection;       // its end; its session's
    int                         socket;           // that end's socket
    const struct d_ssl_context* tls;              // its context
    const char*                 server_name;      // whom it expects
    bool                        implicit;         // TLS from the first byte
    bool                        reject_leftover;  // fail, not discard
    size_t                      exact;            // go-ahead bytes; 0: line
    enum d_starttls_error       upgrade;          // negotiation's outcome
    enum d_starttls_error       reply;            // EHLO's answer, read
    enum d_starttls_error       end;              // the read after it
    const char*                 detail;           // the session's detail
    size_t                      discarded;        // dropped at the switch
    unsigned int                epoch;            // after the upgrade
};

/*
d_tests_starttls_session_on
  Opens `_session` over a TCP connection adopting `_socket`, which the
session then owns, with `_options`; on failure the connection is closed
here.
*/
static bool
d_tests_starttls_session_on(
    struct d_starttls_session* _session,
    struct d_tcp_connection*   _connection,
    int                        _socket,
    struct d_starttls_options* _options
)
{
    _options->owns_connection = true;

    if ( (d_tests_starttls_adopt(_connection,
                                 _socket)) &&
         (d_starttls_init(_session,
                          _options,
                          &_connection->base) == D_STARTTLS_OK) )
    {
        return true;
    }

    d_tcp_connection_close(_connection);

    return false;
}

/*
d_tests_starttls_serve
  The server thread: greets, waits for STARTTLS, sends the go-ahead it was
given, and upgrades -- or upgrades at once for implicit TLS -- then reads a
line, answers it, and closes, with close_notify unless told to truncate.
*/
static void*
d_tests_starttls_serve(
    void* _argument
)
{
    struct d_tests_starttls_server* server  = _argument;
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_starttls_options       options =
        d_starttls_options_default(D_STARTTLS_ROLE_SERVER);
    const char*                     line   = NULL;
    size_t                          length = 0;

    options.tls = server->tls;

    if (!d_tests_starttls_session_on(&session,
                                     &server->connection,
                                     server->socket,
                                     &options))
    {
        return NULL;
    }

    // the plaintext exchange, unless TLS comes first
    if ( (server->implicit) ||
         ( (d_starttls_send_line(&session,
                                 "220 localhost ESMTP") == D_STARTTLS_OK) &&
           (d_tests_starttls_line_is(&session,
                                     "STARTTLS"))                         &&
           (d_starttls_send(&session,
                            server->go_ahead,
                            strlen(server->go_ahead)) == D_STARTTLS_OK) ) )
    {
        server->upgrade = d_starttls_upgrade(&session);
    }

    // one line each way under TLS, then the end
    if (server->upgrade == D_STARTTLS_OK)
    {
        server->read = d_starttls_recv_line(&session,
                                            &line,
                                            &length);

        if (server->read == D_STARTTLS_OK)
        {
            (void)snprintf(server->line,
                           sizeof(server->line),
                           "%s",
                           line);
            (void)d_starttls_send_line(&session,
                                       "250 secure");
        }

        if (!server->truncate)
        {
            (void)d_starttls_close(&session);
        }
    }

    d_starttls_destroy(&session);

    return NULL;
}

/*
d_tests_starttls_exchange
  The client's secure half: sends a line, reads the answer, which must be
"250 secure", and reads once more, which should find the server's close.
*/
static void
d_tests_starttls_exchange(
    struct d_tests_starttls_client* _client,
    struct d_starttls_session*      _session
)
{
    const char* line   = NULL;
    size_t      length = 0;

    _client->reply = d_starttls_send_line(_session,
                                          "EHLO secure");

    if (_client->reply == D_STARTTLS_OK)
    {
        _client->reply = d_starttls_recv_line(_session,
                                              &line,
                                              &length);
    }

    if ( (_client->reply == D_STARTTLS_OK) &&
         (strcmp(line,
                 "250 secure") != 0) )
    {
        _client->reply = D_STARTTLS_ERROR_PROTOCOL;
    }

    if (_client->reply == D_STARTTLS_OK)
    {
        _client->end = d_starttls_recv_line(_session,
                                            &line,
                                            &length);
    }

    return;
}

/*
d_tests_starttls_converse
  The client's side: reads the greeting and negotiates -- or upgrades at
once for implicit TLS -- then, once secure, exchanges a line each way.
*/
static void
d_tests_starttls_converse(
    struct d_tests_starttls_client* _client
)
{
    struct d_starttls_session    session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_smtp smtp    =
    {
        .request_status = D_STARTTLS_OK,
        .exact          = _client->exact
    };
    struct d_starttls_options    options =
        d_tests_starttls_options(D_STARTTLS_ROLE_CLIENT,
                                 D_STARTTLS_POLICY_REQUIRED,
                                 _client->tls);

    options.server_name     = _client->server_name;
    options.reject_leftover = _client->reject_leftover;

    if (!d_tests_starttls_session_on(&session,
                                     &_client->connection,
                                     _client->socket,
                                     &options))
    {
        return;
    }

    if (_client->implicit)
    {
        _client->upgrade = d_starttls_upgrade(&session);
    }
    else if (d_tests_starttls_line_is(&session,
                                      "220 localhost ESMTP"))
    {
        _client->upgrade =
            d_starttls_negotiate(&session,
                                 &d_tests_starttls_smtp_negotiator,
                                 &smtp,
                                 true);
    }

    _client->epoch     = d_starttls_epoch(&session);
    _client->discarded = session.discarded;

    if (_client->upgrade == D_STARTTLS_OK)
    {
        d_tests_starttls_exchange(_client,
                                  &session);
    }

    _client->detail = d_starttls_detail(&session);
    (void)d_starttls_close(&session);
    d_starttls_destroy(&session);

    return;
}

/*
d_tests_starttls_meet
  Runs `_server` on a thread and `_client` on this one, over a fresh socket
pair, and waits for the server to finish. Every outcome starts out as
D_TESTS_STARTTLS_UNREACHED.
*/
static bool
d_tests_starttls_meet(
    struct d_tests_starttls_client* _client,
    struct d_tests_starttls_server* _server
)
{
    int       sockets[2] = { -1, -1 };
    pthread_t thread;

    _client->upgrade = D_TESTS_STARTTLS_UNREACHED;
    _client->reply   = D_TESTS_STARTTLS_UNREACHED;
    _client->end     = D_TESTS_STARTTLS_UNREACHED;
    _client->detail  = NULL;
    _server->upgrade = D_TESTS_STARTTLS_UNREACHED;
    _server->read    = D_TESTS_STARTTLS_UNREACHED;
    _server->line[0] = '\0';

    if (!d_tests_starttls_socket_pair(sockets))
    {
        return false;
    }

    _client->socket = sockets[0];
    _server->socket = sockets[1];

    if (pthread_create(&thread,
                       NULL,
                       d_tests_starttls_serve,
                       _server) != 0)
    {
        (void)close(sockets[0]);
        (void)close(sockets[1]);

        return false;
    }

    d_tests_starttls_converse(_client);

    return (pthread_join(thread,
                         NULL) == 0);
}

/*
d_tests_starttls_parties
  A client with `_client_tls` expecting `_server_name`, and a server with
`_server_tls` that sends the go-ahead an honest server sends.
*/
static void
d_tests_starttls_parties(
    struct d_tests_starttls_client* _client,
    struct d_tests_starttls_server* _server,
    const struct d_ssl_context*     _client_tls,
    const struct d_ssl_context*     _server_tls,
    const char*                     _server_name
)
{
    const struct d_tests_starttls_client client =
    {
        .socket          = -1,
        .tls             = _client_tls,
        .server_name     = _server_name,
        .implicit        = false,
        .reject_leftover = true,
        .exact           = 0
    };
    const struct d_tests_starttls_server server =
    {
        .socket   = -1,
        .tls      = _server_tls,
        .implicit = false,
        .go_ahead = "220 Go ahead\r\n",
        .truncate = false
    };

    *_client = client;
    *_server = server;

    return;
}

/*
d_tests_starttls_completed
  Whether both sides upgraded and exchanged their lines, and the client saw
the server's close_notify.
*/
static bool
d_tests_starttls_completed(
    const struct d_tests_starttls_client* _client,
    const struct d_tests_starttls_server* _server
)
{
    return ( (_client->upgrade == D_STARTTLS_OK)       &&
             (_client->epoch == 1)                     &&
             (_client->reply == D_STARTTLS_OK)         &&
             (_client->end == D_STARTTLS_ERROR_CLOSED) &&
             (_server->upgrade == D_STARTTLS_OK)       &&
             (_server->read == D_STARTTLS_OK)          &&
             (strcmp(_server->line,
                     "EHLO secure") == 0) );
}

/*
d_tests_starttls_pair_of
  A client verifying against the run's CA, and a server presenting
`_cert_file` and `_key_file` with the floor `_min_version`
(D_SSL_VERSION_NONE for the default); both or neither are made.
*/
static bool
d_tests_starttls_pair_of(
    struct d_ssl_context* _client_tls,
    struct d_ssl_context* _server_tls,
    const char*           _cert_file,
    const char*           _key_file,
    enum d_ssl_version    _min_version
)
{
    const struct d_tests_starttls_side verifier = d_tests_starttls_verifier();
    struct d_tests_starttls_side       server   =
        d_tests_starttls_presenter(_cert_file,
                                   _key_file);

    server.min_version = _min_version;

    if ( (!g_ready) ||
         (!d_tests_starttls_context(_client_tls,
                                    &verifier)) )
    {
        return false;
    }

    // half a pair is no pair
    if (!d_tests_starttls_context(_server_tls,
                                  &server))
    {
        d_ssl_context_destroy(_client_tls);

        return false;
    }

    return true;
}

/*
d_tests_starttls_conversation
  One conversation between the usual pair of contexts, the server
presenting `_cert_file` and `_key_file` and the client expecting
`_server_name`; `_client` and `_server` receive what each saw. False if the
contexts or the meeting could not be set up.
*/
static bool
d_tests_starttls_conversation(
    struct d_tests_starttls_client* _client,
    struct d_tests_starttls_server* _server,
    const char*                     _cert_file,
    const char*                     _key_file,
    const char*                     _server_name
)
{
    struct d_ssl_context client_tls = { .engine = NULL };
    struct d_ssl_context server_tls = { .engine = NULL };

    if (!d_tests_starttls_pair_of(&client_tls,
                                  &server_tls,
                                  _cert_file,
                                  _key_file,
                                  D_SSL_VERSION_NONE))
    {
        return false;
    }

    d_tests_starttls_parties(_client,
                             _server,
                             &client_tls,
                             &server_tls,
                             _server_name);

    const bool met = d_tests_starttls_meet(_client,
                                           _server);

    d_ssl_context_destroy(&client_tls);
    d_ssl_context_destroy(&server_tls);

    return met;
}

/*
d_tests_sa_starttls_openssl_starttls
  Tests the following:
  - a greeting, STARTTLS, and the go-ahead in plaintext; then a verified
    handshake for localhost, and a line each way under TLS
  - the server's close_notify reaches the client as an orderly close
*/
bool
d_tests_sa_starttls_openssl_starttls(
    struct d_test_counter* _counter
)
{
    const char* const              TEST   = "OpenSSL STARTTLS";
    struct d_tests_starttls_client client = { .socket = -1 };
    struct d_tests_starttls_server server = { .socket = -1 };

    const bool met = d_tests_starttls_conversation(&client,
                                                   &server,
                                                   g_pki.server_cert,
                                                   g_pki.server_key,
                                                   "localhost");

    return d_tests_net_assert( (met) &&
                               (d_tests_starttls_completed(&client,
                                                           &server)),
                               TEST,
                               "upgraded, verified, a line each way, closed",
                               _counter);
}

/*
d_tests_sa_starttls_openssl_implicit
  Tests the following:
  - implicit TLS: both sides upgrade first thing
  - an IP literal is verified against the certificate's addresses
  - a server whose floor is TLS 1.3 still meets a default client
*/
bool
d_tests_sa_starttls_openssl_implicit(
    struct d_test_counter* _counter
)
{
    const char* const              TEST       = "OpenSSL implicit TLS";
    struct d_ssl_context           client_tls = { .engine = NULL };
    struct d_ssl_context           server_tls = { .engine = NULL };
    struct d_tests_starttls_client client     = { .socket = -1 };
    struct d_tests_starttls_server server     = { .socket = -1 };
    bool                           met        = false;

    if (d_tests_starttls_pair_of(&client_tls,
                                 &server_tls,
                                 g_pki.server_cert,
                                 g_pki.server_key,
                                 D_SSL_VERSION_TLS1_3))
    {
        d_tests_starttls_parties(&client,
                                 &server,
                                 &client_tls,
                                 &server_tls,
                                 "127.0.0.1");
        client.implicit = true;
        server.implicit = true;
        met             = d_tests_starttls_meet(&client,
                                                &server);
        d_ssl_context_destroy(&client_tls);
        d_ssl_context_destroy(&server_tls);
    }

    return d_tests_net_assert( (met) &&
                               (d_tests_starttls_completed(&client,
                                                           &server)),
                               TEST,
                               "implicit TLS to 127.0.0.1 at a 1.3 floor",
                               _counter);
}

/*
d_tests_starttls_refuses
  A conversation in which the client must refuse the server's certificate:
true if the client reports D_STARTTLS_ERROR_VERIFY with a detail that
mentions `_word`, at epoch 0, and the server's upgrade fails too.
*/
static bool
d_tests_starttls_refuses(
    const char* _cert_file,
    const char* _key_file,
    const char* _server_name,
    const char* _word
)
{
    struct d_tests_starttls_client client = { .socket = -1 };
    struct d_tests_starttls_server server = { .socket = -1 };

    return ( (d_tests_starttls_conversation(&client,
                                            &server,
                                            _cert_file,
                                            _key_file,
                                            _server_name)) &&
             (client.upgrade == D_STARTTLS_ERROR_VERIFY)   &&
             (client.epoch == 0)                           &&
             (d_tests_starttls_mentions(client.detail,
                                        _word))            &&
             (server.upgrade != D_STARTTLS_OK)             &&
             (server.upgrade != D_TESTS_STARTTLS_UNREACHED) );
}

/*
d_tests_sa_starttls_openssl_verify
  Tests the following:
  - a certificate for another name, from an untrusted CA, or expired, is
    refused with D_STARTTLS_ERROR_VERIFY and the reason as its detail; so is
    an IP literal the certificate does not carry
*/
bool
d_tests_sa_starttls_openssl_verify(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "OpenSSL verification";
    bool              result = true;

    const bool named   = d_tests_starttls_refuses(g_pki.server_cert,
                                                  g_pki.server_key,
                                                  "wrong.example",
                                                  "mismatch");
    const bool rogue   = d_tests_starttls_refuses(g_pki.rogue_cert,
                                                  g_pki.rogue_key,
                                                  "localhost",
                                                  "issuer");
    const bool expired = d_tests_starttls_refuses(g_pki.expired_cert,
                                                  g_pki.expired_key,
                                                  "localhost",
                                                  "expired");
    const bool address = d_tests_starttls_refuses(g_pki.server_cert,
                                                  g_pki.server_key,
                                                  "127.0.0.2",
                                                  "mismatch");

    result = d_tests_net_assert(named,
                                TEST,
                                "a certificate for another name",
                                _counter) && result;
    result = d_tests_net_assert(rogue,
                                TEST,
                                "a certificate from an untrusted CA",
                                _counter) && result;
    result = d_tests_net_assert(expired,
                                TEST,
                                "an expired certificate",
                                _counter) && result;
    result = d_tests_net_assert(address,
                                TEST,
                                "an IP address the certificate lacks",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_openssl_unverified
  Tests the following:
  - a client that does not verify, and names no server, accepts a
    certificate from an untrusted CA
*/
bool
d_tests_sa_starttls_openssl_unverified(
    struct d_test_counter* _counter
)
{
    const char* const                  TEST       = "OpenSSL unverified";
    const struct d_tests_starttls_side trusting   =
    {
        .role        = D_STARTTLS_ROLE_CLIENT,
        .verify      = false,
        .ca_file     = NULL,
        .cert_file   = NULL,
        .key_file    = NULL,
        .min_version = D_SSL_VERSION_NONE
    };
    const struct d_tests_starttls_side rogue      =
        d_tests_starttls_presenter(g_pki.rogue_cert,
                                   g_pki.rogue_key);
    struct d_ssl_context               client_tls = { .engine = NULL };
    struct d_ssl_context               server_tls = { .engine = NULL };
    struct d_tests_starttls_client     client     = { .socket = -1 };
    struct d_tests_starttls_server     server     = { .socket = -1 };
    bool                               accepted   = false;

    if ( (g_ready)                             &&
         (d_tests_starttls_context(&client_tls,
                                   &trusting)) &&
         (d_tests_starttls_context(&server_tls,
                                   &rogue)) )
    {
        d_tests_starttls_parties(&client,
                                 &server,
                                 &client_tls,
                                 &server_tls,
                                 NULL);
        accepted = ( (d_tests_starttls_meet(&client,
                                            &server)) &&
                     (d_tests_starttls_completed(&client,
                                                 &server)) );
    }

    d_ssl_context_destroy(&client_tls);
    d_ssl_context_destroy(&server_tls);

    return d_tests_net_assert(accepted,
                              TEST,
                              "a client that does not verify accepts it",
                              _counter);
}

/*
d_tests_sa_starttls_openssl_truncation
  Tests the following:
  - a server that ends without close_notify leaves the client with a fatal
    D_STARTTLS_ERROR_TRUNCATED, after the data that did arrive
*/
bool
d_tests_sa_starttls_openssl_truncation(
    struct d_test_counter* _counter
)
{
    const char* const              TEST       = "OpenSSL truncation";
    struct d_ssl_context           client_tls = { .engine = NULL };
    struct d_ssl_context           server_tls = { .engine = NULL };
    struct d_tests_starttls_client client     = { .socket = -1 };
    struct d_tests_starttls_server server     = { .socket = -1 };
    bool                           met        = false;

    if (d_tests_starttls_pair_of(&client_tls,
                                 &server_tls,
                                 g_pki.server_cert,
                                 g_pki.server_key,
                                 D_SSL_VERSION_NONE))
    {
        d_tests_starttls_parties(&client,
                                 &server,
                                 &client_tls,
                                 &server_tls,
                                 "localhost");
        server.truncate = true;
        met             = d_tests_starttls_meet(&client,
                                                &server);
        d_ssl_context_destroy(&client_tls);
        d_ssl_context_destroy(&server_tls);
    }

    return d_tests_net_assert( (met)                                      &&
                               (client.upgrade == D_STARTTLS_OK)          &&
                               (client.reply == D_STARTTLS_OK)            &&
                               (client.end == D_STARTTLS_ERROR_TRUNCATED) &&
                               (server.read == D_STARTTLS_OK),
                               TEST,
                               "an end without close_notify is truncation",
                               _counter);
}

/*
d_tests_starttls_injected
  A conversation with a server that appends plaintext to its go-ahead; the
client reads the go-ahead as a line, or as exactly its bytes when `_exact`,
and refuses or drops what follows as `_reject` says.
*/
static bool
d_tests_starttls_injected(
    struct d_tests_starttls_client* _client,
    struct d_tests_starttls_server* _server,
    bool                            _exact,
    bool                            _reject
)
{
    struct d_ssl_context client_tls = { .engine = NULL };
    struct d_ssl_context server_tls = { .engine = NULL };
    bool                 met        = false;

    if (d_tests_starttls_pair_of(&client_tls,
                                 &server_tls,
                                 g_pki.server_cert,
                                 g_pki.server_key,
                                 D_SSL_VERSION_NONE))
    {
        d_tests_starttls_parties(_client,
                                 _server,
                                 &client_tls,
                                 &server_tls,
                                 "localhost");
        _server->go_ahead        = "220 Go ahead\r\n250 injected\r\n";
        _client->exact           = (_exact) ? 14 : 0;
        _client->reject_leftover = _reject;
        met                      = d_tests_starttls_meet(_client,
                                                         _server);
        d_ssl_context_destroy(&client_tls);
        d_ssl_context_destroy(&server_tls);
    }

    return met;
}

/*
d_tests_sa_starttls_openssl_injection
  Tests the following:
  - a server that appends plaintext to its go-ahead is refused with
    D_STARTTLS_ERROR_INJECTION, whether the client's reply read buffered it
    or it still waits in the socket, which tcp.c counts
  - with reject_leftover off, the client drops it, counts it, and completes
    the handshake
*/
bool
d_tests_sa_starttls_openssl_injection(
    struct d_test_counter* _counter
)
{
    const char* const              TEST   = "OpenSSL injection";
    struct d_tests_starttls_client client = { .socket = -1 };
    struct d_tests_starttls_server server = { .socket = -1 };
    bool                           result = true;

    const bool buffered = ( (d_tests_starttls_injected(&client,
                                                       &server,
                                                       false,
                                                       true))              &&
                            (client.upgrade == D_STARTTLS_ERROR_INJECTION) &&
                            (server.upgrade != D_STARTTLS_OK) );
    const bool waiting  = ( (d_tests_starttls_injected(&client,
                                                       &server,
                                                       true,
                                                       true))              &&
                            (client.upgrade == D_STARTTLS_ERROR_INJECTION) &&
                            (server.upgrade != D_STARTTLS_OK) );
    const bool dropped  = ( (d_tests_starttls_injected(&client,
                                                       &server,
                                                       true,
                                                       false))    &&
                            (d_tests_starttls_completed(&client,
                                                        &server)) &&
                            (client.discarded == 14) );

    result = d_tests_net_assert(buffered,
                                TEST,
                                "appended plaintext, buffered, is refused",
                                _counter) && result;
    result = d_tests_net_assert(waiting,
                                TEST,
                                "appended plaintext in the socket too",
                                _counter) && result;
    result = d_tests_net_assert(dropped,
                                TEST,
                                "or dropped and counted when allowed",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_read_to_end
  As the peer, reads `_socket` until it ends, keeping the first bytes in
`_out`; false if a read fails or times out instead.
*/
static bool
d_tests_starttls_read_to_end(
    int    _socket,
    char*  _out,
    size_t _capacity
)
{
    size_t kept       = 0;
    char   chunk[256] = { 0 };

    // until the end of the stream
    for (;;)
    {
        const ssize_t count = read(_socket,
                                   chunk,
                                   sizeof(chunk));

        if (count == 0)
        {
            _out[kept] = '\0';

            return true;
        }

        if (count < 0)
        {
            return false;
        }

        for (ssize_t i = 0; (i < count) && ((kept + 1) < _capacity); ++i)
        {
            _out[kept++] = chunk[i];
        }
    }
}

/*
d_tests_starttls_attack
  Plays a client that pipelines a command after STARTTLS against `_server`,
reading everything the server sends until it hangs up into `_heard`.
*/
static bool
d_tests_starttls_attack(
    struct d_tests_starttls_server* _server,
    char*                           _heard,
    size_t                          _capacity
)
{
    const char* const attack     = "STARTTLS\r\nMAIL FROM:<evil>\r\n";
    int               sockets[2] = { -1, -1 };
    pthread_t         thread;
    bool              played     = false;

    if (!d_tests_starttls_socket_pair(sockets))
    {
        return false;
    }

    _server->socket = sockets[1];

    if (pthread_create(&thread,
                       NULL,
                       d_tests_starttls_serve,
                       _server) == 0)
    {
        played = ( (write(sockets[0],
                          attack,
                          strlen(attack)) == (ssize_t)strlen(attack)) &&
                   (d_tests_starttls_read_to_end(sockets[0],
                                                 _heard,
                                                 _capacity)) );
        played = ( (pthread_join(thread,
                                 NULL) == 0) &&
                   (played) );
    }
    else
    {
        (void)close(sockets[1]);
    }

    (void)close(sockets[0]);

    return played;
}

/*
d_tests_sa_starttls_openssl_pipelined
  Tests the following:
  - a server refuses a command pipelined after STARTTLS, the injection of
    CVE-2011-0411, after sending its go-ahead and nothing more
*/
bool
d_tests_sa_starttls_openssl_pipelined(
    struct d_test_counter* _counter
)
{
    const char* const              TEST       = "OpenSSL pipelined command";
    struct d_ssl_context           client_tls = { .engine = NULL };
    struct d_ssl_context           server_tls = { .engine = NULL };
    struct d_tests_starttls_client client     = { .socket = -1 };
    struct d_tests_starttls_server server     = { .socket = -1 };
    char                           heard[256] = { 0 };
    bool                           refused    = false;

    if (d_tests_starttls_pair_of(&client_tls,
                                 &server_tls,
                                 g_pki.server_cert,
                                 g_pki.server_key,
                                 D_SSL_VERSION_NONE))
    {
        d_tests_starttls_parties(&client,
                                 &server,
                                 &client_tls,
                                 &server_tls,
                                 "localhost");
        server.upgrade = D_TESTS_STARTTLS_UNREACHED;
        refused        = ( (d_tests_starttls_attack(&server,
                                                    heard,
                                                    sizeof(heard))) &&
                           (strcmp(heard,
                                   "220 localhost ESMTP\r\n"
                                   "220 Go ahead\r\n") == 0)        &&
                           (server.upgrade == D_STARTTLS_ERROR_INJECTION) );
        d_ssl_context_destroy(&client_tls);
        d_ssl_context_destroy(&server_tls);
    }

    return d_tests_net_assert(refused,
                              TEST,
                              "a server refuses a pipelined command",
                              _counter);
}

/*
d_tests_starttls_demanding
  A server that verifies its clients against the run's CA, and two clients:
one presenting the run's client certificate, one presenting none. All three
or none are made.
*/
static bool
d_tests_starttls_demanding(
    struct d_ssl_context* _server_tls,
    struct d_ssl_context* _holder_tls,
    struct d_ssl_context* _stranger_tls
)
{
    const struct d_tests_starttls_side strict   =
    {
        .role        = D_STARTTLS_ROLE_SERVER,
        .verify      = true,
        .ca_file     = g_pki.ca,
        .cert_file   = g_pki.server_cert,
        .key_file    = g_pki.server_key,
        .min_version = D_SSL_VERSION_NONE
    };
    const struct d_tests_starttls_side holder   =
    {
        .role        = D_STARTTLS_ROLE_CLIENT,
        .verify      = true,
        .ca_file     = g_pki.ca,
        .cert_file   = g_pki.client_cert,
        .key_file    = g_pki.client_key,
        .min_version = D_SSL_VERSION_NONE
    };
    const struct d_tests_starttls_side stranger = d_tests_starttls_verifier();

    const bool made = ( (g_ready)                           &&
                        (d_tests_starttls_context(_server_tls,
                                                  &strict)) &&
                        (d_tests_starttls_context(_holder_tls,
                                                  &holder)) &&
                        (d_tests_starttls_context(_stranger_tls,
                                                  &stranger)) );

    // a context made before one that failed is not left behind
    if (!made)
    {
        d_ssl_context_destroy(_server_tls);
        d_ssl_context_destroy(_holder_tls);
        d_ssl_context_destroy(_stranger_tls);
    }

    return made;
}

/*
d_tests_sa_starttls_openssl_client_cert
  Tests the following:
  - a verifying server accepts a client certificate from the run's CA
  - and fails a client that presents none, which then cannot use the
    connection either
*/
bool
d_tests_sa_starttls_openssl_client_cert(
    struct d_test_counter* _counter
)
{
    const char* const              TEST         = "OpenSSL client certificates";
    struct d_ssl_context           strict_tls   = { .engine = NULL };
    struct d_ssl_context           holder_tls   = { .engine = NULL };
    struct d_ssl_context           stranger_tls = { .engine = NULL };
    struct d_tests_starttls_client client       = { .socket = -1 };
    struct d_tests_starttls_server server       = { .socket = -1 };
    bool                           accepted     = false;
    bool                           turned_away  = false;
    bool                           result       = true;

    if (d_tests_starttls_demanding(&strict_tls,
                                   &holder_tls,
                                   &stranger_tls))
    {
        d_tests_starttls_parties(&client,
                                 &server,
                                 &holder_tls,
                                 &strict_tls,
                                 "localhost");
        accepted = ( (d_tests_starttls_meet(&client,
                                            &server)) &&
                     (d_tests_starttls_completed(&client,
                                                 &server)) );
        d_tests_starttls_parties(&client,
                                 &server,
                                 &stranger_tls,
                                 &strict_tls,
                                 "localhost");
        turned_away =
            ( (d_tests_starttls_meet(&client,
                                     &server))               &&
              (server.upgrade != D_STARTTLS_OK)              &&
              (server.upgrade != D_TESTS_STARTTLS_UNREACHED) &&
              ( (client.upgrade != D_STARTTLS_OK) ||
                (client.reply != D_STARTTLS_OK) ) );
        d_ssl_context_destroy(&strict_tls);
        d_ssl_context_destroy(&holder_tls);
        d_ssl_context_destroy(&stranger_tls);
    }

    result = d_tests_net_assert(accepted,
                                TEST,
                                "a client certificate from the CA is taken",
                                _counter) && result;
    result = d_tests_net_assert(turned_away,
                                TEST,
                                "a client without one is turned away",
                                _counter) && result;

    return result;
}
