/*******************************************************************************
* djinterp [net]                                               ftp_client_data.c
*
* Implementation of the FTP and FTPS client declared in ftp_client.h: data
* connections and transfers.
*   A passive data connection is dialed to the control connection's peer, as
* TCP recorded it, and an active one is accepted on the control connection's
* own local address, from that peer alone. While PROT P is in force, TLS
* runs over every data connection with the client as the TLS client. Data
* ends at close_notify over TLS, or at the end of a plain stream.
*
*
* path:      /src/djinterp/net/ftp/ftp_client_data.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_client.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint16_t
#include <string.h>   // memchr, memcpy, strchr, strcmp, strcspn, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"    // d_pack_text
#include "../../../../inc/djinterp/net/ftp/ftp_command.h"   // commands
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"    // d_ftp_buffer
#include "../../../../inc/djinterp/net/ftp/ftp_endpoint.h"  // d_ftp_endpoint
#include "../../../../inc/djinterp/net/ftp/ftp_feature.h"   // MLST feature
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"   // listing formats
#include "../../../../inc/djinterp/net/ftp/ftp_options.h"   // data modes
#include "../../../../inc/djinterp/net/ftp/ftp_reply.h"     // replies
#include "../../../../inc/djinterp/net/ftp/ftp_security.h"  // PROT levels
#include "../../../../inc/djinterp/net/ftp/ftp_transfer.h"  // TYPE, ASCII
#include "../../../../inc/djinterp/net/net.h"               // d_net_endpoint
#include "../../../../inc/djinterp/net/ssl/ssl.h"           // d_ssl_session
#include "../../../../inc/djinterp/net/tcp/tcp.h"           // d_tcp_listener
#include "./ftp_client_internal.h"                          // shared helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_ftp_internal_transfer
//   struct: one transfer's request: its command and argument, the callback
// that takes or supplies the data, and whether ASCII conversion applies.
struct d_ftp_internal_transfer
{
    enum d_ftp_command command;   // RETR, STOR, LIST, or MLSD
    const char*        argument;  // the path, or NULL
    d_ftp_sink_fn      sink;      // downloads: takes the data
    d_ftp_source_fn    source;    // uploads: supplies the data
    void*              context;   // passed to the callback
    bool               convert;   // ASCII type: convert line ends
};

//==============================================================================
// 3.  CLIENT
//==============================================================================

/*
d_ftp_internal_client_type
  File-local: sends TYPE once per session, for the options' representation
type.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_type(
    struct d_ftp_client* _client
)
{
    // once per session is enough
    if (_client->type_sent)
    {
        return D_FTP_OK;
    }

    char                text[D_FTP_TYPE_ARGUMENT_SIZE] = { 0 };
    struct d_ftp_buffer argument = { NULL, 0u, 0u };

    d_ftp_buffer_init(&argument,
                      text,
                      sizeof(text));

    enum d_ftp_error error = d_ftp_type_format(&_client->options.type,
                                               &argument);

    // the argument, then the command, then its verdict
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_exchange(_client,
                                               D_FTP_COMMAND_TYPE,
                                               text);
    }

    if (error == D_FTP_OK)
    {
        error = d_ftp_error_from_reply(_client->reply.code);
    }

    _client->type_sent = (error == D_FTP_OK);

    return error;
}

/*
d_ftp_internal_client_epsv
  File-local: asks for an extended passive port. A refusal in automatic mode
is remembered, so later transfers go straight to PASV, and leaves
`_out_port` at 0.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_epsv(
    struct d_ftp_client* _client,
    uint16_t*            _out_port
)
{
    struct d_ftp_endpoint  offered = { .family = D_FTP_FAMILY_NONE };
    const enum d_ftp_error error   =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_EPSV,
                                       NULL);

    // the exchange itself failed
    if (error != D_FTP_OK)
    {
        return error;
    }

    // refused: PASV may stand in, in automatic mode only
    if (_client->reply.code != D_FTP_REPLY_EXTENDED_PASSIVE)
    {
        if (_client->options.data_mode != D_FTP_DATA_PASSIVE_AUTO)
        {
            return d_ftp_internal_client_refusal(_client);
        }

        _client->extended_refused = true;

        return D_FTP_OK;
    }

    // a 229 without a port is no answer
    if (d_ftp_parse_epsv(_client->reply.text.data,
                         _client->reply.text.length,
                         &offered) != D_FTP_OK)
    {
        return D_FTP_ERROR_UNEXPECTED_REPLY;
    }

    *_out_port = offered.port;

    return D_FTP_OK;
}

/*
d_ftp_internal_client_pasv
  File-local: asks for a passive port with PASV. The port always lands in
`_target`; the address does only when the options trust PASV addresses,
leaving `_target`'s family NONE otherwise.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_pasv(
    struct d_ftp_client*   _client,
    struct d_ftp_endpoint* _target
)
{
    struct d_ftp_endpoint  offered = { .family = D_FTP_FAMILY_NONE };
    const enum d_ftp_error error   =
        d_ftp_internal_client_exchange(_client,
                                       D_FTP_COMMAND_PASV,
                                       NULL);

    // the exchange failed, or PASV was refused
    if ( (error != D_FTP_OK) ||
         (_client->reply.code != D_FTP_REPLY_PASSIVE) )
    {
        return (error != D_FTP_OK) ? error
                                   : d_ftp_internal_client_refusal(_client);
    }

    // a 227 without an address and port is no answer
    if (d_ftp_parse_pasv(_client->reply.text.data,
                         _client->reply.text.length,
                         &offered) != D_FTP_OK)
    {
        return D_FTP_ERROR_UNEXPECTED_REPLY;
    }

    // the whole endpoint when trusted, otherwise its port alone
    if (!_client->options.ignore_pasv_address)
    {
        *_target = offered;
    }
    else
    {
        _target->port = offered.port;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_passive
  File-local: opens a passive data connection: EPSV, or PASV where EPSV is
refused or not wanted. The connection goes to the control connection's peer
-- EPSV names no address, and a PASV address counts only when the options
trust it, since servers often send private ones and a hostile server can
aim a client anywhere.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_passive(
    struct d_ftp_client* _client
)
{
    const enum d_ftp_data_mode mode   = _client->options.data_mode;
    struct d_ftp_endpoint      target = { .family = D_FTP_FAMILY_NONE };
    struct d_net_endpoint      peer;
    enum d_ftp_error           error  = D_FTP_OK;

    d_net_endpoint_init(&peer);

    // the server's own address, as the control connection recorded it
    if (!d_tcp_connection_remote_endpoint(&_client->control.tcp,
                                          &peer))
    {
        return D_FTP_ERROR_DATA_CONNECTION;
    }

    // EPSV where wanted, and not yet refused
    if ( (mode == D_FTP_DATA_EXTENDED_PASSIVE) ||
         ( (mode == D_FTP_DATA_PASSIVE_AUTO) &&
           (!_client->extended_refused) ) )
    {
        error = d_ftp_internal_client_epsv(_client,
                                           &target.port);
    }

    // PASV where EPSV was not used, or was refused
    if ( (error == D_FTP_OK) &&
         (target.port == 0u) )
    {
        error = d_ftp_internal_client_pasv(_client,
                                           &target);
    }

    // connect only once a port is known
    if (error != D_FTP_OK)
    {
        return error;
    }

    return d_ftp_internal_client_dial(
               _client,
               &_client->data,
               (target.family != D_FTP_FAMILY_NONE) ? target.address
                                                    : peer.host,
               target.port,
               D_FTP_ERROR_DATA_CONNECTION);
}

/*
d_ftp_internal_client_endpoint
  File-local: converts a transport endpoint into the FTP foundation's, for
EPRT and PORT: an IPv6 literal, holding a colon, or IPv4. An IPv4-mapped
IPv6 address (::ffff:a.b.c.d) reads as the IPv4 address it carries, which
PORT needs; a scope suffix (%eth0) is dropped, since EPRT cannot carry one.
*/
D_STATIC bool
d_ftp_internal_client_endpoint(
    const struct d_net_endpoint* _from,
    struct d_ftp_endpoint*       _out
)
{
    const char* host = _from->host;

    // a mapped address carries IPv4 after its prefix
    if ( (strncmp(host,
                  "::ffff:",
                  7u) == 0) &&
         (strchr(host + 7,
                 '.')) )
    {
        host += 7;
    }

    const size_t length = strcspn(host,
                                  "%");

    // an address that fits the record, NUL included
    if ( (length == 0u) ||
         (length >= sizeof(_out->address)) )
    {
        return false;
    }

    memcpy(_out->address,
           host,
           length);

    _out->address[length] = '\0';
    _out->family          = (memchr(host,
                                    ':',
                                    length)) ? D_FTP_FAMILY_IPV6
                                             : D_FTP_FAMILY_IPV4;
    _out->port            = _from->port;

    return true;
}

/*
d_ftp_internal_client_listen
  File-local: listens on an ephemeral port of the control connection's own
local address -- the one the server already reaches -- and reports where,
as EPRT and PORT will name it.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_listen(
    struct d_ftp_client*   _client,
    struct d_tcp_listener* _listener,
    struct d_ftp_endpoint* _out_local
)
{
    struct d_net_endpoint local;
    struct d_net_endpoint bound;
    struct d_net_endpoint where;

    d_net_endpoint_init(&local);
    d_net_endpoint_init(&bound);
    d_net_endpoint_init(&where);

    // this end's address, as the control connection recorded it
    if (!d_tcp_connection_local_endpoint(&_client->control.tcp,
                                         &local))
    {
        return D_FTP_ERROR_DATA_CONNECTION;
    }

    const struct d_pack_text host  = { local.host, strlen(local.host) };
    enum d_net_error         error = d_net_endpoint_set(&where,
                                                        host,
                                                        0u,
                                                        D_NET_PROTOCOL_TCP);

    // a listener on it, at a port the system picks
    if (error == D_NET_ERROR_NONE)
    {
        error = d_tcp_listen(_listener,
                             &where,
                             NULL);
    }

    _client->net_error = error;

    // the port it picked, as the commands name it
    if ( (error != D_NET_ERROR_NONE)                                ||
         (!d_tcp_listener_local_endpoint(_listener,
                                         &bound))                   ||
         (!d_ftp_internal_client_endpoint(&bound,
                                          _out_local)) )
    {
        return D_FTP_ERROR_DATA_CONNECTION;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_announce
  File-local: tells the server where to connect, with EPRT or PORT, leaving
the reply for the caller to judge.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_announce(
    struct d_ftp_client*         _client,
    enum d_ftp_command           _command,
    const struct d_ftp_endpoint* _local
)
{
    char                text[D_FTP_EPRT_ARGUMENT_SIZE] = { 0 };
    struct d_ftp_buffer argument = { NULL, 0u, 0u };

    d_ftp_buffer_init(&argument,
                      text,
                      sizeof(text));

    const enum d_ftp_error format =
        (_command == D_FTP_COMMAND_EPRT)
        ? d_ftp_format_eprt(_local,
                            &argument)
        : d_ftp_format_port(_local,
                            &argument);

    // an address the command cannot carry
    if (format != D_FTP_OK)
    {
        return D_FTP_ERROR_DATA_CONNECTION;
    }

    return d_ftp_internal_client_exchange(_client,
                                          _command,
                                          text);
}

/*
d_ftp_internal_client_active
  File-local: listens, then names the port with EPRT, or with PORT where
EPRT is refused or not wanted. PORT cannot carry IPv6, so an IPv6
connection uses EPRT alone.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_active(
    struct d_ftp_client*   _client,
    struct d_tcp_listener* _listener
)
{
    const enum d_ftp_data_mode mode  = _client->options.data_mode;
    struct d_ftp_endpoint      local = { .family = D_FTP_FAMILY_NONE };
    enum d_ftp_error           error = d_ftp_internal_client_listen(_client,
                                                                    _listener,
                                                                    &local);

    // EPRT where wanted, where not yet refused, or where PORT cannot serve
    if ( (error == D_FTP_OK)                           &&
         ( (local.family == D_FTP_FAMILY_IPV6)    ||
           (mode == D_FTP_DATA_EXTENDED_ACTIVE)   ||
           ( (mode == D_FTP_DATA_ACTIVE_AUTO) &&
             (!_client->extended_refused) ) ) )
    {
        error = d_ftp_internal_client_announce(_client,
                                               D_FTP_COMMAND_EPRT,
                                               &local);

        // a failure, or agreement
        if ( (error != D_FTP_OK) ||
             (d_ftp_internal_client_positive(_client)) )
        {
            return error;
        }

        // PORT stands in only in automatic mode, and only for IPv4
        if ( (mode != D_FTP_DATA_ACTIVE_AUTO) ||
             (local.family != D_FTP_FAMILY_IPV4) )
        {
            return d_ftp_internal_client_refusal(_client);
        }

        _client->extended_refused = true;
    }

    // PORT, when EPRT was not the answer
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_announce(_client,
                                               D_FTP_COMMAND_PORT,
                                               &local);
    }

    return ( (error != D_FTP_OK) ||
             (d_ftp_internal_client_positive(_client)) )
           ? error
           : d_ftp_internal_client_refusal(_client);
}

/*
d_ftp_internal_client_accept
  File-local: accepts the server's active-mode connection. Anyone can
connect to a listening port, so a connection from any address but the
control connection's peer is refused: without TLS it could be another host
stealing the transfer or feeding it. TCP records both peers the same way,
as numeric addresses, so the texts compare exactly.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_accept(
    struct d_ftp_client*   _client,
    struct d_tcp_listener* _listener
)
{
    struct d_net_endpoint expected;
    struct d_net_endpoint actual;
    enum d_net_error      error = d_tcp_accept(_listener,
                                               &_client->data.tcp);

    d_net_endpoint_init(&expected);
    d_net_endpoint_init(&actual);

    // the connection must come from the server
    if ( (error == D_NET_ERROR_NONE)                                  &&
         ( (!d_tcp_connection_remote_endpoint(&_client->control.tcp,
                                              &expected))           ||
           (!d_tcp_connection_remote_endpoint(&_client->data.tcp,
                                              &actual))             ||
           (strcmp(expected.host,
                   actual.host) != 0) ) )
    {
        error = D_NET_ERROR_ACCESS_DENIED;
    }

    _client->net_error = error;

    // anything else closes it
    if (error != D_NET_ERROR_NONE)
    {
        d_tcp_connection_close(&_client->data.tcp);

        return D_FTP_ERROR_DATA_CONNECTION;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_request
  File-local: sends a transfer command and reads its first reply, which
must be preliminary -- 125 or 150, the data connection opening. A final
reply here refuses the transfer.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_request(
    struct d_ftp_client*                  _client,
    const struct d_ftp_internal_transfer* _transfer
)
{
    enum d_ftp_error error = d_ftp_internal_client_send(_client,
                                                        _transfer->command,
                                                        _transfer->argument);

    // the first reply
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_reply(_client);
    }

    // which must be preliminary
    if ( (error == D_FTP_OK) &&
         (d_ftp_reply_class_of(_client->reply.code) !=
          D_FTP_REPLY_CLASS_PRELIMINARY) )
    {
        error = d_ftp_internal_client_refusal(_client);
    }

    return error;
}

/*
d_ftp_internal_client_begin
  File-local: starts a transfer: TYPE, the data connection or the port it
will come to, the command and its preliminary reply, then TLS over the data
connection while PROT P is in force. RFC 4217 makes the FTP client the TLS
client of every data connection, active ones included. `*_out_started`
reports whether the server answered with that preliminary reply, after
which a final reply is owed whatever else fails.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_begin(
    struct d_ftp_client*                  _client,
    const struct d_ftp_internal_transfer* _transfer,
    bool*                                 _out_started
)
{
    const enum d_ftp_data_mode mode   = _client->options.data_mode;
    const bool                 active =
        ( (mode == D_FTP_DATA_ACTIVE_AUTO) ||
          (mode == D_FTP_DATA_ACTIVE)      ||
          (mode == D_FTP_DATA_EXTENDED_ACTIVE) );
    struct d_tcp_listener      listener;

    d_tcp_listener_init(&listener);

    enum d_ftp_error error = d_ftp_internal_client_type(_client);

    // the data connection, or the port it will come to
    if (error == D_FTP_OK)
    {
        error = (active) ? d_ftp_internal_client_active(_client,
                                                        &listener)
                         : d_ftp_internal_client_passive(_client);
    }

    // the command, answered first with a preliminary reply
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_request(_client,
                                              _transfer);
    }

    *_out_started = (error == D_FTP_OK);

    // an active connection arrives now
    if ( (error == D_FTP_OK) &&
         (active) )
    {
        error = d_ftp_internal_client_accept(_client,
                                             &listener);
    }

    d_tcp_listener_close(&listener);

    // TLS on the data connection while PROT P is in force
    if ( (error == D_FTP_OK) &&
         (_client->protection == D_FTP_PROTECTION_PRIVATE) )
    {
        error = d_ftp_internal_client_secure(_client,
                                             &_client->data);
    }

    // a transfer that cannot start leaves no data connection
    if (error != D_FTP_OK)
    {
        d_ftp_internal_client_drop(&_client->data,
                                   false);
    }

    return error;
}

/*
d_ftp_internal_client_settle
  File-local: reads a started transfer's final reply, even after its data
failed, so that the next command's reply is not mistaken for this one's.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_settle(
    struct d_ftp_client* _client
)
{
    enum d_ftp_error error = D_FTP_OK;

    // read past any further preliminary reply
    do
    {
        error = d_ftp_internal_client_reply(_client);
    } while ( (error == D_FTP_OK) &&
              (d_ftp_reply_class_of(_client->reply.code) ==
               D_FTP_REPLY_CLASS_PRELIMINARY) );

    return (error != D_FTP_OK) ? error
                               : d_ftp_error_from_reply(_client->reply.code);
}

/*
d_ftp_internal_client_deliver
  File-local: hands received data to the sink, converting CR LF to LF for an
ASCII type. Conversion never grows the data, but a full buffer can stop it
early, so it runs in rounds; a CR held back can leave a round with nothing.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_deliver(
    const struct d_ftp_internal_transfer* _transfer,
    struct d_ftp_ascii_state*             _ascii,
    const char*                           _data,
    size_t                                _size
)
{
    // binary data passes straight through
    if (!_ascii)
    {
        return _transfer->sink(_transfer->context,
                               _data,
                               _size);
    }

    char   text[D_FTP_CLIENT_CHUNK_SIZE] = { 0 };
    size_t offset = 0u;

    // convert, then deliver, until every byte is taken
    while (offset < _size)
    {
        struct d_ftp_buffer out  = { NULL, 0u, 0u };
        size_t              used = 0u;

        d_ftp_buffer_init(&out,
                          text,
                          sizeof(text));

        enum d_ftp_error error = d_ftp_ascii_from_network(_ascii,
                                                          _data + offset,
                                                          _size - offset,
                                                          &used,
                                                          &out);

        offset += used;

        // a round may have produced nothing to deliver
        if ( (error == D_FTP_OK) &&
             (out.length > 0u) )
        {
            error = _transfer->sink(_transfer->context,
                                    text,
                                    out.length);
        }

        if (error != D_FTP_OK)
        {
            return error;
        }
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_flush
  File-local: delivers the CR an ASCII transfer ended on, if it did.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_flush(
    const struct d_ftp_internal_transfer* _transfer,
    struct d_ftp_ascii_state*             _ascii
)
{
    char                text[4] = { 0 };
    struct d_ftp_buffer out     = { NULL, 0u, 0u };

    d_ftp_buffer_init(&out,
                      text,
                      sizeof(text));

    enum d_ftp_error error = d_ftp_ascii_finish(_ascii,
                                                &out);

    // a final CR is data too
    if ( (error == D_FTP_OK) &&
         (out.length > 0u) )
    {
        error = _transfer->sink(_transfer->context,
                                text,
                                out.length);
    }

    return error;
}

/*
d_ftp_internal_client_receive
  File-local: reads the data connection to its end. A plain stream ends when
the server closes it; a TLS stream only at close_notify -- one that simply
stops was cut short, and is a TLS failure rather than a complete file.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_receive(
    struct d_ftp_client*                  _client,
    const struct d_ftp_internal_transfer* _transfer
)
{
    char                            chunk[D_FTP_CLIENT_CHUNK_SIZE] = { 0 };
    struct d_ftp_ascii_state        ascii = { false };
    struct d_ftp_ascii_state* const state = (_transfer->convert) ? &ascii
                                                                 : NULL;

    // until the data ends
    for (;;)
    {
        size_t                  received = 0u;
        const enum d_ssl_status status   =
            d_ftp_internal_client_read(&_client->data,
                                       chunk,
                                       sizeof(chunk),
                                       &received);

        // close_notify, or a plain stream's end: the data is complete
        if (status == D_SSL_STATUS_CONNECTION_CLOSED)
        {
            break;
        }

        // anything else failed, TLS ending without close_notify included
        if (status != D_SSL_STATUS_OK)
        {
            return d_ftp_internal_client_failure(
                       _client,
                       &_client->data,
                       status,
                       D_FTP_ERROR_DATA_CONNECTION);
        }

        const enum d_ftp_error error =
            d_ftp_internal_client_deliver(_transfer,
                                          state,
                                          chunk,
                                          received);

        // the sink abandons the transfer
        if (error != D_FTP_OK)
        {
            return error;
        }
    }

    return (state) ? d_ftp_internal_client_flush(_transfer,
                                                 state)
                   : D_FTP_OK;
}

/*
d_ftp_internal_client_write_data
  File-local: writes bytes to the data connection.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_write_data(
    struct d_ftp_client* _client,
    const char*          _data,
    size_t               _size
)
{
    // a round that produced nothing sends nothing
    if (_size == 0u)
    {
        return D_FTP_OK;
    }

    const enum d_ssl_status status =
        d_ftp_internal_client_write(&_client->data,
                                    _data,
                                    _size);

    return (status == D_SSL_STATUS_OK)
           ? D_FTP_OK
           : d_ftp_internal_client_failure(_client,
                                           &_client->data,
                                           status,
                                           D_FTP_ERROR_DATA_CONNECTION);
}

/*
d_ftp_internal_client_transmit
  File-local: sends source data, converting LF to CR LF for an ASCII type.
Conversion can double the data, so it runs in rounds of a buffer's worth.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_transmit(
    struct d_ftp_client*      _client,
    struct d_ftp_ascii_state* _ascii,
    const char*               _data,
    size_t                    _size
)
{
    // binary data goes as it is
    if (!_ascii)
    {
        return d_ftp_internal_client_write_data(_client,
                                                _data,
                                                _size);
    }

    char   text[D_FTP_CLIENT_CHUNK_SIZE] = { 0 };
    size_t offset = 0u;

    // convert, then send, until every byte is taken
    while (offset < _size)
    {
        struct d_ftp_buffer out  = { NULL, 0u, 0u };
        size_t              used = 0u;

        d_ftp_buffer_init(&out,
                          text,
                          sizeof(text));

        enum d_ftp_error error = d_ftp_ascii_to_network(_ascii,
                                                        _data + offset,
                                                        _size - offset,
                                                        &used,
                                                        &out);

        offset += used;

        // the converted round goes out before the next
        if (error == D_FTP_OK)
        {
            error = d_ftp_internal_client_write_data(_client,
                                                     text,
                                                     out.length);
        }

        if (error != D_FTP_OK)
        {
            return error;
        }
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_pump
  File-local: moves the source's data onto the data connection until the
source reports the end of the file.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_pump(
    struct d_ftp_client*                  _client,
    const struct d_ftp_internal_transfer* _transfer
)
{
    char                            chunk[D_FTP_CLIENT_CHUNK_SIZE] = { 0 };
    struct d_ftp_ascii_state        ascii = { false };
    struct d_ftp_ascii_state* const state = (_transfer->convert) ? &ascii
                                                                 : NULL;

    // until the source runs dry
    for (;;)
    {
        size_t           size  = 0u;
        enum d_ftp_error error = _transfer->source(_transfer->context,
                                                   chunk,
                                                   sizeof(chunk),
                                                   &size);

        // the source failed, or the file is done
        if ( (error != D_FTP_OK) ||
             (size == 0u) )
        {
            return error;
        }

        // a source that overfilled its buffer cannot be trusted
        if (size > sizeof(chunk))
        {
            return D_FTP_ERROR_INVALID_ARGUMENT;
        }

        error = d_ftp_internal_client_transmit(_client,
                                               state,
                                               chunk,
                                               size);

        if (error != D_FTP_OK)
        {
            return error;
        }
    }
}

/*
d_ftp_internal_client_download
  File-local: runs a transfer whose data flows to the client. After the
server's close_notify this end answers with its own; after a failure the
connection just closes, and a server still sending sees the transfer
abandoned.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_download(
    struct d_ftp_client*                  _client,
    const struct d_ftp_internal_transfer* _transfer
)
{
    bool             started = false;
    enum d_ftp_error error   = d_ftp_internal_client_begin(_client,
                                                           _transfer,
                                                           &started);

    // the data, to its end
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_receive(_client,
                                              _transfer);
    }

    d_ftp_internal_client_drop(&_client->data,
                               (error == D_FTP_OK));

    // once started, the final reply is owed whatever happened
    if (started)
    {
        const enum d_ftp_error final = d_ftp_internal_client_settle(_client);

        error = (error != D_FTP_OK) ? error : final;
    }

    return error;
}

/*
d_ftp_internal_client_close_upload
  File-local: ends an upload's TLS stream both ways: this end's close_notify,
then the server's in reply, before the connection closes. Servers such as
vsftpd answer close_notify with their own and count a connection gone
before theirs is written as a failed transfer. Once this end's close_notify
is on the wire the upload is complete, and the server's final reply is the
verdict, so however the wait ends -- close_notify, the end of the stream,
or a failure -- is no error; anything else the server sends is discarded,
up to one chunk.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_close_upload(
    struct d_ftp_client* _client
)
{
    char              scratch[256] = { 0 };
    enum d_ssl_status status       =
        d_ssl_session_shutdown(&_client->data.tls);

    // without close_notify the server cannot tell the upload is complete
    if (status != D_SSL_STATUS_OK)
    {
        return d_ftp_internal_client_failure(_client,
                                             &_client->data,
                                             status,
                                             D_FTP_ERROR_DATA_CONNECTION);
    }

    // then the server's close_notify, or the end of the stream
    for (size_t discarded = 0u;
         (status == D_SSL_STATUS_OK) &&
         (discarded <= D_FTP_CLIENT_CHUNK_SIZE); )
    {
        size_t received = 0u;

        status     = d_ssl_session_read(&_client->data.tls,
                                        scratch,
                                        sizeof(scratch),
                                        &received);
        discarded += received;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_client_upload
  File-local: runs a transfer whose data flows to the server. Over TLS the
data ends with close_notify, and only then does the connection close, so
the server can tell a complete upload from one cut short.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_client_upload(
    struct d_ftp_client*                  _client,
    const struct d_ftp_internal_transfer* _transfer
)
{
    bool             started = false;
    enum d_ftp_error error   = d_ftp_internal_client_begin(_client,
                                                           _transfer,
                                                           &started);

    // the data, to the source's end
    if (error == D_FTP_OK)
    {
        error = d_ftp_internal_client_pump(_client,
                                           _transfer);
    }

    // close_notify marks the end of a complete upload
    if ( (error == D_FTP_OK) &&
         (_client->data.secured) )
    {
        error = d_ftp_internal_client_close_upload(_client);
    }

    d_ftp_internal_client_drop(&_client->data,
                               false);

    // once started, the final reply is owed whatever happened
    if (started)
    {
        const enum d_ftp_error final = d_ftp_internal_client_settle(_client);

        error = (error != D_FTP_OK) ? error : final;
    }

    return error;
}

/*
d_ftp_client_retrieve
  RETR, converted when the representation type is ASCII.
*/
enum d_ftp_error
d_ftp_client_retrieve(
    struct d_ftp_client* _client,
    const char*          _path,
    d_ftp_sink_fn        _sink,
    void*                _context
)
{
    // parameter validation
    if ( (!_client)         ||
         (!_path)           ||
         (_path[0] == '\0') ||
         (!_sink) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // transfers need a session
    if (!_client->logged_in)
    {
        return D_FTP_ERROR_BAD_SEQUENCE;
    }

    const struct d_ftp_internal_transfer transfer =
    {
        D_FTP_COMMAND_RETR,
        _path,
        _sink,
        NULL,
        _context,
        (_client->options.type.data_type == D_FTP_TYPE_ASCII)
    };

    return d_ftp_internal_client_download(_client,
                                          &transfer);
}

/*
d_ftp_client_store
  STOR, converted when the representation type is ASCII.
*/
enum d_ftp_error
d_ftp_client_store(
    struct d_ftp_client* _client,
    const char*          _path,
    d_ftp_source_fn      _source,
    void*                _context
)
{
    // parameter validation
    if ( (!_client)         ||
         (!_path)           ||
         (_path[0] == '\0') ||
         (!_source) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // transfers need a session
    if (!_client->logged_in)
    {
        return D_FTP_ERROR_BAD_SEQUENCE;
    }

    const struct d_ftp_internal_transfer transfer =
    {
        D_FTP_COMMAND_STOR,
        _path,
        NULL,
        _source,
        _context,
        (_client->options.type.data_type == D_FTP_TYPE_ASCII)
    };

    return d_ftp_internal_client_upload(_client,
                                        &transfer);
}

/*
d_ftp_client_list
  MLSD when FEAT listed MLST or MLSD, since its facts parse exactly; LIST
otherwise. A listing passes through unconverted: the listing parser takes
CR LF as it comes.
*/
enum d_ftp_error
d_ftp_client_list(
    struct d_ftp_client*       _client,
    const char*                _path,
    d_ftp_sink_fn              _sink,
    void*                      _context,
    enum d_ftp_listing_format* _out_format
)
{
    // parameter validation
    if ( (!_client) ||
         (!_sink) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // transfers need a session
    if (!_client->logged_in)
    {
        return D_FTP_ERROR_BAD_SEQUENCE;
    }

    const bool                           machine  =
        ( (_client->features & D_FTP_FEATURE_MLST) != 0u );
    const struct d_ftp_internal_transfer transfer =
    {
        (machine) ? D_FTP_COMMAND_MLSD : D_FTP_COMMAND_LIST,
        _path,
        _sink,
        NULL,
        _context,
        false
    };

    // the dialect the caller will parse
    if (_out_format)
    {
        *_out_format = (machine) ? D_FTP_LISTING_MLSX : D_FTP_LISTING_AUTO;
    }

    return d_ftp_internal_client_download(_client,
                                          &transfer);
}
