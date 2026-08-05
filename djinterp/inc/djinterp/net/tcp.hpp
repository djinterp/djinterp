/******************************************************************************
* djinterp [net]                                                       tcp.hpp
*
*   POSIX / BSD-sockets backend for the net subframework. It supplies the
* concrete transport the generic foundation is written against: a socket-backed
* connection plus a connector and an acceptor that model the net::connector and
* net::acceptor concepts, so net::client / net::server / net::serve drive real
* TCP (and unix-domain) sockets unchanged. This is to net.hpp what curl.hpp is
* to web.hpp -- the platform layer beneath the abstraction.
*
* CONTENTS (namespace djinterp::net):
*   internal:   errno / getaddrinfo mapping, socket option helpers, address
*               conversion, a timeout-capable connect
*   socket_options    tunables (nodelay, reuse addr/port, connect timeout,
*                     listen backlog)
*   socket_connection a connection over a connected socket descriptor
*   tcp_connector     models connector -- resolves + connects (TCP or unix)
*   tcp_acceptor      models acceptor  -- binds/listens/accepts, with a
*                     self-pipe so close() unblocks a pending accept()
*
*   ROBUSTNESS: EINTR is retried throughout; errno is mapped to io_error;
* SIGPIPE is suppressed per-send (MSG_NOSIGNAL) or per-socket (SO_NOSIGPIPE);
* descriptors are set close-on-exec; the listener carries SO_REUSEADDR. An
* optional non-blocking connect timeout guards against a hung connect. Blocking
* throughout -- a non-blocking / reactor backend is a separate module that can
* reuse the readiness backend env_net.h detects.
*
*   Requires:  net.hpp, env_net.h, and a BSD sockets platform
*              (D_ENV_NET_HAS_BSD_SOCKETS). A Winsock backend is a sibling file.
*
* path:      /inc/djinterp/net/      tcp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_NET_TCP_
#define DJINTERP_NET_TCP_ 1

// djinterp
#include "./net.hpp"
#include "../core/env/net/env_net.h"


#if !D_ENV_NET_HAS_BSD_SOCKETS
    #error "net/tcp.hpp requires a BSD sockets platform (see env_net.h)"
#endif


// std / POSIX
#include <cerrno>
#include <cstring>
#include <string>
#include <utility>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                        INTERNAL HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // from_errno
    //   function: maps a POSIX errno value onto the neutral io_error taxonomy.
    D_NODISCARD inline io_error
    from_errno(
        int _errnum
    )
    {
        switch (_errnum)
        {
            case 0:
                return io_error::none;

            case EINTR:
                return io_error::interrupted;

            // EAGAIN and EWOULDBLOCK are frequently the same value; list the
            // second only when it differs to avoid a duplicate case label
            case EAGAIN:
        #if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
        #endif
                return io_error::would_block;

            case ECONNRESET:
            case EPIPE:
                return io_error::connection_reset;

            case ECONNREFUSED:
                return io_error::connection_refused;

            case ECONNABORTED:
                return io_error::connection_aborted;

            case ETIMEDOUT:
                return io_error::timed_out;

            case EADDRINUSE:
                return io_error::address_in_use;

            case EADDRNOTAVAIL:
            case EAFNOSUPPORT:
                return io_error::address_invalid;

            case EHOSTUNREACH:
                return io_error::host_unreachable;

            case ENETUNREACH:
            case ENETDOWN:
                return io_error::network_down;

            case EACCES:
            case EPERM:
                return io_error::access_denied;

            case EINVAL:
            case EFAULT:
                return io_error::invalid_argument;

            case EMFILE:
            case ENFILE:
                return io_error::too_many_open_files;

            case ENOMEM:
            case ENOBUFS:
                return io_error::out_of_memory;

            default:
                return io_error::unknown;
        }
    }

    // from_gai
    //   function: maps a getaddrinfo EAI_* result onto io_error.
    D_NODISCARD inline io_error
    from_gai(
        int _gai
    )
    {
    #ifdef EAI_SYSTEM
        if (_gai == EAI_SYSTEM)
        {
            return from_errno(errno);
        }
    #endif

        switch (_gai)
        {
        #ifdef EAI_AGAIN
            case EAI_AGAIN:
                return io_error::timed_out;
        #endif
        #ifdef EAI_MEMORY
            case EAI_MEMORY:
                return io_error::out_of_memory;
        #endif
        #ifdef EAI_NONAME
            case EAI_NONAME:
                return io_error::host_unreachable;
        #endif
        #ifdef EAI_FAIL
            case EAI_FAIL:
                return io_error::host_unreachable;
        #endif
            default:
                return io_error::address_invalid;
        }
    }

    // send_flags
    //   function: the flags for send() -- suppressing SIGPIPE where the platform
    // supports doing so at the call site (Linux MSG_NOSIGNAL).
    D_NODISCARD inline int
    send_flags()
    {
    #ifdef MSG_NOSIGNAL
        return MSG_NOSIGNAL;
    #else
        return 0;
    #endif
    }

    // set_cloexec
    //   function: marks a descriptor close-on-exec so it does not leak across an
    // exec in a forking program.
    inline void
    set_cloexec(
        int _fd
    )
    {
    #ifdef FD_CLOEXEC
        const int flags = ::fcntl(_fd, F_GETFD, 0);

        if (flags >= 0)
        {
            (void)::fcntl(_fd, F_SETFD, flags | FD_CLOEXEC);
        }
    #else
        (void)_fd;
    #endif

        return;
    }

    // set_nodelay
    //   function: enables/disables Nagle's algorithm via TCP_NODELAY.
    inline void
    set_nodelay(
        int  _fd,
        bool _on
    )
    {
    #ifdef TCP_NODELAY
        const int value = _on ? 1 : 0;
        (void)::setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
    #else
        (void)_fd;
        (void)_on;
    #endif

        return;
    }

    // suppress_sigpipe
    //   function: suppresses SIGPIPE at the socket level where the platform
    // offers it (Apple/BSD SO_NOSIGPIPE); a no-op elsewhere (Linux relies on
    // MSG_NOSIGNAL per send instead).
    inline void
    suppress_sigpipe(
        int _fd
    )
    {
    #ifdef SO_NOSIGPIPE
        const int value = 1;
        (void)::setsockopt(_fd, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
    #else
        (void)_fd;
    #endif

        return;
    }

    // endpoint_from_sockaddr
    //   function: converts a socket address to a net::endpoint (numeric host via
    // inet_ntop; port via ntohs; the path for unix addresses).
    D_NODISCARD inline endpoint
    endpoint_from_sockaddr(
        const sockaddr* _addr,
        socklen_t       _len
    )
    {
        endpoint result;

        if (_addr == nullptr)
        {
            return result;
        }

        (void)_len;

        if (_addr->sa_family == AF_INET)
        {
            const sockaddr_in* in4 =
                reinterpret_cast<const sockaddr_in*>(_addr);

            char text[INET_ADDRSTRLEN];
            std::memset(text, 0, sizeof(text));
            ::inet_ntop(AF_INET, &in4->sin_addr, text, sizeof(text));

            result.host  = text;
            result.port  = static_cast<port_type>(ntohs(in4->sin_port));
            result.proto = protocol::tcp;
        }
        else if (_addr->sa_family == AF_INET6)
        {
            const sockaddr_in6* in6 =
                reinterpret_cast<const sockaddr_in6*>(_addr);

            char text[INET6_ADDRSTRLEN];
            std::memset(text, 0, sizeof(text));
            ::inet_ntop(AF_INET6, &in6->sin6_addr, text, sizeof(text));

            result.host  = text;
            result.port  = static_cast<port_type>(ntohs(in6->sin6_port));
            result.proto = protocol::tcp;
        }
        else if (_addr->sa_family == AF_UNIX)
        {
            const sockaddr_un* un =
                reinterpret_cast<const sockaddr_un*>(_addr);

            result = endpoint::local(un->sun_path);
        }

        return result;
    }

    // connect_fd
    //   function: connects an already-created socket to `_addr`. With
    // `_timeout_ms` <= 0 it performs a plain blocking connect (retrying EINTR);
    // otherwise it connects non-blocking and waits up to the timeout via poll(),
    // then restores blocking mode. Returns none on success.
    D_NODISCARD inline io_error
    connect_fd(
        int             _fd,
        const sockaddr* _addr,
        socklen_t       _len,
        long            _timeout_ms
    )
    {
        // plain blocking connect
        if (_timeout_ms <= 0)
        {
            for (;;)
            {
                if (::connect(_fd, _addr, _len) == 0)
                {
                    return io_error::none;
                }

                if (errno == EINTR)
                {
                    continue;
                }

                return from_errno(errno);
            }
        }

        // non-blocking connect with a bounded wait
        const int original = ::fcntl(_fd, F_GETFL, 0);

        if (original < 0)
        {
            return from_errno(errno);
        }

        if (::fcntl(_fd, F_SETFL, original | O_NONBLOCK) < 0)
        {
            return from_errno(errno);
        }

        io_error result = io_error::none;
        const int rc     = ::connect(_fd, _addr, _len);

        if (rc == 0)
        {
            result = io_error::none;
        }
        else if (errno == EINPROGRESS)
        {
            // wait for the socket to become writable (or the timeout)
            struct pollfd pfd;
            pfd.fd      = _fd;
            pfd.events  = POLLOUT;
            pfd.revents = 0;

            int pr = 0;

            for (;;)
            {
                pr = ::poll(&pfd, 1, static_cast<int>(_timeout_ms));

                if ( (pr < 0) &&
                     (errno == EINTR) )
                {
                    continue;
                }

                break;
            }

            if (pr == 0)
            {
                result = io_error::timed_out;
            }
            else if (pr < 0)
            {
                result = from_errno(errno);
            }
            else
            {
                // retrieve the connect outcome
                int       so_error = 0;
                socklen_t so_len   = sizeof(so_error);

                if (::getsockopt(_fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len) < 0)
                {
                    result = from_errno(errno);
                }
                else
                {
                    result = from_errno(so_error);
                }
            }
        }
        else
        {
            result = from_errno(errno);
        }

        // restore the original (blocking) mode regardless of outcome
        (void)::fcntl(_fd, F_SETFL, original);

        return result;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                          SOCKET OPTIONS                                ///
///////////////////////////////////////////////////////////////////////////////

// socket_options
//   struct: tunables shared by the connector and acceptor. Defaults are safe
// and conservative: Nagle left on (no_delay false), SO_REUSEADDR on for the
// listener, SO_REUSEPORT off, a blocking connect (no timeout), and a listen
// backlog of 128.
struct socket_options
{
    bool no_delay;
    bool reuse_address;
    bool reuse_port;
    long connect_timeout_ms;
    int  listen_backlog;

    socket_options()
        : no_delay(false),
          reuse_address(true),
          reuse_port(false),
          connect_timeout_ms(0),
          listen_backlog(128)
    {
    }
};


///////////////////////////////////////////////////////////////////////////////
///                        SOCKET CONNECTION                               ///
///////////////////////////////////////////////////////////////////////////////

// socket_connection
//   class: a net::connection over a connected socket descriptor. Owns the
// descriptor (closes it on destruction); movable, non-copyable. read()/write()
// retry EINTR and map errno; write() suppresses SIGPIPE. The remote/local
// endpoints are captured at construction.
class socket_connection : public connection
{
public:
    explicit socket_connection(
        int      _fd,
        protocol _proto = protocol::tcp
    )
        : m_fd(_fd),
          m_proto(_proto),
          m_remote(),
          m_local()
    {
        query_endpoints();
    }

    ~socket_connection() override
    {
        reset();
    }

    socket_connection(
        socket_connection&& _other
    ) D_NOEXCEPT
        : connection(),
          m_fd(_other.m_fd),
          m_proto(_other.m_proto),
          m_remote(std::move(_other.m_remote)),
          m_local(std::move(_other.m_local))
    {
        _other.m_fd = -1;
    }

    socket_connection&
    operator=(
        socket_connection&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            reset();

            m_fd     = _other.m_fd;
            m_proto  = _other.m_proto;
            m_remote = std::move(_other.m_remote);
            m_local  = std::move(_other.m_local);

            _other.m_fd = -1;
        }

        return *this;
    }

    // read
    //   function: recv up to `_size` bytes; 0 bytes signals EOF.
    D_NODISCARD io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) override
    {
        if (m_fd < 0)
        {
            return io_result(0, io_error::closed);
        }

        if (_size == 0)
        {
            return io_result(0, io_error::none);
        }

        for (;;)
        {
            const ssize_t r = ::recv(m_fd, _buffer, _size, 0);

            if (r > 0)
            {
                return io_result(static_cast<std::size_t>(r), io_error::none);
            }

            // orderly shutdown by the peer
            if (r == 0)
            {
                return io_result(0, io_error::none);
            }

            if (errno == EINTR)
            {
                continue;
            }

            return io_result(0, internal::from_errno(errno));
        }
    }

    // write
    //   function: send up to `_size` bytes; SIGPIPE suppressed.
    D_NODISCARD io_result
    write(
        const void* _data,
        std::size_t _size
    ) override
    {
        if (m_fd < 0)
        {
            return io_result(0, io_error::closed);
        }

        if (_size == 0)
        {
            return io_result(0, io_error::none);
        }

        for (;;)
        {
            const ssize_t w = ::send(m_fd, _data, _size, internal::send_flags());

            if (w >= 0)
            {
                return io_result(static_cast<std::size_t>(w), io_error::none);
            }

            if (errno == EINTR)
            {
                continue;
            }

            return io_result(0, internal::from_errno(errno));
        }
    }

    // is_open
    //   function: whether the descriptor is still held.
    D_NODISCARD bool
    is_open() const override
    {
        return (m_fd >= 0);
    }

    // close
    //   function: closes the descriptor (idempotent).
    void
    close() override
    {
        reset();

        return;
    }

    // shutdown
    //   function: half-closes the given direction(s).
    D_NODISCARD io_error
    shutdown(
        shutdown_mode _mode
    ) override
    {
        if (m_fd < 0)
        {
            return io_error::closed;
        }

        int how = SHUT_RDWR;

        if (_mode == shutdown_mode::read)
        {
            how = SHUT_RD;
        }
        else if (_mode == shutdown_mode::write)
        {
            how = SHUT_WR;
        }

        if (::shutdown(m_fd, how) < 0)
        {
            return internal::from_errno(errno);
        }

        return io_error::none;
    }

    // remote_endpoint / local_endpoint
    //   function: the addresses captured at construction.
    D_NODISCARD endpoint
    remote_endpoint() const override
    {
        return m_remote;
    }

    D_NODISCARD endpoint
    local_endpoint() const override
    {
        return m_local;
    }

    // native_handle
    //   function: the underlying descriptor (for advanced/backend use).
    D_NODISCARD int
    native_handle() const
    {
        return m_fd;
    }

private:
    // query_endpoints
    //   function: fills the remote/local endpoints from the live socket.
    void
    query_endpoints()
    {
        if (m_fd < 0)
        {
            return;
        }

        sockaddr_storage storage;
        socklen_t        length = sizeof(storage);

        if (::getpeername(m_fd,
                          reinterpret_cast<sockaddr*>(&storage),
                          &length) == 0)
        {
            m_remote = internal::endpoint_from_sockaddr(
                           reinterpret_cast<sockaddr*>(&storage),
                           length);
        }

        length = sizeof(storage);

        if (::getsockname(m_fd,
                          reinterpret_cast<sockaddr*>(&storage),
                          &length) == 0)
        {
            m_local = internal::endpoint_from_sockaddr(
                          reinterpret_cast<sockaddr*>(&storage),
                          length);
        }

        return;
    }

    // reset
    //   function: closes and clears the descriptor.
    void
    reset()
    {
        if (m_fd >= 0)
        {
            (void)::close(m_fd);
            m_fd = -1;
        }

        return;
    }

    int      m_fd;
    protocol m_proto;
    endpoint m_remote;
    endpoint m_local;
};


///////////////////////////////////////////////////////////////////////////////
///                          TCP CONNECTOR                                 ///
///////////////////////////////////////////////////////////////////////////////

// tcp_connector
//   class: a net::connector producing socket_connections. For TCP endpoints it
// resolves the host with getaddrinfo (IPv4 or IPv6) and tries each result until
// one connects; for unix endpoints it connects to the path. Honors the
// connect-timeout and no-delay options.
class tcp_connector
{
public:
    tcp_connector()
        : m_options()
    {
    }

    explicit tcp_connector(
        const socket_options& _options
    )
        : m_options(_options)
    {
    }

    // options
    //   function: mutable access to the socket options.
    D_NODISCARD socket_options&
    options()
    {
        return m_options;
    }

    // connect
    //   function: connects to `_endpoint`, dispatching on protocol.
    D_NODISCARD open_result
    connect(
        const endpoint& _endpoint
    )
    {
        if (_endpoint.is_unix())
        {
            return connect_unix(_endpoint);
        }

        return connect_tcp(_endpoint);
    }

private:
    // connect_tcp
    //   function: resolves and connects a TCP endpoint.
    D_NODISCARD open_result
    connect_tcp(
        const endpoint& _endpoint
    )
    {
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        const std::string port_text =
            std::to_string(static_cast<unsigned>(_endpoint.port));

        struct addrinfo* results = nullptr;

        const int gai = ::getaddrinfo(_endpoint.host.c_str(),
                                      port_text.c_str(),
                                      &hints,
                                      &results);

        if (gai != 0)
        {
            return open_result(internal::from_gai(gai));
        }

        io_error last = io_error::unknown;

        // try each resolved address until one connects
        for (struct addrinfo* ai = results; ai != nullptr; ai = ai->ai_next)
        {
            const int fd = ::socket(ai->ai_family,
                                    ai->ai_socktype,
                                    ai->ai_protocol);

            if (fd < 0)
            {
                last = internal::from_errno(errno);
                continue;
            }

            internal::set_cloexec(fd);
            internal::suppress_sigpipe(fd);

            if (m_options.no_delay)
            {
                internal::set_nodelay(fd, true);
            }

            const io_error e = internal::connect_fd(fd,
                                                    ai->ai_addr,
                                                    static_cast<socklen_t>(ai->ai_addrlen),
                                                    m_options.connect_timeout_ms);

            if (e == io_error::none)
            {
                ::freeaddrinfo(results);

                return open_result(
                    std::unique_ptr<connection>(
                        new socket_connection(fd, protocol::tcp)));
            }

            (void)::close(fd);
            last = e;
        }

        ::freeaddrinfo(results);

        return open_result(last);
    }

    // connect_unix
    //   function: connects a unix-domain endpoint by path.
    D_NODISCARD open_result
    connect_unix(
        const endpoint& _endpoint
    )
    {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);

        if (fd < 0)
        {
            return open_result(internal::from_errno(errno));
        }

        internal::set_cloexec(fd);
        internal::suppress_sigpipe(fd);

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;

        if (_endpoint.host.size() >= sizeof(addr.sun_path))
        {
            (void)::close(fd);
            return open_result(io_error::address_invalid);
        }

        std::strncpy(addr.sun_path,
                     _endpoint.host.c_str(),
                     sizeof(addr.sun_path) - 1);

        const io_error e = internal::connect_fd(
                               fd,
                               reinterpret_cast<sockaddr*>(&addr),
                               static_cast<socklen_t>(sizeof(addr)),
                               m_options.connect_timeout_ms);

        if (e != io_error::none)
        {
            (void)::close(fd);
            return open_result(e);
        }

        return open_result(
            std::unique_ptr<connection>(
                new socket_connection(fd, protocol::unix_socket)));
    }

    socket_options m_options;
};


///////////////////////////////////////////////////////////////////////////////
///                          TCP ACCEPTOR                                  ///
///////////////////////////////////////////////////////////////////////////////

// tcp_acceptor
//   class: a net::acceptor. bind() resolves + binds + listens (TCP or unix);
// accept() blocks for the next connection; close() shuts the listener down and
// -- via an internal self-pipe polled alongside the listener -- unblocks any
// accept() in progress so a server can stop promptly. Movable, non-copyable;
// closes and (for unix) unlinks on destruction.
class tcp_acceptor
{
public:
    tcp_acceptor()
        : m_fd(-1),
          m_wake_read(-1),
          m_wake_write(-1),
          m_options(),
          m_proto(protocol::tcp),
          m_unix_path()
    {
    }

    explicit tcp_acceptor(
        const socket_options& _options
    )
        : m_fd(-1),
          m_wake_read(-1),
          m_wake_write(-1),
          m_options(_options),
          m_proto(protocol::tcp),
          m_unix_path()
    {
    }

    ~tcp_acceptor()
    {
        close();
    }

    tcp_acceptor(
        tcp_acceptor&& _other
    ) D_NOEXCEPT
        : m_fd(_other.m_fd),
          m_wake_read(_other.m_wake_read),
          m_wake_write(_other.m_wake_write),
          m_options(_other.m_options),
          m_proto(_other.m_proto),
          m_unix_path(std::move(_other.m_unix_path))
    {
        _other.m_fd         = -1;
        _other.m_wake_read  = -1;
        _other.m_wake_write = -1;
    }

    tcp_acceptor&
    operator=(
        tcp_acceptor&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            close();

            m_fd         = _other.m_fd;
            m_wake_read  = _other.m_wake_read;
            m_wake_write = _other.m_wake_write;
            m_options    = _other.m_options;
            m_proto      = _other.m_proto;
            m_unix_path  = std::move(_other.m_unix_path);

            _other.m_fd         = -1;
            _other.m_wake_read  = -1;
            _other.m_wake_write = -1;
        }

        return *this;
    }

    // bind
    //   function: binds and listens on `_endpoint`, dispatching on protocol.
    D_NODISCARD io_error
    bind(
        const endpoint& _endpoint
    )
    {
        if (_endpoint.is_unix())
        {
            return bind_unix(_endpoint);
        }

        return bind_tcp(_endpoint);
    }

    // accept
    //   function: waits for and returns the next incoming connection. Returns
    // an error open_result of io_error::closed when the acceptor is closed
    // (including a close() from another thread that unblocks this call).
    D_NODISCARD open_result
    accept()
    {
        if (m_fd < 0)
        {
            return open_result(io_error::closed);
        }

        for (;;)
        {
            struct pollfd fds[2];

            fds[0].fd      = m_fd;
            fds[0].events  = POLLIN;
            fds[0].revents = 0;
            fds[1].fd      = m_wake_read;
            fds[1].events  = POLLIN;
            fds[1].revents = 0;

            const int pr = ::poll(fds, 2, -1);

            if (pr < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                return open_result(internal::from_errno(errno));
            }

            // a wake signals shutdown
            if (fds[1].revents & POLLIN)
            {
                return open_result(io_error::closed);
            }

            // listener error / hangup
            if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                return open_result(io_error::closed);
            }

            if (fds[0].revents & POLLIN)
            {
                const int cfd = ::accept(m_fd, nullptr, nullptr);

                if (cfd < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    if ( (errno == EAGAIN) ||
                         (errno == EWOULDBLOCK) )
                    {
                        continue;
                    }

                    // a transient per-connection failure should not kill the
                    // listener; report it and let the serve loop decide
                    if (errno == ECONNABORTED)
                    {
                        continue;
                    }

                    return open_result(internal::from_errno(errno));
                }

                internal::set_cloexec(cfd);
                internal::suppress_sigpipe(cfd);

                if ( m_options.no_delay &&
                     (m_proto == protocol::tcp) )
                {
                    internal::set_nodelay(cfd, true);
                }

                return open_result(
                    std::unique_ptr<connection>(
                        new socket_connection(cfd, m_proto)));
            }
        }
    }

    // is_open
    //   function: whether the listener is bound and open.
    D_NODISCARD bool
    is_open() const
    {
        return (m_fd >= 0);
    }

    // close
    //   function: unblocks a pending accept() (self-pipe), closes the listener,
    // and unlinks a unix path. Idempotent; safe to call from another thread to
    // stop a running accept loop.
    void
    close()
    {
        // wake any accept() blocked in poll()
        if (m_wake_write >= 0)
        {
            const char byte = 1;
            const ssize_t n = ::write(m_wake_write, &byte, 1);
            (void)n;
        }

        if (m_fd >= 0)
        {
            (void)::shutdown(m_fd, SHUT_RDWR);
            (void)::close(m_fd);
            m_fd = -1;
        }

        if (m_wake_write >= 0)
        {
            (void)::close(m_wake_write);
            m_wake_write = -1;
        }

        if (m_wake_read >= 0)
        {
            (void)::close(m_wake_read);
            m_wake_read = -1;
        }

        if (!m_unix_path.empty())
        {
            (void)::unlink(m_unix_path.c_str());
            m_unix_path.clear();
        }

        return;
    }

    // local_endpoint
    //   function: the actual bound address (resolves an ephemeral port 0 to the
    // port the OS assigned).
    D_NODISCARD endpoint
    local_endpoint() const
    {
        if (m_fd < 0)
        {
            return endpoint();
        }

        sockaddr_storage storage;
        socklen_t        length = sizeof(storage);

        if (::getsockname(m_fd,
                          reinterpret_cast<sockaddr*>(&storage),
                          &length) == 0)
        {
            return internal::endpoint_from_sockaddr(
                       reinterpret_cast<sockaddr*>(&storage),
                       length);
        }

        return endpoint();
    }

    // native_handle
    //   function: the underlying listening descriptor (for advanced/backend use,
    // e.g. registering the listener with an event loop). -1 when unbound.
    D_NODISCARD int
    native_handle() const
    {
        return m_fd;
    }

private:
    // setup_wake
    //   function: creates the self-pipe used to interrupt accept(). Returns
    // false on failure.
    D_NODISCARD bool
    setup_wake()
    {
        int fds[2];

    #if defined(__linux__)
        if (::pipe2(fds, O_CLOEXEC) != 0)
        {
            return false;
        }
    #else
        if (::pipe(fds) != 0)
        {
            return false;
        }

        internal::set_cloexec(fds[0]);
        internal::set_cloexec(fds[1]);
    #endif

        m_wake_read  = fds[0];
        m_wake_write = fds[1];

        return true;
    }

    // bind_tcp
    //   function: resolves (AI_PASSIVE), creates, options, binds, and listens a
    // TCP socket.
    D_NODISCARD io_error
    bind_tcp(
        const endpoint& _endpoint
    )
    {
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags    = AI_PASSIVE;

        const std::string port_text =
            std::to_string(static_cast<unsigned>(_endpoint.port));

        const char* node =
            _endpoint.host.empty() ? nullptr : _endpoint.host.c_str();

        struct addrinfo* results = nullptr;

        const int gai = ::getaddrinfo(node, port_text.c_str(), &hints, &results);

        if (gai != 0)
        {
            return internal::from_gai(gai);
        }

        io_error last = io_error::unknown;

        for (struct addrinfo* ai = results; ai != nullptr; ai = ai->ai_next)
        {
            const int fd = ::socket(ai->ai_family,
                                    ai->ai_socktype,
                                    ai->ai_protocol);

            if (fd < 0)
            {
                last = internal::from_errno(errno);
                continue;
            }

            internal::set_cloexec(fd);

            if (m_options.reuse_address)
            {
                const int value = 1;
                (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                                   &value, sizeof(value));
            }

        #ifdef SO_REUSEPORT
            if (m_options.reuse_port)
            {
                const int value = 1;
                (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                                   &value, sizeof(value));
            }
        #endif

            if (::bind(fd, ai->ai_addr,
                       static_cast<socklen_t>(ai->ai_addrlen)) < 0)
            {
                last = internal::from_errno(errno);
                (void)::close(fd);
                continue;
            }

            if (::listen(fd, m_options.listen_backlog) < 0)
            {
                last = internal::from_errno(errno);
                (void)::close(fd);
                continue;
            }

            ::freeaddrinfo(results);

            m_fd    = fd;
            m_proto = protocol::tcp;

            if (!setup_wake())
            {
                (void)::close(m_fd);
                m_fd = -1;
                return io_error::out_of_memory;
            }

            return io_error::none;
        }

        ::freeaddrinfo(results);

        return last;
    }

    // bind_unix
    //   function: creates, binds, and listens a unix-domain socket at the path.
    D_NODISCARD io_error
    bind_unix(
        const endpoint& _endpoint
    )
    {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);

        if (fd < 0)
        {
            return internal::from_errno(errno);
        }

        internal::set_cloexec(fd);

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;

        if (_endpoint.host.size() >= sizeof(addr.sun_path))
        {
            (void)::close(fd);
            return io_error::address_invalid;
        }

        std::strncpy(addr.sun_path,
                     _endpoint.host.c_str(),
                     sizeof(addr.sun_path) - 1);

        // remove any stale socket file at the path
        (void)::unlink(_endpoint.host.c_str());

        if (::bind(fd, reinterpret_cast<sockaddr*>(&addr),
                   static_cast<socklen_t>(sizeof(addr))) < 0)
        {
            const io_error e = internal::from_errno(errno);
            (void)::close(fd);
            return e;
        }

        if (::listen(fd, m_options.listen_backlog) < 0)
        {
            const io_error e = internal::from_errno(errno);
            (void)::close(fd);
            return e;
        }

        m_fd        = fd;
        m_proto     = protocol::unix_socket;
        m_unix_path = _endpoint.host;

        if (!setup_wake())
        {
            (void)::close(m_fd);
            m_fd = -1;
            m_unix_path.clear();
            return io_error::out_of_memory;
        }

        return io_error::none;
    }

    int            m_fd;
    int            m_wake_read;
    int            m_wake_write;
    socket_options m_options;
    protocol       m_proto;
    std::string    m_unix_path;
};


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_TCP_
