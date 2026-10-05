/*******************************************************************************
* djinterp [net]                                                    ftp_client.c
*
* Implementation of the FTP and FTPS client declared in ftp_client.h: the
* session, and the helpers ftp_client_data.c shares.
*   Every connection is a TCP connection read and written through net.h. A
* TLS session reads and writes through the same two functions a plain
* connection does, which map net.h's results onto SSL statuses, so the
* protocol code above them never asks which is in force. Control replies are
* parsed from a read-ahead buffer; whatever the parser has not taken stays
* there for the next reply, which is also how an AUTH TLS upgrade can tell
* that nothing was left over.
*
*
* path:      /src/djinterp/net/ftp/ftp_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_client.h"  // corresponding header
// std
#include <limits.h>   // LONG_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint16_t, uint32_t
#include <string.h>   // memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"    // d_pack_text
#include "../../../../inc/djinterp/net/ftp/ftp_command.h"   // commands
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"    // d_ftp_buffer
#include "../../../../inc/djinterp/net/ftp/ftp_feature.h"   // d_ftp_features
#include "../../../../inc/djinterp/net/ftp/ftp_options.h"   // d_ftp_options
#include "../../../../inc/djinterp/net/ftp/ftp_reply.h"     // replies
#include "../../../../inc/djinterp/net/ftp/ftp_security.h"  // PROT levels
#include "../../../../inc/djinterp/net/net.h"               // d_net_endpoint
#include "../../../../inc/djinterp/net/ssl/ssl.h"           // d_ssl_session
#include "../../../../inc/djinterp/net/tcp/tcp.h"           // d_tcp_connect
#include "./ftp_client_internal.h"                          // shared helpers


//==============================================================================
// 3.  CLIENT
//==============================================================================

/*
d_ftp_internal_client_transport_read
  File-local: reads a connection's transport, as a TLS session's transport
or directly. net.h's end of stream, a count of 0 without an error, reads as
CONNECTION_CLOSED, which the kernel turns into UNEXPECTED_EOF when it
precedes close_notify; any error is kept in the connection's `failure`.
*/
D_STATIC enum d_ssl_status
d_ftp_internal_client_transport_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_read
)
{
    struct d_ftp_connection* const connection = _context;
    const struct d_net_io_result   result     =
        d_net_connection_read(&connection->tcp.base,
                              _buffer,
                              _capacity);

    *_out_read          = result.count;
    connection->failure = result.error;

    // a failed read
    if (result.error != D_NET_ERROR_NONE)
    {
        return D_SSL_STATUS_TRANSPORT_ERROR;
    }

    return (result.count == 0u) ? D_SSL_STATUS_CONNECTION_CLOSED
                                : D_SSL_STATUS_OK;
}

/*
d_ftp_internal_client_transport_write
  File-local: writes to a connection's transport. A peer that has gone reads
as CONNECTION_CLOSED, which the kernel defines as "no longer reads".
*/
D_STATIC enum d_ssl_status
d_ftp_internal_client_transport_write(
    void*       _context,
    const void* _data,
    size_t      _size,
    size_t*     _out_written
)
{
    struct d_ftp_connection* const connection = _context;
    const struct d_net_io_result   result     =
        d_net_connection_write(&connection->tcp.base,
                               _data,
                               _size);

    *_out_written       = result.count;
    connection->failure = result.error;

    // written, at least in part
    if (result.error == D_NET_ERROR_NONE)
    {
        return (result.count > 0u) ? D_SSL_STATUS_OK
                                   : D_SSL_STATUS_TRANSPORT_ERROR;
    }

    return ( (result.error == D_NET_ERROR_CLOSED)             ||
             (result.error == D_NET_ERROR_CONNECTION_RESET)   ||
             (result.error == D_NET_ERROR_CONNECTION_ABORTED) )
           ? D_SSL_STATUS_CONNECTION_CLOSED
           : D_SSL_STATUS_TRANSPORT_ERROR;
}

/*
d_ftp_internal_client_transport
  File-local: a connection's transport, as the SSL kernel takes one.
*/
D_STATIC struct d_ssl_transport
d_ftp_internal_client_transport(
    struct d_ftp_connection* _connection
)
{
    const struct d_ssl_transport transport =
    {
        d_ftp_internal_client_transport_read,
        d_ftp_internal_client_transport_write,
        _connection
    };

    return transport;
}

/*
d_ftp_internal_client_read
  Without TLS the transport is read directly, through the same mapping a TLS
session reads it by.
*/
enum d_ssl_status
d_ftp_internal_client_read(
    struct d_ftp_connection* _connection,
    void*                    _buffer,
    size_t                   _capacity,
    size_t*                  _out_read
)
{
    // TLS, when in force, owns the transport's bytes
    if (_connection->secured)
    {
        return d_ssl_session_read(&_connection->tls,
                                  _buffer,
                                  _capacity,
                                  _out_read);
    }

    return d_ftp_internal_client_transport_read(_connection,
                                                _buffer,
                                                _capacity,
                                                _out_read);
}

/*
d_ftp_internal_client_write
  Without TLS the kernel's write-all loops over the same transport write a
TLS session uses.
*/
enum d_ssl_status
d_ftp_internal_client_write(
    struct d_ftp_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    size_t written = 0u;

    // TLS, when in force, owns the transport's bytes
    if (_connection->secured)
    {
        return d_ssl_session_write_all(&_connection->tls,
                                       _data,
                                       _size,
                                       &written);
    }

    return d_ssl_transport_write_all(
               d_ftp_internal_client_transport(_connection),
               _data,
               _size,
               &written);
}

/*
d_ftp_internal_client_failure
  The transport's own error decides first, since a TLS session reports every
transport failure alike; the rest is TLS's.
*/
enum d_ftp_error
d_ftp_internal_client_failure(
    struct d_ftp_client*           _client,
    const struct d_ftp_connection* _connection,
    enum d_ssl_status              _status,
    enum d_ftp_error               _lost
)
{
    const bool framed = (_connection == &_client->control);

    _client->net_error = _connection->failure;

    // the transport's wait ran out
    if (_connection->failure == D_NET_ERROR_TIMED_OUT)
    {
        return D_FTP_ERROR_TIMEOUT;
    }

    // the stream ended or broke
    if ( (_status == D_SSL_STATUS_CONNECTION_CLOSED) ||
         (_status == D_SSL_STATUS_TRANSPORT_ERROR)   ||
         ( (framed) &&
           (_status == D_SSL_STATUS_UNEXPECTED_EOF) ) )
    {
        return _lost;
    }

    _client->tls_status = _status;

    return D_FTP_ERROR_TLS;
}

/*
d_ftp_internal_client_dial_error
  File-local: the FTP error for a failed connection attempt. A host the
resolver rejects is D_FTP_ERROR_RESOLVE; an unknown name, which TCP reports
as HOST_UNREACHABLE just as it does a host truly unreachable, stays `_lost`,
with the exact cause left in `net_error`.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_dial_error(
    enum d_net_error _error,
    enum d_ftp_error _lost
)
{
    switch (_error)
    {
        case D_NET_ERROR_NONE:
            return D_FTP_OK;

        case D_NET_ERROR_TIMED_OUT:
            return D_FTP_ERROR_TIMEOUT;

        case D_NET_ERROR_ADDRESS_INVALID:
            return D_FTP_ERROR_RESOLVE;

        case D_NET_ERROR_OUT_OF_MEMORY:
            return D_FTP_ERROR_OUT_OF_MEMORY;

        default:
            break;
    }

    return _lost;
}

/*
d_ftp_internal_client_dial
  The options' connect timeout is clamped into a long, whose width varies:
32 bits on LLP64 platforms.
*/
enum d_ftp_error
d_ftp_internal_client_dial(
    struct d_ftp_client*     _client,
    struct d_ftp_connection* _connection,
    const char*              _host,
    uint16_t                 _port,
    enum d_ftp_error         _lost
)
{
    const uint32_t           timeout = _client->options.connect_timeout_ms;
    const struct d_pack_text name    = { _host, strlen(_host) };
    struct d_net_endpoint    where;
    struct d_tcp_options     options;

    d_net_endpoint_init(&where);
    d_tcp_options_init(&options);

    options.connect_timeout_ms = (timeout > (uint32_t)LONG_MAX)
                                 ? LONG_MAX
                                 : (long)timeout;
    options.no_delay           = true;

    enum d_net_error error = d_net_endpoint_set(&where,
                                                name,
                                                _port,
                                                D_NET_PROTOCOL_TCP);

    // a host the endpoint cannot hold makes no connection
    if (error == D_NET_ERROR_NONE)
    {
        error = d_tcp_connect(&_connection->tcp,
                              &where,
                              &options);
    }

    _connection->failure = error;
    _client->net_error   = error;

    return d_ftp_internal_client_dial_error(error,
                                            _lost);
}

/*
d_ftp_internal_client_secure
  The session names the server's host, which the kernel checks the
certificate against and, for a name, sends as SNI. Every connection of a
session uses the same context and host: that is what lets an engine resume
the control session on data connections.
*/
enum d_ftp_error
d_ftp_internal_client_secure(
    struct d_ftp_client*     _client,
    struct d_ftp_connection* _connection
)
{
    const struct d_pack_text host   =
    {
        _client->host,
        strlen(_client->host)
    };
    enum d_ssl_status        status =
        d_ssl_session_init(&_connection->tls,
                           _client->tls,
                           d_ftp_internal_client_transport(_connection),
                           host);

    // the handshake runs only on a session that could be made
    if (status == D_SSL_STATUS_OK)
    {
        status = d_ssl_session_handshake(&_connection->tls);
    }

    // a failure leaves nothing behind
    if (status != D_SSL_STATUS_OK)
    {
        _client->tls_status = status;
        _client->net_error  = _connection->failure;
        d_ssl_session_destroy(&_connection->tls);

        return (_connection->failure == D_NET_ERROR_TIMED_OUT)
               ? D_FTP_ERROR_TIMEOUT
               : D_FTP_ERROR_TLS;
    }

    _connection->secured = true;

    return D_FTP_OK;
}

/*
d_ftp_internal_client_drop
  The session is released before the transport it runs over, and the
transport's error is forgotten with it.
*/
void
d_ftp_internal_client_drop(
    struct d_ftp_connection* _connection,
    bool                     _orderly
)
{
    // TLS goes before the transport it runs over
    if (_connection->secured)
    {
        // close_notify, when the close is orderly
        if (_orderly)
        {
            (void)d_ssl_session_shutdown(&_connection->tls);
        }

        d_ssl_session_destroy(&_connection->tls);
        _connection->secured = false;
    }

    d_tcp_connection_close(&_connection->tcp);
    _connection->failure = D_NET_ERROR_NONE;

    return;
}

/*
d_ftp_internal_client_reset
  File-local: closes both connections and returns the session's state to a
new client's. The options and the context stay, and so do the last reply
and the diagnostics, for the caller to inspect.
*/
D_STATIC void
d_ftp_internal_client_reset(
    struct d_ftp_client* _client,
    bool                 _orderly
)
{
    d_ftp_internal_client_drop(&_client->data,
                               false);
    d_ftp_internal_client_drop(&_client->control,
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
enum d_ftp_error
d_ftp_internal_client_reply(
    struct d_ftp_client* _client
)
{
    // parse what is buffered, reading more whenever it runs out
    for (;;)
    {
        size_t                  used   = 0u;
        const enum d_ftp_status parsed =
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

        size_t                  received = 0u;
        const enum d_ssl_status status   =
            d_ftp_internal_client_read(&_client->control,
                                       _client->input,
                                       sizeof(_client->input),
                                       &received);

        // the control connection failed
        if (status != D_SSL_STATUS_OK)
        {
            return d_ftp_internal_client_failure(
                       _client,
                       &_client->control,
                       status,
                       D_FTP_ERROR_CONNECTION_CLOSED);
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
enum d_ftp_error
d_ftp_internal_client_send(
    struct d_ftp_client* _client,
    enum d_ftp_command   _command,
    const char*          _argument
)
{
    char                line[D_FTP_CLIENT_LINE_SIZE] = { 0 };
    struct d_ftp_buffer buffer = { NULL, 0u, 0u };

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

    const enum d_ssl_status status =
        d_ftp_internal_client_write(&_client->control,
                                    buffer.data,
                                    buffer.length);

    // the control connection failed
    if (status != D_SSL_STATUS_OK)
    {
        return d_ftp_internal_client_failure(_client,
                                             &_client->control,
                                             status,
                                             D_FTP_ERROR_CONNECTION_CLOSED);
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_exchange
  Only failing to send or to read is an error here; the final reply is left
for the caller to judge.
*/
enum d_ftp_error
d_ftp_internal_client_exchange(
    struct d_ftp_client* _client,
    enum d_ftp_command   _command,
    const char*          _argument
)
{
    enum d_ftp_error error = d_ftp_internal_client_send(_client,
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
bool
d_ftp_internal_client_positive(
    const struct d_ftp_client* _client
)
{
    return (d_ftp_reply_class_of(_client->reply.code) ==
            D_FTP_REPLY_CLASS_COMPLETION);
}

/*
d_ftp_internal_client_refusal
  A positive reply that is the wrong one is unexpected, not success.
*/
enum d_ftp_error
d_ftp_internal_client_refusal(
    const struct d_ftp_client* _client
)
{
    const enum d_ftp_error error =
        d_ftp_error_from_reply(_client->reply.code);

    return (error != D_FTP_OK) ? error : D_FTP_ERROR_UNEXPECTED_REPLY;
}

/*
d_ftp_internal_client_plan_ok
  File-local: reports whether the TLS context checks at least what the
options require: a client's plan that verifies the chain, or pins, when
verify_peer is set, and the host name too when verify_host is.
*/
D_STATIC bool
d_ftp_internal_client_plan_ok(
    const struct d_ftp_client* _client
)
{
    const struct d_ftp_options* const options = &_client->options;

    // no context, or one never initialized
    if ( (!_client->tls) ||
         (!_client->tls->engine) )
    {
        return false;
    }

    const struct d_ssl_plan* const plan = &_client->tls->plan;

    return ( (plan->role == D_SSL_ROLE_CLIENT) &&
             ( (!options->verify_peer) ||
               (plan->verify_chain)    ||
               (plan->check_pins) )     &&
             ( (!options->verify_peer) ||
               (!options->verify_host) ||
               (plan->verify_hostname) ) );
}

/*
d_ftp_client_tls_config
  The kernel's client defaults already verify the chain and the host name
against the system trust store; only what the options turn off changes.
*/
void
d_ftp_client_tls_config(
    const struct d_ftp_options* _options,
    struct d_ssl_config*        _config
)
{
    // parameter validation
    if ( (!_options) ||
         (!_config) )
    {
        return;
    }

    d_ssl_config_init(_config,
                      D_SSL_ROLE_CLIENT);

    _config->verify          = (_options->verify_peer) ? D_SSL_VERIFY_REQUIRED
                                                       : D_SSL_VERIFY_NONE;
    _config->verify_hostname = ( (_options->verify_peer) &&
                                 (_options->verify_host) );

    return;
}

/*
d_ftp_client_init
  Zeroes the whole client first, so every private field starts defined, then
sets what zero does not mean: closed connections, a parser over the reply
storage, and clear data protection.
*/
void
d_ftp_client_init(
    struct d_ftp_client*        _client,
    const struct d_ftp_options* _options,
    const struct d_ssl_context* _tls
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

    _client->tls        = _tls;
    _client->protection = D_FTP_PROTECTION_CLEAR;
    _client->tls_status = D_SSL_STATUS_OK;
    _client->net_error  = D_NET_ERROR_NONE;

    d_tcp_connection_init(&_client->control.tcp);
    d_tcp_connection_init(&_client->data.tcp);
    d_ftp_reply_parser_init(&_client->parser,
                            _client->reply_text,
                            sizeof(_client->reply_text));

    return;
}

/*
d_ftp_internal_client_greeting
  File-local: reads the server's greeting. 120 promises a 220 later, so it is
waited out; anything but 220 ends the session.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_greeting(
    struct d_ftp_client* _client
)
{
    enum d_ftp_error error = D_FTP_OK;

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
  File-local: upgrades the control connection with AUTH TLS. Bytes received
after the 234 but before the handshake are plaintext the server had no
business sending: the shape of a STARTTLS command injection, where a forged
reply rides in the clear behind the real one and a careless client reads it
as protected. Such bytes already buffered refuse the upgrade here; ones
still in flight reach the handshake first, where the kernel's sniffing
rejects them as NOT_TLS.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_upgrade(
    struct d_ftp_client* _client
)
{
    const enum d_ftp_error error =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_AUTH,
                                       "TLS");

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
D_STATIC enum d_ftp_error
d_ftp_internal_client_features(
    struct d_ftp_client* _client
)
{
    struct d_ftp_features  features = { 0 };
    const enum d_ftp_error error    =
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
  File-local: everything a connection needs before the login: TLS before the
first byte for implicit FTPS, the greeting, AUTH TLS after it for explicit
FTPS, and the features, read once the connection is as secure as it will
be, so no one on the path can edit the list.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_open(
    struct d_ftp_client* _client
)
{
    const enum d_ftp_security security = _client->options.security;
    enum d_ftp_error          error    = D_FTP_OK;

    // implicit FTPS: TLS before the first byte
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

    // explicit FTPS: AUTH TLS after the greeting
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
  Checks the TLS context against the options before any byte is sent, so a
misconfigured context fails here instead of verifying less than asked.
*/
enum d_ftp_error
d_ftp_client_connect(
    struct d_ftp_client* _client,
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

    const enum d_ftp_security security = _client->options.security;
    const size_t              length   = strlen(_host);

    // a host that fits, and a context that checks what the options require
    if ( (length >= sizeof(_client->host)) ||
         ( (security != D_FTP_SECURITY_NONE) &&
           (!d_ftp_internal_client_plan_ok(_client)) ) )
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

    enum d_ftp_error error = d_ftp_internal_client_dial(_client,
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
D_STATIC enum d_ftp_error
d_ftp_internal_client_credentials(
    struct d_ftp_client* _client
)
{
    const struct d_ftp_options* const options  = &_client->options;
    const char* const                 user     =
        (options->user) ? options->user : "anonymous";
    const char* const                 password =
        (options->password) ? options->password : "anonymous@";
    enum d_ftp_error                  error    =
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
  File-local: PBSZ 0, then PROT. TLS protects all or nothing, so only C and
P mean anything (RFC 4217 section 9). A refusal keeps the data clear, which
only a caller who did not require security accepts.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_protect(
    struct d_ftp_client* _client
)
{
    const enum d_ftp_protection level     = _client->options.data_protection;
    const bool                  required  = _client->options.require_security;
    const char                  letter[2] = { (char)level, '\0' };

    // no integrity-only or confidentiality-only level exists in TLS
    if ( (level != D_FTP_PROTECTION_CLEAR) &&
         (level != D_FTP_PROTECTION_PRIVATE) )
    {
        return D_FTP_ERROR_UNSUPPORTED;
    }

    enum d_ftp_error error = d_ftp_internal_client_exchange(_client,
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
  File-local: CCC. Once the server agrees, both ends close TLS: this end's
close_notify goes first, then the server's must arrive before anything else,
and the transport carries plaintext from there on.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_clear(
    struct d_ftp_client* _client
)
{
    const enum d_ftp_error error =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_CCC,
                                       NULL);

    // refused: the control connection stays encrypted, which loses nothing
    if ( (error != D_FTP_OK) ||
         (!d_ftp_internal_client_positive(_client)) )
    {
        return error;
    }

    char              scratch[1] = { 0 };
    size_t            received   = 0u;
    enum d_ssl_status status     =
        d_ssl_session_shutdown(&_client->control.tls);

    // the server's close_notify is the only thing that may come next
    if (status == D_SSL_STATUS_OK)
    {
        status = d_ssl_session_read(&_client->control.tls,
                                    scratch,
                                    sizeof(scratch),
                                    &received);

        // data where close_notify belonged
        if (status == D_SSL_STATUS_OK)
        {
            return D_FTP_ERROR_SECURITY;
        }
    }

    // anything but the server's close_notify is a failure
    if (status != D_SSL_STATUS_CONNECTION_CLOSED)
    {
        return d_ftp_internal_client_failure(_client,
                                             &_client->control,
                                             status,
                                             D_FTP_ERROR_CONNECTION_CLOSED);
    }

    d_ssl_session_destroy(&_client->control.tls);
    _client->control.secured = false;

    return D_FTP_OK;
}

/*
d_ftp_client_login
  Data protection is agreed after the login, as most clients and servers
expect, and only on a secured control connection: PROT means nothing
without TLS.
*/
enum d_ftp_error
d_ftp_client_login(
    struct d_ftp_client* _client
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

    enum d_ftp_error error = d_ftp_internal_client_credentials(_client);

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
  File-local: reports whether a command changes state the client keeps --
the login, TLS, data protection, the representation type, the data
connection -- or opens a data connection, and so must go through this
header's own functions.
*/
D_STATIC bool
d_ftp_internal_client_reserved(
    enum d_ftp_command _command
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
enum d_ftp_error
d_ftp_client_command(
    struct d_ftp_client* _client,
    enum d_ftp_command   _command,
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

    const enum d_ftp_error error =
        d_ftp_internal_client_exchange(_client,
                                       _command,
                                       _argument);

    return (error != D_FTP_OK) ? error
                               : d_ftp_error_from_reply(_client->reply.code);
}

/*
d_ftp_client_quit
  The connections close in order -- close_notify on the control connection
before its transport goes -- even when QUIT itself fails.
*/
enum d_ftp_error
d_ftp_client_quit(
    struct d_ftp_client* _client
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

    enum d_ftp_error error = d_ftp_internal_client_exchange(_client,
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
void
d_ftp_client_close(
    struct d_ftp_client* _client
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
