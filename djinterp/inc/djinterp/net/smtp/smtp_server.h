/*******************************************************************************
* djinterp [net]                                                   smtp_server.h
*
* djinterp SMTP server.
*   The receiving side of RFC 5321 as a session engine: d_smtp_server_session()
* runs one connection, from greeting to QUIT, over any d_smtp_transport, and
* hands each accepted message to the caller's handler. It advertises and
* implements PIPELINING, SIZE, 8BITMIME, ENHANCEDSTATUSCODES and SMTPUTF8,
* plus STARTTLS when the transport can upgrade and a certificate is
* configured, and AUTH PLAIN and LOGIN when the handler can check
* credentials.
*   The engine owns no sockets and no threads. A caller accepts connections
* (d_smtp_listener_* serves) and decides how sessions run in parallel; the
* C++ facade in net/smtp.hpp runs each on a thread of its own.
*   Hardening: plaintext pipelined behind STARTTLS is discarded (the
* CVE-2011-0411 injection) and the session starts over after the upgrade;
* only CRLF "." CRLF ends message data, closing the SMTP-smuggling ambiguity
* of bare line feeds; AUTH is withheld from plaintext sessions unless allowed;
* message size and recipient count are bounded; and a client that keeps
* erring is dropped.
*
*
* path:      /inc/djinterp/net/smtp/smtp_server.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION
    -------------
    1.  Limits
    2.  Server configuration
         1.  d_smtp_server_config
2.  HANDLERS
    --------
    1.  Session view
         1.  d_smtp_session_info
    2.  Callbacks
    3.  Handler table
         1.  d_smtp_server_handler
3.  SESSIONS
    --------
    1.  Initialization
    2.  Running a session
*/

#ifndef DJINTERP_NET_SMTP_SMTP_SERVER_H
#define DJINTERP_NET_SMTP_SMTP_SERVER_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "./smtp_common.h"     // d_smtp_error
#include "./smtp_transport.h"  // d_smtp_transport, d_smtp_tls_options


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONFIGURATION
//==============================================================================
// What a server calls itself, what it accepts, and what it demands.


// 1.1    Limits
//------------------------------------------------------------------------------
// D_SMTP_SERVER_MESSAGE_MAX
//   constant: default largest message accepted, advertised as SIZE (10 MiB).
#define D_SMTP_SERVER_MESSAGE_MAX     10485760u

// D_SMTP_SERVER_RECIPIENTS_MAX
//   constant: default recipients per message -- the minimum RFC 5321 section
// 4.5.3.1.8 requires a server to accept.
#define D_SMTP_SERVER_RECIPIENTS_MAX  100u

// D_SMTP_SERVER_ERRORS_MAX
//   constant: syntax and sequence errors tolerated before the session is
// dropped with 421.
#define D_SMTP_SERVER_ERRORS_MAX      10u

// 1.2    Server configuration
//------------------------------------------------------------------------------
// 1.2.1
// d_smtp_server_config
//   struct: one server's settings. Strings are borrowed and must outlive
// every session that uses them.
struct d_smtp_server_config
{
    const char*               domain;               // name in replies
    size_t                    max_message_size;     // bytes; SIZE
    size_t                    max_recipients;       // per message
    struct d_smtp_tls_options tls;                  // certificate: STARTTLS
    unsigned int              auth_mechanisms;      // PLAIN and/or LOGIN
    bool                      require_tls;          // MAIL needs TLS first
    bool                      require_auth;         // MAIL needs AUTH first
    bool                      allow_insecure_auth;  // AUTH without TLS
    bool                      secure;               // already encrypted
};


//==============================================================================
// 2.  HANDLERS
//==============================================================================
// The server decides nothing about mail on its own: senders, recipients and
// messages go to the caller's callbacks, which answer with 0 to accept or a
// 4yz or 5yz reply code to refuse. The server supplies the enhanced status
// and text; a code outside 400-599 is treated as a local error (451).


// 2.1    Session view
//------------------------------------------------------------------------------
// 2.1.1
// d_smtp_session_info
//   struct: what a callback may know about its session. Every pointer is
// valid only for the duration of the callback.
struct d_smtp_session_info
{
    const char*        client_domain;    // EHLO or HELO argument
    const char*        user;             // authenticated identity, or NULL
    const char*        sender;           // open transaction's reverse-path,
                                         // "" for the null sender, or NULL
    const char* const* recipients;       // forward-paths accepted so far
    size_t             recipient_count;
    bool               secure;           // TLS is active
    bool               eight_bit;        // BODY=8BITMIME was declared
    bool               utf8;             // SMTPUTF8 was declared
};

// 2.2    Callbacks
//------------------------------------------------------------------------------
// d_smtp_server_mail_fn
//   function type: decides on the sender of a new transaction.
typedef unsigned int (*d_smtp_server_mail_fn)(
                         void*                             _context,
                         const struct d_smtp_session_info* _session,
                         const char*                       _sender);

// d_smtp_server_rcpt_fn
//   function type: decides on one recipient.
typedef unsigned int (*d_smtp_server_rcpt_fn)(
                         void*                             _context,
                         const struct d_smtp_session_info* _session,
                         const char*                       _recipient);

// d_smtp_server_message_fn
//   function type: takes delivery of a complete message -- transparency
// removed, every line ending CRLF, and NUL-terminated for convenience.
typedef unsigned int (*d_smtp_server_message_fn)(
                         void*                             _context,
                         const struct d_smtp_session_info* _session,
                         const char*                       _data,
                         size_t                            _length);

// d_smtp_server_auth_fn
//   function type: checks a user and password; true accepts.
typedef bool         (*d_smtp_server_auth_fn)(void*       _context,
                                              const char* _user,
                                              const char* _secret);

// 2.3    Handler table
//------------------------------------------------------------------------------
// 2.3.1
// d_smtp_server_handler
//   struct: the callbacks and the context passed to each.
struct d_smtp_server_handler
{
    void*                    context;
    d_smtp_server_mail_fn    mail;      // NULL: every sender accepted
    d_smtp_server_rcpt_fn    rcpt;      // NULL: every recipient accepted
    d_smtp_server_message_fn message;   // NULL: accepted and discarded
    d_smtp_server_auth_fn    auth;      // NULL: AUTH is not offered
};


//==============================================================================
// 3.  SESSIONS
//==============================================================================


// 3.1    Initialization
//------------------------------------------------------------------------------
// defaults -- neither fails, NULL is ignored. A configuration starts as
// "localhost" with the D_SMTP_SERVER_* limits, PLAIN and LOGIN, and no
// requirements; a handler starts with every callback NULL.
void              d_smtp_server_config_init(
                      struct d_smtp_server_config* _config);
void              d_smtp_server_handler_init(
                      struct d_smtp_server_handler* _handler);

// 3.2    Running a session
//------------------------------------------------------------------------------
/**
 * @brief Serves one connection, from the 220 greeting until QUIT or until
 *        the connection fails.
 *
 * @note The transport's `starttls` performs the server side of the
 *       handshake; the socket transport does so for accepted sockets.
 *
 * @param[in] _transport  the connection; not closed by the session.
 * @param[in] _config     the server's settings; `domain` must be valid.
 * @param[in] _handler    the caller's callbacks.
 * @return D_SMTP_OK when the client said QUIT; D_SMTP_ERROR_INVALID for bad
 *         arguments; D_SMTP_ERROR_NO_MEMORY; D_SMTP_ERROR_PROTOCOL when the
 *         client was dropped for errors; or the transport's error --
 *         D_SMTP_ERROR_CLOSED, D_SMTP_ERROR_TIMEOUT, D_SMTP_ERROR_TLS.
 */
enum d_smtp_error d_smtp_server_session(
                      const struct d_smtp_transport*      _transport,
                      const struct d_smtp_server_config*  _config,
                      const struct d_smtp_server_handler* _handler);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SMTP_SMTP_SERVER_H
