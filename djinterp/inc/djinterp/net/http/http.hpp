/*******************************************************************************
* djinterp [net]                                                        http.hpp
*
* HTTP's semantics in C++, over net/http/http.h.
*   Scoped enumerations defined from the C ones, common field names and media
* types as constexpr constants, and each operation a thin call into the C
* foundation. It supplies
*     - the method, version, status code, class, and error enumerations    [1]
*     - common field names and media types, from http.h's literals         [2]
*     - methods, versions, status codes, field syntax, and error names     [3]
*   C++11 and later, over the standard library and net.hpp, so any header may
* include it. web.hpp does, naming these types by its old names while HTTP
* moves out of it. The native client this path held is retired to
* _retired/relay94_net_http; its successor arrives with HTTP/1.1's message
* syntax.
*
*
* path:      /inc/djinterp/net/http/http.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
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
*/

#ifndef DJINTERP_NET_HTTP_HTTP_HPP
#define DJINTERP_NET_HTTP_HTTP_HPP 1

// std
#include <string>  // std::string
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../net.hpp"          // NS_NET, the net C++ layer
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
enum class http_method : unsigned char
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

// 1.1.2
// http_version
//   enum: a protocol version.
enum class http_version : unsigned char
{
    unknown  = D_HTTP_VERSION_UNKNOWN,
    http_1_0 = D_HTTP_VERSION_1_0,
    http_1_1 = D_HTTP_VERSION_1_1,
    http_2   = D_HTTP_VERSION_2,
    http_3   = D_HTTP_VERSION_3
};

// 1.1.3
// http_status
//   enum: every registered status code, by name. Any int from 100 to 599 is
// a status code, and the operations take the int; two names from before RFC
// 9110 stay, as aliases, as web.hpp spelled them.
enum class http_status : int
{
    continue_                     = D_HTTP_STATUS_CONTINUE,
    switching_protocols           = D_HTTP_STATUS_SWITCHING_PROTOCOLS,
    processing                    = D_HTTP_STATUS_PROCESSING,
    early_hints                   = D_HTTP_STATUS_EARLY_HINTS,
    ok                            = D_HTTP_STATUS_OK,
    created                       = D_HTTP_STATUS_CREATED,
    accepted                      = D_HTTP_STATUS_ACCEPTED,
    non_authoritative_information = D_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION,
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
    proxy_authentication_required = D_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,
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
    unavailable_for_legal_reasons = D_HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS,
    internal_server_error         = D_HTTP_STATUS_INTERNAL_SERVER_ERROR,
    not_implemented               = D_HTTP_STATUS_NOT_IMPLEMENTED,
    bad_gateway                   = D_HTTP_STATUS_BAD_GATEWAY,
    service_unavailable           = D_HTTP_STATUS_SERVICE_UNAVAILABLE,
    gateway_timeout               = D_HTTP_STATUS_GATEWAY_TIMEOUT,
    http_version_not_supported    = D_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
    variant_also_negotiates       = D_HTTP_STATUS_VARIANT_ALSO_NEGOTIATES,
    insufficient_storage          = D_HTTP_STATUS_INSUFFICIENT_STORAGE,
    loop_detected                 = D_HTTP_STATUS_LOOP_DETECTED,
    not_extended                  = D_HTTP_STATUS_NOT_EXTENDED,
    network_authentication_required =
        D_HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED,
    // 413 and 422 under their names before RFC 9110
    payload_too_large             = D_HTTP_STATUS_CONTENT_TOO_LARGE,
    unprocessable_entity          = D_HTTP_STATUS_UNPROCESSABLE_CONTENT
};

// 1.1.4
// http_status_class
//   enum: a status code's class; unknown and success are web.hpp's names
// for none and successful, kept as aliases.
enum class http_status_class : unsigned char
{
    none          = D_HTTP_STATUS_CLASS_NONE,
    informational = D_HTTP_STATUS_CLASS_INFORMATIONAL,
    successful    = D_HTTP_STATUS_CLASS_SUCCESSFUL,
    redirection   = D_HTTP_STATUS_CLASS_REDIRECTION,
    client_error  = D_HTTP_STATUS_CLASS_CLIENT_ERROR,
    server_error  = D_HTTP_STATUS_CLASS_SERVER_ERROR,
    unknown       = D_HTTP_STATUS_CLASS_NONE,
    success       = D_HTTP_STATUS_CLASS_SUCCESSFUL
};

// 1.1.5
// http_error
//   enum: why HTTP text was refused.
enum class http_error : unsigned char
{
    none        = D_HTTP_OK,
    argument    = D_HTTP_ERROR_ARGUMENT,
    method      = D_HTTP_ERROR_METHOD,
    version     = D_HTTP_ERROR_VERSION,
    status      = D_HTTP_ERROR_STATUS,
    field_name  = D_HTTP_ERROR_FIELD_NAME,
    field_value = D_HTTP_ERROR_FIELD_VALUE,
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

// 1.2    Conversions
//------------------------------------------------------------------------------
// to_c / from_c -- the same values under the other language's names; the
// enumerators are defined as each other, so these are casts
D_NODISCARD inline d_http_method
to_c(
    http_method _method
) noexcept
{
    return static_cast<d_http_method>(_method);
}

D_NODISCARD inline http_method
from_c(
    d_http_method _method
) noexcept
{
    return static_cast<http_method>(_method);
}

D_NODISCARD inline d_http_version
to_c(
    http_version _version
) noexcept
{
    return static_cast<d_http_version>(_version);
}

D_NODISCARD inline http_version
from_c(
    d_http_version _version
) noexcept
{
    return static_cast<http_version>(_version);
}

D_NODISCARD inline d_http_status_class
to_c(
    http_status_class _class
) noexcept
{
    return static_cast<d_http_status_class>(_class);
}

D_NODISCARD inline http_status_class
from_c(
    d_http_status_class _class
) noexcept
{
    return static_cast<http_status_class>(_class);
}

D_NODISCARD inline d_http_error
to_c(
    http_error _error
) noexcept
{
    return static_cast<d_http_error>(_error);
}

D_NODISCARD inline http_error
from_c(
    d_http_error _error
) noexcept
{
    return static_cast<http_error>(_error);
}

NS_INTERNAL

    // http_pack
    //   function: a C view of a string's characters.
    D_NODISCARD inline ::d_pack_text
    http_pack(
        const std::string& _text
    ) noexcept
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
    ) noexcept
    {
        return (_status < 0) ? 0u
                             : static_cast<unsigned int>(_status);
    }

NS_END  // internal


//==============================================================================
// 2.  NAMES
//==============================================================================
// http.h's literals as constexpr constants, under web.hpp's names for them.


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
// http_method_name -- the name, or nullptr for other; safe and idempotent
// as RFC 9110 defines them, and other is neither
D_NODISCARD inline const char*
http_method_name(
    http_method _method
) noexcept
{
    return ::d_http_method_name(to_c(_method));
}

D_NODISCARD inline bool
http_method_is_safe(
    http_method _method
) noexcept
{
    return ::d_http_method_is_safe(to_c(_method));
}

D_NODISCARD inline bool
http_method_is_idempotent(
    http_method _method
) noexcept
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
D_NODISCARD inline http_error
parse_http_method(
    const std::string& _text,
    http_method&       _out
) noexcept
{
    ::d_http_method  method = D_HTTP_METHOD_OTHER;
    const http_error found  = from_c(::d_http_method_parse(
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
// http_version_name -- "HTTP/1.1" and the rest, or nullptr for unknown
D_NODISCARD inline const char*
http_version_name(
    http_version _version
) noexcept
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
D_NODISCARD inline http_error
parse_http_version(
    const std::string& _text,
    http_version&      _out
) noexcept
{
    ::d_http_version version = D_HTTP_VERSION_UNKNOWN;
    const http_error found   = from_c(::d_http_version_parse(
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
// the registered reason phrase ("" if none, never nullptr), whether a
// response may carry content, and whether it is one a client may follow
D_NODISCARD inline bool
http_status_is_valid(
    int _status
) noexcept
{
    return ::d_http_status_is_valid(internal::http_code(_status));
}

D_NODISCARD inline http_status_class
http_status_class_of(
    int _status
) noexcept
{
    return from_c(::d_http_status_class_of(internal::http_code(_status)));
}

D_NODISCARD inline http_status_class
http_status_class_of(
    http_status _status
) noexcept
{
    return http_status_class_of(static_cast<int>(_status));
}

D_NODISCARD inline const char*
http_status_reason(
    int _status
) noexcept
{
    return ::d_http_status_reason(internal::http_code(_status));
}

D_NODISCARD inline const char*
http_status_reason(
    http_status _status
) noexcept
{
    return http_status_reason(static_cast<int>(_status));
}

D_NODISCARD inline bool
http_status_allows_content(
    int _status
) noexcept
{
    return ::d_http_status_allows_content(internal::http_code(_status));
}

D_NODISCARD inline bool
http_status_allows_content(
    http_status _status
) noexcept
{
    return http_status_allows_content(static_cast<int>(_status));
}

D_NODISCARD inline bool
http_status_is_redirect(
    int _status
) noexcept
{
    return ::d_http_status_is_redirect(internal::http_code(_status));
}

D_NODISCARD inline bool
http_status_is_redirect(
    http_status _status
) noexcept
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
D_NODISCARD inline http_error
parse_http_status(
    const std::string& _text,
    int&               _out
) noexcept
{
    unsigned int     status = 0u;
    const http_error found  = from_c(::d_http_status_parse(
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
) noexcept
{
    return ::d_http_is_token(internal::http_pack(_text));
}

D_NODISCARD inline bool
http_is_field_value(
    const std::string& _text
) noexcept
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
) noexcept
{
    return ::d_http_field_name_equal(internal::http_pack(_name),
                                     internal::http_pack(_other));
}

// 3.5    Errors
//------------------------------------------------------------------------------
// http_error_name -- a short phrase for each error, for messages
D_NODISCARD inline const char*
http_error_name(
    http_error _error
) noexcept
{
    return ::d_http_error_name(to_c(_error));
}


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_HTTP_HTTP_HPP
