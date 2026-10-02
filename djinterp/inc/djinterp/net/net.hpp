/*******************************************************************************
* djinterp [net]                                                         net.hpp
*
* The C++ layer of the net foundation.
*   Built on net/net.h rather than beside it. The C header supplies the error
* taxonomy, the transport vocabulary, endpoint parsing and formatting, and the
* framing codec; this header gives them C++ spellings that convert to and from
* their C originals at no cost -- the enumerations share values, and every
* conversion is a cast or a field copy -- and adds what C cannot express:
*     - an owning std::string endpoint                                      [3]
*     - the runtime-polymorphic connection, with open_result, and bridges
*       between it and net.h's d_net_connection in both directions          [4]
*     - the is_byte_stream trait, and from C++20 the byte_stream concept,
*       mirroring the connection surface statically                         [5]
*     - generic stream algorithms, templates that call a concrete backend
*       directly, never through a table                                     [6]
*   DISPATCH: hold many connections polymorphically through connection&, and
* write hot generic code against is_byte_stream for static dispatch on a
* concrete backend. The abstract connection is itself a byte stream, so one
* algorithm serves both.
*   C++11 or later. Later standards only add: C++17 brings the byte_buffer
* aliases, C++20 the byte_stream concept. Errors are returned, never thrown.
* Link net.c, which defines what net.h declares.
*
*
* path:      /inc/djinterp/net/net.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMESPACE
    ---------
    1.  Keywords
         1.  D_KEYWORD_NET
         2.  NS_NET
2.  VOCABULARY
    ----------
    1.  Scalars and buffers
         1.  byte
         2.  port_type
         3.  byte_buffer
         4.  bytes
    2.  Errors
         1.  io_error
    3.  Results and modes
         1.  io_result
         2.  shutdown_mode
         3.  protocol
    4.  Conversions to and from C
3.  ENDPOINTS
    ---------
    1.  The endpoint record
         1.  endpoint
    2.  Endpoint conversions
4.  CONNECTIONS
    -----------
    1.  The connection interface
         1.  connection
         2.  open_result
    2.  Bridges to C
         1.  c_connection
         2.  c_connection_adapter
5.  BYTE STREAMS
    ------------
    1.  Stream detection
         1.  is_byte_stream
         2.  byte_stream
    2.  Buffers
6.  STREAM ALGORITHMS
    -----------------
    1.  Defaults
         1.  default_io_chunk
         2.  default_max_frame
    2.  Exact transfers
    3.  Bulk transfers
    4.  Framing
*/

#ifndef DJINTERP_NET_NET_HPP
#define DJINTERP_NET_NET_HPP 1

// std
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint32_t
#include <cstring>      // std::memcpy
#include <memory>       // std::unique_ptr
#include <string>       // std::string
#include <type_traits>  // std::integral_constant, std::is_same
#include <utility>      // std::declval, std::move
#include <vector>       // std::vector
// djinterp
#include "../djinterp.hpp"  // framework root
#include "./net.h"          // the C foundation
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include "../core/container/buffer/byte_buffer.hpp"  // byte_buffer
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


//==============================================================================
// 1.  NAMESPACE
//==============================================================================
// The version macros live in net.h, which every C++ includer reaches through
// this header.


// 1.1    Keywords
//------------------------------------------------------------------------------
// 1.1.1
// D_KEYWORD_NET
//   keyword: resolves to `net`, for identifiers, macros, and namespaces of the
// networking subframework. Guarded, so the core may adopt it.
#ifndef D_KEYWORD_NET
    #define D_KEYWORD_NET net
#endif  // D_KEYWORD_NET

// 1.1.2
// NS_NET
//   namespace: opens the `net` namespace, the root of networking. Nest it
// inside NS_DJINTERP and close it with NS_END.
#ifndef NS_NET
    #define NS_NET D_NAMESPACE(D_KEYWORD_NET)
#endif  // NS_NET


NS_DJINTERP
NS_NET


//==============================================================================
// 2.  VOCABULARY
//==============================================================================
// The C vocabulary under C++ names. Each enumeration's enumerators are
// defined as their C counterparts, so the two cannot disagree.


// 2.1    Scalars and buffers
//------------------------------------------------------------------------------
// 2.1.1
// byte
//   type: a single octet of wire data.
using byte = unsigned char;

// 2.1.2
// port_type
//   type: a TCP or UDP port number, in host byte order; net.h's d_net_port.
using port_type = ::d_net_port;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
// 2.1.3
// byte_buffer
//   type: the framework's growable byte buffer; C++17 and later, its own
// floor. Below that, the algorithms take a std::vector<byte> or a
// std::string instead.
using byte_buffer = ::djinterp::byte_buffer<>;

// 2.1.4
// bytes
//   type: shorthand for byte_buffer; C++17 and later.
using bytes = byte_buffer;
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

// 2.2    Errors
//------------------------------------------------------------------------------
// 2.2.1
// io_error
//   enum: net.h's d_net_error under C++ names: the transport failure
// taxonomy every backend maps its native codes onto.
enum class io_error : unsigned char
{
    none                = D_NET_ERROR_NONE,
    closed              = D_NET_ERROR_CLOSED,
    would_block         = D_NET_ERROR_WOULD_BLOCK,
    timed_out           = D_NET_ERROR_TIMED_OUT,
    interrupted         = D_NET_ERROR_INTERRUPTED,
    connection_reset    = D_NET_ERROR_CONNECTION_RESET,
    connection_refused  = D_NET_ERROR_CONNECTION_REFUSED,
    connection_aborted  = D_NET_ERROR_CONNECTION_ABORTED,
    address_in_use      = D_NET_ERROR_ADDRESS_IN_USE,
    address_invalid     = D_NET_ERROR_ADDRESS_INVALID,
    host_unreachable    = D_NET_ERROR_HOST_UNREACHABLE,
    network_down        = D_NET_ERROR_NETWORK_DOWN,
    access_denied       = D_NET_ERROR_ACCESS_DENIED,
    invalid_argument    = D_NET_ERROR_INVALID_ARGUMENT,
    message_too_large   = D_NET_ERROR_MESSAGE_TOO_LARGE,
    too_many_open_files = D_NET_ERROR_TOO_MANY_OPEN_FILES,
    out_of_memory       = D_NET_ERROR_OUT_OF_MEMORY,
    unknown             = D_NET_ERROR_UNKNOWN
};

/**
 * @brief Describes an error in a short, static, English phrase.
 *
 * @param[in] _error  the error.
 * @return the phrase d_net_error_string gives; never NULL.
 */
D_NODISCARD inline const char*
to_string(
    io_error _error
)
{
    return ::d_net_error_string(static_cast<d_net_error>(_error));
}

// 2.3    Results and modes
//------------------------------------------------------------------------------
// 2.3.1
// io_result
//   struct: the outcome of one read or write: how many bytes moved, and any
// error. A count of 0 with io_error::none is a clean end of stream.
struct io_result
{
    io_result()
        : count(0),
          error(io_error::none)
    {}

    io_result(
        std::size_t _count,
        io_error    _error
    )
        : count(_count),
          error(_error)
    {}

    // ok -- whether no error occurred; the end of the stream is not one
    D_NODISCARD bool
    ok() const
    {
        return (error == io_error::none);
    }

    // eof -- whether this is a clean end of stream: no bytes, no error
    D_NODISCARD bool
    eof() const
    {
        return ( (count == 0) &&
                 (error == io_error::none) );
    }

    std::size_t count;  // bytes transferred
    io_error    error;  // io_error::none on success
};

// 2.3.2
// shutdown_mode
//   enum: which direction of a duplex connection a shutdown closes.
enum class shutdown_mode : unsigned char
{
    read  = D_NET_SHUTDOWN_READ,
    write = D_NET_SHUTDOWN_WRITE,
    both  = D_NET_SHUTDOWN_BOTH
};

// 2.3.3
// protocol
//   enum: the transport an endpoint or connection uses. `unix_socket`, not
// `unix`, because `unix` is a predefined macro on some platforms.
enum class protocol : unsigned char
{
    tcp         = D_NET_PROTOCOL_TCP,
    udp         = D_NET_PROTOCOL_UDP,
    unix_socket = D_NET_PROTOCOL_UNIX
};

// 2.4    Conversions to and from C
//------------------------------------------------------------------------------
// to_c and from_c cross between each C++ vocabulary type and its net.h
// original. Each is a cast or a field copy, and inlines to nothing.
D_NODISCARD inline d_net_error
to_c(
    io_error _error
)
{
    return static_cast<d_net_error>(_error);
}

D_NODISCARD inline io_error
from_c(
    d_net_error _error
)
{
    return static_cast<io_error>(_error);
}

D_NODISCARD inline d_net_shutdown
to_c(
    shutdown_mode _mode
)
{
    return static_cast<d_net_shutdown>(_mode);
}

D_NODISCARD inline shutdown_mode
from_c(
    d_net_shutdown _mode
)
{
    return static_cast<shutdown_mode>(_mode);
}

D_NODISCARD inline d_net_protocol
to_c(
    protocol _protocol
)
{
    return static_cast<d_net_protocol>(_protocol);
}

D_NODISCARD inline protocol
from_c(
    d_net_protocol _protocol
)
{
    return static_cast<protocol>(_protocol);
}

D_NODISCARD inline d_net_io_result
to_c(
    const io_result& _result
)
{
    return ::d_net_io_result_make(_result.count,
                                  to_c(_result.error));
}

D_NODISCARD inline io_result
from_c(
    const d_net_io_result& _result
)
{
    return io_result(_result.count,
                     from_c(_result.error));
}


//==============================================================================
// 3.  ENDPOINTS
//==============================================================================
// An endpoint owning its host as a std::string. Parsing and formatting go
// through net.h's span functions, so the C and C++ forms read and write
// exactly the same text.


// 3.1    The endpoint record
//------------------------------------------------------------------------------
// 3.1.1
// endpoint
//   struct: a transport address: a host, a port, and a protocol. `host` is a
// name or a numeric literal, resolved by a backend, never here; for
// protocol::unix_socket it is a filesystem path and `port` is 0.
struct endpoint
{
    endpoint()
        : host(),
          port(0),
          proto(protocol::tcp)
    {}

    endpoint(
        const std::string& _host,
        port_type          _port,
        protocol           _proto = protocol::tcp
    )
        : host(_host),
          port(_port),
          proto(_proto)
    {}

    // local -- a unix-domain endpoint at a path; so named because `unix` is
    // a predefined macro on some platforms
    D_NODISCARD static endpoint
    local(
        const std::string& _path
    )
    {
        return endpoint(_path,
                        0,
                        protocol::unix_socket);
    }

    // is_unix -- whether this is a unix-domain endpoint
    D_NODISCARD bool
    is_unix() const
    {
        return (proto == protocol::unix_socket);
    }

    /**
     * @brief Formats the endpoint as d_net_endpoint_format_parts does.
     *
     * @return "host:port", "[host]:port" for a host containing a colon, or
     *         the path of a unix-domain endpoint.
     */
    D_NODISCARD std::string
    to_string() const
    {
        const ::d_pack_text text   = { host.data(), host.size() };
        const std::size_t   length =
            ::d_net_endpoint_format_parts(text,
                                          port,
                                          to_c(proto),
                                          nullptr,
                                          0);

        // measured first, so the second pass always fits
        std::string out(length + 1,
                        '\0');

        (void)::d_net_endpoint_format_parts(text,
                                            port,
                                            to_c(proto),
                                            &out[0],
                                            out.size());
        out.resize(length);

        return out;
    }

    // equality -- all three fields
    D_NODISCARD bool
    operator==(
        const endpoint& _other
    ) const
    {
        return ( (host == _other.host) &&
                 (port == _other.port) &&
                 (proto == _other.proto) );
    }

    D_NODISCARD bool
    operator!=(
        const endpoint& _other
    ) const
    {
        return (!(*this == _other));
    }

    /**
     * @brief Parses "host:port" or "[host]:port" into an endpoint.
     *
     * The rules are d_net_endpoint_parse_parts's: a host containing a colon
     * must be bracketed, neither the host nor the port may be empty, and
     * the port lies in 1..65535.
     *
     * @param[in]  _text   the text.
     * @param[out] _out    receives the endpoint; unchanged on failure.
     * @param[in]  _proto  the protocol to record; not unix_socket, which has
     *                     no text form -- use local().
     * @return true if the text parsed.
     */
    D_NODISCARD static bool
    parse(
        const std::string& _text,
        endpoint&          _out,
        protocol           _proto = protocol::tcp
    )
    {
        // unix-domain endpoints have no text form
        if (_proto == protocol::unix_socket)
        {
            return false;
        }

        const ::d_pack_text text       = { _text.data(), _text.size() };
        ::d_pack_text       host_text  = { nullptr, 0 };
        port_type           port_value = 0;

        // the C parser validates, and hands back the host within _text
        if (::d_net_endpoint_parse_parts(text,
                                         &host_text,
                                         &port_value) != D_NET_ERROR_NONE)
        {
            return false;
        }

        _out = endpoint(std::string(host_text.data,
                                    host_text.length),
                        port_value,
                        _proto);

        return true;
    }

    std::string host;   // a name, a numeric literal, or a path
    port_type   port;   // host byte order; 0 for unix-domain
    protocol    proto;  // the transport
};

// 3.2    Endpoint conversions
//------------------------------------------------------------------------------
// from_c copies a d_net_endpoint into an endpoint, and cannot fail.
D_NODISCARD inline endpoint
from_c(
    const d_net_endpoint& _endpoint
)
{
    const ::d_pack_text host = ::d_net_endpoint_host(&_endpoint);

    return endpoint(std::string(host.data,
                                host.length),
                    _endpoint.port,
                    from_c(_endpoint.protocol));
}

/**
 * @brief Stores an endpoint in a d_net_endpoint.
 *
 * @param[in]  _endpoint  the endpoint.
 * @param[out] _out       receives it; unchanged on failure.
 * @return io_error::none; io_error::address_invalid for a host longer than
 *         D_NET_HOST_MAX or holding a NUL.
 */
D_NODISCARD inline io_error
to_c(
    const endpoint& _endpoint,
    d_net_endpoint& _out
)
{
    const ::d_pack_text host = { _endpoint.host.data(),
                                 _endpoint.host.size() };

    return from_c(::d_net_endpoint_set(&_out,
                                       host,
                                       _endpoint.port,
                                       to_c(_endpoint.proto)));
}


//==============================================================================
// 4.  CONNECTIONS
//==============================================================================
// The runtime-polymorphic I/O object, and the two bridges that let a C
// connection serve C++ callers and a C++ connection serve C ones.


// 4.1    The connection interface
//------------------------------------------------------------------------------
// 4.1.1
// connection
//   class: an abstract, bidirectional byte stream. Backends derive from it
// and implement the four pure operations; the other three have defaults a
// backend may override. Every operation reports failure by its return value
// and none throws. The base is neither copyable nor directly constructible.
class connection
{
public:
    virtual ~connection() = default;

    /**
     * @brief Reads up to `_size` bytes into `_buffer`.
     *
     * A blocking connection waits until at least one byte arrives, the
     * stream ends, or an error strikes.
     *
     * @param[out] _buffer  receives the bytes.
     * @param[in]  _size    its size in bytes.
     * @return the bytes read and any error; a count of 0 with io_error::none
     *         is the end of the stream.
     */
    D_NODISCARD virtual io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) = 0;

    /**
     * @brief Writes up to `_size` bytes from `_data`.
     *
     * A short count is not an error; write_all insists on the whole buffer.
     *
     * @param[in] _data  the bytes.
     * @param[in] _size  how many.
     * @return the bytes written and any error.
     */
    D_NODISCARD virtual io_result
    write(
        const void* _data,
        std::size_t _size
    ) = 0;

    // is_open -- whether the connection is still usable
    D_NODISCARD virtual bool
    is_open() const = 0;

    // close -- releases the underlying resource; idempotent
    virtual void
    close() = 0;

    /**
     * @brief Closes one or both directions, leaving the resource open.
     *
     * The default does nothing and succeeds; stream backends override it.
     *
     * @param[in] _mode  the direction or directions.
     * @return io_error::none, or the backend's error.
     */
    D_NODISCARD virtual io_error
    shutdown(
        shutdown_mode _mode
    )
    {
        (void)_mode;

        return io_error::none;
    }

    // remote_endpoint, local_endpoint -- the peer's and the local address;
    // the defaults, for backends that cannot tell, are empty endpoints
    D_NODISCARD virtual endpoint
    remote_endpoint() const
    {
        return endpoint();
    }

    D_NODISCARD virtual endpoint
    local_endpoint() const
    {
        return endpoint();
    }

protected:
    connection() = default;
    connection(const connection&)            D_DELETE;
    connection& operator=(const connection&) D_DELETE;
};

// 4.1.2
// open_result
//   struct: an owned connection, or the error that prevented producing one;
// the shared return type of connect and accept. Move-only.
struct open_result
{
    open_result()
        : conn(),
          error(io_error::none)
    {}

    explicit open_result(
        io_error _error
    )
        : conn(),
          error(_error)
    {}

    explicit open_result(
        std::unique_ptr<connection> _conn
    )
        : conn(std::move(_conn)),
          error(io_error::none)
    {}

    // ok -- whether a connection was produced, with no error
    D_NODISCARD bool
    ok() const
    {
        return ( (conn != nullptr) &&
                 (error == io_error::none) );
    }

    // get -- the connection, still owned here, or nullptr
    D_NODISCARD connection*
    get() const
    {
        return conn.get();
    }

    // release -- hands ownership of the connection to the caller
    D_NODISCARD connection*
    release()
    {
        return conn.release();
    }

    // access -- the owned connection; undefined when there is none
    D_NODISCARD connection&
    operator*() const
    {
        return *conn;
    }

    D_NODISCARD connection*
    operator->() const
    {
        return conn.get();
    }

    std::unique_ptr<connection> conn;   // the connection, when ok
    io_error                    error;  // why there is none, otherwise
};

// 4.2    Bridges to C
//------------------------------------------------------------------------------
// 4.2.1
// c_connection
//   class: a C++ connection over a C one, which it owns. Every operation
// forwards to net.h's dispatch, so a C backend serves C++ callers unchanged.
// Destruction closes the C connection and releases it through its destroy
// operation, as d_net_connection_destroy does.
class c_connection : public connection
{
public:
    explicit c_connection(
        d_net_connection* _connection
    ) D_NOEXCEPT
        : connection(),
          m_connection(_connection)
    {}

    // moves transfer ownership, leaving the source empty
    c_connection(
        c_connection&& _other
    ) D_NOEXCEPT
        : connection(),
          m_connection(_other.m_connection)
    {
        _other.m_connection = nullptr;
    }

    c_connection&
    operator=(
        c_connection&& _other
    ) D_NOEXCEPT
    {
        // assigning a connection to itself keeps it
        if (this != &_other)
        {
            ::d_net_connection_destroy(m_connection);
            m_connection        = _other.m_connection;
            _other.m_connection = nullptr;
        }

        return *this;
    }

    ~c_connection() override
    {
        ::d_net_connection_destroy(m_connection);
    }

    // the connection's operations, each forwarded to net.h's dispatch
    D_NODISCARD io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) override
    {
        return from_c(::d_net_connection_read(m_connection,
                                              _buffer,
                                              _size));
    }

    D_NODISCARD io_result
    write(
        const void* _data,
        std::size_t _size
    ) override
    {
        return from_c(::d_net_connection_write(m_connection,
                                               _data,
                                               _size));
    }

    D_NODISCARD bool
    is_open() const override
    {
        return ::d_net_connection_is_open(m_connection);
    }

    void
    close() override
    {
        ::d_net_connection_close(m_connection);

        return;
    }

    D_NODISCARD io_error
    shutdown(
        shutdown_mode _mode
    ) override
    {
        return from_c(::d_net_connection_shutdown(m_connection,
                                                  to_c(_mode)));
    }

    D_NODISCARD endpoint
    remote_endpoint() const override
    {
        d_net_endpoint address = {};

        // an unknown address is the empty endpoint
        if (!::d_net_connection_remote_endpoint(m_connection,
                                                &address))
        {
            return endpoint();
        }

        return from_c(address);
    }

    D_NODISCARD endpoint
    local_endpoint() const override
    {
        d_net_endpoint address = {};

        // an unknown address is the empty endpoint
        if (!::d_net_connection_local_endpoint(m_connection,
                                               &address))
        {
            return endpoint();
        }

        return from_c(address);
    }

    // native -- the C connection, still owned here
    D_NODISCARD d_net_connection*
    native() const D_NOEXCEPT
    {
        return m_connection;
    }

    // release -- hands the C connection back, owned by the caller again
    D_NODISCARD d_net_connection*
    release() D_NOEXCEPT
    {
        d_net_connection* const released = m_connection;

        m_connection = nullptr;

        return released;
    }

private:
    d_net_connection* m_connection;  // owned; may be NULL
};

// 4.2.2
// c_connection_adapter
//   class: a C connection over a C++ one, which it borrows. get() returns a
// d_net_connection* that C code -- net.h's algorithms, or a C protocol
// kernel -- drives exactly as it drives a C backend. The adapter must outlive
// every use of that pointer, so it neither copies nor moves. Its operations
// are noexcept: should the connection throw, the program terminates rather
// than unwinding through C frames, which cannot be unwound.
class c_connection_adapter
{
public:
    explicit c_connection_adapter(
        connection& _target
    ) D_NOEXCEPT
        : m_base(),
          m_target(&_target)
    {
        m_base.vtable = &m_vtable();
    }

    c_connection_adapter(const c_connection_adapter&)            D_DELETE;
    c_connection_adapter& operator=(const c_connection_adapter&) D_DELETE;

    // get -- the C connection, valid while the adapter lives
    D_NODISCARD d_net_connection*
    get() D_NOEXCEPT
    {
        return &m_base;
    }

private:
    // m_of -- the adapter whose first member a C connection is
    static c_connection_adapter&
    m_of(
        d_net_connection* _connection
    ) D_NOEXCEPT
    {
        return *reinterpret_cast<c_connection_adapter*>(_connection);
    }

    static const c_connection_adapter&
    m_of(
        const d_net_connection* _connection
    ) D_NOEXCEPT
    {
        return *reinterpret_cast<const c_connection_adapter*>(_connection);
    }

    // m_export -- stores a known address; an empty host means unknown
    static bool
    m_export(
        const endpoint& _address,
        d_net_endpoint* _out
    ) D_NOEXCEPT
    {
        return ( (!_address.host.empty()) &&
                 (to_c(_address,
                       *_out) == io_error::none) );
    }

    // the operations, each forwarded to the borrowed connection
    static d_net_io_result
    m_read(
        d_net_connection* _connection,
        void*             _buffer,
        std::size_t       _size
    ) D_NOEXCEPT
    {
        return to_c(m_of(_connection).m_target->read(_buffer,
                                                     _size));
    }

    static d_net_io_result
    m_write(
        d_net_connection* _connection,
        const void*       _data,
        std::size_t       _size
    ) D_NOEXCEPT
    {
        return to_c(m_of(_connection).m_target->write(_data,
                                                      _size));
    }

    static bool
    m_is_open(
        const d_net_connection* _connection
    ) D_NOEXCEPT
    {
        return m_of(_connection).m_target->is_open();
    }

    static void
    m_close(
        d_net_connection* _connection
    ) D_NOEXCEPT
    {
        m_of(_connection).m_target->close();

        return;
    }

    static d_net_error
    m_shutdown(
        d_net_connection* _connection,
        d_net_shutdown    _mode
    ) D_NOEXCEPT
    {
        return to_c(m_of(_connection).m_target->shutdown(from_c(_mode)));
    }

    static bool
    m_remote_endpoint(
        const d_net_connection* _connection,
        d_net_endpoint*         _out
    ) D_NOEXCEPT
    {
        return m_export(m_of(_connection).m_target->remote_endpoint(),
                        _out);
    }

    static bool
    m_local_endpoint(
        const d_net_connection* _connection,
        d_net_endpoint*         _out
    ) D_NOEXCEPT
    {
        return m_export(m_of(_connection).m_target->local_endpoint(),
                        _out);
    }

    // m_vtable -- the one table every adapter points at; no destroy, since
    // the adapter's storage belongs to its owner
    static const d_net_connection_vtable&
    m_vtable() D_NOEXCEPT
    {
        static const d_net_connection_vtable table =
        {
            &c_connection_adapter::m_read,
            &c_connection_adapter::m_write,
            &c_connection_adapter::m_is_open,
            &c_connection_adapter::m_close,
            &c_connection_adapter::m_shutdown,
            &c_connection_adapter::m_remote_endpoint,
            &c_connection_adapter::m_local_endpoint,
            nullptr
        };

        return table;
    }

    d_net_connection m_base;    // the C head; must stay first
    connection*      m_target;  // borrowed
};

static_assert(std::is_standard_layout<c_connection_adapter>::value,
              "c_connection_adapter must be standard-layout, so that its "
              "first member converts to the adapter and back");


//==============================================================================
// 5.  BYTE STREAMS
//==============================================================================
// A byte stream is any type offering the connection surface statically: read
// and write returning io_result, is_open convertible to bool, and close. The
// abstract connection is one, as is every concrete backend.


// 5.1    Stream detection
//------------------------------------------------------------------------------
NS_INTERNAL

    // is_byte_stream_helper
    //   trait: detects the four operations. m_check's first overload
    // survives substitution only when all four are present with their
    // shapes; the second catches every other type.
    template<typename Type>
    struct is_byte_stream_helper
    {
    private:
        template<typename Candidate>
        static std::integral_constant<
            bool,
            ( std::is_same<decltype(std::declval<Candidate&>().read(
                               std::declval<void*>(),
                               std::declval<std::size_t>())),
                           io_result>::value &&
              std::is_same<decltype(std::declval<Candidate&>().write(
                               std::declval<const void*>(),
                               std::declval<std::size_t>())),
                           io_result>::value &&
              std::is_convertible<decltype(
                                      std::declval<Candidate&>().is_open()),
                                  bool>::value &&
              std::is_void<decltype(static_cast<void>(
                               std::declval<Candidate&>().close()))>::value )>
        m_check(int);

        template<typename Candidate>
        static std::false_type
        m_check(...);

    public:
        static constexpr bool value = decltype(m_check<Type>(0))::value;
    };

NS_END  // internal

// 5.1.1
// is_byte_stream
//   trait: whether Type is a byte stream; available from C++11.
template<typename Type>
struct is_byte_stream
    : std::integral_constant<bool,
                             internal::is_byte_stream_helper<Type>::value>
{};

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// 5.1.2
// byte_stream
//   concept: is_byte_stream as a concept, for constraining templates; C++20
// and later.
template<typename Type>
concept byte_stream = is_byte_stream<Type>::value;
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

// 5.2    Buffers
//------------------------------------------------------------------------------
NS_INTERNAL

    // append_bytes
    //   function: appends bytes to a result buffer. The algorithms accept a
    // std::vector<byte>, a std::string, or any buffer with a member
    // append(const byte*, std::size_t) -- djinterp::byte_buffer among them.
    inline void
    append_bytes(
        std::vector<byte>& _out,
        const byte*        _data,
        std::size_t        _size
    )
    {
        _out.insert(_out.end(),
                    _data,
                    _data + _size);

        return;
    }

    inline void
    append_bytes(
        std::string& _out,
        const byte*  _data,
        std::size_t  _size
    )
    {
        _out.append(reinterpret_cast<const char*>(_data),
                    _size);

        return;
    }

    template<typename Buffer>
    auto
    append_bytes(
        Buffer&     _out,
        const byte* _data,
        std::size_t _size
    ) -> decltype(static_cast<void>(_out.append(_data,
                                                 _size)))
    {
        _out.append(_data,
                    _size);

        return;
    }

NS_END  // internal


//==============================================================================
// 6.  STREAM ALGORITHMS
//==============================================================================
// Templates over any byte stream: a concrete backend is called directly, and
// the abstract connection through its virtuals. Each asserts that its stream
// is one. Scratch space is a stack buffer of D_NET_IO_CHUNK bytes, the size
// net.h's algorithms use; results go to a caller's buffer.


// 6.1    Defaults
//------------------------------------------------------------------------------
// 6.1.1
// default_io_chunk
//   constant: the most a bulk algorithm reads at once by default; cfg_net.h's
// D_NET_IO_CHUNK, which also bounds any larger request.
D_CONSTEXPR std::size_t default_io_chunk = D_NET_IO_CHUNK;

// 6.1.2
// default_max_frame
//   constant: the default ceiling on a frame's payload; cfg_net.h's
// D_NET_FRAME_MAX.
D_CONSTEXPR std::size_t default_max_frame = D_NET_FRAME_MAX;

// 6.2    Exact transfers
//------------------------------------------------------------------------------
/**
 * @brief Reads exactly `_size` bytes, looping over short reads.
 *
 * @param[in,out] _stream  the stream.
 * @param[out]    _buffer  receives the bytes.
 * @param[in]     _size    how many to read.
 * @return io_error::none once all are read; io_error::closed if the stream
 *         ends first; otherwise the stream's error, or
 *         io_error::invalid_argument for a NULL `_buffer` with a nonzero
 *         `_size`.
 */
template<typename Stream>
D_NODISCARD io_error
read_exactly(
    Stream&     _stream,
    void*       _buffer,
    std::size_t _size
)
{
    static_assert(is_byte_stream<Stream>::value,
                  "read_exactly: the stream must be a byte stream");

    // parameter validation
    if ( (!_buffer) &&
         (_size != 0) )
    {
        return io_error::invalid_argument;
    }

    byte* const out  = static_cast<byte*>(_buffer);
    std::size_t done = 0;

    // read until the request is met
    while (done < _size)
    {
        const io_result result = _stream.read(out + done,
                                              _size - done);

        done += result.count;

        // a failure ends the read
        if (!result.ok())
        {
            return result.error;
        }

        // so does the end of the stream, before the request was met
        if (result.count == 0)
        {
            return io_error::closed;
        }
    }

    return io_error::none;
}

/**
 * @brief Writes all of `_data`, looping over short writes.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _data    the bytes.
 * @param[in]     _size    how many.
 * @return io_error::none once all are written; io_error::closed if the
 *         stream takes nothing without an error; otherwise the failures of
 *         read_exactly.
 */
template<typename Stream>
D_NODISCARD io_error
write_all(
    Stream&     _stream,
    const void* _data,
    std::size_t _size
)
{
    static_assert(is_byte_stream<Stream>::value,
                  "write_all: the stream must be a byte stream");

    // parameter validation
    if ( (!_data) &&
         (_size != 0) )
    {
        return io_error::invalid_argument;
    }

    const byte* const in   = static_cast<const byte*>(_data);
    std::size_t       done = 0;

    // write until everything is taken
    while (done < _size)
    {
        const io_result result = _stream.write(in + done,
                                               _size - done);

        done += result.count;

        // a failure ends the write
        if (!result.ok())
        {
            return result.error;
        }

        // so does a stream that takes nothing
        if (result.count == 0)
        {
            return io_error::closed;
        }
    }

    return io_error::none;
}

// write_all -- the string form: the string's bytes, without a terminator
template<typename Stream>
D_NODISCARD io_error
write_all(
    Stream&            _stream,
    const std::string& _text
)
{
    return write_all(_stream,
                     _text.data(),
                     _text.size());
}

// 6.3    Bulk transfers
//------------------------------------------------------------------------------
/**
 * @brief Reads once, appending what arrives to `_out`.
 *
 * @param[in,out] _stream  the stream.
 * @param[in,out] _out     the buffer appended to.
 * @param[in]     _max     the most to read, capped at D_NET_IO_CHUNK; 0
 *                         reads nothing.
 * @return the read's result; its bytes are appended even alongside an
 *         error.
 */
template<typename Stream,
         typename Buffer>
D_NODISCARD io_result
read_available(
    Stream&     _stream,
    Buffer&     _out,
    std::size_t _max = default_io_chunk
)
{
    static_assert(is_byte_stream<Stream>::value,
                  "read_available: the stream must be a byte stream");

    // a request for nothing is answered here
    if (_max == 0)
    {
        return io_result();
    }

    // the scratch space one read lands in
    byte chunk[D_NET_IO_CHUNK] = {};

    const std::size_t want   = (_max < sizeof(chunk)) ? _max
                                                      : sizeof(chunk);
    const io_result   result = _stream.read(chunk,
                                            want);

    // keep whatever arrived
    if (result.count > 0)
    {
        internal::append_bytes(_out,
                               chunk,
                               result.count);
    }

    return result;
}

/**
 * @brief Reads to the end of the stream, appending everything to `_out`.
 *
 * @param[in,out] _stream  the stream.
 * @param[in,out] _out     the buffer appended to.
 * @param[in]     _chunk   the most each read asks for, capped at
 *                         D_NET_IO_CHUNK.
 * @return io_error::none at a clean end of stream; the stream's error; or
 *         io_error::invalid_argument for a `_chunk` of 0.
 */
template<typename Stream,
         typename Buffer>
D_NODISCARD io_error
read_all(
    Stream&     _stream,
    Buffer&     _out,
    std::size_t _chunk = default_io_chunk
)
{
    static_assert(is_byte_stream<Stream>::value,
                  "read_all: the stream must be a byte stream");

    // parameter validation: a zero chunk would read nothing, forever
    if (_chunk == 0)
    {
        return io_error::invalid_argument;
    }

    // read until the stream ends or fails
    for (;;)
    {
        const io_result result = read_available(_stream,
                                                _out,
                                                _chunk);

        // a failure, or the end of the stream, ends the read
        if ( (!result.ok()) ||
             (result.count == 0) )
        {
            return result.error;
        }
    }
}

/**
 * @brief Copies one stream into another until the source ends.
 *
 * @param[in,out] _source  the stream read from.
 * @param[in,out] _sink    the stream written to.
 * @param[in]     _chunk   the most each read asks for, capped at
 *                         D_NET_IO_CHUNK.
 * @return io_error::none when the source ends cleanly; otherwise the first
 *         error from either stream, or io_error::invalid_argument for a
 *         `_chunk` of 0.
 */
template<typename Source,
         typename Sink>
D_NODISCARD io_error
pump(
    Source&     _source,
    Sink&       _sink,
    std::size_t _chunk = default_io_chunk
)
{
    static_assert(is_byte_stream<Source>::value,
                  "pump: the source must be a byte stream");
    static_assert(is_byte_stream<Sink>::value,
                  "pump: the sink must be a byte stream");

    // parameter validation: a zero chunk would copy nothing, forever
    if (_chunk == 0)
    {
        return io_error::invalid_argument;
    }

    // the scratch space each chunk passes through
    byte chunk[D_NET_IO_CHUNK] = {};

    const std::size_t want = (_chunk < sizeof(chunk)) ? _chunk
                                                      : sizeof(chunk);

    // copy until the source ends or either side fails
    for (;;)
    {
        const io_result result = _source.read(chunk,
                                              want);
        const io_error  sent   = write_all(_sink,
                                           chunk,
                                           result.count);

        // the sink's failure takes precedence: its bytes are the ones lost
        if (sent != io_error::none)
        {
            return sent;
        }

        // a failure, or the end of the source, ends the copy
        if ( (!result.ok()) ||
             (result.count == 0) )
        {
            return result.error;
        }
    }
}

// 6.4    Framing
//------------------------------------------------------------------------------
/**
 * @brief Writes one frame: a 4-byte big-endian length, then the payload.
 *
 * The wire form is net.h's, through the same header codec. A frame no larger
 * than D_NET_IO_CHUNK goes out in a single write, so its header and payload
 * never straddle a Nagle delay.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _data    the payload; may be NULL when `_size` is 0.
 * @param[in]     _size    its length.
 * @param[in]     _max     the ceiling.
 * @return io_error::none; io_error::message_too_large, with nothing
 *         written, above the ceiling or 2^32 - 1; or the failures of
 *         write_all.
 */
template<typename Stream>
D_NODISCARD io_error
write_frame(
    Stream&     _stream,
    const void* _data,
    std::size_t _size,
    std::size_t _max = default_max_frame
)
{
    static_assert(is_byte_stream<Stream>::value,
                  "write_frame: the stream must be a byte stream");

    // parameter validation
    if ( (!_data) &&
         (_size != 0) )
    {
        return io_error::invalid_argument;
    }

    const io_error checked = from_c(::d_net_frame_check(_size,
                                                        _max));

    // an oversized frame writes nothing
    if (checked != io_error::none)
    {
        return checked;
    }

    // the header, and room for a small payload behind it
    byte frame[D_NET_IO_CHUNK] = {};

    ::d_net_frame_header_encode(static_cast<std::uint32_t>(_size),
                                frame);

    // a frame that fits the buffer goes out in one write
    if (_size <= (sizeof(frame) - D_NET_FRAME_HEADER_SIZE))
    {
        // an empty payload has no data to copy
        if (_size != 0)
        {
            std::memcpy(frame + D_NET_FRAME_HEADER_SIZE,
                        _data,
                        _size);
        }

        return write_all(_stream,
                         frame,
                         D_NET_FRAME_HEADER_SIZE + _size);
    }

    const io_error header = write_all(_stream,
                                      frame,
                                      D_NET_FRAME_HEADER_SIZE);

    // the payload follows only a header that went out whole
    if (header != io_error::none)
    {
        return header;
    }

    return write_all(_stream,
                     _data,
                     _size);
}

// write_frame -- the string form: the string's bytes as the payload
template<typename Stream>
D_NODISCARD io_error
write_frame(
    Stream&            _stream,
    const std::string& _text,
    std::size_t        _max = default_max_frame
)
{
    return write_frame(_stream,
                       _text.data(),
                       _text.size(),
                       _max);
}

/**
 * @brief Reads one frame, replacing `_out` with its payload.
 *
 * A declared length above the ceiling is refused before its payload is read,
 * which leaves the stream mid-frame: close the connection after it.
 *
 * @param[in,out] _stream  the stream.
 * @param[out]    _out     receives the payload; empty after any failure.
 * @param[in]     _max     the ceiling.
 * @return io_error::none; io_error::message_too_large above the ceiling;
 *         io_error::closed if the stream ends within the frame; otherwise
 *         the failures of read_exactly.
 */
template<typename Stream,
         typename Buffer>
D_NODISCARD io_error
read_frame(
    Stream&     _stream,
    Buffer&     _out,
    std::size_t _max = default_max_frame
)
{
    static_assert(is_byte_stream<Stream>::value,
                  "read_frame: the stream must be a byte stream");

    _out.clear();

    // the header, then the scratch space the payload passes through
    byte header[D_NET_FRAME_HEADER_SIZE] = {};
    byte chunk[D_NET_IO_CHUNK]           = {};

    const io_error head = read_exactly(_stream,
                                       header,
                                       sizeof(header));

    // without a whole header there is no frame
    if (head != io_error::none)
    {
        return head;
    }

    std::size_t remaining = ::d_net_frame_header_decode(header);

    // a length above the ceiling is refused before its payload is read
    if (remaining > _max)
    {
        return io_error::message_too_large;
    }

    // collect the payload a chunk at a time
    while (remaining > 0)
    {
        const std::size_t want = (remaining < sizeof(chunk)) ? remaining
                                                             : sizeof(chunk);
        const io_error    part = read_exactly(_stream,
                                              chunk,
                                              want);

        // a short or failed read leaves no partial payload behind
        if (part != io_error::none)
        {
            _out.clear();

            return part;
        }

        internal::append_bytes(_out,
                               chunk,
                               want);
        remaining -= want;
    }

    return io_error::none;
}


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_NET_HPP
