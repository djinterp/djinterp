/*******************************************************************************
* djinterp [net]                                                    ftp_client.h
*
* An FTP client, plain or secured with TLS: FTPS (RFC 4217).
*   The client speaks the control protocol through the net/ftp/ vocabulary and
* moves bytes through the TCP transport (net/tcp/tcp.h), reading and writing
* every connection through net.h's d_net_connection, so the client itself
* holds no OS code. For FTPS it runs TLS through the SSL kernel
* (net/ssl/ssl.h), carried over each connection: explicitly, upgrading the
* control connection with AUTH TLS, or implicitly, from the first byte on
* port 990; and, after PBSZ 0 and PROT P, on every data connection, where
* the client is always the TLS client (RFC 4217 section 7) in passive and
* active mode alike.
*   The TLS library is whatever engine the caller's d_ssl_context was made
* with. Control and data sessions come from that one context and name the
* same host, so an engine that caches client sessions by host resumes the
* control session on each data connection, as servers such as vsftpd require
* by default. d_ftp_client_tls_config() fills a kernel configuration from the
* options.
*   Security settings are never weakened silently: a context whose plan checks
* less than the options ask is refused before connecting; with
* require_security set, a server that refuses AUTH TLS ends the session before
* any credential is sent, and one that refuses PBSZ or PROT P fails the login;
* plaintext received after the server agreed to AUTH TLS, which an attacker
* could have injected, aborts the upgrade; and a TLS data stream that stops
* without close_notify is reported as truncated, not complete.
*   Every call blocks. The TCP transport bounds each connection attempt by
* the options' connect_timeout_ms; it has no read timeouts yet, so
* response_timeout_ms and idle_timeout_ms take effect only once it does, and
* an active-mode transfer waits for the server's connection without limit.
* A client holds two TLS sessions and its buffers, so it is large: give it
* static or heap storage, and do not copy it once initialized. Buffer sizes
* come from cfg_ftp.h.
*
*
* path:      /inc/djinterp/net/ftp/ftp_client.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.28
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
2.  TYPES
    -----
    1.  Transfer callbacks
         1.  d_ftp_sink_fn
         2.  d_ftp_source_fn
    2.  Connections and clients
         1.  d_ftp_connection
         2.  d_ftp_client
3.  CLIENT
    ------
    1.  Setup
    2.  Session
    3.  Commands
    4.  Transfers
    5.  Teardown
*/

#ifndef DJINTERP_NET_FTP_FTP_CLIENT_H
#define DJINTERP_NET_FTP_FTP_CLIENT_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // uint16_t, uint32_t
// djinterp
#include "../../c/djinterp.h"                // framework root
#include "../../config/net/ftp/cfg_ftp.h"  // D_INTERNAL_FTP_*
#include "../net.h"                          // d_net_error
#include "../ssl/ssl.h"                      // d_ssl_context, d_ssl_session
#include "../tcp/tcp.h"                      // d_tcp_connection
#include "./ftp_command.h"                   // d_ftp_command
#include "./ftp_common.h"                    // d_ftp_error
#include "./ftp_listing.h"                   // d_ftp_listing_format
#include "./ftp_options.h"                   // d_ftp_options
#include "./ftp_reply.h"                     // d_ftp_reply_parser
#include "./ftp_security.h"                  // d_ftp_protection


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
// reply -- FEAT, HELP, and STAT run long -- is truncated but fully parsed.
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


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Transfer callbacks
//------------------------------------------------------------------------------
// 2.1.1
// d_ftp_sink_fn
//   function pointer: takes `_size` (at least 1) bytes of a download. Returns
// D_FTP_OK to go on, or any error to abandon the transfer, which then
// returns that error.
typedef enum d_ftp_error (*d_ftp_sink_fn)(void*       _context,
                                          const void* _data,
                                          size_t      _size);

// 2.1.2
// d_ftp_source_fn
//   function pointer: supplies up to `_capacity` bytes of an upload in
// `_buffer`, storing the count in `*_out_size`; a count of 0 ends the file.
// Returns D_FTP_OK, or any error to abandon the transfer.
typedef enum d_ftp_error (*d_ftp_source_fn)(void*   _context,
                                            void*   _buffer,
                                            size_t  _capacity,
                                            size_t* _out_size);


// 2.2    Connections and clients
//------------------------------------------------------------------------------
// 2.2.1
// d_ftp_connection
//   struct: one connection: its TCP transport, read and written as a
// d_net_connection through `tcp.base`, and the TLS session over it while
// `secured` is set. `failure` is the transport's last error, kept even when
// TLS is what reported the failure.
struct d_ftp_connection
{
    struct d_tcp_connection tcp;      // the transport
    struct d_ssl_session    tls;      // valid while secured
    bool                    secured;  // TLS in force
    enum d_net_error        failure;  // the transport's last error
};

// 2.2.2
// d_ftp_client
//   struct: a client session. `reply`, `features`, `protection`,
// `tls_status`, `net_error`, the flags, and each connection's `secured` may
// be read freely; the other fields are the client's own.
struct d_ftp_client
{
    struct d_ftp_options        options;          // copied at init
    const struct d_ssl_context* tls;              // borrowed; NULL: plain only
    char                        host[D_SSL_HOSTNAME_MAX + 2];
    struct d_ftp_connection     control;          // the control connection
    struct d_ftp_connection     data;             // the transfer's connection
    struct d_ftp_reply_parser   parser;           // reads control replies
    char                        reply_text[D_FTP_CLIENT_REPLY_SIZE];
    struct d_ftp_reply          reply;            // the last reply read
    char                        input[D_FTP_CLIENT_INPUT_SIZE];
    size_t                      input_start;      // first byte not parsed
    size_t                      input_end;        // end of the bytes read
    uint32_t                    features;         // D_FTP_FEATURE_* bits
    enum d_ftp_protection       protection;       // PROT level in force
    bool                        connected;        // control connection up
    bool                        logged_in;        // login accepted
    bool                        type_sent;        // TYPE set this session
    bool                        extended_refused; // EPSV or EPRT refused
    enum d_ssl_status           tls_status;       // cause of ERROR_TLS
    enum d_net_error            net_error;        // cause of a lost link
};


//==============================================================================
// 3.  CLIENT
//==============================================================================


// 3.1    Setup
//------------------------------------------------------------------------------
// d_ftp_client_tls_config() fills a kernel configuration for an FTPS client:
// a client's, verifying the server's chain and its name as the options'
// verify_peer and verify_host ask, against the system trust store. Set trust
// anchors or pins on it before d_ssl_context_init(). NULL arguments are
// ignored.
void d_ftp_client_tls_config(const struct d_ftp_options* _options,
                             struct d_ssl_config*        _config);
/**
 * @brief Prepares a client: closed, with its options copied.
 *
 * @param[out] _client  the client.
 * @param[in]  _options the options, copied; NULL for d_ftp_options_init()'s
 *                      defaults. The strings they point to must outlive the
 *                      client.
 * @param[in]  _tls     the TLS context for FTPS, a client's, borrowed for the
 *                      client's life; NULL allows plain FTP only.
 */
void d_ftp_client_init(struct d_ftp_client*        _client,
                       const struct d_ftp_options* _options,
                       const struct d_ssl_context* _tls);


// 3.2    Session
//------------------------------------------------------------------------------
/**
 * @brief Connects, secures the control connection as the options ask, and
 *        reads the server's features.
 *
 * Implicit FTPS handshakes before the greeting; explicit FTPS sends AUTH TLS
 * after it. FEAT follows once the connection is as secure as it will get, so
 * no one on the path can edit the list.
 *
 * @param[in,out] _client the client, closed.
 * @param[in]     _host   the server's host name or address literal: where to
 *                        connect, and the name its certificate must carry.
 * @param[in]     _port   the control port; 0 for the security mode's
 *                        default, 990 for implicit FTPS and 21 otherwise.
 * @post   On success the client is connected; secured if the options asked
 *         for TLS, unless the server refused it and require_security is off.
 *         On failure it is closed again.
 * @return D_FTP_OK; D_FTP_ERROR_INVALID_ARGUMENT for NULL, empty, or overlong
 *         arguments, a client already connected, TLS options without a
 *         context, or a context whose plan checks less than the options
 *         require; D_FTP_ERROR_RESOLVE, D_FTP_ERROR_CONNECT, or
 *         D_FTP_ERROR_TIMEOUT when no connection is made, with the
 *         transport's error in `net_error`; D_FTP_ERROR_TLS for a
 *         failed handshake, with the kernel's status in `tls_status`;
 *         D_FTP_ERROR_SECURITY if the server refused AUTH TLS while security
 *         is required, or sent plaintext after agreeing to it; the error of
 *         a greeting other than 220; or a control-connection failure.
 */
enum d_ftp_error d_ftp_client_connect(struct d_ftp_client* _client,
                                      const char*          _host,
                                      uint16_t             _port);
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
enum d_ftp_error d_ftp_client_login(struct d_ftp_client* _client);


// 3.3    Commands
//------------------------------------------------------------------------------
/**
 * @brief Sends one command and reads its final reply.
 *
 * For commands without a data connection -- CWD, PWD, MKD, DELE, SIZE, MDTM,
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
enum d_ftp_error d_ftp_client_command(struct d_ftp_client* _client,
                                      enum d_ftp_command   _command,
                                      const char*          _argument);


// 3.4    Transfers
//------------------------------------------------------------------------------
// Each transfer opens a data connection as the options' data_mode says,
// sending TYPE first if it has not been sent, and runs TLS over it while PROT
// P is in force. Each needs a logged-in client, and returns D_FTP_OK only once
// all the data has moved and the server has confirmed it. Failing, it
// returns the first error: D_FTP_ERROR_BAD_SEQUENCE when not logged in;
// D_FTP_ERROR_INVALID_ARGUMENT for NULL or empty arguments; the error of a
// refused command or of the final reply; the callback's own error, which
// abandons the transfer; D_FTP_ERROR_DATA_CONNECTION or D_FTP_ERROR_TIMEOUT
// for a data connection that could not be made or failed; D_FTP_ERROR_TLS for
// TLS that failed on it, or that ended without close_notify; or a
// control-connection failure. `reply` holds the last reply read.
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
enum d_ftp_error d_ftp_client_retrieve(struct d_ftp_client* _client,
                                       const char*          _path,
                                       d_ftp_sink_fn        _sink,
                                       void*                _context);
/**
 * @brief Uploads a file (STOR) from a source.
 *
 * With an ASCII representation type, LF line ends from the source go out as
 * CR LF. Over TLS the data ends with close_notify, so the server can tell a
 * complete upload from a cut one.
 *
 * @param[in,out] _client  the client, logged in.
 * @param[in]     _path    the file to write.
 * @param[in]     _source  supplies the data.
 * @param[in]     _context passed to `_source`.
 * @return as described above.
 */
enum d_ftp_error d_ftp_client_store(struct d_ftp_client* _client,
                                    const char*          _path,
                                    d_ftp_source_fn      _source,
                                    void*                _context);
/**
 * @brief Lists a directory into a sink: MLSD where the server offers it,
 *        LIST otherwise.
 *
 * The listing reaches the sink as the server sent it, lines ending in CR LF,
 * ready for d_ftp_span_next_line() and d_ftp_listing_parse().
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
enum d_ftp_error d_ftp_client_list(struct d_ftp_client*       _client,
                                   const char*                _path,
                                   d_ftp_sink_fn              _sink,
                                   void*                      _context,
                                   enum d_ftp_listing_format* _out_format);


// 3.5    Teardown
//------------------------------------------------------------------------------
/**
 * @brief Says QUIT, closes TLS in order, and closes the connections.
 *
 * @param[in,out] _client the client.
 * @post   The client is closed, whatever the result, and may connect again.
 * @return D_FTP_OK; the error of a QUIT that failed or was refused;
 *         D_FTP_ERROR_BAD_SEQUENCE when not connected; or
 *         D_FTP_ERROR_INVALID_ARGUMENT for a NULL client.
 */
enum d_ftp_error d_ftp_client_quit(struct d_ftp_client* _client);
// d_ftp_client_close() closes every connection at once, sending nothing, to
// abandon a session; the client may connect again. NULL is ignored, and
// closing twice is harmless.
void             d_ftp_client_close(struct d_ftp_client* _client);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_CLIENT_H
