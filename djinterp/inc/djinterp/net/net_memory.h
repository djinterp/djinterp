/*******************************************************************************
* djinterp [net]                                                    net_memory.h
*
* In-memory connections: net.h's connection over caller memory.
*   Two backends, each embedding d_net_connection as its first member, so all
* of net.h runs over them as it runs over a socket:
*     - a pipe: two bounded rings joined back to back, whose ends are both
*       connections. What one end writes the other reads, so a client and a
*       server -- TLS sessions over them included -- can talk in one thread
*       with no socket at all                                             [2]
*     - a memory stream: reads from fixed input and writes into a sink, for
*       feeding a protocol a scripted conversation and keeping its answer [3]
*   Neither waits nor allocates. An empty pipe answers a read with
* D_NET_ERROR_WOULD_BLOCK and a full one a write, and a chunk limit forces
* short reads and writes, so every path a non-blocking caller resumes along
* gets exercised.
*   C99 or later; declarations have C linkage. Link net_memory.c and net.c.
*
*
* path:      /inc/djinterp/net/net_memory.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RINGS
    -----
    1.  The ring
         1.  d_net_ring
2.  PIPES
    -----
    1.  Types
         1.  d_net_pipe_side
         2.  d_net_pipe_end
         3.  d_net_pipe
    2.  Operations
3.  MEMORY STREAMS
    --------------
    1.  Types
         1.  d_net_memory
    2.  Operations
*/

#ifndef DJINTERP_NET_NET_MEMORY_H
#define DJINTERP_NET_NET_MEMORY_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../c/djinterp.h"          // framework root
#include "../c/util/sink_common.h"  // d_pack_sink
#include "./net_common.h"           // d_net_connection, d_net_error


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  RINGS
//==============================================================================
// The storage behind a pipe, as plain data. A ring's fields are the
// module's: a pipe keeps them consistent.


// 1.1    The ring
//------------------------------------------------------------------------------
// 1.1.1
// d_net_ring
//   struct: a bounded byte ring over caller storage, written by one pipe end
// and read by the other. `closed` marks its writer finished, `abandoned` its
// reader gone.
struct d_net_ring
{
    unsigned char* data;       // the caller's storage
    size_t         capacity;   // its size in bytes
    size_t         head;       // where the next byte is read
    size_t         size;       // bytes waiting
    bool           closed;     // the writer will write no more
    bool           abandoned;  // the reader will read no more
};


//==============================================================================
// 2.  PIPES
//==============================================================================
// A two-ended byte stream in memory: ring 0 carries what end A writes to end
// B, and ring 1 the reverse. Each end is a connection with these semantics:
//     - A read takes what the end's inbound ring holds, up to the request
//       and the end's chunk limit. An empty ring is the end of the stream,
//       a count of 0 with D_NET_OK, once the other end has shut down its
//       writing or closed, and D_NET_ERROR_WOULD_BLOCK until then.
//     - A write puts what the outbound ring has room for, up to the chunk
//       limit; a full ring is D_NET_ERROR_WOULD_BLOCK. Writing after this
//       end shut down its writing is D_NET_ERROR_CLOSED, and after the other
//       end closed, with no one left to read, D_NET_ERROR_CONNECTION_RESET.
//     - Shutting down READ makes every later read the end of the stream;
//       WRITE lets the other end drain what was sent and then read the end;
//       BOTH does both.
//     - Closing does both and more: the end is no longer open, and its own
//       reads and writes report D_NET_ERROR_CLOSED. Closing again does
//       nothing.
//     - available counts the bytes a read would return; it is false on a
//       closed end. Neither end has an address.
// An initialized pipe holds pointers into itself, so it must not be copied or
// moved.


// 2.1    Types
//------------------------------------------------------------------------------
// 2.1.1
// d_net_pipe_side
//   enum: names an end of a pipe.
enum d_net_pipe_side
{
    D_NET_PIPE_A = 0,
    D_NET_PIPE_B = 1
};

// 2.1.2
// d_net_pipe_end
//   struct: one end of a pipe, and the connection over it. base comes first,
// so &end->base is a d_net_connection* that every net.h function accepts.
struct d_net_pipe_end
{
    struct d_net_connection base;      // the net.h view; must stay first
    struct d_net_ring*      inbound;   // the ring this end reads
    struct d_net_ring*      outbound;  // the ring this end writes
    size_t                  chunk;     // most bytes per read or write; 0: any
    bool                    open;      // until closed
    bool                    reading;   // until its reading is shut down
};

// 2.1.3
// d_net_pipe
//   struct: both rings, and both ends.
struct d_net_pipe
{
    struct d_net_ring     rings[2];  // A to B, then B to A
    struct d_net_pipe_end ends[2];   // A, then B
};

// 2.2    Operations
//------------------------------------------------------------------------------
/**
 * @brief Makes a pipe over caller storage, both ends open and empty.
 *
 * @param[out] _pipe        the pipe; it must not move once initialized.
 * @param[in]  _buffer_a    storage for the bytes end A sends; it must
 *                          outlive the pipe.
 * @param[in]  _capacity_a  its size in bytes; at least 1.
 * @param[in]  _buffer_b    storage for the bytes end B sends; it must
 *                          outlive the pipe.
 * @param[in]  _capacity_b  its size in bytes; at least 1.
 * @return D_NET_OK, or D_NET_ERROR_INVALID_ARGUMENT for a NULL argument or a
 *         zero capacity, the pipe then left as it was.
 */
enum d_net_error         d_net_pipe_init(struct d_net_pipe* _pipe,
                                         void*              _buffer_a,
                                         size_t             _capacity_a,
                                         void*              _buffer_b,
                                         size_t             _capacity_b);

// ends and limits -- d_net_pipe_connection returns end _side's connection,
// or NULL for a NULL pipe or an unknown side; d_net_pipe_set_chunk caps that
// end's reads and writes at _chunk bytes, 0 lifting the cap, and ignores a
// NULL pipe or an unknown side
struct d_net_connection* d_net_pipe_connection(struct d_net_pipe*   _pipe,
                                               enum d_net_pipe_side _side);
void                     d_net_pipe_set_chunk(struct d_net_pipe*   _pipe,
                                              enum d_net_pipe_side _side,
                                              size_t               _chunk);


//==============================================================================
// 3.  MEMORY STREAMS
//==============================================================================
// A connection whose reads are fixed in advance and whose writes are kept: a
// protocol under test reads a peer's whole side of a conversation and its
// own goes to a sink. Its semantics:
//     - A read takes the next bytes of the input, up to the request and the
//       chunk limit. Past the input's end it is the end of the stream, or
//       D_NET_ERROR_WOULD_BLOCK when block_at_end is set.
//     - A write hands its bytes to the output sink, which a NULL `write`
//       makes a discard. A sink that takes fewer than it is given has
//       stopped listening: the write reports what it took, and
//       D_NET_ERROR_CONNECTION_RESET.
//     - Closing makes the stream no longer open, and its reads and writes
//       report D_NET_ERROR_CLOSED. It has no half-close, and no address.
//     - available counts the input not yet read; it is false once closed.


// 3.1    Types
//------------------------------------------------------------------------------
// 3.1.1
// d_net_memory
//   struct: a memory stream, and the connection over it. base comes first,
// as in a pipe end. `chunk` and `block_at_end` are the caller's to set after
// d_net_memory_init; every other field is the module's.
struct d_net_memory
{
    struct d_net_connection base;          // the net.h view; must stay first
    const unsigned char*    input;         // what reads return, in order
    size_t                  input_size;    // its length
    size_t                  input_offset;  // how much has been read
    size_t                  chunk;         // most bytes per read; 0: any
    bool                    block_at_end;  // the input's end would block
    bool                    open;          // until closed
    struct d_pack_sink      output;        // where writes go
};

// 3.2    Operations
//------------------------------------------------------------------------------
// d_net_memory_init -- an open stream at the start of `_input`, which it
// borrows, as it does `_output`'s context, with no chunk limit and the end
// of the stream at the input's end; a NULL `_input` reads as empty, and a
// NULL `_memory` is ignored
void                     d_net_memory_init(struct d_net_memory* _memory,
                                           const void*          _input,
                                           size_t               _input_size,
                                           struct d_pack_sink   _output);


D_EXTERN_C_END


#endif  // DJINTERP_NET_NET_MEMORY_H
