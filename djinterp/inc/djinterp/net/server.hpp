/*******************************************************************************
* djinterp [net]                                                      server.hpp
*
*   A small HTTP/1.1 server: it accepts connections from a transport acceptor,
* reads requests off each, invokes a request -> response handler, and writes
* the responses, looping for keep-alive. Templating the acceptor selects the
* wire: net::tcp_acceptor serves http; net::tls_acceptor<> (include
* tls/tls.hpp) serves https, the caller loading the certificate and key on
* its context. The vocabulary is http.hpp's.
*
* CONTENTS (namespace djinterp::net::http):
*   server_options   keep-alive, header/body caps, per-connection request cap,
*                    concurrent-connection cap, Server header
*   request_handler  re_std::function<response(const request&)>
*   internal:        a buffered line reader, request parsing (line, headers,
*                    Content-Length / chunked body), response serialisation
*   serve_connection drive the request/response (keep-alive) loop on one
*                    connection -- usable with any net::connection
*   server           binds/runs/stops, a thread of dmutex.h's per connection
*
*   HTTP/1.1 keep-alive is honoured (Connection negotiation, bounded reuse);
* the request body is read by Content-Length or chunked transfer-encoding.
* Blocking, one thread per connection for its lifetime, at most
* max_connections at once -- the reactor is the path to massive
* idle-connection counts.
*
*   PATCHED, NOT REBUILT. The owner's answer to the register's item 23 (C,
* 2026.10.05): patch the header to compile. It had been built on a generic
* net::server that was never written and on the HTTP reader retired with
* the old client, and it included itself; it now carries what it needs --
* the reader, the chunked decoder, an accept loop with a thread per
* connection -- and the execution-policy parameter, which only the missing
* server gave meaning to, is gone. Decision 4 still rebuilds both servers:
* a generic one at the net root, the HTTP one in net/http/ on the native
* parser.
*
*   Requires:  http.hpp, net_common.hpp, tcp.hpp; dmutex.h's threads (link
*              dmutex.c, net_common.c, net_common.cpp and tcp.c), and HTTP's
*              foundation (link http/http.c and
*              the codec's sources, under codec/). C++98 and later.
*
*
* path:      /inc/djinterp/net/server.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_NET_SERVER_HPP
#define DJINTERP_NET_SERVER_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::sprintf
#include <new>      // std::nothrow
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../c/sync/dmutex.h"     // d_thread_create, d_mutex_t, d_cond_t
#include "./net_common.h"  // d_net_ascii_lower
#include "./codec/codec_hex.h"    // d_net_hex_value
#include "./codec/codec_line.h"   // d_net_line_find
#include "./http/http.hpp"        // request, response, headers
#include "./net_common.hpp"       // connection, open_result, write_all
#include "./tcp/tcp.hpp"          // tcp_acceptor, the default transport
// re_std
#include "../../re_std/functional/function.hpp"  // re_std::function


NS_DJINTERP
NS_NET
D_NAMESPACE(http)


///////////////////////////////////////////////////////////////////////////////
///                          SERVER OPTIONS                                ///
///////////////////////////////////////////////////////////////////////////////

// server_options
//   struct: tunables for the HTTP server. Defaults: keep-alive on, a 64 KiB
// header cap, a 32 MiB body cap, up to 100 requests per kept-alive connection,
// at most 64 connections served at once, and an identifying Server header.
struct server_options
{
    bool        keep_alive;
    std::size_t max_header_bytes;
    std::size_t max_body_bytes;
    int         max_requests_per_connection;
    std::size_t max_connections;
    std::string server_name;

    server_options()
        : keep_alive(true),
          max_header_bytes(64u * 1024u),
          max_body_bytes(32u * 1024u * 1024u),
          max_requests_per_connection(100),
          max_connections(64u),
          server_name("djinterp-http/1.0")
    {
    }
};

// request_handler
//   type: an application handler mapping a parsed request to a response; an
// empty handler answers 404.
typedef re_std::function<response(const request&)> request_handler;


///////////////////////////////////////////////////////////////////////////////
///                        INTERNAL HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // request_read
    //   enum: the outcome of trying to read one request from a connection.
    struct request_read
    {
        enum value
        {
            ok,           // a complete request was parsed
            clean_eof,    // the peer closed at a request boundary
            too_large,    // a header or the body exceeded its cap
            bad_request   // malformed or truncated
        };
    };

    // icontains
    //   function: case-insensitive (ASCII) substring test.
    D_NODISCARD inline bool
    icontains(
        const std::string& _haystack,
        const std::string& _needle
    )
    {
        // nothing is in everything; something longer is in nothing
        if (_needle.empty())
        {
            return true;
        }

        if (_needle.size() > _haystack.size())
        {
            return false;
        }

        const std::size_t last = _haystack.size() - _needle.size();

        // each start position
        for (std::size_t i = 0; i <= last; ++i)
        {
            std::size_t j = 0;

            // the needle, matched character by character
            while ( (j < _needle.size()) &&
                    (::d_net_ascii_lower(_haystack[i + j]) ==
                     ::d_net_ascii_lower(_needle[j])) )
            {
                ++j;
            }

            // all of it matched
            if (j == _needle.size())
            {
                return true;
            }
        }

        return false;
    }

    // parse_size
    //   function: a non-negative size in the given base (10 or 16), the
    // whole of `_text` after trimming; false for anything else, overflow
    // included.
    D_NODISCARD inline bool
    parse_size(
        const std::string& _text,
        unsigned           _base,
        std::size_t&       _out
    )
    {
        const std::string digits = ::djinterp::net::http_trim_whitespace(
                                       _text);
        std::size_t       value  = 0;

        // no digits is no size
        if (digits.empty())
        {
            return false;
        }

        // each digit, checked for overflow before it is added
        for (std::size_t i = 0; i < digits.size(); ++i)
        {
            const int      hex   = ::d_net_hex_value(digits[i]);
            const unsigned digit = (hex >= 0) ? static_cast<unsigned>(hex)
                                              : 16u;

            // not a digit of this base, or a value past size_t
            if ( (digit >= _base) ||
                 (value > (static_cast<std::size_t>(-1) - digit) / _base) )
            {
                return false;
            }

            value = (value * _base) + digit;
        }

        _out = value;

        return true;
    }

    // decimal
    //   function: a number as text (std::to_string is C++11's).
    D_NODISCARD inline std::string
    decimal(
        unsigned long _number
    )
    {
        char      digits[32];
        const int length = std::sprintf(digits,
                                        "%lu",
                                        _number);

        return std::string(digits,
                           static_cast<std::size_t>(length));
    }

    // reader
    //   class: buffers reads from a net::connection so a request can be
    // parsed line by line and by count without over-reading. Bytes are held
    // in a std::string; the consumed prefix is reclaimed as it grows.
    class reader
    {
    public:
        explicit reader(
            net::connection& _connection
        )
            : m_connection(_connection),
              m_buffer(),
              m_pos(0u),
              m_eof(false)
        {
        }

        // read_line
        //   function: reads one CRLF- (or LF-) terminated line into `_out`,
        // without the terminator, framed by codec_line.h. closed at the end
        // of the stream before a terminator (`_out` holding what came),
        // message_too_large once the line passes `_max` characters (0 for no
        // cap), a CR that would end it counted, `_out` holding the first
        // `_max` + 1.
        D_NODISCARD net::io_error::value
        read_line(
            std::string& _out,
            std::size_t  _max
        )
        {
            std::size_t scanned = 0u;

            _out.clear();

            // until a line feed, refilling as needed
            for (;;)
            {
                const ::d_pack_text held  = { m_buffer.data() + m_pos,
                                              m_buffer.size() - m_pos };
                ::d_net_line        line  = { { D_NULLPTR, 0u }, 0u, false };
                const bool          found = ::d_net_line_find(held,
                                                              scanned,
                                                              &line);
                const std::size_t   taken =
                    (found) ? (line.text.length + ((line.crlf) ? 1u : 0u))
                            : held.length;

                // past the cap: as much of the line as reached it is taken
                if ( (_max != 0u) &&
                     (taken > _max) )
                {
                    return take(_out,
                                _max + 1u,
                                net::io_error::message_too_large);
                }

                // the line, its terminator consumed
                if (found)
                {
                    _out.assign(line.text.data,
                                line.text.length);
                    m_pos += line.size;
                    compact();

                    return net::io_error::none;
                }

                scanned = held.length;

                const net::io_error::value filled = fill();

                // no more bytes: what came of the line is taken
                if (filled != net::io_error::none)
                {
                    return take(_out,
                                held.length,
                                filled);
                }
            }
        }

        // read_exact
        //   function: appends exactly `_count` bytes to `_out`; closed on a
        // short read.
        D_NODISCARD net::io_error::value
        read_exact(
            std::string& _out,
            std::size_t  _count
        )
        {
            // until enough are buffered
            while ((m_buffer.size() - m_pos) < _count)
            {
                const net::io_error::value filled = fill();

                // the stream ended first
                if (filled != net::io_error::none)
                {
                    return filled;
                }
            }

            _out.append(m_buffer,
                        m_pos,
                        _count);
            m_pos += _count;
            compact();

            return net::io_error::none;
        }

    private:
        // take -- consumes `_count` buffered bytes into `_out`, returning
        // `_error`
        D_NODISCARD net::io_error::value
        take(
            std::string&         _out,
            std::size_t          _count,
            net::io_error::value _error
        )
        {
            _out.assign(m_buffer,
                        m_pos,
                        _count);
            m_pos += _count;

            return _error;
        }

        // fill -- reads another chunk; closed at the end of the stream
        D_NODISCARD net::io_error::value
        fill()
        {
            // an ended stream gives no more
            if (m_eof)
            {
                return net::io_error::closed;
            }

            char                 chunk[16384];
            const net::io_result got = m_connection.read(chunk,
                                                         sizeof(chunk));

            // a failed read
            if (got.error != net::io_error::none)
            {
                return got.error;
            }

            // nothing, with no error: the end of the stream
            if (got.count == 0u)
            {
                m_eof = true;

                return net::io_error::closed;
            }

            m_buffer.append(chunk,
                            got.count);

            return net::io_error::none;
        }

        // compact -- reclaims a consumed prefix past 64 KiB
        void
        compact()
        {
            // only a large consumed prefix is worth moving the rest for
            if (m_pos > (64u * 1024u))
            {
                m_buffer.erase(0u,
                               m_pos);
                m_pos = 0u;
            }

            return;
        }

        net::connection& m_connection;
        std::string      m_buffer;
        std::size_t      m_pos;
        bool             m_eof;
    };

    // skip_trailers
    //   function: after the last chunk, reads the trailers, which are
    // ignored, up to the blank line that ends them.
    D_NODISCARD inline net::io_error::value
    skip_trailers(
        reader& _reader
    )
    {
        std::string          trailer;
        net::io_error::value got = net::io_error::none;

        // each trailer, ignored
        do
        {
            got = _reader.read_line(trailer,
                                    65536u);

            // the trailers ended early
            if (got != net::io_error::none)
            {
                return got;
            }
        }
        while (!trailer.empty());

        return net::io_error::none;
    }

    // read_chunk_data
    //   function: one chunk's `_size` bytes, appended to `_body`, and the
    // empty line after them; message_too_large when they would take the body
    // past `_max` (0 for no cap), unknown when that line is not empty.
    D_NODISCARD inline net::io_error::value
    read_chunk_data(
        reader&      _reader,
        std::string& _body,
        std::size_t  _size,
        std::size_t  _max
    )
    {
        // past the cap
        if ( (_max != 0u) &&
             ( (_size > _max) ||
               (_body.size() > (_max - _size)) ) )
        {
            return net::io_error::message_too_large;
        }

        net::io_error::value got = _reader.read_exact(_body,
                                                      _size);

        // a short chunk
        if (got != net::io_error::none)
        {
            return got;
        }

        std::string ending;

        got = _reader.read_line(ending,
                                8u);

        // the CRLF after the data, which must be empty
        if (got != net::io_error::none)
        {
            return got;
        }

        return (ending.empty()) ? net::io_error::none
                                : net::io_error::unknown;
    }

    // read_chunked
    //   function: decodes a chunked transfer-encoded body into `_body`,
    // skipping chunk extensions and trailers; message_too_large past `_max`
    // bytes (0 for no cap), unknown for a malformed chunk header.
    D_NODISCARD inline net::io_error::value
    read_chunked(
        reader&      _reader,
        std::string& _body,
        std::size_t  _max
    )
    {
        // each chunk, until the last
        for (;;)
        {
            std::string          line;
            net::io_error::value got = _reader.read_line(line,
                                                         4096u);

            // no chunk header
            if (got != net::io_error::none)
            {
                return got;
            }

            const std::size_t semi = line.find(';');
            std::size_t       size = 0u;

            // a size that is not hex
            if (!parse_size(line.substr(0u,
                                        semi),
                            16u,
                            size))
            {
                return net::io_error::unknown;
            }

            // the last chunk: trailers up to the blank line
            if (size == 0u)
            {
                return skip_trailers(_reader);
            }

            got = read_chunk_data(_reader,
                                  _body,
                                  size,
                                  _max);

            // a chunk too large, short, or badly ended
            if (got != net::io_error::none)
            {
                return got;
            }
        }
    }

    // parse_request_line
    //   function: METHOD SP target SP version, into `_out`'s method and url;
    // whether the version is HTTP/1.1. False when the two spaces are not
    // there.
    D_NODISCARD inline bool
    parse_request_line(
        const std::string& _line,
        request&           _out,
        bool&              _http_1_1
    )
    {
        const std::size_t sp1 = _line.find(' ');
        const std::size_t sp2 = (sp1 == std::string::npos)
                                    ? std::string::npos
                                    : _line.find(' ',
                                                 sp1 + 1u);

        // no METHOD SP target SP version
        if (sp2 == std::string::npos)
        {
            return false;
        }

        net::http_method::value method = net::http_method::get;

        (void)method_from_string(_line.substr(0u,
                                              sp1),
                                 method);
        _out.method = method;
        _out.url    = _line.substr(sp1 + 1u,
                                   sp2 - sp1 - 1u);
        _http_1_1   = icontains(_line.substr(sp2 + 1u),
                                "HTTP/1.1");

        return true;
    }

    // read_headers
    //   function: each header line up to the blank one, into `_out`'s
    // headers; a line with no colon is skipped.
    D_NODISCARD inline request_read::value
    read_headers(
        reader&               _reader,
        const server_options& _options,
        request&              _out
    )
    {
        std::string line;

        // each header, to the blank line
        for (;;)
        {
            const net::io_error::value got = _reader.read_line(
                                                 line,
                                                 _options.max_header_bytes);

            // a header past the cap
            if (got == net::io_error::message_too_large)
            {
                return request_read::too_large;
            }

            // the stream ended mid-headers
            if (got != net::io_error::none)
            {
                return request_read::bad_request;
            }

            // the blank line ends the head
            if (line.empty())
            {
                return request_read::ok;
            }

            const std::size_t colon = line.find(':');

            // a line with no colon is skipped
            if (colon != std::string::npos)
            {
                const std::string name  =
                    ::djinterp::net::http_trim_whitespace(line.substr(0u,
                                                                      colon));
                const std::string value =
                    ::djinterp::net::http_trim_whitespace(
                        line.substr(colon + 1u));

                _out.headers.push_back(header_field(name,
                                                    value));
            }
        }
    }

    // parse_head
    //   function: the request line and the headers, into `_out`; whether
    // the client wants the connection kept alive.
    D_NODISCARD inline request_read::value
    parse_head(
        reader&               _reader,
        const server_options& _options,
        request&              _out,
        bool&                 _keep_alive
    )
    {
        std::string                line;
        bool                       http_1_1 = false;
        const net::io_error::value first    = _reader.read_line(
                                                  line,
                                                  _options.max_header_bytes);

        // the end of the stream, cleanly or mid-line
        if (first == net::io_error::closed)
        {
            return (line.empty()) ? request_read::clean_eof
                                  : request_read::bad_request;
        }

        // a request line past the cap
        if (first == net::io_error::message_too_large)
        {
            return request_read::too_large;
        }

        // a read failure, or no METHOD SP target SP version
        if ( (first != net::io_error::none) ||
             (!parse_request_line(line,
                                  _out,
                                  http_1_1)) )
        {
            return request_read::bad_request;
        }

        const request_read::value headers = read_headers(_reader,
                                                         _options,
                                                         _out);

        // a header past the cap, or a stream that ended mid-headers
        if (headers != request_read::ok)
        {
            return headers;
        }

        const std::string connection =
            header_value(_out.headers,
                         http_field::connection,
                         std::string());

        // HTTP/1.1 keeps alive unless told to close; 1.0 only when asked
        _keep_alive = (http_1_1) ? !icontains(connection,
                                             "close")
                                 : icontains(connection,
                                             "keep-alive");

        return request_read::ok;
    }

    // parse_body
    //   function: the body the head announces -- chunked, or by
    // Content-Length, or none -- into `_out`.
    D_NODISCARD inline request_read::value
    parse_body(
        reader&               _reader,
        const server_options& _options,
        request&              _out
    )
    {
        // a chunked body
        if (icontains(header_value(_out.headers,
                                   "Transfer-Encoding",
                                   std::string()),
                      "chunked"))
        {
            const net::io_error::value got =
                read_chunked(_reader,
                             _out.body,
                             _options.max_body_bytes);

            return (got == net::io_error::none)
                       ? request_read::ok
                       : (got == net::io_error::message_too_large)
                             ? request_read::too_large
                             : request_read::bad_request;
        }

        const header_field* const length_field =
            find_header(_out.headers,
                        http_field::content_length);

        // no length, no body
        if (!length_field)
        {
            return request_read::ok;
        }

        std::size_t length = 0u;

        // a length that is no number
        if (!parse_size(length_field->second,
                        10u,
                        length))
        {
            return request_read::bad_request;
        }

        // a body past the cap
        if ( (_options.max_body_bytes != 0u) &&
             (length > _options.max_body_bytes) )
        {
            return request_read::too_large;
        }

        return (_reader.read_exact(_out.body,
                                   length) == net::io_error::none)
                   ? request_read::ok
                   : request_read::bad_request;
    }

    // read_request
    //   function: a request's head, then the body it announces, into
    // `_out`; whether the client wants the connection kept alive.
    D_NODISCARD inline request_read::value
    read_request(
        reader&               _reader,
        const server_options& _options,
        request&              _out,
        bool&                 _keep_alive
    )
    {
        const request_read::value head = parse_head(_reader,
                                                    _options,
                                                    _out,
                                                    _keep_alive);

        // a head that did not parse announces no body
        if (head != request_read::ok)
        {
            return head;
        }

        return parse_body(_reader,
                          _options,
                          _out);
    }

    // build_response_wire
    //   function: serialises a response, supplying Server, Content-Length
    // (when not chunked), and the negotiated Connection when the handler has
    // not. `_head` omits the body for a HEAD request.
    D_NODISCARD inline std::string
    build_response_wire(
        const response&       _response,
        const server_options& _options,
        bool                  _keep_alive,
        bool                  _head
    )
    {
        const int   code = (_response.status != 0) ? _response.status
                                                   : 200;
        std::string out;

        out.reserve(128u + _response.body.size());
        out += "HTTP/1.1 ";
        out += decimal(static_cast<unsigned long>(code));
        out += " ";
        out += reason_phrase(code);
        out += "\r\n";

        const bool chunked = icontains(header_value(_response.headers,
                                                    "Transfer-Encoding",
                                                    std::string()),
                                       "chunked");

        // the server's name, unless the handler gave one
        if ( (!_options.server_name.empty()) &&
             (!has_header(_response.headers,
                          "Server")) )
        {
            out += "Server: " + _options.server_name + "\r\n";
        }

        // the length, unless chunked or given
        if ( (!chunked) &&
             (!has_header(_response.headers,
                          http_field::content_length)) )
        {
            out += std::string(http_field::content_length) + ": " +
                   decimal(static_cast<unsigned long>(
                       _response.body.size())) + "\r\n";
        }

        // the connection's fate, unless given
        if (!has_header(_response.headers,
                        http_field::connection))
        {
            out += std::string(http_field::connection) +
                   ((_keep_alive) ? ": keep-alive\r\n"
                                  : ": close\r\n");
        }

        // the handler's own fields
        for (std::size_t i = 0u; i < _response.headers.size(); ++i)
        {
            out += _response.headers[i].first + ": " +
                   _response.headers[i].second + "\r\n";
        }

        out += "\r\n";

        // no body for HEAD
        if (!_head)
        {
            out += _response.body;
        }

        return out;
    }

    // write_status
    //   function: writes a status-only response (protocol errors), closing.
    inline void
    write_status(
        net::connection&      _connection,
        int                   _code,
        const server_options& _options
    )
    {
        response status;

        status.status = _code;
        (void)net::write_all(_connection,
                             build_response_wire(status,
                                                 _options,
                                                 false,
                                                 false));

        return;
    }

    // handle
    //   function: the handler's answer to a request; with no handler, nothing
    // is found.
    D_NODISCARD inline response
    handle(
        const request_handler& _handler,
        const request&         _incoming
    )
    {
        response answer;

        // the handler answers; with none, nothing is found
        if (!_handler.empty())
        {
            answer = _handler(_incoming);
        }
        else
        {
            answer.status = 404;
        }

        return answer;
    }

    // keeps_alive
    //   function: whether a connection stays open after a response: the
    // server allows it, the client asked for it, and the request cap leaves
    // room for another.
    D_NODISCARD inline bool
    keeps_alive(
        const server_options& _options,
        bool                  _wants_keep,
        int                   _served
    )
    {
        return ( (_options.keep_alive) &&
                 (_wants_keep) &&
                 ( (_options.max_requests_per_connection <= 0) ||
                   ((_served + 1) < _options.max_requests_per_connection) ) );
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                        SERVE CONNECTION                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // serve_requests
    //   function: serve_connection's loop, leaving the connection open for
    // its caller to close.
    inline void
    serve_requests(
        net::connection&       _connection,
        const request_handler& _handler,
        const server_options&  _options
    );

NS_END  // internal

// serve_connection
//   function: runs the HTTP request/response loop on one connection. It parses
// a request, invokes `_handler`, writes the response, and repeats while
// keep-alive is in effect (bounded by the request cap), then closes. Usable
// directly with any net::connection (one from a custom accept path, say).
inline void
serve_connection(
    net::connection&       _connection,
    const request_handler& _handler,
    const server_options&  _options
)
{
    internal::serve_requests(_connection,
                             _handler,
                             _options);
    _connection.close();

    return;
}

/*
internal::serve_requests
  One request at a time: the head, then the body it announces, the handler,
and the response, until the client leaves, a request fails, or keep-alive
ends.
*/
inline void
internal::serve_requests(
    net::connection&       _connection,
    const request_handler& _handler,
    const server_options&  _options
)
{
    internal::reader reader(_connection);
    int              served = 0;

    // requests, while the connection is kept
    for (;;)
    {
        request                             incoming;
        bool                                wants_keep = false;
        const internal::request_read::value outcome =
            internal::read_request(reader,
                                   _options,
                                   incoming,
                                   wants_keep);

        // the client left between requests
        if (outcome == internal::request_read::clean_eof)
        {
            break;
        }

        // too much, or malformed: a status, then the connection closes
        if (outcome != internal::request_read::ok)
        {
            internal::write_status(_connection,
                                   (outcome ==
                                    internal::request_read::too_large)
                                       ? 413
                                       : 400,
                                   _options);

            break;
        }

        const response answer = internal::handle(_handler,
                                                 incoming);
        const bool     keep   = internal::keeps_alive(_options,
                                                      wants_keep,
                                                      served);

        // a write that fails ends the connection
        if (net::write_all(_connection,
                           internal::build_response_wire(
                               answer,
                               _options,
                               keep,
                               incoming.method == net::http_method::head)) !=
            net::io_error::none)
        {
            break;
        }

        ++served;

        // a connection not kept ends here
        if (!keep)
        {
            break;
        }
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                             SERVER                                     ///
///////////////////////////////////////////////////////////////////////////////

// server
//   class: an HTTP/1.1 server over a transport acceptor -- net::tcp_acceptor
// for http, net::tls_acceptor<> for https (load the certificate and key via
// acceptor().context()). bind(), then run(handler), which blocks serving,
// a thread per connection and at most max_connections at once, until
// stop(); stop() is thread-safe, and run() returns once every connection
// has finished. The server must outlive its run(). Non-copyable.
template<typename Acceptor = net::tcp_acceptor>
class server
{
public:
    server()
        : m_acceptor(),
          m_options(),
          m_handler(),
          m_sessions(),
          m_stopping(false)
    {
        (void)::d_mutex_init(&m_lock);
        (void)::d_cond_init(&m_finished);
    }

    ~server()
    {
        (void)::d_cond_destroy(&m_finished);
        (void)::d_mutex_destroy(&m_lock);
    }

    // options
    //   function: mutable access to the server options; set before run().
    D_NODISCARD server_options&
    options()
    {
        return m_options;
    }

    // acceptor
    //   function: the transport acceptor (to load a TLS certificate and key).
    D_NODISCARD Acceptor&
    acceptor()
    {
        return m_acceptor;
    }

    // bind
    //   function: binds the transport acceptor.
    D_NODISCARD net::io_error::value
    bind(
        const net::endpoint& _endpoint
    )
    {
        return m_acceptor.bind(_endpoint);
    }

    // local_endpoint
    //   function: the bound address (resolves an ephemeral port).
    D_NODISCARD net::endpoint
    local_endpoint() const
    {
        return m_acceptor.local_endpoint();
    }

    // run
    //   function: serves HTTP with `_handler` until stop(), then waits for
    // the connections still being served. Returns none after a stop, or the
    // accept error that ended the loop otherwise.
    D_NODISCARD net::io_error::value
    run(
        const request_handler& _handler
    )
    {
        net::io_error::value result = net::io_error::none;

        m_handler = _handler;

        // accept until stopped, or the acceptor fails for good
        while (!stopping())
        {
            net::open_result           accepted;
            const net::io_error::value got = m_acceptor.accept(accepted);

            // a failed accept: the end, unless only one connection failed
            if (got != net::io_error::none)
            {
                // a connection the peer gave up on: carry on
                if ( (got == net::io_error::connection_aborted) ||
                     (got == net::io_error::interrupted) ||
                     (got == net::io_error::out_of_memory) )
                {
                    continue;
                }

                result = (stopping()) ? net::io_error::none
                                      : got;

                break;
            }

            start_session(accepted.release());
        }

        finish_sessions();

        return result;
    }

    // stop
    //   function: stops serving: run() accepts nothing more, every
    // connection's reading side is shut, so an idle keep-alive connection
    // ends at once and a busy one after its current response, and run()
    // returns once they have. Safe from any thread, including a handler's.
    // Shutting the reading side is safe beside a read in progress, for TCP
    // and for TLS alike (TLS passes it to its transport), where closing
    // would not be.
    void
    stop()
    {
        (void)::d_mutex_lock(&m_lock);
        m_stopping = true;

        // each live connection's reads end
        for (std::size_t i = 0u; i < m_sessions.size(); ++i)
        {
            // a finished session's connection is closed already
            if (!m_sessions[i]->done)
            {
                (void)m_sessions[i]->connection->shutdown(
                    net::shutdown_mode::read);
            }
        }

        (void)::d_mutex_unlock(&m_lock);
        m_acceptor.close();

        return;
    }

private:
    D_DELETED_FN(server(const server&))
    D_DELETED_FN(server& operator=(const server&))

    // session
    //   struct: one connection being served on a thread of its own. The
    // connection is closed under the server's lock, so stop() never touches
    // it closed, and deleted by run()'s thread once the session is joined.
    struct session
    {
        server*          owner;
        net::connection* connection;  // owned
        d_thread_t       thread;
        bool             done;        // under the owner's lock
    };

    // session_main
    //   function: a connection's thread: serves it, closes it, and says so.
    static d_thread_result_t
    session_main(
        void* _session
    )
    {
        session* const work  = static_cast<session*>(_session);
        server* const  owner = work->owner;

        internal::serve_requests(*work->connection,
                                 owner->m_handler,
                                 owner->m_options);

        (void)::d_mutex_lock(&owner->m_lock);
        work->connection->close();
        work->done = true;
        (void)::d_cond_broadcast(&owner->m_finished);
        (void)::d_mutex_unlock(&owner->m_lock);

        return D_THREAD_SUCCESS;
    }

    // stopping -- whether stop() was called
    bool
    stopping()
    {
        (void)::d_mutex_lock(&m_lock);

        const bool stop = m_stopping;

        (void)::d_mutex_unlock(&m_lock);

        return stop;
    }

    // start_session -- a thread for the connection, once there is room
    void
    start_session(
        net::connection* _connection
    )
    {
        reap(m_options.max_connections);

        session* const work = new (std::nothrow) session();

        // no memory for the session: the connection is dropped
        if (!work)
        {
            delete _connection;

            return;
        }

        work->owner      = this;
        work->connection = _connection;
        work->done       = false;

        // the session is listed before its thread starts, so stop() can
        // reach it at once
        (void)::d_mutex_lock(&m_lock);
        m_sessions.push_back(work);

        const bool late = m_stopping;

        // a stop that came while it was accepted shuts it at once
        if (late)
        {
            (void)_connection->shutdown(net::shutdown_mode::read);
        }

        (void)::d_mutex_unlock(&m_lock);

        // a thread that cannot start serves nobody
        if (::d_thread_create(&work->thread,
                              &session_main,
                              work) != 0)
        {
            (void)::d_mutex_lock(&m_lock);
            m_sessions.pop_back();
            (void)::d_mutex_unlock(&m_lock);
            delete _connection;
            delete work;
        }

        return;
    }

    // reap -- joins finished sessions, waiting while more than `_room`
    // (0 for no cap) are still running
    void
    reap(
        std::size_t _room
    )
    {
        (void)::d_mutex_lock(&m_lock);

        // until fewer than the cap are running
        for (;;)
        {
            std::vector<session*> running;

            // the finished are joined, the rest kept
            for (std::size_t i = 0u; i < m_sessions.size(); ++i)
            {
                session* const work = m_sessions[i];

                // a finished session's thread ends at once
                if (work->done)
                {
                    (void)::d_thread_join(work->thread,
                                          D_NULLPTR);
                    delete work->connection;
                    delete work;
                }
                else
                {
                    running.push_back(work);
                }
            }

            m_sessions.swap(running);

            // room enough, or no cap
            if ( (_room == 0u) ||
                 (m_sessions.size() < _room) )
            {
                break;
            }

            (void)::d_cond_wait(&m_finished,
                                &m_lock);
        }

        (void)::d_mutex_unlock(&m_lock);

        return;
    }

    // finish_sessions -- waits for every session, and joins them: room for
    // fewer than one is room for none
    void
    finish_sessions()
    {
        reap(1u);

        return;
    }

    Acceptor              m_acceptor;
    server_options        m_options;
    request_handler       m_handler;
    std::vector<session*> m_sessions;  // under m_lock
    bool                  m_stopping;  // under m_lock
    d_mutex_t             m_lock;
    d_cond_t              m_finished;
};


NS_END  // http
NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_SERVER_HPP
