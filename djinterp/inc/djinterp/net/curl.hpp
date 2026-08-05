/******************************************************************************
* djinterp [web]                                                      curl.hpp
*
*   libcurl-specific foundational module for the djinterp web subframework.
* It is the thin, RAII-safe C++ layer over libcurl's easy interface that the
* neutral web vocabulary (web.hpp) is executed through, and the substrate the
* vendor integrations (e.g. web/vendor/claude.hpp) build on.
*
* CONTENTS (all in namespace djinterp::web::curl):
*   0.  availability gate + <curl/curl.h>
*   I.   library / runtime feature queries (version, curl_version_info bits)
*   II.  error mapping (CURLcode -> web::transport_error, message)
*   III. RAII wrappers -- scoped_global, ensure_global, slist, easy
*   IV.  header bridge + write/header callback trampolines (internal)
*   V.   options + perform() / perform_stream() drivers over web::request
*
*   All API-presence is compile-time gated through env_curl.h; whether a given
* libcurl BUILD supports a wire feature (HTTP/2, a TLS backend, ...) is a
* RUNTIME property, exposed here via the feature_supported() helpers.
*
*   Requires:  web.hpp, env_curl.h, and libcurl (>= 7.17). Including this header
* where D_ENV_CURL_AVAILABLE is 0 is a hard error by design.
*
* path:      /inc/djinterp/web/curl/curl.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.16
******************************************************************************/

#ifndef DJINTERP_WEB_CURL_
#define DJINTERP_WEB_CURL_ 1

// djinterp -- neutral web vocabulary + curl-specific detection
#include "../web.hpp"
#include "../../core/env/web/env_curl.h"


// ===========================================================================
// 0.   AVAILABILITY GATE
// ===========================================================================

#if !D_ENV_CURL_AVAILABLE
    #error "curl.hpp requires libcurl: <curl/curl.h> was not found, or the "  \
           "target platform has no networking. Install libcurl development "  \
           "headers, or pre-define D_ENV_CURL_AVAILABLE / the D_ENV_CURL_* "  \
           "flags to describe a cross-compilation target."
#endif

// std
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
// libcurl
#include <curl/curl.h>


// D_KEYWORD_CURL
//   keyword: resolves to `curl`. Namespace/identifier tag for the libcurl
// integration. Guarded so the core may adopt it later without collision.
#ifndef D_KEYWORD_CURL
    #define D_KEYWORD_CURL              curl
#endif


NS_DJINTERP
NS_WEB
D_NAMESPACE(D_KEYWORD_CURL)


///////////////////////////////////////////////////////////////////////////////
///              I.   LIBRARY / RUNTIME FEATURE QUERIES                     ///
///////////////////////////////////////////////////////////////////////////////

// version_string
//   function: the runtime libcurl version banner (curl_version()).
D_NODISCARD inline const char*
version_string()
{
    return curl_version();
}

// version_info
//   function: the full runtime version-info record, or nullptr if unavailable.
// Callers inspect ->features, ->ssl_version, ->host, etc.
D_NODISCARD inline const curl_version_info_data*
version_info()
{
    return curl_version_info(CURLVERSION_NOW);
}

// feature_supported
//   function: whether the running libcurl was BUILT with the given
// CURL_VERSION_* feature bit(s) set (e.g. CURL_VERSION_SSL,
// CURL_VERSION_HTTP2). This is the runtime complement to the compile-time
// D_ENV_CURL_HAS_* API flags.
D_NODISCARD inline bool
feature_supported(
    int _feature_mask
)
{
    const curl_version_info_data* info = version_info();

    return ( info &&
             ((info->features & _feature_mask) == _feature_mask) );
}

// supports_ssl
//   function: whether TLS/SSL is available in the running libcurl build.
D_NODISCARD inline bool
supports_ssl()
{
    return feature_supported(CURL_VERSION_SSL);
}

// supports_http2
//   function: whether HTTP/2 is available in the running libcurl build.
D_NODISCARD inline bool
supports_http2()
{
    return feature_supported(CURL_VERSION_HTTP2);
}


///////////////////////////////////////////////////////////////////////////////
///                       II.   ERROR MAPPING                              ///
///////////////////////////////////////////////////////////////////////////////

// error_message
//   function: the human-readable message for a CURLcode (curl_easy_strerror).
D_NODISCARD inline const char*
error_message(
    CURLcode _code
)
{
    return curl_easy_strerror(_code);
}

// to_transport_error
//   function: maps a CURLcode onto the library-neutral web::transport_error so
// higher layers can reason about failures without depending on libcurl. Codes
// without a specific neutral counterpart collapse to transport_error::unknown;
// CURLE_OK maps to transport_error::none.
D_NODISCARD inline transport_error
to_transport_error(
    CURLcode _code
)
{
    switch (_code)
    {
        case CURLE_OK:
            return transport_error::none;

        case CURLE_UNSUPPORTED_PROTOCOL:
        case CURLE_URL_MALFORMAT:
            return transport_error::unsupported_protocol;

        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_COULDNT_RESOLVE_PROXY:
            return transport_error::could_not_resolve_host;

        case CURLE_COULDNT_CONNECT:
            return transport_error::could_not_connect;

        case CURLE_OPERATION_TIMEDOUT:
            return transport_error::timed_out;

        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_PEER_FAILED_VERIFICATION:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CIPHER:
        case CURLE_USE_SSL_FAILED:
            return transport_error::tls_error;

        case CURLE_TOO_MANY_REDIRECTS:
            return transport_error::too_many_redirects;

        case CURLE_WRITE_ERROR:
            return transport_error::write_error;

        case CURLE_READ_ERROR:
            return transport_error::read_error;

        case CURLE_ABORTED_BY_CALLBACK:
            return transport_error::canceled;

        case CURLE_OUT_OF_MEMORY:
            return transport_error::out_of_memory;

        default:
            return transport_error::unknown;
    }
}


///////////////////////////////////////////////////////////////////////////////
///                      III.   RAII WRAPPERS                              ///
///////////////////////////////////////////////////////////////////////////////

// scoped_global
//   class: RAII owner of the process-global libcurl state. Constructing it
// calls curl_global_init; destroying it calls curl_global_cleanup (only if the
// init succeeded). Neither copyable nor movable -- create exactly one, early,
// on the main thread, before any other libcurl use.
class scoped_global
{
public:
    explicit scoped_global(
        long _flags = CURL_GLOBAL_DEFAULT
    )
        : m_code(curl_global_init(_flags))
    {
    }

    ~scoped_global()
    {
        // balance a successful init with exactly one cleanup
        if (m_code == CURLE_OK)
        {
            curl_global_cleanup();
        }
    }

    scoped_global(const scoped_global&)            D_DELETE;
    scoped_global& operator=(const scoped_global&) D_DELETE;
    scoped_global(scoped_global&&)                 D_DELETE;
    scoped_global& operator=(scoped_global&&)      D_DELETE;

    // ok
    //   function: whether global initialization succeeded.
    D_NODISCARD bool
    ok() const
    {
        return (m_code == CURLE_OK);
    }

    // code
    //   function: the CURLcode returned by curl_global_init.
    D_NODISCARD CURLcode
    code() const
    {
        return m_code;
    }

private:
    CURLcode m_code;
};

// ensure_global
//   function: lazily performs a one-time, thread-safe curl_global_init (via a
// function-local static) and returns whether it succeeded. Intended for the
// convenience drivers below so callers need not manage global state manually.
// Deliberately does NOT register a cleanup: the initialization lives for the
// duration of the process. Prefer scoped_global when deterministic teardown is
// required.
inline bool
ensure_global(
    long _flags = CURL_GLOBAL_DEFAULT
)
{
    static const CURLcode s_code = curl_global_init(_flags);

    return (s_code == CURLE_OK);
}

// slist
//   class: RAII owner of a curl_slist (the linked string list libcurl uses for
// request headers and similar). Move-only; frees the whole list on destruction.
class slist
{
public:
    slist()
        : m_list(nullptr)
    {
    }

    ~slist()
    {
        if (m_list)
        {
            curl_slist_free_all(m_list);
        }
    }

    slist(const slist&)            D_DELETE;
    slist& operator=(const slist&) D_DELETE;

    slist(
        slist&& _other
    ) D_NOEXCEPT
        : m_list(_other.m_list)
    {
        _other.m_list = nullptr;
    }

    slist&
    operator=(
        slist&& _other
    ) D_NOEXCEPT
    {
        // guard against self-move, then adopt the source list
        if (this != &_other)
        {
            if (m_list)
            {
                curl_slist_free_all(m_list);
            }

            m_list        = _other.m_list;
            _other.m_list = nullptr;
        }

        return *this;
    }

    // append
    //   function: appends a copy of `_value` to the list. Returns true on
    // success; on allocation failure the list is left unchanged and false is
    // returned.
    bool
    append(
        const char* _value
    )
    {
        struct curl_slist* next = curl_slist_append(m_list, _value);

        if (!next)
        {
            return false;
        }

        m_list = next;

        return true;
    }

    // append
    //   function: std::string overload of append.
    bool
    append(
        const std::string& _value
    )
    {
        return append(_value.c_str());
    }

    // get
    //   function: the underlying curl_slist pointer (nullptr when empty).
    D_NODISCARD struct curl_slist*
    get() const
    {
        return m_list;
    }

    // empty
    //   function: whether the list holds no entries.
    D_NODISCARD bool
    empty() const
    {
        return (m_list == nullptr);
    }

    // release
    //   function: relinquishes ownership of the list and returns it; the caller
    // becomes responsible for curl_slist_free_all.
    D_NODISCARD struct curl_slist*
    release()
    {
        struct curl_slist* out = m_list;
        m_list = nullptr;

        return out;
    }

private:
    struct curl_slist* m_list;
};

// easy
//   class: RAII wrapper over a libcurl easy handle (CURL*). Move-only. Exposes
// a generic setopt/getinfo pair plus typed convenience setters for the common
// options; all setters return the CURLcode so callers may check them.
class easy
{
public:
    easy()
        : m_handle(curl_easy_init())
    {
    }

    ~easy()
    {
        if (m_handle)
        {
            curl_easy_cleanup(m_handle);
        }
    }

    easy(const easy&)            D_DELETE;
    easy& operator=(const easy&) D_DELETE;

    easy(
        easy&& _other
    ) D_NOEXCEPT
        : m_handle(_other.m_handle)
    {
        _other.m_handle = nullptr;
    }

    easy&
    operator=(
        easy&& _other
    ) D_NOEXCEPT
    {
        // guard against self-move, then adopt the source handle
        if (this != &_other)
        {
            if (m_handle)
            {
                curl_easy_cleanup(m_handle);
            }

            m_handle        = _other.m_handle;
            _other.m_handle = nullptr;
        }

        return *this;
    }

    // operator bool
    //   function: whether the handle was successfully created.
    D_NODISCARD explicit operator bool() const
    {
        return (m_handle != nullptr);
    }

    // get
    //   function: the underlying CURL* (nullptr if construction failed).
    D_NODISCARD CURL*
    get() const
    {
        return m_handle;
    }

    // reset
    //   function: restores the handle to its default state (curl_easy_reset),
    // preserving live connections while clearing all options.
    void
    reset()
    {
        if (m_handle)
        {
            curl_easy_reset(m_handle);
        }

        return;
    }

    // setopt
    //   function: generic option setter forwarding to curl_easy_setopt. `_value`
    // must have the type the option expects (a long, a pointer, an off_t, ...).
    template<typename _Value>
    CURLcode
    setopt(
        CURLoption _option,
        _Value     _value
    )
    {
        return curl_easy_setopt(m_handle, _option, _value);
    }

    // getinfo
    //   function: generic info getter forwarding to curl_easy_getinfo. `_out`
    // points to storage of the type the info item yields.
    template<typename _Value>
    CURLcode
    getinfo(
        CURLINFO _info,
        _Value*  _out
    )
    {
        return curl_easy_getinfo(m_handle, _info, _out);
    }

    // set_url
    //   function: sets the target URL (CURLOPT_URL).
    CURLcode
    set_url(
        const std::string& _url
    )
    {
        return setopt(CURLOPT_URL, _url.c_str());
    }

    // set_custom_request
    //   function: sets the request method token (CURLOPT_CUSTOMREQUEST).
    CURLcode
    set_custom_request(
        const char* _method
    )
    {
        return setopt(CURLOPT_CUSTOMREQUEST, _method);
    }

    // set_body
    //   function: sets a copied request body (CURLOPT_POSTFIELDSIZE +
    // CURLOPT_COPYPOSTFIELDS). libcurl copies the bytes, so `_body` need not
    // outlive the call. The size must be set before the copy so binary bodies
    // with embedded NULs are handled correctly.
    CURLcode
    set_body(
        const std::string& _body
    )
    {
        const CURLcode rc = setopt(CURLOPT_POSTFIELDSIZE,
                                   static_cast<long>(_body.size()));

        if (rc != CURLE_OK)
        {
            return rc;
        }

        return setopt(CURLOPT_COPYPOSTFIELDS, _body.c_str());
    }

    // set_headers
    //   function: attaches a request header list (CURLOPT_HTTPHEADER). The list
    // must outlive the perform() call.
    CURLcode
    set_headers(
        struct curl_slist* _headers
    )
    {
        return setopt(CURLOPT_HTTPHEADER, _headers);
    }

    // set_user_agent
    //   function: sets the User-Agent (CURLOPT_USERAGENT).
    CURLcode
    set_user_agent(
        const std::string& _agent
    )
    {
        return setopt(CURLOPT_USERAGENT, _agent.c_str());
    }

    // set_accept_encoding
    //   function: sets the accepted content encodings (CURLOPT_ACCEPT_ENCODING).
    // An empty string advertises every coding this libcurl build supports.
    CURLcode
    set_accept_encoding(
        const std::string& _encoding
    )
    {
        return setopt(CURLOPT_ACCEPT_ENCODING, _encoding.c_str());
    }

    // set_timeout_ms
    //   function: sets the whole-transfer timeout (CURLOPT_TIMEOUT_MS).
    CURLcode
    set_timeout_ms(
        long _ms
    )
    {
        return setopt(CURLOPT_TIMEOUT_MS, _ms);
    }

    // set_connect_timeout_ms
    //   function: sets the connection-phase timeout (CURLOPT_CONNECTTIMEOUT_MS).
    CURLcode
    set_connect_timeout_ms(
        long _ms
    )
    {
        return setopt(CURLOPT_CONNECTTIMEOUT_MS, _ms);
    }

    // set_follow_location
    //   function: enables/disables redirect following (CURLOPT_FOLLOWLOCATION).
    CURLcode
    set_follow_location(
        bool _follow
    )
    {
        return setopt(CURLOPT_FOLLOWLOCATION, _follow ? 1L : 0L);
    }

    // set_max_redirects
    //   function: caps the number of redirects followed (CURLOPT_MAXREDIRS).
    CURLcode
    set_max_redirects(
        long _max
    )
    {
        return setopt(CURLOPT_MAXREDIRS, _max);
    }

    // set_verify_tls
    //   function: enables/disables peer and host TLS verification
    // (CURLOPT_SSL_VERIFYPEER + CURLOPT_SSL_VERIFYHOST). Disabling is insecure
    // and intended only for local testing.
    CURLcode
    set_verify_tls(
        bool _verify
    )
    {
        const CURLcode rc = setopt(CURLOPT_SSL_VERIFYPEER, _verify ? 1L : 0L);

        if (rc != CURLE_OK)
        {
            return rc;
        }

        return setopt(CURLOPT_SSL_VERIFYHOST, _verify ? 2L : 0L);
    }

    // set_verbose
    //   function: toggles libcurl's verbose diagnostics (CURLOPT_VERBOSE).
    CURLcode
    set_verbose(
        bool _verbose
    )
    {
        return setopt(CURLOPT_VERBOSE, _verbose ? 1L : 0L);
    }

    // response_code
    //   function: the last response's status code (CURLINFO_RESPONSE_CODE), or
    // 0 if unavailable.
    D_NODISCARD long
    response_code()
    {
        long code = 0;
        getinfo(CURLINFO_RESPONSE_CODE, &code);

        return code;
    }

    // perform
    //   function: runs the transfer synchronously (curl_easy_perform).
    CURLcode
    perform()
    {
        return curl_easy_perform(m_handle);
    }

private:
    CURL* m_handle;
};


///////////////////////////////////////////////////////////////////////////////
///           IV.   HEADER BRIDGE + CALLBACK TRAMPOLINES                    ///
///////////////////////////////////////////////////////////////////////////////

// body_sink
//   type: a callback receiving response-body chunks as they arrive. Returning
// false aborts the transfer (surfaced as transport_error::canceled).
using body_sink = std::function<bool(const char*, std::size_t)>;

NS_INTERNAL

    // write_trampoline
    //   function: libcurl CURLOPT_WRITEFUNCTION callback. Forwards each chunk
    // to the body_sink pointed at by _userdata; a sink returning false yields a
    // short count, which libcurl treats as a write error / abort.
    inline std::size_t
    write_trampoline(
        char*       _ptr,
        std::size_t _size,
        std::size_t _nmemb,
        void*       _userdata
    )
    {
        const std::size_t total = _size * _nmemb;
        body_sink*        sink  = static_cast<body_sink*>(_userdata);

        // no sink -> silently discard, but report success so the transfer runs
        if ( (!sink) ||
             (!*sink) )
        {
            return total;
        }

        return (*sink)(_ptr, total) ? total : 0;
    }

    // header_trampoline
    //   function: libcurl CURLOPT_HEADERFUNCTION callback. Parses each
    // "Name: Value" line into the header_list pointed at by _userdata; the
    // status line and blank separators (which carry no colon) are ignored.
    inline std::size_t
    header_trampoline(
        char*       _buffer,
        std::size_t _size,
        std::size_t _nitems,
        void*       _userdata
    )
    {
        const std::size_t total = _size * _nitems;
        header_list*      out   = static_cast<header_list*>(_userdata);

        // parse into the sink when present
        if (out)
        {
            std::string line(_buffer, total);

            // strip the trailing CRLF
            while ( (!line.empty()) &&
                    ((line.back() == '\r') || (line.back() == '\n')) )
            {
                line.pop_back();
            }

            const std::size_t colon = line.find(':');

            // a colon distinguishes a real field from the status line / blank
            if (colon != std::string::npos)
            {
                std::string name  = line.substr(0, colon);
                std::string value = line.substr(colon + 1);

                // trim optional leading whitespace from the value
                const std::size_t first = value.find_first_not_of(" \t");

                if (first != std::string::npos)
                {
                    value = value.substr(first);
                }
                else
                {
                    value.clear();
                }

                // trim trailing whitespace from the value
                const std::size_t last = value.find_last_not_of(" \t");

                if (last != std::string::npos)
                {
                    value.erase(last + 1);
                }

                out->push_back(header_field(name, value));
            }
        }

        return total;
    }

NS_END  // internal

// make_header_slist
//   function: builds a curl_slist (as an owning slist) from a neutral
// web::header_list, formatting each field as "Name: Value".
D_NODISCARD inline slist
make_header_slist(
    const header_list& _headers
)
{
    slist list;

    // format and append each header field
    for (const header_field& field : _headers)
    {
        const std::string line = field.first + ": " + field.second;
        list.append(line);
    }

    return list;
}


///////////////////////////////////////////////////////////////////////////////
///               V.   OPTIONS + PERFORM DRIVERS                           ///
///////////////////////////////////////////////////////////////////////////////

// options
//   struct: per-request transport options consumed by the perform drivers. The
// defaults are safe and conventional (verified TLS, redirects followed, the
// framework User-Agent, and all supported content encodings advertised).
struct options
{
    long        timeout_ms;          // whole transfer; 0 = no explicit limit
    long        connect_timeout_ms;  // connect phase; 0 = no explicit limit
    bool        follow_redirects;
    long        max_redirects;
    bool        verify_tls;
    bool        verbose;
    std::string accept_encoding;     // "" = advertise all supported codings
    std::string user_agent;

    options()
        : timeout_ms(0),
          connect_timeout_ms(0),
          follow_redirects(true),
          max_redirects(30),
          verify_tls(true),
          verbose(false),
          accept_encoding(),
          user_agent(D_WEB_DEFAULT_USER_AGENT)
    {
    }
};

NS_INTERNAL

    // configure_easy
    //   function: applies a web::request and options to an easy handle, wiring
    // method, body, headers (via the caller-owned slist), and the standard
    // transport options. The header slist and body_sink must outlive the
    // subsequent perform() call.
    inline void
    configure_easy(
        easy&          _easy,
        const request& _request,
        const options& _options,
        slist&         _headers,
        body_sink*     _body_sink,
        header_list*   _response_headers
    )
    {
        _easy.set_url(_request.url);
        _easy.set_follow_location(_options.follow_redirects);
        _easy.set_max_redirects(_options.max_redirects);
        _easy.set_verify_tls(_options.verify_tls);
        _easy.set_verbose(_options.verbose);
        _easy.set_accept_encoding(_options.accept_encoding);
        _easy.set_user_agent(
            _options.user_agent.empty() ? std::string(D_WEB_DEFAULT_USER_AGENT)
                                        : _options.user_agent);

        // apply timeouts only when explicitly requested
        if (_options.timeout_ms > 0)
        {
            _easy.set_timeout_ms(_options.timeout_ms);
        }

        if (_options.connect_timeout_ms > 0)
        {
            _easy.set_connect_timeout_ms(_options.connect_timeout_ms);
        }

        // method: a custom-request token covers every verb uniformly; HEAD
        // additionally suppresses the response body
        _easy.set_custom_request(to_string(_request.method));

        if (_request.method == http_method::head)
        {
            _easy.setopt(CURLOPT_NOBODY, 1L);
        }

        // body (copied by libcurl) when present
        if (!_request.body.empty())
        {
            _easy.set_body(_request.body);
        }

        // request headers
        _headers = make_header_slist(_request.headers);

        if (!_headers.empty())
        {
            _easy.set_headers(_headers.get());
        }

        // response body + header sinks
        _easy.setopt(CURLOPT_WRITEFUNCTION, &internal::write_trampoline);
        _easy.setopt(CURLOPT_WRITEDATA, _body_sink);
        _easy.setopt(CURLOPT_HEADERFUNCTION, &internal::header_trampoline);
        _easy.setopt(CURLOPT_HEADERDATA, _response_headers);

        return;
    }

NS_END  // internal

// perform_stream
//   function: executes `_request`, delivering response-body chunks to `_sink`
// as they arrive (no full-body buffering) and recording status + response
// headers into `_meta`. `_sink` returning false aborts the transfer. Returns
// the neutral transport_error (transport_error::none on success). Suitable for
// streaming responses such as server-sent events.
D_NODISCARD inline transport_error
perform_stream(
    const request&   _request,
    const body_sink& _sink,
    response&        _meta,
    const options&   _options = options()
)
{
    _meta = response();

    // one-time global init for the convenience path
    if (!ensure_global())
    {
        _meta.error = transport_error::out_of_memory;

        return _meta.error;
    }

    easy handle;

    // a null handle means libcurl could not allocate one
    if (!handle)
    {
        _meta.error = transport_error::out_of_memory;

        return _meta.error;
    }

    slist     headers;
    body_sink sink = _sink;

    internal::configure_easy(handle,
                             _request,
                             _options,
                             headers,
                             &sink,
                             &_meta.headers);

    const CURLcode rc = handle.perform();
    _meta.error = to_transport_error(rc);

    // capture the status code whenever the exchange produced one
    if (rc == CURLE_OK)
    {
        _meta.status = static_cast<int>(handle.response_code());
    }

    return _meta.error;
}

// perform
//   function: executes `_request` and returns a fully-populated neutral
// web::response (status, headers, buffered body, and transport error). This is
// the primary entry point for one-shot request/response exchanges.
D_NODISCARD inline response
perform(
    const request& _request,
    const options& _options = options()
)
{
    response res;

    // buffer the body via a sink that appends to a local string
    std::string body;
    body_sink   sink =
        [&body](const char* _data, std::size_t _length) -> bool
        {
            body.append(_data, _length);

            return true;
        };

    const transport_error err = perform_stream(_request, sink, res, _options);

    // move the buffered body into the response on success
    if (err == transport_error::none)
    {
        res.body.swap(body);
    }

    return res;
}


NS_END  // curl
NS_END  // web
NS_END  // djinterp


#endif  // DJINTERP_WEB_CURL_
