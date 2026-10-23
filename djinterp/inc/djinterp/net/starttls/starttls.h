/*******************************************************************************
* djinterp [net]                                                      starttls.h
*
* STARTTLS common module: the in-band TLS upgrade every STARTTLS protocol
* shares.
*   A STARTTLS protocol opens in plaintext, asks the peer to switch, and on the
* peer's go-ahead runs a TLS handshake over the same connection. The wire
* syntax differs per protocol -- SMTP's STARTTLS and 220, POP3's STLS, FTP's
* AUTH TLS, XMPP's <starttls/> and <proceed/>, PostgreSQL's SSLRequest -- but
* the part that decides whether the result is secure does not, and it lives
* here once:
*     - the upgrade policy, so a stripped or refused offer fails closed
*       when TLS is required instead of continuing in plaintext        [1][5]
*     - the upgrade itself, which refuses plaintext still waiting at the
*       switch (the injection class of CVE-2011-0411), binds
*       verification to the host name the caller configured, and marks
*       what was learned before it stale                                  [5]
*     - bounded, CRLF-strict line I/O over plaintext and TLS alike, and a
*       command writer that refuses embedded line breaks                  [3]
*   The stream is any net.h connection: a TCP connection from tcp.h, or
* net_memory.h's pipe in a test. TLS is the SSL kernel's: the upgrade makes a
* d_ssl_session over that same connection from the d_ssl_context the caller
* configured, so every engine the kernel has -- the OpenSSL family's among
* them -- serves here, and the kernel's checks of names, pins, and versions
* hold for every upgrade.                                                 [1]
*   A protocol module derives from this one by embedding a d_starttls_session
* as its first member and supplying a d_starttls_negotiator: two callbacks
* that send its upgrade request and classify the reply. Implicit TLS (SMTPS,
* IMAPS) needs no negotiator: initialize, then upgrade at once.
*   Every call blocks, and a session belongs to one thread at a time.
* Deadlines come from the connection: a TCP connection's I/O timeout turns a
* stalled peer into D_STARTTLS_ERROR_TIMED_OUT.
*   Requires cfg_starttls.h. Link net.c, ssl.c, the codec's line buffer, and
* the engine the caller's context is made with.
*
*
* path:      /inc/djinterp/net/starttls/starttls.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Outcomes and modes
         1.  d_starttls_error
         2.  d_starttls_policy
         3.  d_starttls_role
         4.  d_starttls_state
         5.  d_starttls_reply
    2.  Session
         1.  fn_starttls_on_upgrade
         2.  d_starttls_options
         3.  d_starttls_session
    3.  Negotiation
         1.  fn_starttls_request
         2.  fn_starttls_reply
         3.  d_starttls_negotiator
    4.  Limits
         1.  D_STARTTLS_BUFFER_MIN
2.  SESSION LIFECYCLE
    -----------------
    1.  Options
    2.  Initialization and teardown
3.  I/O
    ---
    1.  Bytes
    2.  Lines
4.  UPGRADE
    -------
    1.  Negotiation
    2.  Upgrade
5.  QUERIES
    -------
    1.  State
    2.  Names
    3.  Mapping
*/
#ifndef DJINTERP_NET_STARTTLS_STARTTLS_H
#define DJINTERP_NET_STARTTLS_STARTTLS_H 1

// std
#include <stdbool.h>                                  // bool
#include <stddef.h>                                   // size_t
// djinterp
#include "../../c/djinterp.h"                         // framework root
#include "../../config/net/starttls/cfg_starttls.h"  // D_INTERNAL_STARTTLS_*
#include "../../env/c/env_vendor_attributes.h"       // D_FORMAT_PRINTF
#include "../codec/codec_line.h"                     // d_net_line_buffer
#include "../net_common.h"                           // d_net_connection
#include "../ssl/ssl.h"                              // d_ssl_context


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Outcomes and modes
//------------------------------------------------------------------------------
// 1.1.1
// d_starttls_error
//   enum: the outcome of every fallible call in this module. An error marked
// FATAL moves the session to D_STARTTLS_STATE_FAILED -- the call that reports
// it has already done so -- after which only d_starttls_close and
// d_starttls_destroy are accepted. The others leave the session as it was.
enum d_starttls_error
{
    // the operation completed
    D_STARTTLS_OK                     = 0,

    // a required pointer was NULL or a value was out of range; nothing was
    // sent or received
    D_STARTTLS_ERROR_INVALID_ARGUMENT = 1,

    // the call is not valid in the session's current state, such as an
    // upgrade of a session that is already secure
    D_STARTTLS_ERROR_STATE            = 2,

    // an allocation failed. FATAL during an upgrade, where the peer is already
    // waiting for a handshake
    D_STARTTLS_ERROR_OUT_OF_MEMORY    = 3,

    // the connection failed, beneath TLS or without it; the session keeps
    // the connection's own error in `net_error`. FATAL
    D_STARTTLS_ERROR_TRANSPORT        = 4,

    // the stream ended: in plaintext, at its end or with the connection
    // lost; under TLS, with the peer's close_notify
    D_STARTTLS_ERROR_CLOSED           = 5,

    // the connection ended without a TLS close_notify, so an attacker may
    // have cut the data short. FATAL
    D_STARTTLS_ERROR_TRUNCATED        = 6,

    // the connection's deadline expired; the position in the stream is
    // unknown. FATAL
    D_STARTTLS_ERROR_TIMED_OUT        = 7,

    // a line did not fit the session's buffer. FATAL when receiving; when
    // sending, nothing was written
    D_STARTTLS_ERROR_LINE_TOO_LONG    = 8,

    // the peer sent malformed input: a bare CR or LF, a NUL inside a line, a
    // stream that ended inside a line, or a reply the negotiator could not
    // classify. FATAL
    D_STARTTLS_ERROR_PROTOCOL         = 9,

    // TLS is required and the peer did not offer it -- what a STARTTLS
    // stripping attack produces. FATAL
    D_STARTTLS_ERROR_NOT_OFFERED      = 10,

    // TLS is required and the peer refused the request. FATAL
    D_STARTTLS_ERROR_REJECTED         = 11,

    // plaintext was waiting at the upgrade point, which would otherwise have
    // been read as if it had arrived under TLS. FATAL
    D_STARTTLS_ERROR_INJECTION        = 12,

    // no TLS context was given, and the policy allows TLS. FATAL during an
    // upgrade
    D_STARTTLS_ERROR_NO_BACKEND       = 13,

    // no TLS session could be made from the context: a server name that is
    // neither a host name nor an address, or one the engine refused. FATAL
    // during an upgrade
    D_STARTTLS_ERROR_CONFIG           = 14,

    // the TLS handshake failed for a reason other than verification: the
    // peer is not speaking TLS, broke the protocol, or hung up. FATAL
    D_STARTTLS_ERROR_HANDSHAKE        = 15,

    // the peer's certificate failed verification: untrusted, expired, not
    // issued for the configured host name, or not matching a pin. FATAL
    D_STARTTLS_ERROR_VERIFY           = 16
};

// 1.1.2
// d_starttls_policy
//   enum: what a session does when TLS cannot be had. The values equal the
// D_CFG_STARTTLS_POLICY_* sentinels, and 0 is deliberately none of them, so
// zero-initialized options are rejected rather than read as DISABLED.
enum d_starttls_policy
{
    // never upgrade: the session stays plaintext by the caller's explicit
    // choice, and d_starttls_negotiate does nothing
    D_STARTTLS_POLICY_DISABLED      = 1,

    // upgrade when the peer offers it; continue in plaintext when it does
    // not, or refuses. Protects against passive eavesdropping only: an
    // active attacker can remove the offer
    D_STARTTLS_POLICY_OPPORTUNISTIC = 2,

    // upgrade or fail. The only policy that holds against an active attacker
    D_STARTTLS_POLICY_REQUIRED      = 3
};

// 1.1.3
// d_starttls_role
//   enum: the side of the exchange a session plays.
enum d_starttls_role
{
    // requests the upgrade and verifies the server
    D_STARTTLS_ROLE_CLIENT = 0,

    // grants the upgrade and presents a certificate
    D_STARTTLS_ROLE_SERVER = 1
};

// 1.1.4
// d_starttls_state
//   enum: where a session is in its life.
enum d_starttls_state
{
    // not yet upgraded; I/O is plaintext
    D_STARTTLS_STATE_PLAINTEXT = 0,

    // upgraded; every byte in either direction passes through TLS
    D_STARTTLS_STATE_SECURE    = 1,

    // a fatal error occurred; the connection must not be reused
    D_STARTTLS_STATE_FAILED    = 2,

    // d_starttls_close has run
    D_STARTTLS_STATE_CLOSED    = 3
};

// 1.1.5
// d_starttls_reply
//   enum: a negotiator's classification of the peer's answer to an upgrade
// request.
enum d_starttls_reply
{
    // the peer agreed; the handshake starts now
    D_STARTTLS_REPLY_PROCEED   = 0,

    // the peer declined in a well-formed way, such as SMTP's 454 or IMAP's NO
    D_STARTTLS_REPLY_REFUSED   = 1,

    // the answer could not be understood; the exchange is out of step
    D_STARTTLS_REPLY_MALFORMED = 2
};

// 1.2    Session
//------------------------------------------------------------------------------
// 1.2.1
// fn_starttls_on_upgrade
//   typedef: called once, after a successful upgrade and before the upgrade
// returns. Its job is to forget: every STARTTLS specification requires that
// what was learned from the peer before the handshake -- its greeting, its
// capability or extension list -- be discarded, because an attacker could
// have written it.
struct d_starttls_session;

typedef void (*fn_starttls_on_upgrade)(struct d_starttls_session* _session,
                                       void*                      _context);

// 1.2.2
// d_starttls_options
//   struct: how a session behaves. Obtain one from d_starttls_options_default
// and change what differs; d_starttls_init reads every field once and keeps
// only `tls`, which must outlive the session.
//   `tls` is the SSL kernel context the upgrade makes its TLS session from --
// its engine, trust, identity, and version floor -- for the session's own
// role. `server_name` is the name a client sends as SNI and requires the
// server's certificate to carry; it comes from the caller's configuration,
// never from the peer, and a server ignores it. `buffer_bytes` sizes the
// receive buffer, and so bounds the longest line the session accepts,
// terminator included. The session borrows its connection unless
// `owns_connection` is set: then closing the session closes the connection,
// and destroying the session destroys it.
struct d_starttls_options
{
    enum d_starttls_role        role;                // the side to play
    enum d_starttls_policy      policy;              // when TLS is mandatory
    const struct d_ssl_context* tls;                 // TLS context; not owned
    const char*                 server_name;         // client: expected peer
    size_t                      buffer_bytes;        // receive buffer size
    bool                        owns_connection;     // closed with the session
    bool                        reject_leftover;     // fail, not discard
    bool                        strict_crlf;         // bare LF is an error
    fn_starttls_on_upgrade      on_upgrade;          // forget pre-TLS state
    void*                       on_upgrade_context;  // passed to on_upgrade
};

// 1.2.3
// d_starttls_session
//   struct: one connection's STARTTLS state. The caller owns the storage --
// usually as the first member of a protocol module's own session -- and
// treats every field as read-only: the functions below keep the buffer
// indices and the state consistent with each other.
//   `connection` carries the plaintext exchange and then the ciphertext:
// from the upgrade on, `ssl` is the TLS session over it, which every byte
// passes through. `net_error` is what the connection reported with the last
// failure it caused, D_NET_OK until one did: the cause behind
// D_STARTTLS_ERROR_TRANSPORT, and behind a timeout, an end, or a truncation
// that the connection's own error brought about -- a reset, say. `epoch`
// counts completed upgrades, so a protocol module can tell whether what it
// cached came from before the handshake. `detail` is a static description
// of the TLS failure that ended the session, or NULL.
struct d_starttls_session
{
    struct d_net_connection*    connection;          // the stream
    const struct d_ssl_context* tls;                 // TLS context; not owned
    struct d_ssl_session*       ssl;                 // TLS, once upgraded
    enum d_net_error            net_error;           // the connection's cause
    enum d_starttls_role        role;                // the side it plays
    enum d_starttls_policy      policy;              // when TLS is mandatory
    enum d_starttls_state       state;               // where it is
    bool                        owns_connection;     // closed with the session
    bool                        reject_leftover;     // fail, not discard
    bool                        strict_crlf;         // bare LF is an error
    char*                       server_name;         // owned copy, or NULL
    char*                       buffer;              // received bytes
    struct d_net_line_buffer    lines;               // their offsets
    size_t                      discarded;           // dropped at upgrade
    unsigned int                epoch;               // upgrades completed
    const char*                 detail;              // TLS failure, or NULL
    fn_starttls_on_upgrade      on_upgrade;          // forget pre-TLS state
    void*                       on_upgrade_context;  // passed to on_upgrade
};

// 1.3    Negotiation
//------------------------------------------------------------------------------
// 1.3.1
// fn_starttls_request
//   typedef: sends a protocol's upgrade request -- SMTP's STARTTLS, POP3's
// STLS, FTP's AUTH TLS, XMPP's <starttls/> -- usually with a single
// d_starttls_send_line or d_starttls_send.
typedef enum d_starttls_error  (*fn_starttls_request)(
    struct d_starttls_session* _session,
    void*                      _context
);

// 1.3.2
// fn_starttls_reply
//   typedef: reads the peer's answer to the request and classifies it into
// *_reply. It returns D_STARTTLS_OK once *_reply is set, or the error that
// stopped the read. It must consume exactly the answer: anything read beyond
// it is plaintext left over at the upgrade point.
typedef enum d_starttls_error  (*fn_starttls_reply)(
    struct d_starttls_session* _session,
    void*                      _context,
    enum d_starttls_reply*     _reply
);

// 1.3.3
// d_starttls_negotiator
//   struct: the protocol-specific half of an upgrade, as a pair of callbacks.
struct d_starttls_negotiator
{
    const char*         name;     // the protocol, such as "SMTP"
    fn_starttls_request request;  // sends the upgrade request
    fn_starttls_reply   reply;    // reads and classifies the answer
};

// 1.4    Limits
//------------------------------------------------------------------------------
// 1.4.1
// D_STARTTLS_BUFFER_MIN
//   constant: the smallest receive buffer a session accepts, in bytes: SMTP's
// reply-line limit (RFC 5321, section 4.5.3.1.5), the shortest line limit
// any STARTTLS protocol sets.
#define D_STARTTLS_BUFFER_MIN 512


//==============================================================================
// 2.  SESSION LIFECYCLE
//==============================================================================


// 2.1    Options
//------------------------------------------------------------------------------
/**
 * @brief Returns the default options for a session in `_role`.
 *
 * @note the defaults are the secure ones: the policy is
 *       D_CFG_STARTTLS_POLICY (`REQUIRED` unless configured otherwise),
 *       plaintext left over at the upgrade fails it, and lines must end in
 *       CRLF. The connection is borrowed. `tls` and `server_name` are `NULL`
 *       and must be supplied.
 *
 * @param[in] _role  the side the session will play.
 * @return the options.
 */
struct d_starttls_options
d_starttls_options_default(enum d_starttls_role _role);

// 2.2    Initialization and teardown
//------------------------------------------------------------------------------
/**
 * @brief Initializes a session over a connection.
 *
 * @param[out] _session     the session to initialize.
 * @param[in]  _options     the session's options.
 * @param[in]  _connection  the plaintext stream, with net.h's four required
 *                          operations. It must outlive the session, which
 *                          borrows it unless the options say it owns it.
 * @pre    `_session` is uninitialized or destroyed.
 * @post   on success the session is `D_STARTTLS_STATE_PLAINTEXT`, and the
 *         caller releases it with d_starttls_destroy().
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_INVALID_ARGUMENT` for a `NULL`
 *         pointer, an incomplete connection, an out-of-range option, a
 *         buffer smaller than D_STARTTLS_BUFFER_MIN, a context for the other
 *         role, or a client whose context checks host names given no server
 *         name; `D_STARTTLS_ERROR_NO_BACKEND` if the policy allows TLS but no
 *         initialized context is given; or `D_STARTTLS_ERROR_OUT_OF_MEMORY`.
 */
D_NODISCARD enum d_starttls_error
d_starttls_init(struct d_starttls_session*       _session,
                const struct d_starttls_options* _options,
                struct d_net_connection*         _connection);
/**
 * @brief Ends a session: sends close_notify if it is secure, then closes the
 *        connection if the session owns it.
 *
 * @param[in,out] _session  the session to close.
 * @post   the session is `D_STARTTLS_STATE_CLOSED`, and an owned connection
 *         is closed even when close_notify could not be sent. A borrowed one
 *         is left open for the caller, who may go on with it in plaintext.
 * @return the outcome of sending close_notify (`D_STARTTLS_OK` when there was
 *         nothing to send), or `D_STARTTLS_ERROR_INVALID_ARGUMENT` if
 *         `_session` is `NULL`.
 */
enum d_starttls_error
d_starttls_close(struct d_starttls_session* _session);
/**
 * @brief Releases a session and its TLS session, and destroys its
 *        connection if it owns it.
 *
 * @note sends nothing: a secure session destroyed without d_starttls_close()
 *       ends without close_notify, which the peer sees as a truncation.
 *
 * @param[in,out] _session  the session; may be `NULL`.
 * @post   every line obtained from the session is invalid, and the session
 *         reads as closed. An owned connection has gone through
 *         d_net_connection_destroy; a borrowed one is as it was.
 */
void
d_starttls_destroy(struct d_starttls_session* _session);


//==============================================================================
// 3.  I/O
//==============================================================================


// 3.1    Bytes
//------------------------------------------------------------------------------
/**
 * @brief Sends every byte of a buffer, through TLS once the session is secure.
 *
 * @param[in,out] _session  an open session.
 * @param[in]     _data     the bytes to send; may be `NULL` if `_size` is `0`.
 * @param[in]     _size     the number of bytes.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_INVALID_ARGUMENT`;
 *         `D_STARTTLS_ERROR_STATE` unless the session is plaintext or secure;
 *         or the fatal error that stopped the write.
 */
D_NODISCARD enum d_starttls_error
d_starttls_send(struct d_starttls_session* _session,
                const void*                _data,
                size_t                     _size);
/**
 * @brief Receives at least one byte, taking bytes already buffered first.
 *
 * @param[in,out] _session   an open session.
 * @param[out]    _buffer    receives the bytes.
 * @param[in]     _capacity  the size of `_buffer`; at least `1`.
 * @param[out]    _received  receives the number of bytes stored.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_CLOSED` at an orderly end of
 *         stream; `D_STARTTLS_ERROR_INVALID_ARGUMENT`;
 *         `D_STARTTLS_ERROR_STATE`; or a fatal error.
 */
D_NODISCARD enum d_starttls_error
d_starttls_recv(struct d_starttls_session* _session,
                void*                      _buffer,
                size_t                     _capacity,
                size_t*                    _received);
/**
 * @brief Receives exactly `_size` bytes, such as the payload of an IMAP
 *        literal whose length the protocol announced.
 *
 * @param[in,out] _session  an open session.
 * @param[out]    _buffer   receives the bytes.
 * @param[in]     _size     the number of bytes.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_CLOSED` if the stream ended before
 *         the first byte; `D_STARTTLS_ERROR_PROTOCOL` (fatal) if it ended
 *         part-way; `D_STARTTLS_ERROR_INVALID_ARGUMENT`;
 *         `D_STARTTLS_ERROR_STATE`; or a fatal error.
 */
D_NODISCARD enum d_starttls_error
d_starttls_recv_exact(struct d_starttls_session* _session,
                      void*                      _buffer,
                      size_t                     _size);
/**
 * @brief Returns the number of received bytes not yet consumed.
 *
 * @param[in] _session  the session.
 * @return the count; `0` if `_session` is `NULL`.
 */
size_t
d_starttls_buffered(const struct d_starttls_session* _session);

// 3.2    Lines
//------------------------------------------------------------------------------
/**
 * @brief Sends one command line, appending CRLF.
 *
 * @note refuses a line that contains CR or LF: a line break inside a command
 *       starts a second command, which is how an unchecked argument injects
 *       one.
 *
 * @param[in,out] _session  an open session.
 * @param[in]     _line     the line, without its terminator.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_INVALID_ARGUMENT` for a `NULL`
 *         pointer or a line containing CR or LF;
 *         `D_STARTTLS_ERROR_LINE_TOO_LONG` if the line and its CRLF exceed the
 *         session's buffer; neither error sends anything. Otherwise
 *         `D_STARTTLS_ERROR_STATE`, `D_STARTTLS_ERROR_OUT_OF_MEMORY`, or a
 *         fatal error.
 */
D_NODISCARD enum d_starttls_error
d_starttls_send_line(struct d_starttls_session* _session,
                     const char*                _line);
/**
 * @brief Formats one command line printf-style and sends it with CRLF.
 *
 * @note checks the formatted result, not the format, for CR, LF, and NUL:
 *       the injection it prevents arrives through an argument.
 *
 * @param[in,out] _session  an open session.
 * @param[in]     _format   the printf-style format, without a terminator.
 * @param[in]     ...       the format arguments.
 * @return as d_starttls_send_line(), with `D_STARTTLS_ERROR_INVALID_ARGUMENT`
 *         also for a formatting error or a NUL in the result.
 */
D_NODISCARD enum d_starttls_error
d_starttls_send_linef(struct d_starttls_session* _session,
                      const char*                _format,
                      ...) D_FORMAT_PRINTF(2, 3);
/**
 * @brief Receives one line and returns it without its terminator.
 *
 * @param[in,out] _session  an open session.
 * @param[out]    _line     receives the NUL-terminated line, which lives in
 *                          the session's buffer.
 * @param[out]    _length   receives the line's length.
 * @post   `*_line` stays valid until the next call that receives from, or
 *         releases, the session.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_CLOSED` if the stream ended
 *         between lines; `D_STARTTLS_ERROR_LINE_TOO_LONG` (fatal) if no line
 *         end arrived within the buffer; `D_STARTTLS_ERROR_PROTOCOL` (fatal)
 *         for a bare CR, a bare LF when strict_crlf is set, a NUL, or a
 *         stream that ended inside a line; `D_STARTTLS_ERROR_INVALID_ARGUMENT`;
 *         `D_STARTTLS_ERROR_STATE`; or a fatal error.
 */
D_NODISCARD enum d_starttls_error
d_starttls_recv_line(struct d_starttls_session* _session,
                     const char**               _line,
                     size_t*                    _length);


//==============================================================================
// 4.  UPGRADE
//==============================================================================


// 4.1    Negotiation
//------------------------------------------------------------------------------
/**
 * @brief Runs the client side of a STARTTLS exchange under the session's
 *        policy.
 *
 * @note `_offered` must reflect what the peer advertised in this session --
 *       its EHLO keywords, its CAPABILITY list -- and an attacker controls
 *       that list; the policy is what makes a missing offer safe.
 *
 * @param[in,out] _session     a plaintext client session.
 * @param[in]     _negotiator  the protocol's request and reply callbacks.
 * @param[in]     _context     passed to both callbacks.
 * @param[in]     _offered     whether the peer advertised the upgrade.
 * @post   on `D_STARTTLS_OK` the session is secure, or plaintext because its
 *         policy permitted that; d_starttls_is_secure() tells which.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_NOT_OFFERED` or
 *         `D_STARTTLS_ERROR_REJECTED` (fatal) under `REQUIRED`;
 *         `D_STARTTLS_ERROR_PROTOCOL` (fatal) for an unclassifiable reply; an
 *         error from a callback (fatal) or from d_starttls_upgrade();
 *         `D_STARTTLS_ERROR_INVALID_ARGUMENT`; or `D_STARTTLS_ERROR_STATE` for
 *         a server or a session that is not plaintext.
 */
D_NODISCARD enum d_starttls_error
d_starttls_negotiate(struct d_starttls_session*          _session,
                     const struct d_starttls_negotiator* _negotiator,
                     void*                               _context,
                     bool                                _offered);

// 4.2    Upgrade
//------------------------------------------------------------------------------
/**
 * @brief Switches the session to TLS: the step each side takes once the
 *        switch is agreed.
 *
 * @note a client calls it after reading the go-ahead, a server after sending
 *       it; for implicit TLS it is the first call after d_starttls_init().
 *       Plaintext still waiting at this point is refused, or dropped when
 *       reject_leftover is `false`, and is never read as TLS data.
 *
 * @param[in,out] _session  a plaintext session whose policy is not
 *                          `DISABLED`.
 * @post   on success the session is secure, its epoch has advanced, and its
 *         on_upgrade callback has run.
 * @return `D_STARTTLS_OK`; `D_STARTTLS_ERROR_INJECTION`,
 *         `D_STARTTLS_ERROR_VERIFY`, `D_STARTTLS_ERROR_HANDSHAKE`, or another
 *         fatal error; `D_STARTTLS_ERROR_INVALID_ARGUMENT`; or
 *         `D_STARTTLS_ERROR_STATE`.
 */
D_NODISCARD enum d_starttls_error
d_starttls_upgrade(struct d_starttls_session* _session);


//==============================================================================
// 5.  QUERIES
//==============================================================================


// 5.1    State
//------------------------------------------------------------------------------
/**
 * @brief Returns where the session is in its life.
 *
 * @param[in] _session  the session.
 * @return its state; `D_STARTTLS_STATE_CLOSED` if `_session` is `NULL`.
 */
enum d_starttls_state
d_starttls_get_state(const struct d_starttls_session* _session);
/**
 * @brief Returns whether the session has been upgraded and is still open.
 *
 * @param[in] _session  the session.
 * @return `true` if the session is `D_STARTTLS_STATE_SECURE`.
 */
bool
d_starttls_is_secure(const struct d_starttls_session* _session);
/**
 * @brief Returns whether the session may carry data under its policy: it is
 *        secure, or it is plaintext and its policy does not require TLS.
 *
 * @param[in] _session  the session.
 * @return `true` if the session is open and its policy is met.
 */
bool
d_starttls_is_satisfied(const struct d_starttls_session* _session);
/**
 * @brief Returns the number of upgrades the session has completed.
 *
 * @param[in] _session  the session.
 * @return the epoch; `0` if `_session` is `NULL`.
 */
unsigned int
d_starttls_epoch(const struct d_starttls_session* _session);
/**
 * @brief Returns a description of the TLS failure that ended the session.
 *
 * @param[in] _session  the session.
 * @return static text, or `NULL` if there is none.
 */
const char*
d_starttls_detail(const struct d_starttls_session* _session);
/**
 * @brief Returns what the connection reported with the last failure it
 *        caused: the cause behind `D_STARTTLS_ERROR_TRANSPORT`, and behind
 *        a timeout, an end, or a truncation the connection's own error
 *        brought about.
 *
 * @param[in] _session  the session.
 * @return the connection's error; `D_NET_OK` until one failed, or if
 *         `_session` is `NULL`.
 */
enum d_net_error
d_starttls_net_error(const struct d_starttls_session* _session);
/**
 * @brief Returns the session's TLS session, which tells what the upgrade
 *        negotiated and why TLS failed: d_ssl_session_info,
 *        d_ssl_session_failure, and d_ssl_session_transport_error.
 *
 * @param[in] _session  the session.
 * @return the TLS session from the upgrade on; `NULL` before it, or if
 *         `_session` is `NULL`.
 */
const struct d_ssl_session*
d_starttls_tls_session(const struct d_starttls_session* _session);

// 5.2    Names
//------------------------------------------------------------------------------
/**
 * @brief Returns a short description of an error.
 *
 * @param[in] _error  the error.
 * @return static text; "unknown status" for a value outside the enumeration.
 */
const char*
d_starttls_error_string(enum d_starttls_error _error);
/**
 * @brief Returns the name of a state.
 *
 * @param[in] _state  the state.
 * @return static text; "unknown" for a value outside the enumeration.
 */
const char*
d_starttls_state_string(enum d_starttls_state _state);
/**
 * @brief Returns the name of a policy.
 *
 * @param[in] _policy  the policy.
 * @return static text; "unknown" for a value outside the enumeration.
 */
const char*
d_starttls_policy_string(enum d_starttls_policy _policy);

// 5.3    Mapping
//------------------------------------------------------------------------------
/**
 * @brief Maps an error onto d_net_error, by the rules net.h gives with it.
 *
 * @param[in] _error  the error.
 * @return the d_net_error it maps to; D_NET_ERROR_UNKNOWN for a value
 *         outside the enumeration.
 */
enum d_net_error
d_starttls_error_to_net(enum d_starttls_error _error);
/**
 * @brief Gives the error a connection's d_net_error is here, by the rules
 *        net.h gives the other way.
 *
 * @note D_NET_OK is D_STARTTLS_OK; INVALID_ARGUMENT, OUT_OF_MEMORY,
 *       TIMED_OUT, and PROTOCOL are their namesakes; a stream that ended or
 *       a connection lost -- CLOSED, CONNECTION_RESET, CONNECTION_ABORTED --
 *       is CLOSED; anything else is TRANSPORT. It is how the plaintext
 *       connection's errors are judged. Under TLS the SSL kernel judges
 *       them, and a connection that ends without close_notify is TRUNCATED.
 *
 * @param[in] _error  the connection's error.
 * @return the module's error; `D_STARTTLS_ERROR_TRANSPORT` for a value
 *         outside the enumeration.
 */
enum d_starttls_error
d_starttls_error_from_net(enum d_net_error _error);


D_EXTERN_C_END


#endif  // DJINTERP_NET_STARTTLS_STARTTLS_H
