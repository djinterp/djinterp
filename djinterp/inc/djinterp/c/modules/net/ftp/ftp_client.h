/*******************************************************************************
* djinterp [c][net]                                                 ftp_client.h
*
* An FTP client, plain, or secured by a layer it is given: FTPS (RFC 4217)
* with module ftps.
*   The client speaks the control protocol through the net/ftp/ vocabulary and
* moves bytes through the TCP transport (net/tcp/tcp.h), reading and writing
* every connection through net.h's dj_net_connection, so the client itself
* holds no OS code. It speaks RFC 2228's security commands too - AUTH, PBSZ,
* PROT, CCC - and runs the mechanism they agree on through a security layer
* (2.2): explicitly, upgrading the control connection after AUTH, or
* implicitly, from the first byte on port 990; and, after PBSZ 0 and PROT P,
* on every data connection, where the client is always the client of the
* mechanism (RFC 4217 section 7) in passive and active mode alike. Module
* ftps gives it TLS through the TLS kernel (net/ftps/ftps.h). Without a
* layer the client is plain FTP, links no TLS, and refuses options that ask
* for security.
*   Security settings are never weakened silently: a layer that checks less
* than the options ask is refused before connecting; with require_security
* set, a server that refuses AUTH ends the session before any credential is
* sent, and one that refuses PBSZ or PROT P fails the login; plaintext
* received after the server agreed to AUTH, which an attacker could have
* injected, aborts the upgrade; and a secured data stream that stops without
* its close is reported as truncated, not complete.
*   Every call blocks. The options bound each connection attempt by
* connect_timeout_ms, each read and write on the control connection by
* response_timeout_ms, and each on a data connection by idle_timeout_ms; an
* active-mode transfer waits for the server's connection without limit.
* A client holds its buffers, so it is large: give it static or heap
* storage, and do not copy it once initialized. Buffer sizes come from
* cfg_ftp.h.
*
*   Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type, it
* compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error
* (decision 101 of the register).
*
*
* path:      /inc/djinterp/c/modules/net/ftp/ftp_client.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.11.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Buffer sizes
         1.  D_FTP_CLIENT_REPLY_SIZE
         2.  D_FTP_CLIENT_INPUT_SIZE
         3.  D_FTP_CLIENT_LINE_SIZE
         4.  D_FTP_CLIENT_CHUNK_SIZE
         5.  D_FTP_CLIENT_HOST_SIZE
2.  TYPES
    -----
    1.  Transfer callbacks
         1.  dj_ftp_sink_fn
         2.  dj_ftp_source_fn
    2.  Security layers
         1.  dj_ftp_io
         2.  dj_ftp_security_ops
    3.  Connections and clients
         1.  dj_ftp_connection
         2.  dj_ftp_client
3.  CLIENT
    ------
    1.  Setup
    2.  Session
    3.  Commands
    4.  Transfers
    5.  Teardown
*/

#ifndef DJINTERP_C_NET_FTP_FTP_CLIENT_H
#define DJINTERP_C_NET_FTP_FTP_CLIENT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../djinterp.h"                // framework root
#include "../../../../config/c/modules/net/ftp/cfg_ftp.h"  // D_INTERNAL_FTP_*
#include "../net_common.h"                   // dj_net_error
#include "../tcp/tcp.h"                      // dj_tcp_connection
#include "./ftp_command.h"                   // dj_ftp_command
#include "./ftp_common.h"                    // dj_ftp_error
#include "./ftp_listing.h"                   // dj_ftp_listing_format
#include "./ftp_options.h"                   // dj_ftp_options
#include "./ftp_reply.h"                     // dj_ftp_reply_parser
#include "./ftp_security.h"                  // dj_ftp_protection
#include "../../../../config/c/modules/net/cfg_net.h"        // D_INTERNAL_NET_REQUIRE_BACKENDS
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint16_t, uint32_t

// 64-bit floor: this header needs a 64-bit integer type, and compiles to
// nothing where the build has none - ISO strict C++98 wherever int64_t is
// `long long`: a 32-bit target, 64-bit Windows or macOS (the owner's ruling of
// 2026.10.03 on round 3's question 1, (a)). The answer is net_common.h's, taken
// once per translation unit, so that net's gated headers agree.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_client.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================


// 1.1    Buffer sizes
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_CLIENT_REPLY_SIZE
//   constant: bytes kept of a reply's text, its terminator included. A longer
// reply - FEAT, HELP, and STAT run long - is truncated but fully parsed.
// Set by D_CFG_FTP_REPLY_SIZE.
#define D_FTP_CLIENT_REPLY_SIZE D_INTERNAL_FTP_REPLY_SIZE

// 1.1.2
// D_FTP_CLIENT_INPUT_SIZE
//   constant: bytes read from the control connection ahead of the parser.
// Set by D_CFG_FTP_INPUT_SIZE.
#define D_FTP_CLIENT_INPUT_SIZE D_INTERNAL_FTP_INPUT_SIZE

// 1.1.3
// D_FTP_CLIENT_LINE_SIZE
//   constant: the longest command line the client sends, CR LF and a
// terminator included; it bounds the paths a command can name. Set by
// D_CFG_FTP_LINE_SIZE.
#define D_FTP_CLIENT_LINE_SIZE  D_INTERNAL_FTP_LINE_SIZE

// 1.1.4
// D_FTP_CLIENT_CHUNK_SIZE
//   constant: bytes moved per read or write of a transfer. Set by
// D_CFG_FTP_CHUNK_SIZE.
#define D_FTP_CLIENT_CHUNK_SIZE D_INTERNAL_FTP_CHUNK_SIZE

// 1.1.5
// D_FTP_CLIENT_HOST_SIZE
//   constant: bytes kept of the server's host name: the longest a name may
// be, 253 characters (RFC 1035 section 2.3.4), a trailing dot, and the
// terminator. The host is what a security layer checks the server against.
#define D_FTP_CLIENT_HOST_SIZE  255u


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Transfer callbacks
//------------------------------------------------------------------------------
// 2.1.1
// dj_ftp_sink_fn
//   function pointer: takes `_size` (at least 1) bytes of a download. Returns
// D_FTP_OK to go on, or any error to abandon the transfer, which then
// returns that error.
typedef enum dj_ftp_error (*dj_ftp_sink_fn)(void*       _context,
                                          const void* _data,
                                          size_t      _size);

// 2.1.2
// dj_ftp_source_fn
//   function pointer: supplies up to `_capacity` bytes of an upload in
// `_buffer`, storing the count in `*_out_size`; a count of 0 ends the file.
// Returns D_FTP_OK, or any error to abandon the transfer.
typedef enum dj_ftp_error (*dj_ftp_source_fn)(void*   _context,
                                            void*   _buffer,
                                            size_t  _capacity,
                                            size_t* _out_size);


// 2.2    Security layers
//------------------------------------------------------------------------------
//   A layer is a mechanism's functions over its own state, which the client
// is given once (d_ftp_client_set_security) and calls on its control and
// data connections. Each function takes the connection it acts on, one of
// the client's two, and one that can fail stores the mechanism's own status
// in `*_status`, which the client keeps as `security_status` when it reports
// D_FTP_ERROR_TLS. Over a secured connection the layer reads and writes the
// connection's `tcp.base` itself, and keeps its `failure` as the transport
// last left it.

struct dj_ftp_connection;

// 2.2.1
// dj_ftp_io
//   enum: how a read, a write, a close or the securing of a connection went,
// the same whether a layer carries it or the connection is plain.
enum dj_ftp_io
{
    D_FTP_IO_OK = 0,     // done
    D_FTP_IO_CLOSED,     // the stream ended, or the peer no longer reads
    D_FTP_IO_TRANSPORT,  // the connection failed: its error in `failure`
    D_FTP_IO_TRUNCATED,  // a secured stream ended without its close
    D_FTP_IO_SECURITY    // the mechanism failed: its status in `*_status`
};

// 2.2.2
// dj_ftp_security_ops
//   struct: a security layer's functions, each over `_layer`, the state the
// layer was given with them.
//     mechanism        AUTH's argument: "TLS" for RFC 4217.
//     honours          whether the mechanism, as set up, checks at least what
//                      `_options` ask - a peer verified, its name checked;
//                      the client refuses to connect otherwise.
//     secure           secures `_connection` as the client of the mechanism
//                      naming `_host`; `_resume` is the secured control
//                      connection, for a data connection to resume its
//                      session, or NULL. A failure leaves nothing behind.
//     read, write      move bytes through the secured connection; a write
//                      sends all of them. The end of the stream is CLOSED.
//     shutdown         sends the mechanism's close, in order.
//     drop             forgets the connection's security, without a word.
//     transport_error  what the transport under it last reported.
struct dj_ftp_security_ops
{
    const char* mechanism;
    bool              (*honours)(const void*                  _layer,
                                 const struct dj_ftp_options* _options);
    enum dj_ftp_io    (*secure)(void*                           _layer,
                                struct dj_ftp_connection*       _connection,
                                const struct dj_ftp_connection* _resume,
                                const char*                     _host,
                                int32_t*                        _status);
    enum dj_ftp_io    (*read)(void*                     _layer,
                              struct dj_ftp_connection* _connection,
                              void*                     _buffer,
                              size_t                    _capacity,
                              size_t*                   _out_read,
                              int32_t*                  _status);
    enum dj_ftp_io    (*write)(void*                     _layer,
                               struct dj_ftp_connection* _connection,
                               const void*               _data,
                               size_t                    _size,
                               int32_t*                  _status);
    enum dj_ftp_io    (*shutdown)(void*                     _layer,
                                  struct dj_ftp_connection* _connection,
                                  int32_t*                  _status);
    void              (*drop)(void*                     _layer,
                              struct dj_ftp_connection* _connection);
    enum dj_net_error (*transport_error)(
                          const void*                     _layer,
                          const struct dj_ftp_connection* _connection);
};


// 2.3    Connections and clients
//------------------------------------------------------------------------------
// 2.3.1
// dj_ftp_connection
//   struct: one connection: its TCP transport, read and written as a
// dj_net_connection through `tcp.base`, and secured by the client's layer
// while `secured` is set. `failure` is the transport's last error, kept even
// when the layer is what reported the failure.
struct dj_ftp_connection
{
    struct dj_tcp_connection tcp;      // the transport
    bool                    secured;  // the layer in force
    enum dj_net_error        failure;  // the transport's last error
};

// 2.3.2
// dj_ftp_client
//   struct: a client session. `reply`, `features`, `protection`,
// `security_status`, `net_error`, the flags, and each connection's `secured`
// may be read freely; the other fields are the client's own.
struct dj_ftp_client
{
    struct dj_ftp_options              options;          // copied at init
    const struct dj_ftp_security_ops*  security;         // NULL: plain only
    void*                              security_layer;   // the layer's state
    char                              host[D_FTP_CLIENT_HOST_SIZE];
    struct dj_ftp_connection     control;          // the control connection
    struct dj_ftp_connection     data;             // the transfer's connection
    struct dj_ftp_reply_parser   parser;           // reads control replies
    char                        reply_text[D_FTP_CLIENT_REPLY_SIZE];
    struct dj_ftp_reply          reply;            // the last reply read
    char                        input[D_FTP_CLIENT_INPUT_SIZE];
    size_t                      input_start;      // first byte not parsed
    size_t                      input_end;        // end of the bytes read
    uint32_t                    features;         // D_FTP_FEATURE_* bits
    enum dj_ftp_protection       protection;       // PROT level in force
    bool                        connected;        // control connection up
    bool                        logged_in;        // login accepted
    bool                        type_sent;        // TYPE set this session
    bool                        extended_refused; // EPSV or EPRT refused
    int32_t                     security_status;  // the layer's, for
                                                  // ERROR_TLS; 0 for none
    int32_t                     security_last;    // the layer's last
                                                  // failure
    enum dj_net_error            net_error;        // cause of a lost link
};


//==============================================================================
// 3.  CLIENT
//==============================================================================


// 3.1    Setup
//------------------------------------------------------------------------------
/**
 * @brief Prepares a client: closed, plain, with its options copied.
 *
 * @param[out] _client  the client.
 * @param[in]  _options the options, copied; NULL for d_ftp_options_init()'s
 *                      defaults. The strings they point to must outlive the
 *                      client.
 */
void d_ftp_client_init(
    struct dj_ftp_client*        _client,
    const struct dj_ftp_options* _options
);

/**
 * @brief Gives a closed client the security layer that options asking for
 *        security run through: module ftps does, for FTPS
 *        (d_ftps_client_init). NULL ops make the client plain again.
 *
 * @param[in,out] _client  the client, not connected.
 * @param[in]     _ops     the layer's functions; they must outlive the
 *                         client.
 * @param[in]     _layer   the state they are given; it must outlive the
 *                         client.
 */
void d_ftp_client_set_security(
    struct dj_ftp_client*             _client,
    const struct dj_ftp_security_ops* _ops,
    void*                             _layer
);


// 3.2    Session
//------------------------------------------------------------------------------
/**
 * @brief Connects, secures the control connection as the options ask, and
 *        reads the server's features.
 *
 * Implicit security secures before the greeting; explicit security sends
 * AUTH, with the layer's mechanism, after it. FEAT follows once the
 * connection is as secure as it will get, so no one on the path can edit
 * the list.
 *
 * @param[in,out] _client the client, closed.
 * @param[in]     _host   the server's host name or address literal: where to
 *                        connect, and the name its certificate must carry.
 * @param[in]     _port   the control port; 0 for the security mode's
 *                        default, 990 for implicit FTPS and 21 otherwise.
 * @post   On success the client is connected; secured if the options asked
 *         for it, unless the server refused it and require_security is off.
 *         On failure it is closed again.
 * @return D_FTP_OK; D_FTP_ERROR_INVALID_ARGUMENT for NULL, empty, or overlong
 *         arguments, a client already connected, security options without a
 *         layer, or a layer that checks less than the options require; D_FTP_ERROR_RESOLVE, D_FTP_ERROR_CONNECT, or
 *         D_FTP_ERROR_TIMED_OUT when no connection is made, with the
 *         transport's error in `net_error`; D_FTP_ERROR_TLS for a
 *         failed handshake, with the layer's status in `security_status`;
 *         D_FTP_ERROR_SECURITY if the server refused AUTH while security is
 *         required, or sent plaintext after agreeing to it; the error of
 *         a greeting other than 220; or a control-connection failure.
 */
enum dj_ftp_error d_ftp_client_connect(
    struct dj_ftp_client* _client,
    const char*           _host,
    uint16_t              _port
);
/**
 * @brief Logs in, then sets up data protection and session options.
 *
 * USER, then PASS and ACCT as the server asks, with the options' credentials
 * (anonymous when none are given). On a secured control connection PBSZ 0
 * and PROT follow, as RFC 4217 requires before protected data; then OPTS
 * UTF8 ON if the options want UTF-8 and the server offers it; then CCC if
 * the options ask to clear the control connection. A refused CCC is not an
 * error: the control connection simply stays encrypted.
 *
 * @param[in,out] _client the client, connected.
 * @return D_FTP_OK; D_FTP_ERROR_BAD_SEQUENCE if the client is not connected
 *         or already logged in; D_FTP_ERROR_LOGIN_DENIED, or the error of
 *         another reply; D_FTP_ERROR_ACCOUNT_REQUIRED if the server wants an
 *         account the options lack; D_FTP_ERROR_UNSUPPORTED for a protection
 *         level TLS cannot provide; D_FTP_ERROR_SECURITY if the server
 *         refused PBSZ or PROT P while security is required; or a
 *         control-connection failure.
 */
enum dj_ftp_error d_ftp_client_login(
    struct dj_ftp_client* _client
);


// 3.3    Commands
//------------------------------------------------------------------------------
/**
 * @brief Sends one command and reads its final reply.
 *
 * For commands without a data connection - CWD, PWD, MKD, DELE, SIZE, MDTM,
 * SITE, and the like. The commands whose state the client keeps (USER, PASS,
 * ACCT, AUTH, PBSZ, PROT, CCC, TYPE, PORT, EPRT, PASV, EPSV, REIN, and QUIT)
 * are refused; the functions of this header send them. Preliminary (1yz)
 * replies are read past.
 *
 * @param[in,out] _client   the client, connected.
 * @param[in]     _command  the command.
 * @param[in]     _argument its argument, or NULL; it must not hold CR or LF.
 * @post   `reply` holds the final reply; its text stays valid until the next
 *         call that reads a reply.
 * @return D_FTP_OK for a positive completion or intermediate reply (2yz,
 *         3yz); otherwise the reply's error, see d_ftp_error_from_reply();
 *         D_FTP_ERROR_INVALID_ARGUMENT for a refused command or an argument
 *         that cannot be sent; D_FTP_ERROR_BAD_SEQUENCE when not connected;
 *         or a control-connection failure.
 */
enum dj_ftp_error d_ftp_client_command(
    struct dj_ftp_client* _client,
    enum dj_ftp_command   _command,
    const char*           _argument
);


// 3.4    Transfers
//------------------------------------------------------------------------------
// Each transfer opens a data connection as the options' data_mode says,
// sending TYPE first if it has not been sent, and secures it with the layer
// while PROT P is in force. Each needs a logged-in client, and returns D_FTP_OK only once
// all the data has moved and the server has confirmed it. Failing, it
// returns the first error: D_FTP_ERROR_BAD_SEQUENCE when not logged in;
// D_FTP_ERROR_INVALID_ARGUMENT for NULL or empty arguments; the error of a
// refused command or of the final reply; the callback's own error, which
// abandons the transfer; D_FTP_ERROR_DATA_CONNECTION or D_FTP_ERROR_TIMED_OUT
// for a data connection that could not be made or failed; D_FTP_ERROR_TLS for
// the layer failing on it, or a secured stream that ended without its close
// (TLS's close_notify); or a control-connection failure. `reply` holds the last reply read.
/**
 * @brief Downloads a file (RETR) into a sink.
 *
 * With an ASCII representation type, the network's CR LF line ends reach
 * the sink as LF.
 *
 * @param[in,out] _client  the client, logged in.
 * @param[in]     _path    the file.
 * @param[in]     _sink    takes the data.
 * @param[in]     _context passed to `_sink`.
 * @return as described above.
 */
enum dj_ftp_error d_ftp_client_retrieve(
    struct dj_ftp_client* _client,
    const char*           _path,
    dj_ftp_sink_fn        _sink,
    void*                 _context
);
/**
 * @brief Uploads a file (STOR) from a source.
 *
 * With an ASCII representation type, LF line ends from the source go out as
 * CR LF. Secured, the data ends with the layer's close (TLS's close_notify),
 * so the server can tell a complete upload from a cut one.
 *
 * @param[in,out] _client  the client, logged in.
 * @param[in]     _path    the file to write.
 * @param[in]     _source  supplies the data.
 * @param[in]     _context passed to `_source`.
 * @return as described above.
 */
enum dj_ftp_error d_ftp_client_store(
    struct dj_ftp_client* _client,
    const char*           _path,
    dj_ftp_source_fn      _source,
    void*                 _context
);
/**
 * @brief Lists a directory into a sink: MLSD where the server offers it,
 *        LIST otherwise.
 *
 * The listing reaches the sink as the server sent it, lines ending in CR LF,
 * ready for d_net_line_next() and d_ftp_listing_parse().
 *
 * @param[in,out] _client     the client, logged in.
 * @param[in]     _path       the directory, or NULL for the current one.
 * @param[in]     _sink       takes the listing.
 * @param[in]     _context    passed to `_sink`.
 * @param[out]    _out_format the dialect to parse it as: D_FTP_LISTING_MLSX
 *                            after MLSD, D_FTP_LISTING_AUTO after LIST. May
 *                            be NULL.
 * @return as described above.
 */
enum dj_ftp_error d_ftp_client_list(
    struct dj_ftp_client*       _client,
    const char*                 _path,
    dj_ftp_sink_fn              _sink,
    void*                       _context,
    enum dj_ftp_listing_format* _out_format
);


// 3.5    Teardown
//------------------------------------------------------------------------------
/**
 * @brief Says QUIT, closes the layer in order, and closes the connections.
 *
 * @param[in,out] _client the client.
 * @post   The client is closed, whatever the result, and may connect again.
 * @return D_FTP_OK; the error of a QUIT that failed or was refused;
 *         D_FTP_ERROR_BAD_SEQUENCE when not connected; or
 *         D_FTP_ERROR_INVALID_ARGUMENT for a NULL client.
 */
enum dj_ftp_error d_ftp_client_quit(
    struct dj_ftp_client* _client
);
// d_ftp_client_close() closes every connection at once, sending nothing, to
// abandon a session; the client may connect again. NULL is ignored, and
// closing twice is harmless.
void d_ftp_client_close(
    struct dj_ftp_client* _client
);


D_EXTERN_C_END


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "ftp_client.h: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_C_NET_FTP_FTP_CLIENT_H
