/*******************************************************************************
* djinterp [net]                                          tcp_tests_sa_threads.c
*
* Standalone tests of the TCP transport that need a second party.
*   A POSIX thread plays the peer: blocked in accept while the listener
* closes, or writing and reading amounts well beyond a socket buffer while
* the test thread does the other half. net.h's algorithms run over the
* sockets unchanged -- exact and bulk transfers, framing, and pumping.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa_threads.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
// enable POSIX.1-2008 (pthread_create, pthread_join) under a strict C
// compiler; a feature-test macro must precede every include, so it sits
// ahead of the suite's header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./tcp_tests_sa.h"  // the suite
// std
#include <string.h>   // memcmp, memset
// posix
#include <pthread.h>  // pthread_create, pthread_join


// d_tests_tcp_peer
//   struct: one thread's share of a transfer: the connection it works, the
// bytes it writes or fills, and how it ended.
struct d_tests_tcp_peer
{
    struct d_tcp_connection* connection;
    unsigned char*           bytes;
    size_t                   size;
    size_t                   done;
    enum d_net_error         outcome;
};

// d_tests_tcp_waiter
//   struct: a thread blocked in accept, and what accept returned.
struct d_tests_tcp_waiter
{
    struct d_tcp_listener*  listener;
    struct d_tcp_connection connection;
    enum d_net_error        outcome;
};

// BULK_SIZE, PUMP_SIZE
//   constant: transfers far beyond any loopback socket buffer.
static const size_t BULK_SIZE = 1024u * 1024u;
static const size_t PUMP_SIZE = 256u * 1024u;

// FRAME_SIZES
//   constant: the frames the framing test sends, the empty one included.
static const size_t FRAME_SIZES[] = { 0u, 1u, 1000u, 70000u };

// the transfers' bytes: what is sent, and what arrives
static unsigned char g_sent[1024u * 1024u];
static unsigned char g_received[1024u * 1024u];

/*
d_tests_tcp_pattern
  Fills a buffer with bytes that differ at every short period, so a
misplaced or dropped byte cannot compare equal.
*/
static void
d_tests_tcp_pattern(
    unsigned char* _bytes,
    size_t         _size
)
{
    // each byte from its index
    for (size_t i = 0u; i < _size; ++i)
    {
        _bytes[i] = (unsigned char)((i * 31u + 7u) & 0xFFu);
    }

    return;
}

/*
d_tests_tcp_writer
  Writes the peer's bytes whole, then shuts down its writing side, which the
reader sees as the end of the stream.
*/
static void*
d_tests_tcp_writer(
    void* _argument
)
{
    struct d_tests_tcp_peer* peer = (struct d_tests_tcp_peer*)_argument;

    peer->outcome = d_net_write_all(&peer->connection->base,
                                    peer->bytes,
                                    peer->size,
                                    &peer->done);
    (void)d_tcp_connection_shutdown(peer->connection,
                                    D_NET_SHUTDOWN_WRITE);

    return NULL;
}

/*
d_tests_tcp_reader
  Reads exactly the peer's size into its bytes.
*/
static void*
d_tests_tcp_reader(
    void* _argument
)
{
    struct d_tests_tcp_peer* peer = (struct d_tests_tcp_peer*)_argument;

    peer->outcome = d_net_read_exactly(&peer->connection->base,
                                       peer->bytes,
                                       peer->size,
                                       &peer->done);

    return NULL;
}

/*
d_tests_tcp_framer
  Writes one frame of each size in FRAME_SIZES, then closes.
*/
static void*
d_tests_tcp_framer(
    void* _argument
)
{
    struct d_tests_tcp_peer* peer = (struct d_tests_tcp_peer*)_argument;
    const size_t count = sizeof(FRAME_SIZES) / sizeof(FRAME_SIZES[0]);

    peer->outcome = D_NET_ERROR_NONE;

    // each frame in turn, stopping at a failure
    for (size_t i = 0u; (i < count) && (peer->outcome == D_NET_ERROR_NONE);
         ++i)
    {
        peer->outcome = d_net_write_frame(&peer->connection->base,
                                          peer->bytes,
                                          FRAME_SIZES[i],
                                          peer->size);
    }

    d_tcp_connection_close(peer->connection);

    return NULL;
}

/*
d_tests_tcp_waiting
  Blocks in accept until a connection comes or the listener closes.
*/
static void*
d_tests_tcp_waiting(
    void* _argument
)
{
    struct d_tests_tcp_waiter* waiter = (struct d_tests_tcp_waiter*)_argument;

    waiter->outcome = d_tcp_accept(waiter->listener,
                                   &waiter->connection);

    return NULL;
}

/*
d_tests_sa_tcp_accept_wake
  Tests the following:
  - an accept blocked in one thread returns D_NET_ERROR_CLOSED when another
    thread closes the listener: the wake pipe's purpose
  - accepting on the closed listener then reports closed at once
*/
bool
d_tests_sa_tcp_accept_wake(
    struct d_test_counter* _counter
)
{
    const char* const         TEST     = "a close wakes a blocked accept";
    struct d_tcp_listener     listener = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint     where    = { .port = 0u };
    struct d_tests_tcp_waiter waiter   = { .listener = &listener };
    pthread_t                 thread;
    bool                      result   = true;

    d_tcp_listener_init(&listener);
    waiter.outcome = D_NET_ERROR_UNKNOWN;

    const bool started = ( (d_tests_tcp_endpoint(&where,
                                                 "127.0.0.1",
                                                 0u)) &&
                           (d_tcp_listen(&listener,
                                         &where,
                                         NULL) == D_NET_ERROR_NONE) &&
                           (pthread_create(&thread,
                                           NULL,
                                           d_tests_tcp_waiting,
                                           &waiter) == 0) );

    // the waiter has time to block before the close
    if (started)
    {
        d_tests_tcp_pause_ms(100L);
        d_tcp_listener_close(&listener);
        (void)pthread_join(thread,
                           NULL);
    }

    result = d_assert_standalone( (started) &&
                                  (waiter.outcome == D_NET_ERROR_CLOSED) &&
                                  (!d_tcp_connection_is_open(
                                        &waiter.connection)),
                                 TEST,
                                 "the blocked accept returns closed",
                                 _counter) && result;
    result = d_assert_standalone(d_tcp_accept(&listener,
                                              &waiter.connection) ==
                                 D_NET_ERROR_CLOSED,
                                 TEST,
                                 "a closed listener then refuses at once",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_bulk
  Tests the following:
  - d_net_write_all and d_net_read_exactly move 1 MiB across a socket, the
    writer on its own thread, byte for byte
  - one byte more reads as the stream ending too soon
*/
bool
d_tests_sa_tcp_bulk(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "bulk transfers";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    struct d_tests_tcp_peer writer = { .bytes = g_sent };
    size_t                  read   = 0u;
    size_t                  extra  = 0u;
    pthread_t               thread;
    bool                    result = true;

    d_tests_tcp_pattern(g_sent,
                        BULK_SIZE);
    memset(g_received,
           0,
           BULK_SIZE);
    writer.connection = &pair.client;
    writer.size       = BULK_SIZE;

    const bool started = ( (d_tests_tcp_pair_open(&pair,
                                                  NULL)) &&
                           (pthread_create(&thread,
                                           NULL,
                                           d_tests_tcp_writer,
                                           &writer) == 0) );
    const bool arrived = ( (started) &&
                           (d_net_read_exactly(&pair.server.base,
                                               g_received,
                                               BULK_SIZE,
                                               &read) == D_NET_ERROR_NONE) );
    const bool ended   = ( (started) &&
                           (d_net_read_exactly(&pair.server.base,
                                               g_received,
                                               1u,
                                               &extra) ==
                            D_NET_ERROR_CLOSED) &&
                           (extra == 0u) );

    // the writer finishes once its bytes are read
    if (started)
    {
        (void)pthread_join(thread,
                           NULL);
    }

    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone( (arrived) &&
                                  (read == BULK_SIZE) &&
                                  (writer.outcome == D_NET_ERROR_NONE) &&
                                  (memcmp(g_sent,
                                          g_received,
                                          BULK_SIZE) == 0),
                                 TEST,
                                 "1 MiB arrives intact",
                                 _counter) && result;
    result = d_assert_standalone(ended,
                                 TEST,
                                 "a byte past the end: ended too soon",
                                 _counter) && result;

    return result;
}

/*
d_tests_tcp_read_frames
  Reads one frame per FRAME_SIZES entry, checking each length and prefix of
the pattern; then the closed stream reports D_NET_ERROR_CLOSED.
*/
static bool
d_tests_tcp_read_frames(
    struct d_tcp_connection* _connection
)
{
    const size_t count  = sizeof(FRAME_SIZES) / sizeof(FRAME_SIZES[0]);
    size_t       length = 0u;
    bool         good   = true;

    // each frame, as the framer sent it
    for (size_t i = 0u; (i < count) && (good); ++i)
    {
        good = ( (d_net_read_frame(&_connection->base,
                                   g_received,
                                   BULK_SIZE,
                                   &length) == D_NET_ERROR_NONE) &&
                 (length == FRAME_SIZES[i]) &&
                 (memcmp(g_received,
                         g_sent,
                         length) == 0) );
    }

    return ( (good) &&
             (d_net_read_frame(&_connection->base,
                               g_received,
                               BULK_SIZE,
                               &length) == D_NET_ERROR_CLOSED) );
}

/*
d_tests_sa_tcp_frames
  Tests the following:
  - frames of 0, 1, 1000, and 70000 bytes cross a socket intact, the
    framer on its own thread
  - after the framer closes, the next frame reports D_NET_ERROR_CLOSED
*/
bool
d_tests_sa_tcp_frames(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "frames over TCP";
    struct d_tests_tcp_pair pair   = { .client = { .socket = -1 } };
    struct d_tests_tcp_peer framer = { .bytes = g_sent };
    pthread_t               thread;
    bool                    result = true;

    d_tests_tcp_pattern(g_sent,
                        BULK_SIZE);
    framer.connection = &pair.client;
    framer.size       = BULK_SIZE;

    const bool started = ( (d_tests_tcp_pair_open(&pair,
                                                  NULL)) &&
                           (pthread_create(&thread,
                                           NULL,
                                           d_tests_tcp_framer,
                                           &framer) == 0) );
    const bool framed  = ( (started) &&
                           (d_tests_tcp_read_frames(&pair.server)) );

    // the framer has closed its end by the time the reads finish
    if (started)
    {
        (void)pthread_join(thread,
                           NULL);
    }

    d_tests_tcp_pair_close(&pair);
    result = d_assert_standalone( (framed) &&
                                  (framer.outcome == D_NET_ERROR_NONE),
                                 TEST,
                                 "each frame arrives; then the stream ends",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_pump
  Tests the following:
  - d_net_pump relays 256 KiB from one TCP connection into another until
    the source ends, with a writer and a reader on threads of their own
  - the relay counts every byte, and the far end receives them in order
*/
bool
d_tests_sa_tcp_pump(
    struct d_test_counter* _counter
)
{
    const char* const       TEST   = "pumping between sockets";
    struct d_tests_tcp_pair in     = { .client = { .socket = -1 } };
    struct d_tests_tcp_pair out    = { .client = { .socket = -1 } };
    struct d_tests_tcp_peer writer = { .bytes = g_sent };
    struct d_tests_tcp_peer reader = { .bytes = g_received };
    size_t                  total  = 0u;
    pthread_t               threads[2];
    bool                    result = true;

    d_tests_tcp_pattern(g_sent,
                        PUMP_SIZE);
    memset(g_received,
           0,
           PUMP_SIZE);
    writer.connection = &in.client;
    writer.size       = PUMP_SIZE;
    reader.connection = &out.server;
    reader.size       = PUMP_SIZE;

    const bool started = ( (d_tests_tcp_pair_open(&in,
                                                  NULL)) &&
                           (d_tests_tcp_pair_open(&out,
                                                  NULL)) &&
                           (pthread_create(&threads[0],
                                           NULL,
                                           d_tests_tcp_writer,
                                           &writer) == 0) &&
                           (pthread_create(&threads[1],
                                           NULL,
                                           d_tests_tcp_reader,
                                           &reader) == 0) );
    const bool pumped  = ( (started) &&
                           (d_net_pump(&in.server.base,
                                       &out.client.base,
                                       &total) == D_NET_ERROR_NONE) );

    // both threads end once the relay has carried everything
    if (started)
    {
        (void)pthread_join(threads[0],
                           NULL);
        (void)pthread_join(threads[1],
                           NULL);
    }

    d_tests_tcp_pair_close(&in);
    d_tests_tcp_pair_close(&out);
    result = d_assert_standalone( (pumped) &&
                                  (total == PUMP_SIZE) &&
                                  (reader.outcome == D_NET_ERROR_NONE) &&
                                  (memcmp(g_sent,
                                          g_received,
                                          PUMP_SIZE) == 0),
                                 TEST,
                                 "256 KiB relayed, counted, and in order",
                                 _counter) && result;

    return result;
}
