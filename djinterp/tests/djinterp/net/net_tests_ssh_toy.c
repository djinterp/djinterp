/*******************************************************************************
* djinterp [net]                                             net_tests_ssh_toy.c
*
* The toy SSH engine net's SSH and SFTP suites run against.
*   Definitions for net_tests_ssh_toy.h: an engine whose sessions let anyone
* in at once, and whose channels serve a script or relay to a socket, each
* call counted and any failure placed where a test asks.
*
*
* path:      /tests/djinterp/net/net_tests_ssh_toy.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./net_tests_ssh_toy.h"  // corresponding header
// std
#include <errno.h>   // errno, EINTR
#include <stdio.h>   // snprintf
#include <string.h>  // memcpy, memset
// posix
#include <sys/socket.h>  // send, socket
#include <unistd.h>      // close, read
// djinterp
#include "../../../inc/djinterp/net/ssh/ssh_auth.h"  // d_ssh_authenticate


// D_TESTS_SSH_TOY_SEND_FLAGS
//   constant: the flags a relayed write is sent with, so that a server gone
// is a failed write rather than a SIGPIPE, where the platform can say so.
#if defined(MSG_NOSIGNAL)
    #define D_TESTS_SSH_TOY_SEND_FLAGS MSG_NOSIGNAL
#else
    #define D_TESTS_SSH_TOY_SEND_FLAGS 0
#endif

// HOST_KEY
//   constant: the host key every toy session presents: an ed25519 key's
// shape -- its type, then 32 bytes -- holding no real key.
static const unsigned char HOST_KEY[] =
{
    0, 0, 0, 11, 's', 's', 'h', '-', 'e', 'd', '2', '5', '5', '1', '9',
    0, 0, 0, 32,
    't', 'o', 'y', ' ', 'h', 'o', 's', 't', ' ', 'k', 'e', 'y', ' ', 'f',
    'o', 'r', ' ', 'n', 'e', 't', '\'', 's', ' ', 's', 's', 'h', ' ', 's',
    'u', 'i', 't', 'e'
};

// HOST
//   constant: the host every toy session is opened for.
static const char* const HOST = "ssh.example";

// the toy d_tests_ssh_toy_connect is starting, which the engine's create
// takes as its session; NULL outside it
static struct d_tests_ssh_toy* d_tests_ssh_toy_starting = NULL;

// the session
/*
d_tests_ssh_toy_once
  The failure set up in `*_fails`, cleared so that it happens once.
*/
static enum d_ssh_error
d_tests_ssh_toy_once(
    enum d_ssh_error* _fails
)
{
    const enum d_ssh_error status = *_fails;

    *_fails = D_SSH_OK;

    return status;
}

/*
d_tests_ssh_toy_create
  The session is the toy being started; outside d_tests_ssh_toy_connect
there is none to make.
*/
static enum d_ssh_error
d_tests_ssh_toy_create(
    const struct d_ssh_algorithms* _algorithms,
    unsigned int                   _timeout_ms,
    void**                         _handle
)
{
    (void)_algorithms;
    (void)_timeout_ms;
    *_handle = d_tests_ssh_toy_starting;

    return (d_tests_ssh_toy_starting) ? D_SSH_OK
                                      : D_SSH_ERROR_UNSUPPORTED;
}

/*
d_tests_ssh_toy_handshake
  Nothing crosses the socket.
*/
static enum d_ssh_error
d_tests_ssh_toy_handshake(
    void*        _handle,
    d_ssh_socket _socket
)
{
    (void)_handle;
    (void)_socket;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_host_key
  The same key for every session.
*/
static enum d_ssh_error
d_tests_ssh_toy_host_key(
    void*                 _handle,
    const unsigned char** _key,
    size_t*               _size
)
{
    (void)_handle;
    *_key  = HOST_KEY;
    *_size = sizeof(HOST_KEY);

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_auth_methods
  No methods: "none" has already let the user in.
*/
static enum d_ssh_error
d_tests_ssh_toy_auth_methods(
    void*        _handle,
    const char*  _user,
    const char** _methods
)
{
    (void)_handle;
    (void)_user;
    *_methods = NULL;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_auth_password
  Never asked, a user being in at once; refused should it be.
*/
static enum d_ssh_error
d_tests_ssh_toy_auth_password(
    void*       _handle,
    const char* _user,
    const char* _password
)
{
    (void)_handle;
    (void)_user;
    (void)_password;

    return D_SSH_ERROR_AUTH;
}

/*
d_tests_ssh_toy_auth_key_file
  Never asked; refused should it be.
*/
static enum d_ssh_error
d_tests_ssh_toy_auth_key_file(
    void*       _handle,
    const char* _user,
    const char* _public_key,
    const char* _private_key,
    const char* _passphrase
)
{
    (void)_handle;
    (void)_user;
    (void)_public_key;
    (void)_private_key;
    (void)_passphrase;

    return D_SSH_ERROR_AUTH;
}

/*
d_tests_ssh_toy_auth_agent
  Never asked; refused should it be.
*/
static enum d_ssh_error
d_tests_ssh_toy_auth_agent(
    void*       _handle,
    const char* _user
)
{
    (void)_handle;
    (void)_user;

    return D_SSH_ERROR_AUTH;
}

/*
d_tests_ssh_toy_auth_interactive
  Never asked; refused should it be.
*/
static enum d_ssh_error
d_tests_ssh_toy_auth_interactive(
    void*         _handle,
    const char*   _user,
    fn_ssh_prompt _prompt,
    void*         _context
)
{
    (void)_handle;
    (void)_user;
    (void)_prompt;
    (void)_context;

    return D_SSH_ERROR_AUTH;
}

/*
d_tests_ssh_toy_disconnect
  Counted.
*/
static enum d_ssh_error
d_tests_ssh_toy_disconnect(
    void*       _handle,
    const char* _reason
)
{
    struct d_tests_ssh_toy* const toy = (struct d_tests_ssh_toy*)_handle;

    (void)_reason;
    toy->disconnects += 1;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_detail
  The toy has one thing to say of any failure.
*/
static const char*
d_tests_ssh_toy_detail(
    void* _handle
)
{
    (void)_handle;

    return "the toy engine failed as it was told to";
}

/*
d_tests_ssh_toy_destroy
  Counted; the toy is the test's, and stays.
*/
static void
d_tests_ssh_toy_destroy(
    void* _handle
)
{
    struct d_tests_ssh_toy* const toy = (struct d_tests_ssh_toy*)_handle;

    toy->destroys += 1;

    return;
}

// channels
/*
d_tests_ssh_toy_channel_open
  The next of the toy's channels, unless a refusal was set up for this open
or none is left.
*/
static enum d_ssh_error
d_tests_ssh_toy_channel_open(
    void*        _handle,
    const char*  _host,
    unsigned int _port,
    void**       _channel
)
{
    struct d_tests_ssh_toy* const toy    = (struct d_tests_ssh_toy*)_handle;
    const enum d_ssh_error        status =
        d_tests_ssh_toy_once(&toy->open_fails);

    (void)_host;
    (void)_port;

    // a refusal set up for this open
    if (status != D_SSH_OK)
    {
        return status;
    }

    // every channel already given
    if (toy->opened >= D_TESTS_SSH_TOY_CHANNELS)
    {
        return D_SSH_ERROR_CHANNEL;
    }

    *_channel    = &toy->channels[toy->opened];
    toy->opened += 1u;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_channel_request
  Records the request, then grants it unless a refusal was set up for it.
*/
static enum d_ssh_error
d_tests_ssh_toy_channel_request(
    void*       _channel,
    const char* _type,
    const char* _value
)
{
    struct d_tests_ssh_toy_channel* const channel =
        (struct d_tests_ssh_toy_channel*)_channel;

    channel->requests += 1;
    (void)snprintf(channel->request,
                   sizeof(channel->request),
                   "%s",
                   _type);
    (void)snprintf(channel->value,
                   sizeof(channel->value),
                   "%s",
                   (_value) ? _value : "");

    return d_tests_ssh_toy_once(&channel->request_fails);
}

/*
d_tests_ssh_toy_relay_read
  What the descriptor reads, waiting for it: its end is the stream's, and
any failure the transport's.
*/
static enum d_ssh_error
d_tests_ssh_toy_relay_read(
    int     _relay,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    ssize_t got = -1;

    // a signal is no failure
    do
    {
        got = read(_relay,
                   _buffer,
                   _capacity);
    } while ( (got < 0) &&
              (errno == EINTR) );

    // bytes, or the end, or a failure
    if (got > 0)
    {
        *_received = (size_t)got;

        return D_SSH_OK;
    }

    return (got == 0) ? D_SSH_ERROR_CLOSED
                      : D_SSH_ERROR_TRANSPORT;
}

/*
d_tests_ssh_toy_script_read
  The next of the scripted bytes, at most `read_chunk` of them; their end is
the stream's.
*/
static enum d_ssh_error
d_tests_ssh_toy_script_read(
    struct d_tests_ssh_toy_channel* _channel,
    void*                           _buffer,
    size_t                          _capacity,
    size_t*                         _received
)
{
    const size_t left  = _channel->input_size - _channel->input_taken;
    size_t       count = (left < _capacity) ? left : _capacity;

    // the chunk a read is held to
    if ( (_channel->read_chunk > 0u) &&
         (count > _channel->read_chunk) )
    {
        count = _channel->read_chunk;
    }

    // the script is read
    if (count == 0u)
    {
        return D_SSH_ERROR_CLOSED;
    }

    memcpy(_buffer,
           _channel->input + _channel->input_taken,
           count);
    _channel->input_taken += count;
    *_received             = count;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_channel_read
  A failure set up for this read comes first, then a lie set up for it;
standard error has ended; standard output is relayed or scripted.
*/
static enum d_ssh_error
d_tests_ssh_toy_channel_read(
    void*             _channel,
    enum d_ssh_stream _stream,
    void*             _buffer,
    size_t            _capacity,
    size_t*           _received
)
{
    struct d_tests_ssh_toy_channel* const channel =
        (struct d_tests_ssh_toy_channel*)_channel;
    const enum d_ssh_error                failure =
        d_tests_ssh_toy_once(&channel->read_fails);

    channel->reads += 1;
    *_received      = 0u;

    // a failure set up for this read
    if (failure != D_SSH_OK)
    {
        return failure;
    }

    // a lie set up for it: success, with bytes never read
    if (channel->lies)
    {
        channel->lies = false;
        *_received    = channel->claims;

        return D_SSH_OK;
    }

    // standard error carries nothing
    if (_stream != D_SSH_STREAM_STDOUT)
    {
        return D_SSH_ERROR_CLOSED;
    }

    return (channel->relay >= 0)
               ? d_tests_ssh_toy_relay_read(channel->relay,
                                            _buffer,
                                            _capacity,
                                            _received)
               : d_tests_ssh_toy_script_read(channel,
                                             _buffer,
                                             _capacity,
                                             _received);
}

/*
d_tests_ssh_toy_relay_write
  As much as the descriptor takes at once; taking nothing is a failure.
*/
static enum d_ssh_error
d_tests_ssh_toy_relay_write(
    int         _relay,
    const void* _data,
    size_t      _size,
    size_t*     _sent
)
{
    ssize_t put = -1;

    // a signal is no failure
    do
    {
        put = send(_relay,
                   _data,
                   _size,
                   D_TESTS_SSH_TOY_SEND_FLAGS);
    } while ( (put < 0) &&
              (errno == EINTR) );

    // anything taken
    if (put > 0)
    {
        *_sent = (size_t)put;

        return D_SSH_OK;
    }

    return D_SSH_ERROR_TRANSPORT;
}

/*
d_tests_ssh_toy_script_write
  Keeps what fits, at most `write_chunk` bytes, and with `echo` makes it
readable too; a channel with no room left has stalled.
*/
static enum d_ssh_error
d_tests_ssh_toy_script_write(
    struct d_tests_ssh_toy_channel* _channel,
    const void*                     _data,
    size_t                          _size,
    size_t*                         _sent
)
{
    const size_t output_room = sizeof(_channel->output) -
                               _channel->output_size;
    const size_t input_room  = sizeof(_channel->input) -
                               _channel->input_size;
    const size_t room        = ( (_channel->echo) &&
                                 (input_room < output_room) ) ? input_room
                                                              : output_room;
    size_t       count       = (_size < room) ? _size : room;

    // the chunk a write is held to
    if ( (_channel->write_chunk > 0u) &&
         (count > _channel->write_chunk) )
    {
        count = _channel->write_chunk;
    }

    // no room: a window that never opens
    if (count == 0u)
    {
        return D_SSH_ERROR_TRANSPORT;
    }

    memcpy(_channel->output + _channel->output_size,
           _data,
           count);
    _channel->output_size += count;

    // what is written comes back
    if (_channel->echo)
    {
        (void)d_tests_ssh_toy_say(_channel,
                                  _data,
                                  count);
    }

    *_sent = count;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_channel_write
  A failure set up for this write comes first; then the data is relayed or
kept.
*/
static enum d_ssh_error
d_tests_ssh_toy_channel_write(
    void*       _channel,
    const void* _data,
    size_t      _size,
    size_t*     _sent
)
{
    struct d_tests_ssh_toy_channel* const channel =
        (struct d_tests_ssh_toy_channel*)_channel;
    const enum d_ssh_error                failure =
        d_tests_ssh_toy_once(&channel->write_fails);

    channel->writes += 1;
    *_sent           = 0u;

    // a failure set up for this write
    if (failure != D_SSH_OK)
    {
        return failure;
    }

    return (channel->relay >= 0)
               ? d_tests_ssh_toy_relay_write(channel->relay,
                                             _data,
                                             _size,
                                             _sent)
               : d_tests_ssh_toy_script_write(channel,
                                              _data,
                                              _size,
                                              _sent);
}

/*
d_tests_ssh_toy_channel_send_eof
  Counted, and granted unless a failure was set up for it.
*/
static enum d_ssh_error
d_tests_ssh_toy_channel_send_eof(
    void* _channel
)
{
    struct d_tests_ssh_toy_channel* const channel =
        (struct d_tests_ssh_toy_channel*)_channel;

    channel->eofs += 1;

    return d_tests_ssh_toy_once(&channel->eof_fails);
}

/*
d_tests_ssh_toy_channel_close
  Counted, with the exit status the test chose.
*/
static enum d_ssh_error
d_tests_ssh_toy_channel_close(
    void* _channel,
    int*  _exit_status
)
{
    struct d_tests_ssh_toy_channel* const channel =
        (struct d_tests_ssh_toy_channel*)_channel;

    channel->closes += 1;
    *_exit_status    = channel->exit_status;

    return D_SSH_OK;
}

/*
d_tests_ssh_toy_channel_free
  Recorded; the channel's storage is the toy's.
*/
static void
d_tests_ssh_toy_channel_free(
    void* _channel
)
{
    struct d_tests_ssh_toy_channel* const channel =
        (struct d_tests_ssh_toy_channel*)_channel;

    channel->freed = true;

    return;
}

// the toy
/*
d_tests_ssh_toy_init
  Zeroed -- D_SSH_OK is 0 -- with every channel scripted.
*/
void
d_tests_ssh_toy_init(
    struct d_tests_ssh_toy* _toy
)
{
    memset(_toy,
           0,
           sizeof(*_toy));

    // no channel relays until a test says so
    for (size_t i = 0u; i < D_TESTS_SSH_TOY_CHANNELS; ++i)
    {
        _toy->channels[i].relay = -1;
    }

    return;
}

/*
d_tests_ssh_toy_engine
  Every operation but channel_pty, which is optional.
*/
const struct d_ssh_engine_vtable*
d_tests_ssh_toy_engine(void)
{
    static const struct d_ssh_engine_vtable ENGINE =
    {
        .name             = "toy",
        .create           = d_tests_ssh_toy_create,
        .handshake        = d_tests_ssh_toy_handshake,
        .host_key         = d_tests_ssh_toy_host_key,
        .auth_methods     = d_tests_ssh_toy_auth_methods,
        .auth_password    = d_tests_ssh_toy_auth_password,
        .auth_key_file    = d_tests_ssh_toy_auth_key_file,
        .auth_agent       = d_tests_ssh_toy_auth_agent,
        .auth_interactive = d_tests_ssh_toy_auth_interactive,
        .channel_open     = d_tests_ssh_toy_channel_open,
        .channel_request  = d_tests_ssh_toy_channel_request,
        .channel_pty      = NULL,
        .channel_read     = d_tests_ssh_toy_channel_read,
        .channel_write    = d_tests_ssh_toy_channel_write,
        .channel_send_eof = d_tests_ssh_toy_channel_send_eof,
        .channel_close    = d_tests_ssh_toy_channel_close,
        .channel_free     = d_tests_ssh_toy_channel_free,
        .disconnect       = d_tests_ssh_toy_disconnect,
        .detail           = d_tests_ssh_toy_detail,
        .destroy          = d_tests_ssh_toy_destroy
    };

    return &ENGINE;
}

/*
d_tests_ssh_toy_connect
  Runs the session's own course -- init, connect, authenticate -- over the
toy. A socket refused by d_ssh_init is still the caller's, and closed here.
*/
bool
d_tests_ssh_toy_connect(
    struct d_tests_ssh_toy* _toy,
    struct d_ssh_session*   _session
)
{
    struct d_ssh_options     options     = d_ssh_options_default();
    struct d_ssh_credentials credentials = d_ssh_credentials_default();
    const int                socket_fd   = socket(AF_UNIX,
                                                  SOCK_STREAM,
                                                  0);

    options.engine          = d_tests_ssh_toy_engine();
    options.host            = HOST;
    options.host_key_policy = D_SSH_HOST_KEY_INSECURE_ACCEPT_ANY;
    credentials.user        = "toy";
    credentials.use_agent   = false;

    const bool made = ( (socket_fd >= 0) &&
                        (d_ssh_init(_session,
                                    &options,
                                    socket_fd,
                                    true) == D_SSH_OK) );

    // a refused socket is still ours
    if ( (!made) &&
         (socket_fd >= 0) )
    {
        (void)close(socket_fd);
    }

    d_tests_ssh_toy_starting = _toy;

    const bool authenticated =
        ( (made)                                &&
          (d_ssh_connect(_session) == D_SSH_OK) &&
          (d_ssh_authenticate(_session,
                              &credentials) == D_SSH_OK) );

    d_tests_ssh_toy_starting = NULL;

    return authenticated;
}

/*
d_tests_ssh_toy_say
  Appends to the script, if it fits.
*/
bool
d_tests_ssh_toy_say(
    struct d_tests_ssh_toy_channel* _channel,
    const void*                     _data,
    size_t                          _size
)
{
    // more than the script holds
    if (_size > (sizeof(_channel->input) - _channel->input_size))
    {
        return false;
    }

    memcpy(_channel->input + _channel->input_size,
           _data,
           _size);
    _channel->input_size += _size;

    return true;
}
