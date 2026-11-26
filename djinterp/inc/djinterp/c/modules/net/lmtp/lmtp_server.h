/*******************************************************************************
* djinterp [c][net]                                                lmtp_server.h
*
* LMTP (RFC 2033): the server, a local delivery agent's side, which takes a
* message and answers for each recipient whether its mailbox took it.
*   An LMTP server is the SMTP kernel's session engine (net/smtp/
* smtp_server.h) in LMTP's dialect: it greets as LMTP, takes LHLO and
* refuses EHLO and HELO, answers DATA with no recipient accepted with 503,
* and, for the data, sends one reply for each recipient accepted, in the
* order of their RCPTs, as soon as each is decided. The handler's `deliver`
* decides each, when it has one: a mailbox full or a user over quota is that
* recipient's refusal alone. Without one, its `message` decides once, for
* every recipient. It offers PIPELINING and ENHANCEDSTATUSCODES, which an
* LMTP server must, and 8BITMIME, which it should (RFC 2033 section 5).
*   A server that answers 2yz for a recipient has taken the message for
* that mailbox, and must not lose it (RFC 2033 section 4.2): `deliver`
* answers 0 only once the message is safe.
*   The engine owns no sockets and no threads, as the SMTP one does not: a
* caller accepts connections - on a unix-domain socket, as LMTP's are
* usually offered, or TCP on any port but 25 - and runs each session.
*
*
* path:      /inc/djinterp/c/modules/net/lmtp/lmtp_server.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SERVER
    ------
    1.  Setup
    2.  Running a session
*/

#ifndef DJINTERP_C_NET_LMTP_LMTP_SERVER_H
#define DJINTERP_C_NET_LMTP_LMTP_SERVER_H 1

// djinterp
#include "../../../djinterp.h"  // framework root
#include "../net_common.h"          // dj_net_connection
#include "../smtp/smtp_common.h"    // dj_smtp_error, dj_smtp_dialect
#include "../smtp/smtp_server.h"    // the kernel's session engine


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SERVER
//==============================================================================


// 1.1    Setup
//------------------------------------------------------------------------------
// d_lmtp_server_config_init - the kernel's defaults, in LMTP's dialect:
// "localhost", its limits, PLAIN and LOGIN, no context, and nothing
// required. NULL is ignored. The handler is the kernel's, made with
// d_smtp_server_handler_init(), `deliver` among its callbacks.
void d_lmtp_server_config_init(
    struct dj_smtp_server_config* _config
);

// 1.2    Running a session
//------------------------------------------------------------------------------
/**
 * @brief Serves one connection in LMTP, from the greeting until QUIT or
 *        until the connection fails, as d_smtp_server_session() does,
 *        whatever dialect the configuration names.
 *
 * @param[in,out] _connection  the connection, borrowed; never closed here.
 * @param[in]     _config      the server's settings.
 * @param[in]     _handler     the caller's callbacks.
 * @return as d_smtp_server_session().
 */
enum dj_smtp_error d_lmtp_server_session(
    struct dj_net_connection*            _connection,
    const struct dj_smtp_server_config*  _config,
    const struct dj_smtp_server_handler* _handler
);


D_EXTERN_C_END


#endif  // DJINTERP_C_NET_LMTP_LMTP_SERVER_H
