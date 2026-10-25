/*******************************************************************************
* djinterp [net]                                     starttls_tests_sa_support.c
*
* Support for the STARTTLS module's standalone tests.
*   The SMTP-like negotiator, the on_upgrade recorder, option and fixture
* helpers, and the socket pairs the tests over TCP adopt. The scripted peer
* and the toy engine are net_tests_peer.c's, and the certificates
* net_tests_pki.c's.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/
// enable POSIX.1-2008 (socketpair, fcntl) under a strict C compiler; a
// feature-test macro must precede every include, so it sits ahead of the
// corresponding header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./starttls_tests_sa.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF
#include <stdio.h>   // snprintf
#include <string.h>  // strcmp, strlen, strncmp
// posix
#include <fcntl.h>       // fcntl, F_GETFD
#include <sys/socket.h>  // socketpair, setsockopt, AF_UNIX, SOCK_STREAM
#include <sys/time.h>    // struct timeval
#include <unistd.h>      // close


// negotiation
/*
d_tests_starttls_smtp_request
  STARTTLS, or the failure the test asked for.
*/
static enum d_starttls_error
d_tests_starttls_smtp_request(
    struct d_starttls_session* _session,
    void*                      _context
)
{
    struct d_tests_starttls_smtp* smtp = _context;

    smtp->requests += 1;

    if (smtp->request_status != D_STARTTLS_OK)
    {
        return smtp->request_status;
    }

    return d_starttls_send_line(_session,
                                "STARTTLS");
}

/*
d_tests_starttls_smtp_reply
  Reads the reply, as a line or as `exact` bytes, and classifies it by its
code.
*/
static enum d_starttls_error
d_tests_starttls_smtp_reply(
    struct d_starttls_session* _session,
    void*                      _context,
    enum d_starttls_reply*     _reply
)
{
    struct d_tests_starttls_smtp* smtp = _context;

    smtp->replies += 1;

    if (smtp->reply_status != D_STARTTLS_OK)
    {
        return smtp->reply_status;
    }

    enum d_starttls_error status = D_STARTTLS_OK;

    // the whole line, or exactly the bytes asked for
    if (smtp->exact > 0)
    {
        const size_t size = (smtp->exact < sizeof(smtp->reply))
                                ? smtp->exact
                                : (sizeof(smtp->reply) - 1);

        status = d_starttls_recv_exact(_session,
                                       smtp->reply,
                                       size);
        smtp->reply[(status == D_STARTTLS_OK) ? size : 0] = '\0';
    }
    else
    {
        const char* line   = NULL;
        size_t      length = 0;

        status = d_starttls_recv_line(_session,
                                      &line,
                                      &length);
        (void)snprintf(smtp->reply,
                       sizeof(smtp->reply),
                       "%s",
                       (status == D_STARTTLS_OK) ? line : "");
    }

    if (status != D_STARTTLS_OK)
    {
        return status;
    }

    // 220 proceeds, 454 refuses, and nothing else is understood
    *_reply = D_STARTTLS_REPLY_MALFORMED;

    if (strncmp(smtp->reply,
                "220",
                3) == 0)
    {
        *_reply = D_STARTTLS_REPLY_PROCEED;
    }
    else if (strncmp(smtp->reply,
                     "454",
                     3) == 0)
    {
        *_reply = D_STARTTLS_REPLY_REFUSED;
    }

    return D_STARTTLS_OK;
}

// d_tests_starttls_smtp_negotiator
//   constant: the SMTP-like negotiator.
const struct d_starttls_negotiator d_tests_starttls_smtp_negotiator =
{
    .name    = "SMTP-like",
    .request = d_tests_starttls_smtp_request,
    .reply   = d_tests_starttls_smtp_reply
};

/*
d_tests_starttls_forget
  Records the call; what a protocol module would discard here, the test has
none of.
*/
void
d_tests_starttls_forget(
    struct d_starttls_session* _session,
    void*                      _context
)
{
    struct d_tests_starttls_forget* forget = _context;

    forget->calls  += 1;
    forget->session = _session;
    forget->secure  = d_starttls_is_secure(_session);
    forget->epoch   = d_starttls_epoch(_session);

    return;
}

// sessions and sockets
/*
d_tests_starttls_options
  The module's defaults, with what every scripted session sets.
*/
struct d_starttls_options
d_tests_starttls_options(
    enum d_starttls_role        _role,
    enum d_starttls_policy      _policy,
    const struct d_ssl_context* _tls
)
{
    struct d_starttls_options options = d_starttls_options_default(_role);

    options.policy      = _policy;
    options.tls         = _tls;
    options.server_name = (_role == D_STARTTLS_ROLE_CLIENT)
                              ? D_TESTS_NET_TOY_SERVER_NAME
                              : NULL;

    return options;
}

/*
d_tests_starttls_plain
  A client whose policy is DISABLED needs no context, so it suits every test
of plaintext I/O. It owns the peer's connection.
*/
bool
d_tests_starttls_plain(
    struct d_starttls_session* _session,
    struct d_tests_net_peer*   _peer,
    size_t                     _buffer_bytes,
    bool                       _strict_crlf
)
{
    struct d_starttls_options options =
        d_tests_starttls_options(D_STARTTLS_ROLE_CLIENT,
                                 D_STARTTLS_POLICY_DISABLED,
                                 NULL);

    if (_buffer_bytes > 0)
    {
        options.buffer_bytes = _buffer_bytes;
    }

    options.strict_crlf     = _strict_crlf;
    options.owns_connection = true;

    return (d_starttls_init(_session,
                            &options,
                            d_tests_net_peer_connection(_peer,
                                                        true)) ==
            D_STARTTLS_OK);
}

/*
d_tests_starttls_line_is
  Receives one line and compares it, length included.
*/
bool
d_tests_starttls_line_is(
    struct d_starttls_session* _session,
    const char*                _expected
)
{
    const char* line   = NULL;
    size_t      length = 0;

    return ( (d_starttls_recv_line(_session,
                                   &line,
                                   &length) == D_STARTTLS_OK) &&
             (length == strlen(_expected))                    &&
             (strcmp(line,
                     _expected) == 0) );
}

/*
d_tests_starttls_fixture_open
  A peer, a toy context for `_role` that verifies as a client, the peer's
counting connection, and options for `_role` and `_policy` that own it.
*/
bool
d_tests_starttls_fixture_open(
    struct d_tests_starttls_fixture* _fixture,
    enum d_starttls_role             _role,
    enum d_starttls_policy           _policy
)
{
    const struct d_tests_net_toy toy    = { .create_status = D_SSL_OK };
    const bool                   client = (_role == D_STARTTLS_ROLE_CLIENT);

    _fixture->peer       = d_tests_net_peer_new();
    _fixture->toy        = toy;
    _fixture->connection = NULL;

    // the peer first, since everything else is over it
    if (!_fixture->peer)
    {
        return false;
    }

    _fixture->connection = d_tests_net_peer_connection(_fixture->peer,
                                                       true);
    _fixture->options    = d_tests_starttls_options(_role,
                                                    _policy,
                                                    &_fixture->tls);
    _fixture->options.owns_connection = true;

    // a context that is not made leaves nothing to destroy
    if (!d_tests_net_toy_context(&_fixture->tls,
                                 &_fixture->toy,
                                 (client) ? D_SSL_ROLE_CLIENT
                                 : D_SSL_ROLE_SERVER,
                                 client))
    {
        d_tests_net_peer_free(_fixture->peer);
        _fixture->peer = NULL;

        return false;
    }

    return true;
}

/*
d_tests_starttls_fixture_close
  Destroys the context and frees the peer; the rest is the caller's
storage.
*/
void
d_tests_starttls_fixture_close(
    struct d_tests_starttls_fixture* _fixture
)
{
    // a fixture that never opened holds nothing
    if (!_fixture->peer)
    {
        return;
    }

    d_ssl_context_destroy(&_fixture->tls);
    d_tests_net_peer_free(_fixture->peer);
    _fixture->peer = NULL;

    return;
}

/*
d_tests_starttls_deadline
  Sets one deadline on both directions of a socket, directly, so the helper
needs nothing of the module under test.
*/
static bool
d_tests_starttls_deadline(
    int  _socket,
    long _seconds
)
{
    const struct timeval deadline = { .tv_sec  = _seconds,
                                      .tv_usec = 0 };

    return ( (setsockopt(_socket,
                         SOL_SOCKET,
                         SO_RCVTIMEO,
                         &deadline,
                         (socklen_t)sizeof(deadline)) == 0) &&
             (setsockopt(_socket,
                         SOL_SOCKET,
                         SO_SNDTIMEO,
                         &deadline,
                         (socklen_t)sizeof(deadline)) == 0) );
}

/*
d_tests_starttls_socket_pair
  Both ends get a five-second deadline, so a test that goes wrong fails
instead of hanging.
*/
bool
d_tests_starttls_socket_pair(
    int _sockets[2]
)
{
    if (socketpair(AF_UNIX,
                   SOCK_STREAM,
                   0,
                   _sockets) != 0)
    {
        return false;
    }

    if ( (!d_tests_starttls_deadline(_sockets[0],
                                     5)) ||
         (!d_tests_starttls_deadline(_sockets[1],
                                     5)) )
    {
        (void)close(_sockets[0]);
        (void)close(_sockets[1]);

        return false;
    }

    return true;
}

/*
d_tests_starttls_is_closed
  A descriptor is closed when fcntl no longer knows it.
*/
bool
d_tests_starttls_is_closed(
    int _socket
)
{
    return ( (fcntl(_socket,
                    F_GETFD) == -1) &&
             (errno == EBADF) );
}

/*
d_tests_starttls_adopt
  The connection owns the socket from here on, even when adopting fails.
*/
bool
d_tests_starttls_adopt(
    struct d_tcp_connection* _connection,
    int                      _socket
)
{
    d_tcp_connection_init(_connection);

    return ( (d_tcp_connection_adopt(_connection,
                                     (d_tcp_socket)_socket,
                                     D_NET_PROTOCOL_UNIX) == D_NET_OK) &&
             (d_tcp_connection_set_timeout(_connection,
                                           5000L) == D_NET_OK) );
}
