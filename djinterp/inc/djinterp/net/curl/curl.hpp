/*******************************************************************************
* djinterp [net]                                                        curl.hpp
*
* The C++ face of the libcurl binding: web.hpp's requests, performed through
* net/curl/curl.h.
*   A derived layer. Every transfer runs through d_curl_perform; this header
* translates web.hpp's vocabulary into the binding's records and back, and
* adds what C lacks:
*     - status mapping to web::transport_error                       [2]
*     - library queries, and RAII over the global state              [3]
*     - options, std::function sinks, and the two drivers            [4]
*   No libcurl type appears here, and the header compiles without libcurl;
* transfers then fail as unsupported_protocol. Define D_CFG_CURL as 1 to make
* a build without libcurl an error instead (cfg_curl.h).
*   A sink may throw. Unwinding through libcurl's C frames is undefined, so
* the exception is caught where libcurl calls back into C++, the transfer
* ends and is cleaned up, and the driver rethrows it. Builds without
* exceptions compile none of this.
*
*
* path:      /inc/djinterp/net/curl/curl.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.16
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMESPACE
    ---------
    1.  Keyword
         1.  D_KEYWORD_CURL
2.  ERRORS
    ------
    1.  Status mapping
3.  LIBRARY
    -------
    1.  Features
         1.  feature
    2.  Library queries
    3.  Global state
         1.  scoped_global
         2.  ensure_global
4.  TRANSFERS
    ---------
    1.  Sinks and options
         1.  body_sink
         2.  options
    2.  The bridge to C
         1.  stream_state
    3.  Drivers
*/

#ifndef DJINTERP_NET_CURL_CURL_HPP
#define DJINTERP_NET_CURL_CURL_HPP 1

// std
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint32_t
#include <exception>   // std::exception_ptr, std::rethrow_exception
#include <functional>  // std::function
#include <string>      // std::string
#include <vector>      // std::vector
// djinterp
#include "../../djinterp.hpp"         // framework root
#include "../../env/cpp/env_cpp98.h"  // D_ENV_CPP98_HAS_EXCEPTION
#include "../web.hpp"                 // request, response, transport_error
#include "./curl.h"                   // the C binding


//==============================================================================
// 1.  NAMESPACE
//==============================================================================


// 1.1    Keyword
//------------------------------------------------------------------------------
// 1.1.1
// D_KEYWORD_CURL
//   keyword: resolves to `curl`, the name of this layer's namespace inside
// djinterp::web. Guarded, so the core may adopt it without collision.
#ifndef D_KEYWORD_CURL
    #define D_KEYWORD_CURL curl
#endif  // D_KEYWORD_CURL


NS_DJINTERP
NS_WEB
D_NAMESPACE(D_KEYWORD_CURL)


//==============================================================================
// 2.  ERRORS
//==============================================================================
// The binding's statuses in web.hpp's vocabulary. The categories match one
// for one; the two the binding adds have no web counterpart and map to the
// nearest honest one.


// 2.1    Status mapping
//------------------------------------------------------------------------------
// to_transport_error
//   function: a binding status as a transport_error. UNSUPPORTED -- a build
// without libcurl -- is unsupported_protocol, and INVALID_ARGUMENT, a field
// the binding refuses, is unknown.
D_NODISCARD inline transport_error
to_transport_error(
    ::d_curl_status _status
) noexcept
{
    switch (_status)
    {
        case D_CURL_STATUS_OK:
            return transport_error::none;

        case D_CURL_STATUS_UNSUPPORTED:
        case D_CURL_STATUS_UNSUPPORTED_PROTOCOL:
            return transport_error::unsupported_protocol;

        case D_CURL_STATUS_COULD_NOT_RESOLVE_HOST:
            return transport_error::could_not_resolve_host;

        case D_CURL_STATUS_COULD_NOT_CONNECT:
            return transport_error::could_not_connect;

        case D_CURL_STATUS_TIMED_OUT:
            return transport_error::timed_out;

        case D_CURL_STATUS_TLS_ERROR:
            return transport_error::tls_error;

        case D_CURL_STATUS_TOO_MANY_REDIRECTS:
            return transport_error::too_many_redirects;

        case D_CURL_STATUS_WRITE_ERROR:
            return transport_error::write_error;

        case D_CURL_STATUS_READ_ERROR:
            return transport_error::read_error;

        case D_CURL_STATUS_CANCELED:
            return transport_error::canceled;

        case D_CURL_STATUS_OUT_OF_MEMORY:
            return transport_error::out_of_memory;

        case D_CURL_STATUS_INVALID_ARGUMENT:
        case D_CURL_STATUS_UNKNOWN:
        default:
            return transport_error::unknown;
    }
}


//==============================================================================
// 3.  LIBRARY
//==============================================================================
// What the libcurl this program runs with can do, and its global state. All
// of it answers without libcurl: nothing is available, nothing supported.


// 3.1    Features
//------------------------------------------------------------------------------
// 3.1.1
// feature
//   enum: a capability libcurl reports at run time, valued as
// d_curl_feature.
enum class feature : unsigned char
{
    ssl        = D_CURL_FEATURE_SSL,
    http2      = D_CURL_FEATURE_HTTP2,
    http3      = D_CURL_FEATURE_HTTP3,
    ipv6       = D_CURL_FEATURE_IPV6,
    libz       = D_CURL_FEATURE_LIBZ,
    brotli     = D_CURL_FEATURE_BROTLI,
    zstd       = D_CURL_FEATURE_ZSTD,
    asynch_dns = D_CURL_FEATURE_ASYNCH_DNS,
    threadsafe = D_CURL_FEATURE_THREADSAFE
};

// 3.2    Library queries
//------------------------------------------------------------------------------
// available
//   function: whether this build performs transfers over libcurl.
D_NODISCARD inline bool
available() noexcept
{
    return (::d_curl_version() != nullptr);
}

// version_string
//   function: the loaded libcurl's version text ("8.5.0"), or "" without
// libcurl.
D_NODISCARD inline const char*
version_string() noexcept
{
    const char* const text = ::d_curl_version();

    return (text != nullptr) ? text : "";
}

// version_number
//   function: the loaded libcurl's version as 0xMMmmpp, or 0 without it.
D_NODISCARD inline std::uint32_t
version_number() noexcept
{
    return ::d_curl_version_number();
}

// supports
//   function: whether the loaded libcurl reports _feature; false without
// libcurl, and for a feature the build's headers predate.
D_NODISCARD inline bool
supports(
    feature _feature
) noexcept
{
    return ::d_curl_supports(static_cast< ::d_curl_feature>(_feature));
}

// supports_ssl
//   function: whether the loaded libcurl speaks TLS.
D_NODISCARD inline bool
supports_ssl() noexcept
{
    return supports(feature::ssl);
}

// supports_http2
//   function: whether the loaded libcurl speaks HTTP/2.
D_NODISCARD inline bool
supports_http2() noexcept
{
    return supports(feature::http2);
}

// error_message
//   function: libcurl's text for one of its codes, as d_curl_result carries
// them; "" without libcurl.
D_NODISCARD inline const char*
error_message(
    int _code
) noexcept
{
    const char* const text = ::d_curl_code_message(_code);

    return (text != nullptr) ? text : "";
}

// 3.3    Global state
//------------------------------------------------------------------------------
// 3.3.1
// scoped_global
//   class: one initialization of libcurl's global state, held for the
// object's lifetime and balanced when it ends. Optional: ensure_global
// covers every transfer. Neither copyable nor movable.
class scoped_global
{
public:
    scoped_global() noexcept
        : m_status(::d_curl_global_init())
    {}

    ~scoped_global()
    {
        // only a successful initialization is balanced
        if (m_status == D_CURL_STATUS_OK)
        {
            ::d_curl_global_cleanup();
        }
    }

    scoped_global(const scoped_global&)            D_DELETE;
    scoped_global& operator=(const scoped_global&) D_DELETE;
    scoped_global(scoped_global&&)                 D_DELETE;
    scoped_global& operator=(scoped_global&&)      D_DELETE;

    // ok
    //   function: whether the initialization succeeded.
    D_NODISCARD bool
    ok() const noexcept
    {
        return (m_status == D_CURL_STATUS_OK);
    }

    // error
    //   function: why the initialization failed, or none.
    D_NODISCARD transport_error
    error() const noexcept
    {
        return to_transport_error(m_status);
    }

private:
    ::d_curl_status m_status;
};

// 3.3.2
// ensure_global
//   function: initializes libcurl's global state once per process, and
// reports whether it is ready. C++11 makes the first call's initialization
// thread-safe, which libcurl's own is not before 7.84. Every transfer calls
// it; the initialization is never balanced, which libcurl permits.
NS_INTERNAL

    // global_error
    //   function: the outcome of the one initialization, as a
    // transport_error.
    D_NODISCARD inline transport_error
    global_error() noexcept
    {
        static const transport_error s_error =
            to_transport_error(::d_curl_global_init());

        return s_error;
    }

NS_END  // internal

D_NODISCARD inline bool
ensure_global() noexcept
{
    return (internal::global_error() == transport_error::none);
}


//==============================================================================
// 4.  TRANSFERS
//==============================================================================
// A request goes out through the binding, and its response comes back: the
// status and fields into a web::response, the body into a sink or the
// response itself. Where redirects are followed, the fields of every
// response in the chain arrive, in order, and only the last one's body.


// 4.1    Sinks and options
//------------------------------------------------------------------------------
// 4.1.1
// body_sink
//   type: receives each piece of a response body as it arrives; returns
// false to end the transfer, which then fails as write_error. It may throw;
// see perform_stream.
using body_sink = std::function<bool(const char*, std::size_t)>;

// 4.1.2
// options
//   struct: how a transfer runs. The defaults are d_curl_options_init's,
// but for the User-Agent, which is web.hpp's.
//     timeout_ms          the whole transfer's limit; 0 or less for none.
//     connect_timeout_ms  the connection phase's limit; 0 or less for
//                         libcurl's.
//     follow_redirects    follow Location, up to max_redirects times.
//     verify_tls          verify the peer's certificate and name. Turning
//                         it off is insecure.
//     verbose             have libcurl describe the transfer on stderr.
//     accept_encoding     the codings to accept and decode; "" for every
//                         one libcurl supports.
//     user_agent          the User-Agent; "" for D_WEB_DEFAULT_USER_AGENT.
//     proxy               the proxy URL; "" for libcurl's default, which
//                         reads the environment's proxy variables.
//     bypass_proxy        use no proxy at all, whatever proxy and the
//                         environment say.
struct options
{
    options()
        : timeout_ms(0),
          connect_timeout_ms(0),
          follow_redirects(true),
          max_redirects(D_CURL_MAX_REDIRECTS_DEFAULT),
          verify_tls(true),
          verbose(false),
          accept_encoding(),
          user_agent(D_WEB_DEFAULT_USER_AGENT),
          proxy(),
          bypass_proxy(false)
    {}

    long        timeout_ms;
    long        connect_timeout_ms;
    bool        follow_redirects;
    long        max_redirects;
    bool        verify_tls;
    bool        verbose;
    std::string accept_encoding;
    std::string user_agent;
    std::string proxy;
    bool        bypass_proxy;
};

// 4.2    The bridge to C
//------------------------------------------------------------------------------
// How a request becomes the binding's records, which borrow from the
// request, the options, and a field list built beside them, all outliving
// the transfer; and how the binding's callbacks reach C++ again.
//
// 4.2.1
// stream_state
//   struct: what the binding's callbacks reach through their context: the
// caller's sink, the header list being filled, and the exception a callback
// caught, if any.
NS_INTERNAL

    struct stream_state
    {
        const body_sink*   sink;
        header_list*       headers;
        std::exception_ptr error;
    };

#if (D_ENV_CPP98_HAS_EXCEPTION == 1)

    // guarded_body
    //   function: hands one body piece to the caller's sink, catching what
    // it throws: the exception waits in _state, and the transfer ends.
    inline bool
    guarded_body(
        stream_state& _state,
        const char*   _data,
        std::size_t   _size
    ) noexcept
    {
        try
        {
            return (*_state.sink)(_data,
                                  _size);
        }
        catch (...)
        {
            _state.error = std::current_exception();

            return false;
        }
    }

    // guarded_field
    //   function: appends one response field to the header list, catching
    // what the allocation throws, as guarded_body does.
    inline bool
    guarded_field(
        stream_state&      _state,
        const d_pack_text& _name,
        const d_pack_text& _value
    ) noexcept
    {
        try
        {
            _state.headers->push_back(
                header_field(std::string(_name.data,
                                         _name.length),
                             std::string(_value.data,
                                         _value.length)));

            return true;
        }
        catch (...)
        {
            _state.error = std::current_exception();

            return false;
        }
    }

    // rethrow_caught
    //   function: resumes the exception a callback caught, now that libcurl
    // is off the stack.
    inline void
    rethrow_caught(
        const stream_state& _state
    )
    {
        // a callback caught one
        if (_state.error)
        {
            std::rethrow_exception(_state.error);
        }

        return;
    }

#else

    // guarded_body
    //   function: hands one body piece to the caller's sink; without
    // exceptions there is nothing to guard against.
    inline bool
    guarded_body(
        stream_state& _state,
        const char*   _data,
        std::size_t   _size
    ) noexcept
    {
        return (*_state.sink)(_data,
                              _size);
    }

    // guarded_field
    //   function: appends one response field to the header list.
    inline bool
    guarded_field(
        stream_state&      _state,
        const d_pack_text& _name,
        const d_pack_text& _value
    ) noexcept
    {
        _state.headers->push_back(
            header_field(std::string(_name.data,
                                     _name.length),
                         std::string(_value.data,
                                     _value.length)));

        return true;
    }

    // rethrow_caught
    //   function: without exceptions, nothing was caught.
    inline void
    rethrow_caught(
        const stream_state& _state
    ) noexcept
    {
        (void)_state;

        return;
    }

#endif  // D_ENV_CPP98_HAS_EXCEPTION

    D_EXTERN_C_BEGIN

    // d_internal_curl_hpp_on_body
    //   function: the binding's body callback. C linkage, as the binding's
    // pointer type requires.
    inline bool
    d_internal_curl_hpp_on_body(
        void*       _state,
        const void* _data,
        std::size_t _size
    ) noexcept
    {
        return guarded_body(*static_cast<stream_state*>(_state),
                            static_cast<const char*>(_data),
                            _size);
    }

    // d_internal_curl_hpp_on_field
    //   function: the binding's header callback. C linkage, as the
    // binding's pointer type requires.
    inline bool
    d_internal_curl_hpp_on_field(
        void*       _state,
        d_pack_text _name,
        d_pack_text _value
    ) noexcept
    {
        return guarded_field(*static_cast<stream_state*>(_state),
                             _name,
                             _value);
    }

    D_EXTERN_C_END

    // c_fields
    //   function: the request's header fields as the binding's records,
    // each borrowing its strings.
    inline std::vector< ::d_curl_header>
    c_fields(
        const header_list& _headers
    )
    {
        std::vector< ::d_curl_header> fields;

        fields.reserve(_headers.size());

        // each field in order
        for (const header_field& field : _headers)
        {
            const ::d_curl_header entry =
            {
                { field.first.data(), field.first.size() },
                { field.second.data(), field.second.size() }
            };

            fields.push_back(entry);
        }

        return fields;
    }

    // c_request
    //   function: the request as the binding's record, borrowing from
    // _request and _fields.
    D_NODISCARD inline ::d_curl_request
    c_request(
        const request&                       _request,
        const std::vector< ::d_curl_header>& _fields
    ) noexcept
    {
        const ::d_curl_request record =
        {
            to_string(_request.method),
            _request.url.c_str(),
            _fields.data(),
            _fields.size(),
            { _request.body.data(), _request.body.size() }
        };

        return record;
    }

    // c_options
    //   function: the options as the binding's record, borrowing their
    // strings. A timeout of 0 or less reads as none, as it always has here.
    D_NODISCARD inline ::d_curl_options
    c_options(
        const options& _options
    ) noexcept
    {
        ::d_curl_options settings = ::d_curl_options();

        ::d_curl_options_init(&settings);
        settings.timeout_ms         = (_options.timeout_ms > 0)
                                          ? _options.timeout_ms
                                          : 0;
        settings.connect_timeout_ms = (_options.connect_timeout_ms > 0)
                                          ? _options.connect_timeout_ms
                                          : 0;
        settings.follow_redirects   = _options.follow_redirects;
        settings.max_redirects      = _options.max_redirects;
        settings.verify_tls         = _options.verify_tls;
        settings.verbose            = _options.verbose;
        settings.accept_encoding    = _options.accept_encoding.c_str();
        settings.user_agent         = (_options.user_agent.empty())
                                          ? D_WEB_DEFAULT_USER_AGENT
                                          : _options.user_agent.c_str();
        settings.proxy              = (_options.proxy.empty())
                                          ? nullptr
                                          : _options.proxy.c_str();

        // no proxy at all, whatever the environment says
        if (_options.bypass_proxy)
        {
            settings.proxy = "";
        }

        return settings;
    }

    // c_sink
    //   function: the binding's sink for a stream: the header callback
    // always, the body callback only for a sink that can take a body.
    D_NODISCARD inline ::d_curl_sink
    c_sink(
        stream_state& _state
    ) noexcept
    {
        const ::d_curl_sink sink =
        {
            (*_state.sink) ? d_internal_curl_hpp_on_body : nullptr,
            d_internal_curl_hpp_on_field,
            &_state
        };

        return sink;
    }

NS_END  // internal

// 4.3    Drivers
//------------------------------------------------------------------------------
/**
 * @brief Performs a request, streaming its body to `_sink` as it arrives.
 *
 * @note `_meta` receives the status, the header fields of every response in
 *       a redirect chain, and the error; the body goes to the sink alone.
 *       The status is set only for a transfer that completed.
 *
 * @param[in]  _request  the request; borrowed for the call.
 * @param[in]  _sink     receives the body; an empty sink discards it.
 * @param[out] _meta     reset, then filled as described above.
 * @param[in]  _options  how the transfer runs.
 * @return the outcome, as stored in `_meta.error`: none once a response
 *         arrived, whatever its HTTP status; unsupported_protocol without
 *         libcurl; unknown for a header field the binding refuses -- a
 *         colon in its name, or a line break or NUL in either part.
 * @throws whatever `_sink` threw, rethrown once the transfer has ended and
 *         been cleaned up; and std::bad_alloc.
 */
D_NODISCARD inline transport_error
perform_stream(
    const request&   _request,
    const body_sink& _sink,
    response&        _meta,
    const options&   _options = options()
)
{
    _meta = response();

    const transport_error global = internal::global_error();

    // libcurl's global state comes first, once per process
    if (global != transport_error::none)
    {
        _meta.error = global;

        return _meta.error;
    }

    const std::vector< ::d_curl_header> fields    =
        internal::c_fields(_request.headers);
    const ::d_curl_request              c_request =
        internal::c_request(_request,
                            fields);
    const ::d_curl_options              c_options =
        internal::c_options(_options);
    internal::stream_state              state     =
        { &_sink, &_meta.headers, std::exception_ptr() };
    const ::d_curl_sink                 c_sink    =
        internal::c_sink(state);
    ::d_curl_result                     result    =
        { D_CURL_STATUS_OK, 0, 0 };

    (void)::d_curl_perform(&c_request,
                           &c_options,
                           &c_sink,
                           &result);
    internal::rethrow_caught(state);
    _meta.error = to_transport_error(result.status);

    // the status belongs to a completed transfer only
    if (_meta.error == transport_error::none)
    {
        _meta.status = static_cast<int>(result.http_status);
    }

    return _meta.error;
}

/**
 * @brief Performs a request, collecting the whole response.
 *
 * @param[in] _request  the request; borrowed for the call.
 * @param[in] _options  how the transfer runs.
 * @return the response: status, header fields, and error as
 *         perform_stream fills them, and the body when error is none.
 * @throws whatever perform_stream throws.
 */
D_NODISCARD inline response
perform(
    const request& _request,
    const options& _options = options()
)
{
    response        result;
    std::string     body;
    const body_sink sink =
        [&body](const char* _data,
                std::size_t _length) -> bool
        {
            body.append(_data,
                        _length);

            return true;
        };
    const transport_error error = perform_stream(_request,
                                                 sink,
                                                 result,
                                                 _options);

    // the body belongs to a completed transfer only
    if (error == transport_error::none)
    {
        result.body.swap(body);
    }

    return result;
}


NS_END  // curl
NS_END  // web
NS_END  // djinterp


#endif  // DJINTERP_NET_CURL_CURL_HPP
