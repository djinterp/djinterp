/*******************************************************************************
* djinterp [c][net]                                                lmtp_client.h
*
* LMTP (RFC 2033): the client, which hands a message to a local delivery
* agent and learns, recipient by recipient, whether each mailbox took it.
*   An LMTP client is the SMTP kernel's client (net/smtp/smtp_client.h) in
* LMTP's dialect, which the kernel speaks: LHLO where EHLO is, and no HELO
* to fall back on; one final reply to the data for each recipient accepted,
* in the order of their RCPTs, where SMTP has one for all; and never port
* 25. Everything else - STARTTLS, AUTH, PIPELINING, SIZE, 8BITMIME,
* SMTPUTF8, the validation that keeps CR and LF off the wire - is SMTP's,
* and so are the types: options, envelopes, reports and errors.
*   LMTP runs between programs on one host, or one trusted network (RFC 2033
* section 5), and its servers - Dovecot's, Cyrus's - commonly listen on a
* unix-domain socket: with `local` set, the options' `host` is the socket's
* path. Over TCP the port is the caller's, since LMTP has none of its own.
* The defaults follow from where LMTP runs: plaintext, or STARTTLS when the
* server offers it and the options give a context to make it with.
*   The report's `deliveries` hold each recipient's own final reply. A server
* that hangs up partway leaves the rest at 0, which RFC 2033 section 5 has a
* client take as temporary failures, to be tried again.
*
* HOW TO USE
* ----------
*     struct dj_smtp_client_options o;
*     static struct dj_lmtp_client  client;
*     struct dj_smtp_recipient_status rcpt[2], delivery[2];
*     struct dj_smtp_send_report    r = { rcpt, 0, 0, delivery, 0 };
*     d_lmtp_client_options_init(&o);
*     o.host  = "/run/dovecot/lmtp";
*     o.local = true;
*     d_lmtp_client_init(&client);
*     d_lmtp_client_connect(&client, &o);
*     d_lmtp_client_send(&client, &envelope, message, length, &r);
*     d_smtp_client_quit(&client.smtp);
*     d_smtp_client_close(&client.smtp);
*
* CONFIGURATION
* -------------
*   None of its own: the SMTP kernel's, and through it tcp.h's and the TLS
* kernel's. Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type,
* it compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an
* #error (decision 101 of the register).
*
*
* path:      /inc/djinterp/c/modules/net/lmtp/lmtp_client.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Constants
         1.  D_LMTP_PORT_FORBIDDEN
    2.  Clients
         1.  dj_lmtp_client
2.  CLIENT
    ------
    1.  Setup
    2.  Opening a session
    3.  Mail transactions
*/

#ifndef DJINTERP_C_NET_LMTP_LMTP_CLIENT_H
#define DJINTERP_C_NET_LMTP_LMTP_CLIENT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../djinterp.h"  // framework root
#include "../../../../config/c/modules/net/cfg_net.h"  // D_INTERNAL_NET_*
#include "../net_common.h"          // dj_net_connection
#include "../smtp/smtp_client.h"    // the kernel's client
#include "../smtp/smtp_common.h"    // dj_smtp_error, dj_smtp_dialect

// 64-bit floor: as smtp_client.h's, whose client this one holds.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_LMTP_PORT_FORBIDDEN
//   constant: the one port LMTP is never spoken on, SMTP's relay port (RFC
// 2033 section 5). LMTP has no port of its own.
#define D_LMTP_PORT_FORBIDDEN  D_SMTP_PORT

// 1.2    Clients
//------------------------------------------------------------------------------
// 1.2.1
// dj_lmtp_client
//   struct: an LMTP session, the kernel's client in LMTP's dialect. Every
// step the two protocols share is the kernel's, on `smtp`:
// d_smtp_client_greeting, _hello (which says LHLO), _starttls (which says
// LHLO again), _authenticate, _mail, _rcpt, _reset, _noop, _quit and
// _close, and the queries. It is large, as the kernel's client is, and must
// not move while connected.
struct dj_lmtp_client
{
    struct dj_smtp_client smtp;  // the kernel's client
};


//==============================================================================
// 2.  CLIENT
//==============================================================================


// 2.1    Setup
//------------------------------------------------------------------------------
// initialization - neither fails; NULL is ignored. Options default to the
// kernel's but in three things: LMTP's dialect, no port, which a session
// over TCP must be given, and STARTTLS when offered, with a context, rather
// than STARTTLS required; a client is closed, in LMTP's dialect.
void d_lmtp_client_options_init(
    struct dj_smtp_client_options* _options
);
void d_lmtp_client_init(
    struct dj_lmtp_client* _client
);

// 2.2    Opening a session
//------------------------------------------------------------------------------
/**
 * @brief Connects, over TCP or a unix-domain socket, reads the greeting,
 *        says LHLO, and applies the STARTTLS policy, as
 *        d_smtp_client_connect() does, in LMTP's dialect whatever the
 *        options name.
 *
 * @param[in,out] _client   a closed client.
 * @param[in]     _options  where and how to connect.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a NULL argument, a
 *         session over TCP with no port or with port 25, or any options
 *         d_smtp_client_connect() refuses; D_SMTP_ERROR_REJECTED when the
 *         server refuses the greeting or LHLO, the reply in
 *         `_client->smtp.reply`; or any other error of
 *         d_smtp_client_connect().
 */
enum dj_smtp_error d_lmtp_client_connect(
    struct dj_lmtp_client*               _client,
    const struct dj_smtp_client_options* _options
);

/**
 * @brief Adopts a caller's connection for an LMTP session, as
 *        d_smtp_client_attach_with() does, in LMTP's dialect whatever the
 *        options name; d_smtp_client_greeting() and d_smtp_client_hello()
 *        on `_client->smtp` are the next steps.
 *
 * @param[in,out] _client      a closed client.
 * @param[in]     _connection  the stream, borrowed; its owner closes it.
 * @param[in]     _options     as d_smtp_client_attach_with() takes them;
 *                             NULL for d_smtp_client_attach_options_init()'s.
 * @return D_SMTP_OK, D_SMTP_ERROR_INVALID_ARGUMENT, or D_SMTP_ERROR_STATE.
 */
enum dj_smtp_error d_lmtp_client_attach(
    struct dj_lmtp_client*                      _client,
    struct dj_net_connection*                   _connection,
    const struct dj_smtp_client_attach_options* _options
);

// 2.3    Mail transactions
//------------------------------------------------------------------------------
/**
 * @brief Carries out a whole transaction, as d_smtp_client_send() does, and
 *        reports each recipient's delivery.
 *
 * @note The report's `recipients` keep each RCPT's reply and `deliveries`
 *       each recipient's final reply; a recipient refused at RCPT, or whose
 *       final reply never came, has a delivery code of 0.
 *
 * @param[in,out] _client    a client ready for mail.
 * @param[in]     _envelope  sender, recipients and extension flags.
 * @param[in]     _message   the message, headers included.
 * @param[in]     _length    its length.
 * @param[out]    _report    per-recipient results, or NULL.
 * @return D_SMTP_OK when the message reached at least one mailbox;
 *         D_SMTP_ERROR_REJECTED when it reached none, the last refusal in
 *         `_client->smtp.reply`; D_SMTP_ERROR_STATE for a client not in
 *         LMTP's dialect or not ready; or any error of
 *         d_smtp_client_send().
 */
enum dj_smtp_error d_lmtp_client_send(
    struct dj_lmtp_client*         _client,
    const struct dj_smtp_envelope* _envelope,
    const char*                    _message,
    size_t                         _length,
    struct dj_smtp_send_report*    _report
);

/**
 * @brief Sends the message of a transaction the caller opened with
 *        d_smtp_client_mail() and d_smtp_client_rcpt(), and keeps each
 *        recipient's final reply, in the order of the RCPTs accepted.
 *
 * @param[in,out] _client    a client with a transaction open.
 * @param[in]     _message   the message, headers included.
 * @param[in]     _length    its length.
 * @param[out]    _replies   one entry per recipient accepted, or NULL.
 * @param[in]     _capacity  entries `_replies` holds.
 * @param[out]    _count     how many final replies came, or NULL.
 * @return as d_smtp_client_data_replies(), and D_SMTP_ERROR_STATE for a
 *         client not in LMTP's dialect.
 */
enum dj_smtp_error d_lmtp_client_data(
    struct dj_lmtp_client*           _client,
    const char*                      _message,
    size_t                           _length,
    struct dj_smtp_recipient_status* _replies,
    size_t                           _capacity,
    size_t*                          _count
);


D_EXTERN_C_END


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "lmtp_client.h: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_C_NET_LMTP_LMTP_CLIENT_H
