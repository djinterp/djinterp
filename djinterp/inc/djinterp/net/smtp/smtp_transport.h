/*******************************************************************************
* djinterp [net]                                                smtp_transport.h
*
* djinterp SMTP transport.
*   How the SMTP kernel moves bytes. A session reads and writes through a
* d_smtp_transport -- a small vtable of read, write, STARTTLS and close -- so
* the client and server engines run unchanged over the built-in socket
* transport, a caller's own stream, or a scripted test double. This header
* also provides the line reader both engines use, and the built-in transport
* itself: BSD sockets with an optional OpenSSL layer, for connecting
* (d_smtp_socket_*) and for listening (d_smtp_listener_*).
*   The built-in transport is compiled where env_smtp.h reports it
* (D_ENV_SMTP_CAN_CONNECT, D_ENV_SMTP_CAN_TLS). Elsewhere its functions still
* exist and report D_SMTP_ERROR_UNSUPPORTED, so callers need no #if of their
* own.
*
*
* path:      /inc/djinterp/net/smtp/smtp_transport.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSPORT INTERFACE
    -------------------
    1.  TLS options
         1.  d_smtp_tls_options
    2.  Transport vtable
         1.  d_smtp_transport
    3.  Transport helpers
2.  LINE READER
    -----------
    1.  Reader type
         1.  d_smtp_reader
    2.  Reader operations
3.  SOCKET TRANSPORT
    ----------------
    1.  Sockets
         1.  d_smtp_socket
    2.  Socket operations
    3.  Listeners
         1.  d_smtp_listener
    4.  Listener operations
*/

#ifndef DJINTERP_NET_SMTP_SMTP_TRANSPORT_H
#define DJINTERP_NET_SMTP_SMTP_TRANSPORT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"                // framework root
#include "../../env/net/smtp/env_smtp.h"     // D_ENV_SMTP_*
#include "./smtp_common.h"                   // d_smtp_error, d_smtp_buffer


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSPORT INTERFACE
//==============================================================================
// A transport is any byte stream that can also be upgraded to TLS in place.
// Its functions block, and report an expired deadline as
// D_SMTP_ERROR_TIMEOUT; the kernel never retries on its behalf.


// 1.1    TLS options
//------------------------------------------------------------------------------
// 1.1.1
// d_smtp_tls_options
//   struct: what a TLS handshake needs. Strings are borrowed for the duration
// of the call that takes them.
struct d_smtp_tls_options
{
    const char* server_name;       // client: SNI, and the name to verify
    const char* ca_file;           // client: PEM trust anchors; NULL = system
    const char* certificate_file;  // server: PEM certificate chain
    const char* private_key_file;  // server: PEM private key
    bool        verify_peer;       // client: verify the certificate
};

// d_smtp_tls_options_init -- no files, no name, verification on; NULL ignored
void              d_smtp_tls_options_init(struct d_smtp_tls_options* _options);

// 1.2    Transport vtable
//------------------------------------------------------------------------------
// d_smtp_read_fn
//   function type: reads at least one byte into `_buffer`, storing the count
// in `_received`; D_SMTP_ERROR_CLOSED at end of stream.
typedef enum d_smtp_error (*d_smtp_read_fn)(void*   _context,
                                            void*   _buffer,
                                            size_t  _capacity,
                                            size_t* _received);

// d_smtp_write_fn
//   function type: writes at least one byte of `_data`, storing the count in
// `_sent`.
typedef enum d_smtp_error (*d_smtp_write_fn)(void*       _context,
                                             const void* _data,
                                             size_t      _length,
                                             size_t*     _sent);

// d_smtp_starttls_fn
//   function type: performs a TLS handshake over the stream, as client or
// server according to how the stream was opened.
typedef enum d_smtp_error (*d_smtp_starttls_fn)(
                              void*                            _context,
                              const struct d_smtp_tls_options* _options);

// d_smtp_close_fn
//   function type: releases the stream; called at most once.
typedef void              (*d_smtp_close_fn)(void* _context);

// 1.2.1
// d_smtp_transport
//   struct: a transport's functions and the context they receive. `read` and
// `write` are required; a NULL `starttls` means the transport cannot upgrade,
// and a NULL `close` means the caller releases the stream itself.
struct d_smtp_transport
{
    void*              context;
    d_smtp_read_fn     read;
    d_smtp_write_fn    write;
    d_smtp_starttls_fn starttls;
    d_smtp_close_fn    close;
};

// 1.3    Transport helpers
//------------------------------------------------------------------------------
/**
 * @brief Writes a whole byte range, looping over short writes.
 *
 * @param[in] _transport  the transport.
 * @param[in] _data       the bytes; may be NULL when `_length` is 0.
 * @param[in] _length     number of bytes.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument or a transport
 *         without `write`; D_SMTP_ERROR_IO when the transport accepts
 *         nothing; or the transport's own error.
 */
enum d_smtp_error d_smtp_transport_write_all(
                      const struct d_smtp_transport* _transport,
                      const void*                    _data,
                      size_t                         _length);


//==============================================================================
// 2.  LINE READER
//==============================================================================
// SMTP is line-oriented in both directions. The reader buffers transport
// reads and hands out one CRLF-terminated line at a time.


// 2.1    Reader type
//------------------------------------------------------------------------------
// D_SMTP_READER_BUFFER
//   constant: bytes the reader requests from the transport per read.
#define D_SMTP_READER_BUFFER        4096u

// 2.1.1
// d_smtp_reader
//   struct: a transport and the bytes read from it but not yet consumed.
struct d_smtp_reader
{
    struct d_smtp_transport transport;                     // copied at init
    size_t                  start;                         // next unread byte
    size_t                  end;                           // end of buffered
    bool                    crlf;                          // last line: CRLF
    char                    buffer[D_SMTP_READER_BUFFER];
};

// 2.2    Reader operations
//------------------------------------------------------------------------------
// d_smtp_reader_init -- binds a reader to a copy of `_transport`, emptied
void              d_smtp_reader_init(struct d_smtp_reader*          _reader,
                                     const struct d_smtp_transport* _transport);

/**
 * @brief Reads one line, without its terminator.
 *
 * @note CRLF ends a line; a bare LF is also accepted, as RFC 5321 section
 *       2.3.8 permits a receiver to. A bare CR inside a line is kept.
 *
 * @param[in,out] _reader    the reader.
 * @param[out]    _line      receives the line, NUL-terminated.
 * @param[in]     _capacity  size of `_line`, NUL included.
 * @param[out]    _length    receives the length stored.
 * @post `_reader->crlf` records whether the line ended with CRLF rather than
 *       a bare LF -- what a server needs to recognize end of data strictly.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID for a NULL argument;
 *         D_SMTP_ERROR_TOO_LONG when the line did not fit -- the whole line
 *         has still been consumed and `_line` holds its beginning, so the
 *         stream stays in sync; or the transport's error.
 */
enum d_smtp_error d_smtp_reader_line(struct d_smtp_reader* _reader,
                                     char*                 _line,
                                     size_t                _capacity,
                                     size_t*               _length);

// buffered-byte control -- O(1), never fail. `pending` counts bytes read but
// not consumed; `discard` drops them, which a STARTTLS upgrade requires.
size_t            d_smtp_reader_pending(const struct d_smtp_reader* _reader);
void              d_smtp_reader_discard(struct d_smtp_reader* _reader);


//==============================================================================
// 3.  SOCKET TRANSPORT
//==============================================================================
// Blocking BSD sockets whose I/O is bounded by SO_RCVTIMEO and SO_SNDTIMEO,
// with TLS layered through OpenSSL when D_ENV_SMTP_CAN_TLS is set. Client
// TLS requires TLS 1.2 or later and verifies the peer by default, against
// the given CA file or the system store, and against the connected name.


// 3.1    Sockets
//------------------------------------------------------------------------------
// 3.1.1
// d_smtp_socket
//   struct: one connection. It must not move while TLS is active: the TLS
// layer holds its address.
struct d_smtp_socket
{
    int   descriptor;   // -1 when closed
    bool  server;       // accepted rather than connected
    void* tls;          // SSL* while TLS is active
    void* tls_context;  // SSL_CTX* owned by this socket
    void* tls_method;   // BIO_METHOD* owned by this socket
};

// 3.2    Socket operations
//------------------------------------------------------------------------------
// d_smtp_socket_init -- a closed socket; NULL is ignored
void              d_smtp_socket_init(struct d_smtp_socket* _socket);

/**
 * @brief Resolves a host and connects to the first address that accepts.
 *
 * @param[in,out] _socket              a closed socket.
 * @param[in]     _host                host name or address literal.
 * @param[in]     _port                port, 1 to 65535.
 * @param[in]     _connect_timeout_ms  limit on each connection attempt; 0 or
 *                                     less waits without limit.
 * @param[in]     _io_timeout_ms       limit on every later read and write; 0
 *                                     or less waits without limit.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID or D_SMTP_ERROR_STATE for bad
 *         arguments or an open socket; D_SMTP_ERROR_RESOLVE;
 *         D_SMTP_ERROR_CONNECT; D_SMTP_ERROR_TIMEOUT; or
 *         D_SMTP_ERROR_UNSUPPORTED without D_ENV_SMTP_CAN_CONNECT.
 */
enum d_smtp_error d_smtp_socket_connect(
                      struct d_smtp_socket* _socket,
                      const char*           _host,
                      unsigned int          _port,
                      long                  _connect_timeout_ms,
                      long                  _io_timeout_ms);

/**
 * @brief Performs the TLS handshake, as client or server by how the socket
 *        was opened.
 *
 * @param[in,out] _socket   an open socket without TLS.
 * @param[in]     _options  a client needs `server_name` when verifying; a
 *                          server needs `certificate_file` and
 *                          `private_key_file`.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID; D_SMTP_ERROR_STATE when TLS is
 *         already active; D_SMTP_ERROR_TLS when the handshake or the
 *         certificate check fails; D_SMTP_ERROR_TIMEOUT; or
 *         D_SMTP_ERROR_UNSUPPORTED without D_ENV_SMTP_CAN_TLS.
 */
enum d_smtp_error d_smtp_socket_start_tls(
                      struct d_smtp_socket*            _socket,
                      const struct d_smtp_tls_options* _options);

// d_smtp_socket_transport -- fills `_transport` with this socket's functions;
// the socket must outlive the transport, whose close closes the socket
void              d_smtp_socket_transport(struct d_smtp_socket*    _socket,
                                          struct d_smtp_transport* _transport);

/**
 * @brief Appends this end's identity for EHLO: the machine's fully qualified
 *        name, or failing that an address literal of the local address.
 *
 * @param[in]     _socket  an open socket.
 * @param[in,out] _buffer  receives the identity.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID; D_SMTP_ERROR_IO; or
 *         D_SMTP_ERROR_TOO_LONG.
 */
enum d_smtp_error d_smtp_socket_identity(const struct d_smtp_socket* _socket,
                                         struct d_smtp_buffer*       _buffer);

// socket state -- O(1), never fail. `close` sends a TLS close_notify when TLS
// is active, releases everything, and leaves the socket as init did.
bool              d_smtp_socket_is_secure(const struct d_smtp_socket* _socket);
void              d_smtp_socket_close(struct d_smtp_socket* _socket);

// 3.3    Listeners
//------------------------------------------------------------------------------
// 3.3.1
// d_smtp_listener
//   struct: a listening socket for the server.
struct d_smtp_listener
{
    int          descriptor;  // -1 when closed
    unsigned int port;        // the bound port, even when 0 was requested
};

// 3.4    Listener operations
//------------------------------------------------------------------------------
// d_smtp_listener_init -- a closed listener; NULL is ignored
void              d_smtp_listener_init(struct d_smtp_listener* _listener);

/**
 * @brief Binds and listens.
 *
 * @param[in,out] _listener  a closed listener.
 * @param[in]     _host      address to bind, or NULL for every interface.
 * @param[in]     _port      port, or 0 for one the system chooses.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID; D_SMTP_ERROR_STATE;
 *         D_SMTP_ERROR_RESOLVE; D_SMTP_ERROR_IO when no address could be
 *         bound; or D_SMTP_ERROR_UNSUPPORTED without D_ENV_SMTP_CAN_SERVE.
 */
enum d_smtp_error d_smtp_listener_open(struct d_smtp_listener* _listener,
                                       const char*             _host,
                                       unsigned int            _port);

/**
 * @brief Waits for and accepts one connection.
 *
 * @param[in,out] _listener       an open listener.
 * @param[in]     _wait_ms        how long to wait; less than 0 waits forever.
 * @param[in]     _io_timeout_ms  I/O limit applied to the accepted socket.
 * @param[out]    _socket         a closed socket, which receives the
 *                                connection.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID; D_SMTP_ERROR_STATE;
 *         D_SMTP_ERROR_TIMEOUT when nothing arrived in time; D_SMTP_ERROR_IO;
 *         or D_SMTP_ERROR_UNSUPPORTED without D_ENV_SMTP_CAN_SERVE.
 */
enum d_smtp_error d_smtp_listener_accept(struct d_smtp_listener* _listener,
                                         long                    _wait_ms,
                                         long                    _io_timeout_ms,
                                         struct d_smtp_socket*   _socket);

// d_smtp_listener_close -- stops listening; idempotent, NULL is ignored
void              d_smtp_listener_close(struct d_smtp_listener* _listener);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SMTP_SMTP_TRANSPORT_H
