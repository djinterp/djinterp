/*******************************************************************************
* djinterp [net]                                                   smtp_client.h
*
* djinterp SMTP client.
*   A submission and relay client for RFC 5321 with the extensions a modern
* submission service expects: STARTTLS (RFC 3207) and implicit TLS (RFC
* 8314), SMTP AUTH (RFC 4954) with PLAIN, LOGIN, CRAM-MD5 and XOAUTH2,
* PIPELINING (RFC 2920), SIZE (RFC 1870), 8BITMIME (RFC 6152) and SMTPUTF8
* (RFC 6531).
*   The client drives any d_smtp_transport. d_smtp_client_connect() opens the
* built-in socket transport and runs the whole opening -- greeting, EHLO and
* the STARTTLS policy -- while d_smtp_client_attach() adopts a caller's
* transport and leaves those steps to the caller. Either way,
* d_smtp_client_send() carries out a whole mail transaction.
*   Defaults are strict: STARTTLS is required unless the caller opts down,
* certificates are verified, credentials never cross an unencrypted channel
* without an explicit opt-in, and every address and argument is validated, so
* CR, LF and NUL can never inject a command.
*
*
* path:      /inc/djinterp/net/smtp/smtp_client.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
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
*/

#ifndef DJINTERP_NET_SMTP_SMTP_CLIENT_H
#define DJINTERP_NET_SMTP_SMTP_CLIENT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"   // framework root
#include "./smtp_common.h"      // replies, capabilities, errors
#include "./smtp_transport.h"   // transport, reader, socket


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
// for the duration of the call.
struct d_smtp_client_options
{
    const char*               host;
    unsigned int              port;                 // 0: by security mode
    enum d_smtp_security      security;
    struct d_smtp_tls_options tls;                  // server_name: host
    const char*               helo_domain;          // NULL: this machine
    long                      connect_timeout_ms;
    long                      io_timeout_ms;
    bool                      allow_insecure_auth;  // AUTH without TLS
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
// A client is a state machine over one connection. It is large (it embeds
// its read buffer and last reply) and must not move while connected, since
// the built-in transport points into it.


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
    D_SMTP_CLIENT_QUIT         // QUIT sent; only close remains
};

// 3.1.2
// d_smtp_client
//   struct: one client session. `reply` always holds the last reply read, so
// a D_SMTP_ERROR_REJECTED can be explained; it survives d_smtp_client_close.
struct d_smtp_client
{
    struct d_smtp_reader       reader;
    struct d_smtp_socket       socket;               // when connect() opened
    struct d_smtp_capabilities capabilities;
    struct d_smtp_reply        reply;
    enum d_smtp_client_state   state;
    bool                       secure;               // channel encrypted
    bool                       allow_insecure_auth;
    bool                       authenticated;
    bool                       utf8;                 // SMTPUTF8 transaction
    char                       helo[D_SMTP_DOMAIN_MAX + 1u];
};

// 3.2    Session setup
//------------------------------------------------------------------------------
// initialization -- neither fails; NULL is ignored. Options default to
// STARTTLS with verification and the D_SMTP_CLIENT_*_TIMEOUT_MS limits.
void              d_smtp_client_options_init(
                      struct d_smtp_client_options* _options);
void              d_smtp_client_init(struct d_smtp_client* _client);

/**
 * @brief Connects, reads the greeting, says EHLO, and applies the STARTTLS
 *        policy, leaving the session ready for authentication or mail.
 *
 * @param[in,out] _client   a closed client.
 * @param[in]     _options  where and how to connect.
 * @post On failure the client is closed again; `_client->reply` explains a
 *       D_SMTP_ERROR_REJECTED.
 * @return D_SMTP_OK; D_SMTP_ERROR_UNSUPPORTED when STARTTLS is required but
 *         not offered; any error of d_smtp_socket_connect() or
 *         d_smtp_socket_start_tls(); or D_SMTP_ERROR_REJECTED.
 */
enum d_smtp_error d_smtp_client_connect(
                      struct d_smtp_client*               _client,
                      const struct d_smtp_client_options* _options);

/**
 * @brief Adopts a caller's transport; the greeting is the next step.
 *
 * @param[in,out] _client     a closed client.
 * @param[in]     _transport  copied; its `close`, if any, runs at close.
 * @param[in]     _secure     whether the transport is already encrypted.
 * @return D_SMTP_OK, D_SMTP_ERROR_INVALID, or D_SMTP_ERROR_STATE.
 */
enum d_smtp_error d_smtp_client_attach(
                      struct d_smtp_client*          _client,
                      const struct d_smtp_transport* _transport,
                      bool                           _secure);

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
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a malformed or missing
 *         identity; D_SMTP_ERROR_REJECTED; or a transport error.
 */
enum d_smtp_error d_smtp_client_hello(struct d_smtp_client* _client,
                                      const char*           _domain);

/**
 * @brief Upgrades the session with STARTTLS and repeats EHLO over TLS, as
 *        RFC 3207 section 4.2 requires.
 *
 * @note Bytes the server sent after its 220 would be read as if they had
 *       been protected; their presence fails the upgrade with
 *       D_SMTP_ERROR_PROTOCOL (response injection).
 *
 * @param[in,out] _client   a client in D_SMTP_CLIENT_READY, not yet secure.
 * @param[in]     _options  handshake options for the transport.
 * @return D_SMTP_OK; D_SMTP_ERROR_UNSUPPORTED when not offered or when the
 *         transport cannot upgrade; D_SMTP_ERROR_PROTOCOL;
 *         D_SMTP_ERROR_REJECTED; or the handshake's error.
 */
enum d_smtp_error d_smtp_client_starttls(
                      struct d_smtp_client*            _client,
                      const struct d_smtp_tls_options* _options);

/**
 * @brief Authenticates with the best mechanism both sides allow.
 *
 * @param[in,out] _client       a client in D_SMTP_CLIENT_READY.
 * @param[in]     _credentials  identity, secret and acceptable mechanisms.
 * @return D_SMTP_OK; D_SMTP_ERROR_INSECURE on a plaintext channel without
 *         `allow_insecure_auth`; D_SMTP_ERROR_UNSUPPORTED when no acceptable
 *         mechanism is offered; D_SMTP_ERROR_AUTH when the server refuses;
 *         D_SMTP_ERROR_TOO_LONG for oversized credentials; or a transport
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
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a malformed sender;
 *         D_SMTP_ERROR_UNSUPPORTED for 8BITMIME or SMTPUTF8 not offered;
 *         D_SMTP_ERROR_TOO_LONG above the server's declared SIZE;
 *         D_SMTP_ERROR_REJECTED; or a transport error.
 */
enum d_smtp_error d_smtp_client_mail(
                      struct d_smtp_client*         _client,
                      const struct d_smtp_envelope* _envelope,
                      size_t                        _size);

/**
 * @brief Adds one recipient with RCPT TO.
 *
 * @param[in,out] _client     a client in D_SMTP_CLIENT_MAIL.
 * @param[in]     _recipient  a mailbox.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID; D_SMTP_ERROR_REJECTED; or a
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
 * @return D_SMTP_OK; D_SMTP_ERROR_TOO_LONG for a line over 998 bytes;
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
enum d_smtp_error d_smtp_client_send(
                      struct d_smtp_client*          _client,
                      const struct d_smtp_envelope*  _envelope,
                      const char*                    _message,
                      size_t                         _length,
                      struct d_smtp_send_report*     _report);

// 3.4    Session control
//------------------------------------------------------------------------------
// RSET, NOOP and QUIT -- each returns D_SMTP_OK on a 2yz reply,
// D_SMTP_ERROR_REJECTED otherwise, D_SMTP_ERROR_STATE out of sequence, or a
// transport error. QUIT leaves the client in D_SMTP_CLIENT_QUIT either way.
enum d_smtp_error d_smtp_client_reset(struct d_smtp_client* _client);
enum d_smtp_error d_smtp_client_noop(struct d_smtp_client* _client);
enum d_smtp_error d_smtp_client_quit(struct d_smtp_client* _client);

// d_smtp_client_close -- releases the transport without QUIT; the last reply
// is kept. Idempotent; NULL is ignored.
void              d_smtp_client_close(struct d_smtp_client* _client);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SMTP_SMTP_CLIENT_H
