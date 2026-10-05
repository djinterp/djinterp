/*******************************************************************************
* djinterp [net]                                                          http.h
*
* HTTP's semantics, as RFC 9110 defines them, in C.
*   The vocabulary every HTTP version shares, and nothing tied to one wire
* format. It supplies
*     - the method, version, status code, and error vocabulary             [1]
*     - common field names and media types, as string constants            [2]
*     - methods by name, and whether each is safe and idempotent           [3]
*     - protocol versions by name                                          [4]
*     - status codes: parsing, classes, reason phrases, and properties     [5]
*     - field syntax: tokens, values, whitespace, and name comparison      [6]
*     - each error's name                                                  [7]
*   Nothing allocates and nothing blocks. HTTP/1.1's message syntax (RFC 9112)
* builds on this header, as does every HTTP backend.
*
*
* path:      /inc/djinterp/net/http/http.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Methods
         1.  d_http_method
    2.  Versions
         1.  d_http_version
    3.  Status codes
         1.  d_http_status
         2.  d_http_status_class
    4.  Errors
         1.  d_http_error
2.  NAMES
    -----
    1.  Field names
         1.  D_HTTP_FIELD_ACCEPT
         2.  D_HTTP_FIELD_ACCEPT_ENCODING
         3.  D_HTTP_FIELD_AUTHORIZATION
         4.  D_HTTP_FIELD_CACHE_CONTROL
         5.  D_HTTP_FIELD_CONNECTION
         6.  D_HTTP_FIELD_CONTENT_ENCODING
         7.  D_HTTP_FIELD_CONTENT_LENGTH
         8.  D_HTTP_FIELD_CONTENT_TYPE
         9.  D_HTTP_FIELD_COOKIE
         10. D_HTTP_FIELD_DATE
         11. D_HTTP_FIELD_EXPECT
         12. D_HTTP_FIELD_HOST
         13. D_HTTP_FIELD_LOCATION
         14. D_HTTP_FIELD_PROXY_AUTHENTICATE
         15. D_HTTP_FIELD_PROXY_AUTHORIZATION
         16. D_HTTP_FIELD_RETRY_AFTER
         17. D_HTTP_FIELD_SERVER
         18. D_HTTP_FIELD_SET_COOKIE
         19. D_HTTP_FIELD_TE
         20. D_HTTP_FIELD_TRAILER
         21. D_HTTP_FIELD_TRANSFER_ENCODING
         22. D_HTTP_FIELD_UPGRADE
         23. D_HTTP_FIELD_USER_AGENT
         24. D_HTTP_FIELD_WWW_AUTHENTICATE
    2.  Media types
         1.  D_HTTP_MEDIA_JSON
         2.  D_HTTP_MEDIA_OCTET_STREAM
         3.  D_HTTP_MEDIA_FORM_URLENCODED
         4.  D_HTTP_MEDIA_MULTIPART_FORM_DATA
         5.  D_HTTP_MEDIA_XML
         6.  D_HTTP_MEDIA_TEXT_PLAIN
         7.  D_HTTP_MEDIA_TEXT_HTML
         8.  D_HTTP_MEDIA_EVENT_STREAM
3.  METHODS
    -------
    1.  Names
    2.  Properties
4.  VERSIONS
    --------
    1.  Names
5.  STATUS CODES
    ------------
    1.  Validity and parsing
    2.  Classes and reason phrases
    3.  Properties
6.  FIELD SYNTAX
    ------------
    1.  Tokens and values
    2.  Names
7.  ERRORS
    ------
    1.  Names
*/

#ifndef DJINTERP_NET_HTTP_HTTP_H
#define DJINTERP_NET_HTTP_HTTP_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_text


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// Methods, versions, status codes, and errors. A method or status code no one
// registered is still valid: HTTP extends both by registry, and a recipient
// must handle what it does not know.


// 1.1    Methods
//------------------------------------------------------------------------------
// 1.1.1
// d_http_method
//   enum: a request method (RFC 9110, section 9; PATCH, RFC 5789). Names are
// case-sensitive, so "get" is not GET; any other token is an extension
// method, D_HTTP_METHOD_OTHER, whose name the caller keeps.
enum d_http_method
{
    D_HTTP_METHOD_GET = 0,
    D_HTTP_METHOD_HEAD,
    D_HTTP_METHOD_POST,
    D_HTTP_METHOD_PUT,
    D_HTTP_METHOD_DELETE,
    D_HTTP_METHOD_PATCH,
    D_HTTP_METHOD_OPTIONS,
    D_HTTP_METHOD_TRACE,
    D_HTTP_METHOD_CONNECT,
    D_HTTP_METHOD_OTHER
};

// 1.2    Versions
//------------------------------------------------------------------------------
// 1.2.1
// d_http_version
//   enum: a protocol version (RFC 9110, section 2.5).
enum d_http_version
{
    D_HTTP_VERSION_UNKNOWN = 0,  // none recognized
    D_HTTP_VERSION_1_0,          // RFC 1945
    D_HTTP_VERSION_1_1,          // RFC 9112
    D_HTTP_VERSION_2,            // RFC 9113
    D_HTTP_VERSION_3             // RFC 9114
};

// 1.3    Status codes
//------------------------------------------------------------------------------
// 1.3.1
// d_http_status
//   enum: every status code in IANA's registry, by name. Any three-digit
// number from 100 to 599 is a status code; the names are a convenience, and
// the functions take the number. 418 is reserved and unused (RFC 9110,
// 15.5.19), and named for the joke it began as.
enum d_http_status
{
    D_HTTP_STATUS_CONTINUE                        = 100,
    D_HTTP_STATUS_SWITCHING_PROTOCOLS             = 101,
    D_HTTP_STATUS_PROCESSING                      = 102,
    D_HTTP_STATUS_EARLY_HINTS                     = 103,
    D_HTTP_STATUS_OK                              = 200,
    D_HTTP_STATUS_CREATED                         = 201,
    D_HTTP_STATUS_ACCEPTED                        = 202,
    D_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION   = 203,
    D_HTTP_STATUS_NO_CONTENT                      = 204,
    D_HTTP_STATUS_RESET_CONTENT                   = 205,
    D_HTTP_STATUS_PARTIAL_CONTENT                 = 206,
    D_HTTP_STATUS_MULTI_STATUS                    = 207,
    D_HTTP_STATUS_ALREADY_REPORTED                = 208,
    D_HTTP_STATUS_IM_USED                         = 226,
    D_HTTP_STATUS_MULTIPLE_CHOICES                = 300,
    D_HTTP_STATUS_MOVED_PERMANENTLY               = 301,
    D_HTTP_STATUS_FOUND                           = 302,
    D_HTTP_STATUS_SEE_OTHER                       = 303,
    D_HTTP_STATUS_NOT_MODIFIED                    = 304,
    D_HTTP_STATUS_USE_PROXY                       = 305,
    D_HTTP_STATUS_TEMPORARY_REDIRECT              = 307,
    D_HTTP_STATUS_PERMANENT_REDIRECT              = 308,
    D_HTTP_STATUS_BAD_REQUEST                     = 400,
    D_HTTP_STATUS_UNAUTHORIZED                    = 401,
    D_HTTP_STATUS_PAYMENT_REQUIRED                = 402,
    D_HTTP_STATUS_FORBIDDEN                       = 403,
    D_HTTP_STATUS_NOT_FOUND                       = 404,
    D_HTTP_STATUS_METHOD_NOT_ALLOWED              = 405,
    D_HTTP_STATUS_NOT_ACCEPTABLE                  = 406,
    D_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED   = 407,
    D_HTTP_STATUS_REQUEST_TIMEOUT                 = 408,
    D_HTTP_STATUS_CONFLICT                        = 409,
    D_HTTP_STATUS_GONE                            = 410,
    D_HTTP_STATUS_LENGTH_REQUIRED                 = 411,
    D_HTTP_STATUS_PRECONDITION_FAILED             = 412,
    D_HTTP_STATUS_CONTENT_TOO_LARGE               = 413,
    D_HTTP_STATUS_URI_TOO_LONG                    = 414,
    D_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE          = 415,
    D_HTTP_STATUS_RANGE_NOT_SATISFIABLE           = 416,
    D_HTTP_STATUS_EXPECTATION_FAILED              = 417,
    D_HTTP_STATUS_IM_A_TEAPOT                     = 418,
    D_HTTP_STATUS_MISDIRECTED_REQUEST             = 421,
    D_HTTP_STATUS_UNPROCESSABLE_CONTENT           = 422,
    D_HTTP_STATUS_LOCKED                          = 423,
    D_HTTP_STATUS_FAILED_DEPENDENCY               = 424,
    D_HTTP_STATUS_TOO_EARLY                       = 425,
    D_HTTP_STATUS_UPGRADE_REQUIRED                = 426,
    D_HTTP_STATUS_PRECONDITION_REQUIRED           = 428,
    D_HTTP_STATUS_TOO_MANY_REQUESTS               = 429,
    D_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    D_HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS   = 451,
    D_HTTP_STATUS_INTERNAL_SERVER_ERROR           = 500,
    D_HTTP_STATUS_NOT_IMPLEMENTED                 = 501,
    D_HTTP_STATUS_BAD_GATEWAY                     = 502,
    D_HTTP_STATUS_SERVICE_UNAVAILABLE             = 503,
    D_HTTP_STATUS_GATEWAY_TIMEOUT                 = 504,
    D_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED      = 505,
    D_HTTP_STATUS_VARIANT_ALSO_NEGOTIATES         = 506,
    D_HTTP_STATUS_INSUFFICIENT_STORAGE            = 507,
    D_HTTP_STATUS_LOOP_DETECTED                   = 508,
    D_HTTP_STATUS_NOT_EXTENDED                    = 510,
    D_HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED = 511
};

// 1.3.2
// d_http_status_class
//   enum: a status code's class, its first digit (RFC 9110, section 15).
enum d_http_status_class
{
    D_HTTP_STATUS_CLASS_NONE = 0,       // not a status code
    D_HTTP_STATUS_CLASS_INFORMATIONAL,  // 1xx
    D_HTTP_STATUS_CLASS_SUCCESSFUL,     // 2xx
    D_HTTP_STATUS_CLASS_REDIRECTION,    // 3xx
    D_HTTP_STATUS_CLASS_CLIENT_ERROR,   // 4xx
    D_HTTP_STATUS_CLASS_SERVER_ERROR    // 5xx
};

// 1.4    Errors
//------------------------------------------------------------------------------
// 1.4.1
// d_http_error
//   enum: why HTTP text was refused. Errors are only ever added at the end,
// so a value never changes meaning.
enum d_http_error
{
    D_HTTP_OK = 0,                     // no error
    D_HTTP_ERROR_ARGUMENT,             // a NULL argument
    D_HTTP_ERROR_METHOD,               // a method that is not a token
    D_HTTP_ERROR_VERSION,              // a version other than "HTTP/1.<digit>"
    D_HTTP_ERROR_STATUS,               // not three digits from 100 to 599
    D_HTTP_ERROR_FIELD_NAME,           // a field name that is not a token
    D_HTTP_ERROR_FIELD_VALUE,          // a field value holding a forbidden byte
    D_HTTP_ERROR_INCOMPLETE,           // more bytes are needed; not a failure
    D_HTTP_ERROR_START_LINE,           // a malformed request or status line
    D_HTTP_ERROR_START_LINE_TOO_LONG,  // a start line over the limit (414)
    D_HTTP_ERROR_HEAD_TOO_LARGE,       // a head over the limit (431)
    D_HTTP_ERROR_LINE_ENDING,          // a bare CR, or an LF without its CR
    D_HTTP_ERROR_OBS_FOLD,             // a field line folded onto the next
    D_HTTP_ERROR_TOO_MANY_FIELDS,      // more field lines than room for them
    D_HTTP_ERROR_TARGET,               // a malformed or misplaced target
    D_HTTP_ERROR_HOST                  // not exactly one valid Host in 1.1
};


//==============================================================================
// 2.  NAMES
//==============================================================================
// The canonical spellings of common field names, from IANA's field name
// registry, and of common media types, as string literals, so that C++ can
// make them constexpr. Field names compare without case; send these.


// 2.1    Field names
//------------------------------------------------------------------------------
// 2.1.1
// D_HTTP_FIELD_ACCEPT
//   constant: the field name "Accept".
#define D_HTTP_FIELD_ACCEPT "Accept"

// 2.1.2
// D_HTTP_FIELD_ACCEPT_ENCODING
//   constant: the field name "Accept-Encoding".
#define D_HTTP_FIELD_ACCEPT_ENCODING "Accept-Encoding"

// 2.1.3
// D_HTTP_FIELD_AUTHORIZATION
//   constant: the field name "Authorization".
#define D_HTTP_FIELD_AUTHORIZATION "Authorization"

// 2.1.4
// D_HTTP_FIELD_CACHE_CONTROL
//   constant: the field name "Cache-Control".
#define D_HTTP_FIELD_CACHE_CONTROL "Cache-Control"

// 2.1.5
// D_HTTP_FIELD_CONNECTION
//   constant: the field name "Connection".
#define D_HTTP_FIELD_CONNECTION "Connection"

// 2.1.6
// D_HTTP_FIELD_CONTENT_ENCODING
//   constant: the field name "Content-Encoding".
#define D_HTTP_FIELD_CONTENT_ENCODING "Content-Encoding"

// 2.1.7
// D_HTTP_FIELD_CONTENT_LENGTH
//   constant: the field name "Content-Length".
#define D_HTTP_FIELD_CONTENT_LENGTH "Content-Length"

// 2.1.8
// D_HTTP_FIELD_CONTENT_TYPE
//   constant: the field name "Content-Type".
#define D_HTTP_FIELD_CONTENT_TYPE "Content-Type"

// 2.1.9
// D_HTTP_FIELD_COOKIE
//   constant: the field name "Cookie".
#define D_HTTP_FIELD_COOKIE "Cookie"

// 2.1.10
// D_HTTP_FIELD_DATE
//   constant: the field name "Date".
#define D_HTTP_FIELD_DATE "Date"

// 2.1.11
// D_HTTP_FIELD_EXPECT
//   constant: the field name "Expect".
#define D_HTTP_FIELD_EXPECT "Expect"

// 2.1.12
// D_HTTP_FIELD_HOST
//   constant: the field name "Host".
#define D_HTTP_FIELD_HOST "Host"

// 2.1.13
// D_HTTP_FIELD_LOCATION
//   constant: the field name "Location".
#define D_HTTP_FIELD_LOCATION "Location"

// 2.1.14
// D_HTTP_FIELD_PROXY_AUTHENTICATE
//   constant: the field name "Proxy-Authenticate".
#define D_HTTP_FIELD_PROXY_AUTHENTICATE "Proxy-Authenticate"

// 2.1.15
// D_HTTP_FIELD_PROXY_AUTHORIZATION
//   constant: the field name "Proxy-Authorization".
#define D_HTTP_FIELD_PROXY_AUTHORIZATION "Proxy-Authorization"

// 2.1.16
// D_HTTP_FIELD_RETRY_AFTER
//   constant: the field name "Retry-After".
#define D_HTTP_FIELD_RETRY_AFTER "Retry-After"

// 2.1.17
// D_HTTP_FIELD_SERVER
//   constant: the field name "Server".
#define D_HTTP_FIELD_SERVER "Server"

// 2.1.18
// D_HTTP_FIELD_SET_COOKIE
//   constant: the field name "Set-Cookie".
#define D_HTTP_FIELD_SET_COOKIE "Set-Cookie"

// 2.1.19
// D_HTTP_FIELD_TE
//   constant: the field name "TE".
#define D_HTTP_FIELD_TE "TE"

// 2.1.20
// D_HTTP_FIELD_TRAILER
//   constant: the field name "Trailer".
#define D_HTTP_FIELD_TRAILER "Trailer"

// 2.1.21
// D_HTTP_FIELD_TRANSFER_ENCODING
//   constant: the field name "Transfer-Encoding".
#define D_HTTP_FIELD_TRANSFER_ENCODING "Transfer-Encoding"

// 2.1.22
// D_HTTP_FIELD_UPGRADE
//   constant: the field name "Upgrade".
#define D_HTTP_FIELD_UPGRADE "Upgrade"

// 2.1.23
// D_HTTP_FIELD_USER_AGENT
//   constant: the field name "User-Agent".
#define D_HTTP_FIELD_USER_AGENT "User-Agent"

// 2.1.24
// D_HTTP_FIELD_WWW_AUTHENTICATE
//   constant: the field name "WWW-Authenticate".
#define D_HTTP_FIELD_WWW_AUTHENTICATE "WWW-Authenticate"

// 2.2    Media types
//------------------------------------------------------------------------------
// 2.2.1
// D_HTTP_MEDIA_JSON
//   constant: the media type "application/json".
#define D_HTTP_MEDIA_JSON "application/json"

// 2.2.2
// D_HTTP_MEDIA_OCTET_STREAM
//   constant: the media type "application/octet-stream".
#define D_HTTP_MEDIA_OCTET_STREAM "application/octet-stream"

// 2.2.3
// D_HTTP_MEDIA_FORM_URLENCODED
//   constant: the media type "application/x-www-form-urlencoded".
#define D_HTTP_MEDIA_FORM_URLENCODED "application/x-www-form-urlencoded"

// 2.2.4
// D_HTTP_MEDIA_MULTIPART_FORM_DATA
//   constant: the media type "multipart/form-data".
#define D_HTTP_MEDIA_MULTIPART_FORM_DATA "multipart/form-data"

// 2.2.5
// D_HTTP_MEDIA_XML
//   constant: the media type "application/xml".
#define D_HTTP_MEDIA_XML "application/xml"

// 2.2.6
// D_HTTP_MEDIA_TEXT_PLAIN
//   constant: the media type "text/plain".
#define D_HTTP_MEDIA_TEXT_PLAIN "text/plain"

// 2.2.7
// D_HTTP_MEDIA_TEXT_HTML
//   constant: the media type "text/html".
#define D_HTTP_MEDIA_TEXT_HTML "text/html"

// 2.2.8
// D_HTTP_MEDIA_EVENT_STREAM
//   constant: the media type "text/event-stream".
#define D_HTTP_MEDIA_EVENT_STREAM "text/event-stream"


//==============================================================================
// 3.  METHODS
//==============================================================================


// 3.1    Names
//------------------------------------------------------------------------------
// d_http_method_name -- a method's name, or NULL for D_HTTP_METHOD_OTHER,
// whose name only the caller has
const char*          d_http_method_name(enum d_http_method _method);

/**
 * @brief Recognizes a method name, which is case-sensitive.
 *
 * @param[in]  _text  the name.
 * @param[out] _out   receives the method: D_HTTP_METHOD_OTHER for a token
 *                    naming none of the registered ones.
 * @return D_HTTP_OK, D_HTTP_ERROR_ARGUMENT, or D_HTTP_ERROR_METHOD when the
 *         text is not a token.
 */
enum d_http_error    d_http_method_parse(struct d_pack_text  _text,
                                         enum d_http_method* _out);

// 3.2    Properties
//------------------------------------------------------------------------------
// safe and idempotent (RFC 9110, 9.2.1 and 9.2.2); D_HTTP_METHOD_OTHER is
// neither, since nothing is known of it
bool                 d_http_method_is_safe(enum d_http_method _method);
bool                 d_http_method_is_idempotent(enum d_http_method _method);


//==============================================================================
// 4.  VERSIONS
//==============================================================================


// 4.1    Names
//------------------------------------------------------------------------------
// d_http_version_name -- "HTTP/1.0", "HTTP/1.1", "HTTP/2", or "HTTP/3"; NULL
// for D_HTTP_VERSION_UNKNOWN
const char*          d_http_version_name(enum d_http_version _version);

/**
 * @brief Recognizes an HTTP/1.x version as a start line writes it (RFC 9112,
 *        section 2.3): "HTTP/" DIGIT "." DIGIT, case-sensitive.
 *
 * @param[in]  _text  the version.
 * @param[out] _out   receives D_HTTP_VERSION_1_0 or D_HTTP_VERSION_1_1; a
 *                    higher 1.x minor version is taken as 1.1, as RFC 9112
 *                    directs a recipient to.
 * @return D_HTTP_OK, D_HTTP_ERROR_ARGUMENT, or D_HTTP_ERROR_VERSION for any
 *         other text, including a major version other than 1.
 */
enum d_http_error    d_http_version_parse(struct d_pack_text   _text,
                                          enum d_http_version* _out);


//==============================================================================
// 5.  STATUS CODES
//==============================================================================


// 5.1    Validity and parsing
//------------------------------------------------------------------------------
// d_http_status_is_valid -- whether a number is a status code, 100 to 599
bool                 d_http_status_is_valid(unsigned int _status);

/**
 * @brief Recognizes a status code as a status line writes it: exactly three
 *        digits (RFC 9112, section 4), from 100 to 599.
 *
 * @param[in]  _text  the digits.
 * @param[out] _out   receives the code.
 * @return D_HTTP_OK, D_HTTP_ERROR_ARGUMENT, or D_HTTP_ERROR_STATUS.
 */
enum d_http_error    d_http_status_parse(struct d_pack_text _text,
                                         unsigned int*      _out);

// 5.2    Classes and reason phrases
//------------------------------------------------------------------------------
// class -- by the first digit, D_HTTP_STATUS_CLASS_NONE for a number that is
// no status code; reason -- the registered phrase, or "" for a code with
// none registered, and never NULL
enum d_http_status_class d_http_status_class_of(unsigned int _status);
const char*              d_http_status_reason(unsigned int _status);

// 5.3    Properties
//------------------------------------------------------------------------------
// allows_content -- false for 1xx, 204, and 304, whose responses never carry
// content (RFC 9110, 6.4.1), and for a number that is no status code;
// is_redirect -- 301, 302, 303, 307, and 308, the codes a client may follow
// on its own (RFC 9110, 15.4)
bool                 d_http_status_allows_content(unsigned int _status);
bool                 d_http_status_is_redirect(unsigned int _status);


//==============================================================================
// 6.  FIELD SYNTAX
//==============================================================================


// 6.1    Tokens and values
//------------------------------------------------------------------------------
// is_token -- one or more tchar (RFC 9110, 5.6.2), as field names, methods,
// and parameter names are; is_field_value -- visible ASCII, obs-text, and
// interior spaces and tabs, with no whitespace at either end and never CR,
// LF, or NUL (RFC 9110, 5.5), and an empty value is one; trim_whitespace --
// the text without the SP and HTAB at its ends, the OWS around a value
bool                 d_http_is_token(struct d_pack_text _text);
bool                 d_http_is_field_value(struct d_pack_text _text);
struct d_pack_text   d_http_trim_whitespace(struct d_pack_text _text);

// 6.2    Names
//------------------------------------------------------------------------------
// d_http_field_name_equal -- whether two field names are the same, ASCII
// case ignored (RFC 9110, 5.1)
bool                 d_http_field_name_equal(struct d_pack_text _name,
                                             struct d_pack_text _other);


//==============================================================================
// 7.  ERRORS
//==============================================================================


// 7.1    Names
//------------------------------------------------------------------------------
// d_http_error_name -- a short phrase for each error, for messages
const char*          d_http_error_name(enum d_http_error _error);


D_EXTERN_C_END


#endif  // DJINTERP_NET_HTTP_HTTP_H
