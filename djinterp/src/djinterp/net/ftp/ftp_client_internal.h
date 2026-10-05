/*******************************************************************************
* djinterp [net]                                           ftp_client_internal.h
*
* Private helpers shared by the two halves of the FTP client.
*   ftp_client.c holds the session -- connecting, securing, logging in, and
* the control connection's replies -- and ftp_client_data.c the data
* connections and transfers. Both read and write connections, and exchange
* commands, through the helpers declared here, which ftp_client.c defines.
* Not installed, and not part of the public interface.
*
*
* path:      /src/djinterp/net/ftp/ftp_client_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONNECTIONS
    -----------
    1.  Transfers
    2.  Lifetime
2.  CONTROL
    -------
    1.  Commands and replies
*/

#ifndef DJINTERP_NET_FTP_FTP_CLIENT_INTERNAL_H
#define DJINTERP_NET_FTP_FTP_CLIENT_INTERNAL_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint16_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_client.h"     // d_ftp_client
#include "../../../../inc/djinterp/net/ftp/ftp_command.h"    // d_ftp_command
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"     // d_ftp_error
#include "../../../../inc/djinterp/net/ssl/ssl.h"            // d_ssl_status


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_client.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONNECTIONS
//==============================================================================


// 1.1    Transfers
//------------------------------------------------------------------------------
// Reads and writes go through a connection's TLS session while one is in
// force and through its transport otherwise, both reported as SSL statuses:
// D_SSL_STATUS_CONNECTION_CLOSED at the end of the stream, and
// D_SSL_STATUS_TRANSPORT_ERROR for a transport failure, whose error the
// connection's `failure` keeps. A write sends everything and flushes it.
// d_ftp_internal_client_failure() turns a failed status into an FTP error:
// a timeout, `_lost` for a stream that ended or broke, or D_FTP_ERROR_TLS
// with the status kept -- a TLS stream ending without close_notify counts
// as lost, not as TLS, on the control connection alone, whose replies frame
// themselves.
enum d_ssl_status d_ftp_internal_client_read(
                      struct d_ftp_connection* _connection,
                      void*                    _buffer,
                      size_t                   _capacity,
                      size_t*                  _out_read);
enum d_ssl_status d_ftp_internal_client_write(
                      struct d_ftp_connection* _connection,
                      const void*              _data,
                      size_t                   _size);
enum d_ftp_error  d_ftp_internal_client_failure(
                      struct d_ftp_client*           _client,
                      const struct d_ftp_connection* _connection,
                      enum d_ssl_status              _status,
                      enum d_ftp_error               _lost);


// 1.2    Lifetime
//------------------------------------------------------------------------------
// d_ftp_internal_client_dial() connects a closed connection to a host and
// port, bounded by the options' connect_timeout_ms, and on failure returns
// D_FTP_ERROR_RESOLVE, D_FTP_ERROR_TIMEOUT, D_FTP_ERROR_OUT_OF_MEMORY, or
// `_lost`, the transport's error kept in `net_error`.
// d_ftp_internal_client_secure() runs a TLS client handshake over a
// connection, naming the server's host, and leaves no session behind on
// failure. d_ftp_internal_client_drop() closes a connection, saying
// close_notify first when `_orderly`.
enum d_ftp_error d_ftp_internal_client_dial(
                     struct d_ftp_client*     _client,
                     struct d_ftp_connection* _connection,
                     const char*              _host,
                     uint16_t                 _port,
                     enum d_ftp_error         _lost);
enum d_ftp_error d_ftp_internal_client_secure(
                     struct d_ftp_client*     _client,
                     struct d_ftp_connection* _connection);
void             d_ftp_internal_client_drop(
                     struct d_ftp_connection* _connection,
                     bool                     _orderly);


//==============================================================================
// 2.  CONTROL
//==============================================================================


// 2.1    Commands and replies
//------------------------------------------------------------------------------
// d_ftp_internal_client_reply() reads the next whole reply into `reply`;
// d_ftp_internal_client_send() writes one command line;
// d_ftp_internal_client_exchange() does both, reading past preliminary
// replies, and leaves the final reply for the caller to judge -- only a
// failure to send or read is an error there. d_ftp_internal_client_positive()
// reports whether the last reply was 2yz, and
// d_ftp_internal_client_refusal() the error for a reply that did not give
// what was asked: its own, or D_FTP_ERROR_UNEXPECTED_REPLY for the wrong
// positive one.
enum d_ftp_error d_ftp_internal_client_reply(struct d_ftp_client* _client);
enum d_ftp_error d_ftp_internal_client_send(struct d_ftp_client* _client,
                                            enum d_ftp_command   _command,
                                            const char*          _argument);
enum d_ftp_error d_ftp_internal_client_exchange(
                     struct d_ftp_client* _client,
                     enum d_ftp_command   _command,
                     const char*          _argument);
bool             d_ftp_internal_client_positive(
                     const struct d_ftp_client* _client);
enum d_ftp_error d_ftp_internal_client_refusal(
                     const struct d_ftp_client* _client);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_CLIENT_INTERNAL_H
