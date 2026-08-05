/******************************************************************************
* djinterp [net]                                                       net.hpp
*
*   Foundational, library-agnostic networking module. It is the backbone every
* transport backend (POSIX sockets, TLS, in-process, ...) and both the client
* and server foundations build on. It owns the shared transport vocabulary, the
* runtime-polymorphic connection interface, the compile-time byte_stream
* concept that mirrors it, and the generic stream algorithms (exact read/write,
* length-prefixed framing, pumping). It depends on NO OS or socket library --
* platform code lives in backends, the way curl.hpp followed web.hpp.
*
* CONTENTS (all in namespace djinterp::net):
*   I.    namespace + version macros
*   II.   byte / buffer / port aliases
*   III.  io_error + to_string ; io_result ; shutdown_mode ; protocol
*   IV.   endpoint (address vocabulary, host:port / IPv6 / unix parsing)
*   V.    connection (abstract, virtual I/O object) + open_result
*   VI.   byte_stream concept (static mirror of the connection surface)
*   VII.  generic stream algorithms
*         - read_exactly / write_all / read_all / read_available / pump
*         - write_frame / read_frame (u32 big-endian length prefix + max guard)
*
*   DISPATCH: hold many connections polymorphically through `connection&`
* (vtable indirection is free next to a syscall, and enables runtime transport
* choice + type-erased connection tables). Write hot generic code against the
* `byte_stream` concept for zero-overhead static dispatch on a concrete backend.
* The abstract connection models byte_stream, so the SAME algorithm serves both.
*
*   Requires C++20 (concepts). Error handling is status/return-value based, with
* no exceptions, matching web.hpp / curl.hpp.
*
* path:      /inc/djinterp/net/net.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_NET_
#define DJINTERP_NET_ 1

// std
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/container/buffer/byte_buffer.hpp"


#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "net.hpp requires C++20 or later (concepts)"
#endif


///////////////////////////////////////////////////////////////////////////////
///                 I.   NAMESPACE + VERSION MACROS                        ///
///////////////////////////////////////////////////////////////////////////////

// D_KEYWORD_NET
//   keyword: resolves to `net`. Used for identifiers, macros, and namespaces
// pertaining to the networking subframework. Guarded so the core may adopt it.
#ifndef D_KEYWORD_NET
    #define D_KEYWORD_NET               net
#endif

// NS_NET
//   namespace: the `net` namespace, the root of networking functionality. Nest
// inside NS_DJINTERP; close with NS_END.
#ifndef NS_NET
    #define NS_NET                      D_NAMESPACE(D_KEYWORD_NET)
#endif

// D_NET_VERSION_MAJOR / _MINOR / _PATCH / _STRING
//   macro: semantic version of the net subframework.
#define D_NET_VERSION_MAJOR         0
#define D_NET_VERSION_MINOR         1
#define D_NET_VERSION_PATCH         0
#define D_NET_VERSION_STRING        "0.1.0"


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                    II.   BYTE / BUFFER / PORT                          ///
///////////////////////////////////////////////////////////////////////////////

// byte
//   type: a single octet of wire data.
using byte = unsigned char;

// byte_buffer
//   type: the framework's growable byte buffer -- an encode/decode-capable
// staged accumulator (core/container/buffer). Wire payloads and frames are
// carried in it, so a received frame can be decoded, or an outbound one
// composed, without an intermediate vector.
using byte_buffer = ::djinterp::byte_buffer<>;

// bytes
//   type: convenience alias for byte_buffer.
using bytes = byte_buffer;

// port_type
//   type: a TCP/UDP port number.
using port_type = std::uint16_t;


///////////////////////////////////////////////////////////////////////////////
///           III.   IO ERROR / RESULT / SHUTDOWN / PROTOCOL               ///
///////////////////////////////////////////////////////////////////////////////

// io_error
//   enum: a library-neutral classification of transport-level failures onto
// which every backend maps its native error codes (errno, WSAGetLastError,
// TLS library codes) so higher layers reason about failures portably. `none`
// is success; `would_block` supports non-blocking backends even though this
// foundation is blocking.
enum class io_error : unsigned char
{
    none = 0,
    closed,              // peer closed / end of stream where data was required
    would_block,         // non-blocking op has no data / cannot proceed now
    timed_out,           // operation exceeded its deadline
    interrupted,         // interrupted by a signal; typically retryable
    connection_reset,    // peer reset the connection
    connection_refused,  // no listener at the target
    connection_aborted,  // local abort (e.g. failed accept)
    address_in_use,      // bind target already in use
    address_invalid,     // malformed / unusable address
    host_unreachable,    // no route to host
    network_down,        // local network interface down / unreachable
    access_denied,       // insufficient permission
    invalid_argument,    // caller-side misuse
    message_too_large,   // framed message exceeds the configured maximum
    too_many_open_files, // process/system descriptor limit reached
    out_of_memory,       // allocation failure
    unknown
};

// to_string(io_error)
//   function: a short human-readable label for an io_error.
D_NODISCARD inline const char*
to_string(
    io_error _error
)
{
    switch (_error)
    {
        case io_error::none:                 return "none";
        case io_error::closed:               return "closed";
        case io_error::would_block:          return "would block";
        case io_error::timed_out:            return "timed out";
        case io_error::interrupted:          return "interrupted";
        case io_error::connection_reset:     return "connection reset";
        case io_error::connection_refused:   return "connection refused";
        case io_error::connection_aborted:   return "connection aborted";
        case io_error::address_in_use:       return "address in use";
        case io_error::address_invalid:      return "address invalid";
        case io_error::host_unreachable:     return "host unreachable";
        case io_error::network_down:         return "network down";
        case io_error::access_denied:        return "access denied";
        case io_error::invalid_argument:     return "invalid argument";
        case io_error::message_too_large:    return "message too large";
        case io_error::too_many_open_files:  return "too many open files";
        case io_error::out_of_memory:        return "out of memory";
        case io_error::unknown:              return "unknown error";
    }

    return "unknown error";
}

// io_result
//   struct: the outcome of a single read/write -- the number of bytes actually
// transferred plus any error. A result of count 0 with error none is a clean
// end-of-stream (EOF).
struct io_result
{
    std::size_t count;
    io_error    error;

    io_result()
        : count(0),
          error(io_error::none)
    {
    }

    io_result(
        std::size_t _count,
        io_error    _error
    )
        : count(_count),
          error(_error)
    {
    }

    // ok
    //   function: whether no error occurred (EOF is not an error).
    D_NODISCARD bool
    ok() const
    {
        return (error == io_error::none);
    }

    // eof
    //   function: whether this is a clean end-of-stream (no bytes, no error).
    D_NODISCARD bool
    eof() const
    {
        return (count == 0) && (error == io_error::none);
    }
};

// shutdown_mode
//   enum: which half (or both) of a duplex connection to shut down.
enum class shutdown_mode : unsigned char
{
    read,
    write,
    both
};

// protocol
//   enum: the transport a connection or endpoint uses.
enum class protocol : unsigned char
{
    tcp,
    udp,
    unix_socket
};


///////////////////////////////////////////////////////////////////////////////
///                        IV.   ENDPOINT                                  ///
///////////////////////////////////////////////////////////////////////////////

// endpoint
//   struct: a transport address -- a host plus a port and protocol. `host` is
// an opaque string (a name or a numeric literal); actual name resolution is a
// backend concern. For unix_socket, `host` carries the filesystem path and
// `port` is unused. Construct network endpoints via the (host, port) ctor and
// domain-socket endpoints via local().
struct endpoint
{
    std::string host;
    port_type   port;
    protocol    proto;

    endpoint()
        : host(),
          port(0),
          proto(protocol::tcp)
    {
    }

    endpoint(
        const std::string& _host,
        port_type          _port,
        protocol           _proto = protocol::tcp
    )
        : host(_host),
          port(_port),
          proto(_proto)
    {
    }

    // local
    //   function: a unix-domain-socket endpoint for the given filesystem path.
    // (Named local() rather than unix() because `unix` is a legacy predefined
    // macro on some toolchains.)
    D_NODISCARD static endpoint
    local(
        const std::string& _path
    )
    {
        endpoint e;
        e.host  = _path;
        e.port  = 0;
        e.proto = protocol::unix_socket;

        return e;
    }

    // is_unix
    //   function: whether this endpoint names a unix-domain socket.
    D_NODISCARD bool
    is_unix() const
    {
        return (proto == protocol::unix_socket);
    }

    // to_string
    //   function: a display form -- the path for unix sockets, "host:port" for
    // named/IPv4 hosts, and "[host]:port" when the host looks like an IPv6
    // literal (contains a colon).
    D_NODISCARD std::string
    to_string() const
    {
        if (is_unix())
        {
            return host;
        }

        const bool looks_v6 = (host.find(':') != std::string::npos);

        std::string out;

        if (looks_v6)
        {
            out += "[";
            out += host;
            out += "]";
        }
        else
        {
            out += host;
        }

        out += ":";
        out += std::to_string(static_cast<unsigned>(port));

        return out;
    }

    // operator==
    //   function: field-wise endpoint equality.
    D_NODISCARD bool
    operator==(
        const endpoint& _other
    ) const
    {
        return ( (host  == _other.host)  &&
                 (port  == _other.port)  &&
                 (proto == _other.proto) );
    }

    // parse
    //   function: parses "host:port" (or "[ipv6]:port") into `_out`, returning
    // true on success. The port must be a decimal number in 1..65535. This
    // parses network addresses only; build unix endpoints with local().
    D_NODISCARD static bool
    parse(
        const std::string& _text,
        endpoint&          _out,
        protocol           _proto = protocol::tcp
    )
    {
        std::string host_part;
        std::string port_part;

        // bracketed IPv6 literal: [addr]:port
        if ( (!_text.empty()) &&
             (_text[0] == '[') )
        {
            const std::size_t rb = _text.find(']');

            if (rb == std::string::npos)
            {
                return false;
            }

            host_part = _text.substr(1, rb - 1);

            // require ":port" immediately after the closing bracket
            if ( ((rb + 1) >= _text.size()) ||
                 (_text[rb + 1] != ':') )
            {
                return false;
            }

            port_part = _text.substr(rb + 2);
        }
        else
        {
            // split on the LAST colon (host may itself be a bare IPv6? no --
            // bare IPv6 must be bracketed; here host has no colons)
            const std::size_t colon = _text.rfind(':');

            if (colon == std::string::npos)
            {
                return false;
            }

            host_part = _text.substr(0, colon);
            port_part = _text.substr(colon + 1);
        }

        // parse the port digits
        if ( host_part.empty() ||
             port_part.empty() )
        {
            return false;
        }

        unsigned long value = 0;

        for (std::size_t i = 0; i < port_part.size(); ++i)
        {
            const char c = port_part[i];

            if ( (c < '0') || (c > '9') )
            {
                return false;
            }

            value = (value * 10) + static_cast<unsigned long>(c - '0');

            if (value > 65535)
            {
                return false;
            }
        }

        if (value == 0)
        {
            return false;
        }

        _out.host  = host_part;
        _out.port  = static_cast<port_type>(value);
        _out.proto = _proto;

        return true;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   CONNECTION (ABSTRACT) + OPEN RESULT                ///
///////////////////////////////////////////////////////////////////////////////

// connection
//   class: the live, runtime-polymorphic I/O object every backend produces. It
// owns an underlying OS resource, so it is non-copyable; derived types manage
// their own moves. The core surface is the minimal read/write/is_open/close;
// the remaining virtuals carry safe default implementations backends may
// override. All operations report failure by return value -- none throw.
class connection
{
public:
    virtual ~connection() = default;

    // read
    //   function: reads up to `_size` bytes into `_buffer`. The result's count
    // is the number read (0 with error none means EOF); a blocking connection
    // blocks until at least one byte is available, EOF, or an error.
    D_NODISCARD virtual io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) = 0;

    // write
    //   function: writes up to `_size` bytes from `_data`. The result's count
    // is the number accepted; a partial write is not an error (use write_all to
    // insist on the whole buffer).
    D_NODISCARD virtual io_result
    write(
        const void* _data,
        std::size_t _size
    ) = 0;

    // is_open
    //   function: whether the connection is currently usable.
    D_NODISCARD virtual bool
    is_open() const = 0;

    // close
    //   function: releases the underlying resource. Idempotent; safe to call on
    // an already-closed connection.
    virtual void
    close() = 0;

    // shutdown
    //   function: shuts down one or both directions while leaving the resource
    // open (a half-close). The default is a no-op returning success; stream
    // backends override it.
    D_NODISCARD virtual io_error
    shutdown(
        shutdown_mode _mode
    )
    {
        (void)_mode;

        return io_error::none;
    }

    // remote_endpoint / local_endpoint
    //   function: the peer / local address, when known. The defaults return an
    // empty endpoint; backends override where they can supply one.
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

// open_result
//   struct: an owned connection or the error that prevented producing one. The
// shared return type of both connect (client) and accept (server). Move-only
// (it holds a unique_ptr).
struct open_result
{
    std::unique_ptr<connection> conn;
    io_error                    error;

    open_result()
        : conn(),
          error(io_error::none)
    {
    }

    explicit open_result(
        io_error _error
    )
        : conn(),
          error(_error)
    {
    }

    explicit open_result(
        std::unique_ptr<connection> _conn
    )
        : conn(std::move(_conn)),
          error(io_error::none)
    {
    }

    // ok
    //   function: whether a connection was produced with no error.
    D_NODISCARD bool
    ok() const
    {
        return (conn != nullptr) && (error == io_error::none);
    }

    // get
    //   function: the raw connection pointer (ownership retained), or nullptr.
    D_NODISCARD connection*
    get() const
    {
        return conn.get();
    }

    // release
    //   function: relinquishes ownership of the connection to the caller.
    D_NODISCARD connection*
    release()
    {
        return conn.release();
    }

    // operator* / operator->
    //   function: access to the owned connection (undefined when none).
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
};


///////////////////////////////////////////////////////////////////////////////
///                     VI.   BYTE_STREAM CONCEPT                          ///
///////////////////////////////////////////////////////////////////////////////

// byte_stream
//   concept: the compile-time mirror of the connection surface -- any type
// offering the core read/write/is_open/close operations with the expected
// shapes. The abstract connection models it (enabling virtual dispatch through
// the generic algorithms below), and so does any concrete backend type
// (enabling zero-overhead static dispatch).
template<typename _Type>
concept byte_stream = requires(_Type&       _stream,
                               void*        _out,
                               const void*  _in,
                               std::size_t  _size)
{
    { _stream.read(_out, _size) }  -> std::same_as<io_result>;
    { _stream.write(_in, _size) }  -> std::same_as<io_result>;
    { _stream.is_open() }          -> std::convertible_to<bool>;
    _stream.close();
};


///////////////////////////////////////////////////////////////////////////////
///                 VII.   GENERIC STREAM ALGORITHMS                       ///
///////////////////////////////////////////////////////////////////////////////

// default_max_frame
//   constant: the default ceiling for a single framed message (64 MiB). Guards
// read_frame against a hostile or corrupt length prefix demanding a huge
// allocation.
D_CONSTEXPR std::size_t default_max_frame = static_cast<std::size_t>(64) * 1024 * 1024;

// read_exactly
//   function: reads exactly `_size` bytes into `_buffer`, looping over short
// reads. Returns none on success, closed if EOF arrives first, or the transport
// error otherwise.
template<byte_stream _Stream>
D_NODISCARD io_error
read_exactly(
    _Stream&    _stream,
    void*       _buffer,
    std::size_t _size
)
{
    byte*       out = static_cast<byte*>(_buffer);
    std::size_t got = 0;

    // accumulate until the full request is satisfied
    while (got < _size)
    {
        const io_result r = _stream.read(out + got, _size - got);

        if (!r.ok())
        {
            return r.error;
        }

        // EOF before the requested count
        if (r.count == 0)
        {
            return io_error::closed;
        }

        got += r.count;
    }

    return io_error::none;
}

// write_all
//   function: writes the entire `_size`-byte buffer, looping over short writes.
// Returns none on success or the transport error otherwise.
template<byte_stream _Stream>
D_NODISCARD io_error
write_all(
    _Stream&    _stream,
    const void* _data,
    std::size_t _size
)
{
    const byte* in   = static_cast<const byte*>(_data);
    std::size_t sent = 0;

    // push until the whole buffer is accepted
    while (sent < _size)
    {
        const io_result r = _stream.write(in + sent, _size - sent);

        if (!r.ok())
        {
            return r.error;
        }

        // a well-behaved blocking write should not accept 0 bytes without error
        if (r.count == 0)
        {
            return io_error::closed;
        }

        sent += r.count;
    }

    return io_error::none;
}

// write_all (string)
//   function: writes an entire std::string's bytes.
template<byte_stream _Stream>
D_NODISCARD io_error
write_all(
    _Stream&           _stream,
    const std::string& _data
)
{
    return write_all(_stream, _data.data(), _data.size());
}

// read_available
//   function: performs a single read of up to `_max` bytes, appending whatever
// arrives to `_out`. Sets `_result` to the underlying io_result so the caller
// can distinguish EOF (count 0, ok) from an error.
template<byte_stream _Stream>
D_NODISCARD io_result
read_available(
    _Stream&     _stream,
    byte_buffer& _out,
    std::size_t  _max = 4096
)
{
    // read into a scratch region, then append what actually arrived: the
    // byte_buffer accumulates through its append protocol, not a raw resize
    std::vector<byte> scratch(_max);

    const io_result r = _stream.read(scratch.data(), _max);

    if (r.count > 0)
    {
        _out.append(scratch.data(), r.count);
    }

    return r;
}

// read_all
//   function: reads the stream to EOF, appending everything to `_out`. Returns
// none on a clean EOF or the transport error that ended the read.
template<byte_stream _Stream>
D_NODISCARD io_error
read_all(
    _Stream&     _stream,
    byte_buffer& _out,
    std::size_t  _chunk = 4096
)
{
    // read chunks until end-of-stream or error
    for (;;)
    {
        const io_result r = read_available(_stream, _out, _chunk);

        if (!r.ok())
        {
            return r.error;
        }

        // clean EOF
        if (r.count == 0)
        {
            return io_error::none;
        }
    }
}

// pump
//   function: copies bytes from `_source` to `_sink` until the source reaches
// EOF. Returns none on completion, or the first transport error encountered on
// either side. Useful for proxying one connection to another.
template<byte_stream _Source, byte_stream _Sink>
D_NODISCARD io_error
pump(
    _Source&    _source,
    _Sink&      _sink,
    std::size_t _chunk = 4096
)
{
    byte_buffer buffer(_chunk);

    // read a chunk, write it whole, repeat until EOF
    for (;;)
    {
        const io_result r = _source.read(buffer.data(), _chunk);

        if (!r.ok())
        {
            return r.error;
        }

        if (r.count == 0)
        {
            return io_error::none;
        }

        const io_error w = write_all(_sink, buffer.data(), r.count);

        if (w != io_error::none)
        {
            return w;
        }
    }
}

// write_frame
//   function: writes a length-prefixed frame -- a 4-byte big-endian (network
// order) unsigned length followed by the payload. Refuses payloads larger than
// `_max`. This turns a raw stream into discrete messages; pair with read_frame.
template<byte_stream _Stream>
D_NODISCARD io_error
write_frame(
    _Stream&    _stream,
    const void* _data,
    std::size_t _size,
    std::size_t _max = default_max_frame
)
{
    // enforce the size ceiling and the 32-bit prefix range
    if ( (_size > _max) ||
         (_size > 0xFFFFFFFFu) )
    {
        return io_error::message_too_large;
    }

    const std::uint32_t length = static_cast<std::uint32_t>(_size);

    // big-endian length header
    byte header[4];
    header[0] = static_cast<byte>((length >> 24) & 0xFF);
    header[1] = static_cast<byte>((length >> 16) & 0xFF);
    header[2] = static_cast<byte>((length >> 8)  & 0xFF);
    header[3] = static_cast<byte>( length        & 0xFF);

    const io_error e = write_all(_stream, header, sizeof(header));

    if (e != io_error::none)
    {
        return e;
    }

    // a zero-length frame is legal and carries just the header
    if (_size == 0)
    {
        return io_error::none;
    }

    return write_all(_stream, _data, _size);
}

// write_frame (string)
//   function: writes a std::string as a single length-prefixed frame.
template<byte_stream _Stream>
D_NODISCARD io_error
write_frame(
    _Stream&           _stream,
    const std::string& _data,
    std::size_t        _max = default_max_frame
)
{
    return write_frame(_stream, _data.data(), _data.size(), _max);
}

// read_frame
//   function: reads one length-prefixed frame written by write_frame into
// `_out` (resized to the payload length). Rejects a declared length exceeding
// `_max` with message_too_large before allocating. Returns none on success,
// closed if the stream ends mid-frame, or the transport error otherwise.
template<byte_stream _Stream>
D_NODISCARD io_error
read_frame(
    _Stream&     _stream,
    byte_buffer& _out,
    std::size_t  _max = default_max_frame
)
{
    // read the 4-byte big-endian length header
    byte header[4];

    const io_error e = read_exactly(_stream, header, sizeof(header));

    if (e != io_error::none)
    {
        return e;
    }

    const std::size_t length =
        (static_cast<std::size_t>(header[0]) << 24) |
        (static_cast<std::size_t>(header[1]) << 16) |
        (static_cast<std::size_t>(header[2]) << 8)  |
        (static_cast<std::size_t>(header[3]));

    // guard the allocation against a hostile length
    if (length > _max)
    {
        return io_error::message_too_large;
    }

    // rebuild _out as the frame payload through the append protocol; a
    // hostile length was already rejected above
    _out.clear();

    // a zero-length frame yields an empty payload
    if (length == 0)
    {
        return io_error::none;
    }

    // read the payload in bounded chunks, appending each -- no single
    // length-sized scratch region is needed, and a short read leaves _out empty
    byte        chunk[16384];
    std::size_t remaining = length;

    while (remaining > 0)
    {
        const std::size_t want =
            (remaining < sizeof(chunk)) ? remaining : sizeof(chunk);

        const io_error frame_err = read_exactly(_stream, chunk, want);

        if (frame_err != io_error::none)
        {
            _out.clear();

            return frame_err;
        }

        _out.append(chunk, want);

        remaining -= want;
    }

    return io_error::none;
}


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_
