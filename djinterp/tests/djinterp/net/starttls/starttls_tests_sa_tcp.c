/*******************************************************************************
* djinterp [net]                                         starttls_tests_sa_tcp.c
*
* Standalone tests of a session over TCP: one end of an AF_UNIX stream pair
* is adopted by tcp.c and handed to the session, and the test plays the peer
* on the other end with plain reads and writes. What tcp.c reports -- a
* deadline, a peer that has gone -- reaches the session through
* d_starttls_error_from_net, with its own error kept as the cause.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_tcp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
// enable POSIX.1-2008 (sigaction, socketpair, fcntl) under a strict C
// compiler; a feature-test macro must precede every include, so it sits
// ahead of the suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./starttls_tests_sa.h"  // the suite
// std
#include <signal.h>  // sigaction, sigemptyset, SIGPIPE, sig_atomic_t
#include <string.h>  // memcmp, strlen
// posix
#include <unistd.h>  // close, read, write


// g_sigpipes
//   variable: SIGPIPEs delivered while d_tests_sa_starttls_tcp_vanished
// listened for them.
static volatile sig_atomic_t g_sigpipes = 0;

/*
d_tests_starttls_on_sigpipe
  Counts a SIGPIPE, which tcp.c exists to prevent.
*/
static void
d_tests_starttls_on_sigpipe(
    int _signal
)
{
    (void)_signal;
    g_sigpipes = g_sigpipes + 1;

    return;
}

/*
d_tests_starttls_write_text
  Writes `_text` to `_socket` with one write, as the peer.
*/
static bool
d_tests_starttls_write_text(
    int         _socket,
    const char* _text
)
{
    const size_t length = strlen(_text);

    return (write(_socket,
                  _text,
                  length) == (ssize_t)length);
}

/*
d_tests_starttls_read_text
  Whether the next bytes on `_socket` are `_expected`, read as the peer.
*/
static bool
d_tests_starttls_read_text(
    int         _socket,
    const char* _expected
)
{
    const size_t length      = strlen(_expected);
    char         buffer[128] = { 0 };
    size_t       got         = 0;

    if (length > sizeof(buffer))
    {
        return false;
    }

    // until as many bytes as expected have arrived
    while (got < length)
    {
        const ssize_t count = read(_socket,
                                   buffer + got,
                                   length - got);

        if (count <= 0)
        {
            return false;
        }

        got += (size_t)count;
    }

    return (memcmp(buffer,
                   _expected,
                   length) == 0);
}

/*
d_tests_starttls_tcp_session
  A plaintext client session over `_connection`, owning it or not.
*/
static bool
d_tests_starttls_tcp_session(
    struct d_starttls_session* _session,
    struct d_tcp_connection*   _connection,
    bool                       _owns
)
{
    struct d_starttls_options options =
        d_tests_starttls_options(D_STARTTLS_ROLE_CLIENT,
                                 D_STARTTLS_POLICY_DISABLED,
                                 NULL);

    options.owns_connection = _owns;

    return (d_starttls_init(_session,
                            &options,
                            &_connection->base) == D_STARTTLS_OK);
}

/*
d_tests_starttls_counts
  Whether five bytes the peer writes on `_peer` are counted as waiting by
the session's connection, without being taken, and then received.
*/
static bool
d_tests_starttls_counts(
    struct d_starttls_session* _session,
    int                        _peer
)
{
    size_t waiting  = 0;
    char   bytes[5] = { 0 };

    return ( (d_tests_starttls_write_text(_peer,
                                          "12345"))                  &&
             (d_net_connection_available(_session->connection,
                                         &waiting))                  &&
             (waiting == 5)                                          &&
             (d_starttls_recv_exact(_session,
                                    bytes,
                                    sizeof(bytes)) == D_STARTTLS_OK) &&
             (memcmp(bytes,
                     "12345",
                     sizeof(bytes)) == 0) );
}

/*
d_tests_starttls_tcp_pair
  A socket pair whose first end a TCP connection adopts; false, with
nothing left open, if either step failed.
*/
static bool
d_tests_starttls_tcp_pair(
    struct d_tcp_connection* _connection,
    int                      _sockets[2]
)
{
    if (!d_tests_starttls_socket_pair(_sockets))
    {
        return false;
    }

    // the connection owns the first end even when adopting fails
    if (!d_tests_starttls_adopt(_connection,
                                _sockets[0]))
    {
        d_tcp_connection_close(_connection);
        (void)close(_sockets[1]);

        return false;
    }

    return true;
}

/*
d_tests_sa_starttls_tcp_exchange
  Tests the following:
  - lines travel both ways over the connection
  - the connection counts bytes waiting without taking them
  - the peer's end of the stream is an orderly close
  - closing the session closes an owned connection, and its socket
*/
bool
d_tests_sa_starttls_tcp_exchange(
    struct d_test_counter* _counter
)
{
    const char* const         TEST       = "TCP exchange";
    struct d_starttls_session session    = { .state = D_STARTTLS_STATE_CLOSED };
    struct d_tcp_connection   connection = { .socket = D_TCP_SOCKET_INVALID };
    int                       sockets[2] = { -1, -1 };
    const char*               line       = NULL;
    size_t                    length     = 0;
    bool                      result     = true;

    if (!d_tests_net_assert(d_tests_starttls_tcp_pair(&connection,
                                                      sockets),
                            TEST,
                            "a TCP connection over a socket pair",
                            _counter))
    {
        return false;
    }

    const bool talked  =
        ( (d_tests_starttls_tcp_session(&session,
                                        &connection,
                                        true))            &&
          (d_tests_starttls_write_text(sockets[1],
                                       "220 ready\r\n"))  &&
          (d_tests_starttls_line_is(&session,
                                    "220 ready"))         &&
          (d_starttls_send_line(&session,
                                "QUIT") == D_STARTTLS_OK) &&
          (d_tests_starttls_read_text(sockets[1],
                                      "QUIT\r\n")) );
    const bool counted = ( (talked) &&
                           (d_tests_starttls_counts(&session,
                                                    sockets[1])) );

    (void)close(sockets[1]);

    const bool ended   =
        ( (counted)                                                  &&
          (d_starttls_recv_line(&session,
                                &line,
                                &length) == D_STARTTLS_ERROR_CLOSED) &&
          (d_starttls_get_state(&session) ==
           D_STARTTLS_STATE_PLAINTEXT)                               &&
          (d_starttls_close(&session) == D_STARTTLS_OK)              &&
          (!d_tcp_connection_is_open(&connection))                   &&
          (d_tests_starttls_is_closed(sockets[0])) );

    d_starttls_destroy(&session);
    d_tcp_connection_close(&connection);
    result = d_tests_net_assert(talked,
                                TEST,
                                "lines both ways",
                                _counter) && result;
    result = d_tests_net_assert(counted,
                                TEST,
                                "waiting bytes are counted, not taken",
                                _counter) && result;
    result = d_tests_net_assert(ended,
                                TEST,
                                "the peer's end; close closes the socket",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_tcp_ownership
  Tests the following:
  - a borrowed connection outlives the session, its socket open
  - an owned one is destroyed by destroy when close was never called, its
    socket closed
*/
bool
d_tests_sa_starttls_tcp_ownership(
    struct d_test_counter* _counter
)
{
    const char* const         TEST       = "TCP ownership";
    struct d_starttls_session session    = { .state = D_STARTTLS_STATE_CLOSED };
    struct d_tcp_connection   connection = { .socket = D_TCP_SOCKET_INVALID };
    int                       sockets[2] = { -1, -1 };
    bool                      result     = true;

    if (!d_tests_net_assert(d_tests_starttls_tcp_pair(&connection,
                                                      sockets),
                            TEST,
                            "a TCP connection over a socket pair",
                            _counter))
    {
        return false;
    }

    const bool lent = d_tests_starttls_tcp_session(&session,
                                                   &connection,
                                                   false);

    d_starttls_destroy(&session);

    const bool kept  = ( (lent)                                  &&
                         (d_tcp_connection_is_open(&connection)) &&
                         (!d_tests_starttls_is_closed(sockets[0])) );
    const bool owned = d_tests_starttls_tcp_session(&session,
                                                    &connection,
                                                    true);

    d_starttls_destroy(&session);

    const bool gone = ( (owned)                                  &&
                        (!d_tcp_connection_is_open(&connection)) &&
                        (d_tests_starttls_is_closed(sockets[0])) );

    d_tcp_connection_close(&connection);
    (void)close(sockets[1]);
    result = d_tests_net_assert(kept,
                                TEST,
                                "a borrowed connection outlives the session",
                                _counter) && result;
    result = d_tests_net_assert(gone,
                                TEST,
                                "destroy closes an owned connection",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_tcp_deadline
  Tests the following:
  - the connection's deadline ends a receive that waits too long with a
    fatal D_STARTTLS_ERROR_TIMED_OUT, the connection's TIMED_OUT kept as
    its cause
*/
bool
d_tests_sa_starttls_tcp_deadline(
    struct d_test_counter* _counter
)
{
    const char* const         TEST       = "TCP deadline";
    struct d_starttls_session session    = { .state = D_STARTTLS_STATE_CLOSED };
    struct d_tcp_connection   connection = { .socket = D_TCP_SOCKET_INVALID };
    int                       sockets[2] = { -1, -1 };
    const char*               line       = NULL;
    size_t                    length     = 0;
    bool                      result     = true;

    if (!d_tests_net_assert(d_tests_starttls_tcp_pair(&connection,
                                                      sockets),
                            TEST,
                            "a TCP connection over a socket pair",
                            _counter))
    {
        return false;
    }

    const bool expired =
        ( (d_tcp_connection_set_timeout(&connection,
                                        50L) == D_NET_OK)           &&
          (d_tests_starttls_tcp_session(&session,
                                        &connection,
                                        true))                      &&
          (d_starttls_recv_line(&session,
                                &line,
                                &length) ==
           D_STARTTLS_ERROR_TIMED_OUT)                              &&
          (d_starttls_net_error(&session) == D_NET_ERROR_TIMED_OUT) &&
          (d_starttls_get_state(&session) ==
           D_STARTTLS_STATE_FAILED) );

    d_starttls_destroy(&session);
    d_tcp_connection_close(&connection);
    (void)close(sockets[1]);
    result = d_tests_net_assert(expired,
                                TEST,
                                "an expired deadline is a fatal timeout",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_tcp_vanished
  Tests the following:
  - a send to a peer that has gone is fatal: the connection was lost, so
    D_STARTTLS_ERROR_CLOSED, with the reset kept as the cause
  - and raises no SIGPIPE
*/
bool
d_tests_sa_starttls_tcp_vanished(
    struct d_test_counter* _counter
)
{
    const char* const         TEST       = "TCP vanished";
    struct d_starttls_session session    = { .state = D_STARTTLS_STATE_CLOSED };
    struct d_tcp_connection   connection = { .socket = D_TCP_SOCKET_INVALID };
    struct sigaction          catcher    = { .sa_flags = 0 };
    struct sigaction          previous   = { .sa_flags = 0 };
    int                       sockets[2] = { -1, -1 };
    bool                      result     = true;

    if (!d_tests_net_assert(d_tests_starttls_tcp_pair(&connection,
                                                      sockets),
                            TEST,
                            "a TCP connection over a socket pair",
                            _counter))
    {
        return false;
    }

    catcher.sa_handler = d_tests_starttls_on_sigpipe;
    (void)sigemptyset(&catcher.sa_mask);
    g_sigpipes = 0;

    const bool listening = (sigaction(SIGPIPE,
                                      &catcher,
                                      &previous) == 0);

    (void)close(sockets[1]);

    const bool failed =
        ( (listening)                                               &&
          (d_tests_starttls_tcp_session(&session,
                                        &connection,
                                        true))                      &&
          (d_starttls_send_line(&session,
                                "NOOP") == D_STARTTLS_ERROR_CLOSED) &&
          (d_starttls_net_error(&session) ==
           D_NET_ERROR_CONNECTION_RESET)                            &&
          (d_starttls_get_state(&session) == D_STARTTLS_STATE_FAILED) );

    d_starttls_destroy(&session);
    d_tcp_connection_close(&connection);

    if (listening)
    {
        (void)sigaction(SIGPIPE,
                        &previous,
                        NULL);
    }

    result = d_tests_net_assert(failed,
                                TEST,
                                "a vanished peer is a fatal lost connection",
                                _counter) && result;
    result = d_tests_net_assert(g_sigpipes == 0,
                                TEST,
                                "and raises no SIGPIPE",
                                _counter) && result;

    return result;
}
