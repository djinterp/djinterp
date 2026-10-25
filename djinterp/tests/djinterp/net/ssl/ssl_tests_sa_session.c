/*******************************************************************************
* djinterp [net]                                          ssl_tests_sa_session.c
*
* Tests of the SSL kernel's sessions over a net.h connection.
*   Each test opens a client and a server session over a pipe and drives
* them in turn: through a handshake, a transfer, the ways a stream ends, and
* the ways a peer is refused. The transport is an ordinary connection
* throughout, so these are also the tests of how the pump reads and writes
* one.
*
*
* path:      /tests/djinterp/net/ssl/ssl_tests_sa_session.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssl_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memcmp, memset, strcmp, strlen
// djinterp
#include "../net_tests_sa_support.h"  // d_tests_net_pattern


/*
d_tests_ssl_ready_pair
  Opens a pair over the run's contexts and handshakes it; false if either
step failed.
*/
static bool
d_tests_ssl_ready_pair(
    struct d_tests_ssl_pair* _pair,
    const char*              _host
)
{
    struct d_tests_ssl_world* const world  = d_tests_ssl_world();
    enum d_ssl_error                client = D_SSL_OK;
    enum d_ssl_error                server = D_SSL_OK;

    return ( (world)                        &&
             (d_tests_ssl_pair_open(_pair,
                                    &world->client,
                                    &world->server,
                                    _host)) &&
             (d_tests_ssl_pair_handshake(_pair,
                                         &client,
                                         &server)) );
}

// d_tests_ssl_broken
//   struct: a transport whose writes all succeed and whose reads fail with
// `error`, for a session that meets a broken connection.
struct d_tests_ssl_broken
{
    struct d_net_connection base;
    enum d_net_error        error;
};

/*
d_tests_ssl_broken_read
  Fails with the error it was given.
*/
static struct d_net_io_result
d_tests_ssl_broken_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    (void)_buffer;
    (void)_size;

    return d_net_io_result_make(0u,
                                ((struct d_tests_ssl_broken*)_connection)
                                ->error);
}

/*
d_tests_ssl_broken_write
  Takes everything.
*/
static struct d_net_io_result
d_tests_ssl_broken_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    (void)_connection;
    (void)_data;

    return d_net_io_result_make(_size,
                                D_NET_OK);
}

/*
d_tests_ssl_broken_is_open
  Always open: the failure is in its reads.
*/
static bool
d_tests_ssl_broken_is_open(
    const struct d_net_connection* _connection
)
{
    (void)_connection;

    return true;
}

/*
d_tests_ssl_broken_close
  Nothing to release.
*/
static void
d_tests_ssl_broken_close(
    struct d_net_connection* _connection
)
{
    (void)_connection;

    return;
}

/*
BROKEN_OPERATIONS
  The broken transport's table: the four required operations.
*/
static const struct d_net_connection_vtable BROKEN_OPERATIONS =
{
    d_tests_ssl_broken_read,
    d_tests_ssl_broken_write,
    d_tests_ssl_broken_is_open,
    d_tests_ssl_broken_close,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
PARTIAL_OPERATIONS
  A table without its required write, which a session must refuse.
*/
static const struct d_net_connection_vtable PARTIAL_OPERATIONS =
{
    d_tests_ssl_broken_read,
    NULL,
    d_tests_ssl_broken_is_open,
    d_tests_ssl_broken_close,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
d_tests_ssl_init_hosts
  The host half of d_tests_sa_ssl_init: a server names none, a verifying
client must name one, and a name must be well formed.
*/
static bool
d_tests_ssl_init_hosts(
    struct d_tests_ssl_world* _world,
    struct d_test_counter*    _counter
)
{
    struct d_net_pipe        pipe    = { .rings[0].size = 0u };
    unsigned char            a[64]   = { 0 };
    unsigned char            b[64]   = { 0 };
    struct d_ssl_session     session = { .owns_transport = false };
    const struct d_pack_text named   = { "localhost", 9u };
    const struct d_pack_text none    = { "", 0u };
    const struct d_pack_text bad     = { "bad..name", 9u };

    (void)d_net_pipe_init(&pipe,
                          a,
                          sizeof(a),
                          b,
                          sizeof(b));

    struct d_net_connection* const end = d_net_pipe_connection(&pipe,
                                                               D_NET_PIPE_A);

    const bool refused =
        ( (d_ssl_session_init(&session,
                              &_world->server,
                              end,
                              named) == D_SSL_ERROR_INVALID_ARGUMENT) &&
          (d_ssl_session_init(&session,
                              &_world->client,
                              end,
                              none) == D_SSL_ERROR_INVALID_ARGUMENT)  &&
          (d_ssl_session_init(&session,
                              &_world->client,
                              end,
                              bad) == D_SSL_ERROR_MALFORMED) );

    d_ssl_session_destroy(&session);

    return d_tests_net_assert(refused,
                              "d_ssl_session_init",
                              "hosts: none for servers, valid for clients",
                              _counter);
}

/*
d_tests_sa_ssl_init
  Tests the following:
  - NULL arguments, and a transport without a required operation, are
    refused
  - a refused session is still a connection, a closed one, whose reads
    report CLOSED
  - hosts (d_tests_ssl_init_hosts)
*/
bool
d_tests_sa_ssl_init(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world   = d_tests_ssl_world();
    struct d_tests_ssl_broken       broken  = { { NULL }, D_NET_OK };
    struct d_tests_ssl_broken       partial = { { &PARTIAL_OPERATIONS },
                                                D_NET_OK };
    struct d_ssl_session            session = { .owns_transport = false };
    const struct d_pack_text        host    = { "localhost", 9u };
    unsigned char                   byte    = 0u;
    bool                            result  = true;

    (void)d_net_connection_init(&broken.base,
                                &BROKEN_OPERATIONS);

    const bool refused =
        ( (world)                                                    &&
          (d_ssl_session_init(NULL,
                              &world->client,
                              &broken.base,
                              host) == D_SSL_ERROR_INVALID_ARGUMENT) &&
          (d_ssl_session_init(&session,
                              NULL,
                              &broken.base,
                              host) == D_SSL_ERROR_INVALID_ARGUMENT) &&
          (d_ssl_session_init(&session,
                              &world->client,
                              NULL,
                              host) == D_SSL_ERROR_INVALID_ARGUMENT) &&
          (d_ssl_session_init(&session,
                              &world->client,
                              &partial.base,
                              host) == D_SSL_ERROR_INVALID_ARGUMENT) );
    const bool closed =
        ( (!d_net_connection_is_open(&session.base)) &&
          (d_net_connection_read(&session.base,
                                 &byte,
                                 1u).error == D_NET_ERROR_CLOSED) );

    result = d_tests_net_assert(refused,
                                "d_ssl_session_init",
                                "NULLs and incomplete transports are refused",
                                _counter) && result;
    result = d_tests_net_assert(closed,
                                "d_ssl_session_init",
                                "a refused session is a closed connection",
                                _counter) && result;

    return ( (world)                            &&
             (d_tests_ssl_init_hosts(world,
                                     _counter)) &&
             (result) );
}

/*
d_tests_sa_ssl_handshake
  Tests the following:
  - a handshake over a pipe cutting every read and write to 7 bytes, with
    both sides blocking on it in turn, completes
  - the client knows the version, the cipher, and the server's certificate;
    the server checked nothing of the client
  - neither side waits on anything afterwards
*/
bool
d_tests_sa_ssl_handshake(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world  = d_tests_ssl_world();
    struct d_tests_ssl_pair         pair   = { .pipe.rings[0].size = 0u };
    struct d_ssl_info               client = { .version = D_SSL_VERSION_NONE };
    struct d_ssl_info               server = { .version = D_SSL_VERSION_NONE };
    enum d_ssl_error                cs     = D_SSL_OK;
    enum d_ssl_error                ss     = D_SSL_OK;

    const bool opened = ( (world) &&
                          (d_tests_ssl_pair_open(&pair,
                                                 &world->client,
                                                 &world->server,
                                                 "localhost")) );

    d_net_pipe_set_chunk(&pair.pipe,
                         D_NET_PIPE_A,
                         7u);
    d_net_pipe_set_chunk(&pair.pipe,
                         D_NET_PIPE_B,
                         7u);

    const bool shaken =
        ( (opened)                                                  &&
          (d_tests_ssl_pair_handshake(&pair,
                                      &cs,
                                      &ss))                         &&
          (d_ssl_session_state(&pair.client) ==
           D_SSL_STATE_ESTABLISHED)                                 &&
          (d_ssl_session_wants(&pair.client) == D_SSL_WANT_NOTHING) &&
          (d_ssl_session_wants(&pair.server) == D_SSL_WANT_NOTHING) &&
          (d_ssl_session_info(&pair.client,
                              &client))                             &&
          (d_ssl_session_info(&pair.server,
                              &server)) );
    const bool learned =
        ( (shaken)                                  &&
          (client.version >= D_SSL_VERSION_TLS1_2)  &&
          (client.cipher[0] != '\0')                &&
          (client.has_peer_certificate)             &&
          (client.verify == D_SSL_VERIFY_FLAG_NONE) &&
          (server.verify == D_SSL_VERIFY_FLAG_NOT_PERFORMED) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert( (learned),
                               "d_ssl_session_handshake",
                               "completes 7 bytes at a time, and learns",
                               _counter);
}

/*
d_tests_sa_ssl_exchange
  Tests the following:
  - 100,000 bytes from the client and 50,000 from the server arrive whole
    and in order, through rings far smaller than either
  - each side counts ciphertext both ways
*/
bool
d_tests_sa_ssl_exchange(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair = { .pipe.rings[0].size = 0u };

    const bool moved =
        ( (d_tests_ssl_ready_pair(&pair,
                                  "localhost")) &&
          (d_tests_ssl_pair_move(&pair.client,
                                 &pair.server,
                                 100000u))      &&
          (d_tests_ssl_pair_move(&pair.server,
                                 &pair.client,
                                 50000u))       &&
          (pair.client.bytes_out > 100000u)     &&
          (pair.client.bytes_in > 50000u) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert(moved,
                              "d_ssl_session_read and write",
                              "both ways, whole and in order",
                              _counter);
}

/*
d_tests_sa_ssl_resume
  Tests the following, over the OpenSSL engine:
  - a resumption offer is refused for a NULL argument, a server's session,
    a session that is not yet established to offer, sessions of different
    contexts, and a session already past its handshake
  - after the first session has read the server's data -- and with it, under
    TLS 1.3, the server's session tickets -- a second session offered the
    first resumes it, and says so; the first did not
  It runs past 75 lines because both pairs, and a third context, serve every
  check.
*/
bool
d_tests_sa_ssl_resume(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world   = d_tests_ssl_world();
    struct d_tests_ssl_pair         first   = { .pipe.rings[0].size = 0u };
    struct d_tests_ssl_pair         second  = { .pipe.rings[0].size = 0u };
    struct d_ssl_context            other   = { .engine = NULL };
    struct d_ssl_config             config  = { .role = D_SSL_ROLE_CLIENT };
    struct d_ssl_info               info    = { .resumed = false };
    enum d_ssl_error                client  = D_SSL_OK;
    enum d_ssl_error                server  = D_SSL_OK;
    bool                            result  = true;

    // the world's contexts, and a second client context
    if (!d_tests_net_assert(world != NULL,
                            "d_ssl_session_resume",
                            "the test world is open",
                            _counter))
    {
        return false;
    }

    d_tests_ssl_client_config(&config);

    const bool opened =
        ( (d_tests_ssl_pair_open(&first,
                                 &world->client,
                                 &world->server,
                                 "localhost")) &&
          (d_tests_ssl_pair_open(&second,
                                 &world->client,
                                 &world->server,
                                 "localhost")) &&
          (d_tests_ssl_context(&other,
                               &config)) );

    result = d_tests_net_assert(
                 ( (opened)                       &&
                   (d_ssl_session_resume(NULL,
                                         &first.client) ==
                    D_SSL_ERROR_INVALID_ARGUMENT) &&
                   (d_ssl_session_resume(&second.client,
                                         NULL) ==
                    D_SSL_ERROR_INVALID_ARGUMENT) &&
                   (d_ssl_session_resume(&second.server,
                                         &first.server) ==
                    D_SSL_ERROR_INVALID_ARGUMENT) &&
                   (d_ssl_session_resume(&second.client,
                                         &first.client) ==
                    D_SSL_ERROR_INVALID_ARGUMENT) ),
                 "d_ssl_session_resume",
                 "refuses NULL, a server, and a session not established",
                 _counter) && result;

    const bool resumed =
        ( (d_tests_ssl_pair_handshake(&first,
                                      &client,
                                      &server))             &&
          (d_tests_ssl_pair_move(&first.server,
                                 &first.client,
                                 64u))                      &&
          (d_ssl_session_resume(&second.client,
                                &first.client) == D_SSL_OK) &&
          (d_tests_ssl_pair_handshake(&second,
                                      &client,
                                      &server))             &&
          (d_ssl_session_info(&second.client,
                              &info))                       &&
          (info.resumed) );

    result = d_tests_net_assert(resumed,
                                "d_ssl_session_resume",
                                "a second session resumes the first",
                                _counter) && result;

    const bool first_full = ( (d_ssl_session_info(&first.client,
                                                  &info)) &&
                              (!info.resumed) );

    result = d_tests_net_assert(
                 ( (first_full)                                   &&
                   (d_ssl_session_resume(&second.client,
                                         &first.client) ==
                    D_SSL_ERROR_INVALID_ARGUMENT) ),
                 "d_ssl_session_resume",
                 "the first was full; an established session takes no offer",
                 _counter) && result;
    d_tests_ssl_pair_close(&second);

    // a session of another context is refused
    struct d_ssl_session stranger = { .context = NULL };
    const struct d_pack_text host = { "localhost",
                                      9u };

    result = d_tests_net_assert(
                 ( (d_ssl_session_init(&stranger,
                                       &other,
                                       d_net_pipe_connection(&first.pipe,
                                                             D_NET_PIPE_A),
                                       host) == D_SSL_OK)            &&
                   (d_ssl_session_resume(&stranger,
                                         &first.client) ==
                    D_SSL_ERROR_INVALID_ARGUMENT) ),
                 "d_ssl_session_resume",
                 "refuses a session of another context",
                 _counter) && result;
    d_ssl_session_destroy(&stranger);
    d_tests_ssl_pair_close(&first);
    d_ssl_context_destroy(&other);

    return result;
}

/*
d_tests_ssl_collect
  Reads `_size` bytes at a pair's server into `_out`, flushing the client
whenever it waits to write; false on any failure, or if the rounds run out.
*/
static bool
d_tests_ssl_collect(
    struct d_tests_ssl_pair* _pair,
    unsigned char*           _out,
    size_t                   _size
)
{
    size_t received = 0u;
    bool   going    = true;

    // turn by turn, until everything has arrived
    for (unsigned int round = 0u;
         ( (going)            &&
           (received < _size) &&
           (round < 100000u) );
         ++round)
    {
        size_t                 count = 0u;
        enum d_ssl_error       flush = D_SSL_OK;
        const enum d_ssl_error read  = d_ssl_session_read(&_pair->server,
                                                          _out + received,
                                                          _size - received,
                                                          &count);

        received += count;

        // the client's ciphertext goes on once the pipe has room
        if (d_ssl_session_wants(&_pair->client) == D_SSL_WANT_WRITE)
        {
            flush = d_ssl_session_flush(&_pair->client);
        }

        going = ( ( (read == D_SSL_OK) ||
                    (read == D_SSL_ERROR_WOULD_BLOCK) ) &&
                  ( (flush == D_SSL_OK) ||
                    (flush == D_SSL_ERROR_WOULD_BLOCK) ) );
    }

    return (received == _size);
}

// d_tests_ssl_record
//   struct: a whole record's plaintext, as sent and as it arrived.
struct d_tests_ssl_record
{
    unsigned char sent[D_SSL_RECORD_PLAINTEXT_MAX];
    unsigned char seen[D_SSL_RECORD_PLAINTEXT_MAX];
};

/*
d_tests_sa_ssl_wants
  Tests the following:
  - an established session awaits nothing
  - a read with nothing to read would block, awaiting the transport's
    bytes
  - a write of a whole record -- more than the session's stage holds --
    into a pipe that takes less is taken whole, and awaits the transport's
    room for the rest
  - the rest goes out as the peer reads, whole, and then nothing is awaited
*/
bool
d_tests_sa_ssl_wants(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_record record = { { 0 }, { 0 } };
    struct d_tests_ssl_pair   pair   = { .pipe.rings[0].size = 0u };
    unsigned char             byte   = 0u;
    size_t                    count  = 0u;
    size_t                    taken  = 0u;
    bool                      result = true;

    d_tests_net_pattern(record.sent,
                        sizeof(record.sent),
                        3u);

    const bool ready   = d_tests_ssl_ready_pair(&pair,
                                                "localhost");
    const bool waiting =
        ( (ready)                                                   &&
          (d_ssl_session_wants(&pair.client) == D_SSL_WANT_NOTHING) &&
          (d_ssl_session_read(&pair.server,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_WOULD_BLOCK)   &&
          (d_ssl_session_wants(&pair.server) == D_SSL_WANT_READ)    &&
          (d_ssl_session_write(&pair.client,
                               record.sent,
                               sizeof(record.sent),
                               &taken) == D_SSL_OK)                 &&
          (taken == sizeof(record.sent))                            &&
          (d_ssl_session_wants(&pair.client) == D_SSL_WANT_WRITE) );
    const bool arrived =
        ( (waiting)                                       &&
          (d_tests_ssl_collect(&pair,
                               record.seen,
                               sizeof(record.seen)))      &&
          (memcmp(record.seen,
                  record.sent,
                  sizeof(record.sent)) == 0)              &&
          (d_ssl_session_flush(&pair.client) == D_SSL_OK) &&
          (d_ssl_session_wants(&pair.client) == D_SSL_WANT_NOTHING) );

    d_tests_ssl_pair_close(&pair);

    result = d_tests_net_assert(waiting,
                                "d_ssl_session_wants",
                                "a blocked call says what it awaits",
                                _counter) && result;

    return d_tests_net_assert(arrived,
                              "d_ssl_session_wants",
                              "a record past the stage goes out whole",
                              _counter) && result;
}

/*
d_tests_sa_ssl_close_notify
  Tests the following:
  - the server's close_notify reaches the client as CLOSED, on every read
  - the client answers with its own, and both sessions are then CLOSED
  - a write after this end's close_notify is refused
*/
bool
d_tests_sa_ssl_close_notify(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair  = { .pipe.rings[0].size = 0u };
    unsigned char           byte  = 0u;
    size_t                  count = 0u;

    const bool ready = d_tests_ssl_ready_pair(&pair,
                                              "localhost");
    const bool ended =
        ( (ready)                                                   &&
          (d_ssl_session_shutdown(&pair.server) == D_SSL_OK)        &&
          (d_ssl_session_read(&pair.client,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_CLOSED)        &&
          (d_ssl_session_read(&pair.client,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_CLOSED)        &&
          (d_ssl_session_shutdown(&pair.client) == D_SSL_OK)        &&
          (d_ssl_session_state(&pair.client) == D_SSL_STATE_CLOSED) &&
          (d_ssl_session_read(&pair.server,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_CLOSED)        &&
          (d_ssl_session_state(&pair.server) == D_SSL_STATE_CLOSED) &&
          (d_ssl_session_write(&pair.client,
                               "x",
                               1u,
                               &count) == D_SSL_ERROR_STATE) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert(ended,
                              "close_notify",
                              "an orderly end, both ways",
                              _counter);
}

/*
d_tests_sa_ssl_truncation
  Tests the following:
  - a transport that ends without close_notify is TRUNCATED, never CLOSED,
    and the failure sticks
  - the transport's last word was the end of its stream: D_NET_OK
*/
bool
d_tests_sa_ssl_truncation(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_pair pair  = { .pipe.rings[0].size = 0u };
    unsigned char           byte  = 0u;
    size_t                  count = 0u;

    const bool ready = d_tests_ssl_ready_pair(&pair,
                                              "localhost");

    // the server's end goes without a word
    d_net_connection_close(d_net_pipe_connection(&pair.pipe,
                                                 D_NET_PIPE_B));

    const bool truncated =
        ( (ready)                                                        &&
          (d_ssl_session_read(&pair.client,
                              &byte,
                              1u,
                              &count) == D_SSL_ERROR_TRUNCATED)          &&
          (d_ssl_session_state(&pair.client) == D_SSL_STATE_FAILED)      &&
          (d_ssl_session_failure(&pair.client) == D_SSL_ERROR_TRUNCATED) &&
          (d_ssl_session_flush(&pair.client) == D_SSL_ERROR_TRUNCATED)   &&
          (d_ssl_session_transport_error(&pair.client) == D_NET_OK) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert(truncated,
                              "truncation",
                              "an end without close_notify is TRUNCATED",
                              _counter);
}

/*
d_tests_sa_ssl_transport_failure
  Tests the following:
  - a transport whose reads fail makes the handshake fail with TRANSPORT,
    and the session keeps the transport's own error
  - a session that has not failed reports no failure
  - no session at all reads as FAILED, with no failure, no transport
    error, and nothing awaited
*/
bool
d_tests_sa_ssl_transport_failure(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world   = d_tests_ssl_world();
    struct d_tests_ssl_broken       broken  = { { NULL },
                                                D_NET_ERROR_TIMED_OUT };
    struct d_ssl_session            session = { .owns_transport = false };
    const struct d_pack_text        host    = { "localhost", 9u };

    const bool nothing =
        ( (d_ssl_session_state(NULL) == D_SSL_STATE_FAILED) &&
          (d_ssl_session_failure(NULL) == D_SSL_OK)         &&
          (d_ssl_session_transport_error(NULL) == D_NET_OK) &&
          (d_ssl_session_wants(NULL) == D_SSL_WANT_NOTHING) );

    (void)d_net_connection_init(&broken.base,
                                &BROKEN_OPERATIONS);

    const bool failed =
        ( (world)                                                      &&
          (d_ssl_session_init(&session,
                              &world->client,
                              &broken.base,
                              host) == D_SSL_OK)                       &&
          (d_ssl_session_failure(&session) == D_SSL_OK)                &&
          (d_ssl_session_handshake(&session) == D_SSL_ERROR_TRANSPORT) &&
          (d_ssl_session_failure(&session) == D_SSL_ERROR_TRANSPORT)   &&
          (d_ssl_session_transport_error(&session) ==
           D_NET_ERROR_TIMED_OUT) );

    d_ssl_session_destroy(&session);

    return ( (d_tests_net_assert(failed,
                                 "transport failures",
                                 "TRANSPORT, with the transport's own error",
                                 _counter)) &&
             (d_tests_net_assert(nothing,
                                 "transport failures",
                                 "no session has failed in name only",
                                 _counter)) );
}

/*
d_tests_ssl_refused
  Handshakes a pair whose client names `_host`, expecting the client to
refuse the server with VERIFY and the flag `_flag`.
*/
static bool
d_tests_ssl_refused(
    const struct d_ssl_context* _client,
    const struct d_ssl_context* _server,
    const char*                 _host,
    uint32_t                    _flag
)
{
    struct d_tests_ssl_pair pair = { .pipe.rings[0].size = 0u };
    struct d_ssl_info       info = { .version = D_SSL_VERSION_NONE };
    enum d_ssl_error        cs   = D_SSL_OK;
    enum d_ssl_error        ss   = D_SSL_OK;

    const bool refused =
        ( (d_tests_ssl_pair_open(&pair,
                                 _client,
                                 _server,
                                 _host))     &&
          (!d_tests_ssl_pair_handshake(&pair,
                                       &cs,
                                       &ss)) &&
          (cs == D_SSL_ERROR_VERIFY)         &&
          (d_ssl_session_info(&pair.client,
                              &info))        &&
          ((info.verify & _flag) != 0u) );

    d_tests_ssl_pair_close(&pair);

    return refused;
}

/*
d_tests_sa_ssl_names
  Tests the following:
  - a server not named for the host the client meant is refused
  - an IP literal is checked against the certificate's addresses, and the
    server's 127.0.0.1 is accepted
  - one trailing dot on a name changes nothing
*/
bool
d_tests_sa_ssl_names(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world  = d_tests_ssl_world();
    struct d_tests_ssl_pair         pair   = { .pipe.rings[0].size = 0u };
    bool                            result = true;

    const bool mismatch =
        ( (world)                                                    &&
          (d_tests_ssl_refused(&world->client,
                               &world->server,
                               "other.example",
                               D_SSL_VERIFY_FLAG_HOSTNAME_MISMATCH)) &&
          (d_tests_ssl_refused(&world->client,
                               &world->server,
                               "127.0.0.2",
                               D_SSL_VERIFY_FLAG_HOSTNAME_MISMATCH)) );
    const bool address = d_tests_ssl_ready_pair(&pair,
                                                "127.0.0.1");

    d_tests_ssl_pair_close(&pair);

    const bool dotted = d_tests_ssl_ready_pair(&pair,
                                               "localhost.");

    d_tests_ssl_pair_close(&pair);

    result = d_tests_net_assert(mismatch,
                                "host names",
                                "a server named otherwise is refused",
                                _counter) && result;
    result = d_tests_net_assert( ( (address) &&
                                   (dotted) ),
                                 "host names",
                                 "addresses and trailing dots match",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_ssl_trust
  Tests the following:
  - a certificate from an untrusted CA, and an expired one, are refused
  - verification is off only when asked, and then the untrusted server is
    accepted unauthenticated
*/
bool
d_tests_sa_ssl_trust(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world    = d_tests_ssl_world();
    struct d_ssl_config             config   = { .verify_hostname = false };
    struct d_ssl_context            rogue    = { .engine = NULL };
    struct d_ssl_context            expired  = { .engine = NULL };
    struct d_ssl_context            trusting = { .engine = NULL };
    struct d_tests_ssl_pair         pair     = { .pipe.rings[0].size = 0u };
    struct d_ssl_info               info     = { .resumed = false };
    enum d_ssl_error                cs       = D_SSL_OK;
    enum d_ssl_error                ss       = D_SSL_OK;

    d_tests_ssl_server_config(&config,
                              (world) ? world->pki.rogue_cert : NULL,
                              (world) ? world->pki.rogue_key : NULL);

    const bool made = ( (world) &&
                        (d_tests_ssl_context(&rogue,
                                             &config)) );

    d_tests_ssl_server_config(&config,
                              (world) ? world->pki.expired_cert : NULL,
                              (world) ? world->pki.expired_key : NULL);

    const bool refused =
        ( (made)                                             &&
          (d_tests_ssl_context(&expired,
                               &config))                     &&
          (d_tests_ssl_refused(&world->client,
                               &rogue,
                               "localhost",
                               D_SSL_VERIFY_FLAG_UNTRUSTED)) &&
          (d_tests_ssl_refused(&world->client,
                               &expired,
                               "localhost",
                               D_SSL_VERIFY_FLAG_EXPIRED)) );

    d_tests_ssl_client_config(&config);
    config.verify          = D_SSL_VERIFY_NONE;
    config.verify_hostname = false;

    const bool unverified =
        ( (made)                               &&
          (d_tests_ssl_context(&trusting,
                               &config))       &&
          (d_tests_ssl_pair_open(&pair,
                                 &trusting,
                                 &rogue,
                                 "localhost")) &&
          (d_tests_ssl_pair_handshake(&pair,
                                      &cs,
                                      &ss))    &&
          (d_ssl_session_info(&pair.client,
                              &info))          &&
          (info.verify == D_SSL_VERIFY_FLAG_NOT_PERFORMED) );

    d_tests_ssl_pair_close(&pair);
    d_ssl_context_destroy(&trusting);
    d_ssl_context_destroy(&expired);
    d_ssl_context_destroy(&rogue);

    return ( (d_tests_net_assert(refused,
                                 "trust",
                                 "untrusted and expired servers are refused",
                                 _counter)) &&
             (d_tests_net_assert(unverified,
                                 "trust",
                                 "verification is off only when asked",
                                 _counter)) );
}

/*
d_tests_ssl_alpn_context
  A context like the world's client or server, offering the ALPN wire list
`_list`.
*/
static bool
d_tests_ssl_alpn_context(
    struct d_ssl_context* _out,
    bool                  _server,
    const char*           _list,
    size_t                _length
)
{
    struct d_tests_ssl_world* const world  = d_tests_ssl_world();
    struct d_ssl_config             config = { .verify_hostname = false };

    // the world's settings for the side
    if (_server)
    {
        d_tests_ssl_server_config(&config,
                                  world->pki.server_cert,
                                  world->pki.server_key);
    }
    else
    {
        d_tests_ssl_client_config(&config);
    }

    config.alpn.data = (const unsigned char*)_list;
    config.alpn.size = _length;

    return d_tests_ssl_context(_out,
                               &config);
}

/*
d_tests_sa_ssl_alpn
  Tests the following:
  - a client offering h2 and http/1.1 and a server choosing from
    http/1.1 alone agree on http/1.1, and both sessions say so
*/
bool
d_tests_sa_ssl_alpn(
    struct d_test_counter* _counter
)
{
    static const char       CLIENT_LIST[] = "\x02h2\x08http/1.1";
    static const char       SERVER_LIST[] = "\x08http/1.1";
    struct d_ssl_context    client        = { .engine = NULL };
    struct d_ssl_context    server        = { .engine = NULL };
    struct d_tests_ssl_pair pair          = { .pipe.rings[0].size = 0u };
    struct d_ssl_info       seen          = { .version = D_SSL_VERSION_NONE };
    struct d_ssl_info       chosen        = { .version = D_SSL_VERSION_NONE };
    enum d_ssl_error        cs            = D_SSL_OK;
    enum d_ssl_error        ss            = D_SSL_OK;

    const bool agreed =
        ( (d_tests_ssl_world())                                &&
          (d_tests_ssl_alpn_context(&client,
                                    false,
                                    CLIENT_LIST,
                                    sizeof(CLIENT_LIST) - 1u)) &&
          (d_tests_ssl_alpn_context(&server,
                                    true,
                                    SERVER_LIST,
                                    sizeof(SERVER_LIST) - 1u)) &&
          (d_tests_ssl_pair_open(&pair,
                                 &client,
                                 &server,
                                 "localhost"))                 &&
          (d_tests_ssl_pair_handshake(&pair,
                                      &cs,
                                      &ss))                    &&
          (d_ssl_session_info(&pair.client,
                              &seen))                          &&
          (d_ssl_session_info(&pair.server,
                              &chosen))                        &&
          (strcmp(seen.alpn,
                  "http/1.1") == 0)                            &&
          (strcmp(chosen.alpn,
                  "http/1.1") == 0) );

    d_tests_ssl_pair_close(&pair);
    d_ssl_context_destroy(&client);
    d_ssl_context_destroy(&server);

    return d_tests_net_assert(agreed,
                              "ALPN",
                              "negotiated, and read on both sides",
                              _counter);
}

/*
d_tests_sa_ssl_plaintext_first
  Tests the following:
  - a pipe that has carried plaintext both ways carries a handshake and
    data afterwards, as a STARTTLS upgrade needs
*/
bool
d_tests_sa_ssl_plaintext_first(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world    = d_tests_ssl_world();
    struct d_tests_ssl_pair         pair     = { .pipe.rings[0].size = 0u };
    char                            read[16] = { 0 };
    enum d_ssl_error                cs       = D_SSL_OK;
    enum d_ssl_error                ss       = D_SSL_OK;

    const bool opened = ( (world) &&
                          (d_tests_ssl_pair_open(&pair,
                                                 &world->client,
                                                 &world->server,
                                                 "localhost")) );

    struct d_net_connection* const a = d_net_pipe_connection(&pair.pipe,
                                                             D_NET_PIPE_A);
    struct d_net_connection* const b = d_net_pipe_connection(&pair.pipe,
                                                             D_NET_PIPE_B);

    const bool upgraded =
        ( (opened)                               &&
          (d_net_write_all(a,
                           "STARTTLS\r\n",
                           10u,
                           NULL) == D_NET_OK)    &&
          (d_net_read_exactly(b,
                              read,
                              10u,
                              NULL) == D_NET_OK) &&
          (d_net_write_all(b,
                           "220 Go\r\n",
                           8u,
                           NULL) == D_NET_OK)    &&
          (d_net_read_exactly(a,
                              read,
                              8u,
                              NULL) == D_NET_OK) &&
          (memcmp(read,
                  "220 Go\r\n",
                  8u) == 0)                      &&
          (d_tests_ssl_pair_handshake(&pair,
                                      &cs,
                                      &ss))      &&
          (d_tests_ssl_pair_move(&pair.client,
                                 &pair.server,
                                 1000u)) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert(upgraded,
                              "upgrades",
                              "plaintext, then TLS, on one connection",
                              _counter);
}

/*
d_tests_sa_ssl_not_tls
  Tests the following:
  - a server whose peer speaks plaintext where the handshake should begin
    fails with NOT_TLS, its engine never having seen the bytes
*/
bool
d_tests_sa_ssl_not_tls(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world = d_tests_ssl_world();
    struct d_tests_ssl_pair         pair  = { .pipe.rings[0].size = 0u };

    const bool opened = ( (world) &&
                          (d_tests_ssl_pair_open(&pair,
                                                 &world->client,
                                                 &world->server,
                                                 "localhost")) );
    const bool sniffed =
        ( (opened)                                                       &&
          (d_net_write_all(d_net_pipe_connection(&pair.pipe,
                                                 D_NET_PIPE_A),
                           "EHLO client.example\r\n",
                           21u,
                           NULL) == D_NET_OK)                            &&
          (d_ssl_session_handshake(&pair.server) == D_SSL_ERROR_NOT_TLS) &&
          (d_ssl_session_failure(&pair.server) == D_SSL_ERROR_NOT_TLS) );

    d_tests_ssl_pair_close(&pair);

    return d_tests_net_assert(sniffed,
                              "sniffing",
                              "plaintext where TLS belongs is NOT_TLS",
                              _counter);
}
