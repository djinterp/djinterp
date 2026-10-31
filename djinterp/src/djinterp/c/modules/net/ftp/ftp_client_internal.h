/*******************************************************************************
* djinterp [c/net]                                         ftp_client_internal.h
*
* Private helpers shared by the two halves of the FTP client.
*   ftp_client.c holds the session -- connecting, securing, logging in, and
* the control connection's replies -- and ftp_client_data.c the data
* connections and transfers. Both read and write connections, and exchange
* commands, through the helpers declared here, which ftp_client.c defines.
* Not installed, and not part of the public interface.
*
*
* path:      /src/djinterp/c/modules/net/ftp/ftp_client_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.16
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
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_client.h"     // dj_ftp_client
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_command.h"    // dj_ftp_command
#include "../../../../../../inc/djinterp/c/modules/net/ftp/ftp_common.h"     // dj_ftp_error
#include "../../../../../../inc/djinterp/c/modules/net/ssl/ssl.h"            // dj_ssl_error
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint16_t


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
// D_SSL_ERROR_CLOSED at the end of the stream, and
// D_SSL_ERROR_TRANSPORT for a transport failure. A write sends everything
// and flushes it. The transport's last error is the connection's `failure`
// without TLS, and the session's record of its transport with it.
// d_ftp_internal_client_failure() turns a failed status, from these calls or
// from the session called directly, into an FTP error, taking the
// transport's error into `failure` and `net_error` first: a timeout, `_lost`
// for a stream that ended or broke, or D_FTP_ERROR_TLS with the status kept
// -- a TLS stream ending without close_notify counts as lost, not as TLS,
// on the control connection alone, whose replies frame themselves.
enum dj_ssl_error d_ftp_internal_client_read(
    struct dj_ftp_connection* _connection,
    void*                     _buffer,
    size_t                    _capacity,
    size_t*                   _out_read
);
enum dj_ssl_error d_ftp_internal_client_write(
    struct dj_ftp_connection* _connection,
    const void*               _data,
    size_t                    _size
);
enum dj_ftp_error d_ftp_internal_client_failure(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    enum dj_ssl_error         _status,
    enum dj_ftp_error         _lost
);


// 1.2    Lifetime
//------------------------------------------------------------------------------
// d_ftp_internal_client_dial() connects a closed connection to a host and
// port, bounded by the options' connect_timeout_ms, and on failure returns
// D_FTP_ERROR_RESOLVE, D_FTP_ERROR_TIMED_OUT, D_FTP_ERROR_OUT_OF_MEMORY, or
// `_lost`, the transport's error kept in `net_error`; it bounds what it
// connects as d_ftp_internal_client_bound() does.
// d_ftp_internal_client_bound() bounds a connection's reads and writes: the
// control connection's by response_timeout_ms, a data connection's by
// idle_timeout_ms, 0 being no limit; it returns the transport's error.
// d_ftp_internal_client_secure() runs a TLS client handshake over a
// connection, naming the server's host, and leaves no session behind on
// failure. d_ftp_internal_client_drop() closes a connection, saying
// close_notify first when `_orderly`.
enum dj_net_error d_ftp_internal_client_bound(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection
);
enum dj_ftp_error d_ftp_internal_client_dial(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection,
    const char*               _host,
    uint16_t                  _port,
    enum dj_ftp_error         _lost
);
enum dj_ftp_error d_ftp_internal_client_secure(
    struct dj_ftp_client*     _client,
    struct dj_ftp_connection* _connection
);
void d_ftp_internal_client_drop(
    struct dj_ftp_connection* _connection,
    bool                      _orderly
);


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
enum dj_ftp_error d_ftp_internal_client_reply(
    struct dj_ftp_client* _client
);
enum dj_ftp_error d_ftp_internal_client_send(
    struct dj_ftp_client* _client,
    enum dj_ftp_command   _command,
    const char*           _argument
);
enum dj_ftp_error d_ftp_internal_client_exchange(
    struct dj_ftp_client* _client,
    enum dj_ftp_command   _command,
    const char*           _argument
);
bool d_ftp_internal_client_positive(
    const struct dj_ftp_client* _client
);
enum dj_ftp_error d_ftp_internal_client_refusal(
    const struct dj_ftp_client* _client
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_CLIENT_INTERNAL_H
