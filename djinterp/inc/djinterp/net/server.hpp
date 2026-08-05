/******************************************************************************
* djinterp [web]                                                http/server.hpp
*
*   A native HTTP/1.1 server, the counterpart to http/http.hpp's client. It
* runs on the net subframework's generic server: net::server accepts and
* dispatches connections, and this layer parses an HTTP request off each
* connection, invokes a request -> response handler, and writes the response --
* looping for keep-alive. Templating the transport acceptor selects the wire:
* net::tcp_acceptor serves http, net::tls_acceptor<> serves https (the caller
* loads the certificate and key on the acceptor's context). It reuses the HTTP
* vocabulary in web.hpp and the buffered reader and body decoders in http.hpp.
*
* CONTENTS (namespace djinterp::web::http):
*   server_options   keep-alive, header/body caps, per-connection request cap,
*                    Server header
*   request_handler  std::function<response(const request&)>
*   internal:        request parsing (line, headers, Content-Length / chunked
*                    body) and response serialisation
*   serve_connection drive the request/response (keep-alive) loop on one
*                    connection -- usable with any net::connection
*   server           binds/runs/stops over net::server, templated on the
*                    transport acceptor (http or https) and execution policy
*
*   HTTP/1.1 keep-alive is honoured (Connection negotiation, bounded reuse);
* the request body is read by Content-Length or chunked transfer-encoding.
* Blocking, one connection per worker for its lifetime -- the reactor is the
* path to massive idle-connection counts.
*
*   Requires:  http.hpp (client-side reader/decoders + web vocabulary),
*              net/server.hpp (+ net/tcp.hpp, net/tls.hpp via http.hpp).
*              Link (for https): -lssl -lcrypto.
*
* path:      /inc/djinterp/web/http/server.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_WEB_HTTP_SERVER_
#define DJINTERP_WEB_HTTP_SERVER_ 1

// djinterp
#include "./http.hpp"
#include "../../net/server.hpp"


// std
#include <functional>
#include <string>
#include <utility>


NS_DJINTERP
NS_WEB
D_NAMESPACE(http)


///////////////////////////////////////////////////////////////////////////////
///                          SERVER OPTIONS                                ///
///////////////////////////////////////////////////////////////////////////////

// server_options
//   struct: tunables for the HTTP server. Defaults: keep-alive on, a 64 KiB
// header cap, a 32 MiB body cap, up to 100 requests per kept-alive connection,
// and an identifying Server header.
struct server_options
{
    bool        keep_alive;
    std::size_t max_header_bytes;
    std::size_t max_body_bytes;
    int         max_requests_per_connection;
    std::string server_name;

    server_options()
        : keep_alive(true),
          max_header_bytes(64u * 1024u),
          max_body_bytes(32u * 1024u * 1024u),
          max_requests_per_connection(100),
          server_name("djinterp-http/1.0")
    {
    }
};

// request_handler
//   type: an application handler mapping a parsed request to a response.
using request_handler = std::function<response(const request&)>;


///////////////////////////////////////////////////////////////////////////////
///                        INTERNAL HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // request_read
    //   enum: the outcome of trying to read one request from a connection.
    enum class request_read
    {
        ok,           // a complete request was parsed
        clean_eof,    // the peer closed at a request boundary (no partial data)
        too_large,    // header or body exceeded the configured cap
        bad_request   // malformed or truncated request
    };

    // parse_request
    //   function: reads and parses one request (request line, headers, and body
    // by Content-Length or chunked) into `_out`, and reports whether the client
    // wants the connection kept alive.
    D_NODISCARD inline request_read
    parse_request(
        reader&               _reader,
        const server_options& _options,
        request&              _out,
        bool&                 _keep_alive
    )
    {
        std::string line;

        // ---- request line: METHOD SP target SP HTTP/x.y ----
        net::io_error e = _reader.read_line(line, _options.max_header_bytes);

        if (e == net::io_error::closed)
        {
            // empty at EOF is an orderly end; partial is a truncated request
            return line.empty() ? request_read::clean_eof
                                 : request_read::bad_request;
        }

        if (e == net::io_error::message_too_large)
        {
            return request_read::too_large;
        }

        if (e != net::io_error::none)
        {
            return request_read::bad_request;
        }

        const std::size_t sp1 = line.find(' ');

        if (sp1 == std::string::npos)
        {
            return request_read::bad_request;
        }

        const std::size_t sp2 = line.find(' ', sp1 + 1);

        if (sp2 == std::string::npos)
        {
            return request_read::bad_request;
        }

        {
            http_method parsed_method = http_method::get;
            (void)method_from_string(line.substr(0, sp1), parsed_method);
            _out.method = parsed_method;
        }

        _out.url = line.substr(sp1 + 1, sp2 - sp1 - 1);

        const bool http_1_1 = icontains(line.substr(sp2 + 1), "HTTP/1.1");

        // ---- headers ----
        for (;;)
        {
            e = _reader.read_line(line, _options.max_header_bytes);

            if (e == net::io_error::message_too_large)
            {
                return request_read::too_large;
            }

            if (e != net::io_error::none)
            {
                return request_read::bad_request;   // EOF mid-headers = truncated
            }

            if (line.empty())
            {
                break;
            }

            const std::size_t colon = line.find(':');

            if (colon == std::string::npos)
            {
                continue;
            }

            _out.headers.push_back(
                header_field(trim(line.substr(0, colon)),
                             trim(line.substr(colon + 1))));
        }

        // ---- keep-alive negotiation (client's wish) ----
        const std::string conn_header =
            header_value(_out.headers, header_name::connection, std::string());

        if (http_1_1)
        {
            _keep_alive = !icontains(conn_header, "close");
        }
        else
        {
            _keep_alive = icontains(conn_header, "keep-alive");
        }

        // ---- body: chunked or Content-Length (else none) ----
        const std::string te = header_value(_out.headers,
                                            "Transfer-Encoding",
                                            std::string());

        if (icontains(te, "chunked"))
        {
            e = read_chunked(_reader, _out.body, _options.max_body_bytes);

            if (e == net::io_error::message_too_large)
            {
                return request_read::too_large;
            }

            if (e != net::io_error::none)
            {
                return request_read::bad_request;
            }

            return request_read::ok;
        }

        const header_field* cl =
            find_header(_out.headers, header_name::content_length);

        if (cl != nullptr)
        {
            std::size_t length = 0;

            if (!parse_dec_size(cl->second, length))
            {
                return request_read::bad_request;
            }

            if ( (_options.max_body_bytes != 0) &&
                 (length > _options.max_body_bytes) )
            {
                return request_read::too_large;
            }

            e = _reader.read_exact(_out.body, length);

            if (e != net::io_error::none)
            {
                return request_read::bad_request;   // truncated body
            }
        }

        return request_read::ok;
    }

    // build_response_wire
    //   function: serialises a response, supplying Server, Content-Length (when
    // not chunked), and the negotiated Connection when the handler has not.
    // `_head` omits the body for a HEAD request.
    D_NODISCARD inline std::string
    build_response_wire(
        const response&       _resp,
        const server_options& _options,
        bool                  _keep_alive,
        bool                  _head
    )
    {
        const int code = (_resp.status != 0) ? _resp.status : 200;

        std::string out;
        out.reserve(128 + _resp.body.size());

        out += "HTTP/1.1 ";
        out += std::to_string(code);
        out += " ";
        out += reason_phrase(code);
        out += "\r\n";

        const bool chunked =
            icontains(header_value(_resp.headers, "Transfer-Encoding", std::string()),
                      "chunked");

        if ( (!_options.server_name.empty()) &&
             (!has_header(_resp.headers, "Server")) )
        {
            out += "Server: ";
            out += _options.server_name;
            out += "\r\n";
        }

        if ( (!chunked) &&
             (!has_header(_resp.headers, header_name::content_length)) )
        {
            out += header_name::content_length;
            out += ": ";
            out += std::to_string(_resp.body.size());
            out += "\r\n";
        }

        if (!has_header(_resp.headers, header_name::connection))
        {
            out += header_name::connection;
            out += _keep_alive ? ": keep-alive\r\n" : ": close\r\n";
        }

        for (const header_field& field : _resp.headers)
        {
            out += field.first;
            out += ": ";
            out += field.second;
            out += "\r\n";
        }

        out += "\r\n";

        if (!_head)
        {
            out += _resp.body;
        }

        return out;
    }

    // write_status
    //   function: writes a minimal status-only response (used for protocol
    // errors), always closing the connection.
    inline void
    write_status(
        net::connection&      _conn,
        int                   _code,
        const server_options& _options
    )
    {
        response resp;
        resp.status = _code;

        const std::string wire =
            build_response_wire(resp, _options, false, false);

        (void)net::write_all(_conn, wire);

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                        SERVE CONNECTION                                ///
///////////////////////////////////////////////////////////////////////////////

// serve_connection
//   function: runs the HTTP request/response loop on one connection. It parses a
// request, invokes `_handler`, writes the response, and repeats while keep-alive
// is in effect (bounded by the request cap), then closes. Usable directly with
// any net::connection (e.g. one obtained from a custom accept path).
inline void
serve_connection(
    net::connection&      _conn,
    const request_handler& _handler,
    const server_options&  _options
)
{
    internal::reader reader(_conn);
    int              served = 0;

    for (;;)
    {
        request                  req;
        bool                     client_keep_alive = false;
        const internal::request_read outcome =
            internal::parse_request(reader, _options, req, client_keep_alive);

        if (outcome == internal::request_read::clean_eof)
        {
            break;
        }

        if (outcome == internal::request_read::too_large)
        {
            internal::write_status(_conn, 413, _options);
            break;
        }

        if (outcome == internal::request_read::bad_request)
        {
            internal::write_status(_conn, 400, _options);
            break;
        }

        const bool head = (req.method == http_method::head);

        response resp;

        if (_handler)
        {
            resp = _handler(req);
        }
        else
        {
            resp.status = 404;
        }

        const bool keep =
            ( _options.keep_alive &&
              client_keep_alive &&
              ( (_options.max_requests_per_connection <= 0) ||
                ((served + 1) < _options.max_requests_per_connection) ) );

        const std::string wire =
            internal::build_response_wire(resp, _options, keep, head);

        if (net::write_all(_conn, wire) != net::io_error::none)
        {
            break;
        }

        ++served;

        if (!keep)
        {
            break;
        }
    }

    _conn.close();

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                             SERVER                                     ///
///////////////////////////////////////////////////////////////////////////////

// server
//   class: an HTTP/1.1 server over net::server. Templated on the transport
// acceptor -- net::tcp_acceptor for http, net::tls_acceptor<> for https (load
// the certificate/key via acceptor().context()) -- and the execution policy
// (default the bounded thread pool). bind() then run(handler) blocks serving
// until stop(); stop() is thread-safe. Non-copyable.
template <typename _Acceptor  = net::tcp_acceptor,
          typename _Execution = net::thread_pool_execution>
class server
{
public:
    server()
        : m_server(_Acceptor()),
          m_options(),
          m_handler()
    {
    }

    template <typename... _ExecutionArgs>
    explicit server(
        _Acceptor          _acceptor,
        _ExecutionArgs&&... _execution_args
    )
        : m_server(std::move(_acceptor),
                   std::forward<_ExecutionArgs>(_execution_args)...),
          m_options(),
          m_handler()
    {
    }

    server(const server&)            D_DELETE;
    server& operator=(const server&) D_DELETE;

    // options
    //   function: mutable access to the server options.
    D_NODISCARD server_options&
    options()
    {
        return m_options;
    }

    // acceptor / execution
    //   function: access to the underlying transport acceptor (e.g. to load a
    // TLS certificate and key) and execution policy.
    D_NODISCARD _Acceptor&
    acceptor()
    {
        return m_server.acceptor();
    }

    D_NODISCARD _Execution&
    execution()
    {
        return m_server.execution();
    }

    // bind
    //   function: binds the transport acceptor.
    D_NODISCARD net::io_error
    bind(
        const net::endpoint& _endpoint
    )
    {
        return m_server.bind(_endpoint);
    }

    // run
    //   function: serves HTTP with `_handler` until stop().
    D_NODISCARD net::io_error
    run(
        request_handler _handler
    )
    {
        m_handler = std::move(_handler);

        return m_server.run(
            [this](net::connection& _conn)
            {
                serve_connection(_conn, m_handler, m_options);
            });
    }

    // stop
    //   function: stops serving (thread-safe).
    void
    stop()
    {
        m_server.stop();

        return;
    }

private:
    net::server<_Acceptor, _Execution> m_server;
    server_options                     m_options;
    request_handler                    m_handler;
};


NS_END  // http
NS_END  // web
NS_END  // djinterp


#endif  // DJINTERP_WEB_HTTP_SERVER_
