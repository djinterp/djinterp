/*******************************************************************************
* djinterp [net]                                          net_tests_sa_support.c
*
* The doubles the net foundation tests run against.
*   A scripted connection with two tables -- one supplying every optional
* operation, one only the required four -- and a sink over fixed storage.
*
*
* path:      /tests/djinterp/net/net_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "./net_tests_sa_support.h"  // corresponding header
// std
#include <string.h>  // memcpy, memset


/*
d_tests_net_mock_of
  Recovers the mock from its head, which is its first member.
*/
static struct d_tests_net_mock*
d_tests_net_mock_of(
    struct d_net_connection* _connection
)
{
    return (struct d_tests_net_mock*)_connection;
}

/*
d_tests_net_mock_read
  Delivers the next slice of the input, cut to the request and the read
limit; once the input is spent, reports the scripted error.
*/
static struct d_net_io_result
d_tests_net_mock_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_tests_net_mock* const mock = d_tests_net_mock_of(_connection);

    const size_t remaining = mock->input_size - mock->input_at;
    size_t       take      = (_size < remaining) ? _size : remaining;

    // a closed mock reads nothing
    if (!mock->open)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // a spent input reports the scripted ending
    if (remaining == 0u)
    {
        return d_net_io_result_make(0u,
                                    mock->read_error);
    }

    // honour the read limit
    if ( (mock->read_limit != 0u) &&
         (take > mock->read_limit) )
    {
        take = mock->read_limit;
    }

    memcpy(_buffer,
           mock->input + mock->input_at,
           take);
    mock->input_at += take;

    return d_net_io_result_make(take,
                                D_NET_ERROR_NONE);
}

/*
d_tests_net_mock_write
  Appends what fits, cut to the write limit; once the output is full,
reports the scripted error, which may be none.
*/
static struct d_net_io_result
d_tests_net_mock_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    struct d_tests_net_mock* const mock = d_tests_net_mock_of(_connection);

    const size_t room = mock->output_capacity - mock->output_length;
    size_t       take = (_size < room) ? _size : room;

    // a closed mock writes nothing
    if (!mock->open)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // a full output reports the scripted ending
    if (room == 0u)
    {
        return d_net_io_result_make(0u,
                                    mock->write_error);
    }

    // honour the write limit
    if ( (mock->write_limit != 0u) &&
         (take > mock->write_limit) )
    {
        take = mock->write_limit;
    }

    memcpy(mock->output + mock->output_length,
           _data,
           take);
    mock->output_length += take;
    mock->writes        += 1u;

    return d_net_io_result_make(take,
                                D_NET_ERROR_NONE);
}

/*
d_tests_net_mock_is_open
  Reports the flag close clears.
*/
static bool
d_tests_net_mock_is_open(
    const struct d_net_connection* _connection
)
{
    return ((const struct d_tests_net_mock*)_connection)->open;
}

/*
d_tests_net_mock_close
  Clears the flag and counts the call, every call, so tests can see how often
close ran.
*/
static void
d_tests_net_mock_close(
    struct d_net_connection* _connection
)
{
    struct d_tests_net_mock* const mock = d_tests_net_mock_of(_connection);

    mock->open    = false;
    mock->closes += 1u;

    return;
}

/*
d_tests_net_mock_shutdown
  Records the mode it was given.
*/
static enum d_net_error
d_tests_net_mock_shutdown(
    struct d_net_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    struct d_tests_net_mock* const mock = d_tests_net_mock_of(_connection);

    mock->shutdowns     += 1u;
    mock->last_shutdown  = _mode;

    return D_NET_ERROR_NONE;
}

/*
d_tests_net_mock_remote
  Knows its peer: 192.0.2.1:80, from the documentation address block.
*/
static bool
d_tests_net_mock_remote(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    const struct d_pack_text host = { "192.0.2.1", 9u };

    (void)_connection;

    return (d_net_endpoint_set(_out,
                               host,
                               80u,
                               D_NET_PROTOCOL_TCP) == D_NET_ERROR_NONE);
}

/*
d_tests_net_mock_local
  Scribbles on the output and then refuses, which the dispatch must not pass
on.
*/
static bool
d_tests_net_mock_local(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    (void)_connection;

    _out->host[0] = 'x';
    _out->host[1] = '\0';
    _out->port    = 9u;

    return false;
}

/*
d_tests_net_mock_destroy
  Counts the call; the mock's storage belongs to the test.
*/
static void
d_tests_net_mock_destroy(
    struct d_net_connection* _connection
)
{
    d_tests_net_mock_of(_connection)->destroys += 1u;

    return;
}

/*
d_tests_net_sink_write
  Takes what fits in the buffer and reports how much that was.
*/
static size_t
d_tests_net_sink_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_net_sink_buffer* const buffer =
        (struct d_tests_net_sink_buffer*)_context;

    const size_t room = buffer->capacity - buffer->length;
    const size_t take = (_size < room) ? _size : room;

    memcpy(buffer->data + buffer->length,
           _data,
           take);
    buffer->length += take;

    return take;
}

/*
d_tests_net_mock_init
  Zeroes the mock, so every counter starts at 0 and every error is none, then
points it at one of two static tables.
*/
void
d_tests_net_mock_init(
    struct d_tests_net_mock* _mock,
    bool                     _full
)
{
    static const struct d_net_connection_vtable FULL =
    {
        d_tests_net_mock_read,
        d_tests_net_mock_write,
        d_tests_net_mock_is_open,
        d_tests_net_mock_close,
        d_tests_net_mock_shutdown,
        d_tests_net_mock_remote,
        d_tests_net_mock_local,
        d_tests_net_mock_destroy
    };
    static const struct d_net_connection_vtable MINIMAL =
    {
        d_tests_net_mock_read,
        d_tests_net_mock_write,
        d_tests_net_mock_is_open,
        d_tests_net_mock_close,
        NULL,
        NULL,
        NULL,
        NULL
    };

    memset(_mock,
           0,
           sizeof(*_mock));
    (void)d_net_connection_init(&_mock->base,
                                (_full) ? &FULL : &MINIMAL);
    _mock->open = true;

    return;
}

/*
d_tests_net_mock_input
  Points the mock's reads at caller memory, from its start.
*/
void
d_tests_net_mock_input(
    struct d_tests_net_mock* _mock,
    const void*              _data,
    size_t                   _size
)
{
    _mock->input      = (const unsigned char*)_data;
    _mock->input_size = _size;
    _mock->input_at   = 0u;

    return;
}

/*
d_tests_net_mock_output
  Points the mock's writes at caller memory, empty.
*/
void
d_tests_net_mock_output(
    struct d_tests_net_mock* _mock,
    void*                    _buffer,
    size_t                   _capacity
)
{
    _mock->output          = (unsigned char*)_buffer;
    _mock->output_capacity = _capacity;
    _mock->output_length   = 0u;

    return;
}

/*
d_tests_net_sink
  Wraps the buffer as a sink, emptying it.
*/
struct d_pack_sink
d_tests_net_sink(
    struct d_tests_net_sink_buffer* _buffer
)
{
    const struct d_pack_sink sink = { d_tests_net_sink_write, _buffer };

    _buffer->length = 0u;

    return sink;
}

/*
d_tests_net_pattern
  A multiplier coprime to 256 makes every run of 256 consecutive bytes
distinct, so a slip by any offset shows.
*/
void
d_tests_net_pattern(
    unsigned char* _buffer,
    size_t         _size,
    unsigned int   _seed
)
{
    for (size_t i = 0u; i < _size; ++i)
    {
        _buffer[i] = (unsigned char)(((i * 31u) + _seed) & 0xFFu);
    }

    return;
}
