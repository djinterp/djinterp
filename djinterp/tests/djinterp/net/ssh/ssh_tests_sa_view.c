/*******************************************************************************
* djinterp [net]                                             ssh_tests_sa_view.c
*
* Tests of a channel as a net.h connection.
*   Each test reaches the channel only through its base, as code written for
* a socket would: net.h's transfers and framing over a window and a script,
* the server's end of the stream, half and full closes, failures placed in
* the engine or elsewhere in the session, and channels that never opened.
*
*
* path:      /tests/djinterp/net/ssh/ssh_tests_sa_view.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssh_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memcmp, memset


/*
d_tests_sa_ssh_view_transfers
  Tests the following:
  - write_all crosses a window three bytes wide whole and in order, the
    channel looping on the window beneath one write through the view
  - read_exactly gathers a script served two bytes at a time, one engine
    read per read through the view
  - a frame written through the view, echoed, reads back whole
*/
bool
d_tests_sa_ssh_view_transfers(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture    = { .channel.exit_status = 0 };
    char                       read[256]  = { 0 };
    char                       frame[200] = { 0 };
    size_t                     length     = 0u;

    // a frame no read or write boundary lines up with
    for (size_t i = 0u; i < sizeof(frame); ++i)
    {
        frame[i] = (char)('a' + (i % 26u));
    }

    const bool                            open =
        d_tests_ssh_fixture_open(&fixture);
    struct d_tests_ssh_toy_channel* const toy  =
        d_tests_ssh_fixture_toy(&fixture);

    toy->write_chunk = 3u;
    toy->read_chunk  = 2u;
    (void)d_tests_ssh_toy_say(toy,
                              "from the server",
                              15u);

    const bool moved =
        ( (open)                                 &&
          (d_net_write_all(&fixture.channel.base,
                           "to the server",
                           13u,
                           NULL) == D_NET_OK)    &&
          (toy->output_size == 13u)              &&
          (memcmp(toy->output,
                  "to the server",
                  13u) == 0)                     &&
          (toy->writes == 5)                     &&
          (d_net_read_exactly(&fixture.channel.base,
                              read,
                              15u,
                              NULL) == D_NET_OK) &&
          (memcmp(read,
                  "from the server",
                  15u) == 0)                     &&
          (toy->reads == 8) );

    toy->echo        = true;
    toy->write_chunk = 0u;
    toy->read_chunk  = 7u;

    const bool framed =
        ( (moved)                                          &&
          (d_net_write_frame(&fixture.channel.base,
                             frame,
                             sizeof(frame),
                             D_NET_FRAME_MAX) == D_NET_OK) &&
          (d_net_read_frame(&fixture.channel.base,
                            read,
                            sizeof(read),
                            &length) == D_NET_OK)          &&
          (length == sizeof(frame))                        &&
          (memcmp(read,
                  frame,
                  sizeof(frame)) == 0) );

    d_tests_ssh_fixture_close(&fixture);

    return d_tests_net_assert(framed,
                              "channel views",
                              "net.h's transfers and frames run over SSH",
                              _counter);
}

/*
d_tests_sa_ssh_view_ends
  Tests the following:
  - the server's EOF is a read of nothing without an error, on every read,
    changing neither the channel, the session, nor `failure`
  - the view stays open past it
  - the view knows no endpoints and counts nothing available
*/
bool
d_tests_sa_ssh_view_ends(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture  = { .channel.exit_status = 0 };
    struct d_net_endpoint      endpoint = { .port = 0u };
    char                       read[64] = { 0 };
    size_t                     count    = 1u;
    const bool                 open     = d_tests_ssh_fixture_open(&fixture);

    (void)d_tests_ssh_toy_say(d_tests_ssh_fixture_toy(&fixture),
                              "last words",
                              10u);

    const bool ended =
        ( (open)                                            &&
          (d_tests_ssh_is(d_net_connection_read(&fixture.channel.base,
                                                read,
                                                sizeof(read)),
                          10u,
                          D_NET_OK))                        &&
          (d_tests_ssh_is(d_net_connection_read(&fixture.channel.base,
                                                read,
                                                sizeof(read)),
                          0u,
                          D_NET_OK))                        &&
          (d_tests_ssh_is(d_net_connection_read(&fixture.channel.base,
                                                read,
                                                sizeof(read)),
                          0u,
                          D_NET_OK))                        &&
          (d_net_connection_is_open(&fixture.channel.base)) &&
          (fixture.channel.state == D_SSH_CHANNEL_OPEN)     &&
          (fixture.channel.failure == D_SSH_OK)             &&
          (d_ssh_get_state(&fixture.session) ==
           D_SSH_STATE_AUTHENTICATED) );
    const bool unknown =
        ( (open)                                         &&
          (!d_net_connection_remote_endpoint(&fixture.channel.base,
                                             &endpoint)) &&
          (!d_net_connection_local_endpoint(&fixture.channel.base,
                                            &endpoint))  &&
          (!d_net_connection_available(&fixture.channel.base,
                                       &count))          &&
          (count == 0u) );

    d_tests_ssh_fixture_close(&fixture);

    return ( d_tests_net_assert(ended,
                                "channel views",
                                "the server's EOF reads as the end, open",
                                _counter) &&
             d_tests_net_assert(unknown,
                                "channel views",
                                "no endpoints, nothing counted",
                                _counter) );
}

/*
d_tests_ssh_view_half_close
  The first half of d_tests_sa_ssh_view_closing, over an open fixture whose
script holds "reply": the shutdowns, then a write and a read past EOF.
*/
static bool
d_tests_ssh_view_half_close(
    struct d_tests_ssh_fixture* _fixture
)
{
    struct d_tests_ssh_toy_channel* const toy     =
        d_tests_ssh_fixture_toy(_fixture);
    struct d_net_connection* const        base    = &_fixture->channel.base;
    char                                  read[8] = { 0 };

    return ( (d_net_connection_shutdown(base,
                                        D_NET_SHUTDOWN_READ) == D_NET_OK)  &&
             (toy->eofs == 0)                                              &&
             (d_net_connection_shutdown(base,
                                        D_NET_SHUTDOWN_WRITE) == D_NET_OK) &&
             (toy->eofs == 1)                                              &&
             (_fixture->channel.eof_sent)                                  &&
             (d_net_connection_shutdown(base,
                                        D_NET_SHUTDOWN_BOTH) == D_NET_OK)  &&
             (toy->eofs == 1)                                              &&
             (d_tests_ssh_is(d_net_connection_write(base,
                                                    "late",
                                                    4u),
                             0u,
                             D_NET_ERROR_CLOSED))                          &&
             (_fixture->channel.failure == D_SSH_ERROR_STATE)              &&
             (toy->writes == 0)                                            &&
             (d_tests_ssh_is(d_net_connection_read(base,
                                                   read,
                                                   sizeof(read)),
                             5u,
                             D_NET_OK))                                    &&
             (memcmp(read,
                     "reply",
                     5u) == 0)                                             &&
             (d_net_connection_is_open(base)) );
}

/*
d_tests_ssh_view_closed
  Whether a view is a closed connection: not open, and reading, writing and
shutting down nothing, in either direction.
*/
static bool
d_tests_ssh_view_closed(
    struct d_net_connection* _base
)
{
    char read[8] = { 0 };

    return ( (!d_net_connection_is_open(_base))   &&
             (d_tests_ssh_is(d_net_connection_read(_base,
                                                   read,
                                                   sizeof(read)),
                             0u,
                             D_NET_ERROR_CLOSED)) &&
             (d_tests_ssh_is(d_net_connection_write(_base,
                                                    "x",
                                                    1u),
                             0u,
                             D_NET_ERROR_CLOSED)) &&
             (d_net_connection_shutdown(_base,
                                        D_NET_SHUTDOWN_BOTH) ==
              D_NET_ERROR_CLOSED)                 &&
             (d_net_connection_shutdown(_base,
                                        D_NET_SHUTDOWN_READ) ==
              D_NET_ERROR_CLOSED) );
}

/*
d_tests_sa_ssh_view_closing
  Tests the following:
  - shutdown(READ) asks nothing of the server; WRITE sends EOF, once, and
    BOTH after it nothing more
  - past EOF a write is a closed connection's, kept as D_SSH_ERROR_STATE,
    and reading goes on
  - close sends no second EOF, closes the channel, records its exit status,
    and leaves a closed connection; closing again asks nothing more
  - destroy through the view closes the channel and leaves its storage to
    d_ssh_channel_free, after which the view is still a closed connection
*/
bool
d_tests_sa_ssh_view_closing(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture = { .channel.exit_status = 0 };
    const bool                 open    = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->exit_status = 7;
    (void)d_tests_ssh_toy_say(d_tests_ssh_fixture_toy(&fixture),
                              "reply",
                              5u);

    const bool half = ( (open) &&
                        (d_tests_ssh_view_half_close(&fixture)) );

    d_net_connection_close(&fixture.channel.base);

    const bool closed =
        ( (half)                                             &&
          (d_tests_ssh_fixture_toy(&fixture)->eofs == 1)     &&
          (d_tests_ssh_fixture_toy(&fixture)->closes == 1)   &&
          (d_ssh_channel_exit_status(&fixture.channel) == 7) &&
          (d_tests_ssh_view_closed(&fixture.channel.base)) );

    d_net_connection_close(&fixture.channel.base);

    const bool again =
        ( (closed)                                         &&
          (d_tests_ssh_fixture_toy(&fixture)->closes == 1) &&
          (d_tests_ssh_fixture_toy(&fixture)->eofs == 1) );

    d_tests_ssh_fixture_close(&fixture);

    const bool reopened = d_tests_ssh_fixture_open(&fixture);

    d_net_connection_destroy(&fixture.channel.base);

    const bool destroyed =
        ( (reopened)                                       &&
          (d_tests_ssh_fixture_toy(&fixture)->eofs == 1)   &&
          (d_tests_ssh_fixture_toy(&fixture)->closes == 1) &&
          (!d_tests_ssh_fixture_toy(&fixture)->freed) );

    d_ssh_channel_free(&fixture.channel);

    const bool freed =
        ( (destroyed)                                         &&
          (d_tests_ssh_fixture_toy(&fixture)->freed)          &&
          (d_ssh_channel_exit_status(&fixture.channel) == -1) &&
          (d_tests_ssh_view_closed(&fixture.channel.base)) );

    d_tests_ssh_fixture_close(&fixture);

    return ( d_tests_net_assert(again,
                                "channel views",
                                "half and full closes, each once",
                                _counter) &&
             d_tests_net_assert(freed,
                                "channel views",
                                "destroy closes and leaves the storage",
                                _counter) );
}

// d_tests_ssh_failure
//   struct: a status an engine call fails with, what the view reports for
// it, and whether it fails the channel.
struct d_tests_ssh_failure
{
    enum d_ssh_error status;
    enum d_net_error reported;
    bool             fatal;
};

// READ_FAILURES
//   constant: the statuses a read through the view is tried with.
static const struct d_tests_ssh_failure READ_FAILURES[] =
{
    { D_SSH_ERROR_TRANSPORT,     D_NET_ERROR_UNKNOWN,       true  },
    { D_SSH_ERROR_TIMED_OUT,     D_NET_ERROR_TIMED_OUT,     true  },
    { D_SSH_ERROR_PROTOCOL,      D_NET_ERROR_PROTOCOL,      true  },
    { D_SSH_ERROR_OUT_OF_MEMORY, D_NET_ERROR_OUT_OF_MEMORY, true  },
    { D_SSH_ERROR_CHANNEL,       D_NET_ERROR_UNKNOWN,       false },
    { D_SSH_ERROR_STATE,         D_NET_ERROR_CLOSED,        false }
};

/*
d_tests_ssh_view_failed
  Whether a channel failed by `_status` answers a read, a write and a
shutdown either way through its view with `_reported`, asking the engine
nothing more, with the status kept and the session failed too.
*/
static bool
d_tests_ssh_view_failed(
    struct d_tests_ssh_fixture* _fixture,
    enum d_ssh_error            _status,
    enum d_net_error            _reported
)
{
    struct d_net_connection* const        base    = &_fixture->channel.base;
    struct d_tests_ssh_toy_channel* const toy     =
        d_tests_ssh_fixture_toy(_fixture);
    const int                             asked   = toy->reads  +
                                                    toy->writes +
                                                    toy->eofs;
    char                                  read[8] = { 0 };

    return ( (d_tests_ssh_is(d_net_connection_read(base,
                                                   read,
                                                   sizeof(read)),
                             0u,
                             _reported))                         &&
             (d_tests_ssh_is(d_net_connection_write(base,
                                                    "x",
                                                    1u),
                             0u,
                             _reported))                         &&
             (d_net_connection_shutdown(base,
                                        D_NET_SHUTDOWN_WRITE) ==
              _reported)                                         &&
             (d_net_connection_shutdown(base,
                                        D_NET_SHUTDOWN_READ) ==
              _reported)                                         &&
             ( (toy->reads + toy->writes + toy->eofs) == asked ) &&
             (!d_net_connection_is_open(base))                   &&
             (_fixture->channel.state == D_SSH_CHANNEL_FAILED)   &&
             (_fixture->channel.failure == _status)              &&
             (d_ssh_get_state(&_fixture->session) ==
              D_SSH_STATE_FAILED) );
}

/*
d_tests_ssh_view_reads_on
  Whether a channel whose read failed with a status that is not fatal reads
its script on through its view, open, in a session untouched.
*/
static bool
d_tests_ssh_view_reads_on(
    struct d_tests_ssh_fixture* _fixture
)
{
    char read[8] = { 0 };

    return ( (d_tests_ssh_is(d_net_connection_read(&_fixture->channel.base,
                                                   read,
                                                   sizeof(read)),
                             4u,
                             D_NET_OK))                          &&
             (d_net_connection_is_open(&_fixture->channel.base)) &&
             (_fixture->channel.state == D_SSH_CHANNEL_OPEN)     &&
             (d_ssh_get_state(&_fixture->session) ==
              D_SSH_STATE_AUTHENTICATED) );
}

/*
d_tests_ssh_view_read_fails
  Opens a fixture whose engine's next read fails as `_failure` says, and
reads through the view: whether the read reported what it should, keeping
the status, and the channel was left as the status leaves it.
*/
static bool
d_tests_ssh_view_read_fails(
    const struct d_tests_ssh_failure* _failure
)
{
    struct d_tests_ssh_fixture fixture = { .channel.exit_status = 0 };
    char                       read[8] = { 0 };
    const bool                 open    = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->read_fails = _failure->status;
    (void)d_tests_ssh_toy_say(d_tests_ssh_fixture_toy(&fixture),
                              "next",
                              4u);

    const bool first =
        ( (open)                               &&
          (d_tests_ssh_is(d_net_connection_read(&fixture.channel.base,
                                                read,
                                                sizeof(read)),
                          0u,
                          _failure->reported)) &&
          (fixture.channel.failure == _failure->status) );
    const bool after =
        ( (first) &&
          ( (_failure->fatal) ? d_tests_ssh_view_failed(&fixture,
                                                        _failure->status,
                                                        _failure->reported)
            : d_tests_ssh_view_reads_on(&fixture) ) );

    d_tests_ssh_fixture_close(&fixture);

    return after;
}

/*
d_tests_ssh_view_write_fails
  Opens a fixture and, as `_eof` says, makes its engine's next write or EOF
fail with `_status`, then writes or shuts down through the view: whether
that reported `_reported`, keeping the status, and whether the channel is
then failed or still writes, as `_fatal` says it should be.
*/
static bool
d_tests_ssh_view_write_fails(
    enum d_ssh_error _status,
    enum d_net_error _reported,
    bool             _fatal,
    bool             _eof
)
{
    struct d_tests_ssh_fixture fixture = { .channel.exit_status = 0 };
    struct d_net_connection*   base    = &fixture.channel.base;
    const bool                 open    = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->write_fails = (_eof) ? D_SSH_OK
                                                            : _status;
    d_tests_ssh_fixture_toy(&fixture)->eof_fails   = (_eof) ? _status
                                                            : D_SSH_OK;

    const enum d_net_error first =
        (_eof) ? d_net_connection_shutdown(base,
                                           D_NET_SHUTDOWN_WRITE)
               : d_net_connection_write(base,
                                        "x",
                                        1u).error;
    const bool             kept  =
        ( (open)                               &&
          (first == _reported)                 &&
          (fixture.channel.failure == _status) &&
          ( (fixture.channel.state == D_SSH_CHANNEL_FAILED) ==
            _fatal ) );
    const bool             then  =
        ( (kept) &&
          ( (_fatal) ? d_tests_ssh_view_failed(&fixture,
                                               _status,
                                               _reported)
            : d_tests_ssh_is(d_net_connection_write(base,
                                                    "y",
                                                    1u),
                             1u,
                             D_NET_OK) ) );

    d_tests_ssh_fixture_close(&fixture);

    return then;
}

/*
d_tests_sa_ssh_view_failures
  Tests the following:
  - a read failing with a fatal status reports its d_ssh_error_to_net and
    fails the channel and session, which then answer every read, write and
    shutdown with it, asking the engine nothing more
  - D_SSH_ERROR_CHANNEL is UNKNOWN and leaves the channel reading on;
    D_SSH_ERROR_STATE from the engine, a channel it has closed, is CLOSED
  - a write or an EOF failing does the same, kept as a read's is
*/
bool
d_tests_sa_ssh_view_failures(
    struct d_test_counter* _counter
)
{
    bool reads = true;

    // each status a read is tried with
    for (size_t i = 0u;
         i < (sizeof(READ_FAILURES) / sizeof(READ_FAILURES[0]));
         ++i)
    {
        reads = d_tests_ssh_view_read_fails(&READ_FAILURES[i]) && reads;
    }

    const bool writes =
        ( (d_tests_ssh_view_write_fails(D_SSH_ERROR_TIMED_OUT,
                                        D_NET_ERROR_TIMED_OUT,
                                        true,
                                        false)) &&
          (d_tests_ssh_view_write_fails(D_SSH_ERROR_CHANNEL,
                                        D_NET_ERROR_UNKNOWN,
                                        false,
                                        false)) &&
          (d_tests_ssh_view_write_fails(D_SSH_ERROR_TRANSPORT,
                                        D_NET_ERROR_UNKNOWN,
                                        true,
                                        true)) );

    return ( d_tests_net_assert(reads,
                                "channel views",
                                "reads fail as their statuses say",
                                _counter) &&
             d_tests_net_assert(writes,
                                "channel views",
                                "writes and EOFs fail likewise",
                                _counter) );
}

/*
d_tests_ssh_view_gone
  Opens a fixture and a second channel of its session, which a fatal read
fails, and so the session with it: whether the fixture's channel was then a
closed connection, its own state untouched, and closed through its view had
no exit status to report.
*/
static bool
d_tests_ssh_view_gone(void)
{
    struct d_tests_ssh_fixture fixture  = { .channel.exit_status = 0 };
    struct d_ssh_channel       other    = { .exit_status = 0 };
    char                       read[8]  = { 0 };
    size_t                     received = 0u;
    const bool                 open     = d_tests_ssh_fixture_open(&fixture);

    fixture.toy.channels[1].read_fails = D_SSH_ERROR_TRANSPORT;

    const bool gone =
        ( (open)                                                   &&
          (d_ssh_channel_open(&fixture.session,
                              &other) == D_SSH_OK)                 &&
          (d_ssh_channel_read(&other,
                              D_SSH_STREAM_STDOUT,
                              read,
                              sizeof(read),
                              &received) == D_SSH_ERROR_TRANSPORT) &&
          (d_tests_ssh_view_closed(&fixture.channel.base))         &&
          (fixture.channel.failure == D_SSH_ERROR_STATE)           &&
          (fixture.channel.state == D_SSH_CHANNEL_OPEN) );

    d_net_connection_close(&fixture.channel.base);

    const bool no_status =
        ( (gone) &&
          (d_ssh_channel_exit_status(&fixture.channel) == -1) );

    d_ssh_channel_free(&other);
    d_tests_ssh_fixture_close(&fixture);

    return no_status;
}

/*
d_tests_sa_ssh_view_elsewhere
  Tests the following:
  - a channel failed outside its view answers through it with the status
    that failed it
  - a channel whose session failed through another channel is a closed
    connection, its own state untouched, and closed has no exit status
  - so is a channel whose session was disconnected, which the engine
    released with the session
*/
bool
d_tests_sa_ssh_view_elsewhere(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture  = { .channel.exit_status = 0 };
    char                       read[8]  = { 0 };
    size_t                     received = 0u;
    const bool                 open     = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->read_fails = D_SSH_ERROR_TIMED_OUT;

    const bool kept =
        ( (open)                                                   &&
          (d_ssh_channel_read(&fixture.channel,
                              D_SSH_STREAM_STDERR,
                              read,
                              sizeof(read),
                              &received) == D_SSH_ERROR_TIMED_OUT) &&
          (d_tests_ssh_view_failed(&fixture,
                                   D_SSH_ERROR_TIMED_OUT,
                                   D_NET_ERROR_TIMED_OUT)) );

    d_tests_ssh_fixture_close(&fixture);

    const bool reopened = d_tests_ssh_fixture_open(&fixture);
    const bool ended    =
        ( (reopened)                                       &&
          (d_ssh_disconnect(&fixture.session,
                            "done") == D_SSH_OK)           &&
          (d_tests_ssh_view_closed(&fixture.channel.base)) &&
          (fixture.channel.failure == D_SSH_ERROR_STATE) );

    // the engine released its channel with the session
    d_tests_ssh_fixture_close(&fixture);

    const bool released = ( (ended) &&
                            (!fixture.toy.channels[0].freed) );

    return ( d_tests_net_assert(kept,
                                "channel views",
                                "a channel failed elsewhere says why",
                                _counter) &&
             d_tests_net_assert( (d_tests_ssh_view_gone()) &&
                                 (released),
                                 "channel views",
                                 "a session gone leaves a closed connection",
                                 _counter) );
}

/*
d_tests_sa_ssh_view_unopened
  Tests the following:
  - a channel whose open the engine refused is a complete connection, and
    a closed one, which closes asking the engine nothing
  - so is one refused for a session not authenticated
  - a zeroed channel is no connection until freed, and a closed one after
*/
bool
d_tests_sa_ssh_view_unopened(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_toy toy     = { .opened = 0u };
    struct d_ssh_session   session = { .port = 0u };
    struct d_ssh_session   never   = { .port = 0u };
    struct d_ssh_channel   channel = { .exit_status = 0 };
    struct d_ssh_channel   zeroed  = { .exit_status = 0 };

    d_tests_ssh_toy_init(&toy);
    toy.open_fails = D_SSH_ERROR_CHANNEL;

    const bool connected = d_tests_ssh_toy_connect(&toy,
                                                   &session);
    const bool refused   =
        ( (connected)                                   &&
          (d_ssh_channel_open(&session,
                              &channel) ==
           D_SSH_ERROR_CHANNEL)                         &&
          (d_net_connection_is_complete(&channel.base)) &&
          (d_tests_ssh_view_closed(&channel.base))      &&
          (d_ssh_channel_exit_status(&channel) == -1)   &&
          (channel.failure == D_SSH_ERROR_STATE) );

    d_net_connection_close(&channel.base);
    d_ssh_channel_free(&channel);
    (void)d_ssh_disconnect(&session,
                           NULL);
    d_ssh_destroy(&session);

    const bool asked_nothing = ( (toy.opened == 0u)          &&
                                 (toy.channels[0].eofs == 0) &&
                                 (toy.channels[0].closes == 0) );
    const bool unauthenticated =
        ( (d_ssh_channel_open(&never,
                              &channel) == D_SSH_ERROR_STATE) &&
          (d_net_connection_is_complete(&channel.base))       &&
          (d_tests_ssh_view_closed(&channel.base)) );
    const bool zero = !d_net_connection_is_complete(&zeroed.base);

    d_ssh_channel_free(&zeroed);

    const bool freed = ( (zero)                                       &&
                         (d_net_connection_is_complete(&zeroed.base)) &&
                         (d_tests_ssh_view_closed(&zeroed.base)) );

    return ( d_tests_net_assert( (refused) &&
                                 (asked_nothing),
                                 "channel views",
                                 "a refused channel is a closed connection",
                                 _counter) &&
             d_tests_net_assert(unauthenticated,
                                "channel views",
                                "so is one refused for its session",
                                _counter) &&
             d_tests_net_assert(freed,
                                "channel views",
                                "a zeroed channel freed is one too",
                                _counter) );
}
