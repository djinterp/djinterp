/*******************************************************************************
* djinterp [net]                                             ssl_tests_sa_view.c
*
* Tests of a session as a net.h connection.
*   Each test reaches the sessions only through their base, as code written
* for a socket would: net.h's transfers and framing, the stream's ends, a
* closed or destroyed session, the transport's addresses, and a session
* running over another session's view.
*
*
* path:      /tests/djinterp/net/ssl/ssl_tests_sa_view.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssl_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memcmp, strcmp
// djinterp
#include "../net_tests_sa_support.h"  // d_tests_net_mock


/*
d_tests_ssl_view_pair
  Opens a pair over the run's contexts and handshakes it; false if either
step failed.
*/
static bool
d_tests_ssl_view_pair(
    struct d_tests_ssl_pair* _pair
)
{
    struct d_tests_ssl_world* const world  = d_tests_ssl_world();
    enum d_ssl_error                client = D_SSL_OK;
    enum d_ssl_error                server = D_SSL_OK;

    return ( (world)                              &&
             (d_tests_ssl_pair_open(_pair,
                                    &world->client,
                                    &world->server,
                                    "localhost")) &&
             (d_tests_ssl_pair_handshake(_pair,
                                         &client,
                                         &server)) );
}

/*
d_tests_ssl_view_is
  Whether a result moved `_count` bytes and reported `_error`.
*/
static bool
d_tests_ssl_view_is(
    struct d_net_io_result _result,
    size_t                 _count,
    enum d_net_error       _error
)
{
    return ( (_result.count == _count) &&
             (_result.error == _error) );
}

/*
d_tests_sa_ssl_view_transfers
  Tests the following:
  - write_all and read_exactly carry bytes through the two views
  - a frame written to one view is read whole from the other
  - an empty view reads as WOULD_BLOCK, as its pipe does
*/
bool
d_tests_sa_ssl_view_transfers(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair       = { .pipe.rings[0].size = 0u };
    char                    read[400]  = { 0 };
    char                    frame[300] = { 0 };
    size_t                  length     = 0u;

    // a frame no record boundary lines up with
    for (size_t i = 0u;
         i < sizeof(frame);
         ++i)
    {
        frame[i] = (char)('a' + (i % 26u));
    }

    const bool ready = d_tests_ssl_view_pair(&pair);
    const bool moved =
        ( (ready)                                        &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.server.base,
                                                     read,
                                                     8u),
                               0u,
                               D_NET_ERROR_WOULD_BLOCK)) &&
          (d_net_write_all(&pair.client.base,
                           "over tls",
                           8u,
                           NULL) == D_NET_OK)            &&
          (d_net_read_exactly(&pair.server.base,
                              read,
                              8u,
                              NULL) == D_NET_OK)         &&
          (memcmp(read,
                  "over tls",
                  8u) == 0) );
    const bool framed =
        ( (moved)                                          &&
          (d_net_write_frame(&pair.server.base,
                             frame,
                             sizeof(frame),
                             D_NET_FRAME_MAX) == D_NET_OK) &&
          (d_net_read_frame(&pair.client.base,
                            read,
                            sizeof(read),
                            &length) == D_NET_OK)          &&
          (length == sizeof(frame))                        &&
          (memcmp(read,
                  frame,
                  sizeof(frame)) == 0) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert(framed,
                              "session views",
                              "net.h's transfers and frames run over TLS",
                              _counter);
}

/*
d_tests_sa_ssl_view_ends
  Tests the following:
  - the peer's close_notify is the end of the stream, a count of 0 without
    an error, on every read
  - the view stays open until it is closed; closed, it reads, writes and
    shuts down nothing, and closing it again does nothing
  - truncation reads as a stream that ended too soon
*/
bool
d_tests_sa_ssl_view_ends(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair   = { .pipe.rings[0].size = 0u };
    unsigned char           byte   = 0u;
    bool                    result = true;

    const bool ended =
        ( (d_tests_ssl_view_pair(&pair))                     &&
          (d_ssl_session_shutdown(&pair.server) == D_SSL_OK) &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.client.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_OK))                    &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.client.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_OK))                    &&
          (d_net_connection_is_open(&pair.client.base)) );

    d_net_connection_close(&pair.client.base);
    d_net_connection_close(&pair.client.base);

    const bool closed =
        ( (ended)                                        &&
          (!d_net_connection_is_open(&pair.client.base)) &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.client.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_ERROR_CLOSED))      &&
          (d_tests_ssl_view_is(d_net_connection_write(&pair.client.base,
                                                      "x",
                                                      1u),
                               0u,
                               D_NET_ERROR_CLOSED))      &&
          (d_net_connection_shutdown(&pair.client.base,
                                     D_NET_SHUTDOWN_BOTH) ==
           D_NET_ERROR_CLOSED) );

    d_tests_ssl_pair_close(&pair);

    const bool ready = d_tests_ssl_view_pair(&pair);

    d_net_connection_close(d_net_pipe_connection(&pair.pipe,
                                                 D_NET_PIPE_B));

    const bool truncated =
        ( (ready)                                                        &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.client.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_ERROR_CLOSED))                      &&
          (d_ssl_session_failure(&pair.client) == D_SSL_ERROR_TRUNCATED) &&
          (!d_net_connection_is_open(&pair.client.base)) );

    d_tests_ssl_pair_close(&pair);

    result = d_tests_net_assert(closed,
                                "session views",
                                "close_notify ends the stream; close closes",
                                _counter) && result;

    return d_tests_net_assert(truncated,
                              "session views",
                              "truncation is a stream ended too soon",
                              _counter) && result;
}

/*
d_tests_ssl_view_mock
  A client session over `_mock`, a full mock given `_output` for the
session's hello; false if the session could not be made.
*/
static bool
d_tests_ssl_view_mock(
    struct d_ssl_session*    _session,
    struct d_tests_net_mock* _mock,
    unsigned char*           _output,
    size_t                   _capacity
)
{
    struct d_tests_ssl_world* const world = d_tests_ssl_world();
    const struct d_pack_text        host  = { "localhost", 9u };

    d_tests_net_mock_init(_mock,
                          true);
    d_tests_net_mock_output(_mock,
                            _output,
                            _capacity);

    return ( (world) &&
             (d_ssl_session_init(_session,
                                 &world->client,
                                 &_mock->base,
                                 host) == D_SSL_OK) );
}

/*
d_tests_sa_ssl_view_failures
  Tests the following:
  - a transport whose read fails surfaces through the view as its own
    error, a timeout here, rather than as TLS's
  - a transport whose write takes nothing and says nothing surfaces as
    UNKNOWN, TRANSPORT's mapping, never as a write that succeeded
*/
bool
d_tests_sa_ssl_view_failures(
    struct d_test_counter* _counter
)
{
    struct d_tests_net_mock mock         = { .open = false };
    struct d_ssl_session    session      = { .owns_transport = false };
    unsigned char           output[4096] = { 0 };
    unsigned char           byte         = 0u;
    bool                    result       = true;

    const bool made  = d_tests_ssl_view_mock(&session,
                                             &mock,
                                             output,
                                             sizeof(output));

    mock.read_error = D_NET_ERROR_TIMED_OUT;

    const bool timed =
        ( (made) &&
          (d_tests_ssl_view_is(d_net_connection_read(&session.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_ERROR_TIMED_OUT)) );

    d_ssl_session_destroy(&session);

    const bool again   = d_tests_ssl_view_mock(&session,
                                               &mock,
                                               output,
                                               0u);
    const bool stalled =
        ( (again)                                                    &&
          (d_tests_ssl_view_is(d_net_connection_write(&session.base,
                                                      "x",
                                                      1u),
                               0u,
                               D_NET_ERROR_UNKNOWN))                 &&
          (d_ssl_session_failure(&session) == D_SSL_ERROR_TRANSPORT) &&
          (d_ssl_session_transport_error(&session) == D_NET_OK) );

    d_ssl_session_destroy(&session);

    result = d_tests_net_assert(timed,
                                "session views",
                                "a transport's error is its own",
                                _counter) && result;

    return d_tests_net_assert(stalled,
                              "session views",
                              "a write that takes nothing is a failure",
                              _counter) && result;
}

/*
d_tests_ssl_view_closed_once
  Whether a session told to own its transport, a mock, closes it once
however often its view is closed.
*/
static bool
d_tests_ssl_view_closed_once(void)
{
    struct d_tests_net_mock mock       = { .open = false };
    struct d_ssl_session    session    = { .owns_transport = false };
    unsigned char           output[64] = { 0 };

    const bool made = d_tests_ssl_view_mock(&session,
                                            &mock,
                                            output,
                                            sizeof(output));

    d_ssl_session_own_transport(&session,
                                true);
    d_net_connection_close(&session.base);
    d_net_connection_close(&session.base);

    const bool once = ( (made) &&
                        (mock.closes == 1u) );

    d_ssl_session_destroy(&session);

    return once;
}

/*
d_tests_sa_ssl_view_ownership
  Tests the following:
  - closing a view sends close_notify, which the peer reads
  - a session borrows its transport: closing the view leaves it open
  - a session told to own its transport closes it with the view, once
    however often the view is closed
*/
bool
d_tests_sa_ssl_view_ownership(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair  = { .pipe.rings[0].size = 0u };
    unsigned char           byte  = 0u;
    size_t                  count = 0u;

    const bool borrowed_ready = d_tests_ssl_view_pair(&pair);

    d_net_connection_close(&pair.client.base);

    const bool borrowed =
        ( (borrowed_ready)                                                &&
          (d_net_connection_is_open(d_net_pipe_connection(&pair.pipe,
                                                          D_NET_PIPE_A))) &&
          (d_ssl_session_read(&pair.server,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_CLOSED) );

    d_tests_ssl_pair_close(&pair);

    const bool owned_ready = d_tests_ssl_view_pair(&pair);

    d_ssl_session_own_transport(&pair.client,
                                true);
    d_ssl_session_own_transport(NULL,
                                true);
    d_net_connection_close(&pair.client.base);

    const bool owned =
        ( (owned_ready)                                                    &&
          (!d_net_connection_is_open(d_net_pipe_connection(&pair.pipe,
                                                           D_NET_PIPE_A))) &&
          (d_ssl_session_read(&pair.server,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_CLOSED) );

    d_tests_ssl_pair_close(&pair);

    return ( (d_tests_net_assert(borrowed,
                                 "session views",
                                 "a borrowed transport outlives the view",
                                 _counter)) &&
             (d_tests_net_assert(owned,
                                 "session views",
                                 "an owned transport closes with it",
                                 _counter)) &&
             (d_tests_net_assert(d_tests_ssl_view_closed_once(),
                                 "session views",
                                 "an owned transport closes once",
                                 _counter)) );
}

/*
d_tests_ssl_view_failed_shutdown
  Whether a session its transport failed answers a shutdown of its view with
the transport's error, leaving the transport alone.
*/
static bool
d_tests_ssl_view_failed_shutdown(void)
{
    struct d_tests_net_mock mock         = { .open = false };
    struct d_ssl_session    session      = { .owns_transport = false };
    unsigned char           output[4096] = { 0 };

    const bool made = d_tests_ssl_view_mock(&session,
                                            &mock,
                                            output,
                                            sizeof(output));

    mock.read_error = D_NET_ERROR_TIMED_OUT;

    const bool refused =
        ( (made)                                                       &&
          (d_ssl_session_handshake(&session) == D_SSL_ERROR_TRANSPORT) &&
          (d_net_connection_shutdown(&session.base,
                                     D_NET_SHUTDOWN_WRITE) ==
           D_NET_ERROR_TIMED_OUT)                                      &&
          (mock.shutdowns == 0u) );

    d_ssl_session_destroy(&session);

    return refused;
}

/*
d_tests_sa_ssl_view_shutdown
  Tests the following:
  - shutting down WRITE sends close_notify, which the peer reads, then shuts
    the transport's sending down, and later writes report a closed
    connection
  - shutting down READ reaches the transport, whose end then truncates the
    TLS stream
  - a failed session answers with its failure, the transport untouched
*/
bool
d_tests_sa_ssl_view_shutdown(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair  = { .pipe.rings[0].size = 0u };
    unsigned char           byte  = 0u;
    size_t                  count = 0u;

    const bool written =
        ( (d_tests_ssl_view_pair(&pair))                                &&
          (d_net_connection_shutdown(&pair.client.base,
                                     D_NET_SHUTDOWN_WRITE) == D_NET_OK) &&
          (d_ssl_session_read(&pair.server,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_CLOSED)            &&
          (d_tests_ssl_view_is(d_net_connection_read(
                                   d_net_pipe_connection(&pair.pipe,
                                                         D_NET_PIPE_B),
                                   &byte,
                                   1u),
                               0u,
                               D_NET_OK))                               &&
          (d_tests_ssl_view_is(d_net_connection_write(&pair.client.base,
                                                      "x",
                                                      1u),
                               0u,
                               D_NET_ERROR_CLOSED)) );

    d_tests_ssl_pair_close(&pair);

    const bool read =
        ( (d_tests_ssl_view_pair(&pair))                               &&
          (d_net_connection_shutdown(&pair.client.base,
                                     D_NET_SHUTDOWN_READ) == D_NET_OK) &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.client.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_ERROR_CLOSED))                    &&
          (d_ssl_session_failure(&pair.client) == D_SSL_ERROR_TRUNCATED) );

    d_tests_ssl_pair_close(&pair);

    return ( (d_tests_net_assert(written,
                                 "session views",
                                 "WRITE sends close_notify",
                                 _counter)) &&
             (d_tests_net_assert(read,
                                 "session views",
                                 "READ reaches the transport",
                                 _counter)) &&
             (d_tests_net_assert(d_tests_ssl_view_failed_shutdown(),
                                 "session views",
                                 "a failed session answers with its failure",
                                 _counter)) );
}

/*
d_tests_sa_ssl_view_destroyed
  Tests the following:
  - a destroyed session is a closed connection: it reads, writes and shuts
    down nothing, has no address, and closes without trouble
*/
bool
d_tests_sa_ssl_view_destroyed(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair    = { .pipe.rings[0].size = 0u };
    struct d_net_endpoint   address = { .port = 0u };
    unsigned char           byte    = 0u;

    const bool ready = d_tests_ssl_view_pair(&pair);

    d_tests_ssl_pair_close(&pair);
    d_net_connection_close(&pair.client.base);

    const bool closed =
        ( (ready)                                                   &&
          (!d_net_connection_is_open(&pair.client.base))            &&
          (d_tests_ssl_view_is(d_net_connection_read(&pair.client.base,
                                                     &byte,
                                                     1u),
                               0u,
                               D_NET_ERROR_CLOSED))                 &&
          (d_tests_ssl_view_is(d_net_connection_write(&pair.client.base,
                                                      "x",
                                                      1u),
                               0u,
                               D_NET_ERROR_CLOSED))                 &&
          (d_net_connection_shutdown(&pair.client.base,
                                     D_NET_SHUTDOWN_WRITE) ==
           D_NET_ERROR_CLOSED)                                      &&
          (!d_net_connection_remote_endpoint(&pair.client.base,
                                             &address))             &&
          (d_ssl_session_transport_error(&pair.client) == D_NET_OK) &&
          (d_ssl_session_failure(&pair.client) == D_SSL_OK) );

    return d_tests_net_assert(closed,
                              "session views",
                              "a destroyed session is a closed connection",
                              _counter);
}

/*
d_tests_sa_ssl_view_endpoints
  Tests the following:
  - a view's addresses are its transport's: the scripted connection's
    remote one, and none where the transport has none
*/
bool
d_tests_sa_ssl_view_endpoints(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world   = d_tests_ssl_world();
    struct d_tests_net_mock         mock    = { .open = false };
    struct d_ssl_session            session = { .owns_transport = false };
    struct d_net_endpoint           address = { .port = 0u };
    const struct d_pack_text        host    = { "localhost", 9u };

    d_tests_net_mock_init(&mock,
                          true);

    const bool passed =
        ( (world)                                      &&
          (d_ssl_session_init(&session,
                              &world->client,
                              &mock.base,
                              host) == D_SSL_OK)       &&
          (d_net_connection_remote_endpoint(&session.base,
                                            &address)) &&
          (strcmp(address.host,
                  "192.0.2.1") == 0)                   &&
          (address.port == 80u)                        &&
          (!d_net_connection_local_endpoint(&session.base,
                                            &address)) );

    d_ssl_session_destroy(&session);

    return d_tests_net_assert(passed,
                              "session views",
                              "addresses are the transport's",
                              _counter);
}

/*
d_tests_ssl_nested_handshake
  Handshakes two sessions in turn, as d_tests_ssl_pair_handshake does, for
sessions that are not a pair's.
*/
static bool
d_tests_ssl_nested_handshake(
    struct d_ssl_session* _client,
    struct d_ssl_session* _server
)
{
    enum d_ssl_error client = D_SSL_ERROR_WOULD_BLOCK;
    enum d_ssl_error server = D_SSL_ERROR_WOULD_BLOCK;

    // turn by turn, until neither side would block
    for (unsigned int round = 0u;
         ( (round < 100000u) &&
           ( (client == D_SSL_ERROR_WOULD_BLOCK) ||
             (server == D_SSL_ERROR_WOULD_BLOCK) ) );
         ++round)
    {
        // the client, while it waits
        if (client == D_SSL_ERROR_WOULD_BLOCK)
        {
            client = d_ssl_session_handshake(_client);
        }

        // the server, likewise
        if (server == D_SSL_ERROR_WOULD_BLOCK)
        {
            server = d_ssl_session_handshake(_server);
        }
    }

    return ( (client == D_SSL_OK) &&
             (server == D_SSL_OK) );
}

/*
d_tests_sa_ssl_nested
  Tests the following:
  - a session runs over another session's view: TLS inside TLS shakes
    hands and carries 20,000 bytes, the outer pair carrying the inner's
    records as its plaintext
*/
bool
d_tests_sa_ssl_nested(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world        = d_tests_ssl_world();
    struct d_tests_ssl_pair         outer        = { .pipe.rings[0].size = 0u };
    struct d_ssl_session            inner_client = { .owns_transport = false };
    struct d_ssl_session            inner_server = { .owns_transport = false };
    const struct d_pack_text        host         = { "localhost", 9u };
    const struct d_pack_text        none         = { "", 0u };

    const bool nested =
        ( (d_tests_ssl_view_pair(&outer))               &&
          (d_ssl_session_init(&inner_client,
                              &world->client,
                              &outer.client.base,
                              host) == D_SSL_OK)        &&
          (d_ssl_session_init(&inner_server,
                              &world->server,
                              &outer.server.base,
                              none) == D_SSL_OK)        &&
          (d_tests_ssl_nested_handshake(&inner_client,
                                        &inner_server)) &&
          (d_tests_ssl_pair_move(&inner_client,
                                 &inner_server,
                                 20000u))               &&
          (outer.client.bytes_out > 20000u) );

    d_ssl_session_destroy(&inner_client);
    d_ssl_session_destroy(&inner_server);
    d_tests_ssl_pair_close(&outer);

    return d_tests_net_assert(nested,
                              "session views",
                              "a session runs over another session",
                              _counter);
}
