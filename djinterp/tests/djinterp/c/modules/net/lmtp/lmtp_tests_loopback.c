/*******************************************************************************
* djinterp [c][net]                                        lmtp_tests_loopback.c
*
* LMTP's client and server together, the server on a thread: over loopback
* TCP, on a port the system chooses; over a unix-domain socket, as LMTP's
* servers are commonly offered; and an LMTP client against an SMTP server,
* which refuses its LHLO, and to which it says nothing more.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests_loopback.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
// POSIX.1-2008 for dmutex.h's threads under a strict C compiler; the
// feature-test macro must precede every include, so it sits ahead of the
// suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./lmtp_tests.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <string.h>   // memcpy, strcmp, strlen
#include <time.h>     // clock, time
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/tcp/tcp.h"          // TCP
#include "../../../../../../inc/djinterp/env/net/tcp/env_tcp.h"            // D_ENV_TCP_HAS_UNIX
#include "../../../../../../inc/djinterp/c/sync/dmutex.h"  // threads


// dj_tests_lmtp_server
//   struct: a server thread's setup and outcome: its listener, settings and
// handler, what the handler saw, the port or path it listens on, and how
// its session ended.
struct dj_tests_lmtp_server
{
    struct dj_tcp_listener         listener;
    struct dj_smtp_server_config   config;
    struct dj_smtp_server_handler  handler;
    struct dj_tests_lmtp_mailboxes boxes;
    unsigned int                   port;
    enum dj_smtp_error             result;
};

/*
d_tests_lmtp_serve_thread
  Server thread: accepts one connection, runs one session over it, then
closes it.
*/
static
dj_thread_result_t d_tests_lmtp_serve_thread(
    void* _server
)
{
    struct dj_tests_lmtp_server* server     = _server;
    struct dj_tcp_connection     connection = { .base = { .vtable = NULL } };

    d_tcp_connection_init(&connection);

    const enum dj_net_error accepted = d_tcp_accept_wait(&server->listener,
                                                        &connection,
                                                        10000L);

    // a connection that never came is the session's end
    if (accepted != D_NET_OK)
    {
        server->result = d_smtp_error_from_net(accepted);

        return D_THREAD_SUCCESS;
    }

    server->result = d_smtp_server_session(&connection.base,
                                           &server->config,
                                           &server->handler);
    d_tcp_connection_close(&connection);

    return D_THREAD_SUCCESS;
}

/*
d_tests_lmtp_server_start
  Listens on `_endpoint` - 127.0.0.1 on a port the system chooses, or a
unix-domain socket's path - in LMTP or, without `_lmtp`, SMTP, and starts
the server thread; false, with nothing left open, when either fails.
*/
static
bool d_tests_lmtp_server_start(
    struct dj_tests_lmtp_server*  _server,
    const struct dj_net_endpoint* _endpoint,
    bool                          _lmtp,
    dj_thread_t*                  _thread
)
{
    struct dj_net_endpoint bound   = { .port = 0 };
    struct dj_tcp_options  options = { .io_timeout_ms = 0 };

    d_net_endpoint_init(&bound);
    d_tcp_options_init(&options);
    options.io_timeout_ms = 10000L;
    d_tcp_listener_init(&_server->listener);

    if (_lmtp)
    {
        d_lmtp_server_config_init(&_server->config);
    }
    else
    {
        d_smtp_server_config_init(&_server->config);
    }

    d_tests_lmtp_handler(&_server->handler,
                         &_server->boxes,
                         true);
    _server->result = D_SMTP_ERROR_STATE;

    // a listener, its port, and the thread that serves it
    if ( (d_tcp_listen(&_server->listener,
                       _endpoint,
                       &options) == D_NET_OK)    &&
         (d_tcp_listener_local_endpoint(&_server->listener,
                                        &bound)) &&
         (d_thread_create(_thread,
                          d_tests_lmtp_serve_thread,
                          _server) == 0) )
    {
        _server->port = bound.port;

        return true;
    }

    d_tcp_listener_close(&_server->listener);

    return false;
}

/*
d_tests_lmtp_server_stop
  Waits for the server thread, then closes its listener, which removes a
unix-domain socket's path.
*/
static
void d_tests_lmtp_server_stop(
    struct dj_tests_lmtp_server* _server,
    dj_thread_t                  _thread
)
{
    (void)d_thread_join(_thread,
                        NULL);
    d_tcp_listener_close(&_server->listener);

    return;
}

/*
d_tests_lmtp_round
  One client session against a started server: connect, send to bob,
reject, full and carol, and quit; the client is left closed, its reply
kept.
*/
static enum dj_smtp_error
d_tests_lmtp_round(
    struct dj_lmtp_client*               _client,
    const struct dj_smtp_client_options* _options,
    struct dj_smtp_send_report*          _report
)
{
    static const char* const      RECIPIENTS[] = { "bob@example.com",
                                                   "reject@example.com",
                                                   "full@example.com",
                                                   "carol@example.com" };
    const struct dj_smtp_envelope envelope     =
    {
        .sender          = "alice@example.org",
        .recipients      = RECIPIENTS,
        .recipient_count = 4u
    };
    enum dj_smtp_error result = d_lmtp_client_connect(_client,
                                                     _options);

    if (result == D_SMTP_OK)
    {
        result = d_lmtp_client_send(_client,
                                    &envelope,
                                    "Subject: loop\r\n\r\n.dot\nend",
                                    strlen("Subject: loop\r\n\r\n.dot\nend"),
                                    _report);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_quit(&_client->smtp);
    }

    d_smtp_client_close(&_client->smtp);

    return result;
}

/*
d_tests_lmtp_delivered
  Whether a round's report and the server's mailboxes agree with what the
test handler does: bob and carol delivered, reject refused at RCPT, full
refused after the data, and the message intact.
*/
static
bool d_tests_lmtp_delivered(
    const struct dj_smtp_send_report*     _report,
    const struct dj_tests_lmtp_mailboxes* _boxes
)
{
    return ( (_report->accepted == 3u)                          &&
             (_report->delivered == 2u)                         &&
             (_report->recipients[0].code == 250u)              &&
             (_report->recipients[1].code == 550u)              &&
             (_report->recipients[2].code == 250u)              &&
             (_report->recipients[3].code == 250u)              &&
             (_report->deliveries[0].code == 250u)              &&
             (_report->deliveries[1].code == 0u)                &&
             (_report->deliveries[2].code == 452u)              &&
             (_report->deliveries[3].code == 250u)              &&
             (_boxes->deliveries == 3u)                         &&
             (strcmp(_boxes->recipient[2],
                     "carol@example.com") == 0)                 &&
             (strcmp(_boxes->domain,
                     "client.example") == 0)                    &&
             (strcmp(_boxes->data,
                     "Subject: loop\r\n\r\n.dot\r\nend\r\n") == 0) );
}

/*
d_tests_lmtp_options
  LMTP's options, in plaintext, as client.example, with ten-second limits.
*/
static
void d_tests_lmtp_options(
    struct dj_smtp_client_options* _options
)
{
    d_lmtp_client_options_init(_options);
    _options->security           = D_SMTP_SECURITY_NONE;
    _options->helo_domain        = "client.example";
    _options->connect_timeout_ms = 10000L;
    _options->io_timeout_ms      = 10000L;

    return;
}

/*
d_tests_lmtp_loopback_tcp
  Tests the following:
  - an LMTP client and server over loopback TCP, on a port of the system's
  - the report and the server agree on each recipient: two delivered, one
    refused at RCPT, one refused after the data
  - the message arrives with transparency undone and CRLF line endings
*/
bool d_tests_lmtp_loopback_tcp(
    struct dj_test_suite* _suite
)
{
    const char* const               TEST        = "LMTP loopback (TCP)";
    struct dj_tests_lmtp_server     server;
    struct dj_net_endpoint          endpoint    = { .port = 0 };
    struct dj_smtp_client_options   options;
    struct dj_lmtp_client           client;
    struct dj_smtp_recipient_status rcpt[4]     = { { .code = 0u } };
    struct dj_smtp_recipient_status delivery[4] = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .recipients = rcpt,
                                                   .deliveries = delivery };
    dj_thread_t                     thread;

    if (!d_tcp_available())
    {
        return d_test_suite_skip(_suite,
                                 "no TCP backend");
    }

    d_net_endpoint_init(&endpoint);
    memcpy(endpoint.host,
           "127.0.0.1",
           sizeof("127.0.0.1"));
    endpoint.protocol = D_NET_PROTOCOL_TCP;

    if (!d_tests_lmtp_server_start(&server,
                                   &endpoint,
                                   true,
                                   &thread))
    {
        return D_TESTS_NET_ASSERT(false,
                                  TEST,
                                  "the loopback server starts",
                                  _suite);
    }

    d_lmtp_client_init(&client);
    d_tests_lmtp_options(&options);
    options.host = "127.0.0.1";
    options.port = server.port;

    const enum dj_smtp_error sent = d_tests_lmtp_round(&client,
                                                      &options,
                                                      &report);

    d_tests_lmtp_server_stop(&server,
                             thread);

    return D_TESTS_NET_ASSERT( ( (sent == D_SMTP_OK)          &&
                                 (server.result == D_SMTP_OK) &&
                                 (d_tests_lmtp_delivered(&report,
                                                         &server.boxes)) ),
                               TEST,
                               "each recipient's delivery crosses loopback",
                               _suite);
}

/*
d_tests_lmtp_socket_path
  A path for a unix-domain socket under TMPDIR, or /tmp, made unlikely to be
another run's by the time and the clock; false when it does not fit.
*/
static
bool d_tests_lmtp_socket_path(
    char*  _out,
    size_t _capacity
)
{
    char       base[160] = { 0 };
    const int  written   = (d_tests_lmtp_setting("TMPDIR",
                                                 base,
                                                 sizeof(base)))
                           ? snprintf(_out,
                                      _capacity,
                                      "%s/d_lmtp_%lx_%lx.sock",
                                      base,
                                      (unsigned long)time(NULL),
                                      (unsigned long)clock())
                           : snprintf(_out,
                                      _capacity,
                                      "/tmp/d_lmtp_%lx_%lx.sock",
                                      (unsigned long)time(NULL),
                                      (unsigned long)clock());

    return ( (written > 0) &&
             ((size_t)written < _capacity) &&
             ((size_t)written < 100u) );
}

/*
d_tests_lmtp_loopback_unix
  Tests the following:
  - the same round over a unix-domain socket: the options' `local` makes
    `host` its path, and no port is asked for
  - with no fully qualified name to give, LHLO names "localhost", or the
    machine's name: the session opens either way
*/
bool d_tests_lmtp_loopback_unix(
    struct dj_test_suite* _suite
)
{
#if (D_ENV_TCP_HAS_UNIX == 1)
    const char* const               TEST        = "LMTP loopback (unix)";
    struct dj_tests_lmtp_server     server;
    struct dj_net_endpoint          endpoint    = { .port = 0 };
    struct dj_smtp_client_options   options;
    struct dj_lmtp_client           client;
    struct dj_smtp_recipient_status rcpt[4]     = { { .code = 0u } };
    struct dj_smtp_recipient_status delivery[4] = { { .code = 0u } };
    struct dj_smtp_send_report      report      = { .recipients = rcpt,
                                                   .deliveries = delivery };
    char                            path[128]   = { 0 };
    dj_thread_t                     thread;

    if ( (!d_tcp_available()) ||
         (!d_tests_lmtp_socket_path(path,
                                    sizeof(path))) )
    {
        return d_test_suite_skip(_suite,
                                 "no TCP backend, or no socket path that "
                                 "fits");
    }

    d_net_endpoint_init(&endpoint);
    memcpy(endpoint.host,
           path,
           strlen(path) + 1u);
    endpoint.protocol = D_NET_PROTOCOL_UNIX;

    if (!d_tests_lmtp_server_start(&server,
                                   &endpoint,
                                   true,
                                   &thread))
    {
        return D_TESTS_NET_ASSERT(false,
                                  TEST,
                                  "the socket's server starts",
                                  _suite);
    }

    d_lmtp_client_init(&client);
    d_tests_lmtp_options(&options);
    options.host  = path;
    options.local = true;

    const enum dj_smtp_error sent = d_tests_lmtp_round(&client,
                                                      &options,
                                                      &report);

    // the identity this end chose, and the round's
    options.helo_domain = NULL;

    const bool held = D_TESTS_NET_ASSERT(
                          ( (sent == D_SMTP_OK)          &&
                            (d_tests_lmtp_delivered(&report,
                                                    &server.boxes)) ),
                          TEST,
                          "each recipient's delivery crosses the socket",
                          _suite);

    d_tests_lmtp_server_stop(&server,
                             thread);

    // without an identity given: this machine's name, or localhost
    if (d_tests_lmtp_server_start(&server,
                                  &endpoint,
                                  true,
                                  &thread))
    {
        const enum dj_smtp_error again = d_tests_lmtp_round(&client,
                                                           &options,
                                                           &report);

        d_tests_lmtp_server_stop(&server,
                                 thread);

        return D_TESTS_NET_ASSERT( ( (again == D_SMTP_OK) &&
                                     (server.boxes.domain[0] != '\0') ),
                                   TEST,
                                   "LHLO names an identity of its own",
                                   _suite) && held;
    }

    return D_TESTS_NET_ASSERT(false,
                              TEST,
                              "the socket's server starts again",
                              _suite) && held;
#else
    return d_test_suite_skip(_suite,
                             "no unix-domain sockets here");
#endif  // D_ENV_TCP_HAS_UNIX
}

/*
d_tests_lmtp_loopback_smtp_server
  Tests the following:
  - an LMTP client against an SMTP server: LHLO is refused, and the
    connect is, with the server's 500 in the client's reply
  - no HELO follows: the server saw no greeting it accepted
*/
bool d_tests_lmtp_loopback_smtp_server(
    struct dj_test_suite* _suite
)
{
    const char* const             TEST     = "LMTP client, SMTP server";
    struct dj_tests_lmtp_server   server;
    struct dj_net_endpoint        endpoint = { .port = 0 };
    struct dj_smtp_client_options options;
    struct dj_lmtp_client         client;
    dj_thread_t                   thread;

    if (!d_tcp_available())
    {
        return d_test_suite_skip(_suite,
                                 "no TCP backend");
    }

    d_net_endpoint_init(&endpoint);
    memcpy(endpoint.host,
           "127.0.0.1",
           sizeof("127.0.0.1"));
    endpoint.protocol = D_NET_PROTOCOL_TCP;

    if (!d_tests_lmtp_server_start(&server,
                                   &endpoint,
                                   false,
                                   &thread))
    {
        return D_TESTS_NET_ASSERT(false,
                                  TEST,
                                  "the SMTP server starts",
                                  _suite);
    }

    d_lmtp_client_init(&client);
    d_tests_lmtp_options(&options);
    options.host = "127.0.0.1";
    options.port = server.port;

    const enum dj_smtp_error opened = d_lmtp_client_connect(&client,
                                                           &options);

    d_smtp_client_close(&client.smtp);
    d_tests_lmtp_server_stop(&server,
                             thread);

    return D_TESTS_NET_ASSERT( ( (opened == D_SMTP_ERROR_REJECTED)   &&
                                 (client.smtp.reply.code == 500u)    &&
                                 (server.boxes.domain[0] == '\0') ),
                               TEST,
                               "LHLO refused, and nothing after it",
                               _suite);
}
