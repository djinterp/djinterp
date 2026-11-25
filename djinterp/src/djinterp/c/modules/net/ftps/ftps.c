/*******************************************************************************
* djinterp [c][net]                                                       ftps.c
*
* Implementation of the FTPS client declared in ftps.h: the TLS layer the
* FTP client runs its connections through, and the setup that gives it one.
*   Each of the layer's functions finds the session of the connection it is
* given - the control connection's or the data connection's - and puts the
* kernel's status as the FTP client reads one: CLOSED at the end of the
* stream, TRANSPORT for the transport's failure, TRUNCATED for a stream that
* stopped without close_notify, and anything else as the mechanism's own,
* kept for d_ftps_client_tls_status.
*
*
* path:      /src/djinterp/c/modules/net/ftps/ftps.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/net/ftps/ftps.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memset, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"                   // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"           // dj_pack_text
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_client.h"   // the FTP client
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_options.h"  // dj_ftp_options
#include "../../../../../../inc/djinterp/c/modules/net/net_common.h"       // dj_net_error
#include "../../../../../../inc/djinterp/c/modules/net/tls/tls.h"          // the kernel
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


//==============================================================================
// 1.  THE LAYER
//==============================================================================

/*
d_ftps_internal_session
  File-local: the session over `_connection`, which is the client's control
connection or its data connection.
*/
D_STATIC struct dj_tls_session*
d_ftps_internal_session(
    struct dj_ftps_client*          _client,
    const struct dj_ftp_connection* _connection
)
{
    return (_connection == &_client->ftp.control) ? &_client->control_tls
                                                  : &_client->data_tls;
}

/*
d_ftps_internal_io
  File-local: the kernel's status as the FTP client reads one.
*/
D_STATIC enum dj_ftp_io
d_ftps_internal_io(
    enum dj_tls_error _status
)
{
    switch (_status)
    {
        case D_TLS_OK:
            return D_FTP_IO_OK;
        case D_TLS_ERROR_CLOSED:
            return D_FTP_IO_CLOSED;
        case D_TLS_ERROR_TRANSPORT:
            return D_FTP_IO_TRANSPORT;
        case D_TLS_ERROR_TRUNCATED:
            return D_FTP_IO_TRUNCATED;
        default:
            return D_FTP_IO_SECURITY;
    }
}

/*
d_ftps_internal_honours
  File-local: whether the context checks at least what the options require:
a client's plan that verifies the chain, or pins, when verify_peer is set,
and the host name too when verify_host is.
*/
D_STATIC bool
d_ftps_internal_honours(
    const void*                  _layer,
    const struct dj_ftp_options* _options
)
{
    const struct dj_ftps_client* const client =
        (const struct dj_ftps_client*)_layer;

    // no context, or one never initialized
    if ( (!client->tls) ||
         (!client->tls->engine) )
    {
        return false;
    }

    const struct dj_tls_plan* const plan = &client->tls->plan;

    return ( (plan->role == D_TLS_ROLE_CLIENT) &&
             ( (!_options->verify_peer) ||
               (plan->verify_chain)     ||
               (plan->check_pins) )      &&
             ( (!_options->verify_peer) ||
               (!_options->verify_host) ||
               (plan->verify_hostname) ) );
}

/*
d_ftps_internal_secure
  File-local: a session over the connection's transport, naming the host,
which the kernel checks the certificate against and, for a name, sends as
SNI; a data connection's is offered the control session to resume. A
failure keeps the transport's error in the connection's `failure` and
leaves no session behind.
*/
D_STATIC enum dj_ftp_io
d_ftps_internal_secure(
    void*                           _layer,
    struct dj_ftp_connection*       _connection,
    const struct dj_ftp_connection* _resume,
    const char*                     _host,
    int32_t*                        _status
)
{
    struct dj_ftps_client* const  client  = (struct dj_ftps_client*)_layer;
    struct dj_tls_session* const  session =
        d_ftps_internal_session(client,
                                _connection);
    const struct dj_pack_text     host    = { _host, strlen(_host) };
    enum dj_tls_error             status  =
        d_tls_session_init(session,
                           client->tls,
                           &_connection->tcp.base,
                           host);

    // a data connection offers the control connection's session
    if ( (status == D_TLS_OK) &&
         (_resume) )
    {
        status = d_tls_session_resume(session,
                                      d_ftps_internal_session(client,
                                                              _resume));
    }

    // the handshake runs only on a session that could be made
    if (status == D_TLS_OK)
    {
        status = d_tls_session_handshake(session);
    }

    *_status = (int32_t)status;

    // a failure leaves nothing behind
    if (status != D_TLS_OK)
    {
        _connection->failure = d_tls_session_transport_error(session);
        d_tls_session_destroy(session);
    }

    return d_ftps_internal_io(status);
}

/*
d_ftps_internal_read
  File-local: reads through the connection's session.
*/
D_STATIC enum dj_ftp_io
d_ftps_internal_read(
    void*                     _layer,
    struct dj_ftp_connection* _connection,
    void*                     _buffer,
    size_t                    _capacity,
    size_t*                   _out_read,
    int32_t*                  _status
)
{
    const enum dj_tls_error status =
        d_tls_session_read(d_ftps_internal_session(
                               (struct dj_ftps_client*)_layer,
                               _connection),
                           _buffer,
                           _capacity,
                           _out_read);

    *_status = (int32_t)status;

    return d_ftps_internal_io(status);
}

/*
d_ftps_internal_write
  File-local: writes all of `_data` through the connection's session.
*/
D_STATIC enum dj_ftp_io
d_ftps_internal_write(
    void*                     _layer,
    struct dj_ftp_connection* _connection,
    const void*               _data,
    size_t                    _size,
    int32_t*                  _status
)
{
    const enum dj_tls_error status =
        d_tls_session_write_all(d_ftps_internal_session(
                                    (struct dj_ftps_client*)_layer,
                                    _connection),
                                _data,
                                _size,
                                NULL);

    *_status = (int32_t)status;

    return d_ftps_internal_io(status);
}

/*
d_ftps_internal_shutdown
  File-local: sends close_notify on the connection's session.
*/
D_STATIC enum dj_ftp_io
d_ftps_internal_shutdown(
    void*                     _layer,
    struct dj_ftp_connection* _connection,
    int32_t*                  _status
)
{
    const enum dj_tls_error status =
        d_tls_session_shutdown(d_ftps_internal_session(
                                   (struct dj_ftps_client*)_layer,
                                   _connection));

    *_status = (int32_t)status;

    return d_ftps_internal_io(status);
}

/*
d_ftps_internal_drop
  File-local: releases the connection's session, saying nothing.
*/
D_STATIC void
d_ftps_internal_drop(
    void*                     _layer,
    struct dj_ftp_connection* _connection
)
{
    d_tls_session_destroy(d_ftps_internal_session(
                              (struct dj_ftps_client*)_layer,
                              _connection));

    return;
}

/*
d_ftps_internal_transport_error
  File-local: what the session's transport last reported.
*/
D_STATIC enum dj_net_error
d_ftps_internal_transport_error(
    const void*                     _layer,
    const struct dj_ftp_connection* _connection
)
{
    const struct dj_ftps_client* const client =
        (const struct dj_ftps_client*)_layer;
    const struct dj_tls_session* const session =
        (_connection == &client->ftp.control) ? &client->control_tls
                                              : &client->data_tls;

    return d_tls_session_transport_error(session);
}

// g_ftps_tls
//   constant: the TLS layer's functions, the same for every client.
static const struct dj_ftp_security_ops g_ftps_tls =
{
    "TLS",
    d_ftps_internal_honours,
    d_ftps_internal_secure,
    d_ftps_internal_read,
    d_ftps_internal_write,
    d_ftps_internal_shutdown,
    d_ftps_internal_drop,
    d_ftps_internal_transport_error
};


//==============================================================================
// 2.  CLIENT
//==============================================================================

/*
d_ftps_client_tls_config
  The kernel's client defaults already verify the chain and the host name
against the system trust store; only what the options turn off changes.
*/
void d_ftps_client_tls_config(
    const struct dj_ftp_options* _options,
    struct dj_tls_config*        _config
)
{
    // parameter validation
    if ( (!_options) ||
         (!_config) )
    {
        return;
    }

    d_tls_config_init(_config,
                      D_TLS_ROLE_CLIENT);

    _config->verify          = (_options->verify_peer) ? D_TLS_VERIFY_REQUIRED
                                                       : D_TLS_VERIFY_NONE;
    _config->verify_hostname = ( (_options->verify_peer) &&
                                 (_options->verify_host) );

    return;
}

/*
d_ftps_client_init
  Zeroes the whole client, sessions included, then prepares its FTP client
and gives it this module's layer over the client itself.
*/
void d_ftps_client_init(
    struct dj_ftps_client*       _client,
    const struct dj_ftp_options* _options,
    const struct dj_tls_context* _tls
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

    d_ftp_client_init(&_client->ftp,
                      _options);
    _client->tls = _tls;
    d_ftp_client_set_security(&_client->ftp,
                              &g_ftps_tls,
                              _client);

    return;
}

/*
d_ftps_client_tls_status
  The FTP client keeps the layer's status as the layer gave it.
*/
enum dj_tls_error
d_ftps_client_tls_status(
    const struct dj_ftps_client* _client
)
{
    return (_client) ? (enum dj_tls_error)_client->ftp.security_status
                     : D_TLS_ERROR_INVALID_ARGUMENT;
}
