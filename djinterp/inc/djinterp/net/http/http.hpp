/*******************************************************************************
* djinterp [net]                                                        http.hpp
*
* HTTP's semantics in C++, over net/http/http.h.
*   Enumerations defined from the C ones and scoped in structs (decision 3.4:
* the type of an http_status is http_status::value), common field names and
* media types as constants, constexpr from C++11, and each operation a thin
* call into the C foundation. It supplies
*     - the method, version, status code, class, and error enumerations    [1]
*     - common field names and media types, from http.h's literals         [2]
*     - methods, versions, status codes, field syntax, and error names     [3]
*   C++98 and later, over the standard library and net.hpp, so any header may
* include it. The native client this path held is retired to
* _retired/relay94_net_http; its successor arrives with HTTP/1.1's message
* syntax.
*
*
* path:      /inc/djinterp/net/http/http.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Enumerations
         1.  http_method
         2.  http_version
         3.  http_status
         4.  http_status_class
         5.  http_error
    2.  Conversions
2.  NAMES
    -----
    1.  Field names
         1.  http_field
    2.  Media types
         1.  http_media
3.  OPERATIONS
    ----------
    1.  Methods
    2.  Versions
    3.  Status codes
    4.  Field syntax
    5.  Errors
4.  MESSAGES
    --------
    1.  Transport errors
         1.  transport_error
         2.  to_string(transport_error)
    2.  Methods, versions and status codes
         1.  to_string(http_method)
         2.  to_string(http_version)
         3.  method_from_string
         4.  is_informational
         5.  is_success
         6.  is_redirection
         7.  is_client_error
         8.  is_server_error
         9.  is_error
         10.  classify
         11.  reason_phrase
         12.  reason_phrase(http_status)
    3.  Headers
         1.  header_field
         2.  header_list
         3.  headers
         4.  find_header
         5.  has_header
         6.  header_value
         7.  set_header
    4.  Requests and responses
         1.  D_NET_HTTP_DEFAULT_USER_AGENT
         2.  request
         3.  response
*/

#ifndef DJINTERP_NET_HTTP_HTTP_HPP
#define DJINTERP_NET_HTTP_HTTP_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../net_common.hpp"   // NS_NET, the net C++ layer
#include "./http.h"            // the C vocabulary


NS_DJINTERP
NS_NET


//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// The C vocabulary under C++ names. Each enumerator is defined as its C
// counterpart, so the two cannot disagree.


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// http_method
//   enum: a request method; other is an extension method, whose name the
// caller keeps.
struct http_method
{
    enum value
    {
        get     = D_HTTP_METHOD_GET,
        head    = D_HTTP_METHOD_HEAD,
        post    = D_HTTP_METHOD_POST,
        put     = D_HTTP_METHOD_PUT,
        delete_ = D_HTTP_METHOD_DELETE,
        patch   = D_HTTP_METHOD_PATCH,
        options = D_HTTP_METHOD_OPTIONS,
        trace   = D_HTTP_METHOD_TRACE,
        connect = D_HTTP_METHOD_CONNECT,
        other   = D_HTTP_METHOD_OTHER
    };
};

// 1.1.2
// http_version
//   enum: a protocol version.
struct http_version
{
    enum value
    {
        unknown  = D_HTTP_VERSION_UNKNOWN,
        http_1_0 = D_HTTP_VERSION_1_0,
        http_1_1 = D_HTTP_VERSION_1_1,
        http_2   = D_HTTP_VERSION_2,
        http_3   = D_HTTP_VERSION_3
    };
};

// 1.1.3
// http_status
//   enum: every registered status code, by name. Any int from 100 to 599 is
// a status code, and the operations take the int.
struct http_status
{
    enum value
    {
        continue_                     = D_HTTP_STATUS_CONTINUE,
        switching_protocols           = D_HTTP_STATUS_SWITCHING_PROTOCOLS,
        processing                    = D_HTTP_STATUS_PROCESSING,
        early_hints                   = D_HTTP_STATUS_EARLY_HINTS,
        ok                            = D_HTTP_STATUS_OK,
        created                       = D_HTTP_STATUS_CREATED,
        accepted                      = D_HTTP_STATUS_ACCEPTED,
        non_authoritative_information =
            D_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION,
        no_content                    = D_HTTP_STATUS_NO_CONTENT,
        reset_content                 = D_HTTP_STATUS_RESET_CONTENT,
        partial_content               = D_HTTP_STATUS_PARTIAL_CONTENT,
        multi_status                  = D_HTTP_STATUS_MULTI_STATUS,
        already_reported              = D_HTTP_STATUS_ALREADY_REPORTED,
        im_used                       = D_HTTP_STATUS_IM_USED,
        multiple_choices              = D_HTTP_STATUS_MULTIPLE_CHOICES,
        moved_permanently             = D_HTTP_STATUS_MOVED_PERMANENTLY,
        found                         = D_HTTP_STATUS_FOUND,
        see_other                     = D_HTTP_STATUS_SEE_OTHER,
        not_modified                  = D_HTTP_STATUS_NOT_MODIFIED,
        use_proxy                     = D_HTTP_STATUS_USE_PROXY,
        temporary_redirect            = D_HTTP_STATUS_TEMPORARY_REDIRECT,
        permanent_redirect            = D_HTTP_STATUS_PERMANENT_REDIRECT,
        bad_request                   = D_HTTP_STATUS_BAD_REQUEST,
        unauthorized                  = D_HTTP_STATUS_UNAUTHORIZED,
        payment_required              = D_HTTP_STATUS_PAYMENT_REQUIRED,
        forbidden                     = D_HTTP_STATUS_FORBIDDEN,
        not_found                     = D_HTTP_STATUS_NOT_FOUND,
        method_not_allowed            = D_HTTP_STATUS_METHOD_NOT_ALLOWED,
        not_acceptable                = D_HTTP_STATUS_NOT_ACCEPTABLE,
        proxy_authentication_required =
            D_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,
        request_timeout               = D_HTTP_STATUS_REQUEST_TIMEOUT,
        conflict                      = D_HTTP_STATUS_CONFLICT,
        gone                          = D_HTTP_STATUS_GONE,
        length_required               = D_HTTP_STATUS_LENGTH_REQUIRED,
        precondition_failed           = D_HTTP_STATUS_PRECONDITION_FAILED,
        content_too_large             = D_HTTP_STATUS_CONTENT_TOO_LARGE,
        uri_too_long                  = D_HTTP_STATUS_URI_TOO_LONG,
        unsupported_media_type        = D_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
        range_not_satisfiable         = D_HTTP_STATUS_RANGE_NOT_SATISFIABLE,
        expectation_failed            = D_HTTP_STATUS_EXPECTATION_FAILED,
        im_a_teapot                   = D_HTTP_STATUS_IM_A_TEAPOT,
        misdirected_request           = D_HTTP_STATUS_MISDIRECTED_REQUEST,
        unprocessable_content         = D_HTTP_STATUS_UNPROCESSABLE_CONTENT,
        locked                        = D_HTTP_STATUS_LOCKED,
        failed_dependency             = D_HTTP_STATUS_FAILED_DEPENDENCY,
        too_early                     = D_HTTP_STATUS_TOO_EARLY,
        upgrade_required              = D_HTTP_STATUS_UPGRADE_REQUIRED,
        precondition_required         = D_HTTP_STATUS_PRECONDITION_REQUIRED,
        too_many_requests             = D_HTTP_STATUS_TOO_MANY_REQUESTS,
        request_header_fields_too_large =
            D_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE,
        unavailable_for_legal_reasons =
            D_HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS,
        internal_server_error         = D_HTTP_STATUS_INTERNAL_SERVER_ERROR,
        not_implemented               = D_HTTP_STATUS_NOT_IMPLEMENTED,
        bad_gateway                   = D_HTTP_STATUS_BAD_GATEWAY,
        service_unavailable           = D_HTTP_STATUS_SERVICE_UNAVAILABLE,
        gateway_timeout               = D_HTTP_STATUS_GATEWAY_TIMEOUT,
        http_version_not_supported    =
            D_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
        variant_also_negotiates       = D_HTTP_STATUS_VARIANT_ALSO_NEGOTIATES,
        insufficient_storage          = D_HTTP_STATUS_INSUFFICIENT_STORAGE,
        loop_detected                 = D_HTTP_STATUS_LOOP_DETECTED,
        not_extended                  = D_HTTP_STATUS_NOT_EXTENDED,
        network_authentication_required =
        D_HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED
    };
};

// 1.1.4
// http_status_class
//   enum: a status code's class.
struct http_status_class
{
    enum value
    {
        none          = D_HTTP_STATUS_CLASS_NONE,
        informational = D_HTTP_STATUS_CLASS_INFORMATIONAL,
        successful    = D_HTTP_STATUS_CLASS_SUCCESSFUL,
        redirection   = D_HTTP_STATUS_CLASS_REDIRECTION,
        client_error  = D_HTTP_STATUS_CLASS_CLIENT_ERROR,
        server_error  = D_HTTP_STATUS_CLASS_SERVER_ERROR
    };
};

// 1.1.5
// http_error
//   enum: why HTTP text was refused.
struct http_error
{
    enum value
    {
        none                = D_HTTP_OK,
        invalid_argument    = D_HTTP_ERROR_INVALID_ARGUMENT,
        method              = D_HTTP_ERROR_METHOD,
        version             = D_HTTP_ERROR_VERSION,
        status              = D_HTTP_ERROR_STATUS,
        field_name          = D_HTTP_ERROR_FIELD_NAME,
        field_value         = D_HTTP_ERROR_FIELD_VALUE,
        incomplete          = D_HTTP_ERROR_INCOMPLETE,
        start_line          = D_HTTP_ERROR_START_LINE,
        start_line_too_long = D_HTTP_ERROR_START_LINE_TOO_LONG,
        head_too_large      = D_HTTP_ERROR_HEAD_TOO_LARGE,
        line_ending         = D_HTTP_ERROR_LINE_ENDING,
        obs_fold            = D_HTTP_ERROR_OBS_FOLD,
        too_many_fields     = D_HTTP_ERROR_TOO_MANY_FIELDS,
        target              = D_HTTP_ERROR_TARGET,
        host                = D_HTTP_ERROR_HOST
    };
};

// 1.2    Conversions
//------------------------------------------------------------------------------
// to_c / from_c -- the same values under the other language's names; the
// enumerators are defined as each other, so these are casts
D_NODISCARD inline d_http_method
to_c(
    http_method::value _method
) D_NOEXCEPT
{
    return static_cast<d_http_method>(_method);
}

D_NODISCARD inline http_method::value
from_c(
    d_http_method _method
) D_NOEXCEPT
{
    return static_cast<http_method::value>(_method);
}

D_NODISCARD inline d_http_version
to_c(
    http_version::value _version
) D_NOEXCEPT
{
    return static_cast<d_http_version>(_version);
}

D_NODISCARD inline http_version::value
from_c(
    d_http_version _version
) D_NOEXCEPT
{
    return static_cast<http_version::value>(_version);
}

D_NODISCARD inline d_http_status_class
to_c(
    http_status_class::value _class
) D_NOEXCEPT
{
    return static_cast<d_http_status_class>(_class);
}

D_NODISCARD inline http_status_class::value
from_c(
    d_http_status_class _class
) D_NOEXCEPT
{
    return static_cast<http_status_class::value>(_class);
}

D_NODISCARD inline d_http_error
to_c(
    http_error::value _error
) D_NOEXCEPT
{
    return static_cast<d_http_error>(_error);
}

D_NODISCARD inline http_error::value
from_c(
    d_http_error _error
) D_NOEXCEPT
{
    return static_cast<http_error::value>(_error);
}

NS_INTERNAL

    // http_pack
    //   function: a C view of a string's characters.
    D_NODISCARD inline ::d_pack_text
    http_pack(
        const std::string& _text
    ) D_NOEXCEPT
    {
        const ::d_pack_text text = { _text.data(),
                                     _text.size() };

        return text;
    }

    // http_code
    //   function: a status int as the C foundation takes it; a negative one
    // becomes 0, which is no status code either.
    D_NODISCARD inline unsigned int
    http_code(
        int _status
    ) D_NOEXCEPT
    {
        return (_status < 0) ? 0u
                             : static_cast<unsigned int>(_status);
    }

NS_END  // internal


//==============================================================================
// 2.  NAMES
//==============================================================================
// http.h's literals as constexpr constants.


// 2.1    Field names
//------------------------------------------------------------------------------
// 2.1.1
// http_field
//   namespace: the canonical spellings of common field names.
D_NAMESPACE(http_field)

    D_CONSTEXPR const char* const accept         = D_HTTP_FIELD_ACCEPT;
    D_CONSTEXPR const char* const accept_encoding =
                                      D_HTTP_FIELD_ACCEPT_ENCODING;
    D_CONSTEXPR const char* const authorization  = D_HTTP_FIELD_AUTHORIZATION;
    D_CONSTEXPR const char* const cache_control  = D_HTTP_FIELD_CACHE_CONTROL;
    D_CONSTEXPR const char* const connection     = D_HTTP_FIELD_CONNECTION;
    D_CONSTEXPR const char* const content_encoding =
                                      D_HTTP_FIELD_CONTENT_ENCODING;
    D_CONSTEXPR const char* const content_length = D_HTTP_FIELD_CONTENT_LENGTH;
    D_CONSTEXPR const char* const content_type   = D_HTTP_FIELD_CONTENT_TYPE;
    D_CONSTEXPR const char* const cookie         = D_HTTP_FIELD_COOKIE;
    D_CONSTEXPR const char* const date           = D_HTTP_FIELD_DATE;
    D_CONSTEXPR const char* const expect         = D_HTTP_FIELD_EXPECT;
    D_CONSTEXPR const char* const host           = D_HTTP_FIELD_HOST;
    D_CONSTEXPR const char* const location       = D_HTTP_FIELD_LOCATION;
    D_CONSTEXPR const char* const proxy_authenticate =
                                      D_HTTP_FIELD_PROXY_AUTHENTICATE;
    D_CONSTEXPR const char* const proxy_authorization =
                                      D_HTTP_FIELD_PROXY_AUTHORIZATION;
    D_CONSTEXPR const char* const retry_after    = D_HTTP_FIELD_RETRY_AFTER;
    D_CONSTEXPR const char* const server         = D_HTTP_FIELD_SERVER;
    D_CONSTEXPR const char* const set_cookie     = D_HTTP_FIELD_SET_COOKIE;
    D_CONSTEXPR const char* const te             = D_HTTP_FIELD_TE;
    D_CONSTEXPR const char* const trailer        = D_HTTP_FIELD_TRAILER;
    D_CONSTEXPR const char* const transfer_encoding =
                                      D_HTTP_FIELD_TRANSFER_ENCODING;
    D_CONSTEXPR const char* const upgrade        = D_HTTP_FIELD_UPGRADE;
    D_CONSTEXPR const char* const user_agent     = D_HTTP_FIELD_USER_AGENT;
    D_CONSTEXPR const char* const www_authenticate =
                                      D_HTTP_FIELD_WWW_AUTHENTICATE;

NS_END  // http_field

// 2.2    Media types
//------------------------------------------------------------------------------
// 2.2.1
// http_media
//   namespace: common media types.
D_NAMESPACE(http_media)

    D_CONSTEXPR const char* const json         = D_HTTP_MEDIA_JSON;
    D_CONSTEXPR const char* const octet_stream = D_HTTP_MEDIA_OCTET_STREAM;
    D_CONSTEXPR const char* const form_urlencoded =
                                      D_HTTP_MEDIA_FORM_URLENCODED;
    D_CONSTEXPR const char* const multipart_form_data =
                                      D_HTTP_MEDIA_MULTIPART_FORM_DATA;
    D_CONSTEXPR const char* const xml          = D_HTTP_MEDIA_XML;
    D_CONSTEXPR const char* const text_plain   = D_HTTP_MEDIA_TEXT_PLAIN;
    D_CONSTEXPR const char* const text_html    = D_HTTP_MEDIA_TEXT_HTML;
    D_CONSTEXPR const char* const event_stream = D_HTTP_MEDIA_EVENT_STREAM;

NS_END  // http_media


//==============================================================================
// 3.  OPERATIONS
//==============================================================================
// Each a thin call into the C foundation.


// 3.1    Methods
//------------------------------------------------------------------------------
// http_method_name -- the name, or null for other; safe and idempotent
// as RFC 9110 defines them, and other is neither
D_NODISCARD inline const char*
http_method_name(
    http_method::value _method
) D_NOEXCEPT
{
    return ::d_http_method_name(to_c(_method));
}

D_NODISCARD inline bool
http_method_is_safe(
    http_method::value _method
) D_NOEXCEPT
{
    return ::d_http_method_is_safe(to_c(_method));
}

D_NODISCARD inline bool
http_method_is_idempotent(
    http_method::value _method
) D_NOEXCEPT
{
    return ::d_http_method_is_idempotent(to_c(_method));
}

/**
 * @brief Recognizes a method name, which is case-sensitive.
 *
 * @param[in]  _text  the name.
 * @param[out] _out   receives the method, other for an extension method;
 *                    untouched on failure.
 * @return http_error::none, or http_error::method when the text is not a
 *         token.
 */
D_NODISCARD inline http_error::value
parse_http_method(
    const std::string&  _text,
    http_method::value& _out
) D_NOEXCEPT
{
    ::d_http_method         method = D_HTTP_METHOD_OTHER;
    const http_error::value found  = from_c(::d_http_method_parse(
                                                internal::http_pack(_text),
                                                &method));

    // only a recognized method is written out
    if (found == http_error::none)
    {
        _out = from_c(method);
    }

    return found;
}

// 3.2    Versions
//------------------------------------------------------------------------------
// http_version_name -- "HTTP/1.1" and the rest, or null for unknown
D_NODISCARD inline const char*
http_version_name(
    http_version::value _version
) D_NOEXCEPT
{
    return ::d_http_version_name(to_c(_version));
}

/**
 * @brief Recognizes an HTTP/1.x version as a start line writes it; a higher
 *        1.x minor version is taken as 1.1.
 *
 * @param[in]  _text  the version.
 * @param[out] _out   receives the version; untouched on failure.
 * @return http_error::none, or http_error::version for any other text.
 */
D_NODISCARD inline http_error::value
parse_http_version(
    const std::string&   _text,
    http_version::value& _out
) D_NOEXCEPT
{
    ::d_http_version        version = D_HTTP_VERSION_UNKNOWN;
    const http_error::value found   = from_c(::d_http_version_parse(
                                                 internal::http_pack(_text),
                                                 &version));

    // only a recognized version is written out
    if (found == http_error::none)
    {
        _out = from_c(version);
    }

    return found;
}

// 3.3    Status codes
//------------------------------------------------------------------------------
// each takes a status int, or an http_status: validity (100 to 599), class,
// the registered reason phrase ("" if none, never null), whether a
// response may carry content, and whether it is one a client may follow
D_NODISCARD inline bool
http_status_is_valid(
    int _status
) D_NOEXCEPT
{
    return ::d_http_status_is_valid(internal::http_code(_status));
}

D_NODISCARD inline http_status_class::value
http_status_class_of(
    int _status
) D_NOEXCEPT
{
    return from_c(::d_http_status_class_of(internal::http_code(_status)));
}

D_NODISCARD inline http_status_class::value
http_status_class_of(
    http_status::value _status
) D_NOEXCEPT
{
    return http_status_class_of(static_cast<int>(_status));
}

D_NODISCARD inline const char*
http_status_reason(
    int _status
) D_NOEXCEPT
{
    return ::d_http_status_reason(internal::http_code(_status));
}

D_NODISCARD inline const char*
http_status_reason(
    http_status::value _status
) D_NOEXCEPT
{
    return http_status_reason(static_cast<int>(_status));
}

D_NODISCARD inline bool
http_status_allows_content(
    int _status
) D_NOEXCEPT
{
    return ::d_http_status_allows_content(internal::http_code(_status));
}

D_NODISCARD inline bool
http_status_allows_content(
    http_status::value _status
) D_NOEXCEPT
{
    return http_status_allows_content(static_cast<int>(_status));
}

D_NODISCARD inline bool
http_status_is_redirect(
    int _status
) D_NOEXCEPT
{
    return ::d_http_status_is_redirect(internal::http_code(_status));
}

D_NODISCARD inline bool
http_status_is_redirect(
    http_status::value _status
) D_NOEXCEPT
{
    return http_status_is_redirect(static_cast<int>(_status));
}

/**
 * @brief Recognizes a status code as a status line writes it: exactly three
 *        digits, from 100 to 599.
 *
 * @param[in]  _text  the digits.
 * @param[out] _out   receives the code; untouched on failure.
 * @return http_error::none, or http_error::status.
 */
D_NODISCARD inline http_error::value
parse_http_status(
    const std::string& _text,
    int&               _out
) D_NOEXCEPT
{
    unsigned int            status = 0u;
    const http_error::value found  = from_c(::d_http_status_parse(
                                                internal::http_pack(_text),
                                                &status));

    // only a recognized code is written out
    if (found == http_error::none)
    {
        _out = static_cast<int>(status);
    }

    return found;
}

// 3.4    Field syntax
//------------------------------------------------------------------------------
// token and field-value syntax (RFC 9110, 5.6.2 and 5.5); a value holding a
// NUL is refused, as C++ strings may hold one; trimming drops the SP and
// HTAB at both ends; field names compare without ASCII case
D_NODISCARD inline bool
http_is_token(
    const std::string& _text
) D_NOEXCEPT
{
    return ::d_http_is_token(internal::http_pack(_text));
}

D_NODISCARD inline bool
http_is_field_value(
    const std::string& _text
) D_NOEXCEPT
{
    return ::d_http_is_field_value(internal::http_pack(_text));
}

D_NODISCARD inline std::string
http_trim_whitespace(
    const std::string& _text
)
{
    const ::d_pack_text trimmed = ::d_http_trim_whitespace(
                                      internal::http_pack(_text));

    return std::string(trimmed.data,
                       trimmed.length);
}

D_NODISCARD inline bool
http_field_name_equal(
    const std::string& _name,
    const std::string& _other
) D_NOEXCEPT
{
    return ::d_http_field_name_equal(internal::http_pack(_name),
                                     internal::http_pack(_other));
}

// 3.5    Errors
//------------------------------------------------------------------------------
// http_error_name -- a short phrase for each error, for messages
D_NODISCARD inline const char*
http_error_name(
    http_error::value _error
) D_NOEXCEPT
{
    return ::d_http_error_name(to_c(_error));
}

// http_error_to_net -- the io_error an error maps to, by the rules net.h
// gives with d_net_error
D_NODISCARD inline io_error::value
http_error_to_net(
    http_error::value _error
) D_NOEXCEPT
{
    return from_c(::d_http_error_to_net(to_c(_error)));
}


//==============================================================================
// 4.  MESSAGES
//==============================================================================
// Transport errors; names and classes of methods, versions and status codes;
// header lists; and the neutral request and response a client sends and
// receives.


// 4.1    Transport errors
//------------------------------------------------------------------------------
// 4.1.1
// transport_error
//   enum: a library-neutral classification of transport-level failures, onto
// which each backend maps its native error codes so higher layers can reason
// about failures without depending on any specific client library.
struct transport_error
{
    enum value
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
};

// 4.1.2
// to_string(transport_error)
//   function: a short human-readable label for a transport error.
D_NODISCARD inline D_CONSTEXPR_CPP14 const char*
to_string(
    transport_error::value _error
)
{
    switch (_error)
    {
        case transport_error::none:
            return "none";
        case transport_error::unsupported_protocol:
            return "unsupported protocol";
        case transport_error::could_not_resolve_host:
            return "could not resolve host";
        case transport_error::could_not_connect:
            return "could not connect";
        case transport_error::timed_out:
            return "timed out";
        case transport_error::tls_error:
            return "TLS error";
        case transport_error::too_many_redirects:
            return "too many redirects";
        case transport_error::write_error:
            return "write error";
        case transport_error::read_error:
            return "read error";
        case transport_error::canceled:
            return "canceled";
        case transport_error::out_of_memory:
            return "out of memory";
        case transport_error::unknown:
            return "unknown error";
    }

    return "unknown error";
}

// 4.2    Methods, versions and status codes
//------------------------------------------------------------------------------
// 4.2.1
// to_string(http_method)
//   function: canonical uppercase wire token for a method (e.g. "GET").
D_NODISCARD inline D_CONSTEXPR_CPP14 const char*
to_string(
    net::http_method::value _method
)
{
    switch (_method)
    {
        case net::http_method::get:     return "GET";
        case net::http_method::head:    return "HEAD";
        case net::http_method::post:    return "POST";
        case net::http_method::put:     return "PUT";
        case net::http_method::delete_: return "DELETE";
        case net::http_method::patch:   return "PATCH";
        case net::http_method::options: return "OPTIONS";
        case net::http_method::trace:   return "TRACE";
        case net::http_method::connect: return "CONNECT";
        case net::http_method::other:   return "";
    }

    return "";
}

// 4.2.2
// to_string(http_version)
//   function: the HTTP version token (e.g. "HTTP/1.1"); "" when unknown.
D_NODISCARD inline D_CONSTEXPR_CPP14 const char*
to_string(
    net::http_version::value _version
)
{
    switch (_version)
    {
        case net::http_version::http_1_0: return "HTTP/1.0";
        case net::http_version::http_1_1: return "HTTP/1.1";
        case net::http_version::http_2:   return "HTTP/2";
        case net::http_version::http_3:   return "HTTP/3";
        case net::http_version::unknown:  return "";
    }

    return "";
}

// 4.2.3
// method_from_string
//   function: maps a (case-insensitive) method token to an http_method,
// writing the result to `_out` and returning true on success; false (and
// `_out` unchanged) for an unrecognized token.
D_NODISCARD inline bool
method_from_string(
    const std::string&  _token,
    net::http_method::value& _out
)
{
    static const struct
    {
        const char*        token;
        net::http_method::value method;
    } methods[] =
    {
        { "GET",     net::http_method::get     },
        { "HEAD",    net::http_method::head    },
        { "POST",    net::http_method::post    },
        { "PUT",     net::http_method::put     },
        { "DELETE",  net::http_method::delete_ },
        { "PATCH",   net::http_method::patch   },
        { "OPTIONS", net::http_method::options },
        { "TRACE",   net::http_method::trace   },
        { "CONNECT", net::http_method::connect }
    };
    const ::d_pack_text token = { _token.data(),
                                  _token.size() };

    // the first token that matches names the method
    for (std::size_t i = 0; i < (sizeof(methods) / sizeof(methods[0])); ++i)
    {
        if (::d_net_ascii_ieq_cstr(token,
                                   methods[i].token))
        {
            _out = methods[i].method;

            return true;
        }
    }

    return false;
}

// 4.2.4
// is_informational
//   function: 1xx -- the request was received, continuing process.
D_NODISCARD inline D_CONSTEXPR bool
is_informational(
    int _code
)
{
    return (_code >= 100) && (_code < 200);
}

// 4.2.5
// is_success
//   function: 2xx -- the request was successfully received and accepted.
D_NODISCARD inline D_CONSTEXPR bool
is_success(
    int _code
)
{
    return (_code >= 200) && (_code < 300);
}

// 4.2.6
// is_redirection
//   function: 3xx -- further action is needed to complete the request.
D_NODISCARD inline D_CONSTEXPR bool
is_redirection(
    int _code
)
{
    return (_code >= 300) && (_code < 400);
}

// 4.2.7
// is_client_error
//   function: 4xx -- the request contains bad syntax or cannot be fulfilled.
D_NODISCARD inline D_CONSTEXPR bool
is_client_error(
    int _code
)
{
    return (_code >= 400) && (_code < 500);
}

// 4.2.8
// is_server_error
//   function: 5xx -- the server failed to fulfill a valid request.
D_NODISCARD inline D_CONSTEXPR bool
is_server_error(
    int _code
)
{
    return (_code >= 500) && (_code < 600);
}

// 4.2.9
// is_error
//   function: any client or server error (>= 400).
D_NODISCARD inline D_CONSTEXPR bool
is_error(
    int _code
)
{
    return (_code >= 400) && (_code < 600);
}

// 4.2.10
// classify
//   function: the http_status_class a status code falls into.
D_NODISCARD inline D_CONSTEXPR_CPP14 net::http_status_class::value
classify(
    int _code
)
{
    if (is_informational(_code))
    {
        return net::http_status_class::informational;
    }

    if (is_success(_code))
    {
        return net::http_status_class::successful;
    }

    if (is_redirection(_code))
    {
        return net::http_status_class::redirection;
    }

    if (is_client_error(_code))
    {
        return net::http_status_class::client_error;
    }

    if (is_server_error(_code))
    {
        return net::http_status_class::server_error;
    }

    return net::http_status_class::none;
}

// 4.2.11
// reason_phrase
//   function: the standard reason phrase for a status code (e.g. "Not Found"
// for 404); "" for a code without a registered phrase here.
D_NODISCARD inline D_CONSTEXPR_CPP14 const char*
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

// 4.2.12
// reason_phrase(http_status)
//   function: reason-phrase overload for the enumerated status type.
D_NODISCARD inline D_CONSTEXPR_CPP14 const char*
reason_phrase(
    net::http_status::value _status
)
{
    return reason_phrase(static_cast<int>(_status));
}

// 4.3    Headers
//------------------------------------------------------------------------------
// 4.3.1
// header_field
//   type: a single HTTP header as an ordered (name, value) pair. Field names
// are case-insensitive on the wire; the lookup helpers below honor that.
typedef std::pair<std::string, std::string> header_field;

// 4.3.2
// header_list
//   type: an ordered collection of header fields (duplicates permitted, as the
// protocol allows).
typedef std::vector<header_field> header_list;

// 4.3.3
// headers
//   type: convenience alias for header_list.
typedef header_list headers;

// 4.3.4
// find_header
//   function: pointer to the first field whose name matches `_name`
// case-insensitively, or null when absent. Const overload.
D_NODISCARD inline const header_field*
find_header(
    const header_list& _headers,
    const std::string& _name
)
{
    // linear scan honoring case-insensitive field names
    for (std::size_t i = 0; i < _headers.size(); ++i)
    {
        // the first match is the one
        if (::djinterp::net::http_field_name_equal(_headers[i].first,
                                                   _name))
        {
            return &_headers[i];
        }
    }

    return D_NULLPTR;
}

// 4.3.5
// has_header
//   function: whether a field named `_name` (case-insensitive) is present.
D_NODISCARD inline bool
has_header(
    const header_list& _headers,
    const std::string& _name
)
{
    return (find_header(_headers,
                        _name) != D_NULLPTR);
}

// 4.3.6
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
    const header_field* field = find_header(_headers,
                                            _name);

    return field ? field->second : _fallback;
}

// 4.3.7
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
    for (std::size_t i = 0; i < _headers.size(); ++i)
    {
        // the first match takes the value
        if (::djinterp::net::http_field_name_equal(_headers[i].first,
                                                   _name))
        {
            _headers[i].second = _value;

            return;
        }
    }

    // no existing field -- append
    _headers.push_back(header_field(_name,
                                    _value));

    return;
}

// 4.4    Requests and responses
//------------------------------------------------------------------------------
// 4.4.1
// D_NET_HTTP_DEFAULT_USER_AGENT
//   macro: the User-Agent net's HTTP clients advertise unless told
// otherwise; pre-define it to change it.
#ifndef D_NET_HTTP_DEFAULT_USER_AGENT
    #define D_NET_HTTP_DEFAULT_USER_AGENT "djinterp-net/" D_NET_VERSION_STRING
#endif  // D_NET_HTTP_DEFAULT_USER_AGENT

// 4.4.2
// request
//   struct: a library-neutral HTTP request. Backends translate it into their
// own native calls; higher layers populate it without knowing the backend.
struct request
{
    net::http_method::value  method;
    std::string         url;
    header_list         headers;
    std::string         body;
    net::http_version::value preferred_version;

    request()
        : method(net::http_method::get),
          url(),
          headers(),
          body(),
          preferred_version(net::http_version::unknown)
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
        ::djinterp::net::set_header(headers,
                                    _name,
                                    _value);

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
        return header_value(headers,
                            _name,
                            _fallback);
    }
};

// 4.4.3
// response
//   struct: a library-neutral HTTP response, as filled in by a backend.
// `status` is the HTTP status code (0 when the exchange never produced one);
// `error` records any transport-level failure in neutral terms.
struct response
{
    int                    status;
    header_list            headers;
    std::string            body;
    transport_error::value error;

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
    //   function: the http_status_class of this response's status code.
    D_NODISCARD net::http_status_class::value
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
        return header_value(headers,
                            _name,
                            _fallback);
    }
};

NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_HTTP_HTTP_HPP
