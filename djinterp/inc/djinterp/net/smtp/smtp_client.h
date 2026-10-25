/*******************************************************************************
* djinterp [net]                                                   smtp_client.h
*
* djinterp SMTP client.
*   A submission and relay client for RFC 5321 with the extensions a modern
* submission service expects: STARTTLS (RFC 3207) and implicit TLS (RFC
* 8314), SMTP AUTH (RFC 4954) with PLAIN, LOGIN, CRAM-MD5 and XOAUTH2,
* PIPELINING (RFC 2920), SIZE (RFC 1870), 8BITMIME (RFC 6152) and SMTPUTF8
* (RFC 6531).
*   The client drives any net.h connection. d_smtp_client_connect() opens a
* TCP connection with tcp.h and runs the whole opening -- greeting, EHLO and
* the STARTTLS policy -- while d_smtp_client_attach() adopts a caller's
* connection and leaves those steps to the caller. Either way,
* d_smtp_client_send() carries out a whole mail transaction. TLS, implicit
* or by STARTTLS, is a session of the SSL kernel over the connection, made
* from a client d_ssl_context the caller configured: its engine, its trust,
* and its version floor.
*   Defaults are strict: STARTTLS is required unless the caller opts down,
* certificates are verified, credentials never cross an unencrypted channel
* without an explicit opt-in, and every address and argument is validated, so
* CR, LF and NUL can never inject a command.
*   The client holds its TLS session, so the header needs the SSL kernel's
* 64-bit integer type, and compiles to nothing where the build has none.
*
*   Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type, it
* compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error
* (decision 101 of the register).
*
*
* path:      /inc/djinterp/net/smtp/smtp_client.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION
    -------------
    1.  Security modes
         1.  d_smtp_security
    2.  Credentials
         1.  d_smtp_credentials
    3.  Client options
         1.  d_smtp_client_options
2.  TRANSACTIONS
    ------------
    1.  Envelope
         1.  d_smtp_envelope
    2.  Send report
         1.  d_smtp_recipient_status
         2.  d_smtp_send_report
3.  CLIENT
    ------
    1.  Client state
         1.  d_smtp_client_state
         2.  d_smtp_client
    2.  Session setup
    3.  Mail transactions
    4.  Session control
    5.  Queries
*/

#ifndef DJINTERP_NET_SMTP_SMTP_CLIENT_H
#define DJINTERP_NET_SMTP_SMTP_CLIENT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "../net_common.h"     // d_net_connection
#include "../ssl/ssl.h"        // d_ssl_context, d_ssl_session
#include "../tcp/tcp.h"        // d_tcp_connection
#include "./smtp_common.h"     // replies, capabilities, errors
#include "./smtp_reader.h"     // d_smtp_reader
#include "../../config/net/cfg_net.h"  // D_INTERNAL_NET_REQUIRE_BACKENDS

// 64-bit floor: this header needs a 64-bit integer type, and compiles to
// nothing where the build has none -- ISO strict C++98 wherever int64_t is
// `long long`: a 32-bit target, 64-bit Windows or macOS (the owner's ruling of
// 2026.10.03 on round 3's question 1, (a)). The answer is net_common.h's, taken
// once per translation unit, so that net's gated headers agree.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONFIGURATION
//==============================================================================
// How a client connects, secures its channel, and proves who it is.


// 1.1    Security modes
//------------------------------------------------------------------------------
// 1.1.1
// d_smtp_security
//   enum: how the channel is protected. STARTTLS is the default because it
// fails closed: a server that does not offer the upgrade -- which is also
// what an attacker stripping it produces -- ends the connection.
enum d_smtp_security
{
    D_SMTP_SECURITY_NONE = 0,           // plaintext; never upgrades
    D_SMTP_SECURITY_STARTTLS_OPTIONAL,  // upgrade when offered, else plaintext
    D_SMTP_SECURITY_STARTTLS,           // upgrade, or fail
    D_SMTP_SECURITY_TLS                 // implicit TLS from the first byte
};

// 1.2    Credentials
//------------------------------------------------------------------------------
// 1.2.1
// d_smtp_credentials
//   struct: an identity and its secret. `mechanism` is a mask of acceptable
// D_SMTP_AUTH_* mechanisms; D_SMTP_AUTH_NONE accepts any password mechanism
// the server offers, in the order PLAIN, LOGIN, CRAM-MD5. XOAUTH2 takes a
// bearer token as its secret, so it is used only when requested.
struct d_smtp_credentials
{
    const char*  user;
    const char*  secret;
    unsigned int mechanism;
};

// 1.3    Client options
//------------------------------------------------------------------------------
// D_SMTP_CLIENT_CONNECT_TIMEOUT_MS
//   constant: default limit on each connection attempt.
#define D_SMTP_CLIENT_CONNECT_TIMEOUT_MS  30000L

// D_SMTP_CLIENT_IO_TIMEOUT_MS
//   constant: default limit on each read and write -- the five minutes RFC
// 5321 section 4.5.3.2 recommends for most replies.
#define D_SMTP_CLIENT_IO_TIMEOUT_MS       300000L

// 1.3.1
// d_smtp_client_options
//   struct: everything d_smtp_client_connect() needs. Strings are borrowed
// for the duration of the call. `tls` is a client context of the SSL kernel,
// borrowed for as long as the session lasts; the modes that require TLS
// need one, and STARTTLS_OPTIONAL without one stays in plaintext.
// `server_name` is the name the server's certificate must carry, and the
// SNI sent; it comes from the caller, never from the server.
struct d_smtp_client_options
{
    const char*                 host;
    unsigned int                port;                 // 0: by security mode
    enum d_smtp_security        security;
    const struct d_ssl_context* tls;                  // NULL: plaintext only
    const char*                 server_name;          // NULL: host
    const char*                 helo_domain;          // NULL: this machine
    long                        connect_timeout_ms;
    long                        io_timeout_ms;
    bool                        allow_insecure_auth;  // AUTH without TLS
};

// d_smtp_client_attach_options
//   struct: what d_smtp_client_attach_with() takes with a caller's stream:
// whether it is already encrypted, the identity d_smtp_client_hello()
// announces when given NULL, and whether AUTH may run without TLS (decision
// 107 of the register). Strings are borrowed for the call.
struct d_smtp_client_attach_options
{
    bool        secure;               // the stream is already encrypted
    const char* helo_domain;          // NULL: hello() names one
    bool        allow_insecure_auth;  // AUTH without TLS
};


//==============================================================================
// 2.  TRANSACTIONS
//==============================================================================
// One message: who it is from, who it is for, and what became of each.


// 2.1    Envelope
//------------------------------------------------------------------------------
// 2.1.1
// d_smtp_envelope
//   struct: the SMTP envelope. The addresses are bare mailboxes, without
// angle brackets; an empty sender is the null reverse-path used for bounces.
struct d_smtp_envelope
{
    const char*        sender;
    const char* const* recipients;
    size_t             recipient_count;
    bool               eight_bit;        // BODY=8BITMIME (RFC 6152)
    bool               utf8;             // SMTPUTF8 (RFC 6531)
};

// 2.2    Send report
//------------------------------------------------------------------------------
// 2.2.1
// d_smtp_recipient_status
//   struct: the server's answer to one RCPT.
struct d_smtp_recipient_status
{
    unsigned int         code;
    struct d_smtp_status status;
};

// 2.2.2
// d_smtp_send_report
//   struct: what became of a transaction. `recipients`, when not NULL, is
// caller-owned storage for one entry per envelope recipient.
struct d_smtp_send_report
{
    struct d_smtp_recipient_status* recipients;
    size_t                          accepted;    // recipients accepted
    unsigned int                    data_code;   // reply to the data; 0 if
                                                 // the data was not sent
};


//==============================================================================
// 3.  CLIENT
//==============================================================================
// A client is a state machine over one connection. It is large -- it embeds
// its read buffer, its last reply, the TCP connection connect() opens, and
// the TLS session it makes -- and must not move while connected, since its
// reader points into it.


// 3.1    Client state
//------------------------------------------------------------------------------
// 3.1.1
// d_smtp_client_state
//   enum: where a session stands.
enum d_smtp_client_state
{
    D_SMTP_CLIENT_CLOSED = 0,  // no transport
    D_SMTP_CLIENT_GREETING,    // connected; the server's 220 is next
    D_SMTP_CLIENT_HELLO,       // greeted; EHLO or HELO is next
    D_SMTP_CLIENT_READY,       // session open; a transaction may start
    D_SMTP_CLIENT_MAIL,        // MAIL accepted; RCPT or DATA may follow
    D_SMTP_CLIENT_QUIT         // QUIT sent, or STARTTLS failed after its
                               // go-ahead: only close remains
};

// 3.1.2
// d_smtp_client
//   struct: one client session. `reply` always holds the last reply read, so
// a D_SMTP_ERROR_REJECTED can be explained; it survives d_smtp_client_close.
//   `connection` is the stream the session runs on: `tcp`'s when connect()
// opened it, or the caller's, which is borrowed. Once this client has made
// TLS over it, `tls` is that session and the reader reads its view. Why a
// session failed is kept: `reader.net_error` holds what the connection
// reported, and `tls_status` what the SSL kernel did when TLS failed,
// D_SSL_OK otherwise.
struct d_smtp_client
{
    struct d_smtp_reader       reader;               // reads the stream
    struct d_net_connection*   connection;           // the plaintext stream
    struct d_tcp_connection    tcp;                  // when connect() opened
    struct d_ssl_session       tls;                  // when this client made
    struct d_smtp_capabilities capabilities;
    struct d_smtp_reply        reply;
    enum d_smtp_client_state   state;
    enum d_ssl_error           tls_status;           // why TLS failed
    bool                       owns_tcp;             // connect() opened `tcp`
    bool                       has_tls;              // `tls` is live
    bool                       secure;               // channel encrypted
    bool                       allow_insecure_auth;
    bool                       authenticated;
    bool                       utf8;                 // SMTPUTF8 transaction
    char                       helo[D_SMTP_DOMAIN_MAX + 1u];
};

// 3.2    Session setup
//------------------------------------------------------------------------------
// initialization -- neither fails; NULL is ignored. Options default to
// STARTTLS and the D_SMTP_CLIENT_*_TIMEOUT_MS limits, without a context.
void              d_smtp_client_options_init(
                      struct d_smtp_client_options* _options);
void              d_smtp_client_init(struct d_smtp_client* _client);

/**
 * @brief Connects, reads the greeting, says EHLO, and applies the STARTTLS
 *        policy, leaving the session ready for authentication or mail.
 *
 * @note Opening the connection is judged apart from what the session reads
 *       and writes: a host whose name does not resolve, or that no route
 *       reaches, is D_SMTP_ERROR_RESOLVE -- tcp.h reports both as
 *       D_NET_ERROR_HOST_UNREACHABLE -- and one that refuses or drops the
 *       connection D_SMTP_ERROR_CONNECT.
 *
 * @param[in,out] _client   a closed client.
 * @param[in]     _options  where and how to connect.
 * @post On failure the client is closed again; `_client->reply` explains a
 *       D_SMTP_ERROR_REJECTED.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for bad options, a mode
 *         that requires TLS without a client context, or a context that
 *         checks names with neither a server name nor a host to check;
 *         D_SMTP_ERROR_UNSUPPORTED when STARTTLS is required but not
 *         offered, without a TCP backend, or with no `helo_domain` and no
 *         identity d_smtp_client_identity() can find; D_SMTP_ERROR_RESOLVE;
 *         D_SMTP_ERROR_CONNECT; D_SMTP_ERROR_TIMED_OUT; any error of
 *         d_smtp_client_starttls(); or D_SMTP_ERROR_REJECTED.
 */
enum d_smtp_error d_smtp_client_connect(
                      struct d_smtp_client*               _client,
                      const struct d_smtp_client_options* _options);

/**
 * @brief Adopts a caller's connection; the greeting is the next step.
 *
 * @note It is d_smtp_client_attach_with() with the defaults and `_secure`:
 *       AUTH without TLS stays refused.
 *
 * @param[in,out] _client      a closed client.
 * @param[in]     _connection  the stream, with net.h's four required
 *                             operations; borrowed, so it must outlive the
 *                             session, and its owner closes it.
 * @param[in]     _secure      whether the stream is already encrypted.
 * @return D_SMTP_OK, D_SMTP_ERROR_INVALID_ARGUMENT, or D_SMTP_ERROR_STATE.
 */
enum d_smtp_error d_smtp_client_attach(struct d_smtp_client*    _client,
                                       struct d_net_connection* _connection,
                                       bool                     _secure);

/**
 * @brief Sets attach options to their defaults: a plaintext stream, no
 *        identity yet, and no AUTH without TLS.
 *
 * @param[out] _options  the options; NULL is ignored.
 */
void d_smtp_client_attach_options_init(
         struct d_smtp_client_attach_options* _options);

/**
 * @brief Adopts a caller's connection with the options that apply to one;
 *        the greeting is the next step.
 *
 * @param[in,out] _client      a closed client.
 * @param[in]     _connection  the stream, with net.h's four required
 *                             operations; borrowed, so it must outlive the
 *                             session, and its owner closes it.
 * @param[in]     _options     whether the stream is encrypted, the identity
 *                             hello announces when given NULL, and whether
 *                             AUTH may run without TLS.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a NULL argument, a
 *         connection without the four operations, or a malformed or overlong
 *         `helo_domain`, leaving the client as it was; or
 *         D_SMTP_ERROR_STATE for a client not closed.
 */
enum d_smtp_error d_smtp_client_attach_with(
                      struct d_smtp_client*                      _client,
                      struct d_net_connection*                   _connection,
                      const struct d_smtp_client_attach_options* _options);

/**
 * @brief Reads the server's greeting.
 *
 * @param[in,out] _client  a client in D_SMTP_CLIENT_GREETING.
 * @return D_SMTP_OK on 220; D_SMTP_ERROR_REJECTED otherwise; or a transport
 *         or protocol error.
 */
enum d_smtp_error d_smtp_client_greeting(struct d_smtp_client* _client);

/**
 * @brief Sends EHLO, falling back to HELO when the server does not know
 *        EHLO, and records the capabilities offered.
 *
 * @param[in,out] _client  a greeted client.
 * @param[in]     _domain  identity to announce, or NULL to reuse the last.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a malformed or missing
 *         identity; D_SMTP_ERROR_REJECTED; or a transport error.
 */
enum d_smtp_error d_smtp_client_hello(struct d_smtp_client* _client,
                                      const char*           _domain);

/**
 * @brief Upgrades the session with STARTTLS and repeats EHLO over TLS, as
 *        RFC 3207 section 4.2 requires.
 *
 * @note Bytes the server sent after its 220 would be read as if they had
 *       been protected; their presence -- already read, or waiting in a
 *       connection that can count them -- fails the upgrade with
 *       D_SMTP_ERROR_PROTOCOL (response injection) before any TLS.
 *
 * @param[in,out] _client       a client in D_SMTP_CLIENT_READY, not yet
 *                              secure.
 * @param[in]     _tls          a client context, borrowed for as long as
 *                              the session lasts.
 * @param[in]     _server_name  the name the server's certificate must
 *                              carry, sent as SNI; required when the
 *                              context checks names.
 * @post After the server's go-ahead, a failure to make TLS leaves the
 *       client in D_SMTP_CLIENT_QUIT: the server awaits a handshake, so no
 *       plaintext command can follow, and only d_smtp_client_close
 *       remains. Once TLS is made, EHLO fails as d_smtp_client_hello does.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a missing or server
 *         context, or a missing or malformed name; D_SMTP_ERROR_UNSUPPORTED
 *         when STARTTLS is not offered; D_SMTP_ERROR_PROTOCOL;
 *         D_SMTP_ERROR_REJECTED; D_SMTP_ERROR_TLS when the handshake or
 *         the check of the server failed, `tls_status` saying why;
 *         D_SMTP_ERROR_CLOSED when the server hung up during it;
 *         D_SMTP_ERROR_TIMED_OUT; D_SMTP_ERROR_OUT_OF_MEMORY; or
 *         D_SMTP_ERROR_TRANSPORT.
 */
enum d_smtp_error d_smtp_client_starttls(
                      struct d_smtp_client*       _client,
                      const struct d_ssl_context* _tls,
                      const char*                 _server_name);

/**
 * @brief Authenticates with the best mechanism both sides allow.
 *
 * @param[in,out] _client       a client in D_SMTP_CLIENT_READY.
 * @param[in]     _credentials  identity, secret and acceptable mechanisms.
 * @return D_SMTP_OK; D_SMTP_ERROR_INSECURE on a plaintext channel without
 *         `allow_insecure_auth`; D_SMTP_ERROR_UNSUPPORTED when no acceptable
 *         mechanism is offered; D_SMTP_ERROR_AUTH when the server refuses;
 *         D_SMTP_ERROR_LINE_TOO_LONG for oversized credentials; or a transport
 *         error.
 */
enum d_smtp_error d_smtp_client_authenticate(
                      struct d_smtp_client*            _client,
                      const struct d_smtp_credentials* _credentials);

// 3.3    Mail transactions
//------------------------------------------------------------------------------
/**
 * @brief Starts a transaction with MAIL FROM.
 *
 * @param[in,out] _client    a client in D_SMTP_CLIENT_READY.
 * @param[in]     _envelope  sender and extension flags; recipients unused.
 * @param[in]     _size      message size for SIZE=, or 0 to omit it.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a malformed sender;
 *         D_SMTP_ERROR_UNSUPPORTED for 8BITMIME or SMTPUTF8 not offered;
 *         D_SMTP_ERROR_LINE_TOO_LONG above the server's declared SIZE;
 *         D_SMTP_ERROR_REJECTED; or a transport error.
 */
enum d_smtp_error d_smtp_client_mail(struct d_smtp_client*         _client,
                                     const struct d_smtp_envelope* _envelope,
                                     size_t                        _size);

/**
 * @brief Adds one recipient with RCPT TO.
 *
 * @param[in,out] _client     a client in D_SMTP_CLIENT_MAIL.
 * @param[in]     _recipient  a mailbox.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT; D_SMTP_ERROR_REJECTED; or a
 *         transport error.
 */
enum d_smtp_error d_smtp_client_rcpt(struct d_smtp_client* _client,
                                     const char*           _recipient);

/**
 * @brief Sends the message with DATA, applying transparency and CRLF
 *        normalization, and ends the transaction.
 *
 * @param[in,out] _client   a client in D_SMTP_CLIENT_MAIL.
 * @param[in]     _message  the message, headers included.
 * @param[in]     _length   its length.
 * @return D_SMTP_OK; D_SMTP_ERROR_LINE_TOO_LONG for a line over 998 bytes;
 *         D_SMTP_ERROR_REJECTED; or a transport error.
 */
enum d_smtp_error d_smtp_client_data(struct d_smtp_client* _client,
                                     const char*           _message,
                                     size_t                _length);

/**
 * @brief Carries out a whole transaction: MAIL, every RCPT, DATA and the
 *        message -- batched in one write when the server offers PIPELINING.
 *
 * @note The message is delivered if at least one recipient is accepted;
 *       the report says which were refused. When no recipient is accepted,
 *       or MAIL or DATA is refused, the transaction is reset and the reply
 *       that refused it is left in `_client->reply`.
 *
 * @param[in,out] _client    a client in D_SMTP_CLIENT_READY.
 * @param[in]     _envelope  sender, recipients and extension flags.
 * @param[in]     _message   the message, headers included.
 * @param[in]     _length    its length.
 * @param[out]    _report    per-recipient results, or NULL.
 * @return D_SMTP_OK, or any error of d_smtp_client_mail(),
 *         d_smtp_client_rcpt() or d_smtp_client_data().
 */
enum d_smtp_error d_smtp_client_send(struct d_smtp_client*         _client,
                                     const struct d_smtp_envelope* _envelope,
                                     const char*                   _message,
                                     size_t                        _length,
                                     struct d_smtp_send_report*    _report);

// 3.4    Session control
//------------------------------------------------------------------------------
// RSET, NOOP and QUIT -- each returns D_SMTP_OK on a 2yz reply,
// D_SMTP_ERROR_REJECTED otherwise, D_SMTP_ERROR_STATE out of sequence, or a
// transport error. QUIT leaves the client in D_SMTP_CLIENT_QUIT either way.
enum d_smtp_error d_smtp_client_reset(struct d_smtp_client* _client);
enum d_smtp_error d_smtp_client_noop(struct d_smtp_client* _client);
enum d_smtp_error d_smtp_client_quit(struct d_smtp_client* _client);

// d_smtp_client_close -- ends the session without QUIT: the TLS session the
// client made sends close_notify and is released, and a connection
// connect() opened is closed, while an attached one is left to its owner.
// The last reply is kept. Idempotent; NULL is ignored.
void              d_smtp_client_close(struct d_smtp_client* _client);

// 3.5    Queries
//------------------------------------------------------------------------------
// d_smtp_client_tls_session -- the TLS session this client made, which tells
// what was negotiated and, after D_SMTP_ERROR_TLS, why it failed; NULL
// before one was made, after close, or for NULL
const struct d_ssl_session*
d_smtp_client_tls_session(const struct d_smtp_client* _client);

/**
 * @brief Appends this end's identity for EHLO: the machine's fully
 *        qualified name, or failing that an address literal of the
 *        connection's local address (RFC 5321 section 4.1.4).
 *
 * @param[in]     _connection  the session's connection.
 * @param[in,out] _buffer      receives the identity.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a NULL argument;
 *         D_SMTP_ERROR_UNSUPPORTED when the name is not fully qualified and
 *         the connection knows no local address; or
 *         D_SMTP_ERROR_LINE_TOO_LONG.
 */
enum d_smtp_error d_smtp_client_identity(
                      const struct d_net_connection* _connection,
                      struct d_smtp_buffer*          _buffer);


D_EXTERN_C_END


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "smtp_client.h: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_NET_SMTP_SMTP_CLIENT_H
