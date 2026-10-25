/*******************************************************************************
* djinterp [net]                                           net_tests_sa_memory.c
*
* Tests of the in-memory connections of net/net_memory.h.
*   The pipe's two ends talk to each other in one thread, so each test drives
* both: what one end writes, the other reads, through every limit -- an empty
* ring, a full one, a chunk limit, a ring that wraps -- and through every way
* an end stops. The memory stream reads a fixed script and writes into a
* sink of the test's choosing.
*
*
* path:      /tests/djinterp/net/net_tests_sa_memory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./net_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memcmp, memset
// djinterp
#include "../../../inc/djinterp/net/net_memory.h"  // pipes, memory streams


/*
d_tests_net_memory_is
  Whether a result moved `_count` bytes and reported `_error`.
*/
static bool
d_tests_net_memory_is(
    struct d_net_io_result _result,
    size_t                 _count,
    enum d_net_error       _error
)
{
    return ( (_result.count == _count) &&
             (_result.error == _error) );
}

/*
d_tests_net_pipe_refusals
  The refusing half of d_tests_sa_net_pipes: init refuses what it cannot
use, and leaves the pipe as it was; an unknown end is NULL.
*/
static bool
d_tests_net_pipe_refusals(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe pipe       = { .rings[0].size = 0u };
    unsigned char     storage[8] = { 0 };

    const bool refused =
        ( (d_net_pipe_init(NULL,
                           storage,
                           4u,
                           storage + 4,
                           4u) == D_NET_ERROR_INVALID_ARGUMENT) &&
          (d_net_pipe_init(&pipe,
                           NULL,
                           4u,
                           storage + 4,
                           4u) == D_NET_ERROR_INVALID_ARGUMENT) &&
          (d_net_pipe_init(&pipe,
                           storage,
                           0u,
                           storage + 4,
                           4u) == D_NET_ERROR_INVALID_ARGUMENT) &&
          (d_net_pipe_init(&pipe,
                           storage,
                           4u,
                           storage + 4,
                           0u) == D_NET_ERROR_INVALID_ARGUMENT) &&
          (pipe.ends[0].base.vtable == NULL) );
    const bool unknown =
        ( (d_net_pipe_connection(NULL,
                                 D_NET_PIPE_A) == NULL) &&
          (d_net_pipe_connection(&pipe,
                                 (enum d_net_pipe_side)2) == NULL) );

    // neither a NULL pipe nor an unknown end has a limit to set
    d_net_pipe_set_chunk(NULL,
                         D_NET_PIPE_A,
                         1u);
    d_net_pipe_set_chunk(&pipe,
                         (enum d_net_pipe_side)2,
                         1u);

    return d_tests_net_assert( ( (refused) &&
                                 (unknown) ),
                               "d_net_pipe_init",
                               "refuses what it cannot use",
                               _counter);
}

/*
d_tests_net_pipe_chunked
  The chunked part of d_tests_net_pipe_limits: end B, under a limit of two
bytes and then of one, reads and writes no more than the limit at a time.
Its reads land at `_read` + 3, after what the test read before.
*/
static bool
d_tests_net_pipe_chunked(
    struct d_net_pipe*   _pipe,
    const unsigned char* _bytes,
    unsigned char*       _read
)
{
    struct d_net_connection* const b = d_net_pipe_connection(_pipe,
                                                             D_NET_PIPE_B);

    d_net_pipe_set_chunk(_pipe,
                         D_NET_PIPE_B,
                         2u);

    const bool chunked =
        ( (d_tests_net_memory_is(d_net_connection_read(b,
                                                       _read + 3,
                                                       7u),
                                 2u,
                                 D_NET_OK)) &&
          (d_tests_net_memory_is(d_net_connection_write(b,
                                                        _bytes,
                                                        5u),
                                 2u,
                                 D_NET_OK)) );

    d_net_pipe_set_chunk(_pipe,
                         D_NET_PIPE_B,
                         1u);

    return ( (chunked)                         &&
             (d_tests_net_memory_is(d_net_connection_read(b,
                                                          _read + 5,
                                                          5u),
                                    1u,
                                    D_NET_OK)) &&
             (d_tests_net_memory_is(d_net_connection_write(b,
                                                           _bytes,
                                                           5u),
                                    1u,
                                    D_NET_OK)) );
}

/*
d_tests_net_pipe_limits
  A full ring takes nothing more until read from; a chunk limit cuts both
reads and writes short, a limit of one byte to one byte
(d_tests_net_pipe_chunked).
*/
static bool
d_tests_net_pipe_limits(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe        pipe      = { .rings[0].size = 0u };
    unsigned char            to_b[8]   = { 0 };
    unsigned char            to_a[8]   = { 0 };
    unsigned char            bytes[10] = { 0 };
    unsigned char            read[10]  = { 0 };
    struct d_net_connection* a         = NULL;
    struct d_net_connection* b         = NULL;

    d_tests_net_pattern(bytes,
                        sizeof(bytes),
                        3u);
    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));
    a = d_net_pipe_connection(&pipe,
                              D_NET_PIPE_A);
    b = d_net_pipe_connection(&pipe,
                              D_NET_PIPE_B);

    const bool full =
        ( (d_tests_net_memory_is(d_net_connection_write(a,
                                                        bytes,
                                                        10u),
                                 8u,
                                 D_NET_OK))                &&
          (d_tests_net_memory_is(d_net_connection_write(a,
                                                        bytes,
                                                        1u),
                                 0u,
                                 D_NET_ERROR_WOULD_BLOCK)) &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       3u),
                                 3u,
                                 D_NET_OK))                &&
          (d_tests_net_memory_is(d_net_connection_write(a,
                                                        bytes + 8,
                                                        2u),
                                 2u,
                                 D_NET_OK)) );
    const bool chunked = d_tests_net_pipe_chunked(&pipe,
                                                  bytes,
                                                  read);

    return d_tests_net_assert( ( (full)    &&
                                 (chunked) &&
                                 (memcmp(read,
                                         bytes,
                                         6u) == 0) ),
                               "d_net_pipe",
                               "full rings wait; chunk limits cut short",
                               _counter);
}

/*
d_tests_net_pipe_wrap
  Writes and reads in sizes that never divide the ring, so every byte
offset of it is crossed, and the stream read must equal the one written.
*/
static bool
d_tests_net_pipe_wrap(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe pipe      = { .rings[0].size = 0u };
    unsigned char     to_b[13]  = { 0 };
    unsigned char     to_a[1]   = { 0 };
    unsigned char     piece[16] = { 0 };
    unsigned char     back[16]  = { 0 };
    bool              same      = true;

    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));

    // each round writes up to 11 bytes and reads them back in two pieces
    for (unsigned int round = 0u;
         round < 200u;
         ++round)
    {
        const size_t           size = 1u + (round % 11u);
        struct d_net_io_result in   = { 0u, D_NET_OK };
        struct d_net_io_result out  = { 0u, D_NET_OK };

        d_tests_net_pattern(piece,
                            size,
                            round);
        in  = d_net_connection_write(d_net_pipe_connection(&pipe,
                                                           D_NET_PIPE_A),
                                     piece,
                                     size);
        out = d_net_connection_read(d_net_pipe_connection(&pipe,
                                                          D_NET_PIPE_B),
                                    back,
                                    size / 2u + 1u);

        // what was not read yet comes with a second read
        if (out.count < size)
        {
            out.count += d_net_connection_read(
                             d_net_pipe_connection(&pipe,
                                                   D_NET_PIPE_B),
                             back + out.count,
                             size - out.count).count;
        }

        same = ( (same)              &&
                 (in.count == size)  &&
                 (out.count == size) &&
                 (memcmp(piece,
                         back,
                         size) == 0) );
    }

    return d_tests_net_assert(same,
                              "d_net_pipe",
                              "a wrapping ring keeps the stream in order",
                              _counter);
}

/*
d_tests_net_pipe_framing
  The two ends limited to a byte per read and write, net.h's framing still
carries a frame whole.
*/
static bool
d_tests_net_pipe_framing(
    struct d_net_pipe*     _pipe,
    struct d_test_counter* _counter
)
{
    unsigned char read[16] = { 0 };
    size_t        count    = 0u;

    d_net_pipe_set_chunk(_pipe,
                         D_NET_PIPE_A,
                         1u);
    d_net_pipe_set_chunk(_pipe,
                         D_NET_PIPE_B,
                         1u);

    const bool framed =
        ( (d_net_write_frame(d_net_pipe_connection(_pipe,
                                                   D_NET_PIPE_A),
                             "framed",
                             6u,
                             16u) == D_NET_OK)   &&
          (d_net_read_frame(d_net_pipe_connection(_pipe,
                                                  D_NET_PIPE_B),
                            read,
                            sizeof(read),
                            &count) == D_NET_OK) &&
          (count == 6u)                          &&
          (memcmp(read,
                  "framed",
                  6u) == 0) );

    return d_tests_net_assert(framed,
                              "d_net_pipe",
                              "framing runs over it a byte at a time",
                              _counter);
}

/*
d_tests_sa_net_pipes
  Tests the following:
  - init refuses NULLs and zero capacities; unknown ends are NULL
    (d_tests_net_pipe_refusals)
  - what A writes B reads, and the reverse; an empty ring would block
  - available counts what is waiting; neither end has an address
  - full rings and chunk limits (d_tests_net_pipe_limits)
  - wrapping rings (d_tests_net_pipe_wrap)
  - net.h's framing runs over a pipe one byte at a time
    (d_tests_net_pipe_framing)
*/
bool
d_tests_sa_net_pipes(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe     pipe     = { .rings[0].size = 0u };
    unsigned char         to_b[64] = { 0 };
    unsigned char         to_a[64] = { 0 };
    unsigned char         read[16] = { 0 };
    size_t                count    = 0u;
    struct d_net_endpoint address  = { .port = 0u };
    bool                  result   = true;

    const bool made =
        (d_net_pipe_init(&pipe,
                         to_b,
                         sizeof(to_b),
                         to_a,
                         sizeof(to_a)) == D_NET_OK);

    struct d_net_connection* const a = d_net_pipe_connection(&pipe,
                                                             D_NET_PIPE_A);
    struct d_net_connection* const b = d_net_pipe_connection(&pipe,
                                                             D_NET_PIPE_B);

    const bool exchanged =
        ( (made)                                           &&
          (d_net_connection_is_open(a))                    &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       16u),
                                 0u,
                                 D_NET_ERROR_WOULD_BLOCK)) &&
          (d_net_write_all(a,
                           "hello",
                           5u,
                           NULL) == D_NET_OK)              &&
          (d_net_connection_available(b,
                                      &count))             &&
          (count == 5u)                                    &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       16u),
                                 5u,
                                 D_NET_OK))                &&
          (memcmp(read,
                  "hello",
                  5u) == 0)                                &&
          (d_net_write_all(b,
                           "hi",
                           2u,
                           NULL) == D_NET_OK)              &&
          (d_net_read_exactly(a,
                              read,
                              2u,
                              NULL) == D_NET_OK)           &&
          (memcmp(read,
                  "hi",
                  2u) == 0)                                &&
          (!d_net_connection_remote_endpoint(a,
                                             &address)) );

    result = d_tests_net_assert(exchanged,
                                "d_net_pipe",
                                "each end reads what the other wrote",
                                _counter) && result;
    result = d_tests_net_pipe_framing(&pipe,
                                      _counter) && result;
    result = d_tests_net_pipe_refusals(_counter) && result;
    result = d_tests_net_pipe_limits(_counter) && result;

    return d_tests_net_pipe_wrap(_counter) && result;
}

/*
d_tests_net_pipe_halves
  Half-closes: an end done writing lets its peer drain and then end, and
still reads what its peer writes; an end done reading reads the end of the
stream, whatever is waiting.
*/
static bool
d_tests_net_pipe_halves(
    struct d_net_pipe*     _pipe,
    struct d_test_counter* _counter
)
{
    struct d_net_connection* const a       = d_net_pipe_connection(
                                                 _pipe,
                                                 D_NET_PIPE_A);
    struct d_net_connection* const b       = d_net_pipe_connection(
                                                 _pipe,
                                                 D_NET_PIPE_B);
    unsigned char                  read[8] = { 0 };
    size_t                         count   = 9u;

    const bool written =
        ( (d_net_write_all(a,
                           "abc",
                           3u,
                           NULL) == D_NET_OK)                           &&
          (d_net_connection_shutdown(a,
                                     D_NET_SHUTDOWN_WRITE) == D_NET_OK) &&
          (d_tests_net_memory_is(d_net_connection_write(a,
                                                        "d",
                                                        1u),
                                 0u,
                                 D_NET_ERROR_CLOSED))                   &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       8u),
                                 3u,
                                 D_NET_OK))                             &&
          (d_net_connection_available(b,
                                      &count))                          &&
          (count == 0u)                                                 &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_OK))                             &&
          (d_net_connection_is_open(a)) );
    const bool reading =
        ( (d_net_write_all(b,
                           "uvw",
                           3u,
                           NULL) == D_NET_OK)                          &&
          (d_tests_net_memory_is(d_net_connection_read(a,
                                                       read,
                                                       8u),
                                 3u,
                                 D_NET_OK))                            &&
          (d_net_write_all(b,
                           "xyz",
                           3u,
                           NULL) == D_NET_OK)                          &&
          (d_net_connection_shutdown(a,
                                     D_NET_SHUTDOWN_READ) == D_NET_OK) &&
          (d_tests_net_memory_is(d_net_connection_read(a,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_OK))                            &&
          (d_net_connection_available(a,
                                      &count))                         &&
          (count == 0u) );

    return d_tests_net_assert( ( (written) &&
                                 (reading) ),
                               "d_net_pipe ends",
                               "half-closes drain, then end the stream",
                               _counter);
}

/*
d_tests_net_pipe_closed
  End A, having sent four bytes, is closed twice: it then does nothing, and
end B drains the four bytes, reads the end of the stream, and finds no one
reading its writes.
*/
static bool
d_tests_net_pipe_closed(
    struct d_net_pipe*     _pipe,
    struct d_test_counter* _counter
)
{
    struct d_net_connection* const a       = d_net_pipe_connection(
                                                 _pipe,
                                                 D_NET_PIPE_A);
    struct d_net_connection* const b       = d_net_pipe_connection(
                                                 _pipe,
                                                 D_NET_PIPE_B);
    unsigned char                  read[8] = { 0 };
    size_t                         count   = 5u;

    (void)d_net_write_all(a,
                          "left",
                          4u,
                          NULL);
    d_net_connection_close(a);
    d_net_connection_close(a);

    const bool closed =
        ( (!d_net_connection_is_open(a))              &&
          (d_tests_net_memory_is(d_net_connection_read(a,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_ERROR_CLOSED)) &&
          (d_tests_net_memory_is(d_net_connection_write(a,
                                                        "x",
                                                        1u),
                                 0u,
                                 D_NET_ERROR_CLOSED)) &&
          (d_net_connection_shutdown(a,
                                     D_NET_SHUTDOWN_BOTH) ==
           D_NET_ERROR_CLOSED)                        &&
          (!d_net_connection_available(a,
                                       &count))       &&
          (count == 0u) );
    const bool peer =
        ( (d_net_connection_is_open(b))     &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       8u),
                                 4u,
                                 D_NET_OK)) &&
          (d_tests_net_memory_is(d_net_connection_read(b,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_OK)) &&
          (d_tests_net_memory_is(d_net_connection_write(b,
                                                        "y",
                                                        1u),
                                 0u,
                                 D_NET_ERROR_CONNECTION_RESET)) );

    return ( (d_tests_net_assert(closed,
                                 "d_net_pipe ends",
                                 "a closed end does nothing, twice over",
                                 _counter)) &&
             (d_tests_net_assert(peer,
                                 "d_net_pipe ends",
                                 "its peer drains, ends, and is reset",
                                 _counter)) );
}

/*
d_tests_sa_net_pipe_ends
  Tests the following:
  - half-closes, in each direction (d_tests_net_pipe_halves)
  - a closed end reads and writes CLOSED, cannot count, and shuts down
    nothing; closing again does nothing (d_tests_net_pipe_closed)
  - the other end drains what was sent and reads the end of the stream, and
    its writes report a peer that has gone (d_tests_net_pipe_closed)
*/
bool
d_tests_sa_net_pipe_ends(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe pipe     = { .rings[0].size = 0u };
    unsigned char     to_b[16] = { 0 };
    unsigned char     to_a[16] = { 0 };
    bool              result   = true;

    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));
    result = d_tests_net_pipe_halves(&pipe,
                                     _counter) && result;
    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));

    return d_tests_net_pipe_closed(&pipe,
                                   _counter) && result;
}

/*
d_tests_net_memory_writes
  The writing half of d_tests_sa_net_memory: a sink takes what it can, a
short one reports the peer gone, and a NULL one discards.
*/
static bool
d_tests_net_memory_writes(
    struct d_test_counter* _counter
)
{
    struct d_net_memory            memory     = { .open = false };
    unsigned char                  storage[4] = { 0 };
    struct d_tests_net_sink_buffer buffer     = { storage,
                                                  sizeof(storage),
                                                  0u };
    const struct d_pack_sink       discard    = { NULL,
                                                  NULL };

    d_net_memory_init(&memory,
                      NULL,
                      0u,
                      d_tests_net_sink(&buffer));

    const bool kept =
        ( (d_tests_net_memory_is(d_net_connection_write(&memory.base,
                                                        "ab",
                                                        2u),
                                 2u,
                                 D_NET_OK))                     &&
          (d_tests_net_memory_is(d_net_connection_write(&memory.base,
                                                        "cdef",
                                                        4u),
                                 2u,
                                 D_NET_ERROR_CONNECTION_RESET)) &&
          (buffer.length == 4u)                                 &&
          (memcmp(storage,
                  "abcd",
                  4u) == 0) );

    d_net_memory_init(&memory,
                      NULL,
                      0u,
                      discard);

    const bool discarded =
        (d_tests_net_memory_is(d_net_connection_write(&memory.base,
                                                      "gone",
                                                      4u),
                               4u,
                               D_NET_OK));

    return d_tests_net_assert( ( (kept) &&
                                 (discarded) ),
                               "d_net_memory",
                               "sinks keep, refuse, or discard writes",
                               _counter);
}

/*
d_tests_net_memory_afresh
  The last part of d_tests_net_memory_reads: `_memory`, read in chunks and
told to stall, is made again over new input, which it reads from its start,
unlimited, and then ends.
*/
static bool
d_tests_net_memory_afresh(
    struct d_net_memory* _memory
)
{
    const struct d_pack_sink discard = { NULL,
                                         NULL };
    char                     read[8] = { 0 };

    d_net_memory_init(_memory,
                      "uvwxyz",
                      6u,
                      discard);

    return ( (d_tests_net_memory_is(d_net_connection_read(&_memory->base,
                                                          read,
                                                          8u),
                                    6u,
                                    D_NET_OK)) &&
             (memcmp(read,
                     "uvwxyz",
                     6u) == 0)                 &&
             (d_tests_net_memory_is(d_net_connection_read(&_memory->base,
                                                          read,
                                                          8u),
                                    0u,
                                    D_NET_OK)) );
}

/*
d_tests_net_memory_reads
  The reading half of d_tests_sa_net_memory: reads follow the input in
chunks, end the stream at its end, or stall there when asked to; a stream
made again over new input starts it afresh, unlimited and ending at its end.
*/
static bool
d_tests_net_memory_reads(
    struct d_test_counter* _counter
)
{
    const struct d_pack_sink discard = { NULL,
                                         NULL };
    struct d_net_memory      memory  = { .open = false };
    char                     read[8] = { 0 };
    size_t                   count   = 0u;

    d_net_memory_init(&memory,
                      "abcdefgh",
                      8u,
                      discard);
    memory.chunk = 3u;

    const bool chunked =
        ( (d_tests_net_memory_is(d_net_connection_read(&memory.base,
                                                       read,
                                                       8u),
                                 3u,
                                 D_NET_OK))      &&
          (d_net_connection_available(&memory.base,
                                      &count))   &&
          (count == 5u)                          &&
          (d_net_read_exactly(&memory.base,
                              read + 3,
                              5u,
                              NULL) == D_NET_OK) &&
          (memcmp(read,
                  "abcdefgh",
                  8u) == 0)                      &&
          (d_tests_net_memory_is(d_net_connection_read(&memory.base,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_OK)) );

    memory.block_at_end = true;

    const bool stalled =
        ( (d_tests_net_memory_is(d_net_connection_read(&memory.base,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_ERROR_WOULD_BLOCK)) &&
          (d_net_connection_shutdown(&memory.base,
                                     D_NET_SHUTDOWN_BOTH) == D_NET_OK) );

    const bool afresh = d_tests_net_memory_afresh(&memory);

    return d_tests_net_assert( ( (chunked) &&
                                 (stalled) &&
                                 (afresh) ),
                               "d_net_memory",
                               "reads follow the input, then end or stall",
                               _counter);
}

/*
d_tests_sa_net_memory
  Tests the following:
  - reads follow the input, cut by the chunk limit, and end the stream at
    its end, or would block there when asked to (d_tests_net_memory_reads)
  - a NULL input reads as empty, and a NULL stream is ignored
  - available counts the input left; a closed stream cannot say, and reads
    and writes CLOSED; the missing half-close succeeds
  - writes (d_tests_net_memory_writes)
*/
bool
d_tests_sa_net_memory(
    struct d_test_counter* _counter
)
{
    const struct d_pack_sink discard = { NULL,
                                         NULL };
    struct d_net_memory      memory  = { .open = false };
    char                     read[8] = { 0 };
    size_t                   count   = 0u;
    bool                     result  = true;

    d_net_memory_init(NULL,
                      "x",
                      1u,
                      discard);
    d_net_memory_init(&memory,
                      "abc",
                      3u,
                      discard);
    d_net_connection_close(&memory.base);

    const bool closed =
        ( (!d_net_connection_is_open(&memory.base))   &&
          (!d_net_connection_available(&memory.base,
                                       &count))       &&
          (d_tests_net_memory_is(d_net_connection_read(&memory.base,
                                                       read,
                                                       8u),
                                 0u,
                                 D_NET_ERROR_CLOSED)) &&
          (d_tests_net_memory_is(d_net_connection_write(&memory.base,
                                                        "x",
                                                        1u),
                                 0u,
                                 D_NET_ERROR_CLOSED)) );

    d_net_memory_init(&memory,
                      NULL,
                      5u,
                      discard);

    const bool empty =
        (d_tests_net_memory_is(d_net_connection_read(&memory.base,
                                                     read,
                                                     8u),
                               0u,
                               D_NET_OK));

    result = d_tests_net_assert( ( (closed) &&
                                   (empty) ),
                                 "d_net_memory",
                                 "closed and empty streams read nothing",
                                 _counter) && result;
    result = d_tests_net_memory_reads(_counter) && result;

    return d_tests_net_memory_writes(_counter) && result;
}
