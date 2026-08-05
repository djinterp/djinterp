/******************************************************************************
* djinterp [web]                                                  http/http.hpp
*
*   A native HTTP/1.1 client built on the net subframework's transport rather
* than on an external library. Where web/curl/curl.hpp delegates the protocol
* to libcurl, this implements HTTP/1.1 directly -- serialising a web::request to
* the wire and parsing a web::response back -- over net::connection, using
* net::tcp_connector for http and net::tls_connector for https. It reuses the
* HTTP vocabulary in web.hpp (methods, status, headers, url_parts) so it is an
* alternative backend, not a parallel type system.
*
* CONTENTS (namespace djinterp::web::http):
*   client_options   verification, connect timeout, response cap, redirect cap,
*                    user agent
*   internal:        transport selection, request serialisation, a buffered
*                    response reader, and response parsing (status line, headers,
*                    and body framing: Content-Length, chunked, close-delimited)
*   client           send(request) with bounded redirect following, plus
*                    get / head / post convenience
*   get / fetch      free convenience wrappers over a default client
*
*   Blocking, one request per connection (Connection: close); it layers on the
* blocking transport and its verification/timeouts. Response body framing covers
* the three HTTP/1.1 cases; redirects (301/302/303/307/308) are followed up to a
* cap with the usual method rewriting.
*
*   Requires:  web.hpp (HTTP vocabulary), net/tcp.hpp + net/tls.hpp (transport).
*              Link (for https): -lssl -lcrypto.
*
* path:      /inc/djinterp/web/http/http.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_WEB_HTTP_
#define DJINTERP_WEB_HTTP_ 1

// djinterp
#include "../web.hpp"
#include "../../net/tcp.hpp"
#include "../../net/tls.hpp"


// std
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>


NS_DJINTERP
NS_WEB
D_NAMESPACE(http)


///////////////////////////////////////////////////////////////////////////////
///                          CLIENT OPTIONS                                ///
///////////////////////////////////////////////////////////////////////////////

// client_options
//   struct: tunables for the HTTP client. Defaults are safe: TLS verification
// on, a blocking connect (no timeout), a 32 MiB response cap, up to 5 redirects,
// and an identifying user agent.
struct client_options
{
    bool        verify_tls;
    long        connect_timeout_ms;
    std::size_t max_response_bytes;
    int         max_redirects;
    std::string user_agent;

    client_options()
        : verify_tls(true),
          connect_timeout_ms(0),
          max_response_bytes(32u * 1024u * 1024u),
          max_redirects(5),
          user_agent("djinterp-http/1.0")
    {
    }
};


///////////////////////////////////////////////////////////////////////////////
///                        INTERNAL HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // map_connect_error
    //   function: maps a transport io_error from connect() onto transport_error,
    // distinguishing a TLS handshake/verification failure.
    D_NODISCARD inline transport_error
    map_connect_error(
        net::io_error _e,
        bool          _tls
    )
    {
        switch (_e)
        {
            case net::io_error::connection_refused:
                return transport_error::could_not_connect;

            case net::io_error::timed_out:
                return transport_error::timed_out;

            case net::io_error::host_unreachable:
            case net::io_error::address_invalid:
                return transport_error::could_not_resolve_host;

            case net::io_error::network_down:
                return transport_error::could_not_connect;

            case net::io_error::access_denied:
                return _tls ? transport_error::tls_error
                            : transport_error::could_not_connect;

            case net::io_error::out_of_memory:
                return transport_error::out_of_memory;

            default:
                return _tls ? transport_error::tls_error
                            : transport_error::could_not_connect;
        }
    }

    // trim
    //   function: strips leading/trailing spaces and tabs.
    D_NODISCARD inline std::string
    trim(
        const std::string& _s
    )
    {
        std::size_t begin = 0;
        std::size_t end   = _s.size();

        while ( (begin < end) &&
                ((_s[begin] == ' ') || (_s[begin] == '\t')) )
        {
            ++begin;
        }

        while ( (end > begin) &&
                ((_s[end - 1] == ' ') || (_s[end - 1] == '\t')) )
        {
            --end;
        }

        return _s.substr(begin, end - begin);
    }

    // icontains
    //   function: case-insensitive substring test (ASCII).
    D_NODISCARD inline bool
    icontains(
        const std::string& _haystack,
        const std::string& _needle
    )
    {
        if (_needle.empty())
        {
            return true;
        }

        if (_needle.size() > _haystack.size())
        {
            return false;
        }

        const std::size_t last = _haystack.size() - _needle.size();

        for (std::size_t i = 0; i <= last; ++i)
        {
            std::size_t j = 0;

            while ( (j < _needle.size()) &&
                    (ascii_to_lower(_haystack[i + j]) == ascii_to_lower(_needle[j])) )
            {
                ++j;
            }

            if (j == _needle.size())
            {
                return true;
            }
        }

        return false;
    }

    // is_redirect
    //   function: whether a status code is a redirect this client will follow.
    D_NODISCARD inline bool
    is_redirect(
        int _status
    )
    {
        return ( (_status == 301) ||
                 (_status == 302) ||
                 (_status == 303) ||
                 (_status == 307) ||
                 (_status == 308) );
    }

    // parse_hex_size / parse_dec_size
    //   function: parse a chunk size (hex) or a Content-Length (decimal).
    D_NODISCARD inline bool
    parse_hex_size(
        const std::string& _s,
        std::size_t&       _out
    )
    {
        errno = 0;
        char*                    end = nullptr;
        const unsigned long long v   = std::strtoull(_s.c_str(), &end, 16);

        if (end == _s.c_str())
        {
            return false;
        }

        _out = static_cast<std::size_t>(v);
        return true;
    }

    D_NODISCARD inline bool
    parse_dec_size(
        const std::string& _s,
        std::size_t&       _out
    )
    {
        const std::string t = trim(_s);

        errno = 0;
        char*                    end = nullptr;
        const unsigned long long v   = std::strtoull(t.c_str(), &end, 10);

        if (end == t.c_str())
        {
            return false;
        }

        _out = static_cast<std::size_t>(v);
        return true;
    }

    // reader
    //   class: buffers reads from a net::connection so a response can be parsed
    // line by line and by count without over-reading. Bytes are held in a
    // std::string; the consumed prefix is reclaimed as it grows.
    class reader
    {
    public:
        explicit reader(
            net::connection& _conn
        )
            : m_conn(_conn),
              m_buffer(),
              m_pos(0),
              m_eof(false)
        {
        }

        // read_line
        //   function: reads one CRLF- (or LF-) terminated line, returning it
        // without the terminator. Returns closed at EOF before a terminator.
        D_NODISCARD net::io_error
        read_line(
            std::string& _out,
            std::size_t  _max
        )
        {
            _out.clear();

            for (;;)
            {
                while (m_pos < m_buffer.size())
                {
                    const char ch = m_buffer[m_pos];
                    ++m_pos;

                    if (ch == '\n')
                    {
                        if ( (!_out.empty()) &&
                             (_out.back() == '\r') )
                        {
                            _out.pop_back();
                        }

                        compact();
                        return net::io_error::none;
                    }

                    _out.push_back(ch);

                    if ( (_max != 0) &&
                         (_out.size() > _max) )
                    {
                        return net::io_error::message_too_large;
                    }
                }

                const net::io_error e = fill();

                if (e != net::io_error::none)
                {
                    return e;
                }
            }
        }

        // read_exact
        //   function: appends exactly `_n` bytes to `_out`. Returns closed on a
        // short read (truncation).
        D_NODISCARD net::io_error
        read_exact(
            std::string& _out,
            std::size_t  _n
        )
        {
            while (available() < _n)
            {
                const net::io_error e = fill();

                if (e != net::io_error::none)
                {
                    return e;
                }
            }

            _out.append(m_buffer, m_pos, _n);
            m_pos += _n;

            compact();
            return net::io_error::none;
        }

        // read_to_eof
        //   function: appends all remaining bytes until the peer closes.
        D_NODISCARD net::io_error
        read_to_eof(
            std::string& _out,
            std::size_t  _max
        )
        {
            for (;;)
            {
                if (available() > 0)
                {
                    _out.append(m_buffer, m_pos, m_buffer.size() - m_pos);
                    m_pos = m_buffer.size();

                    if ( (_max != 0) &&
                         (_out.size() > _max) )
                    {
                        return net::io_error::message_too_large;
                    }
                }

                const net::io_error e = fill();

                if (e == net::io_error::closed)
                {
                    return net::io_error::none;   // orderly end of body
                }

                if (e != net::io_error::none)
                {
                    return e;
                }
            }
        }

    private:
        std::size_t
        available() const
        {
            return (m_buffer.size() - m_pos);
        }

        // fill
        //   function: reads another chunk from the connection. Returns closed at
        // EOF.
        D_NODISCARD net::io_error
        fill()
        {
            if (m_eof)
            {
                return net::io_error::closed;
            }

            char            temp[16384];
            const net::io_result r = m_conn.read(temp, sizeof(temp));

            if (r.error != net::io_error::none)
            {
                return r.error;
            }

            if (r.count == 0)
            {
                m_eof = true;
                return net::io_error::closed;
            }

            m_buffer.append(temp, r.count);
            return net::io_error::none;
        }

        void
        compact()
        {
            if (m_pos > (64u * 1024u))
            {
                m_buffer.erase(0, m_pos);
                m_pos = 0;
            }

            return;
        }

        net::connection& m_conn;
        std::string      m_buffer;
        std::size_t      m_pos;
        bool             m_eof;
    };

    // build_request_wire
    //   function: serialises a request to HTTP/1.1 origin-form wire bytes,
    // supplying Host, User-Agent, Accept, Content-Length, and Connection: close
    // when the caller has not.
    D_NODISCARD inline std::string
    build_request_wire(
        const request&        _req,
        const url_parts&      _url,
        url_scheme            _scheme,
        const client_options& _options
    )
    {
        std::string target = _url.path.empty() ? std::string("/") : _url.path;

        if (!_url.query.empty())
        {
            target += "?";
            target += _url.query;
        }

        std::string out;
        out.reserve(256 + _req.body.size());

        out += to_string(_req.method);
        out += " ";
        out += target;
        out += " HTTP/1.1\r\n";

        // Host (with port only when non-default)
        if (!has_header(_req.headers, header_name::host))
        {
            out += header_name::host;
            out += ": ";
            out += _url.host;

            const port_type dflt = default_port(_scheme);

            if ( (_url.port != 0) &&
                 (_url.port != dflt) )
            {
                out += ":";
                out += std::to_string(static_cast<unsigned>(_url.port));
            }

            out += "\r\n";
        }

        if ( (!has_header(_req.headers, header_name::user_agent)) &&
             (!_options.user_agent.empty()) )
        {
            out += header_name::user_agent;
            out += ": ";
            out += _options.user_agent;
            out += "\r\n";
        }

        if (!has_header(_req.headers, header_name::accept))
        {
            out += header_name::accept;
            out += ": */*\r\n";
        }

        if ( (!_req.body.empty()) &&
             (!has_header(_req.headers, header_name::content_length)) )
        {
            out += header_name::content_length;
            out += ": ";
            out += std::to_string(_req.body.size());
            out += "\r\n";
        }

        if (!has_header(_req.headers, header_name::connection))
        {
            out += header_name::connection;
            out += ": close\r\n";
        }

        // caller-supplied headers verbatim
        for (const header_field& field : _req.headers)
        {
            out += field.first;
            out += ": ";
            out += field.second;
            out += "\r\n";
        }

        out += "\r\n";
        out += _req.body;

        return out;
    }

    // read_chunked
    //   function: decodes a chunked transfer-encoded body into `_body`.
    D_NODISCARD inline net::io_error
    read_chunked(
        reader&      _reader,
        std::string& _body,
        std::size_t  _max
    )
    {
        for (;;)
        {
            std::string line;
            net::io_error e = _reader.read_line(line, 4096);

            if (e != net::io_error::none)
            {
                return e;
            }

            const std::size_t semi = line.find(';');
            std::string       hex  = (semi == std::string::npos)
                                         ? line
                                         : line.substr(0, semi);
            hex = trim(hex);

            std::size_t size = 0;

            if (!parse_hex_size(hex, size))
            {
                return net::io_error::unknown;   // malformed chunk header
            }

            if (size == 0)
            {
                // consume trailer headers up to the blank line
                for (;;)
                {
                    std::string trailer;
                    e = _reader.read_line(trailer, 65536);

                    if (e != net::io_error::none)
                    {
                        return e;
                    }

                    if (trailer.empty())
                    {
                        break;
                    }
                }

                return net::io_error::none;
            }

            if ( (_max != 0) &&
                 (_body.size() + size > _max) )
            {
                return net::io_error::message_too_large;
            }

            e = _reader.read_exact(_body, size);

            if (e != net::io_error::none)
            {
                return e;
            }

            // the CRLF that terminates the chunk data
            std::string crlf;
            e = _reader.read_line(crlf, 8);

            if (e != net::io_error::none)
            {
                return e;
            }
        }
    }

    // parse_response
    //   function: parses a full response (status line, headers, body) from the
    // reader into `_out`. `_head` suppresses body reading for a HEAD request.
    D_NODISCARD inline net::io_error
    parse_response(
        reader&      _reader,
        bool         _head,
        response&    _out,
        std::size_t  _max
    )
    {
        std::string line;

        // ---- status line: HTTP/x.y CODE [reason] ----
        net::io_error e = _reader.read_line(line, 8192);

        if (e != net::io_error::none)
        {
            return e;
        }

        {
            const std::size_t sp1 = line.find(' ');

            if (sp1 == std::string::npos)
            {
                return net::io_error::unknown;
            }

            const std::size_t sp2   = line.find(' ', sp1 + 1);
            const std::string code  = (sp2 == std::string::npos)
                                          ? line.substr(sp1 + 1)
                                          : line.substr(sp1 + 1, sp2 - sp1 - 1);

            std::size_t parsed = 0;

            if (!parse_dec_size(code, parsed))
            {
                return net::io_error::unknown;
            }

            _out.status = static_cast<int>(parsed);
        }

        // ---- headers ----
        for (;;)
        {
            e = _reader.read_line(line, 65536);

            if (e != net::io_error::none)
            {
                return e;
            }

            if (line.empty())
            {
                break;
            }

            const std::size_t colon = line.find(':');

            if (colon == std::string::npos)
            {
                continue;   // skip a malformed header line
            }

            const std::string name  = trim(line.substr(0, colon));
            const std::string value = trim(line.substr(colon + 1));

            // preserve all fields (e.g. repeated Set-Cookie)
            _out.headers.push_back(header_field(name, value));
        }

        // ---- body framing ----
        const bool no_body = ( _head ||
                               (_out.status == 204) ||
                               (_out.status == 304) ||
                               ((_out.status >= 100) && (_out.status < 200)) );

        if (no_body)
        {
            return net::io_error::none;
        }

        const std::string te = header_value(_out.headers,
                                            "Transfer-Encoding",
                                            std::string());

        if (icontains(te, "chunked"))
        {
            return read_chunked(_reader, _out.body, _max);
        }

        const header_field* cl =
            find_header(_out.headers, header_name::content_length);

        if (cl != nullptr)
        {
            std::size_t length = 0;

            if (!parse_dec_size(cl->second, length))
            {
                return net::io_error::unknown;
            }

            if ( (_max != 0) &&
                 (length > _max) )
            {
                return net::io_error::message_too_large;
            }

            return _reader.read_exact(_out.body, length);
        }

        // no length and not chunked: read until the peer closes
        return _reader.read_to_eof(_out.body, _max);
    }

    // open_transport
    //   function: connects to the URL's host/port with the scheme-appropriate
    // transport (TLS for secure schemes). On failure, sets `_terr`.
    D_NODISCARD inline net::open_result
    open_transport(
        const url_parts&      _url,
        url_scheme            _scheme,
        const client_options& _options,
        transport_error&      _terr
    )
    {
        const port_type port =
            (_url.port != 0) ? _url.port : default_port(_scheme);

        const net::endpoint ep(_url.host, port);

        if (is_secure(_scheme))
        {
            net::tls_connector<> connector;
            connector.set_verify(_options.verify_tls);
            connector.set_server_name(_url.host);
            connector.transport().options().connect_timeout_ms =
                _options.connect_timeout_ms;

            net::open_result r = connector.connect(ep);

            if (!r.ok())
            {
                _terr = map_connect_error(r.error, true);
            }

            return r;
        }

        net::tcp_connector connector;
        connector.options().connect_timeout_ms = _options.connect_timeout_ms;

        net::open_result r = connector.connect(ep);

        if (!r.ok())
        {
            _terr = map_connect_error(r.error, false);
        }

        return r;
    }

    // resolve_redirect
    //   function: resolves a Location value against the current URL (absolute,
    // scheme-relative, root-relative, or path-relative). Empty on failure.
    D_NODISCARD inline std::string
    resolve_redirect(
        const url_parts& _base,
        url_scheme       _base_scheme,
        const std::string& _location
    )
    {
        // absolute (has its own scheme)?
        {
            url_parts probe;

            if ( split_url(_location, probe) &&
                 (!probe.scheme.empty()) )
            {
                return _location;
            }
        }

        std::string origin = to_string(_base_scheme);
        origin += "://";
        origin += _base.host;

        const port_type dflt = default_port(_base_scheme);

        if ( (_base.port != 0) &&
             (_base.port != dflt) )
        {
            origin += ":";
            origin += std::to_string(static_cast<unsigned>(_base.port));
        }

        // scheme-relative ("//host/path")
        if ( (_location.size() >= 2) &&
             (_location[0] == '/') &&
             (_location[1] == '/') )
        {
            std::string out = to_string(_base_scheme);
            out += ":";
            out += _location;
            return out;
        }

        // root-relative ("/path")
        if ( (!_location.empty()) &&
             (_location[0] == '/') )
        {
            return origin + _location;
        }

        // path-relative: resolve against the directory of the base path
        std::string base_path = _base.path.empty() ? std::string("/") : _base.path;
        const std::size_t slash = base_path.find_last_of('/');
        const std::string dir   = (slash == std::string::npos)
                                      ? std::string("/")
                                      : base_path.substr(0, slash + 1);

        return origin + dir + _location;
    }

    // apply_redirect_method
    //   function: rewrites the request method/body for a redirect per the usual
    // rules: 303 (and 301/302 on POST) become GET with no body; 307/308 preserve
    // the method and body.
    inline void
    apply_redirect_method(
        request& _req,
        int      _status
    )
    {
        if (_status == 303)
        {
            _req.method = http_method::get;
            _req.body.clear();
            return;
        }

        if ( (_status == 301) ||
             (_status == 302) )
        {
            if ( (_req.method != http_method::get) &&
                 (_req.method != http_method::head) )
            {
                _req.method = http_method::get;
                _req.body.clear();
            }
        }

        // 307 / 308: method and body are preserved
        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                             CLIENT                                     ///
///////////////////////////////////////////////////////////////////////////////

// client
//   class: an HTTP/1.1 client over the net transport. send() performs a request
// and returns a web::response, following redirects up to the configured cap;
// get / head / post are convenience wrappers. The response carries a
// transport_error (response.error) on a transport-level failure.
class client
{
public:
    client()
        : m_options()
    {
    }

    explicit client(
        const client_options& _options
    )
        : m_options(_options)
    {
    }

    // options
    //   function: mutable access to the client options.
    D_NODISCARD client_options&
    options()
    {
        return m_options;
    }

    // send
    //   function: performs `_request`, following redirects up to the cap.
    D_NODISCARD response
    send(
        const request& _request
    )
    {
        request     req = _request;
        std::string url = _request.url;
        int         followed = 0;

        for (;;)
        {
            url_parts parts;

            if (!split_url(url, parts))
            {
                response r;
                r.error = transport_error::unsupported_protocol;
                return r;
            }

            const url_scheme scheme = scheme_from_string(parts.scheme);

            if ( (scheme != url_scheme::http) &&
                 (scheme != url_scheme::https) )
            {
                response r;
                r.error = transport_error::unsupported_protocol;
                return r;
            }

            req.url = url;

            const bool head = (req.method == http_method::head);

            response resp = perform_once(req, parts, scheme, head);

            if (resp.error != transport_error::none)
            {
                return resp;
            }

            if (!internal::is_redirect(resp.status))
            {
                return resp;
            }

            const header_field* location =
                find_header(resp.headers, header_name::location);

            if ( (location == nullptr) ||
                 (location->second.empty()) )
            {
                return resp;   // redirect without a target: hand it back
            }

            if (followed >= m_options.max_redirects)
            {
                resp.error = transport_error::too_many_redirects;
                return resp;
            }

            const std::string next =
                internal::resolve_redirect(parts, scheme, location->second);

            if (next.empty())
            {
                return resp;
            }

            internal::apply_redirect_method(req, resp.status);

            url = next;
            ++followed;
        }
    }

    // get / head / post
    //   function: convenience request builders.
    D_NODISCARD response
    get(
        const std::string& _url
    )
    {
        request req;
        req.method = http_method::get;
        req.url    = _url;

        return send(req);
    }

    D_NODISCARD response
    head(
        const std::string& _url
    )
    {
        request req;
        req.method = http_method::head;
        req.url    = _url;

        return send(req);
    }

    D_NODISCARD response
    post(
        const std::string& _url,
        const std::string& _body,
        const std::string& _content_type
    )
    {
        request req;
        req.method = http_method::post;
        req.url    = _url;
        req.body   = _body;

        if (!_content_type.empty())
        {
            req.set_header(header_name::content_type, _content_type);
        }

        return send(req);
    }

private:
    // perform_once
    //   function: a single request/response with no redirect handling.
    D_NODISCARD response
    perform_once(
        const request&   _req,
        const url_parts& _url,
        url_scheme       _scheme,
        bool             _head
    )
    {
        response        resp;
        transport_error terr = transport_error::none;

        net::open_result transport =
            internal::open_transport(_url, _scheme, m_options, terr);

        if (!transport.ok())
        {
            resp.error = terr;
            return resp;
        }

        net::connection& conn = *transport.conn;

        // ---- send ----
        const std::string wire =
            internal::build_request_wire(_req, _url, _scheme, m_options);

        if (net::write_all(conn, wire) != net::io_error::none)
        {
            resp.error = transport_error::write_error;
            conn.close();
            return resp;
        }

        // ---- receive ----
        internal::reader   rd(conn);
        const net::io_error pe =
            internal::parse_response(rd, _head, resp, m_options.max_response_bytes);

        conn.close();

        if (pe == net::io_error::none)
        {
            resp.error = transport_error::none;
            return resp;
        }

        // a failure before any status is a read failure; a failure after the
        // status is a truncated/oversized body (partial content is retained)
        if (resp.status == 0)
        {
            resp.error = transport_error::read_error;
        }
        else if (pe == net::io_error::message_too_large)
        {
            resp.error = transport_error::out_of_memory;
        }
        else
        {
            resp.error = transport_error::read_error;
        }

        return resp;
    }

    client_options m_options;
};


///////////////////////////////////////////////////////////////////////////////
///                        FREE CONVENIENCE                                ///
///////////////////////////////////////////////////////////////////////////////

// get
//   function: a one-shot GET with default options.
D_NODISCARD inline response
get(
    const std::string& _url
)
{
    client c;
    return c.get(_url);
}

// fetch
//   function: performs a prepared request with default options.
D_NODISCARD inline response
fetch(
    const request& _request
)
{
    client c;
    return c.send(_request);
}


NS_END  // http
NS_END  // web
NS_END  // djinterp


#endif  // DJINTERP_WEB_HTTP_
