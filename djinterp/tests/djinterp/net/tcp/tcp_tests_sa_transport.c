/*******************************************************************************
* djinterp [net]                                        tcp_tests_sa_transport.c
*
* Standalone tests of TCP connections and listeners, one thread each.
*   Everything here fits in one thread: a connect completes into the
* listener's backlog before accept is called, and the texts exchanged are far
* smaller than a socket buffer.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa_transport.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
// enable POSIX.1-2008 (fcntl, fopen's companions) under a strict C
// compiler; a feature-test macro must precede every include, so it sits
// ahead of the suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./tcp_tests_sa.h"  // the suite
// std
#include <stdio.h>   // fopen, fclose
#include <errno.h>   // EAGAIN, ECONNREFUSED, EPIPE
#include <string.h>  // memset, strcmp, strlen
// posix
#include <fcntl.h>   // fcntl, F_GETFD, F_GETFL, F_SETFL, FD_CLOEXEC
#include <unistd.h>  // close, dup


/*
d_tests_sa_tcp_options
  Tests the following:
  - d_tcp_options_init gives tcp.hpp's defaults, the backlog from cfg_tcp.h
  - a NULL argument is ignored
  - the BSD backend is built and available
*/
bool
d_tests_sa_tcp_options(
    struct d_test_counter* _counter
)
{
    const char* const    TEST    = "d_tcp_options_init";
    struct d_tcp_options options = { .connect_timeout_ms = 5 };
    bool                 result  = true;

    d_tcp_options_init(&options);
    d_tcp_options_init(NULL);

    const bool defaults = ( (options.connect_timeout_ms == 0) &&
                            (options.listen_backlog == 128) &&
                            (!options.no_delay) &&
                            (options.reuse_address) &&
                            (!options.reuse_port) );

    result = d_assert_standalone(defaults,
                                 TEST,
                                 "tcp.hpp's defaults; NULL is ignored",
                                 _counter) && result;
    result = d_assert_standalone(d_tcp_available(),
                                 TEST,
                                 "the BSD backend is available",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_closed
  Tests the following:
  - a fresh connection is closed: no socket, no endpoints; reads, writes,
    and shutdowns report D_NET_ERROR_CLOSED; release yields nothing
  - a fresh listener is closed, and accepting on it reports closed
  - closing either twice is harmless
*/
bool
d_tests_sa_tcp_closed(
    struct d_test_counter* _counter
)
{
    const char* const       TEST       = "closed connections";
    struct d_tcp_connection connection = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_listener   listener   = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   endpoint   = { .port = 0u };
    char                    byte       = 'x';
    bool                    result     = true;

    d_tcp_connection_init(&connection);
    d_tcp_listener_init(&listener);

    const struct d_net_io_result got = d_tcp_connection_read(&connection,
                                                             &byte,
                                                             1u);
    const struct d_net_io_result put = d_tcp_connection_write(&connection,
                                                              &byte,
                                                              1u);
    const bool closed = ( (!d_tcp_connection_is_open(&connection)) &&
                          (d_tcp_connection_native(&connection) ==
                           D_TCP_SOCKET_INVALID) &&
                          (got.count == 0u) &&
                          (got.error == D_NET_ERROR_CLOSED) &&
                          (put.error == D_NET_ERROR_CLOSED) &&
                          (d_tcp_connection_shutdown(&connection,
                                                     D_NET_SHUTDOWN_BOTH) ==
                           D_NET_ERROR_CLOSED) &&
                          (!d_tcp_connection_remote_endpoint(&connection,
                                                             &endpoint)) &&
                          (!d_tcp_connection_local_endpoint(&connection,
                                                            &endpoint)) &&
                          (d_tcp_connection_release(&connection) ==
                           D_TCP_SOCKET_INVALID) );
    const bool idle   = ( (!d_tcp_listener_is_open(&listener)) &&
                          (d_tcp_listener_native(&listener) ==
                           D_TCP_SOCKET_INVALID) &&
                          (!d_tcp_listener_local_endpoint(&listener,
                                                          &endpoint)) &&
                          (d_tcp_accept(&listener,
                                        &connection) ==
                           D_NET_ERROR_CLOSED) );

    d_tcp_connection_close(&connection);
    d_tcp_connection_close(&connection);
    d_tcp_listener_close(&listener);
    d_tcp_listener_close(&listener);
    result = d_assert_standalone(closed,
                                 TEST,
                                 "a fresh connection is closed throughout",
                                 _counter) && result;
    result = d_assert_standalone(idle,
                                 TEST,
                                 "a fresh listener is closed; accept says so",
                                 _counter) && result;

    return result;
}

/*
d_tests_tcp_refuses_udp
  Whether connect and listen refuse an endpoint turned UDP, and adopt
refuses no socket, leaving the connection closed.
*/
static bool
d_tests_tcp_refuses_udp(
    struct d_tcp_connection* _connection,
    struct d_tcp_listener*   _listener,
    struct d_net_endpoint*   _endpoint
)
{
    _endpoint->protocol = D_NET_PROTOCOL_UDP;

    return ( (d_tcp_connect(_connection,
                            _endpoint,
                            NULL) == D_NET_ERROR_INVALID_ARGUMENT) &&
             (d_tcp_listen(_listener,
                           _endpoint,
                           NULL) == D_NET_ERROR_INVALID_ARGUMENT) &&
             (d_tcp_connection_adopt(_connection,
                                     D_TCP_SOCKET_INVALID,
                                     D_NET_PROTOCOL_TCP) ==
              D_NET_ERROR_INVALID_ARGUMENT) &&
             (!d_tcp_connection_is_open(_connection)) );
}

/*
d_tests_sa_tcp_refusals
  Tests the following:
  - NULL connections, listeners, endpoints, and buffers are refused
  - a UDP endpoint is refused by connect and listen alike
  - adopting no socket is refused
*/
bool
d_tests_sa_tcp_refusals(
    struct d_test_counter* _counter
)
{
    const char* const       TEST       = "argument refusals";
    struct d_tcp_connection connection = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_listener   listener   = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   endpoint   = { .port = 0u };
    bool                    result     = true;

    d_tcp_listener_init(&listener);

    const bool prepared = d_tests_tcp_endpoint(&endpoint,
                                               "127.0.0.1",
                                               9u);
    const bool nulls    = ( (d_tcp_connect(NULL,
                                           &endpoint,
                                           NULL) ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_connect(&connection,
                                           NULL,
                                           NULL) ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_listen(NULL,
                                          &endpoint,
                                          NULL) ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_accept(NULL,
                                          &connection) ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_accept(&listener,
                                          NULL) ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_connection_read(NULL,
                                                   NULL,
                                                   0u).error ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_connection_read(&connection,
                                                   NULL,
                                                   1u).error ==
                             D_NET_ERROR_INVALID_ARGUMENT) &&
                            (d_tcp_connection_shutdown(NULL,
                                                       D_NET_SHUTDOWN_BOTH) ==
                             D_NET_ERROR_INVALID_ARGUMENT) );

    const bool udp = d_tests_tcp_refuses_udp(&connection,
                                             &listener,
                                             &endpoint);

    result = d_assert_standalone((prepared) && (nulls),
                                 TEST,
                                 "NULL arguments are refused",
                                 _counter) && result;
    result = d_assert_standalone(udp,
                                 TEST,
                                 "UDP, and adopting no socket, are refused",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_loopback
  Tests the following:
  - a listener on port 0 records its ephemeral port
  - each end records both endpoints, and they agree across the connection
  - bytes cross in both directions
*/
bool
d_tests_sa_tcp_loopback(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "loopback connections";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    struct d_net_endpoint   bound  = { .port = 0u };
    struct d_net_endpoint   remote = { .port = 0u };
    struct d_net_endpoint   local  = { .port = 0u };
    struct d_net_endpoint   peer   = { .port = 0u };
    bool                    result = true;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              NULL);
    const bool agreed = ( (opened) &&
                          (d_tcp_listener_local_endpoint(&pair.listener,
                                                         &bound)) &&
                          (strcmp(bound.host,
                                  "127.0.0.1") == 0) &&
                          (bound.port != 0u) &&
                          (d_tcp_connection_remote_endpoint(&pair.client,
                                                            &remote)) &&
                          (d_net_endpoint_equal(&remote,
                                                &bound)) &&
                          (d_tcp_connection_local_endpoint(&pair.client,
                                                           &local)) &&
                          (d_tcp_connection_remote_endpoint(&pair.server,
                                                            &peer)) &&
                          (d_net_endpoint_equal(&local,
                                                &peer)) );
    const bool crossed = ( (opened) &&
                           (d_tcp_connection_is_open(&pair.client)) &&
                           (d_tcp_connection_is_open(&pair.server)) &&
                           (d_tests_tcp_exchange(&pair.client,
                                                 &pair.server,
                                                 "hello, server")) &&
                           (d_tests_tcp_exchange(&pair.server,
                                                 &pair.client,
                                                 "hello, client")) );

    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone(agreed,
                                 TEST,
                                 "endpoints are recorded and agree",
                                 _counter) && result;
    result = d_assert_standalone(crossed,
                                 TEST,
                                 "bytes cross in both directions",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_half_close
  Tests the following:
  - after a write shutdown the peer reads the end of the stream as (0, NONE)
  - the other direction still carries bytes
  - writing after one's own shutdown reports a reset, not SIGPIPE
*/
bool
d_tests_sa_tcp_half_close(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "half-closed connections";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    char                    byte   = 'x';
    bool                    result = true;

    const bool opened = ( (d_tests_tcp_pair_open(&pair,
                                                 NULL)) &&
                          (d_tcp_connection_shutdown(&pair.client,
                                                     D_NET_SHUTDOWN_WRITE) ==
                           D_NET_ERROR_NONE) );

    const struct d_net_io_result end = d_tcp_connection_read(&pair.server,
                                                             &byte,
                                                             1u);
    const struct d_net_io_result put = d_tcp_connection_write(&pair.client,
                                                              &byte,
                                                              1u);

    result = d_assert_standalone( (opened) &&
                                  (end.count == 0u) &&
                                  (end.error == D_NET_ERROR_NONE),
                                 TEST,
                                 "the peer reads the end of the stream",
                                 _counter) && result;
    result = d_assert_standalone( (opened) &&
                                  (d_tests_tcp_exchange(&pair.server,
                                                        &pair.client,
                                                        "still open")),
                                 TEST,
                                 "the other direction still carries bytes",
                                 _counter) && result;
    result = d_assert_standalone(put.error == D_NET_ERROR_CONNECTION_RESET,
                                 TEST,
                                 "a write after shutdown is a reset",
                                 _counter) && result;
    d_tests_tcp_pair_close(&pair);

    return result;
}

/*
d_tests_sa_tcp_net_view
  Tests the following:
  - &connection->base is a d_net_connection that answers for the socket
  - net.h's dispatch reaches the endpoints, shutdown, and close
  - closing through the view releases the descriptor
*/
bool
d_tests_sa_tcp_net_view(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "the d_net_connection view";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    struct d_net_endpoint   direct = { .port = 0u };
    struct d_net_endpoint   viewed = { .port = 0u };
    char                    byte   = 'x';
    bool                    result = true;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              NULL);
    const bool agrees = ( (opened) &&
                          (d_net_connection_is_open(&pair.client.base)) &&
                          (d_tcp_connection_remote_endpoint(&pair.client,
                                                            &direct)) &&
                          (d_net_connection_remote_endpoint(&pair.client.base,
                                                            &viewed)) &&
                          (d_net_endpoint_equal(&direct,
                                                &viewed)) &&
                          (d_net_connection_shutdown(&pair.client.base,
                                                     D_NET_SHUTDOWN_WRITE) ==
                           D_NET_ERROR_NONE) &&
                          (d_net_connection_read(&pair.server.base,
                                                 &byte,
                                                 1u).count == 0u) );
    const d_tcp_socket native = d_tcp_connection_native(&pair.client);

    d_net_connection_close(&pair.client.base);
    result = d_assert_standalone(agrees,
                                 TEST,
                                 "the view dispatches to the socket",
                                 _counter) && result;
    result = d_assert_standalone( (opened) &&
                                  (!d_net_connection_is_open(
                                        &pair.client.base)) &&
                                  (d_tests_tcp_descriptor_closed(native)),
                                 TEST,
                                 "closing through the view frees the socket",
                                 _counter) && result;
    d_tests_tcp_pair_close(&pair);

    return result;
}

/*
d_tests_sa_tcp_refused
  Tests the following:
  - connecting to a port nothing listens on reports a refusal, blocking or
    with a timeout, and leaves the connection closed
*/
bool
d_tests_sa_tcp_refused(
    struct d_test_counter* _counter
)
{
    const char* const       TEST       = "refused connections";
    struct d_tcp_listener   listener   = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_connection connection = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   where      = { .port = 0u };
    struct d_tcp_options    timed      = { .connect_timeout_ms = 0 };
    bool                    result     = true;

    d_tcp_listener_init(&listener);
    d_tcp_options_init(&timed);
    timed.connect_timeout_ms = 2000;

    // a port just released has no listener
    const bool freed = ( (d_tests_tcp_endpoint(&where,
                                               "127.0.0.1",
                                               0u)) &&
                         (d_tcp_listen(&listener,
                                       &where,
                                       NULL) == D_NET_ERROR_NONE) &&
                         (d_tcp_listener_local_endpoint(&listener,
                                                        &where)) );

    d_tcp_listener_close(&listener);

    const bool blocking = ( (freed) &&
                            (d_tcp_connect(&connection,
                                           &where,
                                           NULL) ==
                             D_NET_ERROR_CONNECTION_REFUSED) &&
                            (!d_tcp_connection_is_open(&connection)) );
    const bool bounded  = ( (freed) &&
                            (d_tcp_connect(&connection,
                                           &where,
                                           &timed) ==
                             D_NET_ERROR_CONNECTION_REFUSED) &&
                            (!d_tcp_connection_is_open(&connection)) );

    result = d_assert_standalone(blocking,
                                 TEST,
                                 "a blocking connect is refused",
                                 _counter) && result;
    result = d_assert_standalone(bounded,
                                 TEST,
                                 "a timed connect is refused too",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_names
  Tests the following:
  - a listener on the empty host serves every local address
  - "localhost" resolves, trying each address until one connects
  - an empty host connects to loopback
  - an unresolvable name fails, leaving the connection closed
*/
bool
d_tests_sa_tcp_names(
    struct d_test_counter* _counter
)
{
    const char* const       TEST     = "resolved names";
    const char* const       UNKNOWN  = "djinterp-no-such-host.invalid";
    struct d_tests_tcp_pair pair     = { .client = { .socket = -1 } };
    struct d_net_endpoint   anywhere = { .port = 0u };
    struct d_net_endpoint   named    = { .port = 0u };
    bool                    result   = true;

    d_tcp_listener_init(&pair.listener);
    d_tcp_connection_init(&pair.client);
    d_tcp_connection_init(&pair.server);

    const bool bound = ( (d_tests_tcp_endpoint(&anywhere,
                                               "",
                                               0u)) &&
                         (d_tcp_listen(&pair.listener,
                                       &anywhere,
                                       NULL) == D_NET_ERROR_NONE) &&
                         (d_tcp_listener_local_endpoint(&pair.listener,
                                                        &anywhere)) );
    const bool by_name = ( (bound) &&
                           (d_tests_tcp_endpoint(&named,
                                                 "localhost",
                                                 anywhere.port)) &&
                           (d_tcp_connect(&pair.client,
                                          &named,
                                          NULL) == D_NET_ERROR_NONE) &&
                           (d_tcp_accept(&pair.listener,
                                         &pair.server) ==
                            D_NET_ERROR_NONE) );
    const bool by_empty = ( (bound) &&
                            (d_tests_tcp_endpoint(&named,
                                                  "",
                                                  anywhere.port)) &&
                            (d_tcp_connect(&pair.client,
                                           &named,
                                           NULL) == D_NET_ERROR_NONE) );
    const bool unknown = ( (d_tests_tcp_endpoint(&named,
                                                 UNKNOWN,
                                                 80u)) &&
                           (d_tcp_connect(&pair.server,
                                          &named,
                                          NULL) != D_NET_ERROR_NONE) &&
                           (!d_tcp_connection_is_open(&pair.server)) );

    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone((by_name) && (by_empty),
                                 TEST,
                                 "localhost and the empty host connect",
                                 _counter) && result;
    result = d_assert_standalone(unknown,
                                 TEST,
                                 "an unresolvable name fails cleanly",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_adopt
  Tests the following:
  - release hands the socket over open, leaving the connection closed
  - adopt makes a working connection of it, recording its endpoints
  - adopting as UDP records UDP endpoints, as tcp.hpp's constructor did
  - a refused adoption closes the socket rather than leaking it
*/
bool
d_tests_sa_tcp_adopt(
    struct d_test_counter* _counter
)
{
    const char* const       TEST    = "adopt and release";
    struct d_tests_tcp_pair pair    = { .client = { .socket = -1 } };
    struct d_tcp_connection adopted = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   remote  = { .port = 0u };
    struct d_net_endpoint   bound   = { .port = 0u };
    bool                    result  = true;

    const bool         opened   = d_tests_tcp_pair_open(&pair,
                                                        NULL);
    const d_tcp_socket released = d_tcp_connection_release(&pair.client);
    const bool         handed   = ( (opened) &&
                                    (released != D_TCP_SOCKET_INVALID) &&
                                    (!d_tcp_connection_is_open(
                                          &pair.client)) &&
                                    (!d_tests_tcp_descriptor_closed(
                                          released)) );
    const bool         working  = ( (handed) &&
                                    (d_tcp_connection_adopt(
                                         &adopted,
                                         released,
                                         D_NET_PROTOCOL_TCP) ==
                                     D_NET_ERROR_NONE) &&
                                    (d_tcp_connection_remote_endpoint(
                                         &adopted,
                                         &remote)) &&
                                    (d_tcp_listener_local_endpoint(
                                         &pair.listener,
                                         &bound)) &&
                                    (d_net_endpoint_equal(&remote,
                                                          &bound)) &&
                                    (d_tests_tcp_exchange(&adopted,
                                                          &pair.server,
                                                          "adopted")) );
    const d_tcp_socket refused  = d_tcp_connection_release(&pair.server);

    result = d_assert_standalone(working,
                                 TEST,
                                 "a released socket is adopted and works",
                                 _counter) && result;
    result = d_assert_standalone( (d_tcp_connection_adopt(
                                       &pair.server,
                                       refused,
                                       (enum d_net_protocol)7) ==
                                   D_NET_ERROR_INVALID_ARGUMENT) &&
                                  (d_tests_tcp_descriptor_closed(refused)),
                                 TEST,
                                 "a refused adoption closes the socket",
                                 _counter) && result;
    d_tcp_connection_close(&adopted);
    d_tests_tcp_pair_close(&pair);

    return result;
}

/*
d_tests_sa_tcp_adopt_udp
  Tests the following:
  - a connected socket adopted as UDP opens, its IP endpoints recorded as
    UDP, as tcp.hpp's socket_connection recorded the protocol it was given
*/
bool
d_tests_sa_tcp_adopt_udp(
    struct d_test_counter* _counter
)
{
    const char* const       TEST    = "adopting as UDP";
    struct d_tests_tcp_pair pair    = { .client = { .socket = -1 } };
    struct d_tcp_connection adopted = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   remote  = { .port = 0u };
    bool                    result  = true;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              NULL);
    const bool marked = ( (opened) &&
                          (d_tcp_connection_adopt(
                               &adopted,
                               d_tcp_connection_release(&pair.client),
                               D_NET_PROTOCOL_UDP) == D_NET_ERROR_NONE) &&
                          (d_tcp_connection_is_open(&adopted)) &&
                          (d_tcp_connection_remote_endpoint(&adopted,
                                                            &remote)) &&
                          (remote.protocol == D_NET_PROTOCOL_UDP) );

    d_tcp_connection_close(&adopted);
    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone(marked,
                                 TEST,
                                 "an adopted UDP socket's endpoints say UDP",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_would_block
  Tests the following:
  - a non-blocking socket with nothing to read reports WOULD_BLOCK, the
    mapping reactor.hpp relies on
  - once a byte arrives, the same read returns it
*/
bool
d_tests_sa_tcp_would_block(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "non-blocking reads";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    char                    byte   = '\0';
    bool                    result = true;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              NULL);
    const int  server = (int)d_tcp_connection_native(&pair.server);
    const bool set    = ( (opened) &&
                          (fcntl(server,
                                 F_SETFL,
                                 fcntl(server,
                                       F_GETFL) | O_NONBLOCK) == 0) );

    const struct d_net_io_result empty = d_tcp_connection_read(&pair.server,
                                                               &byte,
                                                               1u);
    const struct d_net_io_result sent  = d_tcp_connection_write(&pair.client,
                                                                "y",
                                                                1u);

    d_tests_tcp_pause_ms(50L);

    const struct d_net_io_result full = d_tcp_connection_read(&pair.server,
                                                              &byte,
                                                              1u);

    result = d_assert_standalone( (set) &&
                                  (empty.count == 0u) &&
                                  (empty.error == D_NET_ERROR_WOULD_BLOCK),
                                 TEST,
                                 "an empty non-blocking read would block",
                                 _counter) && result;
    result = d_assert_standalone( (sent.count == 1u) &&
                                  (full.count == 1u) &&
                                  (byte == 'y'),
                                 TEST,
                                 "an arrived byte is then read",
                                 _counter) && result;
    d_tests_tcp_pair_close(&pair);

    return result;
}

/*
d_tests_sa_tcp_native
  Tests the following:
  - d_tcp_error_from_native maps errno values as the transport does
  - d_tcp_socket_cloexec marks a descriptor dup() left inheritable
  - neither preparation minds an invalid socket
*/
bool
d_tests_sa_tcp_native(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "native errors and sockets";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    bool                    result = true;

    const bool mapped = ( (d_tcp_error_from_native(0) ==
                           D_NET_ERROR_NONE) &&
                          (d_tcp_error_from_native(EAGAIN) ==
                           D_NET_ERROR_WOULD_BLOCK) &&
                          (d_tcp_error_from_native(ECONNREFUSED) ==
                           D_NET_ERROR_CONNECTION_REFUSED) &&
                          (d_tcp_error_from_native(EPIPE) ==
                           D_NET_ERROR_CONNECTION_RESET) );
    const bool opened = d_tests_tcp_pair_open(&pair,
                                              NULL);
    const int  copy   = (opened) ? dup((int)d_tcp_connection_native(
                                               &pair.client))
                                 : -1;
    const bool plain  = ( (copy >= 0) &&
                          ((fcntl(copy,
                                  F_GETFD) & FD_CLOEXEC) == 0) );

    d_tcp_socket_cloexec((d_tcp_socket)copy);
    d_tcp_socket_no_sigpipe((d_tcp_socket)copy);
    d_tcp_socket_cloexec(D_TCP_SOCKET_INVALID);
    d_tcp_socket_no_sigpipe(D_TCP_SOCKET_INVALID);

    const bool marked = ( (plain) &&
                          ((fcntl(copy,
                                  F_GETFD) & FD_CLOEXEC) != 0) );

    (void)close(copy);
    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone(mapped,
                                 TEST,
                                 "native codes map as the transport's do",
                                 _counter) && result;
    result = d_assert_standalone(marked,
                                 TEST,
                                 "a dup()ed socket is marked close-on-exec",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_listener_errors
  Tests the following:
  - binding a port another listener holds reports ADDRESS_IN_USE, with or
    without SO_REUSEADDR, and leaves the second listener closed
  - listening again on an open listener closes the first binding
*/
bool
d_tests_sa_tcp_listener_errors(
    struct d_test_counter* _counter
)
{
    const char* const     TEST   = "listener errors";
    struct d_tcp_listener first  = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_listener second = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint where  = { .port = 0u };
    struct d_net_endpoint fresh  = { .port = 0u };
    struct d_tcp_options  strict = { .connect_timeout_ms = 0 };
    bool                  result = true;

    d_tcp_listener_init(&first);
    d_tcp_listener_init(&second);
    d_tcp_options_init(&strict);
    strict.reuse_address = false;

    const bool held = ( (d_tests_tcp_endpoint(&fresh,
                                              "127.0.0.1",
                                              0u)) &&
                        (d_tcp_listen(&first,
                                      &fresh,
                                      NULL) == D_NET_ERROR_NONE) &&
                        (d_tcp_listener_local_endpoint(&first,
                                                       &where)) );
    const bool taken = ( (held) &&
                         (d_tcp_listen(&second,
                                       &where,
                                       &strict) ==
                          D_NET_ERROR_ADDRESS_IN_USE) &&
                         (d_tcp_listen(&second,
                                       &where,
                                       NULL) ==
                          D_NET_ERROR_ADDRESS_IN_USE) &&
                         (!d_tcp_listener_is_open(&second)) );

    // rebinding the first frees its old port for the second
    const bool rebound = ( (held) &&
                           (d_tcp_listen(&first,
                                         &fresh,
                                         NULL) == D_NET_ERROR_NONE) &&
                           (d_tcp_listen(&second,
                                         &where,
                                         NULL) == D_NET_ERROR_NONE) );

    d_tcp_listener_close(&first);
    d_tcp_listener_close(&second);
    result = d_assert_standalone(taken,
                                 TEST,
                                 "a held port is in use, reuse or not",
                                 _counter) && result;
    result = d_assert_standalone(rebound,
                                 TEST,
                                 "listening again releases the old binding",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_unix
  Tests the following:
  - a unix-domain listener creates its path and records it
  - a client connects by path; bytes cross both ways
  - the client records the listener's path as its peer; the server finds
    its unnamed client has no endpoint
  - closing the listener removes the path
*/
bool
d_tests_sa_tcp_unix(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "unix-domain connections";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    struct d_net_endpoint   where  = { .port = 0u };
    struct d_net_endpoint   peer   = { .port = 0u };
    bool                    result = true;

    // this test's own path
    char path[128] = { 0 };

    d_tcp_listener_init(&pair.listener);
    d_tcp_connection_init(&pair.client);
    d_tcp_connection_init(&pair.server);

    const struct d_pack_text text = { path,
                                      (d_tests_tcp_unix_path(path,
                                                             sizeof(path),
                                                             "basic"))
                                          ? strlen(path)
                                          : 0u };

    const bool opened = ( (d_net_endpoint_set_local(&where,
                                                    text) ==
                           D_NET_ERROR_NONE) &&
                          (d_tcp_listen(&pair.listener,
                                        &where,
                                        NULL) == D_NET_ERROR_NONE) &&
                          (d_tests_tcp_path_exists(path)) &&
                          (d_tcp_connect(&pair.client,
                                         &where,
                                         NULL) == D_NET_ERROR_NONE) &&
                          (d_tcp_accept(&pair.listener,
                                        &pair.server) ==
                           D_NET_ERROR_NONE) );
    const bool named  = ( (opened) &&
                          (d_tcp_connection_remote_endpoint(&pair.client,
                                                            &peer)) &&
                          (d_net_endpoint_equal(&peer,
                                                &where)) &&
                          (!d_tcp_connection_remote_endpoint(&pair.server,
                                                             &peer)) );

    result = d_assert_standalone( (opened) &&
                                  (d_tests_tcp_exchange(&pair.client,
                                                        &pair.server,
                                                        "by path")) &&
                                  (d_tests_tcp_exchange(&pair.server,
                                                        &pair.client,
                                                        "and back")),
                                 TEST,
                                 "bytes cross a unix-domain connection",
                                 _counter) && result;
    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone( (named) &&
                                  (!d_tests_tcp_path_exists(path)),
                                 TEST,
                                 "endpoints recorded; the path is removed",
                                 _counter) && result;

    return result;
}

/*
d_tests_tcp_make_stale
  Leaves an ordinary file at a path, as a crashed listener would leave its
socket file.
*/
static bool
d_tests_tcp_make_stale(
    const char* _path
)
{
    FILE* stale = fopen(_path,
                        "w");

    // the file only has to exist
    if (!stale)
    {
        return false;
    }

    return (fclose(stale) == 0);
}

/*
d_tests_tcp_refuses_path
  Whether listen and connect both refuse a unix-domain path, as address
problems rather than system errors.
*/
static bool
d_tests_tcp_refuses_path(
    struct d_tcp_listener*   _listener,
    struct d_tcp_connection* _connection,
    const char*              _path
)
{
    const struct d_pack_text text  = { _path,
                                       strlen(_path) };
    struct d_net_endpoint    where = { .port = 0u };

    return ( (d_net_endpoint_set_local(&where,
                                       text) == D_NET_ERROR_NONE) &&
             (d_tcp_listen(_listener,
                           &where,
                           NULL) == D_NET_ERROR_ADDRESS_INVALID) &&
             (d_tcp_connect(_connection,
                            &where,
                            NULL) == D_NET_ERROR_ADDRESS_INVALID) );
}

/*
d_tests_sa_tcp_unix_paths
  Tests the following:
  - a file already at the path is replaced, as tcp.hpp replaced it
  - a path too long for sun_path is refused by listen and connect
  - connecting to a path nothing listens on fails
*/
bool
d_tests_sa_tcp_unix_paths(
    struct d_test_counter* _counter
)
{
    const char* const       TEST       = "unix-domain paths";
    struct d_tcp_listener   listener   = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_connection connection = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   where      = { .port = 0u };
    bool                    result     = true;

    // this test's own path, and one too long for any sun_path
    char path[128] = { 0 };
    char long_path[200] = { 0 };

    d_tcp_listener_init(&listener);
    memset(long_path,
           'p',
           sizeof(long_path) - 1u);

    const struct d_pack_text text = { path,
                                      (d_tests_tcp_unix_path(path,
                                                             sizeof(path),
                                                             "stale"))
                                          ? strlen(path)
                                          : 0u };

    const bool replaced = ( (d_tests_tcp_make_stale(path)) &&
                            (d_net_endpoint_set_local(&where,
                                                      text) ==
                             D_NET_ERROR_NONE) &&
                            (d_tcp_listen(&listener,
                                          &where,
                                          NULL) == D_NET_ERROR_NONE) );

    d_tcp_listener_close(&listener);

    const bool missing = ( (d_tcp_connect(&connection,
                                          &where,
                                          NULL) != D_NET_ERROR_NONE) &&
                           (!d_tcp_connection_is_open(&connection)) );
    const bool refused = d_tests_tcp_refuses_path(&listener,
                                                  &connection,
                                                  long_path);

    result = d_assert_standalone((replaced) && (missing),
                                 TEST,
                                 "a stale file is replaced; absence fails",
                                 _counter) && result;
    result = d_assert_standalone(refused,
                                 TEST,
                                 "an overlong path is refused",
                                 _counter) && result;

    return result;
}
