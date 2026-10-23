/*******************************************************************************
* djinterp [net]                                                   ssh_channel.c
*
*   Definitions for ssh_channel.h. Every call checks the channel and its
* session before reaching the engine, and a fatal engine result fails
* both. The last functions make a channel a net.h connection.
*
*
* path:      /src/djinterp/net/ssh/ssh_channel.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_channel.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/net/net_common.h"  // d_net_connection
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"  // d_ssh_engine_vtable
#include "../../../../inc/djinterp/net/ssh/ssh_session.h"  // d_ssh_session
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


// the table of a channel's connection view, which the last functions define
D_STATIC const struct d_net_connection_vtable*
d_ssh_internal_view_table(void);

/*
d_ssh_internal_channel_open
  File-local: opens a session channel, or a direct-tcpip one when `_host` is
set, into the caller's storage, whose view is a closed connection until the
channel opens.
*/
D_STATIC enum d_ssh_error
d_ssh_internal_channel_open(
    struct d_ssh_session* _session,
    const char*           _host,
    unsigned int          _port,
    struct d_ssh_channel* _channel
)
{
    memset(_channel,
           0,
           sizeof(*_channel));
    (void)d_net_connection_init(&_channel->base,
                                d_ssh_internal_view_table());
    _channel->exit_status = -1;

    if (_session->state != D_SSH_STATE_AUTHENTICATED)
    {
        return D_SSH_ERROR_STATE;
    }

    void*                  handle = NULL;
    const enum d_ssh_error status =
        _session->engine->channel_open(_session->handle,
                                       _host,
                                       _port,
                                       &handle);

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_settle(_session,
                                     status);
    }

    // an engine that reports success must have produced a channel
    if (!handle)
    {
        return d_ssh_internal_fail(_session,
                                   D_SSH_ERROR_PROTOCOL,
                                   "the engine opened no channel");
    }

    _channel->session = _session;
    _channel->handle  = handle;
    _channel->state   = D_SSH_CHANNEL_OPEN;

    return D_SSH_OK;
}

/*
d_ssh_internal_channel_ready
  File-local: whether a channel can carry a request or data now.
*/
D_STATIC enum d_ssh_error
d_ssh_internal_channel_ready(
    const struct d_ssh_channel* _channel
)
{
    if ( (!_channel->session)                     ||
         (!_channel->handle)                      ||
         (_channel->state != D_SSH_CHANNEL_OPEN)  ||
         (_channel->session->state != D_SSH_STATE_AUTHENTICATED) )
    {
        return D_SSH_ERROR_STATE;
    }

    return D_SSH_OK;
}

/*
d_ssh_internal_channel_settle
  File-local: applies an engine result to the session, and marks the channel
failed with it, keeping the status that failed it: a channel cannot outlive
its session's transport.
*/
D_STATIC enum d_ssh_error
d_ssh_internal_channel_settle(
    struct d_ssh_channel* _channel,
    enum d_ssh_error      _status
)
{
    const enum d_ssh_error status =
        d_ssh_internal_settle(_channel->session,
                              _status);

    if (d_ssh_internal_is_fatal(status))
    {
        _channel->state   = D_SSH_CHANNEL_FAILED;
        _channel->failure = status;
    }

    return status;
}

/*
d_ssh_internal_channel_request
  File-local: sends exec, subsystem, or shell. A session channel runs exactly
one program, so a second request is refused before it is sent.
*/
D_STATIC enum d_ssh_error
d_ssh_internal_channel_request(
    struct d_ssh_channel* _channel,
    const char*           _type,
    const char*           _value
)
{
    const enum d_ssh_error ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->requested)
    {
        return D_SSH_ERROR_STATE;
    }

    const enum d_ssh_error status =
        _channel->session->engine->channel_request(_channel->handle,
                                                   _type,
                                                   _value);

    if (status == D_SSH_OK)
    {
        _channel->requested = true;
    }

    return d_ssh_internal_channel_settle(_channel,
                                         status);
}

/*
d_ssh_channel_open
  A session channel carries nothing until a request starts a program on it.
*/
enum d_ssh_error
d_ssh_channel_open(
    struct d_ssh_session* _session,
    struct d_ssh_channel* _channel
)
{
    if ( (!_session) ||
         (!_channel) )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    return d_ssh_internal_channel_open(_session,
                                       NULL,
                                       0,
                                       _channel);
}

/*
d_ssh_channel_open_tcpip
  The destination is resolved by the server, not here.
*/
enum d_ssh_error
d_ssh_channel_open_tcpip(
    struct d_ssh_session* _session,
    const char*           _host,
    unsigned int          _port,
    struct d_ssh_channel* _channel
)
{
    if ( (!_session)          ||
         (!_host)             ||
         (_host[0] == '\0')   ||
         (_port == 0)         ||
         (_port > 65535)      ||
         (!_channel) )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    return d_ssh_internal_channel_open(_session,
                                       _host,
                                       _port,
                                       _channel);
}

/*
d_ssh_channel_request_pty
  Must precede the program's start, which is what `requested` records.
*/
enum d_ssh_error
d_ssh_channel_request_pty(
    struct d_ssh_channel* _channel,
    const char*           _term,
    unsigned int          _columns,
    unsigned int          _rows
)
{
    if ( (!_channel) ||
         (!_term)    ||
         (_term[0] == '\0') )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    const enum d_ssh_error ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->requested)
    {
        return D_SSH_ERROR_STATE;
    }

    if (!_channel->session->engine->channel_pty)
    {
        return D_SSH_ERROR_UNSUPPORTED;
    }

    const enum d_ssh_error status =
        _channel->session->engine->channel_pty(_channel->handle,
                                               _term,
                                               _columns,
                                               _rows);

    return d_ssh_internal_channel_settle(_channel,
                                         status);
}

/*
d_ssh_channel_exec
  The command is passed as it is; quoting is the caller's responsibility.
*/
enum d_ssh_error
d_ssh_channel_exec(
    struct d_ssh_channel* _channel,
    const char*           _command
)
{
    if ( (!_channel) ||
         (!_command) ||
         (_command[0] == '\0') )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    return d_ssh_internal_channel_request(_channel,
                                          "exec",
                                          _command);
}

/*
d_ssh_channel_subsystem
  A named service the server defines, such as "sftp".
*/
enum d_ssh_error
d_ssh_channel_subsystem(
    struct d_ssh_channel* _channel,
    const char*           _name
)
{
    if ( (!_channel) ||
         (!_name)    ||
         (_name[0] == '\0') )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    return d_ssh_internal_channel_request(_channel,
                                          "subsystem",
                                          _name);
}

/*
d_ssh_channel_shell
  The request carries no value.
*/
enum d_ssh_error
d_ssh_channel_shell(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    return d_ssh_internal_channel_request(_channel,
                                          "shell",
                                          NULL);
}

/*
d_ssh_channel_read
  An ended stream is D_SSH_ERROR_CLOSED, which changes neither the channel nor
the session. An engine that claims success with no byte, or with more than
there was room for, has broken its promise, and is treated as a stalled
transport, as in d_ssh_channel_write: a read of nothing must never pass for
the stream's end.
*/
enum d_ssh_error
d_ssh_channel_read(
    struct d_ssh_channel* _channel,
    enum d_ssh_stream     _stream,
    void*                 _buffer,
    size_t                _capacity,
    size_t*               _received
)
{
    if (_received)
    {
        *_received = 0;
    }

    if ( (!_channel)        ||
         (!_buffer)         ||
         (_capacity == 0)   ||
         (!_received)       ||
         ( (_stream != D_SSH_STREAM_STDOUT) &&
           (_stream != D_SSH_STREAM_STDERR) ) )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    const enum d_ssh_error ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    enum d_ssh_error status =
        _channel->session->engine->channel_read(_channel->handle,
                                                _stream,
                                                _buffer,
                                                _capacity,
                                                _received);

    // success brings at least a byte, and no more than fit
    if ( (status == D_SSH_OK) &&
         ( (*_received == 0) ||
           (*_received > _capacity) ) )
    {
        *_received = 0;
        status     = D_SSH_ERROR_TRANSPORT;
    }

    return d_ssh_internal_channel_settle(_channel,
                                         status);
}

/*
d_ssh_channel_write
  Loops until everything is written, since each engine write takes only what
the server's window allows. An engine that claims success without progress
is treated as a stalled transport rather than looped on forever.
*/
enum d_ssh_error
d_ssh_channel_write(
    struct d_ssh_channel* _channel,
    const void*           _data,
    size_t                _size
)
{
    if ( (!_channel) ||
         ( (!_data) &&
           (_size > 0) ) )
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    const enum d_ssh_error ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->eof_sent)
    {
        return D_SSH_ERROR_STATE;
    }

    const unsigned char* const bytes = (const unsigned char*)_data;
    size_t                     done  = 0;

    // as much as the window allows each time
    while (done < _size)
    {
        size_t           sent   = 0;
        enum d_ssh_error status =
            _channel->session->engine->channel_write(_channel->handle,
                                                     bytes + done,
                                                     _size - done,
                                                     &sent);

        if ( (status == D_SSH_OK) &&
             ( (sent == 0) ||
               (sent > (_size - done)) ) )
        {
            status = D_SSH_ERROR_TRANSPORT;
        }

        if (status != D_SSH_OK)
        {
            return d_ssh_internal_channel_settle(_channel,
                                                 status);
        }

        done += sent;
    }

    return D_SSH_OK;
}

/*
d_ssh_channel_send_eof
  Idempotent: a second EOF is not sent.
*/
enum d_ssh_error
d_ssh_channel_send_eof(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    const enum d_ssh_error ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->eof_sent)
    {
        return D_SSH_OK;
    }

    const enum d_ssh_error status =
        _channel->session->engine->channel_send_eof(_channel->handle);

    if (status == D_SSH_OK)
    {
        _channel->eof_sent = true;
    }

    return d_ssh_internal_channel_settle(_channel,
                                         status);
}

/*
d_ssh_channel_close
  A channel whose session can no longer exchange messages is simply marked
closed; the engine releases it when it is freed.
*/
enum d_ssh_error
d_ssh_channel_close(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return D_SSH_ERROR_INVALID_ARGUMENT;
    }

    if (_channel->state == D_SSH_CHANNEL_CLOSED)
    {
        return D_SSH_OK;
    }

    struct d_ssh_session* const session = _channel->session;

    if ( (_channel->state == D_SSH_CHANNEL_FAILED) ||
         (!session)                                ||
         (!_channel->handle)                       ||
         (session->state != D_SSH_STATE_AUTHENTICATED) )
    {
        _channel->state = D_SSH_CHANNEL_CLOSED;

        return D_SSH_OK;
    }

    int                    exit_status = -1;
    const enum d_ssh_error status      =
        session->engine->channel_close(_channel->handle,
                                       &exit_status);

    _channel->state = D_SSH_CHANNEL_CLOSED;

    if (status == D_SSH_OK)
    {
        _channel->exit_status = exit_status;
    }

    return d_ssh_internal_settle(session,
                                 status);
}

/*
d_ssh_channel_exit_status
  Meaningful only once the channel has closed.
*/
int
d_ssh_channel_exit_status(
    const struct d_ssh_channel* _channel
)
{
    if ( (!_channel) ||
         (_channel->state != D_SSH_CHANNEL_CLOSED) )
    {
        return -1;
    }

    return _channel->exit_status;
}

/*
d_ssh_channel_free
  Skips the engine when its session has already been released, which freed
the engine's channels along with it. The view stays a closed connection.
*/
void
d_ssh_channel_free(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return;
    }

    if ( (_channel->handle)  &&
         (_channel->session) &&
         (_channel->session->handle) )
    {
        _channel->session->engine->channel_free(_channel->handle);
    }

    memset(_channel,
           0,
           sizeof(*_channel));
    (void)d_net_connection_init(&_channel->base,
                                d_ssh_internal_view_table());
    _channel->exit_status = -1;

    return;
}

/*
d_ssh_internal_view_error
  File-local: what a read, write or shutdown through the view that failed
with `_status` reports, in net.h's terms. A failed channel answers with the
status that failed it, which d_ssh_internal_channel_settle kept; any other
keeps `_status`, and one the channel can no longer carry -- closed, past
EOF, in a session gone -- is a closed connection.
*/
D_STATIC enum d_net_error
d_ssh_internal_view_error(
    struct d_ssh_channel* _channel,
    enum d_ssh_error      _status
)
{
    // a failed channel keeps the status that failed it
    if (_channel->state == D_SSH_CHANNEL_FAILED)
    {
        return d_ssh_error_to_net(_channel->failure);
    }

    _channel->failure = _status;

    return (_status == D_SSH_ERROR_STATE) ? D_NET_ERROR_CLOSED
                                          : d_ssh_error_to_net(_status);
}

/*
d_ssh_internal_view_read
  File-local: the channel's standard output, whose end is a read of nothing.
*/
D_STATIC struct d_net_io_result
d_ssh_internal_view_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_ssh_channel* const channel  = (struct d_ssh_channel*)_connection;
    size_t                      received = 0u;
    const enum d_ssh_error      status   =
        d_ssh_channel_read(channel,
                           D_SSH_STREAM_STDOUT,
                           _buffer,
                           _size,
                           &received);

    // data, or the stream's end
    if ( (status == D_SSH_OK) ||
         (status == D_SSH_ERROR_CLOSED) )
    {
        return d_net_io_result_make(received,
                                    D_NET_OK);
    }

    return d_net_io_result_make(0u,
                                d_ssh_internal_view_error(channel,
                                                          status));
}

/*
d_ssh_internal_view_write
  File-local: everything given, as d_ssh_channel_write writes it, or a
failure with nothing counted.
*/
D_STATIC struct d_net_io_result
d_ssh_internal_view_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    struct d_ssh_channel* const channel = (struct d_ssh_channel*)_connection;
    const enum d_ssh_error      status  = d_ssh_channel_write(channel,
                                                              _data,
                                                              _size);

    return (status == D_SSH_OK)
               ? d_net_io_result_make(_size,
                                      D_NET_OK)
               : d_net_io_result_make(0u,
                                      d_ssh_internal_view_error(channel,
                                                                status));
}

/*
d_ssh_internal_view_is_open
  File-local: open for as long as the channel can carry data.
*/
D_STATIC bool
d_ssh_internal_view_is_open(
    const struct d_net_connection* _connection
)
{
    return (d_ssh_internal_channel_ready(
                (const struct d_ssh_channel*)_connection) == D_SSH_OK);
}

/*
d_ssh_internal_view_close
  File-local: EOF, then the channel closed. Each is harmless where it has
nothing to do, and the storage stays the caller's.
*/
D_STATIC void
d_ssh_internal_view_close(
    struct d_net_connection* _connection
)
{
    struct d_ssh_channel* const channel = (struct d_ssh_channel*)_connection;

    (void)d_ssh_channel_send_eof(channel);
    (void)d_ssh_channel_close(channel);

    return;
}

/*
d_ssh_internal_view_shutdown
  File-local: sending ends with EOF; SSH has nothing to say of reading. A
channel that can carry nothing answers as its reads would. The mode arrives
validated.
*/
D_STATIC enum d_net_error
d_ssh_internal_view_shutdown(
    struct d_net_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    struct d_ssh_channel* const channel = (struct d_ssh_channel*)_connection;

    // a channel gone, or failed, says so
    if (d_ssh_internal_channel_ready(channel) != D_SSH_OK)
    {
        return d_ssh_internal_view_error(channel,
                                         D_SSH_ERROR_STATE);
    }

    // reading needs nothing from the server
    if (_mode == D_NET_SHUTDOWN_READ)
    {
        return D_NET_OK;
    }

    const enum d_ssh_error status = d_ssh_channel_send_eof(channel);

    return (status == D_SSH_OK) ? D_NET_OK
                                : d_ssh_internal_view_error(channel,
                                                            status);
}

/*
d_ssh_internal_view_table
  File-local: the table every channel's base points at. No endpoints, which
are the session's socket's; no destroy, the storage being the caller's;
and no available, which no engine counts.
*/
D_STATIC const struct d_net_connection_vtable*
d_ssh_internal_view_table(void)
{
    static const struct d_net_connection_vtable CHANNEL_OPERATIONS =
    {
        d_ssh_internal_view_read,
        d_ssh_internal_view_write,
        d_ssh_internal_view_is_open,
        d_ssh_internal_view_close,
        d_ssh_internal_view_shutdown,
        NULL,
        NULL,
        NULL,
        NULL
    };

    return &CHANNEL_OPERATIONS;
}
