/*******************************************************************************
* djinterp [net]                                                           tcp.c
*
* The TCP transport over BSD sockets or Winsock 2.
*   Implements tcp.h in parts the preprocessor chooses whole: a BSD layer and
* a Winsock layer, each defining the same static d_tcp_os_* functions; the
* socket code the two backends share, written against those functions; and
* stubs for a build without a backend. What needs no socket -- initializers,
* queries, and the d_net_connection table -- is common to every build.
*
*
* path:      /src/djinterp/net/tcp/tcp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
// enable POSIX.1-2008 (getaddrinfo, poll, inet_ntop) under a strict C
// compiler, and Windows Vista's API (WSAPoll, InitOnceExecuteOnce) under
// Winsock; a feature-test macro must precede every include, so these sit
// ahead of the corresponding header
#if defined(_WIN32) || defined(_WIN64)
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif  // _WIN32_WINNT
#else
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif  // _POSIX_C_SOURCE
#endif

#include "../../../../inc/djinterp/net/tcp/tcp.h"  // corresponding header
// std
#include <errno.h>   // errno, EINTR, EAGAIN, and the codes mapped below
#include <limits.h>  // INT_MAX
#include <stddef.h>  // offsetof
#include <string.h>  // memchr, memcpy, memset, strlen
// platform
#if (D_INTERNAL_TCP_BACKEND == D_ENV_TCP_BACKEND_BSD)
    #include <arpa/inet.h>    // inet_ntop, ntohs
    #include <fcntl.h>        // fcntl, FD_CLOEXEC, O_NONBLOCK
    #include <netdb.h>        // getaddrinfo, freeaddrinfo, EAI_*
    #include <netinet/in.h>   // sockaddr_in, sockaddr_in6, IPPROTO_TCP
    #include <poll.h>         // poll, struct pollfd
    #include <sys/socket.h>   // socket, bind, listen, accept, connect
    #include <sys/un.h>       // sockaddr_un
    #include <unistd.h>       // close, pipe, write, unlink
    #if D_ENV_TCP_HAS_NODELAY
        #include <netinet/tcp.h>  // TCP_NODELAY
    #endif  // D_ENV_TCP_HAS_NODELAY
#elif (D_INTERNAL_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK)
    #include <winsock2.h>     // SOCKET, WSAStartup, WSAPoll, closesocket
    #include <ws2tcpip.h>     // getaddrinfo, inet_ntop, socklen_t
#endif  // D_INTERNAL_TCP_BACKEND


#if (D_INTERNAL_TCP_BACKEND == D_ENV_TCP_BACKEND_BSD)

// d_tcp_native
//   type: the BSD sockets handle, a descriptor.
typedef int d_tcp_native;

// D_INTERNAL_TCP_SEND_FLAGS
//   macro: MSG_NOSIGNAL where the system has it, so a send to a departed
// peer reports EPIPE rather than raising SIGPIPE. SO_NOSIGPIPE covers the
// systems without it.
#ifdef MSG_NOSIGNAL
    #define D_INTERNAL_TCP_SEND_FLAGS MSG_NOSIGNAL
#else
    #define D_INTERNAL_TCP_SEND_FLAGS 0
#endif  // MSG_NOSIGNAL

/*
d_tcp_os_handle
  Narrows a d_tcp_socket to the descriptor it holds.
*/
static d_tcp_native
d_tcp_os_handle(
    d_tcp_socket _socket
)
{
    return (d_tcp_native)_socket;
}

/*
d_tcp_os_map
  Maps an errno value onto d_net_error, as tcp.hpp did. EWOULDBLOCK is tested
ahead of the switch: it equals EAGAIN on most systems, and a duplicate case
label would not compile.
*/
static enum d_net_error
d_tcp_os_map(
    int _code
)
{
    // EWOULDBLOCK may be EAGAIN, so it cannot have a case of its own
    if ( (_code == EAGAIN) ||
         (_code == EWOULDBLOCK) )
    {
        return D_NET_ERROR_WOULD_BLOCK;
    }

    switch (_code)
    {
        case 0:
            return D_NET_ERROR_NONE;
        case EINTR:
            return D_NET_ERROR_INTERRUPTED;
        case ECONNRESET:
        case EPIPE:
            return D_NET_ERROR_CONNECTION_RESET;
        case ECONNREFUSED:
            return D_NET_ERROR_CONNECTION_REFUSED;
        case ECONNABORTED:
            return D_NET_ERROR_CONNECTION_ABORTED;
        case ETIMEDOUT:
            return D_NET_ERROR_TIMED_OUT;
        case EADDRINUSE:
            return D_NET_ERROR_ADDRESS_IN_USE;
        case EADDRNOTAVAIL:
        case EAFNOSUPPORT:
            return D_NET_ERROR_ADDRESS_INVALID;
        case EHOSTUNREACH:
            return D_NET_ERROR_HOST_UNREACHABLE;
        case ENETUNREACH:
        case ENETDOWN:
            return D_NET_ERROR_NETWORK_DOWN;
        case EACCES:
        case EPERM:
            return D_NET_ERROR_ACCESS_DENIED;
        case EINVAL:
        case EFAULT:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case EMFILE:
        case ENFILE:
            return D_NET_ERROR_TOO_MANY_OPEN_FILES;
        case ENOMEM:
        case ENOBUFS:
            return D_NET_ERROR_OUT_OF_MEMORY;
        default:
            return D_NET_ERROR_UNKNOWN;
    }
}

/*
d_tcp_os_error
  The last socket call's error.
*/
static enum d_net_error
d_tcp_os_error(void)
{
    return d_tcp_os_map(errno);
}

/*
d_tcp_os_interrupted
  Whether the last socket call was interrupted by a signal.
*/
static bool
d_tcp_os_interrupted(void)
{
    return (errno == EINTR);
}

/*
d_tcp_os_in_progress
  Whether the last connect is still completing, as a non-blocking one does.
*/
static bool
d_tcp_os_in_progress(void)
{
    return (errno == EINPROGRESS);
}

/*
d_tcp_os_resolver_system
  Whether a getaddrinfo failure is EAI_SYSTEM, whose cause is left in errno.
*/
static bool
d_tcp_os_resolver_system(
    int _code
)
{
    return (_code == EAI_SYSTEM);
}

/*
d_tcp_os_startup
  BSD sockets need no starting.
*/
static bool
d_tcp_os_startup(void)
{
    return true;
}

/*
d_tcp_os_cloexec
  Marks a descriptor close-on-exec; a failure leaves it as it was.
*/
static void
d_tcp_os_cloexec(
    d_tcp_native _handle
)
{
    const int flags = fcntl(_handle,
                            F_GETFD,
                            0);

    // an unreadable flag set is left alone
    if (flags >= 0)
    {
        (void)fcntl(_handle,
                    F_SETFD,
                    flags | FD_CLOEXEC);
    }

    return;
}

/*
d_tcp_os_no_sigpipe
  Sets SO_NOSIGPIPE where the system has it, as macOS and the BSDs do.
Elsewhere each send passes MSG_NOSIGNAL instead.
*/
static void
d_tcp_os_no_sigpipe(
    d_tcp_native _handle
)
{
#ifdef SO_NOSIGPIPE
    const int value = 1;

    (void)setsockopt(_handle,
                     SOL_SOCKET,
                     SO_NOSIGPIPE,
                     &value,
                     (socklen_t)sizeof(value));
#else
    (void)_handle;
#endif  // SO_NOSIGPIPE

    return;
}

/*
d_tcp_os_prepare
  Readies a new socket: close-on-exec, and without SIGPIPE where the socket
itself can say so.
*/
static void
d_tcp_os_prepare(
    d_tcp_native _handle
)
{
    d_tcp_os_cloexec(_handle);
    d_tcp_os_no_sigpipe(_handle);

    return;
}

/*
d_tcp_os_open
  A new, prepared stream socket of an address family.
*/
static d_tcp_socket
d_tcp_os_open(
    int _family
)
{
    const d_tcp_native handle = socket(_family,
                                       SOCK_STREAM,
                                       0);

    // a failure leaves errno for the caller
    if (handle < 0)
    {
        return D_TCP_SOCKET_INVALID;
    }

    d_tcp_os_prepare(handle);

    return (d_tcp_socket)handle;
}

/*
d_tcp_os_accept
  Accepts one waiting connection from a listening socket, prepared as a new
socket is.
*/
static d_tcp_socket
d_tcp_os_accept(
    d_tcp_socket _listening
)
{
    const d_tcp_native accepted = accept(d_tcp_os_handle(_listening),
                                         NULL,
                                         NULL);

    // a failure leaves errno for the caller
    if (accepted < 0)
    {
        return D_TCP_SOCKET_INVALID;
    }

    d_tcp_os_prepare(accepted);

    return (d_tcp_socket)accepted;
}

/*
d_tcp_os_close
  Closes a socket. EINTR is not retried: the descriptor is released anyway.
*/
static void
d_tcp_os_close(
    d_tcp_socket _socket
)
{
    (void)close(d_tcp_os_handle(_socket));

    return;
}

/*
d_tcp_os_receive
  One recv, retried while signals interrupt it.
*/
static struct d_net_io_result
d_tcp_os_receive(
    d_tcp_socket _socket,
    void*        _buffer,
    size_t       _size
)
{
    // retry what a signal interrupted
    for (;;)
    {
        const ssize_t received = recv(d_tcp_os_handle(_socket),
                                      _buffer,
                                      _size,
                                      0);

        // bytes, or the end of the stream
        if (received >= 0)
        {
            return d_net_io_result_make((size_t)received,
                                        D_NET_ERROR_NONE);
        }

        // anything but an interruption is the answer
        if (!d_tcp_os_interrupted())
        {
            return d_net_io_result_make(0u,
                                        d_tcp_os_error());
        }
    }
}

/*
d_tcp_os_send
  One send, retried while signals interrupt it.
*/
static struct d_net_io_result
d_tcp_os_send(
    d_tcp_socket _socket,
    const void*  _data,
    size_t       _size
)
{
    // retry what a signal interrupted
    for (;;)
    {
        const ssize_t sent = send(d_tcp_os_handle(_socket),
                                  _data,
                                  _size,
                                  D_INTERNAL_TCP_SEND_FLAGS);

        // what the system took
        if (sent >= 0)
        {
            return d_net_io_result_make((size_t)sent,
                                        D_NET_ERROR_NONE);
        }

        // anything but an interruption is the answer
        if (!d_tcp_os_interrupted())
        {
            return d_net_io_result_make(0u,
                                        d_tcp_os_error());
        }
    }
}

/*
d_tcp_os_set_blocking
  Switches a socket between blocking and non-blocking mode.
*/
static bool
d_tcp_os_set_blocking(
    d_tcp_socket _socket,
    bool         _blocking
)
{
    const d_tcp_native handle = d_tcp_os_handle(_socket);
    const int          flags  = fcntl(handle,
                                      F_GETFL,
                                      0);

    // changing one flag needs the others
    if (flags < 0)
    {
        return false;
    }

    return (fcntl(handle,
                  F_SETFL,
                  (_blocking) ? (flags & ~O_NONBLOCK)
                              : (flags | O_NONBLOCK)) == 0);
}

/*
d_tcp_os_poll
  poll, which BSD sockets and Winsock share but for its name.
*/
static int
d_tcp_os_poll(
    struct pollfd* _watched,
    unsigned int   _count,
    int            _timeout_ms
)
{
    return poll(_watched,
                (nfds_t)_count,
                _timeout_ms);
}

/*
d_tcp_os_wake_open
  Gives a listener its wake pipe, both ends close-on-exec. d_tcp_listener_close
writes to it, so an accept blocked in poll wakes.
*/
static bool
d_tcp_os_wake_open(
    struct d_tcp_listener* _listener
)
{
    // the pipe's read and write ends
    int ends[2] = { -1, -1 };

    // without the pipe, closing could not wake a blocked accept
    if (pipe(ends) != 0)
    {
        return false;
    }

    d_tcp_os_cloexec(ends[0]);
    d_tcp_os_cloexec(ends[1]);
    _listener->wake_read  = (d_tcp_socket)ends[0];
    _listener->wake_write = (d_tcp_socket)ends[1];

    return true;
}

/*
d_tcp_os_wake_signal
  Writes the wake pipe's one byte.
*/
static void
d_tcp_os_wake_signal(
    struct d_tcp_listener* _listener
)
{
    const unsigned char byte = 1u;

    // a listener without a pipe has no one to wake
    if (_listener->wake_write != D_TCP_SOCKET_INVALID)
    {
        const ssize_t written = write(d_tcp_os_handle(_listener->wake_write),
                                      &byte,
                                      1u);

        (void)written;
    }

    return;
}

/*
d_tcp_os_wake_close
  Closes both ends of a listener's wake pipe.
*/
static void
d_tcp_os_wake_close(
    struct d_tcp_listener* _listener
)
{
    // each end is closed only if it was opened
    if (_listener->wake_read != D_TCP_SOCKET_INVALID)
    {
        d_tcp_os_close(_listener->wake_read);
    }

    // likewise the write end
    if (_listener->wake_write != D_TCP_SOCKET_INVALID)
    {
        d_tcp_os_close(_listener->wake_write);
    }

    _listener->wake_read  = D_TCP_SOCKET_INVALID;
    _listener->wake_write = D_TCP_SOCKET_INVALID;

    return;
}

/*
d_tcp_os_await_accept
  Waits until the listener has a connection to accept or its wake pipe is
written. The pipe is examined first, so a close always wins; any event on
it, a hang-up included, counts as a wake.
*/
static enum d_net_error
d_tcp_os_await_accept(
    struct d_tcp_listener* _listener
)
{
    struct pollfd watched[2] =
    {
        { .fd      = d_tcp_os_handle(_listener->socket),
          .events  = POLLIN,
          .revents = 0 },
        { .fd      = d_tcp_os_handle(_listener->wake_read),
          .events  = POLLIN,
          .revents = 0 }
    };

    // wait, retrying what a signal interrupts
    for (;;)
    {
        const int ready = d_tcp_os_poll(watched,
                                        2u,
                                        -1);

        // a failed wait is the system's error, unless a signal caused it
        if (ready < 0)
        {
            // an interrupted wait is resumed
            if (d_tcp_os_interrupted())
            {
                continue;
            }

            return d_tcp_os_error();
        }

        // the listener is closing
        if (watched[1].revents != 0)
        {
            return D_NET_ERROR_CLOSED;
        }

        // a connection is waiting, or the socket has failed
        if (watched[0].revents != 0)
        {
            return ((watched[0].revents & POLLIN) != 0)
                       ? D_NET_ERROR_NONE
                       : D_NET_ERROR_UNKNOWN;
        }
    }
}

/*
d_tcp_os_unix_address
  Builds a sockaddr_un from a unix-domain endpoint's path, refusing a path
empty or too long for sun_path with its terminator.
*/
static enum d_net_error
d_tcp_os_unix_address(
    const struct d_net_endpoint* _endpoint,
    struct sockaddr_storage*     _address,
    socklen_t*                   _length
)
{
    const struct d_pack_text path  = d_net_endpoint_host(_endpoint);
    struct sockaddr_un*      local = (struct sockaddr_un*)_address;

    // the path and its terminator must fit
    if ( (path.length == 0u) ||
         (path.length >= sizeof(local->sun_path)) )
    {
        return D_NET_ERROR_ADDRESS_INVALID;
    }

    memset(_address,
           0,
           sizeof(*_address));
    local->sun_family = AF_UNIX;
    memcpy(local->sun_path,
           path.data,
           path.length);
    *_length = (socklen_t)sizeof(*local);

    return D_NET_ERROR_NONE;
}

/*
d_tcp_os_endpoint_other
  Records an address that is neither IPv4 nor IPv6: a named unix-domain
socket's path. An unnamed one, as a unix-domain client usually is, has no
endpoint.
*/
static bool
d_tcp_os_endpoint_other(
    const struct sockaddr_storage* _address,
    socklen_t                      _length,
    struct d_net_endpoint*         _out
)
{
    const struct sockaddr_un* local = (const struct sockaddr_un*)_address;
    const size_t              start = offsetof(struct sockaddr_un,
                                               sun_path);

    // only a named unix-domain socket is recorded here
    if ( (_address->ss_family != AF_UNIX) ||
         ((size_t)_length <= start)       ||
         (local->sun_path[0] == '\0') )
    {
        return false;
    }

    // the path ends at its terminator, or where the address ends
    const size_t   room = (size_t)_length - start;
    const char*    end  = memchr(local->sun_path,
                                 '\0',
                                 room);
    const struct d_pack_text path = { local->sun_path,
                                      (end) ? (size_t)(end - local->sun_path)
                                            : room };

    return (d_net_endpoint_set_local(_out,
                                     path) == D_NET_ERROR_NONE);
}

/*
d_tcp_os_remove_path
  Removes the file at a unix-domain endpoint's path, if there is one.
*/
static void
d_tcp_os_remove_path(
    const struct d_net_endpoint* _endpoint
)
{
    (void)unlink(_endpoint->host);

    return;
}

/*
d_tcp_os_shutdown_how
  The shutdown() direction for a mode, or -1 for no mode.
*/
static int
d_tcp_os_shutdown_how(
    enum d_net_shutdown _mode
)
{
    switch (_mode)
    {
        case D_NET_SHUTDOWN_READ:
            return SHUT_RD;
        case D_NET_SHUTDOWN_WRITE:
            return SHUT_WR;
        case D_NET_SHUTDOWN_BOTH:
            return SHUT_RDWR;
        default:
            return -1;
    }
}

#elif (D_INTERNAL_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK)

// d_tcp_native
//   type: the Winsock handle, a SOCKET.
typedef SOCKET d_tcp_native;

// d_tcp_winsock_once, d_tcp_winsock_started
//   state: Winsock is started once, on first use, and never stopped; the
// process's exit releases it.
static INIT_ONCE d_tcp_winsock_once    = INIT_ONCE_STATIC_INIT;
static bool      d_tcp_winsock_started = false;

/*
d_tcp_os_handle
  Narrows a d_tcp_socket to the SOCKET it holds.
*/
static d_tcp_native
d_tcp_os_handle(
    d_tcp_socket _socket
)
{
    return (d_tcp_native)_socket;
}

/*
d_tcp_os_map
  Maps a Winsock error onto d_net_error, following the BSD layer's mapping
code for code.
*/
static enum d_net_error
d_tcp_os_map(
    int _code
)
{
    switch (_code)
    {
        case 0:
            return D_NET_ERROR_NONE;
        case WSAEINTR:
            return D_NET_ERROR_INTERRUPTED;
        case WSAEWOULDBLOCK:
            return D_NET_ERROR_WOULD_BLOCK;
        case WSAECONNRESET:
        case WSAESHUTDOWN:
            return D_NET_ERROR_CONNECTION_RESET;
        case WSAECONNREFUSED:
            return D_NET_ERROR_CONNECTION_REFUSED;
        case WSAECONNABORTED:
            return D_NET_ERROR_CONNECTION_ABORTED;
        case WSAETIMEDOUT:
            return D_NET_ERROR_TIMED_OUT;
        case WSAEADDRINUSE:
            return D_NET_ERROR_ADDRESS_IN_USE;
        case WSAEADDRNOTAVAIL:
        case WSAEAFNOSUPPORT:
            return D_NET_ERROR_ADDRESS_INVALID;
        case WSAEHOSTUNREACH:
            return D_NET_ERROR_HOST_UNREACHABLE;
        case WSAENETUNREACH:
        case WSAENETDOWN:
        case WSANOTINITIALISED:
            return D_NET_ERROR_NETWORK_DOWN;
        case WSAEACCES:
            return D_NET_ERROR_ACCESS_DENIED;
        case WSAEINVAL:
        case WSAEFAULT:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case WSAEMFILE:
            return D_NET_ERROR_TOO_MANY_OPEN_FILES;
        case WSAENOBUFS:
        case WSA_NOT_ENOUGH_MEMORY:
            return D_NET_ERROR_OUT_OF_MEMORY;
        default:
            return D_NET_ERROR_UNKNOWN;
    }
}

/*
d_tcp_os_error
  The last socket call's error.
*/
static enum d_net_error
d_tcp_os_error(void)
{
    return d_tcp_os_map(WSAGetLastError());
}

/*
d_tcp_os_interrupted
  Whether the last socket call was interrupted, as closesocket interrupts a
call blocked on the same socket.
*/
static bool
d_tcp_os_interrupted(void)
{
    return (WSAGetLastError() == WSAEINTR);
}

/*
d_tcp_os_in_progress
  Whether the last connect is still completing: Winsock reports a
non-blocking connect under way as WSAEWOULDBLOCK.
*/
static bool
d_tcp_os_in_progress(void)
{
    return (WSAGetLastError() == WSAEWOULDBLOCK);
}

/*
d_tcp_os_resolver_system
  Winsock reports resolver failures directly; none defers to another code.
*/
static bool
d_tcp_os_resolver_system(
    int _code
)
{
    (void)_code;

    return false;
}

/*
d_tcp_winsock_start
  The one-time initializer: starts Winsock 2.2, WINSOCK_VERSION, and
records whether it did.
*/
static BOOL CALLBACK
d_tcp_winsock_start(
    PINIT_ONCE _once,
    PVOID      _parameter,
    PVOID*     _context
)
{
    WSADATA data;

    (void)_once;
    (void)_parameter;
    (void)_context;
    d_tcp_winsock_started = (WSAStartup(WINSOCK_VERSION,
                                        &data) == 0);

    return TRUE;
}

/*
d_tcp_os_startup
  Starts Winsock on first use; InitOnceExecuteOnce makes that thread-safe.
*/
static bool
d_tcp_os_startup(void)
{
    (void)InitOnceExecuteOnce(&d_tcp_winsock_once,
                              d_tcp_winsock_start,
                              NULL,
                              NULL);

    return d_tcp_winsock_started;
}

/*
d_tcp_os_cloexec
  Keeps a socket from being inherited by child processes, Winsock's
close-on-exec.
*/
static void
d_tcp_os_cloexec(
    d_tcp_native _handle
)
{
    (void)SetHandleInformation((HANDLE)_handle,
                               HANDLE_FLAG_INHERIT,
                               0);

    return;
}

/*
d_tcp_os_no_sigpipe
  Nothing to do: Winsock never raises SIGPIPE.
*/
static void
d_tcp_os_no_sigpipe(
    d_tcp_native _handle
)
{
    (void)_handle;

    return;
}

/*
d_tcp_os_prepare
  Readies a new socket: not inherited by child processes.
*/
static void
d_tcp_os_prepare(
    d_tcp_native _handle
)
{
    d_tcp_os_cloexec(_handle);
    d_tcp_os_no_sigpipe(_handle);

    return;
}

/*
d_tcp_os_open
  A new, prepared stream socket of an address family.
*/
static d_tcp_socket
d_tcp_os_open(
    int _family
)
{
    const d_tcp_native handle = socket(_family,
                                       SOCK_STREAM,
                                       0);

    // a failure leaves its code for WSAGetLastError
    if (handle == INVALID_SOCKET)
    {
        return D_TCP_SOCKET_INVALID;
    }

    d_tcp_os_prepare(handle);

    return (d_tcp_socket)handle;
}

/*
d_tcp_os_accept
  Accepts one waiting connection from a listening socket, prepared as a new
socket is.
*/
static d_tcp_socket
d_tcp_os_accept(
    d_tcp_socket _listening
)
{
    const d_tcp_native accepted = accept(d_tcp_os_handle(_listening),
                                         NULL,
                                         NULL);

    // a failure leaves its code for WSAGetLastError
    if (accepted == INVALID_SOCKET)
    {
        return D_TCP_SOCKET_INVALID;
    }

    d_tcp_os_prepare(accepted);

    return (d_tcp_socket)accepted;
}

/*
d_tcp_os_close
  Closes a socket.
*/
static void
d_tcp_os_close(
    d_tcp_socket _socket
)
{
    (void)closesocket(d_tcp_os_handle(_socket));

    return;
}

/*
d_tcp_os_receive
  One recv of at most INT_MAX bytes, Winsock's limit.
*/
static struct d_net_io_result
d_tcp_os_receive(
    d_tcp_socket _socket,
    void*        _buffer,
    size_t       _size
)
{
    const int want     = (_size > (size_t)INT_MAX) ? INT_MAX
                                                   : (int)_size;
    const int received = recv(d_tcp_os_handle(_socket),
                              (char*)_buffer,
                              want,
                              0);

    // a failure's code is the answer
    if (received == SOCKET_ERROR)
    {
        return d_net_io_result_make(0u,
                                    d_tcp_os_error());
    }

    return d_net_io_result_make((size_t)received,
                                D_NET_ERROR_NONE);
}

/*
d_tcp_os_send
  One send of at most INT_MAX bytes, Winsock's limit.
*/
static struct d_net_io_result
d_tcp_os_send(
    d_tcp_socket _socket,
    const void*  _data,
    size_t       _size
)
{
    const int want = (_size > (size_t)INT_MAX) ? INT_MAX
                                               : (int)_size;
    const int sent = send(d_tcp_os_handle(_socket),
                          (const char*)_data,
                          want,
                          0);

    // a failure's code is the answer
    if (sent == SOCKET_ERROR)
    {
        return d_net_io_result_make(0u,
                                    d_tcp_os_error());
    }

    return d_net_io_result_make((size_t)sent,
                                D_NET_ERROR_NONE);
}

/*
d_tcp_os_set_blocking
  Switches a socket between blocking and non-blocking mode.
*/
static bool
d_tcp_os_set_blocking(
    d_tcp_socket _socket,
    bool         _blocking
)
{
    // FIONBIO is unsigned in the headers, while the parameter is a long
    const long request = (long)FIONBIO;
    u_long     mode    = (_blocking) ? 0u
                                     : 1u;

    return (ioctlsocket(d_tcp_os_handle(_socket),
                        request,
                        &mode) == 0);
}

/*
d_tcp_os_poll
  WSAPoll, Winsock's poll.
*/
static int
d_tcp_os_poll(
    struct pollfd* _watched,
    unsigned int   _count,
    int            _timeout_ms
)
{
    return WSAPoll(_watched,
                   (ULONG)_count,
                   _timeout_ms);
}

/*
d_tcp_os_wake_open
  Winsock needs no wake pipe: closesocket cancels an accept blocked on the
socket, so the pipe's ends stay invalid.
*/
static bool
d_tcp_os_wake_open(
    struct d_tcp_listener* _listener
)
{
    (void)_listener;

    return true;
}

/*
d_tcp_os_wake_signal
  Nothing to write; see d_tcp_os_wake_open.
*/
static void
d_tcp_os_wake_signal(
    struct d_tcp_listener* _listener
)
{
    (void)_listener;

    return;
}

/*
d_tcp_os_wake_close
  Nothing to close; see d_tcp_os_wake_open.
*/
static void
d_tcp_os_wake_close(
    struct d_tcp_listener* _listener
)
{
    (void)_listener;

    return;
}

/*
d_tcp_os_await_accept
  Nothing to await: accept itself blocks, and closesocket interrupts it.
*/
static enum d_net_error
d_tcp_os_await_accept(
    struct d_tcp_listener* _listener
)
{
    (void)_listener;

    return D_NET_ERROR_NONE;
}

/*
d_tcp_os_unix_address
  Unix-domain sockets are not served under Winsock.
*/
static enum d_net_error
d_tcp_os_unix_address(
    const struct d_net_endpoint* _endpoint,
    struct sockaddr_storage*     _address,
    socklen_t*                   _length
)
{
    (void)_endpoint;
    (void)_address;
    (void)_length;

    return D_NET_ERROR_ADDRESS_INVALID;
}

/*
d_tcp_os_endpoint_other
  Only IPv4 and IPv6 addresses have endpoints under Winsock.
*/
static bool
d_tcp_os_endpoint_other(
    const struct sockaddr_storage* _address,
    socklen_t                      _length,
    struct d_net_endpoint*         _out
)
{
    (void)_address;
    (void)_length;
    (void)_out;

    return false;
}

/*
d_tcp_os_remove_path
  No unix-domain path is ever created under Winsock.
*/
static void
d_tcp_os_remove_path(
    const struct d_net_endpoint* _endpoint
)
{
    (void)_endpoint;

    return;
}

/*
d_tcp_os_shutdown_how
  The shutdown() direction for a mode, or -1 for no mode.
*/
static int
d_tcp_os_shutdown_how(
    enum d_net_shutdown _mode
)
{
    switch (_mode)
    {
        case D_NET_SHUTDOWN_READ:
            return SD_RECEIVE;
        case D_NET_SHUTDOWN_WRITE:
            return SD_SEND;
        case D_NET_SHUTDOWN_BOTH:
            return SD_BOTH;
        default:
            return -1;
    }
}

#endif  // D_INTERNAL_TCP_BACKEND

#if (D_INTERNAL_TCP_BACKEND != D_ENV_TCP_BACKEND_NONE)

/*
d_tcp_resolver_error
  Maps a getaddrinfo failure onto d_net_error, as tcp.hpp did: a name that
does not resolve is an unreachable host, and a temporary failure a timeout.
*/
static enum d_net_error
d_tcp_resolver_error(
    int _code
)
{
    // the platform's catch-all defers to the system's own error
    if (d_tcp_os_resolver_system(_code))
    {
        return d_tcp_os_error();
    }

    switch (_code)
    {
        case EAI_AGAIN:
            return D_NET_ERROR_TIMED_OUT;
        case EAI_MEMORY:
            return D_NET_ERROR_OUT_OF_MEMORY;
        case EAI_NONAME:
        case EAI_FAIL:
            return D_NET_ERROR_HOST_UNREACHABLE;
        default:
            return D_NET_ERROR_ADDRESS_INVALID;
    }
}

/*
d_tcp_set_option
  Sets an int socket option to 0 or 1. A failure is ignored, as tcp.hpp
ignored it: the options only tune a working socket.
*/
static void
d_tcp_set_option(
    d_tcp_socket _socket,
    int          _level,
    int          _name,
    bool         _on
)
{
    const int value = (_on) ? 1
                            : 0;

    (void)setsockopt(d_tcp_os_handle(_socket),
                     _level,
                     _name,
                     (const char*)&value,
                     (socklen_t)sizeof(value));

    return;
}

/*
d_tcp_set_no_delay
  Sets TCP_NODELAY where the platform has it.
*/
static void
d_tcp_set_no_delay(
    d_tcp_socket _socket
)
{
#if D_ENV_TCP_HAS_NODELAY
    d_tcp_set_option(_socket,
                     IPPROTO_TCP,
                     TCP_NODELAY,
                     true);
#else
    (void)_socket;
#endif  // D_ENV_TCP_HAS_NODELAY

    return;
}

/*
d_tcp_set_reuse_port
  Sets SO_REUSEPORT where the platform has it.
*/
static void
d_tcp_set_reuse_port(
    d_tcp_socket _socket
)
{
#ifdef SO_REUSEPORT
    d_tcp_set_option(_socket,
                     SOL_SOCKET,
                     SO_REUSEPORT,
                     true);
#else
    (void)_socket;
#endif  // SO_REUSEPORT

    return;
}

/*
d_tcp_endpoint_numeric
  Records an IP address as numeric text, a port, and a protocol.
*/
static bool
d_tcp_endpoint_numeric(
    int                    _family,
    const void*            _address,
    d_net_port             _port,
    enum d_net_protocol    _protocol,
    struct d_net_endpoint* _out
)
{
    // room for the longest numeric address, IPv6's, and its terminator
    char text[INET6_ADDRSTRLEN] = { 0 };

    // a family inet_ntop cannot print has no endpoint
    if (!inet_ntop(_family,
                   _address,
                   text,
                   (socklen_t)sizeof(text)))
    {
        return false;
    }

    const struct d_pack_text host = { text,
                                      strlen(text) };

    return (d_net_endpoint_set(_out,
                               host,
                               _port,
                               _protocol) == D_NET_ERROR_NONE);
}

/*
d_tcp_endpoint_from
  Records an address as an endpoint: IPv4 and IPv6 here, marked UDP for a
datagram socket and TCP otherwise, and anything else by the platform layer.
An address it cannot express leaves _out empty.
*/
static bool
d_tcp_endpoint_from(
    const struct sockaddr_storage* _address,
    socklen_t                      _length,
    enum d_net_protocol            _protocol,
    struct d_net_endpoint*         _out
)
{
    const enum d_net_protocol ip = (_protocol == D_NET_PROTOCOL_UDP)
                                       ? D_NET_PROTOCOL_UDP
                                       : D_NET_PROTOCOL_TCP;

    d_net_endpoint_init(_out);

    // IPv4: the address and port of a sockaddr_in
    if (_address->ss_family == AF_INET)
    {
        const struct sockaddr_in* ip4 = (const struct sockaddr_in*)_address;

        return d_tcp_endpoint_numeric(AF_INET,
                                      &ip4->sin_addr,
                                      ntohs(ip4->sin_port),
                                      ip,
                                      _out);
    }

    // IPv6 likewise
    if (_address->ss_family == AF_INET6)
    {
        const struct sockaddr_in6* ip6 = (const struct sockaddr_in6*)_address;

        return d_tcp_endpoint_numeric(AF_INET6,
                                      &ip6->sin6_addr,
                                      ntohs(ip6->sin6_port),
                                      ip,
                                      _out);
    }

    return d_tcp_os_endpoint_other(_address,
                                   _length,
                                   _out);
}

/*
d_tcp_record_endpoints
  Records both ends of a connection's socket, leaving empty any the system
cannot name; a unix-domain client is usually unnamed.
*/
static void
d_tcp_record_endpoints(
    struct d_tcp_connection* _connection
)
{
    const d_tcp_native      handle  = d_tcp_os_handle(_connection->socket);
    struct sockaddr_storage address = { 0 };
    socklen_t               length  = (socklen_t)sizeof(address);

    d_net_endpoint_init(&_connection->remote);
    d_net_endpoint_init(&_connection->local);

    // the peer, where it has a name
    if (getpeername(handle,
                    (struct sockaddr*)&address,
                    &length) == 0)
    {
        (void)d_tcp_endpoint_from(&address,
                                  length,
                                  _connection->protocol,
                                  &_connection->remote);
    }

    length = (socklen_t)sizeof(address);

    // this end, likewise
    if (getsockname(handle,
                    (struct sockaddr*)&address,
                    &length) == 0)
    {
        (void)d_tcp_endpoint_from(&address,
                                  length,
                                  _connection->protocol,
                                  &_connection->local);
    }

    return;
}

/*
d_tcp_take
  Makes a closed connection the owner of a connected socket.
*/
static enum d_net_error
d_tcp_take(
    struct d_tcp_connection* _connection,
    d_tcp_socket             _socket,
    enum d_net_protocol      _protocol
)
{
    _connection->socket   = _socket;
    _connection->protocol = _protocol;
    d_tcp_record_endpoints(_connection);

    return D_NET_ERROR_NONE;
}

/*
d_tcp_wait_writable
  Waits until a socket is writable, which is how a connect under way
reports finishing, or until the timeout expires; a negative timeout waits
indefinitely. A signal restarts the wait with its whole timeout, which can
stretch it.
*/
static enum d_net_error
d_tcp_wait_writable(
    d_tcp_socket _socket,
    int          _wait_ms
)
{
    struct pollfd watched = { .fd      = d_tcp_os_handle(_socket),
                              .events  = POLLOUT,
                              .revents = 0 };

    // wait, retrying what a signal interrupts
    for (;;)
    {
        const int ready = d_tcp_os_poll(&watched,
                                        1u,
                                        _wait_ms);

        // finished, one way or the other
        if (ready > 0)
        {
            return D_NET_ERROR_NONE;
        }

        // out of time
        if (ready == 0)
        {
            return D_NET_ERROR_TIMED_OUT;
        }

        // anything but an interruption is the answer
        if (!d_tcp_os_interrupted())
        {
            return d_tcp_os_error();
        }
    }
}

/*
d_tcp_pending_error
  A finished connect's outcome: the socket's pending error, SO_ERROR.
*/
static enum d_net_error
d_tcp_pending_error(
    d_tcp_socket _socket
)
{
    int       code   = 0;
    socklen_t length = (socklen_t)sizeof(code);

    // an unreadable outcome is the system's error
    if (getsockopt(d_tcp_os_handle(_socket),
                   SOL_SOCKET,
                   SO_ERROR,
                   (char*)&code,
                   &length) != 0)
    {
        return d_tcp_os_error();
    }

    return d_tcp_os_map(code);
}

/*
d_tcp_connect_once
  One connect. One that has not finished at once -- under way on a
non-blocking socket, or interrupted by a signal, after which it continues
in the background -- is finished by waiting for the socket to become
writable and reading its pending error; calling connect again would not
report the outcome.
*/
static enum d_net_error
d_tcp_connect_once(
    d_tcp_socket           _socket,
    const struct sockaddr* _address,
    socklen_t              _length,
    int                    _wait_ms
)
{
    // connected at once
    if (connect(d_tcp_os_handle(_socket),
                _address,
                _length) == 0)
    {
        return D_NET_ERROR_NONE;
    }

    // under way: wait for the outcome
    if ( (d_tcp_os_in_progress()) ||
         (d_tcp_os_interrupted()) )
    {
        const enum d_net_error ready = d_tcp_wait_writable(_socket,
                                                           _wait_ms);

        return (ready == D_NET_ERROR_NONE) ? d_tcp_pending_error(_socket)
                                           : ready;
    }

    return d_tcp_os_error();
}

/*
d_tcp_connect_socket
  Connects a socket to one address. Without a timeout the connect blocks;
with one the socket is non-blocking for the connect, and blocking again
after, as every socket here is.
*/
static enum d_net_error
d_tcp_connect_socket(
    d_tcp_socket           _socket,
    const struct sockaddr* _address,
    socklen_t              _length,
    long                   _timeout_ms
)
{
    const bool timed = (_timeout_ms > 0);
    const int  wait  = (!timed)                   ? -1
                     : (_timeout_ms > INT_MAX)    ? INT_MAX
                                                  : (int)_timeout_ms;

    // a timed connect must not block
    if ( (timed) &&
         (!d_tcp_os_set_blocking(_socket,
                                 false)) )
    {
        return d_tcp_os_error();
    }

    const enum d_net_error result = d_tcp_connect_once(_socket,
                                                       _address,
                                                       _length,
                                                       wait);

    // the socket blocks again
    if (timed)
    {
        (void)d_tcp_os_set_blocking(_socket,
                                    true);
    }

    return result;
}

/*
d_tcp_resolve
  Resolves an endpoint into stream addresses: passive ones to bind, where an
empty host means every local address, or active ones to connect to, where
it means loopback.
*/
static enum d_net_error
d_tcp_resolve(
    const struct d_net_endpoint* _endpoint,
    bool                         _passive,
    struct addrinfo**            _out
)
{
    // the port as text, as getaddrinfo takes it
    char port[D_NET_PORT_TEXT_MAX + 1] = { 0 };

    const struct addrinfo hints = { .ai_flags    = (_passive) ? AI_PASSIVE
                                                              : 0,
                                    .ai_family   = AF_UNSPEC,
                                    .ai_socktype = SOCK_STREAM };

    (void)d_net_port_format(_endpoint->port,
                            port,
                            sizeof(port));

    const int code = getaddrinfo((_endpoint->host[0] != '\0') ? _endpoint->host
                                                              : NULL,
                                 port,
                                 &hints,
                                 _out);

    return (code == 0) ? D_NET_ERROR_NONE
                       : d_tcp_resolver_error(code);
}

/*
d_tcp_connect_address
  Opens a socket for one resolved address and connects it.
*/
static enum d_net_error
d_tcp_connect_address(
    struct d_tcp_connection*    _connection,
    const struct addrinfo*      _entry,
    const struct d_tcp_options* _options
)
{
    const d_tcp_socket candidate = d_tcp_os_open(_entry->ai_family);

    // a family that cannot be opened fails this address
    if (candidate == D_TCP_SOCKET_INVALID)
    {
        return d_tcp_os_error();
    }

    // Nagle is disabled before the first byte
    if (_options->no_delay)
    {
        d_tcp_set_no_delay(candidate);
    }

    const enum d_net_error result = d_tcp_connect_socket(
                                        candidate,
                                        _entry->ai_addr,
                                        (socklen_t)_entry->ai_addrlen,
                                        _options->connect_timeout_ms);

    // a failed connect's socket is discarded
    if (result != D_NET_ERROR_NONE)
    {
        d_tcp_os_close(candidate);

        return result;
    }

    return d_tcp_take(_connection,
                      candidate,
                      D_NET_PROTOCOL_TCP);
}

/*
d_tcp_connect_tcp
  Tries each resolved address in turn, stopping at the first to connect;
the last failure is reported when none does.
*/
static enum d_net_error
d_tcp_connect_tcp(
    struct d_tcp_connection*     _connection,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    struct addrinfo*       results = NULL;
    const enum d_net_error found   = d_tcp_resolve(_endpoint,
                                                   false,
                                                   &results);

    // an unresolved host has no address to try
    if (found != D_NET_ERROR_NONE)
    {
        return found;
    }

    enum d_net_error last = D_NET_ERROR_HOST_UNREACHABLE;

    // try each address until one connects
    for (const struct addrinfo* entry = results;
         entry != NULL;
         entry = entry->ai_next)
    {
        last = d_tcp_connect_address(_connection,
                                     entry,
                                     _options);

        // the first connection ends the search
        if (last == D_NET_ERROR_NONE)
        {
            break;
        }
    }

    freeaddrinfo(results);

    return last;
}

/*
d_tcp_connect_unix
  Connects to a unix-domain socket's path.
*/
static enum d_net_error
d_tcp_connect_unix(
    struct d_tcp_connection*     _connection,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    struct sockaddr_storage address = { 0 };
    socklen_t               length  = 0;
    const enum d_net_error  built   = d_tcp_os_unix_address(_endpoint,
                                                            &address,
                                                            &length);

    // a path the platform cannot address fails before a socket exists
    if (built != D_NET_ERROR_NONE)
    {
        return built;
    }

    const d_tcp_socket candidate = d_tcp_os_open(address.ss_family);

    // a socket that cannot be opened ends the attempt
    if (candidate == D_TCP_SOCKET_INVALID)
    {
        return d_tcp_os_error();
    }

    const enum d_net_error result = d_tcp_connect_socket(
                                        candidate,
                                        (const struct sockaddr*)&address,
                                        length,
                                        _options->connect_timeout_ms);

    // a failed connect's socket is discarded
    if (result != D_NET_ERROR_NONE)
    {
        d_tcp_os_close(candidate);

        return result;
    }

    return d_tcp_take(_connection,
                      candidate,
                      D_NET_PROTOCOL_UNIX);
}

/*
d_tcp_available
  Starting the backend is the test of it: BSD sockets always pass, Winsock
passes once started.
*/
bool
d_tcp_available(void)
{
    return d_tcp_os_startup();
}

/*
d_tcp_error_from_native
  The platform layer's mapping, shared with every other function here.
*/
enum d_net_error
d_tcp_error_from_native(
    int _code
)
{
    return d_tcp_os_map(_code);
}

/*
d_tcp_socket_cloexec
  The platform layer's close-on-exec, for a socket made elsewhere.
*/
void
d_tcp_socket_cloexec(
    d_tcp_socket _socket
)
{
    // no socket, nothing to mark
    if (_socket != D_TCP_SOCKET_INVALID)
    {
        d_tcp_os_cloexec(d_tcp_os_handle(_socket));
    }

    return;
}

/*
d_tcp_socket_no_sigpipe
  The platform layer's per-socket SIGPIPE suppression, for a socket made
elsewhere.
*/
void
d_tcp_socket_no_sigpipe(
    d_tcp_socket _socket
)
{
    // no socket, nothing to mark
    if (_socket != D_TCP_SOCKET_INVALID)
    {
        d_tcp_os_no_sigpipe(d_tcp_os_handle(_socket));
    }

    return;
}

/*
d_tcp_connect
  Validates, starts the backend, and dispatches on the protocol. The
connection is closed first, so every failure leaves it closed.
*/
enum d_net_error
d_tcp_connect(
    struct d_tcp_connection*     _connection,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    // parameter validation
    if ( (!_connection) ||
         (!_endpoint) )
    {
        d_tcp_connection_init(_connection);

        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    struct d_tcp_options defaults = { .connect_timeout_ms = 0 };

    d_tcp_options_init(&defaults);
    d_tcp_connection_init(_connection);

    // Winsock starts here on first use
    if (!d_tcp_os_startup())
    {
        return D_NET_ERROR_NETWORK_DOWN;
    }

    const struct d_tcp_options* chosen = (_options) ? _options
                                                    : &defaults;

    switch (_endpoint->protocol)
    {
        case D_NET_PROTOCOL_TCP:
            return d_tcp_connect_tcp(_connection,
                                     _endpoint,
                                     chosen);
        case D_NET_PROTOCOL_UNIX:
            return d_tcp_connect_unix(_connection,
                                      _endpoint,
                                      chosen);
        default:
            return D_NET_ERROR_INVALID_ARGUMENT;
    }
}

/*
d_tcp_connection_adopt
  A refused socket is closed rather than leaked, since its ownership passed
with the call. UDP is accepted so a connected datagram socket can be
wrapped, as tcp.hpp's socket_connection allowed.
*/
enum d_net_error
d_tcp_connection_adopt(
    struct d_tcp_connection* _connection,
    d_tcp_socket             _socket,
    enum d_net_protocol      _protocol
)
{
    d_tcp_connection_init(_connection);

    // parameter validation; a refused socket is still the callee's
    if ( (!_connection)                          ||
         (_socket == D_TCP_SOCKET_INVALID)       ||
         ( (_protocol != D_NET_PROTOCOL_TCP)  &&
           (_protocol != D_NET_PROTOCOL_UDP)  &&
           (_protocol != D_NET_PROTOCOL_UNIX) ) )
    {
        // a real socket is closed, not leaked
        if (_socket != D_TCP_SOCKET_INVALID)
        {
            d_tcp_os_close(_socket);
        }

        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    return d_tcp_take(_connection,
                      _socket,
                      _protocol);
}

/*
d_tcp_connection_read
  A zero-byte read is answered here: recv would return 0, which reads as the
end of the stream.
*/
struct d_net_io_result
d_tcp_connection_read(
    struct d_tcp_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    // parameter validation
    if ( (!_connection) ||
         ( (!_buffer) &&
           (_size != 0u) ) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_INVALID_ARGUMENT);
    }

    // a closed connection has nothing to read
    if (_connection->socket == D_TCP_SOCKET_INVALID)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // nothing asked for, nothing read
    if (_size == 0u)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_NONE);
    }

    return d_tcp_os_receive(_connection->socket,
                            _buffer,
                            _size);
}

/*
d_tcp_connection_write
  A zero-byte write is answered here, never reaching the system.
*/
struct d_net_io_result
d_tcp_connection_write(
    struct d_tcp_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    // parameter validation
    if ( (!_connection) ||
         ( (!_data) &&
           (_size != 0u) ) )
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_INVALID_ARGUMENT);
    }

    // a closed connection takes nothing
    if (_connection->socket == D_TCP_SOCKET_INVALID)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_CLOSED);
    }

    // nothing given, nothing written
    if (_size == 0u)
    {
        return d_net_io_result_make(0u,
                                    D_NET_ERROR_NONE);
    }

    return d_tcp_os_send(_connection->socket,
                         _data,
                         _size);
}

/*
d_tcp_connection_shutdown
  shutdown() on the socket, its direction translated.
*/
enum d_net_error
d_tcp_connection_shutdown(
    struct d_tcp_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    // parameter validation
    if (!_connection)
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // a closed connection has no direction left
    if (_connection->socket == D_TCP_SOCKET_INVALID)
    {
        return D_NET_ERROR_CLOSED;
    }

    const int how = d_tcp_os_shutdown_how(_mode);

    // an unknown mode is refused
    if (how < 0)
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    return (shutdown(d_tcp_os_handle(_connection->socket),
                     how) == 0) ? D_NET_ERROR_NONE
                                : d_tcp_os_error();
}

/*
d_tcp_connection_close
  The socket is marked closed before it is released, so the connection
never names a socket another open may reuse.
*/
void
d_tcp_connection_close(
    struct d_tcp_connection* _connection
)
{
    // NULL, and a closed connection, need nothing
    if ( (_connection) &&
         (_connection->socket != D_TCP_SOCKET_INVALID) )
    {
        const d_tcp_socket closing = _connection->socket;

        _connection->socket = D_TCP_SOCKET_INVALID;
        d_tcp_os_close(closing);
    }

    return;
}

/*
d_tcp_bind_listen
  Binds a socket to an address and starts it listening.
*/
static enum d_net_error
d_tcp_bind_listen(
    d_tcp_socket           _socket,
    const struct sockaddr* _address,
    socklen_t              _length,
    int                    _backlog
)
{
    const d_tcp_native handle = d_tcp_os_handle(_socket);

    // the address first
    if (bind(handle,
             _address,
             _length) != 0)
    {
        return d_tcp_os_error();
    }

    return (listen(handle,
                   _backlog) == 0) ? D_NET_ERROR_NONE
                                   : d_tcp_os_error();
}

/*
d_tcp_bind_address
  Opens a socket for one resolved address, sets the reuse options, and binds
and listens.
*/
static enum d_net_error
d_tcp_bind_address(
    struct d_tcp_listener*      _listener,
    const struct addrinfo*      _entry,
    const struct d_tcp_options* _options
)
{
    const d_tcp_socket candidate = d_tcp_os_open(_entry->ai_family);

    // a family that cannot be opened fails this address
    if (candidate == D_TCP_SOCKET_INVALID)
    {
        return d_tcp_os_error();
    }

    d_tcp_set_option(candidate,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     _options->reuse_address);

    // SO_REUSEPORT is only ever turned on
    if (_options->reuse_port)
    {
        d_tcp_set_reuse_port(candidate);
    }

    const enum d_net_error bound = d_tcp_bind_listen(
                                       candidate,
                                       _entry->ai_addr,
                                       (socklen_t)_entry->ai_addrlen,
                                       _options->listen_backlog);

    // a socket that failed to bind is discarded
    if (bound != D_NET_ERROR_NONE)
    {
        d_tcp_os_close(candidate);

        return bound;
    }

    _listener->socket   = candidate;
    _listener->protocol = D_NET_PROTOCOL_TCP;

    return D_NET_ERROR_NONE;
}

/*
d_tcp_listen_tcp
  Binds the first resolved address that accepts it; the last failure is
reported when none does.
*/
static enum d_net_error
d_tcp_listen_tcp(
    struct d_tcp_listener*       _listener,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    struct addrinfo*       results = NULL;
    const enum d_net_error found   = d_tcp_resolve(_endpoint,
                                                   true,
                                                   &results);

    // an unresolved host has no address to bind
    if (found != D_NET_ERROR_NONE)
    {
        return found;
    }

    enum d_net_error last = D_NET_ERROR_ADDRESS_INVALID;

    // bind the first address that allows it
    for (const struct addrinfo* entry = results;
         entry != NULL;
         entry = entry->ai_next)
    {
        last = d_tcp_bind_address(_listener,
                                  entry,
                                  _options);

        // the first bound address ends the search
        if (last == D_NET_ERROR_NONE)
        {
            break;
        }
    }

    freeaddrinfo(results);

    return last;
}

/*
d_tcp_listen_unix
  Binds a unix-domain path. A file already at the path, typically left by a
listener that did not close, is removed first, as tcp.hpp removed it.
*/
static enum d_net_error
d_tcp_listen_unix(
    struct d_tcp_listener*       _listener,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    struct sockaddr_storage address = { 0 };
    socklen_t               length  = 0;
    const enum d_net_error  built   = d_tcp_os_unix_address(_endpoint,
                                                            &address,
                                                            &length);

    // a path the platform cannot address fails before a socket exists
    if (built != D_NET_ERROR_NONE)
    {
        return built;
    }

    const d_tcp_socket candidate = d_tcp_os_open(address.ss_family);

    // a socket that cannot be opened ends the attempt
    if (candidate == D_TCP_SOCKET_INVALID)
    {
        return d_tcp_os_error();
    }

    d_tcp_os_remove_path(_endpoint);

    const enum d_net_error bound = d_tcp_bind_listen(
                                       candidate,
                                       (const struct sockaddr*)&address,
                                       length,
                                       _options->listen_backlog);

    // a socket that failed to bind is discarded
    if (bound != D_NET_ERROR_NONE)
    {
        d_tcp_os_close(candidate);

        return bound;
    }

    _listener->socket    = candidate;
    _listener->protocol  = D_NET_PROTOCOL_UNIX;
    _listener->owns_path = true;

    return d_net_endpoint_set_local(&_listener->local,
                                    d_net_endpoint_host(_endpoint));
}

/*
d_tcp_listen_protocol
  Binds by the endpoint's protocol.
*/
static enum d_net_error
d_tcp_listen_protocol(
    struct d_tcp_listener*       _listener,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    switch (_endpoint->protocol)
    {
        case D_NET_PROTOCOL_TCP:
            return d_tcp_listen_tcp(_listener,
                                    _endpoint,
                                    _options);
        case D_NET_PROTOCOL_UNIX:
            return d_tcp_listen_unix(_listener,
                                     _endpoint,
                                     _options);
        default:
            return D_NET_ERROR_INVALID_ARGUMENT;
    }
}

/*
d_tcp_record_listener
  Records a TCP listener's bound address, which reveals an ephemeral port. A
unix-domain listener recorded its path when it bound.
*/
static void
d_tcp_record_listener(
    struct d_tcp_listener* _listener
)
{
    struct sockaddr_storage address = { 0 };
    socklen_t               length  = (socklen_t)sizeof(address);

    // only TCP needs asking
    if ( (_listener->protocol == D_NET_PROTOCOL_TCP) &&
         (getsockname(d_tcp_os_handle(_listener->socket),
                      (struct sockaddr*)&address,
                      &length) == 0) )
    {
        (void)d_tcp_endpoint_from(&address,
                                  length,
                                  D_NET_PROTOCOL_TCP,
                                  &_listener->local);
    }

    return;
}

/*
d_tcp_listen
  A listener already open is closed first. The wake pipe comes after the
bind, so a failed bind leaves nothing to undo but the socket.
*/
enum d_net_error
d_tcp_listen(
    struct d_tcp_listener*       _listener,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    // parameter validation
    if ( (!_listener) ||
         (!_endpoint) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    struct d_tcp_options defaults = { .connect_timeout_ms = 0 };

    d_tcp_options_init(&defaults);
    d_tcp_listener_close(_listener);
    d_tcp_listener_init(_listener);

    const struct d_tcp_options* chosen = (_options) ? _options
                                                    : &defaults;
    const enum d_net_error      bound  = (d_tcp_os_startup())
                                  ? d_tcp_listen_protocol(_listener,
                                                          _endpoint,
                                                          chosen)
                                  : D_NET_ERROR_NETWORK_DOWN;

    // without an open socket there is nothing more to do
    if (bound != D_NET_ERROR_NONE)
    {
        return bound;
    }

    // without its wake pipe, the listener could not be closed from afar
    if (!d_tcp_os_wake_open(_listener))
    {
        const enum d_net_error failed = d_tcp_os_error();

        d_tcp_listener_close(_listener);

        return failed;
    }

    _listener->no_delay = chosen->no_delay;
    d_tcp_record_listener(_listener);

    return D_NET_ERROR_NONE;
}

/*
d_tcp_welcome
  Makes a connection of an accepted socket, disabling Nagle first when the
listener was asked to.
*/
static enum d_net_error
d_tcp_welcome(
    struct d_tcp_listener*   _listener,
    struct d_tcp_connection* _connection,
    d_tcp_socket             _accepted
)
{
    // Nagle applies to TCP alone
    if ( (_listener->no_delay) &&
         (_listener->protocol == D_NET_PROTOCOL_TCP) )
    {
        d_tcp_set_no_delay(_accepted);
    }

    return d_tcp_take(_connection,
                      _accepted,
                      _listener->protocol);
}

/*
d_tcp_accept
  Waits, then accepts. The listener's socket is read afresh each time round,
so a close from another thread is seen however the wait ends.
*/
enum d_net_error
d_tcp_accept(
    struct d_tcp_listener*   _listener,
    struct d_tcp_connection* _connection
)
{
    d_tcp_connection_init(_connection);

    // parameter validation
    if ( (!_listener) ||
         (!_connection) )
    {
        return D_NET_ERROR_INVALID_ARGUMENT;
    }

    // wait and accept, retrying what signals interrupt
    for (;;)
    {
        // a closed listener accepts nothing
        if (_listener->socket == D_TCP_SOCKET_INVALID)
        {
            return D_NET_ERROR_CLOSED;
        }

        const enum d_net_error ready = d_tcp_os_await_accept(_listener);

        // a woken or failed wait ends here
        if (ready != D_NET_ERROR_NONE)
        {
            return (_listener->socket == D_TCP_SOCKET_INVALID)
                       ? D_NET_ERROR_CLOSED
                       : ready;
        }

        const d_tcp_socket accepted = d_tcp_os_accept(_listener->socket);

        // a connection
        if (accepted != D_TCP_SOCKET_INVALID)
        {
            return d_tcp_welcome(_listener,
                                 _connection,
                                 accepted);
        }

        // anything but an interruption is the answer
        if (!d_tcp_os_interrupted())
        {
            return (_listener->socket == D_TCP_SOCKET_INVALID)
                       ? D_NET_ERROR_CLOSED
                       : d_tcp_os_error();
        }
    }
}

/*
d_tcp_listener_close
  The socket is marked closed first, so an accept woken in another thread
reads it as closed; the wake byte goes out before the descriptors close.
*/
void
d_tcp_listener_close(
    struct d_tcp_listener* _listener
)
{
    // NULL, and a closed listener, need nothing
    if ( (!_listener) ||
         (_listener->socket == D_TCP_SOCKET_INVALID) )
    {
        return;
    }

    const d_tcp_socket listening = _listener->socket;

    _listener->socket = D_TCP_SOCKET_INVALID;
    d_tcp_os_wake_signal(_listener);
    (void)shutdown(d_tcp_os_handle(listening),
                   d_tcp_os_shutdown_how(D_NET_SHUTDOWN_BOTH));
    d_tcp_os_close(listening);
    d_tcp_os_wake_close(_listener);

    // a unix-domain path the listener created goes with it
    if (_listener->owns_path)
    {
        d_tcp_os_remove_path(&_listener->local);
        _listener->owns_path = false;
    }

    return;
}

#else

/*
d_tcp_available
  No backend was built.
*/
bool
d_tcp_available(void)
{
    return false;
}

/*
d_tcp_error_from_native
  Without a backend no native code has a meaning, bar success.
*/
enum d_net_error
d_tcp_error_from_native(
    int _code
)
{
    return (_code == 0) ? D_NET_ERROR_NONE
                        : D_NET_ERROR_UNKNOWN;
}

/*
d_tcp_socket_cloexec
  Without a backend there is no socket to mark.
*/
void
d_tcp_socket_cloexec(
    d_tcp_socket _socket
)
{
    (void)_socket;

    return;
}

/*
d_tcp_socket_no_sigpipe
  Without a backend there is no socket to mark.
*/
void
d_tcp_socket_no_sigpipe(
    d_tcp_socket _socket
)
{
    (void)_socket;

    return;
}

/*
d_tcp_connect
  Without a backend no connection can open; the arguments are still
checked, as the backends check them.
*/
enum d_net_error
d_tcp_connect(
    struct d_tcp_connection*     _connection,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    (void)_options;
    d_tcp_connection_init(_connection);

    return ( (_connection) &&
             (_endpoint) ) ? D_NET_ERROR_NETWORK_DOWN
                           : D_NET_ERROR_INVALID_ARGUMENT;
}

/*
d_tcp_connection_adopt
  Without a backend there is no socket to adopt.
*/
enum d_net_error
d_tcp_connection_adopt(
    struct d_tcp_connection* _connection,
    d_tcp_socket             _socket,
    enum d_net_protocol      _protocol
)
{
    (void)_protocol;
    d_tcp_connection_init(_connection);

    return ( (_connection) &&
             (_socket != D_TCP_SOCKET_INVALID) )
               ? D_NET_ERROR_NETWORK_DOWN
               : D_NET_ERROR_INVALID_ARGUMENT;
}

/*
d_tcp_connection_read
  A stub connection is never open.
*/
struct d_net_io_result
d_tcp_connection_read(
    struct d_tcp_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    (void)_buffer;
    (void)_size;

    return d_net_io_result_make(0u,
                                (_connection) ? D_NET_ERROR_CLOSED
                                              : D_NET_ERROR_INVALID_ARGUMENT);
}

/*
d_tcp_connection_write
  A stub connection is never open.
*/
struct d_net_io_result
d_tcp_connection_write(
    struct d_tcp_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    (void)_data;
    (void)_size;

    return d_net_io_result_make(0u,
                                (_connection) ? D_NET_ERROR_CLOSED
                                              : D_NET_ERROR_INVALID_ARGUMENT);
}

/*
d_tcp_connection_shutdown
  A stub connection is never open.
*/
enum d_net_error
d_tcp_connection_shutdown(
    struct d_tcp_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    (void)_mode;

    return (_connection) ? D_NET_ERROR_CLOSED
                         : D_NET_ERROR_INVALID_ARGUMENT;
}

/*
d_tcp_connection_close
  A stub connection holds no socket to close.
*/
void
d_tcp_connection_close(
    struct d_tcp_connection* _connection
)
{
    // only the marker changes
    if (_connection)
    {
        _connection->socket = D_TCP_SOCKET_INVALID;
    }

    return;
}

/*
d_tcp_listen
  Without a backend no listener can open.
*/
enum d_net_error
d_tcp_listen(
    struct d_tcp_listener*       _listener,
    const struct d_net_endpoint* _endpoint,
    const struct d_tcp_options*  _options
)
{
    (void)_options;
    d_tcp_listener_init(_listener);

    return ( (_listener) &&
             (_endpoint) ) ? D_NET_ERROR_NETWORK_DOWN
                           : D_NET_ERROR_INVALID_ARGUMENT;
}

/*
d_tcp_accept
  A stub listener is never open.
*/
enum d_net_error
d_tcp_accept(
    struct d_tcp_listener*   _listener,
    struct d_tcp_connection* _connection
)
{
    d_tcp_connection_init(_connection);

    return ( (_listener) &&
             (_connection) ) ? D_NET_ERROR_CLOSED
                             : D_NET_ERROR_INVALID_ARGUMENT;
}

/*
d_tcp_listener_close
  A stub listener holds no socket to close.
*/
void
d_tcp_listener_close(
    struct d_tcp_listener* _listener
)
{
    // only the marker changes
    if (_listener)
    {
        _listener->socket = D_TCP_SOCKET_INVALID;
    }

    return;
}

#endif  // D_INTERNAL_TCP_BACKEND

/*
d_tcp_options_init
  The defaults tcp.hpp's socket_options had, the backlog from cfg_tcp.h.
*/
void
d_tcp_options_init(
    struct d_tcp_options* _options
)
{
    // NULL is ignored
    if (_options)
    {
        _options->connect_timeout_ms = 0;
        _options->listen_backlog     = D_INTERNAL_TCP_LISTEN_BACKLOG;
        _options->no_delay           = false;
        _options->reuse_address      = true;
        _options->reuse_port         = false;
    }

    return;
}

/*
d_tcp_read_operation
  The table's read: a d_net_connection here is the base, and so the start,
of a d_tcp_connection.
*/
static struct d_net_io_result
d_tcp_read_operation(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    return d_tcp_connection_read((struct d_tcp_connection*)_connection,
                                 _buffer,
                                 _size);
}

/*
d_tcp_write_operation
  The table's write, as d_tcp_read_operation.
*/
static struct d_net_io_result
d_tcp_write_operation(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    return d_tcp_connection_write((struct d_tcp_connection*)_connection,
                                  _data,
                                  _size);
}

/*
d_tcp_is_open_operation
  The table's is_open, as d_tcp_read_operation.
*/
static bool
d_tcp_is_open_operation(
    const struct d_net_connection* _connection
)
{
    return d_tcp_connection_is_open(
               (const struct d_tcp_connection*)_connection);
}

/*
d_tcp_close_operation
  The table's close, as d_tcp_read_operation.
*/
static void
d_tcp_close_operation(
    struct d_net_connection* _connection
)
{
    d_tcp_connection_close((struct d_tcp_connection*)_connection);

    return;
}

/*
d_tcp_shutdown_operation
  The table's shutdown, as d_tcp_read_operation.
*/
static enum d_net_error
d_tcp_shutdown_operation(
    struct d_net_connection* _connection,
    enum d_net_shutdown      _mode
)
{
    return d_tcp_connection_shutdown((struct d_tcp_connection*)_connection,
                                     _mode);
}

/*
d_tcp_remote_operation
  The table's remote_endpoint, as d_tcp_read_operation.
*/
static bool
d_tcp_remote_operation(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    return d_tcp_connection_remote_endpoint(
               (const struct d_tcp_connection*)_connection,
               _out);
}

/*
d_tcp_local_operation
  The table's local_endpoint, as d_tcp_read_operation.
*/
static bool
d_tcp_local_operation(
    const struct d_net_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    return d_tcp_connection_local_endpoint(
               (const struct d_tcp_connection*)_connection,
               _out);
}

/*
TCP_OPERATIONS
  The table every TCP connection's base points at. It has no destroy: a
connection's storage is its owner's.
*/
static const struct d_net_connection_vtable TCP_OPERATIONS =
{
    d_tcp_read_operation,
    d_tcp_write_operation,
    d_tcp_is_open_operation,
    d_tcp_close_operation,
    d_tcp_shutdown_operation,
    d_tcp_remote_operation,
    d_tcp_local_operation,
    NULL
};

/*
d_tcp_connection_init
  The table is complete, so d_net_connection_init cannot refuse it.
*/
void
d_tcp_connection_init(
    struct d_tcp_connection* _connection
)
{
    // NULL is ignored
    if (_connection)
    {
        (void)d_net_connection_init(&_connection->base,
                                    &TCP_OPERATIONS);
        d_net_endpoint_init(&_connection->remote);
        d_net_endpoint_init(&_connection->local);
        _connection->socket   = D_TCP_SOCKET_INVALID;
        _connection->protocol = D_NET_PROTOCOL_TCP;
    }

    return;
}

/*
d_tcp_copy_endpoint
  Copies a recorded endpoint out, if there is one; an empty host was never
recorded.
*/
static bool
d_tcp_copy_endpoint(
    const struct d_net_endpoint* _recorded,
    struct d_net_endpoint*       _out
)
{
    // nothing recorded, or nowhere to put it
    if ( (!_out) ||
         (_recorded->host[0] == '\0') )
    {
        return false;
    }

    *_out = *_recorded;

    return true;
}

/*
d_tcp_connection_is_open
  Open while it holds a socket.
*/
bool
d_tcp_connection_is_open(
    const struct d_tcp_connection* _connection
)
{
    return ( (_connection) &&
             (_connection->socket != D_TCP_SOCKET_INVALID) );
}

/*
d_tcp_connection_remote_endpoint
  The peer recorded at opening.
*/
bool
d_tcp_connection_remote_endpoint(
    const struct d_tcp_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    return ( (_connection) &&
             (d_tcp_copy_endpoint(&_connection->remote,
                                  _out)) );
}

/*
d_tcp_connection_local_endpoint
  This end, recorded at opening.
*/
bool
d_tcp_connection_local_endpoint(
    const struct d_tcp_connection* _connection,
    struct d_net_endpoint*         _out
)
{
    return ( (_connection) &&
             (d_tcp_copy_endpoint(&_connection->local,
                                  _out)) );
}

/*
d_tcp_connection_native
  The socket, or D_TCP_SOCKET_INVALID.
*/
d_tcp_socket
d_tcp_connection_native(
    const struct d_tcp_connection* _connection
)
{
    return (_connection) ? _connection->socket
                         : D_TCP_SOCKET_INVALID;
}

/*
d_tcp_connection_release
  Forgets the socket without closing it.
*/
d_tcp_socket
d_tcp_connection_release(
    struct d_tcp_connection* _connection
)
{
    // NULL holds no socket
    if (!_connection)
    {
        return D_TCP_SOCKET_INVALID;
    }

    const d_tcp_socket released = _connection->socket;

    _connection->socket = D_TCP_SOCKET_INVALID;

    return released;
}

/*
d_tcp_listener_init
  Closed, with no socket, no wake pipe, and nothing recorded.
*/
void
d_tcp_listener_init(
    struct d_tcp_listener* _listener
)
{
    // NULL is ignored
    if (_listener)
    {
        d_net_endpoint_init(&_listener->local);
        _listener->socket     = D_TCP_SOCKET_INVALID;
        _listener->wake_read  = D_TCP_SOCKET_INVALID;
        _listener->wake_write = D_TCP_SOCKET_INVALID;
        _listener->protocol   = D_NET_PROTOCOL_TCP;
        _listener->no_delay   = false;
        _listener->owns_path  = false;
    }

    return;
}

/*
d_tcp_listener_is_open
  Open while it holds a socket.
*/
bool
d_tcp_listener_is_open(
    const struct d_tcp_listener* _listener
)
{
    return ( (_listener) &&
             (_listener->socket != D_TCP_SOCKET_INVALID) );
}

/*
d_tcp_listener_local_endpoint
  The address recorded when the listener bound.
*/
bool
d_tcp_listener_local_endpoint(
    const struct d_tcp_listener* _listener,
    struct d_net_endpoint*       _out
)
{
    return ( (_listener) &&
             (d_tcp_copy_endpoint(&_listener->local,
                                  _out)) );
}

/*
d_tcp_listener_native
  The listening socket, or D_TCP_SOCKET_INVALID.
*/
d_tcp_socket
d_tcp_listener_native(
    const struct d_tcp_listener* _listener
)
{
    return (_listener) ? _listener->socket
                       : D_TCP_SOCKET_INVALID;
}
