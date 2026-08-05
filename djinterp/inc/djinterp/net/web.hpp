/******************************************************************************
* djinterp [web]                                                       web.hpp
*
*   Foundational, library-agnostic web module for the djinterp framework. It
* is the backbone every web backend (libcurl, and future transports) builds
* on: it owns the shared HTTP vocabulary and the neutral request/response
* types, and depends on NO HTTP client library.
*
* CONTENTS (all in namespace djinterp::web):
*   I.    namespace + version macros
*   II.   byte / buffer aliases
*   III.  enumerations
*         - http_method, http_version, url_scheme
*         - http_status + status_category
*         - transport_error (library-neutral transport failures)
*   IV.   enumeration helpers (to_string / from_string / classifiers /
*         reason_phrase / default_port / is_secure)
*   V.    header + query vocabulary, ASCII case-insensitive helpers
*   VI.   percent-encoding + query-string build/parse
*   VII.  constant tables (content_type::*, header_name::*)
*   VIII. neutral request / response aggregates + basic URL split
*
*   Everything here is header-only; the small classifiers are constexpr, the
* switch-driven mappers are constexpr on C++14+ (relaxed constexpr) and plain
* inline on C++11, and the string-building utilities are ordinary inline
* functions. Requires C++11 or later.
*
* path:      /inc/djinterp/web/web.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.16
******************************************************************************/

#ifndef DJINTERP_WEB_
#define DJINTERP_WEB_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/env/web/env_web.h"
#include "../core/container/buffer/byte_buffer.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "web.hpp requires C++11 or later"
#endif


///////////////////////////////////////////////////////////////////////////////
///                 I.   NAMESPACE + VERSION MACROS                        ///
///////////////////////////////////////////////////////////////////////////////

// D_KEYWORD_WEB
//   keyword: resolves to `web`. Used for variables, macros, and namespaces
// pertaining to the web subframework. Guarded so the core may adopt it later
// without colliding with this definition.
#ifndef D_KEYWORD_WEB
    #define D_KEYWORD_WEB               web
#endif

// NS_WEB
//   namespace: the `web` namespace, the root of all web-related functionality.
// Nest inside NS_DJINTERP; close with NS_END.
#ifndef NS_WEB
    #define NS_WEB                      D_NAMESPACE(D_KEYWORD_WEB)
#endif

// D_WEB_VERSION_MAJOR / _MINOR / _PATCH
//   macro: semantic version of the web subframework itself.
#define D_WEB_VERSION_MAJOR         0
#define D_WEB_VERSION_MINOR         1
#define D_WEB_VERSION_PATCH         0

// D_WEB_VERSION_STRING
//   macro: dotted version string of the web subframework.
#define D_WEB_VERSION_STRING        "0.1.0"

// D_WEB_DEFAULT_USER_AGENT
//   macro: default User-Agent advertised by djinterp web clients.
#ifndef D_WEB_DEFAULT_USER_AGENT
    #define D_WEB_DEFAULT_USER_AGENT    "djinterp-web/" D_WEB_VERSION_STRING
#endif

// D_WEB_CONSTEXPR14
//   macro (internal): `constexpr` where relaxed (statement-body) constexpr is
// available (C++14+), otherwise empty so the same function is a plain inline on
// C++11. Distinct from D_CONSTEXPR, which is used for C++11-valid single-return
// constexpr functions below.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_WEB_CONSTEXPR14           D_CONSTEXPR
#else
    #define D_WEB_CONSTEXPR14
#endif


NS_DJINTERP
NS_WEB


///////////////////////////////////////////////////////////////////////////////
///                    II.   BYTE / BUFFER ALIASES                         ///
///////////////////////////////////////////////////////////////////////////////

// byte
//   type: a single octet of wire data.
using byte = unsigned char;

// byte_buffer
//   type: the framework's growable byte buffer (core/container/buffer) -- the
// same encode/decode-capable accumulator the net layer carries payloads in.
using byte_buffer = ::djinterp::byte_buffer<>;

// bytes
//   type: convenience alias for byte_buffer.
using bytes = byte_buffer;

// port_type
//   type: a TCP/UDP port number.
using port_type = std::uint16_t;


///////////////////////////////////////////////////////////////////////////////
///                       III.   ENUMERATIONS                              ///
///////////////////////////////////////////////////////////////////////////////

// http_method
//   enum: the HTTP request methods. `delete_` carries a trailing underscore to
// avoid the `delete` keyword (and the Win32 DELETE macro); the rest are the
// verbs' lowercase spellings.
enum class http_method : unsigned char
{
    get,
    head,
    post,
    put,
    delete_,
    patch,
    options,
    trace,
    connect
};

// http_version
//   enum: the HTTP wire-protocol versions a request may pin or a response may
// report. `unknown` is the zero value / unset sentinel.
enum class http_version : unsigned char
{
    unknown = 0,
    http_1_0,
    http_1_1,
    http_2,
    http_3
};

// url_scheme
//   enum: the URI schemes this vocabulary recognizes. `unknown` is the zero
// value for an unrecognized or absent scheme.
enum class url_scheme : unsigned char
{
    unknown = 0,
    http,
    https,
    ws,
    wss,
    ftp,
    ftps,
    file,
    data
};

// http_status
//   enum: the commonly used HTTP status codes. Not exhaustive -- any integer
// is a valid status on the wire, so the classifier helpers below take a plain
// int and this enum is a convenience for the well-known values.
enum class http_status : int
{
    // 1xx informational
    continue_             = 100,
    switching_protocols   = 101,
    // 2xx success
    ok                    = 200,
    created               = 201,
    accepted              = 202,
    no_content            = 204,
    partial_content       = 206,
    // 3xx redirection
    moved_permanently     = 301,
    found                 = 302,
    see_other             = 303,
    not_modified          = 304,
    temporary_redirect    = 307,
    permanent_redirect    = 308,
    // 4xx client error
    bad_request           = 400,
    unauthorized          = 401,
    forbidden             = 403,
    not_found             = 404,
    method_not_allowed    = 405,
    not_acceptable        = 406,
    request_timeout       = 408,
    conflict              = 409,
    gone                  = 410,
    length_required       = 411,
    payload_too_large     = 413,
    uri_too_long          = 414,
    unsupported_media_type = 415,
    im_a_teapot           = 418,
    unprocessable_entity  = 422,
    too_many_requests     = 429,
    // 5xx server error
    internal_server_error = 500,
    not_implemented       = 501,
    bad_gateway           = 502,
    service_unavailable   = 503,
    gateway_timeout       = 504,
    http_version_not_supported = 505
};

// status_category
//   enum: the class an HTTP status code falls into, keyed on its leading
// digit. `unknown` covers codes outside the 100-599 range.
enum class status_category : unsigned char
{
    unknown = 0,
    informational,
    success,
    redirection,
    client_error,
    server_error
};

// transport_error
//   enum: a library-neutral classification of transport-level failures, onto
// which each backend maps its native error codes so higher layers can reason
// about failures without depending on any specific client library.
enum class transport_error : unsigned char
{
    none = 0,
    unsupported_protocol,
    could_not_resolve_host,
    could_not_connect,
    timed_out,
    tls_error,
    too_many_redirects,
    write_error,
    read_error,
    canceled,
    out_of_memory,
    unknown
};


///////////////////////////////////////////////////////////////////////////////
///                    IV.   ENUMERATION HELPERS                           ///
///////////////////////////////////////////////////////////////////////////////

// to_string(http_method)
//   function: canonical uppercase wire token for a method (e.g. "GET").
D_NODISCARD D_WEB_CONSTEXPR14 const char*
to_string(
    http_method _method
)
{
    switch (_method)
    {
        case http_method::get:     return "GET";
        case http_method::head:    return "HEAD";
        case http_method::post:    return "POST";
        case http_method::put:     return "PUT";
        case http_method::delete_: return "DELETE";
        case http_method::patch:   return "PATCH";
        case http_method::options: return "OPTIONS";
        case http_method::trace:   return "TRACE";
        case http_method::connect: return "CONNECT";
    }

    return "";
}

// to_string(http_version)
//   function: the HTTP version token (e.g. "HTTP/1.1"); "" when unknown.
D_NODISCARD D_WEB_CONSTEXPR14 const char*
to_string(
    http_version _version
)
{
    switch (_version)
    {
        case http_version::http_1_0: return "HTTP/1.0";
        case http_version::http_1_1: return "HTTP/1.1";
        case http_version::http_2:   return "HTTP/2";
        case http_version::http_3:   return "HTTP/3";
        case http_version::unknown:  return "";
    }

    return "";
}

// to_string(url_scheme)
//   function: the lowercase scheme token (e.g. "https"); "" when unknown.
D_NODISCARD D_WEB_CONSTEXPR14 const char*
to_string(
    url_scheme _scheme
)
{
    switch (_scheme)
    {
        case url_scheme::http:  return "http";
        case url_scheme::https: return "https";
        case url_scheme::ws:    return "ws";
        case url_scheme::wss:   return "wss";
        case url_scheme::ftp:   return "ftp";
        case url_scheme::ftps:  return "ftps";
        case url_scheme::file:  return "file";
        case url_scheme::data:  return "data";
        case url_scheme::unknown: return "";
    }

    return "";
}

// to_string(transport_error)
//   function: a short human-readable label for a transport error.
D_NODISCARD D_WEB_CONSTEXPR14 const char*
to_string(
    transport_error _error
)
{
    switch (_error)
    {
        case transport_error::none:                 return "none";
        case transport_error::unsupported_protocol: return "unsupported protocol";
        case transport_error::could_not_resolve_host: return "could not resolve host";
        case transport_error::could_not_connect:    return "could not connect";
        case transport_error::timed_out:            return "timed out";
        case transport_error::tls_error:            return "TLS error";
        case transport_error::too_many_redirects:   return "too many redirects";
        case transport_error::write_error:          return "write error";
        case transport_error::read_error:           return "read error";
        case transport_error::canceled:             return "canceled";
        case transport_error::out_of_memory:        return "out of memory";
        case transport_error::unknown:              return "unknown error";
    }

    return "unknown error";
}

// is_informational
//   function: 1xx -- the request was received, continuing process.
D_NODISCARD D_CONSTEXPR bool
is_informational(
    int _code
)
{
    return (_code >= 100) && (_code < 200);
}

// is_success
//   function: 2xx -- the request was successfully received and accepted.
D_NODISCARD D_CONSTEXPR bool
is_success(
    int _code
)
{
    return (_code >= 200) && (_code < 300);
}

// is_redirection
//   function: 3xx -- further action is needed to complete the request.
D_NODISCARD D_CONSTEXPR bool
is_redirection(
    int _code
)
{
    return (_code >= 300) && (_code < 400);
}

// is_client_error
//   function: 4xx -- the request contains bad syntax or cannot be fulfilled.
D_NODISCARD D_CONSTEXPR bool
is_client_error(
    int _code
)
{
    return (_code >= 400) && (_code < 500);
}

// is_server_error
//   function: 5xx -- the server failed to fulfill a valid request.
D_NODISCARD D_CONSTEXPR bool
is_server_error(
    int _code
)
{
    return (_code >= 500) && (_code < 600);
}

// is_error
//   function: any client or server error (>= 400).
D_NODISCARD D_CONSTEXPR bool
is_error(
    int _code
)
{
    return (_code >= 400) && (_code < 600);
}

// classify
//   function: the status_category a status code falls into.
D_NODISCARD D_WEB_CONSTEXPR14 status_category
classify(
    int _code
)
{
    if (is_informational(_code))
    {
        return status_category::informational;
    }

    if (is_success(_code))
    {
        return status_category::success;
    }

    if (is_redirection(_code))
    {
        return status_category::redirection;
    }

    if (is_client_error(_code))
    {
        return status_category::client_error;
    }

    if (is_server_error(_code))
    {
        return status_category::server_error;
    }

    return status_category::unknown;
}

// reason_phrase
//   function: the standard reason phrase for a status code (e.g. "Not Found"
// for 404); "" for a code without a registered phrase here.
D_NODISCARD D_WEB_CONSTEXPR14 const char*
reason_phrase(
    int _code
)
{
    switch (_code)
    {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 206: return "Partial Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 418: return "I'm a Teapot";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "";
    }
}

// reason_phrase(http_status)
//   function: reason-phrase overload for the enumerated status type.
D_NODISCARD D_WEB_CONSTEXPR14 const char*
reason_phrase(
    http_status _status
)
{
    return reason_phrase(static_cast<int>(_status));
}

// is_secure
//   function: whether a scheme implies transport security (TLS).
D_NODISCARD D_WEB_CONSTEXPR14 bool
is_secure(
    url_scheme _scheme
)
{
    return ( (_scheme == url_scheme::https) ||
             (_scheme == url_scheme::wss)   ||
             (_scheme == url_scheme::ftps) );
}

// default_port
//   function: the well-known default port for a scheme, or 0 when the scheme
// has no network port (e.g. file/data) or is unknown.
D_NODISCARD D_WEB_CONSTEXPR14 port_type
default_port(
    url_scheme _scheme
)
{
    switch (_scheme)
    {
        case url_scheme::http:  return 80;
        case url_scheme::https: return 443;
        case url_scheme::ws:    return 80;
        case url_scheme::wss:   return 443;
        case url_scheme::ftp:   return 21;
        case url_scheme::ftps:  return 990;
        case url_scheme::file:  return 0;
        case url_scheme::data:  return 0;
        case url_scheme::unknown: return 0;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///              V.   HEADER / QUERY VOCABULARY + ASCII HELPERS            ///
///////////////////////////////////////////////////////////////////////////////

// header_field
//   type: a single HTTP header as an ordered (name, value) pair. Field names
// are case-insensitive on the wire; the lookup helpers below honor that.
using header_field = std::pair<std::string, std::string>;

// header_list
//   type: an ordered collection of header fields (duplicates permitted, as the
// protocol allows).
using header_list = std::vector<header_field>;

// headers
//   type: convenience alias for header_list.
using headers = header_list;

// query_param
//   type: a single URL query parameter as a (key, value) pair (values as yet
// undecoded/decoded per the caller).
using query_param = std::pair<std::string, std::string>;

// query_list
//   type: an ordered collection of query parameters.
using query_list = std::vector<query_param>;

// ascii_to_lower
//   function: lowercases a single ASCII byte; non-ASCII-letters pass through.
// Locale-independent by construction (the wire is ASCII).
D_NODISCARD D_CONSTEXPR char
ascii_to_lower(
    char _c
)
{
    return ( (_c >= 'A') && (_c <= 'Z') )
           ? static_cast<char>(_c - 'A' + 'a')
           : _c;
}

// iequals
//   function: ASCII case-insensitive string equality (header-name comparison).
D_NODISCARD inline bool
iequals(
    const std::string& _a,
    const std::string& _b
)
{
    // fast reject on length mismatch
    if (_a.size() != _b.size())
    {
        return false;
    }

    // compare byte-by-byte, folding ASCII case
    for (std::size_t i = 0; i < _a.size(); ++i)
    {
        if (ascii_to_lower(_a[i]) != ascii_to_lower(_b[i]))
        {
            return false;
        }
    }

    return true;
}

// find_header
//   function: pointer to the first field whose name matches `_name`
// case-insensitively, or nullptr when absent. Const overload.
D_NODISCARD inline const header_field*
find_header(
    const header_list& _headers,
    const std::string& _name
)
{
    // linear scan honoring case-insensitive field names
    for (const header_field& field : _headers)
    {
        if (iequals(field.first, _name))
        {
            return &field;
        }
    }

    return nullptr;
}

// has_header
//   function: whether a field named `_name` (case-insensitive) is present.
D_NODISCARD inline bool
has_header(
    const header_list& _headers,
    const std::string& _name
)
{
    return (find_header(_headers, _name) != nullptr);
}

// header_value
//   function: the value of the first field named `_name` (case-insensitive),
// or `_fallback` when the field is absent.
D_NODISCARD inline std::string
header_value(
    const header_list& _headers,
    const std::string& _name,
    const std::string& _fallback = std::string()
)
{
    const header_field* field = find_header(_headers, _name);

    return field ? field->second : _fallback;
}

// set_header
//   function: sets `_name` to `_value`, replacing the first existing
// case-insensitive match in place or appending a new field if none exists.
inline void
set_header(
    header_list&       _headers,
    const std::string& _name,
    const std::string& _value
)
{
    // replace the first case-insensitive match, if any
    for (header_field& field : _headers)
    {
        if (iequals(field.first, _name))
        {
            field.second = _value;

            return;
        }
    }

    // no existing field -- append
    _headers.push_back(header_field(_name, _value));

    return;
}


///////////////////////////////////////////////////////////////////////////////
///          VI.   PERCENT-ENCODING + QUERY-STRING BUILD / PARSE           ///
///////////////////////////////////////////////////////////////////////////////

// hex_digit
//   function: the uppercase hex digit for a nibble value 0-15 ('0'-'9',
// 'A'-'F'); '0' for out-of-range input.
D_NODISCARD D_CONSTEXPR char
hex_digit(
    int _nibble
)
{
    return ( (_nibble >= 0) && (_nibble <= 9) )
           ? static_cast<char>('0' + _nibble)
           : ( (_nibble >= 10) && (_nibble <= 15) )
             ? static_cast<char>('A' + (_nibble - 10))
             : '0';
}

// hex_value
//   function: the numeric value 0-15 of a hex digit character, or -1 if the
// character is not a hex digit.
D_NODISCARD D_CONSTEXPR int
hex_value(
    char _c
)
{
    return ( (_c >= '0') && (_c <= '9') ) ? (_c - '0')
         : ( (_c >= 'a') && (_c <= 'f') ) ? (_c - 'a' + 10)
         : ( (_c >= 'A') && (_c <= 'F') ) ? (_c - 'A' + 10)
         : -1;
}

// is_unreserved
//   function: whether a byte is an RFC 3986 "unreserved" character
// (ALPHA / DIGIT / '-' / '.' / '_' / '~') and thus never percent-encoded.
D_NODISCARD D_CONSTEXPR bool
is_unreserved(
    char _c
)
{
    return ( ( (_c >= 'A') && (_c <= 'Z') ) ||
             ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= '0') && (_c <= '9') ) ||
             (_c == '-') || (_c == '.')     ||
             (_c == '_') || (_c == '~') );
}

// percent_encode
//   function: percent-encodes `_input` per RFC 3986, escaping every byte that
// is not "unreserved" as %XX (uppercase). Suitable for a single URL component
// (path segment, query key/value).
D_NODISCARD inline std::string
percent_encode(
    const std::string& _input
)
{
    std::string result;
    result.reserve(_input.size());

    // escape any byte outside the unreserved set
    for (std::size_t i = 0; i < _input.size(); ++i)
    {
        const char c = _input[i];

        if (is_unreserved(c))
        {
            result.push_back(c);
        }
        else
        {
            const unsigned char u = static_cast<unsigned char>(c);
            result.push_back('%');
            result.push_back(hex_digit((u >> 4) & 0x0F));
            result.push_back(hex_digit(u & 0x0F));
        }
    }

    return result;
}

// percent_decode
//   function: reverses percent-encoding, turning each valid %XX into its byte.
// A '+' becomes a space only when `_plus_as_space` is true (the
// application/x-www-form-urlencoded convention). Malformed escapes are copied
// through verbatim.
D_NODISCARD inline std::string
percent_decode(
    const std::string& _input,
    bool               _plus_as_space = false
)
{
    std::string result;
    result.reserve(_input.size());

    std::size_t i = 0;

    // walk the input, decoding %XX escapes
    while (i < _input.size())
    {
        const char c = _input[i];

        if ( (c == '%') && ((i + 2) < _input.size()) )
        {
            const int hi = hex_value(_input[i + 1]);
            const int lo = hex_value(_input[i + 2]);

            if ( (hi >= 0) && (lo >= 0) )
            {
                result.push_back(static_cast<char>((hi << 4) | lo));
                i += 3;

                continue;
            }
        }

        if (_plus_as_space && (c == '+'))
        {
            result.push_back(' ');
        }
        else
        {
            result.push_back(c);
        }

        ++i;
    }

    return result;
}

// encode_query
//   function: serializes query parameters into an encoded query string
// ("k1=v1&k2=v2"), percent-encoding each key and value. No leading '?'.
D_NODISCARD inline std::string
encode_query(
    const query_list& _params
)
{
    std::string result;

    // join encoded key=value pairs with '&'
    for (std::size_t i = 0; i < _params.size(); ++i)
    {
        if (i != 0)
        {
            result.push_back('&');
        }

        result += percent_encode(_params[i].first);
        result.push_back('=');
        result += percent_encode(_params[i].second);
    }

    return result;
}

// parse_query
//   function: splits an encoded query string (with or without a leading '?')
// into decoded key/value pairs. A parameter with no '=' yields an empty value;
// '+' is decoded as space (form convention).
D_NODISCARD inline query_list
parse_query(
    const std::string& _query
)
{
    query_list result;

    std::size_t start = 0;

    // skip an optional leading '?'
    if ( (!_query.empty()) && (_query[0] == '?') )
    {
        start = 1;
    }

    // split on '&', then each token on its first '='
    while (start <= _query.size())
    {
        std::size_t amp = _query.find('&', start);

        if (amp == std::string::npos)
        {
            amp = _query.size();
        }

        if (amp > start)
        {
            const std::string token = _query.substr(start, amp - start);
            const std::size_t eq    = token.find('=');

            if (eq == std::string::npos)
            {
                result.push_back(
                    query_param(percent_decode(token, true), std::string()));
            }
            else
            {
                result.push_back(query_param(
                    percent_decode(token.substr(0, eq), true),
                    percent_decode(token.substr(eq + 1), true)));
            }
        }

        start = amp + 1;
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///                     VII.   CONSTANT TABLES                             ///
///////////////////////////////////////////////////////////////////////////////

// content_type
//   namespace: canonical MIME type strings for common HTTP payloads.
D_NAMESPACE(content_type)

    // application/json
    D_CONSTEXPR const char* const json               = "application/json";
    // application/octet-stream
    D_CONSTEXPR const char* const octet_stream       = "application/octet-stream";
    // application/x-www-form-urlencoded
    D_CONSTEXPR const char* const form_urlencoded    = "application/x-www-form-urlencoded";
    // multipart/form-data
    D_CONSTEXPR const char* const multipart_form_data = "multipart/form-data";
    // application/xml
    D_CONSTEXPR const char* const xml                = "application/xml";
    // text/plain
    D_CONSTEXPR const char* const text_plain         = "text/plain";
    // text/html
    D_CONSTEXPR const char* const text_html          = "text/html";
    // text/event-stream (server-sent events)
    D_CONSTEXPR const char* const event_stream       = "text/event-stream";

NS_END  // content_type

// header_name
//   namespace: canonical spellings of frequently used HTTP header names.
D_NAMESPACE(header_name)

    D_CONSTEXPR const char* const accept             = "Accept";
    D_CONSTEXPR const char* const accept_encoding    = "Accept-Encoding";
    D_CONSTEXPR const char* const authorization      = "Authorization";
    D_CONSTEXPR const char* const connection         = "Connection";
    D_CONSTEXPR const char* const content_length     = "Content-Length";
    D_CONSTEXPR const char* const content_type       = "Content-Type";
    D_CONSTEXPR const char* const host               = "Host";
    D_CONSTEXPR const char* const location           = "Location";
    D_CONSTEXPR const char* const user_agent         = "User-Agent";

NS_END  // header_name


///////////////////////////////////////////////////////////////////////////////
///            VIII.   NEUTRAL REQUEST / RESPONSE + URL SPLIT              ///
///////////////////////////////////////////////////////////////////////////////

// url_parts
//   struct: the components of an absolute URL, as produced by split_url. All
// fields are raw (still percent-encoded where the source was); `port` is 0
// when the URL carried no explicit port.
struct url_parts
{
    std::string scheme;
    std::string host;
    port_type   port;
    std::string path;
    std::string query;
    std::string fragment;

    url_parts()
        : scheme(),
          host(),
          port(0),
          path(),
          query(),
          fragment()
    {
    }
};

// scheme_from_string
//   function: maps a (case-insensitive) scheme token to a url_scheme, or
// url_scheme::unknown when unrecognized.
D_NODISCARD inline url_scheme
scheme_from_string(
    const std::string& _scheme
)
{
    if (iequals(_scheme, "http"))  { return url_scheme::http;  }
    if (iequals(_scheme, "https")) { return url_scheme::https; }
    if (iequals(_scheme, "ws"))    { return url_scheme::ws;    }
    if (iequals(_scheme, "wss"))   { return url_scheme::wss;   }
    if (iequals(_scheme, "ftp"))   { return url_scheme::ftp;   }
    if (iequals(_scheme, "ftps"))  { return url_scheme::ftps;  }
    if (iequals(_scheme, "file"))  { return url_scheme::file;  }
    if (iequals(_scheme, "data"))  { return url_scheme::data;  }

    return url_scheme::unknown;
}

// method_from_string
//   function: maps a (case-insensitive) method token to an http_method,
// writing the result to `_out` and returning true on success; false (and
// `_out` unchanged) for an unrecognized token.
D_NODISCARD inline bool
method_from_string(
    const std::string& _token,
    http_method&       _out
)
{
    if (iequals(_token, "GET"))     { _out = http_method::get;     return true; }
    if (iequals(_token, "HEAD"))    { _out = http_method::head;    return true; }
    if (iequals(_token, "POST"))    { _out = http_method::post;    return true; }
    if (iequals(_token, "PUT"))     { _out = http_method::put;     return true; }
    if (iequals(_token, "DELETE"))  { _out = http_method::delete_; return true; }
    if (iequals(_token, "PATCH"))   { _out = http_method::patch;   return true; }
    if (iequals(_token, "OPTIONS")) { _out = http_method::options; return true; }
    if (iequals(_token, "TRACE"))   { _out = http_method::trace;   return true; }
    if (iequals(_token, "CONNECT")) { _out = http_method::connect; return true; }

    return false;
}

// split_url
//   function: a best-effort split of an absolute URL of the common shape
//   scheme "://" [host [":" port]] [path] ["?" query] ["#" fragment]
// into url_parts. This is a pragmatic splitter, not a full RFC 3986 parser: it
// does not perform userinfo handling, IPv6-literal bracket parsing, or
// normalization. Returns true when a "scheme://" prefix was found.
D_NODISCARD inline bool
split_url(
    const std::string& _url,
    url_parts&         _out
)
{
    const std::string sep = "://";
    const std::size_t scheme_end = _url.find(sep);

    // require an absolute URL with a scheme
    if (scheme_end == std::string::npos)
    {
        return false;
    }

    _out = url_parts();
    _out.scheme = _url.substr(0, scheme_end);

    const std::size_t authority_start = scheme_end + sep.size();

    // the authority runs until the first '/', '?' or '#'
    std::size_t authority_end = _url.size();

    for (std::size_t i = authority_start; i < _url.size(); ++i)
    {
        const char c = _url[i];

        if ( (c == '/') || (c == '?') || (c == '#') )
        {
            authority_end = i;

            break;
        }
    }

    const std::string authority =
        _url.substr(authority_start, authority_end - authority_start);

    // split authority into host and optional ":port"
    const std::size_t colon = authority.rfind(':');

    if (colon == std::string::npos)
    {
        _out.host = authority;
    }
    else
    {
        _out.host = authority.substr(0, colon);

        // parse the port digits (ignore malformed trailing content)
        const std::string port_str = authority.substr(colon + 1);
        unsigned long     port_val = 0;
        bool              any      = false;

        for (std::size_t i = 0; i < port_str.size(); ++i)
        {
            const char c = port_str[i];

            if ( (c < '0') || (c > '9') )
            {
                break;
            }

            port_val = (port_val * 10) + static_cast<unsigned long>(c - '0');
            any      = true;
        }

        if (any && (port_val <= 0xFFFF))
        {
            _out.port = static_cast<port_type>(port_val);
        }
    }

    // remainder = path ["?" query] ["#" fragment]
    std::string remainder = _url.substr(authority_end);

    // peel off the fragment first (it is last on the wire)
    const std::size_t hash = remainder.find('#');

    if (hash != std::string::npos)
    {
        _out.fragment = remainder.substr(hash + 1);
        remainder     = remainder.substr(0, hash);
    }

    // then the query
    const std::size_t question = remainder.find('?');

    if (question != std::string::npos)
    {
        _out.query = remainder.substr(question + 1);
        remainder  = remainder.substr(0, question);
    }

    _out.path = remainder;

    return true;
}

// request
//   struct: a library-neutral HTTP request. Backends translate it into their
// own native calls; higher layers populate it without knowing the backend.
struct request
{
    http_method  method;
    std::string  url;
    header_list  headers;
    std::string  body;
    http_version preferred_version;

    request()
        : method(http_method::get),
          url(),
          headers(),
          body(),
          preferred_version(http_version::unknown)
    {
    }

    // set_header
    //   function: sets a header on this request (replace-or-append,
    // case-insensitive). Returns *this for chaining.
    request&
    set_header(
        const std::string& _name,
        const std::string& _value
    )
    {
        ::djinterp::web::set_header(headers, _name, _value);

        return *this;
    }

    // header
    //   function: the value of a request header, or `_fallback` if absent.
    D_NODISCARD std::string
    header(
        const std::string& _name,
        const std::string& _fallback = std::string()
    ) const
    {
        return header_value(headers, _name, _fallback);
    }
};

// response
//   struct: a library-neutral HTTP response, as filled in by a backend.
// `status` is the HTTP status code (0 when the exchange never produced one);
// `error` records any transport-level failure in neutral terms.
struct response
{
    int             status;
    header_list     headers;
    std::string     body;
    transport_error error;

    response()
        : status(0),
          headers(),
          body(),
          error(transport_error::none)
    {
    }

    // ok
    //   function: whether the response carries a 2xx status.
    D_NODISCARD bool
    ok() const
    {
        return is_success(status);
    }

    // category
    //   function: the status_category of this response's status code.
    D_NODISCARD status_category
    category() const
    {
        return classify(status);
    }

    // header
    //   function: the value of a response header, or `_fallback` if absent.
    D_NODISCARD std::string
    header(
        const std::string& _name,
        const std::string& _fallback = std::string()
    ) const
    {
        return header_value(headers, _name, _fallback);
    }
};


NS_END  // web
NS_END  // djinterp


#endif  // DJINTERP_WEB_
