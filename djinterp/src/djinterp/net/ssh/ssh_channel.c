/*******************************************************************************
* djinterp [net]                                                   ssh_channel.c
*
*   Definitions for ssh_channel.h. Every call checks the channel and its
* session before reaching the engine, and a fatal engine result fails
* both.
*
*
* path:      /src/djinterp/net/ssh/ssh_channel.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_channel.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/net/ssh/ssh_engine.h"  // d_ssh_engine_vtable
#include "../../../../inc/djinterp/net/ssh/ssh_session.h"  // d_ssh_session
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


/*
d_ssh_internal_channel_open
  File-local: opens a session channel, or a direct-tcpip one when `_host` is
set, into the caller's storage.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_channel_open(
    struct d_ssh_session* _session,
    const char*           _host,
    unsigned int          _port,
    struct d_ssh_channel* _channel
)
{
    memset(_channel, 0, sizeof(*_channel));
    _channel->exit_status = -1;

    if (_session->state != D_SSH_STATE_AUTHENTICATED)
    {
        return D_SSH_ERR_STATE;
    }

    void*                   handle = NULL;
    const enum d_ssh_status status =
        _session->engine->channel_open(_session->handle,
                                       _host,
                                       _port,
                                       &handle);

    if (status != D_SSH_OK)
    {
        return d_ssh_internal_settle(_session, status);
    }

    // an engine that reports success must have produced a channel
    if (!handle)
    {
        return d_ssh_internal_fail(_session,
                                   D_SSH_ERR_PROTOCOL,
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
D_STATIC enum d_ssh_status
d_ssh_internal_channel_ready(
    const struct d_ssh_channel* _channel
)
{
    if ( (!_channel->session)                     ||
         (!_channel->handle)                      ||
         (_channel->state != D_SSH_CHANNEL_OPEN)  ||
         (_channel->session->state != D_SSH_STATE_AUTHENTICATED) )
    {
        return D_SSH_ERR_STATE;
    }

    return D_SSH_OK;
}

/*
d_ssh_internal_channel_settle
  File-local: applies an engine result to the session, and marks the channel
failed with it: a channel cannot outlive its session's transport.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_channel_settle(
    struct d_ssh_channel* _channel,
    enum d_ssh_status     _status
)
{
    const enum d_ssh_status status =
        d_ssh_internal_settle(_channel->session, _status);

    if (d_ssh_internal_is_fatal(status))
    {
        _channel->state = D_SSH_CHANNEL_FAILED;
    }

    return status;
}

/*
d_ssh_internal_channel_request
  File-local: sends exec, subsystem, or shell. A session channel runs exactly
one program, so a second request is refused before it is sent.
*/
D_STATIC enum d_ssh_status
d_ssh_internal_channel_request(
    struct d_ssh_channel* _channel,
    const char*           _type,
    const char*           _value
)
{
    const enum d_ssh_status ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->requested)
    {
        return D_SSH_ERR_STATE;
    }

    const enum d_ssh_status status =
        _channel->session->engine->channel_request(_channel->handle,
                                                   _type,
                                                   _value);

    if (status == D_SSH_OK)
    {
        _channel->requested = true;
    }

    return d_ssh_internal_channel_settle(_channel, status);
}

/*
d_ssh_channel_open
  A session channel carries nothing until a request starts a program on it.
*/
enum d_ssh_status
d_ssh_channel_open(
    struct d_ssh_session* _session,
    struct d_ssh_channel* _channel
)
{
    if ( (!_session) ||
         (!_channel) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    return d_ssh_internal_channel_open(_session, NULL, 0, _channel);
}

/*
d_ssh_channel_open_tcpip
  The destination is resolved by the server, not here.
*/
enum d_ssh_status
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
        return D_SSH_ERR_ARGUMENT;
    }

    return d_ssh_internal_channel_open(_session, _host, _port, _channel);
}

/*
d_ssh_channel_request_pty
  Must precede the program's start, which is what `requested` records.
*/
enum d_ssh_status
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
        return D_SSH_ERR_ARGUMENT;
    }

    const enum d_ssh_status ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->requested)
    {
        return D_SSH_ERR_STATE;
    }

    if (!_channel->session->engine->channel_pty)
    {
        return D_SSH_ERR_UNSUPPORTED;
    }

    const enum d_ssh_status status =
        _channel->session->engine->channel_pty(_channel->handle,
                                               _term,
                                               _columns,
                                               _rows);

    return d_ssh_internal_channel_settle(_channel, status);
}

/*
d_ssh_channel_exec
  The command is passed as it is; quoting is the caller's responsibility.
*/
enum d_ssh_status
d_ssh_channel_exec(
    struct d_ssh_channel* _channel,
    const char*           _command
)
{
    if ( (!_channel) ||
         (!_command) ||
         (_command[0] == '\0') )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    return d_ssh_internal_channel_request(_channel, "exec", _command);
}

/*
d_ssh_channel_subsystem
  A named service the server defines, such as "sftp".
*/
enum d_ssh_status
d_ssh_channel_subsystem(
    struct d_ssh_channel* _channel,
    const char*           _name
)
{
    if ( (!_channel) ||
         (!_name)    ||
         (_name[0] == '\0') )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    return d_ssh_internal_channel_request(_channel, "subsystem", _name);
}

/*
d_ssh_channel_shell
  The request carries no value.
*/
enum d_ssh_status
d_ssh_channel_shell(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    return d_ssh_internal_channel_request(_channel, "shell", NULL);
}

/*
d_ssh_channel_read
  An ended stream is D_SSH_ERR_CLOSED, which changes neither the channel nor
the session.
*/
enum d_ssh_status
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
        return D_SSH_ERR_ARGUMENT;
    }

    const enum d_ssh_status ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    const enum d_ssh_status status =
        _channel->session->engine->channel_read(_channel->handle,
                                                _stream,
                                                _buffer,
                                                _capacity,
                                                _received);

    return d_ssh_internal_channel_settle(_channel, status);
}

/*
d_ssh_channel_write
  Loops until everything is written, since each engine write takes only what
the server's window allows. An engine that claims success without progress
is treated as a stalled transport rather than looped on forever.
*/
enum d_ssh_status
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
        return D_SSH_ERR_ARGUMENT;
    }

    const enum d_ssh_status ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->eof_sent)
    {
        return D_SSH_ERR_STATE;
    }

    const unsigned char* const bytes = (const unsigned char*)_data;
    size_t                     done  = 0;

    // as much as the window allows each time
    while (done < _size)
    {
        size_t            sent   = 0;
        enum d_ssh_status status =
            _channel->session->engine->channel_write(_channel->handle,
                                                     bytes + done,
                                                     _size - done,
                                                     &sent);

        if ( (status == D_SSH_OK) &&
             ( (sent == 0) ||
               (sent > (_size - done)) ) )
        {
            status = D_SSH_ERR_IO;
        }

        if (status != D_SSH_OK)
        {
            return d_ssh_internal_channel_settle(_channel, status);
        }

        done += sent;
    }

    return D_SSH_OK;
}

/*
d_ssh_channel_send_eof
  Idempotent: a second EOF is not sent.
*/
enum d_ssh_status
d_ssh_channel_send_eof(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const enum d_ssh_status ready = d_ssh_internal_channel_ready(_channel);

    if (ready != D_SSH_OK)
    {
        return ready;
    }

    if (_channel->eof_sent)
    {
        return D_SSH_OK;
    }

    const enum d_ssh_status status =
        _channel->session->engine->channel_send_eof(_channel->handle);

    if (status == D_SSH_OK)
    {
        _channel->eof_sent = true;
    }

    return d_ssh_internal_channel_settle(_channel, status);
}

/*
d_ssh_channel_close
  A channel whose session can no longer exchange messages is simply marked
closed; the engine releases it when it is freed.
*/
enum d_ssh_status
d_ssh_channel_close(
    struct d_ssh_channel* _channel
)
{
    if (!_channel)
    {
        return D_SSH_ERR_ARGUMENT;
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

    int                     exit_status = -1;
    const enum d_ssh_status status      =
        session->engine->channel_close(_channel->handle, &exit_status);

    _channel->state = D_SSH_CHANNEL_CLOSED;

    if (status == D_SSH_OK)
    {
        _channel->exit_status = exit_status;
    }

    return d_ssh_internal_settle(session, status);
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
the engine's channels along with it.
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

    memset(_channel, 0, sizeof(*_channel));
    _channel->exit_status = -1;

    return;
}
