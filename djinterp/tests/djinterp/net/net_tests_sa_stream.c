/*******************************************************************************
* djinterp [net]                                           net_tests_sa_stream.c
*
* Tests of connections, the stream algorithms, and framing.
*   Every test runs over the scripted connection of net_tests_sa_support.c,
* cutting reads and writes short and failing on cue, so the algorithms' loops
* and edges are exercised without a socket. Also the suite's runner.
*
*
* path:      /tests/djinterp/net/net_tests_sa_stream.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "./net_tests_sa.h"  // corresponding header
// std
#include <stdio.h>   // printf
#include <string.h>  // memcmp, strcmp


// d_tests_net_case
//   struct: one entry of the suite.
struct d_tests_net_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

/*
d_tests_net_result_is
  Tells whether a result carries exactly the given count and error.
*/
static bool
d_tests_net_result_is(
    struct d_net_io_result _result,
    size_t                 _count,
    enum d_net_error       _error
)
{
    return ( (_result.count == _count) &&
             (_result.error == _error) );
}

/*
d_tests_net_liar_read
  Claims one byte more than it was asked for.
*/
static struct d_net_io_result
d_tests_net_liar_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    (void)_connection;
    (void)_buffer;

    return d_net_io_result_make(_size + 1u,
                                D_NET_ERROR_NONE);
}

/*
d_tests_net_liar_write
  Claims one byte more than it was given.
*/
static struct d_net_io_result
d_tests_net_liar_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    (void)_connection;
    (void)_data;

    return d_net_io_result_make(_size + 1u,
                                D_NET_ERROR_NONE);
}

/*
d_tests_net_liar_is_open
  Is always open.
*/
static bool
d_tests_net_liar_is_open(
    const struct d_net_connection* _connection
)
{
    (void)_connection;

    return true;
}

/*
d_tests_net_liar_close
  Has nothing to release.
*/
static void
d_tests_net_liar_close(
    struct d_net_connection* _connection
)
{
    (void)_connection;

    return;
}

/*
LIAR_OPERATIONS
  A complete table whose transfers claim more than they were asked for.
*/
static const struct d_net_connection_vtable LIAR_OPERATIONS =
{
    d_tests_net_liar_read,
    d_tests_net_liar_write,
    d_tests_net_liar_is_open,
    d_tests_net_liar_close,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
PARTIAL_OPERATIONS
  A table missing its required write, which init must refuse.
*/
static const struct d_net_connection_vtable PARTIAL_OPERATIONS =
{
    d_tests_net_liar_read,
    NULL,
    d_tests_net_liar_is_open,
    d_tests_net_liar_close,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
d_tests_net_dispatch_edges
  The refusal half of d_tests_sa_net_dispatch: NULL and incomplete
connections and NULL buffers are refused, and zero-byte requests are
answered without reaching the backend.
*/
static bool
d_tests_net_dispatch_edges(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "d_net_connection";
    struct d_net_connection liar   = { NULL };
    struct d_net_connection empty  = { NULL };
    unsigned char           octet  = 0u;
    bool                    result = true;

    (void)d_net_connection_init(&liar,
                                &LIAR_OPERATIONS);

    const bool refused = ( (d_tests_net_result_is(
                                d_net_connection_read(NULL,
                                                      &octet,
                                                      1u),
                                0u,
                                D_NET_ERROR_INVALID_ARGUMENT)) &&
                           (d_tests_net_result_is(
                                d_net_connection_read(&empty,
                                                      &octet,
                                                      1u),
                                0u,
                                D_NET_ERROR_INVALID_ARGUMENT)) &&
                           (d_tests_net_result_is(
                                d_net_connection_write(&liar,
                                                       NULL,
                                                       1u),
                                0u,
                                D_NET_ERROR_INVALID_ARGUMENT)) &&
                           (!d_net_connection_is_open(&empty)) );
    const bool nothing = ( (d_tests_net_result_is(
                                d_net_connection_read(&liar,
                                                      NULL,
                                                      0u),
                                0u,
                                D_NET_ERROR_NONE)) &&
                           (d_tests_net_result_is(
                                d_net_connection_write(&liar,
                                                       NULL,
                                                       0u),
                                0u,
                                D_NET_ERROR_NONE)) );

    result = d_assert_standalone(refused,
                                 TEST,
                                 "NULL and incomplete arguments are refused",
                                 _counter) && result;
    result = d_assert_standalone(nothing,
                                 TEST,
                                 "zero-byte requests never reach a backend",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_net_dispatch
  Tests the following:
  - init adopts a complete table, and refuses an incomplete one or NULL
  - a backend claiming more than it was asked for is held to UNKNOWN
  - refusals and zero-byte requests (d_tests_net_dispatch_edges)
*/
bool
d_tests_sa_net_dispatch(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "d_net_connection";
    struct d_net_connection liar   = { NULL };
    struct d_net_connection empty  = { NULL };
    unsigned char           octet  = 0u;
    bool                    result = true;

    const bool init = ( (d_net_connection_init(&liar,
                                               &LIAR_OPERATIONS)) &&
                        (!d_net_connection_init(&empty,
                                                &PARTIAL_OPERATIONS)) &&
                        (empty.vtable == NULL) &&
                        (!d_net_connection_init(NULL,
                                                &LIAR_OPERATIONS)) );
    const bool held = ( (d_tests_net_result_is(
                             d_net_connection_read(&liar,
                                                   &octet,
                                                   1u),
                             0u,
                             D_NET_ERROR_UNKNOWN)) &&
                        (d_tests_net_result_is(
                             d_net_connection_write(&liar,
                                                    &octet,
                                                    1u),
                             0u,
                             D_NET_ERROR_UNKNOWN)) );

    result = d_assert_standalone(init,
                                 TEST,
                                 "init adopts only complete tables",
                                 _counter) && result;
    result = d_assert_standalone(held,
                                 TEST,
                                 "a backend overclaiming is held to UNKNOWN",
                                 _counter) && result;

    return d_tests_net_dispatch_edges(_counter) && result;
}

/*
d_tests_net_lifetime_end
  The ending half of d_tests_sa_net_lifetime: close repeats safely, and
destroy closes, then releases through the table when it can.
*/
static bool
d_tests_net_lifetime_end(
    struct d_test_counter* _counter
)
{
    struct d_tests_net_mock full    = { .open = false };
    struct d_tests_net_mock minimal = { .open = false };

    d_tests_net_mock_init(&full,
                          true);
    d_tests_net_mock_init(&minimal,
                          false);
    d_net_connection_close(&full.base);
    d_net_connection_close(&full.base);

    const bool closed = ( (!d_net_connection_is_open(&full.base)) &&
                          (full.closes == 2u) );

    d_net_connection_destroy(&full.base);
    d_net_connection_destroy(&minimal.base);
    d_net_connection_destroy(NULL);

    const bool destroyed = ( (full.closes == 3u) &&
                             (full.destroys == 1u) &&
                             (minimal.closes == 1u) &&
                             (!minimal.open) );

    return d_assert_standalone( ( (closed) &&
                                  (destroyed) ),
                                "d_net_connection lifetime",
                                "close repeats; destroy closes, releases",
                                _counter);
}

/*
d_tests_sa_net_lifetime
  Tests the following:
  - shutdown reaches a backend, defaults to success, and validates modes
  - endpoints are reported when known, and emptied when not
  - closing and destroying (d_tests_net_lifetime_end)
*/
bool
d_tests_sa_net_lifetime(
    struct d_test_counter* _counter
)
{
    const char* const       TEST    = "d_net_connection lifetime";
    struct d_tests_net_mock full    = { .open = false };
    struct d_tests_net_mock minimal = { .open = false };
    struct d_net_endpoint   address = { .port = 0u };
    bool                    result  = true;

    d_tests_net_mock_init(&full,
                          true);
    d_tests_net_mock_init(&minimal,
                          false);

    const bool halves = ( (d_net_connection_shutdown(&full.base,
                                                     D_NET_SHUTDOWN_WRITE) ==
                           D_NET_ERROR_NONE) &&
                          (full.last_shutdown == D_NET_SHUTDOWN_WRITE) &&
                          (d_net_connection_shutdown(&minimal.base,
                                                     D_NET_SHUTDOWN_BOTH) ==
                           D_NET_ERROR_NONE) &&
                          (d_net_connection_shutdown(&full.base,
                                                     (enum d_net_shutdown)9) ==
                           D_NET_ERROR_INVALID_ARGUMENT) &&
                          (full.shutdowns == 1u) );
    const bool known = ( (d_net_connection_remote_endpoint(&full.base,
                                                           &address)) &&
                         (strcmp(address.host,
                                 "192.0.2.1") == 0) &&
                         (address.port == 80u) );
    const bool unknown = ( (!d_net_connection_local_endpoint(&full.base,
                                                             &address)) &&
                           (address.host[0] == '\0') &&
                           (address.port == 0u) &&
                           (!d_net_connection_remote_endpoint(&minimal.base,
                                                              &address)) &&
                           (!d_net_connection_remote_endpoint(&full.base,
                                                              NULL)) );

    result = d_assert_standalone(halves,
                                 TEST,
                                 "shutdown reaches, defaults, and validates",
                                 _counter) && result;
    result = d_assert_standalone(( (known) &&
                                   (unknown) ),
                                 TEST,
                                 "endpoints are reported, or emptied",
                                 _counter) && result;

    return d_tests_net_lifetime_end(_counter) && result;
}

/*
d_tests_net_write_stops
  The stopping half of d_tests_net_exact_writes: a stalled connection, and a
failing one, end the write with the bytes written so far counted.
*/
static bool
d_tests_net_write_stops(
    struct d_test_counter* _counter
)
{
    struct d_tests_net_mock mock = { .open = false };
    size_t                  done = 0u;

    // the bytes to write, and room for the six a full connection takes
    unsigned char source[10] = { 0 };
    unsigned char target[16] = { 0 };

    d_tests_net_pattern(source,
                        sizeof(source),
                        1u);
    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_output(&mock,
                            target,
                            6u);
    mock.write_limit = 2u;

    const bool stalled = ( (d_net_write_all(&mock.base,
                                            source,
                                            sizeof(source),
                                            &done) == D_NET_ERROR_CLOSED) &&
                           (done == 6u) );

    d_tests_net_mock_output(&mock,
                            target,
                            6u);
    mock.write_error = D_NET_ERROR_TIMED_OUT;

    const bool failed = ( (d_net_write_all(&mock.base,
                                           source,
                                           sizeof(source),
                                           &done) == D_NET_ERROR_TIMED_OUT) &&
                          (done == 6u) );

    return d_assert_standalone( ( (stalled) &&
                                  (failed) ),
                                "d_net_write_all",
                                "stalls and failures end it, counted",
                                _counter);
}

/*
d_tests_net_exact_writes
  The write half of d_tests_sa_net_exact: short writes are looped over until
the whole buffer is taken; stops are d_tests_net_write_stops's.
*/
static bool
d_tests_net_exact_writes(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "d_net_write_all";
    struct d_tests_net_mock mock   = { .open = false };
    size_t                  done   = 0u;
    bool                    result = true;

    // the bytes to write, and room for them
    unsigned char source[10] = { 0 };
    unsigned char target[16] = { 0 };

    d_tests_net_pattern(source,
                        sizeof(source),
                        1u);
    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_output(&mock,
                            target,
                            sizeof(target));
    mock.write_limit = 2u;

    const bool whole = ( (d_net_write_all(&mock.base,
                                          source,
                                          sizeof(source),
                                          &done) == D_NET_ERROR_NONE) &&
                         (done == 10u) &&
                         (mock.writes == 5u) &&
                         (memcmp(target,
                                 source,
                                 sizeof(source)) == 0) );

    result = d_assert_standalone(whole,
                                 TEST,
                                 "short writes are looped over",
                                 _counter) && result;

    return d_tests_net_write_stops(_counter) && result;
}

/*
d_tests_net_read_stops
  The stopping half of d_tests_sa_net_exact: a stream ending early, and one
failing, stop the read with the bytes read so far counted.
*/
static bool
d_tests_net_read_stops(
    struct d_test_counter* _counter
)
{
    struct d_tests_net_mock mock = { .open = false };
    size_t                  done = 9u;

    // the stream, and room for more than it holds
    unsigned char source[10] = { 0 };
    unsigned char target[16] = { 0 };

    d_tests_net_pattern(source,
                        sizeof(source),
                        2u);
    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_input(&mock,
                           source,
                           4u);
    mock.read_limit = 3u;

    const bool early = ( (d_net_read_exactly(&mock.base,
                                             target,
                                             10u,
                                             &done) == D_NET_ERROR_CLOSED) &&
                         (done == 4u) );

    d_tests_net_mock_input(&mock,
                           source,
                           4u);
    mock.read_error = D_NET_ERROR_WOULD_BLOCK;

    const bool failed = ( (d_net_read_exactly(&mock.base,
                                              target,
                                              10u,
                                              &done) ==
                           D_NET_ERROR_WOULD_BLOCK) &&
                          (done == 4u) );

    return d_assert_standalone( ( (early) &&
                                  (failed) ),
                                "d_net_read_exactly",
                                "ends and failures stop it, counted",
                                _counter);
}

/*
d_tests_sa_net_exact
  Tests the following:
  - read_exactly loops over short reads until the request is met
  - NULL connections are refused, and zero-byte requests succeed
  - early ends and failures (d_tests_net_read_stops)
  - write_all (d_tests_net_exact_writes)
*/
bool
d_tests_sa_net_exact(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "d_net_read_exactly";
    struct d_tests_net_mock mock   = { .open = false };
    size_t                  done   = 9u;
    bool                    result = true;

    // the stream, and room for it
    unsigned char source[10] = { 0 };
    unsigned char target[16] = { 0 };

    d_tests_net_pattern(source,
                        sizeof(source),
                        2u);
    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_input(&mock,
                           source,
                           sizeof(source));
    mock.read_limit = 3u;

    const bool whole = ( (d_net_read_exactly(&mock.base,
                                             target,
                                             10u,
                                             &done) == D_NET_ERROR_NONE) &&
                         (done == 10u) &&
                         (memcmp(target,
                                 source,
                                 sizeof(source)) == 0) );
    const bool edges = ( (d_net_read_exactly(NULL,
                                             target,
                                             1u,
                                             &done) ==
                          D_NET_ERROR_INVALID_ARGUMENT) &&
                         (done == 0u) &&
                         (d_net_read_exactly(&mock.base,
                                             NULL,
                                             0u,
                                             NULL) == D_NET_ERROR_NONE) );

    result = d_assert_standalone(whole,
                                 TEST,
                                 "short reads are looped over",
                                 _counter) && result;
    result = d_assert_standalone(edges,
                                 TEST,
                                 "NULL is refused; zero bytes succeed",
                                 _counter) && result;

    result = d_tests_net_read_stops(_counter) && result;

    return d_tests_net_exact_writes(_counter) && result;
}

/*
d_tests_net_pumps
  The pump half of d_tests_sa_net_bulk: pump copies a stream across short
reads and short writes, reports a sink that stops taking bytes as CLOSED
with what it did take counted, and refuses NULL.
*/
static bool
d_tests_net_pumps(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "d_net_pump";
    struct d_tests_net_mock source = { .open = false };
    struct d_tests_net_mock sink   = { .open = false };
    size_t                  total  = 0u;
    bool                    result = true;

    // more than two chunks, and a target that holds it all
    unsigned char data[10000]   = { 0 };
    unsigned char target[10000] = { 0 };

    d_tests_net_pattern(data,
                        sizeof(data),
                        3u);
    d_tests_net_mock_init(&source,
                          false);
    d_tests_net_mock_init(&sink,
                          false);
    d_tests_net_mock_input(&source,
                           data,
                           sizeof(data));
    d_tests_net_mock_output(&sink,
                            target,
                            sizeof(target));
    source.read_limit = 1000u;
    sink.write_limit  = 333u;

    const bool copied = ( (d_net_pump(&source.base,
                                      &sink.base,
                                      &total) == D_NET_ERROR_NONE) &&
                          (total == sizeof(data)) &&
                          (memcmp(target,
                                  data,
                                  sizeof(data)) == 0) );

    d_tests_net_mock_input(&source,
                           data,
                           sizeof(data));
    d_tests_net_mock_output(&sink,
                            target,
                            100u);

    const bool stalled = ( (d_net_pump(&source.base,
                                       &sink.base,
                                       &total) == D_NET_ERROR_CLOSED) &&
                           (total == 100u) &&
                           (d_net_pump(&source.base,
                                       NULL,
                                       NULL) ==
                            D_NET_ERROR_INVALID_ARGUMENT) );

    result = d_assert_standalone(copied,
                                 TEST,
                                 "a stream is copied across short transfers",
                                 _counter) && result;
    result = d_assert_standalone(stalled,
                                 TEST,
                                 "a stalled sink ends it, counted; NULL fails",
                                 _counter) && result;

    return result;
}

/*
d_tests_net_read_all_stops
  The stopping half of d_tests_sa_net_bulk: a full sink, a failing stream,
and a sink with no write function each end read_all.
*/
static bool
d_tests_net_read_all_stops(
    struct d_test_counter* _counter
)
{
    const struct d_pack_sink       none   = { NULL, NULL };
    struct d_tests_net_mock        mock   = { .open = false };
    struct d_tests_net_sink_buffer buffer = { .length = 0u };
    size_t                         total  = 0u;

    // the stream, and a sink too small for it
    unsigned char data[10000]  = { 0 };
    unsigned char target[5000] = { 0 };

    d_tests_net_pattern(data,
                        sizeof(data),
                        4u);
    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_input(&mock,
                           data,
                           sizeof(data));
    mock.read_limit = 777u;
    buffer.data     = target;
    buffer.capacity = sizeof(target);

    const bool full = ( (d_net_read_all(&mock.base,
                                        d_tests_net_sink(&buffer),
                                        &total) ==
                         D_NET_ERROR_MESSAGE_TOO_LARGE) &&
                        (buffer.length == 5000u) &&
                        (total < 5000u) );

    d_tests_net_mock_input(&mock,
                           data,
                           100u);
    mock.read_error = D_NET_ERROR_CONNECTION_RESET;

    const bool torn = ( (d_net_read_all(&mock.base,
                                        d_tests_net_sink(&buffer),
                                        &total) ==
                         D_NET_ERROR_CONNECTION_RESET) &&
                        (total == 100u) &&
                        (d_net_read_all(&mock.base,
                                        none,
                                        NULL) ==
                         D_NET_ERROR_INVALID_ARGUMENT) );

    return d_assert_standalone( ( (full) &&
                                  (torn) ),
                                "d_net_read_all",
                                "full sinks, failures, and NULL end it",
                                _counter);
}

/*
d_tests_sa_net_bulk
  Tests the following:
  - read_all drains a stream into a sink across short reads
  - full sinks, failures, and NULL sinks (d_tests_net_read_all_stops)
  - pump (d_tests_net_pumps)
*/
bool
d_tests_sa_net_bulk(
    struct d_test_counter* _counter
)
{
    const char* const              TEST   = "d_net_read_all";
    struct d_tests_net_mock        mock   = { .open = false };
    struct d_tests_net_sink_buffer buffer = { .length = 0u };
    size_t                         total  = 0u;
    bool                           result = true;

    // the stream, and room for all of it
    unsigned char data[10000]   = { 0 };
    unsigned char target[10000] = { 0 };

    d_tests_net_pattern(data,
                        sizeof(data),
                        4u);
    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_input(&mock,
                           data,
                           sizeof(data));
    mock.read_limit = 777u;
    buffer.data     = target;
    buffer.capacity = sizeof(target);

    const bool drained = ( (d_net_read_all(&mock.base,
                                           d_tests_net_sink(&buffer),
                                           &total) == D_NET_ERROR_NONE) &&
                           (total == sizeof(data)) &&
                           (memcmp(target,
                                   data,
                                   sizeof(data)) == 0) );

    result = d_assert_standalone(drained,
                                 TEST,
                                 "a stream is drained into a sink",
                                 _counter) && result;

    result = d_tests_net_read_all_stops(_counter) && result;

    return d_tests_net_pumps(_counter) && result;
}

/*
d_tests_net_frame_out
  Writes one frame through a scripted connection into _wire and returns the
bytes it took; _out_writes, when not NULL, receives how many writes that
was.
*/
static size_t
d_tests_net_frame_out(
    const void*    _payload,
    size_t         _size,
    unsigned char* _wire,
    size_t         _capacity,
    size_t*        _out_writes
)
{
    struct d_tests_net_mock writer = { .open = false };

    d_tests_net_mock_init(&writer,
                          false);
    d_tests_net_mock_output(&writer,
                            _wire,
                            _capacity);
    (void)d_net_write_frame(&writer.base,
                            _payload,
                            _size,
                            D_NET_FRAME_MAX);

    // the count of writes is wanted only where the split is tested
    if (_out_writes)
    {
        *_out_writes = writer.writes;
    }

    return writer.output_length;
}

/*
d_tests_net_frame_refusals
  The refusing half of d_tests_net_frame_reads: a buffer too small for the
declared length, and a header cut short, are refused.
*/
static bool
d_tests_net_frame_refusals(
    struct d_test_counter* _counter
)
{
    struct d_tests_net_mock reader = { .open = false };
    size_t                  length = 0u;
    size_t                  sent   = 0u;

    // a frame larger than one chunk, its wire form, and room to read it
    unsigned char payload[D_NET_IO_CHUNK + 100u]                        = { 0 };
    unsigned char wire[D_NET_IO_CHUNK + 100u + D_NET_FRAME_HEADER_SIZE] = { 0 };
    unsigned char back[D_NET_IO_CHUNK + 100u]                           = { 0 };

    d_tests_net_pattern(payload,
                        sizeof(payload),
                        5u);
    sent = d_tests_net_frame_out(payload,
                                 sizeof(payload),
                                 wire,
                                 sizeof(wire),
                                 NULL);
    d_tests_net_mock_init(&reader,
                          false);
    d_tests_net_mock_input(&reader,
                           wire,
                           sent);

    const bool small = ( (d_net_read_frame(&reader.base,
                                           back,
                                           10u,
                                           &length) ==
                          D_NET_ERROR_MESSAGE_TOO_LARGE) &&
                         (length == sizeof(payload)) );

    d_tests_net_mock_input(&reader,
                           wire,
                           2u);

    const bool cut = ( (d_net_read_frame(&reader.base,
                                         back,
                                         sizeof(back),
                                         &length) == D_NET_ERROR_CLOSED) &&
                       (length == 0u) );

    return d_assert_standalone( ( (small) &&
                                  (cut) ),
                                "d_net_read_frame",
                                "short buffers and headers are refused",
                                _counter);
}

/*
d_tests_net_frame_reads
  The reading half of d_tests_sa_net_frames: a frame larger than one chunk
goes out in two writes and comes back whole; a stream torn mid-payload
reports the declared length. Refusals are d_tests_net_frame_refusals's.
*/
static bool
d_tests_net_frame_reads(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "d_net_read_frame";
    struct d_tests_net_mock reader = { .open = false };
    size_t                  length = 0u;
    size_t                  writes = 0u;
    size_t                  sent   = 0u;
    bool                    result = true;

    // a frame larger than one chunk, its wire form, and room to read it
    unsigned char payload[D_NET_IO_CHUNK + 100u]                        = { 0 };
    unsigned char wire[D_NET_IO_CHUNK + 100u + D_NET_FRAME_HEADER_SIZE] = { 0 };
    unsigned char back[D_NET_IO_CHUNK + 100u]                           = { 0 };

    d_tests_net_pattern(payload,
                        sizeof(payload),
                        5u);
    sent = d_tests_net_frame_out(payload,
                                 sizeof(payload),
                                 wire,
                                 sizeof(wire),
                                 &writes);
    d_tests_net_mock_init(&reader,
                          false);
    d_tests_net_mock_input(&reader,
                           wire,
                           sent);

    const bool back_again = ( (d_net_read_frame(&reader.base,
                                                back,
                                                sizeof(back),
                                                &length) ==
                               D_NET_ERROR_NONE) &&
                              (length == sizeof(payload)) &&
                              (memcmp(back,
                                      payload,
                                      sizeof(payload)) == 0) );

    d_tests_net_mock_input(&reader,
                           wire,
                           100u);

    const bool torn = ( (d_net_read_frame(&reader.base,
                                          back,
                                          sizeof(back),
                                          &length) == D_NET_ERROR_CLOSED) &&
                        (length == sizeof(payload)) );

    result = d_assert_standalone(( (writes == 2u) &&
                                   (sent == sizeof(wire)) &&
                                   (back_again) ),
                                 TEST,
                                 "a large frame makes the round trip",
                                 _counter) && result;
    result = d_assert_standalone(torn,
                                 TEST,
                                 "a torn payload reports its length",
                                 _counter) && result;

    return d_tests_net_frame_refusals(_counter) && result;
}

/*
d_tests_net_frame_ceiling
  A frame over the ceiling is refused before anything is written.
*/
static bool
d_tests_net_frame_ceiling(
    struct d_test_counter* _counter
)
{
    struct d_tests_net_mock mock = { .open = false };

    // room the refused frame must leave untouched
    unsigned char wire[16] = { 0 };

    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_output(&mock,
                            wire,
                            sizeof(wire));

    const bool refused = ( (d_net_write_frame(&mock.base,
                                              "hello",
                                              5u,
                                              4u) ==
                            D_NET_ERROR_MESSAGE_TOO_LARGE) &&
                           (mock.output_length == 0u) );

    return d_assert_standalone(refused,
                               "d_net_write_frame",
                               "frames over the ceiling write nothing",
                               _counter);
}

/*
d_tests_sa_net_frames
  Tests the following:
  - a small frame is one write of a big-endian length and its payload
  - an empty frame is a bare header
  - a frame over the ceiling (d_tests_net_frame_ceiling)
  - large frames and refusals (d_tests_net_frame_reads)
*/
bool
d_tests_sa_net_frames(
    struct d_test_counter* _counter
)
{
    static const unsigned char EXPECTED[9] =
    {
        0x00u, 0x00u, 0x00u, 0x05u, 'h', 'e', 'l', 'l', 'o'
    };
    const char* const       TEST   = "d_net_write_frame";
    struct d_tests_net_mock mock   = { .open = false };
    bool                    result = true;

    // room for the small frame
    unsigned char wire[16] = { 0 };

    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_output(&mock,
                            wire,
                            sizeof(wire));

    const bool whole = ( (d_net_write_frame(&mock.base,
                                            "hello",
                                            5u,
                                            D_NET_FRAME_MAX) ==
                          D_NET_ERROR_NONE) &&
                         (mock.writes == 1u) &&
                         (mock.output_length == sizeof(EXPECTED)) &&
                         (memcmp(wire,
                                 EXPECTED,
                                 sizeof(EXPECTED)) == 0) );

    d_tests_net_mock_output(&mock,
                            wire,
                            sizeof(wire));

    const bool bare = ( (d_net_write_frame(&mock.base,
                                           NULL,
                                           0u,
                                           D_NET_FRAME_MAX) ==
                         D_NET_ERROR_NONE) &&
                        (mock.output_length == D_NET_FRAME_HEADER_SIZE) &&
                        (memcmp(wire,
                                EXPECTED,
                                3u) == 0) &&
                        (wire[3] == 0u) );

    result = d_assert_standalone(whole,
                                 TEST,
                                 "a small frame is one big-endian write",
                                 _counter) && result;
    result = d_assert_standalone(bare,
                                 TEST,
                                 "an empty frame is a bare header",
                                 _counter) && result;
    result = d_tests_net_frame_ceiling(_counter) && result;

    return d_tests_net_frame_reads(_counter) && result;
}

/*
d_tests_net_frame_sink_stops
  The stopping half of d_tests_sa_net_frames_sink: a length over the ceiling
is refused before the sink sees a byte, a full sink ends the frame, and a
sink with no write function is refused.
*/
static bool
d_tests_net_frame_sink_stops(
    struct d_test_counter* _counter
)
{
    const struct d_pack_sink       none   = { NULL, NULL };
    struct d_tests_net_mock        reader = { .open = false };
    struct d_tests_net_sink_buffer buffer = { .length = 0u };
    size_t                         length = 0u;
    size_t                         sent   = 0u;

    // a frame of several chunks, its wire form, and a sink's storage
    unsigned char payload[(3u * D_NET_IO_CHUNK) + 17u] = { 0 };
    unsigned char wire[(3u * D_NET_IO_CHUNK) + 21u]    = { 0 };
    unsigned char target[(3u * D_NET_IO_CHUNK) + 17u]  = { 0 };

    d_tests_net_pattern(payload,
                        sizeof(payload),
                        6u);
    sent = d_tests_net_frame_out(payload,
                                 sizeof(payload),
                                 wire,
                                 sizeof(wire),
                                 NULL);
    d_tests_net_mock_init(&reader,
                          false);
    d_tests_net_mock_input(&reader,
                           wire,
                           sent);
    buffer.data     = target;
    buffer.capacity = sizeof(target);

    const bool ceiling = ( (d_net_read_frame_sink(&reader.base,
                                                  d_tests_net_sink(&buffer),
                                                  100u,
                                                  &length) ==
                            D_NET_ERROR_MESSAGE_TOO_LARGE) &&
                           (buffer.length == 0u) &&
                           (length == sizeof(payload)) );

    d_tests_net_mock_input(&reader,
                           wire,
                           sent);
    buffer.capacity = 100u;

    const bool full = ( (d_net_read_frame_sink(&reader.base,
                                               d_tests_net_sink(&buffer),
                                               D_NET_FRAME_MAX,
                                               NULL) ==
                         D_NET_ERROR_MESSAGE_TOO_LARGE) &&
                        (buffer.length == 100u) &&
                        (d_net_read_frame_sink(&reader.base,
                                               none,
                                               D_NET_FRAME_MAX,
                                               NULL) ==
                         D_NET_ERROR_INVALID_ARGUMENT) );

    return d_assert_standalone( ( (ceiling) &&
                                  (full) ),
                                "d_net_read_frame_sink",
                                "ceilings, full sinks, and NULL end it",
                                _counter);
}

/*
d_tests_sa_net_frames_sink
  Tests the following:
  - a frame of several chunks reaches a sink whole, across short reads
  - ceilings, full sinks, and NULL sinks (d_tests_net_frame_sink_stops)
*/
bool
d_tests_sa_net_frames_sink(
    struct d_test_counter* _counter
)
{
    const char* const              TEST   = "d_net_read_frame_sink";
    struct d_tests_net_mock        reader = { .open = false };
    struct d_tests_net_sink_buffer buffer = { .length = 0u };
    size_t                         length = 0u;
    size_t                         sent   = 0u;
    bool                           result = true;

    // a frame of several chunks, its wire form, and a sink's storage
    unsigned char payload[(3u * D_NET_IO_CHUNK) + 17u] = { 0 };
    unsigned char wire[(3u * D_NET_IO_CHUNK) + 21u]    = { 0 };
    unsigned char target[(3u * D_NET_IO_CHUNK) + 17u]  = { 0 };

    d_tests_net_pattern(payload,
                        sizeof(payload),
                        6u);
    sent = d_tests_net_frame_out(payload,
                                 sizeof(payload),
                                 wire,
                                 sizeof(wire),
                                 NULL);
    d_tests_net_mock_init(&reader,
                          false);
    d_tests_net_mock_input(&reader,
                           wire,
                           sent);
    reader.read_limit = 1000u;
    buffer.data       = target;
    buffer.capacity   = sizeof(target);

    const bool whole = ( (d_net_read_frame_sink(&reader.base,
                                                d_tests_net_sink(&buffer),
                                                D_NET_FRAME_MAX,
                                                &length) ==
                          D_NET_ERROR_NONE) &&
                         (length == sizeof(payload)) &&
                         (buffer.length == sizeof(payload)) &&
                         (memcmp(target,
                                 payload,
                                 sizeof(payload)) == 0) );

    result = d_assert_standalone(whole,
                                 TEST,
                                 "a multi-chunk frame reaches the sink",
                                 _counter) && result;

    return d_tests_net_frame_sink_stops(_counter) && result;
}

/*
d_tests_sa_net_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_net_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_net_case CASES[] =
    {
        { "errors",           d_tests_sa_net_errors },
        { "results",          d_tests_sa_net_results },
        { "ports",            d_tests_sa_net_ports },
        { "endpoint records", d_tests_sa_net_endpoint_records },
        { "endpoint parsing", d_tests_sa_net_endpoint_parse },
        { "endpoint text",    d_tests_sa_net_endpoint_format },
        { "frame codec",      d_tests_sa_net_frame_codec },
        { "dispatch",         d_tests_sa_net_dispatch },
        { "lifetime",         d_tests_sa_net_lifetime },
        { "exact transfers",  d_tests_sa_net_exact },
        { "bulk transfers",   d_tests_sa_net_bulk },
        { "url components",   d_tests_sa_net_url_components },
        { "url refusals",     d_tests_sa_net_url_refusals },
        { "url authorities",  d_tests_sa_net_url_authority },
        { "url hosts",        d_tests_sa_net_url_hosts },
        { "url format",       d_tests_sa_net_url_format },
        { "url resolution",   d_tests_sa_net_url_resolve },
        { "url normal forms", d_tests_sa_net_url_normalize },
        { "url escapes",      d_tests_sa_net_url_percent },
        { "url endpoints",    d_tests_sa_net_url_endpoints },
        { "frames",           d_tests_sa_net_frames },
        { "frames to a sink", d_tests_sa_net_frames_sink }
    };
    bool all = true;

    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        const bool passed = CASES[i].run(_counter);

        _counter->tests_total  += 1u;
        _counter->tests_passed += (passed) ? 1u : 0u;
        all                     = ( (all) &&
                                    (passed) );
        printf("  [%s] %s\n",
               (passed) ? "PASS" : "FAIL",
               CASES[i].name);
    }

    return all;
}
