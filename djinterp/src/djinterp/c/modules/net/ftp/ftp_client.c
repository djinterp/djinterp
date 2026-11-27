/*******************************************************************************
* djinterp [c][net]                                                 ftp_client.c
*
* Implementation of the FTP client declared in ftp_client.h: the session,
* and the helpers ftp_client_data.c shares.
*   Every connection is a TCP connection read and written through net.h. A
* security layer, when the client has one, runs over it; a plain connection
* is read and written directly, its results put as the layer puts its own,
* so the protocol code above them never asks which is in force. Control
* replies are parsed from a read-ahead buffer; whatever the parser has not
* taken stays there for the next reply, which is also how an AUTH upgrade
* can tell that nothing was left over.
*
*
* path:      /src/djinterp/c/modules/net/ftp/ftp_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_client.h"  // corresponding header
// std
#include <limits.h>   // LONG_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcpy, memset, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"    // dj_pack_text
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_command.h"   // commands
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_common.h"    // dj_ftp_buffer
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_feature.h"   // dj_ftp_features
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_options.h"   // dj_ftp_options
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_reply.h"     // replies
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_security.h"  // PROT levels
#include "../../../../../../inc/djinterp/c/modules/net/net_common.h"        // dj_net_endpoint
#include "../../../../../../inc/djinterp/c/modules/net/tcp/tcp.h"           // d_tcp_connect
#include "./ftp_client_internal.h"                          // shared helpers
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint16_t, uint32_t


//==============================================================================
// 3.  CLIENT
//==============================================================================

/*
d_ftp_internal_client_plain_read
  File-local: reads the TCP connection directly: the end of the stream, a
count of 0 without an error, reads as CLOSED, and any error as TRANSPORT,
kept in the connection's `failure`.
*/
D_STATIC enum dj_ftp_io
d_ftp_internal_client_plain_read(
    struct dj_ftp_connection* _connection,
    void*                    _buffer,
    size_t                   _capacity,
    size_t*                  _out_read
)
{
    const struct dj_net_io_result result =
        d_net_connection_read(&_connection->tcp.base,
                              _buffer,
                              _capacity);

    *_out_read           = result.count;
    _connection->failure = result.error;

    // a failed read
    if (result.error != D_NET_OK)
    {
        return D_FTP_IO_TRANSPORT;
    }

    return (result.count == 0u) ? D_FTP_IO_CLOSED
                                : D_FTP_IO_OK;
}

/*
d_ftp_internal_client_plain_write
  File-local: writes all of `_data` to the TCP connection directly. A peer
that has gone reads as CLOSED, one that no longer reads; a write that took
nothing, or failed otherwise, is TRANSPORT.
*/
D_STATIC enum dj_ftp_io
d_ftp_internal_client_plain_write(
    struct dj_ftp_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    const unsigned char* const bytes = (const unsigned char*)_data;
    size_t                     done  = 0u;

    // looped over short writes
    while (done < _size)
    {
        const struct dj_net_io_result result =
            d_net_connection_write(&_connection->tcp.base,
                                   bytes + done,
                                   _size - done);

        _connection->failure = result.error;

        // a failed write
        if (result.error != D_NET_OK)
        {
            return ( (result.error == D_NET_ERROR_CLOSED)             ||
                     (result.error == D_NET_ERROR_CONNECTION_RESET)   ||
                     (result.error == D_NET_ERROR_CONNECTION_ABORTED) )
                   ? D_FTP_IO_CLOSED
                   : D_FTP_IO_TRANSPORT;
        }

        // a write that took nothing would loop forever
        if (result.count == 0u)
        {
            return D_FTP_IO_TRANSPORT;
        }

        done += result.count;
    }

    return D_FTP_IO_OK;
}

/*
d_ftp_internal_client_read
  Without the layer the connection is read directly; with it, the layer's
status for a failure is kept for d_ftp_internal_client_failure.
*/
enum dj_ftp_io
d_ftp_internal_client_read(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    void*                    _buffer,
    size_t                   _capacity,
    size_t*                  _out_read
)
{
    // without the layer, the connection itself
    if (!_connection->secured)
    {
        return d_ftp_internal_client_plain_read(_connection,
                                                _buffer,
                                                _capacity,
                                                _out_read);
    }

    return _client->security->read(_client->security_layer,
                                   _connection,
                                   _buffer,
                                   _capacity,
                                   _out_read,
                                   &_client->security_last);
}

/*
d_ftp_internal_client_write
  Without the layer the connection is written directly; with it, the
layer's status for a failure is kept for d_ftp_internal_client_failure.
*/
enum dj_ftp_io
d_ftp_internal_client_write(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    // without the layer, the connection itself
    if (!_connection->secured)
    {
        return d_ftp_internal_client_plain_write(_connection,
                                                 _data,
                                                 _size);
    }

    return _client->security->write(_client->security_layer,
                                    _connection,
                                    _data,
                                    _size,
                                    &_client->security_last);
}

/*
d_ftp_internal_client_shutdown
  Sends the layer's close on a secured connection; a plain one has none to
send.
*/
enum dj_ftp_io
d_ftp_internal_client_shutdown(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection
)
{
    // a plain connection has no close of its own
    if (!_connection->secured)
    {
        return D_FTP_IO_OK;
    }

    return _client->security->shutdown(_client->security_layer,
                                       _connection,
                                       &_client->security_last);
}

/*
d_ftp_internal_client_failure
  The transport's own error decides first, since a layer reports every
transport failure alike; the rest is the layer's. A layer keeps its
transport's last outcome itself, so when secured the connection's `failure`
is taken from it here, whichever call failed.
*/
enum dj_ftp_error
d_ftp_internal_client_failure(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    enum dj_ftp_io            _io,
    enum dj_ftp_error         _lost
)
{
    const bool framed = (_connection == &_client->control);

    // secured, the layer has the transport's last word
    if (_connection->secured)
    {
        _connection->failure =
            _client->security->transport_error(_client->security_layer,
                                               _connection);
    }

    _client->net_error = _connection->failure;

    // the transport's wait ran out
    if (_connection->failure == D_NET_ERROR_TIMED_OUT)
    {
        return D_FTP_ERROR_TIMED_OUT;
    }

    // the stream ended or broke
    if ( (_io == D_FTP_IO_CLOSED)    ||
         (_io == D_FTP_IO_TRANSPORT) ||
         ( (framed) &&
           (_io == D_FTP_IO_TRUNCATED) ) )
    {
        return _lost;
    }

    _client->security_status = _client->security_last;

    return D_FTP_ERROR_TLS;
}

/*
d_ftp_internal_client_dial_error
  File-local: the FTP error for a failed connection attempt. A host the
resolver rejects - a malformed address, a name that does not resolve
(HOST_NOT_FOUND), an address this host does not have - is
D_FTP_ERROR_RESOLVE; a host no route reaches stays `_lost`, with the exact
cause left in `net_error`.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_dial_error(
    enum dj_net_error _error,
    enum dj_ftp_error _lost
)
{
    switch (_error)
    {
        case D_NET_OK:
            return D_FTP_OK;

        case D_NET_ERROR_TIMED_OUT:
            return D_FTP_ERROR_TIMED_OUT;

        case D_NET_ERROR_ADDRESS_INVALID:
        case D_NET_ERROR_ADDRESS_UNAVAILABLE:
        case D_NET_ERROR_HOST_NOT_FOUND:
            return D_FTP_ERROR_RESOLVE;

        case D_NET_ERROR_OUT_OF_MEMORY:
            return D_FTP_ERROR_OUT_OF_MEMORY;

        default:
            break;
    }

    return _lost;
}

/*
d_ftp_internal_client_limit
  File-local: a limit of the options, in milliseconds, clamped into a long,
whose width varies: 32 bits on LLP64 platforms.
*/
D_STATIC
long d_ftp_internal_client_limit(
    uint32_t _limit_ms
)
{
    return (_limit_ms > (uint32_t)LONG_MAX) ? LONG_MAX
                                             : (long)_limit_ms;
}

/*
d_ftp_internal_client_bound
  The control connection waits for replies, so response_timeout_ms bounds
it; a data connection carries a transfer, so idle_timeout_ms does. 0 is no
limit, as d_tcp_connection_set_timeout reads it.
*/
enum dj_net_error
d_ftp_internal_client_bound(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection
)
{
    const uint32_t limit = (_connection == &_client->control)
                           ? _client->options.response_timeout_ms
                           : _client->options.idle_timeout_ms;

    return d_tcp_connection_set_timeout(&_connection->tcp,
                                        d_ftp_internal_client_limit(limit));
}

/*
d_ftp_internal_client_dial
  Connects within the options' connect timeout, then bounds the
connection's reads and writes by d_ftp_internal_client_bound; a limit the
socket refuses closes the connection again.
*/
enum dj_ftp_error
d_ftp_internal_client_dial(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    const char*              _host,
    uint16_t                 _port,
    enum dj_ftp_error         _lost
)
{
    const struct dj_pack_text name = { _host, strlen(_host) };
    struct dj_net_endpoint    where = { 0 };
    struct dj_tcp_options     options = { 0 };

    d_net_endpoint_init(&where);
    d_tcp_options_init(&options);

    options.connect_timeout_ms =
        d_ftp_internal_client_limit(_client->options.connect_timeout_ms);
    options.no_delay           = true;

    enum dj_net_error error = d_net_endpoint_set(&where,
                                                name,
                                                _port,
                                                D_NET_PROTOCOL_TCP);

    // a host the endpoint cannot hold makes no connection
    if (error == D_NET_OK)
    {
        error = d_tcp_connect(&_connection->tcp,
                              &where,
                              &options);
    }

    // bound its reads and writes; a limit the socket refuses closes it
    if (error == D_NET_OK)
    {
        error = d_ftp_internal_client_bound(_client,
                                            _connection);

        if (error != D_NET_OK)
        {
            d_tcp_connection_close(&_connection->tcp);
        }
    }

    _connection->failure = error;
    _client->net_error   = error;

    return d_ftp_internal_client_dial_error(error,
                                            _lost);
}

/*
d_ftp_internal_client_secure
  The layer names the server's host, which it checks the server against.
Every connection of a session uses the same layer and host, and a data
connection is offered the control connection to resume its session from: a
server may require it, as vsftpd does by default. Whether the server took it
is the handshake's to say.
*/
enum dj_ftp_error
d_ftp_internal_client_secure(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection
)
{
    const struct dj_ftp_connection* const resume =
        ( (_connection != &_client->control) &&
          (_client->control.secured) ) ? &_client->control
                                       : NULL;
    int32_t                              status = 0;
    const enum dj_ftp_io                 io     =
        (_client->security)
        ? _client->security->secure(_client->security_layer,
                                    _connection,
                                    resume,
                                    _client->host,
                                    &status)
        : D_FTP_IO_SECURITY;

    // a failure leaves nothing behind but the transport's error
    if (io != D_FTP_IO_OK)
    {
        _client->security_status = status;
        _client->net_error       = _connection->failure;

        return (_connection->failure == D_NET_ERROR_TIMED_OUT)
               ? D_FTP_ERROR_TIMED_OUT
               : D_FTP_ERROR_TLS;
    }

    _connection->secured = true;

    return D_FTP_OK;
}

/*
d_ftp_internal_client_drop
  The layer is released before the transport it runs over, and the
transport's error is forgotten with it.
*/
void d_ftp_internal_client_drop(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    bool                      _orderly
)
{
    // the layer goes before the transport it runs over
    if (_connection->secured)
    {
        // its close, when the close is orderly
        if (_orderly)
        {
            (void)d_ftp_internal_client_shutdown(_client,
                                                 _connection);
        }

        _client->security->drop(_client->security_layer,
                                _connection);
        _connection->secured = false;
    }

    d_tcp_connection_close(&_connection->tcp);
    _connection->failure = D_NET_OK;

    return;
}

/*
d_ftp_internal_client_reset
  File-local: closes both connections and returns the session's state to a
new client's. The options and the context stay, and so do the last reply
and the diagnostics, for the caller to inspect.
*/
D_STATIC
void d_ftp_internal_client_reset(
    struct dj_ftp_client* _client,
    bool                  _orderly
)
{
    d_ftp_internal_client_drop(_client,
                               &_client->data,
                               false);
    d_ftp_internal_client_drop(_client,
                               &_client->control,
                               _orderly);
    d_ftp_reply_parser_reset(&_client->parser);

    _client->input_start      = 0u;
    _client->input_end        = 0u;
    _client->features         = 0u;
    _client->protection       = D_FTP_PROTECTION_CLEAR;
    _client->connected        = false;
    _client->logged_in        = false;
    _client->type_sent        = false;
    _client->extended_refused = false;

    return;
}

/*
d_ftp_internal_client_reply
  A server may send several replies at once, so the bytes the parser has
not taken stay in `input` for the next call. The parser only stops short at
the end of a reply, so when it wants more, `input` is spent.
*/
enum dj_ftp_error
d_ftp_internal_client_reply(
    struct dj_ftp_client* _client
)
{
    // parse what is buffered, reading more whenever it runs out
    for (;;)
    {
        size_t                  used   = 0u;
        const enum dj_ftp_status parsed =
            d_ftp_reply_parser_feed(&_client->parser,
                                    _client->input + _client->input_start,
                                    _client->input_end - _client->input_start,
                                    &used,
                                    &_client->reply);

        _client->input_start += used;

        // a reply is complete, or the server's bytes are not replies
        if (parsed != D_FTP_STATUS_PENDING)
        {
            return (parsed == D_FTP_STATUS_COMPLETE) ? D_FTP_OK
                                                     : D_FTP_ERROR_MALFORMED;
        }

        size_t               received = 0u;
        const enum dj_ftp_io io       =
            d_ftp_internal_client_read(_client,
                                       &_client->control,
                                       _client->input,
                                       sizeof(_client->input),
                                       &received);

        // the control connection failed
        if (io != D_FTP_IO_OK)
        {
            return d_ftp_internal_client_failure(
                       _client,
                       &_client->control,
                       io,
                       D_FTP_ERROR_CLOSED);
        }

        _client->input_start = 0u;
        _client->input_end   = received;
    }
}

/*
d_ftp_internal_client_send
  The formatter refuses CR and LF in arguments, so no argument can smuggle
in a second command.
*/
enum dj_ftp_error
d_ftp_internal_client_send(
    struct dj_ftp_client* _client,
    enum dj_ftp_command   _command,
    const char*          _argument
)
{
    char                line[D_FTP_CLIENT_LINE_SIZE] = { 0 };
    struct dj_ftp_buffer buffer = { NULL, 0u, 0u };

    d_ftp_buffer_init(&buffer,
                      line,
                      sizeof(line));

    // a line too long to send is as unsendable as a malformed one
    if (d_ftp_command_format(_command,
                             _argument,
                             &buffer) != D_FTP_OK)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const enum dj_ftp_io io =
        d_ftp_internal_client_write(_client,
                                    &_client->control,
                                    buffer.data,
                                    buffer.length);

    // the control connection failed
    if (io != D_FTP_IO_OK)
    {
        return d_ftp_internal_client_failure(_client,
                                             &_client->control,
                                             io,
                                             D_FTP_ERROR_CLOSED);
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_exchange
  Only failing to send or to read is an error here; the final reply is left
for the caller to judge.
*/
enum dj_ftp_error
d_ftp_internal_client_exchange(
    struct dj_ftp_client* _client,
    enum dj_ftp_command   _command,
    const char*          _argument
)
{
    enum dj_ftp_error error = d_ftp_internal_client_send(_client,
                                                        _command,
                                                        _argument);

    // read past preliminary replies to the final one
    while (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_reply(_client);

        // a final reply ends the exchange
        if ( (error == D_FTP_OK) &&
             (d_ftp_reply_class_of(_client->reply.code) !=
              D_FTP_REPLY_CLASS_PRELIMINARY) )
        {
            break;
        }
    }

    return error;
}

/*
d_ftp_internal_client_positive
  2yz only.
*/
bool d_ftp_internal_client_positive(
    const struct dj_ftp_client* _client
)
{
    return (d_ftp_reply_class_of(_client->reply.code) ==
            D_FTP_REPLY_CLASS_COMPLETION);
}

/*
d_ftp_internal_client_refusal
  A positive reply that is the wrong one is unexpected, not success.
*/
enum dj_ftp_error
d_ftp_internal_client_refusal(
    const struct dj_ftp_client* _client
)
{
    const enum dj_ftp_error error =
        d_ftp_error_from_reply(_client->reply.code);

    return (error != D_FTP_OK) ? error : D_FTP_ERROR_UNEXPECTED_REPLY;
}

/*
d_ftp_internal_client_honoured
  File-local: reports whether the client has a layer that checks at least
what the options require.
*/
D_STATIC
bool d_ftp_internal_client_honoured(
    const struct dj_ftp_client* _client
)
{
    return ( (_client->security) &&
             (_client->security->honours(_client->security_layer,
                                         &_client->options)) );
}

/*
d_ftp_client_init
  Zeroes the whole client first, so every private field starts defined, then
sets what zero does not mean: closed connections, a parser over the reply
storage, and clear data protection. A client starts plain.
*/
void d_ftp_client_init(
    struct dj_ftp_client*        _client,
    const struct dj_ftp_options* _options
)
{
    // parameter validation
    if (!_client)
    {
        return;
    }

    memset(_client,
           0,
           sizeof(*_client));

    // the caller's options, or the defaults
    if (_options)
    {
        _client->options = *_options;
    }
    else
    {
        d_ftp_options_init(&_client->options);
    }

    _client->security        = NULL;
    _client->security_layer  = NULL;
    _client->protection      = D_FTP_PROTECTION_CLEAR;
    _client->security_status = 0;
    _client->net_error       = D_NET_OK;

    d_tcp_connection_init(&_client->control.tcp);
    d_tcp_connection_init(&_client->data.tcp);
    d_ftp_reply_parser_init(&_client->parser,
                            _client->reply_text,
                            sizeof(_client->reply_text));

    return;
}

/*
d_ftp_client_set_security
  Only a closed client takes a layer: one in force on a connection cannot be
swapped under it.
*/
void d_ftp_client_set_security(
    struct dj_ftp_client*             _client,
    const struct dj_ftp_security_ops* _ops,
    void*                             _layer
)
{
    // parameter validation
    if ( (!_client) ||
         (_client->connected) )
    {
        return;
    }

    _client->security       = _ops;
    _client->security_layer = (_ops) ? _layer : NULL;

    return;
}

/*
d_ftp_internal_client_greeting
  File-local: reads the server's greeting. 120 promises a 220 later, so it is
waited out; anything but 220 ends the session.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_greeting(
    struct dj_ftp_client* _client
)
{
    enum dj_ftp_error error = D_FTP_OK;

    // "ready in a few minutes" comes before the real greeting
    do
    {
        error = d_ftp_internal_client_reply(_client);
    } while ( (error == D_FTP_OK) &&
              (_client->reply.code == D_FTP_REPLY_SERVICE_READY_IN) );

    // a failure, or a welcome
    if ( (error != D_FTP_OK) ||
         (_client->reply.code == D_FTP_REPLY_SERVICE_READY) )
    {
        return error;
    }

    return d_ftp_internal_client_refusal(_client);
}

/*
d_ftp_internal_client_upgrade
  File-local: upgrades the control connection with AUTH and the layer's
mechanism. Bytes received after the 234 but before the handshake are
plaintext the server had no business sending: the shape of a STARTTLS
command injection, where a forged reply rides in the clear behind the real
one and a careless client reads it as protected. Such bytes already buffered
refuse the upgrade here; ones still in flight reach the handshake first,
where TLS's sniffing rejects them.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_upgrade(
    struct dj_ftp_client* _client
)
{
    const enum dj_ftp_error error =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_AUTH,
                                       _client->security->mechanism);

    // the exchange itself failed
    if (error != D_FTP_OK)
    {
        return error;
    }

    // refused: plaintext goes on only where the options allow it
    if (_client->reply.code != D_FTP_REPLY_SECURITY_ACCEPTED)
    {
        return (_client->options.require_security) ? D_FTP_ERROR_SECURITY
                                                   : D_FTP_OK;
    }

    // plaintext after the 234 would pass for the start of the handshake
    if (_client->input_start != _client->input_end)
    {
        return D_FTP_ERROR_SECURITY;
    }

    return d_ftp_internal_client_secure(_client,
                                        &_client->control);
}

/*
d_ftp_internal_client_features
  File-local: reads FEAT into `features`. A server without FEAT has no
features to report, which is no failure; only losing the connection is.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_features(
    struct dj_ftp_client* _client
)
{
    struct dj_ftp_features  features = { 0 };
    const enum dj_ftp_error error    =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_FEAT,
                                       NULL);

    // a failure, or no list to read
    if ( (error != D_FTP_OK) ||
         (_client->reply.code != D_FTP_REPLY_SYSTEM_STATUS) )
    {
        return error;
    }

    // a list that cannot be read offers nothing
    if (d_ftp_features_parse(_client->reply.text.data,
                             _client->reply.text.length,
                             &features) == D_FTP_OK)
    {
        _client->features = features.flags;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_open
  File-local: everything a connection needs before the login: the layer
before the first byte for implicit security, the greeting, AUTH after it
for explicit security, and the features, read once the connection is as
secure as it will be, so no one on the path can edit the list.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_open(
    struct dj_ftp_client* _client
)
{
    const enum dj_ftp_security security = _client->options.security;
    enum dj_ftp_error          error    = D_FTP_OK;

    // implicit security: the layer before the first byte
    if (security == D_FTP_SECURITY_IMPLICIT)
    {
        error = d_ftp_internal_client_secure(_client,
                                             &_client->control);
    }

    // the greeting, read secured when implicit
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_greeting(_client);
    }

    // explicit security: AUTH after the greeting
    if ( (error == D_FTP_OK) &&
         (security == D_FTP_SECURITY_EXPLICIT) )
    {
        error = d_ftp_internal_client_upgrade(_client);
    }

    // the features, last
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_features(_client);
    }

    return error;
}

/*
d_ftp_client_connect
  Checks the layer against the options before any byte is sent, so a
misconfigured one fails here instead of verifying less than asked.
*/
enum dj_ftp_error
d_ftp_client_connect(
    struct dj_ftp_client* _client,
    const char*          _host,
    uint16_t             _port
)
{
    // parameter validation
    if ( (!_client)            ||
         (!_host)              ||
         (_host[0] == '\0')    ||
         (_client->connected) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const enum dj_ftp_security security = _client->options.security;
    const size_t              length   = strlen(_host);

    // a host that fits, and a layer that checks what the options require
    if ( (length >= sizeof(_client->host)) ||
         ( (security != D_FTP_SECURITY_NONE) &&
           (!d_ftp_internal_client_honoured(_client)) ) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const uint16_t port = (uint16_t)(
        (_port != 0u)                           ? _port
        : (security == D_FTP_SECURITY_IMPLICIT) ? D_FTP_PORT_IMPLICIT_TLS
                                                : D_FTP_PORT_CONTROL);

    memcpy(_client->host,
           _host,
           length + 1u);

    enum dj_ftp_error error = d_ftp_internal_client_dial(_client,
                                                        &_client->control,
                                                        _host,
                                                        port,
                                                        D_FTP_ERROR_CONNECT);

    // connected: the rest of the opening, which may fail in turn
    if (error == D_FTP_OK)
    {
        _client->connected = true;
        error              = d_ftp_internal_client_open(_client);
    }

    // a failure anywhere closes everything
    if (error != D_FTP_OK)
    {
        d_ftp_internal_client_reset(_client,
                                    false);
    }

    return error;
}

/*
d_ftp_internal_client_credentials
  File-local: USER, then PASS on 331 and ACCT on 332. A server may accept
the user alone (230), and RFC 2228 adds 232 for a login the security
exchange already authorized.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_credentials(
    struct dj_ftp_client* _client
)
{
    const struct dj_ftp_options* const options  = &_client->options;
    const char* const                 user     =
        (options->user) ? options->user : "anonymous";
    const char* const                 password =
        (options->password) ? options->password : "anonymous@";
    enum dj_ftp_error                  error    =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_USER,
                                       user);

    // 331: the password
    if ( (error == D_FTP_OK) &&
         (_client->reply.code == D_FTP_REPLY_NEED_PASSWORD) )
    {
        error = d_ftp_internal_client_exchange(_client,
                                               D_FTP_COMMAND_PASS,
                                               password);
    }

    // 332: an account, which the options may lack
    if ( (error == D_FTP_OK) &&
         (_client->reply.code == D_FTP_REPLY_NEED_ACCOUNT) )
    {
        error = (options->account)
                ? d_ftp_internal_client_exchange(_client,
                                                 D_FTP_COMMAND_ACCT,
                                                 options->account)
                : D_FTP_ERROR_ACCOUNT_REQUIRED;
    }

    // a failure, or in: 230, 232, or 202 (already logged in)
    if ( (error != D_FTP_OK)                                     ||
         (_client->reply.code == D_FTP_REPLY_LOGGED_IN)          ||
         (_client->reply.code == D_FTP_REPLY_SECURITY_LOGGED_IN) ||
         (_client->reply.code == D_FTP_REPLY_SUPERFLUOUS) )
    {
        return error;
    }

    error = d_ftp_error_from_reply(_client->reply.code);

    return (error != D_FTP_OK) ? error : D_FTP_ERROR_LOGIN_DENIED;
}

/*
d_ftp_internal_client_protect
  File-local: PBSZ 0, then PROT. TLS, the one mechanism a layer gives,
protects all or nothing, so only C and P mean anything (RFC 4217 section 9). A refusal keeps the data clear, which
only a caller who did not require security accepts.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_protect(
    struct dj_ftp_client* _client
)
{
    const enum dj_ftp_protection level     = _client->options.data_protection;
    const bool                  required  = _client->options.require_security;
    const char                  letter[2] = { (char)level, '\0' };

    // no integrity-only or confidentiality-only level exists in TLS
    if ( (level != D_FTP_PROTECTION_CLEAR) &&
         (level != D_FTP_PROTECTION_PRIVATE) )
    {
        return D_FTP_ERROR_UNSUPPORTED;
    }

    enum dj_ftp_error error = d_ftp_internal_client_exchange(_client,
                                                            D_FTP_COMMAND_PBSZ,
                                                            "0");

    // PBSZ 0 is RFC 4217's precondition for PROT
    if ( (error == D_FTP_OK) &&
         (!d_ftp_internal_client_positive(_client)) )
    {
        return (required) ? D_FTP_ERROR_SECURITY : D_FTP_OK;
    }

    // then the level itself
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_exchange(_client,
                                               D_FTP_COMMAND_PROT,
                                               letter);
    }

    // a failure, or agreement
    if ( (error != D_FTP_OK) ||
         (d_ftp_internal_client_positive(_client)) )
    {
        _client->protection = (error == D_FTP_OK) ? level
                                                  : _client->protection;

        return error;
    }

    return ( (required) &&
             (level == D_FTP_PROTECTION_PRIVATE) ) ? D_FTP_ERROR_SECURITY
                                                  : D_FTP_OK;
}

/*
d_ftp_internal_client_clear
  File-local: CCC. Once the server agrees, both ends close the layer: this
end's close goes first, then the server's must arrive before anything else,
and the transport carries plaintext from there on.
*/
D_STATIC enum dj_ftp_error
d_ftp_internal_client_clear(
    struct dj_ftp_client* _client
)
{
    const enum dj_ftp_error error =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_CCC,
                                       NULL);

    // refused: the control connection stays encrypted, which loses nothing
    if ( (error != D_FTP_OK) ||
         (!d_ftp_internal_client_positive(_client)) )
    {
        return error;
    }

    char           scratch[1] = { 0 };
    size_t         received   = 0u;
    enum dj_ftp_io io         =
        d_ftp_internal_client_shutdown(_client,
                                       &_client->control);

    // the server's close is the only thing that may come next
    if (io == D_FTP_IO_OK)
    {
        io = d_ftp_internal_client_read(_client,
                                        &_client->control,
                                        scratch,
                                        sizeof(scratch),
                                        &received);

        // data where the close belonged
        if (io == D_FTP_IO_OK)
        {
            return D_FTP_ERROR_SECURITY;
        }
    }

    // anything but the server's close is a failure
    if (io != D_FTP_IO_CLOSED)
    {
        return d_ftp_internal_client_failure(_client,
                                             &_client->control,
                                             io,
                                             D_FTP_ERROR_CLOSED);
    }

    _client->security->drop(_client->security_layer,
                            &_client->control);
    _client->control.secured = false;

    return D_FTP_OK;
}

/*
d_ftp_client_login
  Data protection is agreed after the login, as most clients and servers
expect, and only on a secured control connection: PROT means nothing
without TLS.
*/
enum dj_ftp_error
d_ftp_client_login(
    struct dj_ftp_client* _client
)
{
    // parameter validation
    if (!_client)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // a connected session, not yet logged in
    if ( (!_client->connected) ||
         (_client->logged_in) )
    {
        return D_FTP_ERROR_BAD_SEQUENCE;
    }

    enum dj_ftp_error error = d_ftp_internal_client_credentials(_client);

    // data protection, on a secured control connection
    if ( (error == D_FTP_OK) &&
         (_client->control.secured) )
    {
        error = d_ftp_internal_client_protect(_client);
    }

    // UTF-8 path names where both ends want them; a refusal changes nothing
    if ( (error == D_FTP_OK)                                  &&
         (_client->options.use_utf8)                          &&
         ( (_client->features & D_FTP_FEATURE_UTF8) != 0u ) )
    {
        error = d_ftp_internal_client_exchange(_client,
                                               D_FTP_COMMAND_OPTS,
                                               "UTF8 ON");
    }

    // a clear control connection, where asked for
    if ( (error == D_FTP_OK)               &&
         (_client->options.clear_control)  &&
         (_client->control.secured) )
    {
        error = d_ftp_internal_client_clear(_client);
    }

    _client->logged_in = (error == D_FTP_OK);

    return error;
}

/*
d_ftp_internal_client_reserved
  File-local: reports whether a command changes state the client keeps -
the login, TLS, data protection, the representation type, the data
connection - or opens a data connection, and so must go through this
header's own functions.
*/
D_STATIC bool
d_ftp_internal_client_reserved(
    enum dj_ftp_command _command
)
{
    switch (_command)
    {
        case D_FTP_COMMAND_USER:
        case D_FTP_COMMAND_PASS:
        case D_FTP_COMMAND_ACCT:
        case D_FTP_COMMAND_REIN:
        case D_FTP_COMMAND_QUIT:
        case D_FTP_COMMAND_AUTH:
        case D_FTP_COMMAND_ADAT:
        case D_FTP_COMMAND_PBSZ:
        case D_FTP_COMMAND_PROT:
        case D_FTP_COMMAND_CCC:
        case D_FTP_COMMAND_MIC:
        case D_FTP_COMMAND_CONF:
        case D_FTP_COMMAND_ENC:
        case D_FTP_COMMAND_TYPE:
        case D_FTP_COMMAND_PORT:
        case D_FTP_COMMAND_EPRT:
        case D_FTP_COMMAND_LPRT:
        case D_FTP_COMMAND_PASV:
        case D_FTP_COMMAND_EPSV:
        case D_FTP_COMMAND_LPSV:
            return true;

        default:
            break;
    }

    return d_ftp_command_uses_data(_command);
}

/*
d_ftp_client_command
  The exchange reads past preliminary replies, so the reply judged is always
the final one.
*/
enum dj_ftp_error
d_ftp_client_command(
    struct dj_ftp_client* _client,
    enum dj_ftp_command   _command,
    const char*          _argument
)
{
    // parameter validation
    if ( (!_client) ||
         (d_ftp_internal_client_reserved(_command)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // a command needs a connection
    if (!_client->connected)
    {
        return D_FTP_ERROR_BAD_SEQUENCE;
    }

    const enum dj_ftp_error error =
        d_ftp_internal_client_exchange(_client,
                                       _command,
                                       _argument);

    return (error != D_FTP_OK) ? error
                               : d_ftp_error_from_reply(_client->reply.code);
}

/*
d_ftp_client_quit
  The connections close in order - close_notify on the control connection
before its transport goes - even when QUIT itself fails.
*/
enum dj_ftp_error
d_ftp_client_quit(
    struct dj_ftp_client* _client
)
{
    // parameter validation
    if (!_client)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // nothing to leave
    if (!_client->connected)
    {
        return D_FTP_ERROR_BAD_SEQUENCE;
    }

    enum dj_ftp_error error = d_ftp_internal_client_exchange(_client,
                                                            D_FTP_COMMAND_QUIT,
                                                            NULL);

    // 221, or the refusal's error
    if (error == D_FTP_OK)
    {
        error = d_ftp_error_from_reply(_client->reply.code);
    }

    d_ftp_internal_client_reset(_client,
                                true);

    return error;
}

/*
d_ftp_client_close
  Nothing is sent: an abandoned session need not be polite.
*/
void d_ftp_client_close(
    struct dj_ftp_client* _client
)
{
    // parameter validation
    if (!_client)
    {
        return;
    }

    d_ftp_internal_client_reset(_client,
                                false);

    return;
}
