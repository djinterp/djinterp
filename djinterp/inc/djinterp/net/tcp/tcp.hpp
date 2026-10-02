/*******************************************************************************
* djinterp [net]                                                         tcp.hpp
*
* The TCP transport for net.hpp: tcp.h's sockets as C++ connections.
*   socket_connection is a net::connection over a d_tcp_connection it holds
* by value; tcp_connector opens one, and tcp_acceptor binds, listens, and
* accepts them, and can be closed from any thread to wake a blocked accept.
* The names and signatures are the replaced C++ tcp.hpp's, so client.hpp,
* server.hpp, reactor.hpp, tls.hpp, and http.hpp compile unchanged, while the
* socket code exists once, in tcp.c.
*   Each member forwards to one tcp.h function, called directly rather than
* through net.h's table. Nothing throws but operator new, which connect and
* accept use because open_result owns its connection.
*   C++11 or later. Link tcp.c and net.c.
*
*
* path:      /inc/djinterp/net/tcp/tcp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPTIONS
    -------
    1.  Tunables
         1.  socket_options
2.  CONNECTIONS
    -----------
    1.  Handles
         1.  socket_native_handle
    2.  Socket connections
         1.  socket_connection
3.  OPENING
    -------
    1.  Connecting
         1.  tcp_connector
    2.  Accepting
         1.  tcp_acceptor
4.  NATIVE HELPERS
    --------------
    1.  Kept for consumers
         1.  from_errno
         2.  set_cloexec
         3.  suppress_sigpipe
*/

#ifndef DJINTERP_NET_TCP_TCP_HPP
#define DJINTERP_NET_TCP_TCP_HPP 1

// std
#include <cstddef>  // std::size_t
#include <memory>   // std::unique_ptr
#include <utility>  // std::move
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../net.hpp"          // connection, endpoint, open_result, to_c
#include "./tcp.h"             // the C transport


NS_DJINTERP
NS_NET


//==============================================================================
// 1.  OPTIONS
//==============================================================================


// 1.1    Tunables
//------------------------------------------------------------------------------
// 1.1.1
// socket_options
//   struct: how connections are opened and listeners bound, with the
// replaced tcp.hpp's defaults; the backlog comes from cfg_tcp.h.
struct socket_options
{
    bool no_delay;            // set TCP_NODELAY, disabling Nagle
    bool reuse_address;       // set SO_REUSEADDR on listeners
    bool reuse_port;          // set SO_REUSEPORT on listeners, where it exists
    long connect_timeout_ms;  // per address tried; 0 waits as the system does
    int  listen_backlog;      // listen()'s backlog

    socket_options()
        : no_delay(false),
          reuse_address(true),
          reuse_port(false),
          connect_timeout_ms(0),
          listen_backlog(D_INTERNAL_TCP_LISTEN_BACKLOG)
    {}
};

NS_INTERNAL

    // to_c_options
    //   function: the d_tcp_options equal to a socket_options.
    D_NODISCARD inline ::d_tcp_options
    to_c_options(
        const socket_options& _options
    ) noexcept
    {
        ::d_tcp_options converted = ::d_tcp_options();

        ::d_tcp_options_init(&converted);
        converted.connect_timeout_ms = _options.connect_timeout_ms;
        converted.listen_backlog     = _options.listen_backlog;
        converted.no_delay           = _options.no_delay;
        converted.reuse_address      = _options.reuse_address;
        converted.reuse_port         = _options.reuse_port;

        return converted;
    }

NS_END  // internal


//==============================================================================
// 2.  CONNECTIONS
//==============================================================================


// 2.1    Handles
//------------------------------------------------------------------------------
// 2.1.1
// socket_native_handle
//   type: a socket's native handle: an int descriptor under BSD sockets, as
// the replaced tcp.hpp returned, and tcp.h's d_tcp_socket under Winsock.
#if (D_INTERNAL_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK)
    using socket_native_handle = ::d_tcp_socket;
#else
    using socket_native_handle = int;
#endif  // D_INTERNAL_TCP_BACKEND

// 2.2    Socket connections
//------------------------------------------------------------------------------
// 2.2.1
// socket_connection
//   class: a connection over a connected socket, which it owns and closes
// on destruction. Both endpoints are recorded when it opens and outlive the
// socket. Move-only.
class socket_connection : public connection
{
public:
    using native_handle_type = socket_native_handle;

    // a closed connection, for connect and accept to open in place
    socket_connection() noexcept
        : connection(),
          m_connection()
    {
        ::d_tcp_connection_init(&m_connection);
    }

    // adopts a connected socket; ownership passes even when it is refused,
    // as an invalid handle is, leaving the connection closed
    explicit socket_connection(
        native_handle_type _fd,
        protocol           _proto = protocol::tcp
    ) noexcept
        : connection(),
          m_connection()
    {
        (void)::d_tcp_connection_adopt(&m_connection,
                                       static_cast<d_tcp_socket>(_fd),
                                       to_c(_proto));
    }

    ~socket_connection() override
    {
        ::d_tcp_connection_close(&m_connection);
    }

    socket_connection(
        socket_connection&& _other
    ) noexcept
        : connection(),
          m_connection(_other.m_connection)
    {
        (void)::d_tcp_connection_release(&_other.m_connection);
    }

    socket_connection&
    operator=(
        socket_connection&& _other
    ) noexcept
    {
        // assigning a connection to itself keeps its socket
        if (this != &_other)
        {
            ::d_tcp_connection_close(&m_connection);
            m_connection = _other.m_connection;
            (void)::d_tcp_connection_release(&_other.m_connection);
        }

        return *this;
    }

    // the connection's operations, each forwarded to tcp.h
    D_NODISCARD io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) override
    {
        return from_c(::d_tcp_connection_read(&m_connection,
                                              _buffer,
                                              _size));
    }

    D_NODISCARD io_result
    write(
        const void* _data,
        std::size_t _size
    ) override
    {
        return from_c(::d_tcp_connection_write(&m_connection,
                                               _data,
                                               _size));
    }

    D_NODISCARD bool
    is_open() const override
    {
        return ::d_tcp_connection_is_open(&m_connection);
    }

    void
    close() override
    {
        ::d_tcp_connection_close(&m_connection);

        return;
    }

    D_NODISCARD io_error
    shutdown(
        shutdown_mode _mode
    ) override
    {
        return from_c(::d_tcp_connection_shutdown(&m_connection,
                                                  to_c(_mode)));
    }

    // the endpoints recorded at opening; an unknown one is a default
    // endpoint, as it was
    D_NODISCARD endpoint
    remote_endpoint() const override
    {
        ::d_net_endpoint recorded = ::d_net_endpoint();

        return (::d_tcp_connection_remote_endpoint(&m_connection,
                                                   &recorded))
                   ? from_c(recorded)
                   : endpoint();
    }

    D_NODISCARD endpoint
    local_endpoint() const override
    {
        ::d_net_endpoint recorded = ::d_net_endpoint();

        return (::d_tcp_connection_local_endpoint(&m_connection,
                                                  &recorded))
                   ? from_c(recorded)
                   : endpoint();
    }

    // native_handle -- the socket, or -1 (INVALID_SOCKET) once closed
    D_NODISCARD native_handle_type
    native_handle() const noexcept
    {
        return static_cast<native_handle_type>(
                   ::d_tcp_connection_native(&m_connection));
    }

    // c_view -- the C connection beneath, still owned here, for tcp.h and
    // net.h's C algorithms
    D_NODISCARD ::d_tcp_connection&
    c_view() noexcept
    {
        return m_connection;
    }

    D_NODISCARD const ::d_tcp_connection&
    c_view() const noexcept
    {
        return m_connection;
    }

private:
    ::d_tcp_connection m_connection;
};


//==============================================================================
// 3.  OPENING
//==============================================================================
// A connector and an acceptor in the shapes client.hpp's connector concept,
// server.hpp, and tls.hpp take.


// 3.1    Connecting
//------------------------------------------------------------------------------
// 3.1.1
// tcp_connector
//   class: opens socket_connections, over TCP or to a unix-domain path by
// the endpoint's protocol, with its socket_options.
class tcp_connector
{
public:
    tcp_connector()
        : m_options()
    {}

    explicit tcp_connector(
        const socket_options& _options
    )
        : m_options(_options)
    {}

    // options -- the tunables the next connect uses
    D_NODISCARD socket_options&
    options()
    {
        return m_options;
    }

    // connect -- resolves and connects, trying each address; the result
    // owns the connection, or holds the error
    D_NODISCARD open_result
    connect(
        const endpoint& _endpoint
    )
    {
        ::d_net_endpoint target = ::d_net_endpoint();
        const io_error   stored = to_c(_endpoint,
                                       target);

        // an endpoint C cannot hold is refused before a socket exists
        if (stored != io_error::none)
        {
            return open_result(stored);
        }

        const ::d_tcp_options tuned = internal::to_c_options(m_options);
        std::unique_ptr<socket_connection> opened(new socket_connection());
        const io_error result = from_c(::d_tcp_connect(&opened->c_view(),
                                                       &target,
                                                       &tuned));

        // a failed connect hands back only its error
        if (result != io_error::none)
        {
            return open_result(result);
        }

        return open_result(std::move(opened));
    }

private:
    socket_options m_options;
};

// 3.2    Accepting
//------------------------------------------------------------------------------
// 3.2.1
// tcp_acceptor
//   class: binds and listens on an endpoint, TCP or a unix-domain path, and
// accepts socket_connections. close may be called from another thread to
// wake an accept blocked in this one. Move-only.
class tcp_acceptor
{
public:
    using native_handle_type = socket_native_handle;

    tcp_acceptor()
        : m_listener(),
          m_options()
    {
        ::d_tcp_listener_init(&m_listener);
    }

    explicit tcp_acceptor(
        const socket_options& _options
    )
        : m_listener(),
          m_options(_options)
    {
        ::d_tcp_listener_init(&m_listener);
    }

    ~tcp_acceptor()
    {
        ::d_tcp_listener_close(&m_listener);
    }

    tcp_acceptor(
        tcp_acceptor&& _other
    ) noexcept
        : m_listener(_other.m_listener),
          m_options(_other.m_options)
    {
        ::d_tcp_listener_init(&_other.m_listener);
    }

    tcp_acceptor&
    operator=(
        tcp_acceptor&& _other
    ) noexcept
    {
        // assigning an acceptor to itself keeps its listener
        if (this != &_other)
        {
            ::d_tcp_listener_close(&m_listener);
            m_listener = _other.m_listener;
            m_options  = _other.m_options;
            ::d_tcp_listener_init(&_other.m_listener);
        }

        return *this;
    }

    // bind -- binds and listens, closing any earlier binding first; port 0
    // takes an ephemeral port, which local_endpoint then reports
    D_NODISCARD io_error
    bind(
        const endpoint& _endpoint
    )
    {
        ::d_net_endpoint target = ::d_net_endpoint();
        const io_error   stored = to_c(_endpoint,
                                       target);

        // an endpoint C cannot hold is refused before a socket exists
        if (stored != io_error::none)
        {
            return stored;
        }

        const ::d_tcp_options tuned = internal::to_c_options(m_options);

        return from_c(::d_tcp_listen(&m_listener,
                                     &target,
                                     &tuned));
    }

    // accept -- waits for a connection; io_error::closed once closed,
    // including by close from another thread during the wait
    D_NODISCARD open_result
    accept()
    {
        std::unique_ptr<socket_connection> accepted(new socket_connection());
        const io_error result = from_c(::d_tcp_accept(&m_listener,
                                                      &accepted->c_view()));

        // a failed accept hands back only its error
        if (result != io_error::none)
        {
            return open_result(result);
        }

        return open_result(std::move(accepted));
    }

    D_NODISCARD bool
    is_open() const
    {
        return ::d_tcp_listener_is_open(&m_listener);
    }

    // close -- stops listening, waking a blocked accept; a unix-domain path
    // the acceptor created is removed
    void
    close()
    {
        ::d_tcp_listener_close(&m_listener);

        return;
    }

    // local_endpoint -- the address bound, or a default endpoint
    D_NODISCARD endpoint
    local_endpoint() const
    {
        ::d_net_endpoint recorded = ::d_net_endpoint();

        return (::d_tcp_listener_local_endpoint(&m_listener,
                                                &recorded))
                   ? from_c(recorded)
                   : endpoint();
    }

    // native_handle -- the listening socket, or -1 (INVALID_SOCKET)
    D_NODISCARD native_handle_type
    native_handle() const noexcept
    {
        return static_cast<native_handle_type>(
                   ::d_tcp_listener_native(&m_listener));
    }

private:
    ::d_tcp_listener m_listener;
    socket_options   m_options;
};


//==============================================================================
// 4.  NATIVE HELPERS
//==============================================================================
// The replaced tcp.hpp's internal helpers that its consumers call:
// reactor.hpp maps errno and prepares the sockets it accepts itself, and
// tls.hpp maps errno. Each now forwards to tcp.h, so the mapping and the
// preparation exist once.


// 4.1    Kept for consumers
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.1.1
    // from_errno
    //   function: a native socket error as an io_error, mapped as tcp.h
    // maps its own.
    D_NODISCARD inline io_error
    from_errno(
        int _errnum
    ) noexcept
    {
        return from_c(::d_tcp_error_from_native(_errnum));
    }

    // 4.1.2
    // set_cloexec
    //   function: marks a socket close-on-exec.
    inline void
    set_cloexec(
        socket_native_handle _fd
    ) noexcept
    {
        ::d_tcp_socket_cloexec(static_cast<d_tcp_socket>(_fd));

        return;
    }

    // 4.1.3
    // suppress_sigpipe
    //   function: stops a socket raising SIGPIPE, where the socket itself
    // can say so; elsewhere tcp.h's sends pass MSG_NOSIGNAL.
    inline void
    suppress_sigpipe(
        socket_native_handle _fd
    ) noexcept
    {
        ::d_tcp_socket_no_sigpipe(static_cast<d_tcp_socket>(_fd));

        return;
    }

NS_END  // internal


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_TCP_TCP_HPP
