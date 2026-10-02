/*******************************************************************************
* djinterp [net]                                                           tcp.h
*
* The TCP transport: stream sockets behind net.h's connection.
*   A d_tcp_connection embeds d_net_connection as its first member, so all of
* net.h -- exact and bulk transfers, pumping, framing -- runs over TCP
* unchanged. Connections and listeners live in caller storage; nothing here
* allocates. TCP and unix-domain stream sockets are both served, the protocol
* taken from the endpoint.
*   Blocking throughout. EINTR is retried; SIGPIPE is suppressed on every
* send; every socket is close-on-exec; and closing a listener wakes an accept
* blocked on it in another thread. Native errors map onto d_net_error as the
* C++ tcp.hpp before this one mapped them.
*   Backends: BSD sockets and Winsock 2, chosen by env_tcp.h. Without one, or
* with D_CFG_TCP 0, the functions compile to stubs that report
* D_NET_ERROR_NETWORK_DOWN. The Winsock backend starts Winsock itself, once,
* on first use.
*   C99 or later; declarations have C linkage. Link tcp.c and net.c.
*
*
* path:      /inc/djinterp/net/tcp/tcp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SOCKETS
    -------
    1.  Handles
         1.  d_tcp_socket
         2.  D_TCP_SOCKET_INVALID
2.  TYPES
    -----
    1.  Options
         1.  d_tcp_options
    2.  Connections
         1.  d_tcp_connection
    3.  Listeners
         1.  d_tcp_listener
3.  THE BACKEND
    -----------
    1.  Availability
    2.  Native errors and sockets
4.  OPTIONS
    -------
    1.  Defaults
5.  CONNECTIONS
    -----------
    1.  Opening
    2.  Transfers
    3.  State and lifetime
6.  LISTENERS
    ---------
    1.  Opening
    2.  Accepting
    3.  State and lifetime
*/

#ifndef DJINTERP_NET_TCP_TCP_H
#define DJINTERP_NET_TCP_TCP_H 1

// std
#include <stddef.h>  // size_t
#include <stdint.h>  // intptr_t
// djinterp
#include "../../c/djinterp.h"              // framework root
#include "../../config/net/tcp/cfg_tcp.h"  // D_INTERNAL_TCP_*
#include "../net.h"                        // d_net_connection, d_net_endpoint


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SOCKETS
//==============================================================================
// A native socket, in a type wide enough for both backends: an int descriptor
// under BSD sockets, a SOCKET under Winsock. Neither backend's headers are
// needed to hold one.


// 1.1    Handles
//------------------------------------------------------------------------------
// 1.1.1
// d_tcp_socket
//   type: a native socket handle.
typedef intptr_t d_tcp_socket;

// 1.1.2
// D_TCP_SOCKET_INVALID
//   constant: the handle of no socket: -1 as a descriptor, INVALID_SOCKET as
// a SOCKET.
#define D_TCP_SOCKET_INVALID ((d_tcp_socket)-1)


//==============================================================================
// 2.  TYPES
//==============================================================================
// The structures are public so they can live in caller storage. Their
// fields are the module's: read them through the functions below.


// 2.1    Options
//------------------------------------------------------------------------------
// 2.1.1
// d_tcp_options
//   struct: how connections are opened and listeners bound. Fill it with
// d_tcp_options_init, then change what differs.
struct d_tcp_options
{
    long connect_timeout_ms;  // per address tried; 0 or less waits as the
                              // system does
    int  listen_backlog;      // listen()'s backlog
    bool no_delay;            // set TCP_NODELAY, disabling Nagle
    bool reuse_address;       // set SO_REUSEADDR on listeners
    bool reuse_port;          // set SO_REUSEPORT on listeners, where it exists
};

// 2.2    Connections
//------------------------------------------------------------------------------
// 2.2.1
// d_tcp_connection
//   struct: a connected stream socket, and the net.h connection over it.
// base comes first, so &connection->base is a d_net_connection* that every
// net.h function accepts. Both endpoints are recorded when the connection
// opens, and remain readable after it closes.
struct d_tcp_connection
{
    struct d_net_connection base;      // the net.h view; must stay first
    struct d_net_endpoint   remote;    // the peer; an empty host is unknown
    struct d_net_endpoint   local;     // this end; an empty host is unknown
    d_tcp_socket            socket;    // D_TCP_SOCKET_INVALID once closed
    enum d_net_protocol     protocol;  // TCP, UNIX, or an adopted UDP
};

// 2.3    Listeners
//------------------------------------------------------------------------------
// 2.3.1
// d_tcp_listener
//   struct: a bound, listening socket. Under BSD sockets it also holds a
// pipe whose write end d_tcp_listener_close uses to wake an accept blocked in
// another thread.
struct d_tcp_listener
{
    struct d_net_endpoint local;       // the bound address, or the path
    d_tcp_socket          socket;      // D_TCP_SOCKET_INVALID once closed
    d_tcp_socket          wake_read;   // the wake pipe's read end
    d_tcp_socket          wake_write;  // the wake pipe's write end
    enum d_net_protocol   protocol;    // TCP or UNIX
    bool                  no_delay;    // applied to accepted connections
    bool                  owns_path;   // a unix-domain path to remove
};


//==============================================================================
// 3.  THE BACKEND
//==============================================================================


// 3.1    Availability
//------------------------------------------------------------------------------
// d_tcp_available -- whether a socket backend is compiled in and, under
// Winsock, started; false for the stubs
bool d_tcp_available(void);

// 3.2    Native errors and sockets
//------------------------------------------------------------------------------
// what tcp.c does to its own codes and sockets, for code that makes sockets
// or reads errno itself, as reactor.hpp does

/**
 * @brief Maps a native socket error code onto d_net_error, as every function
 *        here maps its own.
 *
 * @param[in] _code  an errno value under BSD sockets, a WSAGetLastError()
 *                   value under Winsock; 0 means no error.
 * @return the matching error; D_NET_ERROR_UNKNOWN for a code without one,
 *         and from the stubs for any code but 0.
 */
enum d_net_error d_tcp_error_from_native(int _code);

// socket preparation -- close-on-exec (not inherited, under Winsock), and
// no SIGPIPE where the socket itself can say so (SO_NOSIGPIPE); each does
// nothing for D_TCP_SOCKET_INVALID, where it does not apply, and in the stubs
void d_tcp_socket_cloexec(d_tcp_socket _socket);
void d_tcp_socket_no_sigpipe(d_tcp_socket _socket);


//==============================================================================
// 4.  OPTIONS
//==============================================================================


// 4.1    Defaults
//------------------------------------------------------------------------------
// d_tcp_options_init -- no connect timeout, cfg_tcp.h's backlog, Nagle left
// on, SO_REUSEADDR on, SO_REUSEPORT off; a NULL argument is ignored
void d_tcp_options_init(struct d_tcp_options* _options);


//==============================================================================
// 5.  CONNECTIONS
//==============================================================================
// Every function accepts a closed connection. NULL is refused where an error
// can be reported and ignored elsewhere.


// 5.1    Opening
//------------------------------------------------------------------------------
// d_tcp_connection_init -- a closed connection: no socket, no endpoints, its
// net.h view ready; a NULL argument is ignored
void             d_tcp_connection_init(struct d_tcp_connection* _connection);

/**
 * @brief Opens a connection to an endpoint.
 *
 * A TCP endpoint's host is resolved, and each address it yields is tried in
 * turn until one connects. A unix-domain endpoint's host is the socket path.
 *
 * @param[out] _connection  receives the connection; closed on failure.
 * @param[in]  _endpoint    where to connect: TCP, or UNIX where
 *                          D_ENV_TCP_HAS_UNIX.
 * @param[in]  _options     connect_timeout_ms and no_delay apply; NULL
 *                          means the defaults.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_INVALID_ARGUMENT for a NULL
 *         argument or a UDP endpoint; D_NET_ERROR_ADDRESS_INVALID for a
 *         unix-domain endpoint without unix-domain support;
 *         D_NET_ERROR_TIMED_OUT when the timeout expires; otherwise the
 *         last address's error, or the resolver's.
 */
enum d_net_error d_tcp_connect(struct d_tcp_connection*     _connection,
                               const struct d_net_endpoint* _endpoint,
                               const struct d_tcp_options*  _options);

/**
 * @brief Makes a connection of a socket already connected.
 *
 * @param[out] _connection  receives the connection; closed on failure.
 * @param[in]  _socket      the socket, whose ownership passes to
 *                          _connection even when this fails.
 * @param[in]  _protocol    D_NET_PROTOCOL_TCP or D_NET_PROTOCOL_UNIX for a
 *                          stream socket, D_NET_PROTOCOL_UDP for a
 *                          connected datagram socket; IP endpoints are
 *                          recorded with it.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_INVALID_ARGUMENT for a NULL
 *         connection, an invalid socket, or an unknown protocol, the socket
 *         then being closed; D_NET_ERROR_NETWORK_DOWN from the stubs.
 */
enum d_net_error d_tcp_connection_adopt(struct d_tcp_connection* _connection,
                                        d_tcp_socket             _socket,
                                        enum d_net_protocol      _protocol);

// 5.2    Transfers
//------------------------------------------------------------------------------
/**
 * @brief Reads up to `_size` bytes, waiting until at least one arrives.
 *
 * @param[in,out] _connection  the connection.
 * @param[out]    _buffer      receives the bytes.
 * @param[in]     _size        its size.
 * @return the bytes read and D_NET_ERROR_NONE; a count of 0 with
 *         D_NET_ERROR_NONE at the end of the stream; D_NET_ERROR_CLOSED on
 *         a closed connection; D_NET_ERROR_WOULD_BLOCK from a non-blocking
 *         socket with nothing to read; otherwise the mapped native error.
 */
struct d_net_io_result d_tcp_connection_read(
                           struct d_tcp_connection* _connection,
                           void*                    _buffer,
                           size_t                   _size);

/**
 * @brief Writes up to `_size` bytes; a short count is not an error.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _data        the bytes.
 * @param[in]     _size        how many.
 * @return the bytes written and D_NET_ERROR_NONE; D_NET_ERROR_CLOSED on a
 *         closed connection; D_NET_ERROR_CONNECTION_RESET when the peer has
 *         gone; otherwise the mapped native error.
 */
struct d_net_io_result d_tcp_connection_write(
                           struct d_tcp_connection* _connection,
                           const void*              _data,
                           size_t                   _size);

// 5.3    State and lifetime
//------------------------------------------------------------------------------
// queries -- O(1), no system call; the endpoints were recorded at opening,
// and a query reports false, leaving _out alone, when one is unknown
bool             d_tcp_connection_is_open(
                     const struct d_tcp_connection* _connection);
bool             d_tcp_connection_remote_endpoint(
                     const struct d_tcp_connection* _connection,
                     struct d_net_endpoint*         _out);
bool             d_tcp_connection_local_endpoint(
                     const struct d_tcp_connection* _connection,
                     struct d_net_endpoint*         _out);
d_tcp_socket     d_tcp_connection_native(
                     const struct d_tcp_connection* _connection);

/**
 * @brief Closes one or both directions, leaving the socket open.
 *
 * @param[in,out] _connection  the connection.
 * @param[in]     _mode        the direction or directions.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_CLOSED on a closed connection;
 *         D_NET_ERROR_INVALID_ARGUMENT for NULL or an unknown mode;
 *         otherwise the mapped native error.
 */
enum d_net_error d_tcp_connection_shutdown(
                     struct d_tcp_connection* _connection,
                     enum d_net_shutdown      _mode);

/**
 * @brief Closes the socket. Closing a closed connection does nothing.
 *
 * @param[in,out] _connection  the connection; NULL is ignored.
 * @post The connection is closed; its endpoints remain readable.
 */
void             d_tcp_connection_close(
                     struct d_tcp_connection* _connection);

/**
 * @brief Hands the socket to the caller without closing it.
 *
 * @param[in,out] _connection  the connection.
 * @return the socket, now the caller's to close; D_TCP_SOCKET_INVALID for
 *         NULL or a closed connection.
 * @post The connection is closed, and closing it again does nothing.
 */
d_tcp_socket     d_tcp_connection_release(
                     struct d_tcp_connection* _connection);


//==============================================================================
// 6.  LISTENERS
//==============================================================================


// 6.1    Opening
//------------------------------------------------------------------------------
// d_tcp_listener_init -- a closed listener; a NULL argument is ignored
void             d_tcp_listener_init(struct d_tcp_listener* _listener);

/**
 * @brief Binds a listener to an endpoint and starts listening.
 *
 * For TCP an empty host means every local address, and port 0 an
 * ephemeral port, which d_tcp_listener_local_endpoint then reports. For
 * UNIX the host is the socket path; a file already there is removed first,
 * and the path is removed again when the listener closes. A listener
 * already open is closed first.
 *
 * @pre _listener was initialized by d_tcp_listener_init, or opened before:
 *      an open listener is closed first, so its fields must be real.
 *
 * @param[in,out] _listener  the listener; closed on failure.
 * @param[in]     _endpoint  where to listen.
 * @param[in]     _options   listen_backlog, reuse_address, reuse_port, and,
 *                           for accepted connections, no_delay apply; NULL
 *                           means the defaults.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_INVALID_ARGUMENT for a NULL
 *         argument or a UDP endpoint; D_NET_ERROR_ADDRESS_INVALID for a
 *         unix-domain endpoint without unix-domain support, or a path too
 *         long; D_NET_ERROR_ADDRESS_IN_USE; otherwise the mapped native or
 *         resolver error.
 */
enum d_net_error d_tcp_listen(struct d_tcp_listener*       _listener,
                              const struct d_net_endpoint* _endpoint,
                              const struct d_tcp_options*  _options);

// 6.2    Accepting
//------------------------------------------------------------------------------
/**
 * @brief Waits for a connection and accepts it.
 *
 * @param[in,out] _listener    the listener.
 * @param[out]    _connection  receives the connection; closed on failure.
 * @return D_NET_ERROR_NONE; D_NET_ERROR_CLOSED when the listener is closed,
 *         including by d_tcp_listener_close from another thread during the
 *         wait; D_NET_ERROR_INVALID_ARGUMENT for NULL; otherwise the mapped
 *         native error, which leaves the listener usable.
 */
enum d_net_error d_tcp_accept(struct d_tcp_listener*   _listener,
                              struct d_tcp_connection* _connection);

// 6.3    State and lifetime
//------------------------------------------------------------------------------
// queries -- O(1), no system call; the local endpoint was recorded when the
// listener bound
bool             d_tcp_listener_is_open(
                     const struct d_tcp_listener* _listener);
bool             d_tcp_listener_local_endpoint(
                     const struct d_tcp_listener* _listener,
                     struct d_net_endpoint*       _out);
d_tcp_socket     d_tcp_listener_native(
                     const struct d_tcp_listener* _listener);

/**
 * @brief Closes the listener, waking an accept blocked on it.
 *
 * Safe to call from another thread while d_tcp_accept waits, which then
 * returns D_NET_ERROR_CLOSED. The listener's storage must outlive both
 * calls. Closing a closed listener does nothing.
 *
 * @param[in,out] _listener  the listener; NULL is ignored.
 * @post The listener is closed, and a unix-domain path it created is gone.
 */
void             d_tcp_listener_close(struct d_tcp_listener* _listener);


D_EXTERN_C_END


#endif  // DJINTERP_NET_TCP_TCP_H
