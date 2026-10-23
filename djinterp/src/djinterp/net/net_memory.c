/*******************************************************************************
* djinterp [net]                                                    net_memory.c
*
* The in-memory connections of net_memory.h.
*   A pipe end and a memory stream are each a table of operations over plain
* data; every byte moves by memcpy, and nothing here waits or allocates. The
* rings wrap, so a byte may be copied in two pieces, and both pieces are
* always taken or put together.
*
*
* path:      /src/djinterp/net/net_memory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../inc/djinterp/net/net_memory.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memcpy, memset
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../inc/djinterp/c/util/sink_common.h"  // d_pack_sink
#include "../../../inc/djinterp/net/net_common.h"      // d_net_connection


//==============================================================================
// 1.  RINGS
//==============================================================================

/*
d_net_ring_take
  Copies _count bytes, at most the ring's size, from its head, wrapping as
needed.
*/
static void
d_net_ring_take(
    struct d_net_ring* _ring,
    unsigned char*     _out,
    size_t             _count
)
{
    const size_t tail_room = _ring->capacity - _ring->head;
    const size_t first     = (tail_room < _count) ? tail_room
                                                  : _count;

    memcpy(_out,
           _ring->data + _ring->head,
           first);
    memcpy(_out + first,
           _ring->data,
           _count - first);

    _ring->head  = (_ring->head + _count) % _ring->capacity;
    _ring->size -= _count;

    return;
}

/*
d_net_ring_put
  Copies _count bytes, at most the ring's free space, to its tail, wrapping
as needed.
*/
static void
d_net_ring_put(
    struct d_net_ring*   _ring,
    const unsigned char* _data,
    size_t               _count
)
{
    const size_t tail      = (_ring->head + _ring->size) % _ring->capacity;
    const size_t tail_room = _ring->capacity - tail;
    const size_t first     = (tail_room < _count) ? tail_room
                                                  : _count;

    memcpy(_ring->data + tail,
           _data,
           first);
    memcpy(_ring->data,
           _data + first,
           _count - first);

    _ring->size += _count;

    return;
}

/*
d_net_limit
  The smaller of a count and a chunk limit, 0 meaning no limit.
*/
static size_t
d_net_limit(
    size_t _count,
    size_t _chunk
)
{
    return ( (_chunk > 0u) &&
             (_count > _chunk) ) ? _chunk
                                 : _count;
}

//==============================================================================
// 2.  PIPES
//==============================================================================

/*
d_net_pipe_read
  Whatever the inbound ring holds, up to the request and the chunk limit. An
empty ring ends the stream once its writer is done, and is "not yet" until
then; an end whose reading is shut down has nothing more to read.
*/
static struct d_net_io_result
d_net_pipe_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_net_pipe_end* const end = (struct d_net_pipe_end*)_connection;

    // a closed end reads nothing at all
    if (!end->open)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // a shut reading side, or a drained ring whose writer is done, is the end
    if ( (!end->reading) ||
         ( (end->inbound->size == 0u) &&
           (end->inbound->closed) ) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_OK);
    }

    // nothing yet
    if (end->inbound->size == 0u)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_WOULD_BLOCK);
    }

    const size_t wanted = (_size < end->inbound->size) ? _size
                                                       : end->inbound->size;
    const size_t count  = d_net_limit(wanted,
                                      end->chunk);

    d_net_ring_take(end->inbound,
                    (unsigned char*)_buffer,
                    count);

    return d_net_io_result_make(count,
                                D_NET_OK);
}

/*
d_net_pipe_write
  As much as the outbound ring has room for, up to the chunk limit. Writing
after this end's own shutdown is CLOSED; into a ring nobody will read,
CONNECTION_RESET, as a socket reports a peer that has gone.
*/
static struct d_net_io_result
d_net_pipe_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    struct d_net_pipe_end* const end = (struct d_net_pipe_end*)_connection;

    // a closed end, or one done writing, takes nothing
    if ( (!end->open) ||
         (end->outbound->closed) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // the other end has gone
    if (end->outbound->abandoned)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CONNECTION_RESET);
    }

    const size_t room = end->outbound->capacity - end->outbound->size;

    // a full ring takes nothing yet
    if (room == 0u)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_WOULD_BLOCK);
    }

    const size_t count = d_net_limit((_size < room) ? _size
                                                    : room,
                                     end->chunk);

    d_net_ring_put(end->outbound,
                   (const unsigned char*)_data,
                   count);

    return d_net_io_result_make(count,
                                D_NET_OK);
}

/*
d_net_pipe_is_open
  Open until closed; a half-closed end is still open.
*/
static bool
d_net_pipe_is_open(
    const struct d_net_connection* _connection
)
{
    return ((const struct d_net_pipe_end*)_connection)->open;
}

/*
d_net_pipe_shut
  Shuts one or both directions down: the outbound ring's writer is done, the
end no longer reads.
*/
static void
d_net_pipe_shut(
    struct d_net_pipe_end* _end,
    bool                   _reading,
    bool                   _writing
)
{
    // the other end drains what was sent, then reads the end of the stream
    if (_writing)
    {
        _end->outbound->closed = true;
    }

    // later reads end the stream
    if (_reading)
    {
        _end->reading = false;
    }

    return;
}

/*
d_net_pipe_close
  Both directions down, and the inbound ring abandoned, so the other end's
writes fail as they would to a socket closed under them.
*/
static void
d_net_pipe_close(
    struct d_net_connection* _connection
)
{
    struct d_net_pipe_end* const end = (struct d_net_pipe_end*)_connection;

    d_net_pipe_shut(end,
                    true,
                    true);
    end->inbound->abandoned = true;
    end->open               = false;

    return;
}

/*
d_net_pipe_shutdown
  The mode arrives validated: net.c checks it before dispatching.
*/
static enum d_net_error
d_net_pipe_shutdown(
    struct d_net_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    struct d_net_pipe_end* const end = (struct d_net_pipe_end*)_connection;

    // a closed end has no direction left
    if (!end->open)
    {
        return D_NET_ERROR_CLOSED;
    }

    d_net_pipe_shut(end,
                    (_mode != D_NET_SHUTDOWN_WRITE),
                    (_mode != D_NET_SHUTDOWN_READ));

    return D_NET_OK;
}

/*
d_net_pipe_available
  What a read would return now: the inbound ring's fill, or nothing once the
reading side is shut.
*/
static bool
d_net_pipe_available(
    const struct d_net_connection* _connection,
    size_t*                        _out_count
)
{
    const struct d_net_pipe_end* const end =
        (const struct d_net_pipe_end*)_connection;

    // a closed end cannot say
    if (!end->open)
    {
        return false;
    }

    *_out_count = (end->reading) ? end->inbound->size
                                 : 0u;

    return true;
}

/*
PIPE_OPERATIONS
  The table both ends of every pipe point at: no addresses, and no destroy,
the storage being the caller's.
*/
static const struct d_net_connection_vtable PIPE_OPERATIONS =
{
    d_net_pipe_read,
    d_net_pipe_write,
    d_net_pipe_is_open,
    d_net_pipe_close,
    d_net_pipe_shutdown,
    NULL,
    NULL,
    NULL,
    d_net_pipe_available
};

/*
d_net_pipe_side_ok
  A pipe has ends A and B and nothing else.
*/
static bool
d_net_pipe_side_ok(
    enum d_net_pipe_side _side
)
{
    return ( (_side == D_NET_PIPE_A) ||
             (_side == D_NET_PIPE_B) );
}

/*
d_net_pipe_init
  Ring 0 carries A to B and ring 1 B to A; each end reads one and writes the
other.
*/
enum d_net_error
d_net_pipe_init(
    struct d_net_pipe* _pipe,
    void*              _buffer_a,
    size_t             _capacity_a,
    void*              _buffer_b,
    size_t             _capacity_b
)
{
    // parameter validation
    if ( (!_pipe)              ||
         (!_buffer_a)          ||
         (_capacity_a == 0u)   ||
         (!_buffer_b)          ||
         (_capacity_b == 0u) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    memset(_pipe,
           0,
           sizeof(*_pipe));

    _pipe->rings[0].data     = (unsigned char*)_buffer_a;
    _pipe->rings[0].capacity = _capacity_a;
    _pipe->rings[1].data     = (unsigned char*)_buffer_b;
    _pipe->rings[1].capacity = _capacity_b;

    // each end reads the ring the other writes
    for (size_t side = 0u;
         side < 2u;
         side++)
    {
        struct d_net_pipe_end* const end = &_pipe->ends[side];

        (void)d_net_connection_init(&end->base,
                                    &PIPE_OPERATIONS);
        end->outbound = &_pipe->rings[side];
        end->inbound  = &_pipe->rings[1u - side];
        end->open     = true;
        end->reading  = true;
    }

    return D_NET_OK;
}

/*
d_net_pipe_connection
  The end's base, which is the end itself.
*/
struct d_net_connection*
d_net_pipe_connection(
    struct d_net_pipe*   _pipe,
    enum d_net_pipe_side _side
)
{
    // no pipe, or no such end
    if ( (!_pipe) ||
         (!d_net_pipe_side_ok(_side)) )
    {
        return NULL;
    }

    return &_pipe->ends[_side].base;
}

/*
d_net_pipe_set_chunk
  Zero lifts the limit.
*/
void
d_net_pipe_set_chunk(
    struct d_net_pipe*   _pipe,
    enum d_net_pipe_side _side,
    size_t               _chunk
)
{
    // a pipe and a real end, or nothing to change
    if ( (_pipe) &&
         (d_net_pipe_side_ok(_side)) )
    {
        _pipe->ends[_side].chunk = _chunk;
    }

    return;
}

//==============================================================================
// 3.  MEMORY STREAMS
//==============================================================================

/*
d_net_memory_read
  The next bytes of the input, up to the request and the chunk limit. The
input's end is the stream's end, or a stall when block_at_end says so.
*/
static struct d_net_io_result
d_net_memory_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_net_memory* const memory    = (struct d_net_memory*)_connection;
    const size_t               remaining = memory->input_size -
                                           memory->input_offset;

    // a closed stream reads nothing at all
    if (!memory->open)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // the input's end is a close or a stall
    if (remaining == 0u)
    {
        return d_net_io_result_make(0u,
                                    (memory->block_at_end)
                                        ? D_NET_ERROR_WOULD_BLOCK
                                        : D_NET_OK);
    }

    const size_t count = d_net_limit((_size < remaining) ? _size
                                                         : remaining,
                                     memory->chunk);

    memcpy(_buffer,
           memory->input + memory->input_offset,
           count);
    memory->input_offset += count;

    return d_net_io_result_make(count,
                                D_NET_OK);
}

/*
d_net_memory_write
  Hands the bytes to the sink. A NULL write discards, as net's sinks
elsewhere may; a sink that takes fewer bytes than it is given has stopped
listening, which the write reports after the bytes it did take.
*/
static struct d_net_io_result
d_net_memory_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    const struct d_net_memory* const memory =
        (const struct d_net_memory*)_connection;

    // a closed stream takes nothing
    if (!memory->open)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // a discarding sink takes everything
    if (!memory->output.write)
    {
        return d_net_io_result_make(_size,
                                    D_NET_OK);
    }

    const size_t taken = memory->output.write(memory->output.context,
                                              _data,
                                              _size);

    // a sink that stopped short has stopped listening
    if (taken < _size)
    {
        return d_net_io_result_make(taken,
                                    D_NET_ERROR_CONNECTION_RESET);
    }

    return d_net_io_result_make(_size,
                                D_NET_OK);
}

/*
d_net_memory_is_open
  Open until closed.
*/
static bool
d_net_memory_is_open(
    const struct d_net_connection* _connection
)
{
    return ((const struct d_net_memory*)_connection)->open;
}

/*
d_net_memory_close
  Nothing to release: the input and the sink are the caller's.
*/
static void
d_net_memory_close(
    struct d_net_connection* _connection
)
{
    ((struct d_net_memory*)_connection)->open = false;

    return;
}

/*
d_net_memory_available
  The input not yet read.
*/
static bool
d_net_memory_available(
    const struct d_net_connection* _connection,
    size_t*                        _out_count
)
{
    const struct d_net_memory* const memory =
        (const struct d_net_memory*)_connection;

    // a closed stream cannot say
    if (!memory->open)
    {
        return false;
    }

    *_out_count = memory->input_size - memory->input_offset;

    return true;
}

/*
MEMORY_OPERATIONS
  The table every memory stream points at: no half-close, no addresses, and
no destroy.
*/
static const struct d_net_connection_vtable MEMORY_OPERATIONS =
{
    d_net_memory_read,
    d_net_memory_write,
    d_net_memory_is_open,
    d_net_memory_close,
    NULL,
    NULL,
    NULL,
    NULL,
    d_net_memory_available
};

/*
d_net_memory_init
  The table is complete, so d_net_connection_init cannot refuse it.
*/
void
d_net_memory_init(
    struct d_net_memory* _memory,
    const void*          _input,
    size_t               _input_size,
    struct d_pack_sink   _output
)
{
    // nothing to initialize
    if (!_memory)
    {
        return;
    }

    (void)d_net_connection_init(&_memory->base,
                                &MEMORY_OPERATIONS);
    _memory->input        = (const unsigned char*)_input;
    _memory->input_size   = (_input) ? _input_size
                                     : 0u;
    _memory->input_offset = 0u;
    _memory->chunk        = 0u;
    _memory->block_at_end = false;
    _memory->open         = true;
    _memory->output       = _output;

    return;
}
