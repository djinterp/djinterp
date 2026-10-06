/*******************************************************************************
* djinterp [net]                                      http_tests_sa_vocabulary.c
*
* Standalone tests of http.h: methods, versions, status codes, reason
* phrases, field syntax, common names, and errors.
*   Every registered status code is looked up, so the enumeration and the
* reason table cannot drift apart unnoticed. Parsing is driven by tables of
* cases, one line each, so adding a case is adding a line.
*
*
* path:      /tests/djinterp/net/http/http_tests_sa_vocabulary.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./http_tests_sa.h"  // the suite
// std
#include <string.h>  // strchr, strcmp, strlen


// d_tests_http_method_case
//   struct: a method's text, and the error and method it must parse to.
struct d_tests_http_method_case
{
    const char*        text;
    enum d_http_error  error;
    enum d_http_method method;
};

// d_tests_http_version_case
//   struct: a version's text, and the error and version it must parse to.
struct d_tests_http_version_case
{
    const char*         text;
    enum d_http_error   error;
    enum d_http_version version;
};

// d_tests_http_status_case
//   struct: a status code's text, and the error and code it must parse to.
struct d_tests_http_status_case
{
    const char*       text;
    enum d_http_error error;
    unsigned int      status;
};

// d_tests_http_value_case
//   struct: bytes, which may hold a NUL, and whether they are a field value.
struct d_tests_http_value_case
{
    const char* data;
    size_t      length;
    bool        valid;
};

// METHOD_CASES
//   constant: every registered method, case, extension methods, refusals.
static const struct d_tests_http_method_case METHOD_CASES[] =
{
    { "GET",      D_HTTP_OK,           D_HTTP_METHOD_GET },
    { "HEAD",     D_HTTP_OK,           D_HTTP_METHOD_HEAD },
    { "POST",     D_HTTP_OK,           D_HTTP_METHOD_POST },
    { "PUT",      D_HTTP_OK,           D_HTTP_METHOD_PUT },
    { "DELETE",   D_HTTP_OK,           D_HTTP_METHOD_DELETE },
    { "PATCH",    D_HTTP_OK,           D_HTTP_METHOD_PATCH },
    { "OPTIONS",  D_HTTP_OK,           D_HTTP_METHOD_OPTIONS },
    { "TRACE",    D_HTTP_OK,           D_HTTP_METHOD_TRACE },
    { "CONNECT",  D_HTTP_OK,           D_HTTP_METHOD_CONNECT },
    { "get",      D_HTTP_OK,           D_HTTP_METHOD_OTHER },
    { "PROPFIND", D_HTTP_OK,           D_HTTP_METHOD_OTHER },
    { "M-SEARCH", D_HTTP_OK,           D_HTTP_METHOD_OTHER },
    { "",         D_HTTP_ERROR_METHOD, D_HTTP_METHOD_OTHER },
    { "GE T",     D_HTTP_ERROR_METHOD, D_HTTP_METHOD_OTHER },
    { "G(T",      D_HTTP_ERROR_METHOD, D_HTTP_METHOD_OTHER }
};

// VERSION_CASES
//   constant: both 1.x versions, a higher minor, and every kind of refusal.
static const struct d_tests_http_version_case VERSION_CASES[] =
{
    { "HTTP/1.0",  D_HTTP_OK,            D_HTTP_VERSION_1_0 },
    { "HTTP/1.1",  D_HTTP_OK,            D_HTTP_VERSION_1_1 },
    { "HTTP/1.9",  D_HTTP_OK,            D_HTTP_VERSION_1_1 },
    { "HTTP/2.0",  D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "HTTP/0.9",  D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "http/1.1",  D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "HTTP/1.",   D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "HTTP/1.10", D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "HTTP/11",   D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "HTTP/1.x",  D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN },
    { "",          D_HTTP_ERROR_VERSION, D_HTTP_VERSION_UNKNOWN }
};

// STATUS_CASES
//   constant: codes at both bounds, and every kind of refusal.
static const struct d_tests_http_status_case STATUS_CASES[] =
{
    { "200",  D_HTTP_OK,           200u },
    { "100",  D_HTTP_OK,           100u },
    { "599",  D_HTTP_OK,           599u },
    { "099",  D_HTTP_ERROR_STATUS, 0u },
    { "600",  D_HTTP_ERROR_STATUS, 0u },
    { "20",   D_HTTP_ERROR_STATUS, 0u },
    { "2000", D_HTTP_ERROR_STATUS, 0u },
    { "2x0",  D_HTTP_ERROR_STATUS, 0u },
    { " 20",  D_HTTP_ERROR_STATUS, 0u },
    { "",     D_HTTP_ERROR_STATUS, 0u }
};

// VALUE_CASES
//   constant: field values, and bytes that are not one.
static const struct d_tests_http_value_case VALUE_CASES[] =
{
    { "",                 0u,  true },
    { "text/html; q=0.9", 16u, true },
    { "a\tb",             3u,  true },
    { "caf\xC3\xA9",      5u,  true },
    { " a",               2u,  false },
    { "a ",               2u,  false },
    { "\t",               1u,  false },
    { "a\rb",             3u,  false },
    { "a\nb",             3u,  false },
    { "a\0b",             3u,  false },
    { "a\x7F",            2u,  false },
    { "a\x01" "b",        3u,  false }
};

// REGISTERED
//   constant: every d_http_status enumerator, to look each one up.
static const unsigned int REGISTERED[] =
{
    D_HTTP_STATUS_CONTINUE, D_HTTP_STATUS_SWITCHING_PROTOCOLS,
    D_HTTP_STATUS_PROCESSING, D_HTTP_STATUS_EARLY_HINTS, D_HTTP_STATUS_OK,
    D_HTTP_STATUS_CREATED, D_HTTP_STATUS_ACCEPTED,
    D_HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION, D_HTTP_STATUS_NO_CONTENT,
    D_HTTP_STATUS_RESET_CONTENT, D_HTTP_STATUS_PARTIAL_CONTENT,
    D_HTTP_STATUS_MULTI_STATUS, D_HTTP_STATUS_ALREADY_REPORTED,
    D_HTTP_STATUS_IM_USED, D_HTTP_STATUS_MULTIPLE_CHOICES,
    D_HTTP_STATUS_MOVED_PERMANENTLY, D_HTTP_STATUS_FOUND,
    D_HTTP_STATUS_SEE_OTHER, D_HTTP_STATUS_NOT_MODIFIED,
    D_HTTP_STATUS_USE_PROXY, D_HTTP_STATUS_TEMPORARY_REDIRECT,
    D_HTTP_STATUS_PERMANENT_REDIRECT, D_HTTP_STATUS_BAD_REQUEST,
    D_HTTP_STATUS_UNAUTHORIZED, D_HTTP_STATUS_PAYMENT_REQUIRED,
    D_HTTP_STATUS_FORBIDDEN, D_HTTP_STATUS_NOT_FOUND,
    D_HTTP_STATUS_METHOD_NOT_ALLOWED, D_HTTP_STATUS_NOT_ACCEPTABLE,
    D_HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,
    D_HTTP_STATUS_REQUEST_TIMEOUT, D_HTTP_STATUS_CONFLICT,
    D_HTTP_STATUS_GONE, D_HTTP_STATUS_LENGTH_REQUIRED,
    D_HTTP_STATUS_PRECONDITION_FAILED, D_HTTP_STATUS_CONTENT_TOO_LARGE,
    D_HTTP_STATUS_URI_TOO_LONG, D_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
    D_HTTP_STATUS_RANGE_NOT_SATISFIABLE, D_HTTP_STATUS_EXPECTATION_FAILED,
    D_HTTP_STATUS_IM_A_TEAPOT, D_HTTP_STATUS_MISDIRECTED_REQUEST,
    D_HTTP_STATUS_UNPROCESSABLE_CONTENT, D_HTTP_STATUS_LOCKED,
    D_HTTP_STATUS_FAILED_DEPENDENCY, D_HTTP_STATUS_TOO_EARLY,
    D_HTTP_STATUS_UPGRADE_REQUIRED, D_HTTP_STATUS_PRECONDITION_REQUIRED,
    D_HTTP_STATUS_TOO_MANY_REQUESTS,
    D_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE,
    D_HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS,
    D_HTTP_STATUS_INTERNAL_SERVER_ERROR, D_HTTP_STATUS_NOT_IMPLEMENTED,
    D_HTTP_STATUS_BAD_GATEWAY, D_HTTP_STATUS_SERVICE_UNAVAILABLE,
    D_HTTP_STATUS_GATEWAY_TIMEOUT, D_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
    D_HTTP_STATUS_VARIANT_ALSO_NEGOTIATES, D_HTTP_STATUS_INSUFFICIENT_STORAGE,
    D_HTTP_STATUS_LOOP_DETECTED, D_HTTP_STATUS_NOT_EXTENDED,
    D_HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED
};

// FIELD_NAMES, MEDIA_TYPES
//   constant: every field-name and media-type constant.
static const char* const FIELD_NAMES[] =
{
    D_HTTP_FIELD_ACCEPT, D_HTTP_FIELD_ACCEPT_ENCODING,
    D_HTTP_FIELD_AUTHORIZATION, D_HTTP_FIELD_CACHE_CONTROL,
    D_HTTP_FIELD_CONNECTION, D_HTTP_FIELD_CONTENT_ENCODING,
    D_HTTP_FIELD_CONTENT_LENGTH, D_HTTP_FIELD_CONTENT_TYPE,
    D_HTTP_FIELD_COOKIE, D_HTTP_FIELD_DATE, D_HTTP_FIELD_EXPECT,
    D_HTTP_FIELD_HOST, D_HTTP_FIELD_LOCATION,
    D_HTTP_FIELD_PROXY_AUTHENTICATE, D_HTTP_FIELD_PROXY_AUTHORIZATION,
    D_HTTP_FIELD_RETRY_AFTER, D_HTTP_FIELD_SERVER, D_HTTP_FIELD_SET_COOKIE,
    D_HTTP_FIELD_TE, D_HTTP_FIELD_TRAILER, D_HTTP_FIELD_TRANSFER_ENCODING,
    D_HTTP_FIELD_UPGRADE, D_HTTP_FIELD_USER_AGENT,
    D_HTTP_FIELD_WWW_AUTHENTICATE
};
static const char* const MEDIA_TYPES[] =
{
    D_HTTP_MEDIA_JSON, D_HTTP_MEDIA_OCTET_STREAM, D_HTTP_MEDIA_FORM_URLENCODED,
    D_HTTP_MEDIA_MULTIPART_FORM_DATA, D_HTTP_MEDIA_XML, D_HTTP_MEDIA_TEXT_PLAIN,
    D_HTTP_MEDIA_TEXT_HTML, D_HTTP_MEDIA_EVENT_STREAM
};

/*
d_tests_http_text
  A view of a C string.
*/
static struct d_pack_text
d_tests_http_text(
    const char* _text
)
{
    const struct d_pack_text view = { _text,
                                      strlen(_text) };

    return view;
}

/*
d_tests_sa_http_methods
  Tests the following:
  - every case of METHOD_CASES: registered names, case-sensitivity,
    extension methods, and refusals
  - each registered name round-trips; D_HTTP_METHOD_OTHER has none
  - safe and idempotent, method by method, as RFC 9110 lists them
*/
bool
d_tests_sa_http_methods(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(METHOD_CASES) / sizeof(METHOD_CASES[0]);
    size_t       right  = 0u;
    bool         result = true;

    // each case, error and method both
    for (size_t i = 0u; i < count; ++i)
    {
        enum d_http_method      method = D_HTTP_METHOD_OTHER;
        const enum d_http_error found  = d_http_method_parse(
                                             d_tests_http_text(
                                                 METHOD_CASES[i].text),
                                             &method);

        // the error, and on success the method and its name
        if ( (found == METHOD_CASES[i].error) &&
             ( (found != D_HTTP_OK)                        ||
               ( (method == METHOD_CASES[i].method) &&
                 ( (method == D_HTTP_METHOD_OTHER) ||
                   (strcmp(d_http_method_name(method),
                           METHOD_CASES[i].text) == 0) ) ) ) )
        {
            right += 1u;
        }
    }

    result = d_assert_standalone(right == count,
                                 "d_http_method_parse",
                                 "every method case parses as listed",
                                 _counter) && result;
    result = d_assert_standalone(
                 (d_http_method_name(D_HTTP_METHOD_OTHER) == NULL)        &&
                 (d_http_method_name((enum d_http_method)99) == NULL)     &&
                 (d_http_method_parse(d_tests_http_text("GET"),
                                      NULL) == D_HTTP_ERROR_ARGUMENT),
                 "d_http_method_name",
                 "extension methods have no name; NULL is refused",
                 _counter) && result;
    result = d_assert_standalone(
                 (d_http_method_is_safe(D_HTTP_METHOD_HEAD))           &&
                 (d_http_method_is_safe(D_HTTP_METHOD_TRACE))          &&
                 (!d_http_method_is_safe(D_HTTP_METHOD_PUT))           &&
                 (d_http_method_is_idempotent(D_HTTP_METHOD_PUT))      &&
                 (d_http_method_is_idempotent(D_HTTP_METHOD_DELETE))   &&
                 (!d_http_method_is_idempotent(D_HTTP_METHOD_POST))    &&
                 (!d_http_method_is_idempotent(D_HTTP_METHOD_PATCH))   &&
                 (!d_http_method_is_idempotent(D_HTTP_METHOD_OTHER)),
                 "d_http_method_is_safe",
                 "safety and idempotence as RFC 9110 lists them",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_http_versions
  Tests the following:
  - every case of VERSION_CASES, the higher-minor rule included
  - each version's name, and none for unknown
*/
bool
d_tests_sa_http_versions(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(VERSION_CASES) / sizeof(VERSION_CASES[0]);
    size_t       right  = 0u;
    bool         result = true;

    // each case, error and version both
    for (size_t i = 0u; i < count; ++i)
    {
        enum d_http_version     version = D_HTTP_VERSION_UNKNOWN;
        const enum d_http_error found   = d_http_version_parse(
                                              d_tests_http_text(
                                                  VERSION_CASES[i].text),
                                              &version);

        // the error, and on success the version
        if ( (found == VERSION_CASES[i].error) &&
             ( (found != D_HTTP_OK) ||
               (version == VERSION_CASES[i].version) ) )
        {
            right += 1u;
        }
    }

    result = d_assert_standalone(right == count,
                                 "d_http_version_parse",
                                 "every version case parses as listed",
                                 _counter) && result;
    result = d_assert_standalone(
                 (strcmp(d_http_version_name(D_HTTP_VERSION_1_1),
                         "HTTP/1.1") == 0)                              &&
                 (strcmp(d_http_version_name(D_HTTP_VERSION_3),
                         "HTTP/3") == 0)                                &&
                 (d_http_version_name(D_HTTP_VERSION_UNKNOWN) == NULL)  &&
                 (d_http_version_name((enum d_http_version)99) == NULL),
                 "d_http_version_name",
                 "each version's name; none for unknown",
                 _counter) && result;

    return result;
}

/*
d_tests_http_status_parses
  How many of STATUS_CASES parse as listed.
*/
static size_t
d_tests_http_status_parses(void)
{
    const size_t count = sizeof(STATUS_CASES) / sizeof(STATUS_CASES[0]);
    size_t       right = 0u;

    // each case, error and code both
    for (size_t i = 0u; i < count; ++i)
    {
        unsigned int            status = 0u;
        const enum d_http_error found  = d_http_status_parse(
                                             d_tests_http_text(
                                                 STATUS_CASES[i].text),
                                             &status);

        // the error, and on success the code
        if ( (found == STATUS_CASES[i].error) &&
             ( (found != D_HTTP_OK) ||
               (status == STATUS_CASES[i].status) ) )
        {
            right += 1u;
        }
    }

    return right;
}

/*
d_tests_sa_http_status
  Tests the following:
  - every case of STATUS_CASES
  - validity and class at every boundary
  - which codes allow content, and which redirect
*/
bool
d_tests_sa_http_status(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(STATUS_CASES) / sizeof(STATUS_CASES[0]);
    bool         result = true;

    result = d_assert_standalone(d_tests_http_status_parses() == count,
                                 "d_http_status_parse",
                                 "every status case parses as listed",
                                 _counter) && result;
    result = d_assert_standalone(
                 (d_http_status_class_of(99u) ==
                  D_HTTP_STATUS_CLASS_NONE)                            &&
                 (d_http_status_class_of(100u) ==
                  D_HTTP_STATUS_CLASS_INFORMATIONAL)                   &&
                 (d_http_status_class_of(199u) ==
                  D_HTTP_STATUS_CLASS_INFORMATIONAL)                   &&
                 (d_http_status_class_of(200u) ==
                  D_HTTP_STATUS_CLASS_SUCCESSFUL)                      &&
                 (d_http_status_class_of(399u) ==
                  D_HTTP_STATUS_CLASS_REDIRECTION)                     &&
                 (d_http_status_class_of(404u) ==
                  D_HTTP_STATUS_CLASS_CLIENT_ERROR)                    &&
                 (d_http_status_class_of(599u) ==
                  D_HTTP_STATUS_CLASS_SERVER_ERROR)                    &&
                 (d_http_status_class_of(600u) ==
                  D_HTTP_STATUS_CLASS_NONE),
                 "d_http_status_class_of",
                 "classes at every boundary",
                 _counter) && result;
    result = d_assert_standalone(
                 (!d_http_status_allows_content(101u))  &&
                 (!d_http_status_allows_content(204u))  &&
                 (!d_http_status_allows_content(304u))  &&
                 (!d_http_status_allows_content(600u))  &&
                 (d_http_status_allows_content(200u))   &&
                 (d_http_status_allows_content(404u))   &&
                 (d_http_status_is_redirect(301u))      &&
                 (d_http_status_is_redirect(308u))      &&
                 (!d_http_status_is_redirect(300u))     &&
                 (!d_http_status_is_redirect(304u))     &&
                 (!d_http_status_is_redirect(306u)),
                 "d_http_status_allows_content",
                 "content and redirects, code by code",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_http_reasons
  Tests the following:
  - every registered code has a phrase, the first and last included
  - phrases RFC 9110 renamed read as it names them
  - codes with none registered get "", never NULL
*/
bool
d_tests_sa_http_reasons(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(REGISTERED) / sizeof(REGISTERED[0]);
    size_t       named  = 0u;
    bool         result = true;

    // each registered code has a phrase
    for (size_t i = 0u; i < count; ++i)
    {
        // a non-empty phrase
        if (d_http_status_reason(REGISTERED[i])[0] != '\0')
        {
            named += 1u;
        }
    }

    result = d_assert_standalone(named == count,
                                 "d_http_status_reason",
                                 "every registered code has a phrase",
                                 _counter) && result;
    result = d_assert_standalone(
                 (strcmp(d_http_status_reason(100u),
                         "Continue") == 0)                         &&
                 (strcmp(d_http_status_reason(404u),
                         "Not Found") == 0)                        &&
                 (strcmp(d_http_status_reason(413u),
                         "Content Too Large") == 0)                &&
                 (strcmp(d_http_status_reason(422u),
                         "Unprocessable Content") == 0)            &&
                 (strcmp(d_http_status_reason(511u),
                         "Network Authentication Required") == 0),
                 "d_http_status_reason",
                 "phrases as RFC 9110 names them",
                 _counter) && result;
    result = d_assert_standalone(
                 (d_http_status_reason(299u)[0] == '\0') &&
                 (d_http_status_reason(306u)[0] == '\0') &&
                 (d_http_status_reason(0u)[0] == '\0')   &&
                 (d_http_status_reason(600u)[0] == '\0'),
                 "d_http_status_reason",
                 "unregistered codes get an empty phrase",
                 _counter) && result;

    return result;
}

/*
d_tests_http_values_right
  How many of VALUE_CASES are judged as listed.
*/
static size_t
d_tests_http_values_right(void)
{
    const size_t count = sizeof(VALUE_CASES) / sizeof(VALUE_CASES[0]);
    size_t       right = 0u;

    // each case judged
    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_pack_text value = { VALUE_CASES[i].data,
                                           VALUE_CASES[i].length };

        // judged as listed
        if (d_http_is_field_value(value) == VALUE_CASES[i].valid)
        {
            right += 1u;
        }
    }

    return right;
}

/*
d_tests_sa_http_syntax
  Tests the following:
  - tokens: every tchar, and the bytes no token holds
  - every case of VALUE_CASES: CR, LF, NUL, DEL, and edge whitespace refused
  - trimming, and field names compared without case
*/
bool
d_tests_sa_http_syntax(
    struct d_test_counter* _counter
)
{
    const size_t             count   = sizeof(VALUE_CASES) /
                                       sizeof(VALUE_CASES[0]);
    const struct d_pack_text trimmed = d_http_trim_whitespace(
                                           d_tests_http_text("  a b \t"));
    const struct d_pack_text blank   = d_http_trim_whitespace(
                                           d_tests_http_text("\t "));
    bool                     result  = true;

    result = d_assert_standalone(
                 (d_http_is_token(d_tests_http_text(
                      "!#$%&'*+-.^_`|~09AZaz")))                  &&
                 (d_http_is_token(d_tests_http_text("a")))       &&
                 (!d_http_is_token(d_tests_http_text("")))       &&
                 (!d_http_is_token(d_tests_http_text("a b")))    &&
                 (!d_http_is_token(d_tests_http_text("a:b")))    &&
                 (!d_http_is_token(d_tests_http_text("(a)")))    &&
                 (!d_http_is_token(d_tests_http_text("\"a\"")))  &&
                 (!d_http_is_token(d_tests_http_text("a\x80"))),
                 "d_http_is_token",
                 "every tchar, and nothing else",
                 _counter) && result;
    result = d_assert_standalone(d_tests_http_values_right() == count,
                                 "d_http_is_field_value",
                                 "every value case judged as listed",
                                 _counter) && result;
    result = d_assert_standalone(
                 (trimmed.length == 3u)                            &&
                 (trimmed.data[0] == 'a')                          &&
                 (blank.length == 0u)                              &&
                 (d_http_field_name_equal(
                      d_tests_http_text("Content-Length"),
                      d_tests_http_text("content-LENGTH")))        &&
                 (!d_http_field_name_equal(
                      d_tests_http_text("Host"),
                      d_tests_http_text("Hos")))                   &&
                 (!d_http_field_name_equal(
                      d_tests_http_text("TE"),
                      d_tests_http_text("TF"))),
                 "d_http_field_name_equal",
                 "trimming, and names compared without case",
                 _counter) && result;

    return result;
}

/*
d_tests_http_media_is_type
  Whether a media type is a token, '/', and a token.
*/
static bool
d_tests_http_media_is_type(
    const char* _media
)
{
    const char* const slash = strchr(_media,
                                     '/');

    // no slash, no type
    if (!slash)
    {
        return false;
    }

    const struct d_pack_text type    = { _media,
                                         (size_t)(slash - _media) };
    const struct d_pack_text subtype = d_tests_http_text(slash + 1);

    return ( (d_http_is_token(type)) &&
             (d_http_is_token(subtype)) );
}

/*
d_tests_sa_http_names
  Tests the following:
  - every field-name constant is a token
  - every media-type constant is a token, '/', and a token
*/
bool
d_tests_sa_http_names(
    struct d_test_counter* _counter
)
{
    const size_t fields = sizeof(FIELD_NAMES) / sizeof(FIELD_NAMES[0]);
    const size_t media  = sizeof(MEDIA_TYPES) / sizeof(MEDIA_TYPES[0]);
    size_t       tokens = 0u;
    size_t       types  = 0u;
    bool         result = true;

    // each field name, a token
    for (size_t i = 0u; i < fields; ++i)
    {
        tokens += (d_http_is_token(d_tests_http_text(FIELD_NAMES[i]))) ? 1u
                                                                        : 0u;
    }

    // each media type, well-formed
    for (size_t i = 0u; i < media; ++i)
    {
        types += (d_tests_http_media_is_type(MEDIA_TYPES[i])) ? 1u
                                                               : 0u;
    }

    result = d_assert_standalone(tokens == fields,
                                 "D_HTTP_FIELD_*",
                                 "every field-name constant is a token",
                                 _counter) && result;
    result = d_assert_standalone(types == media,
                                 "D_HTTP_MEDIA_*",
                                 "every media type is type/subtype",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_http_errors
  Tests the following:
  - every error has a name, and no two share one
  - an unknown value is named as unknown
*/
bool
d_tests_sa_http_errors(
    struct d_test_counter* _counter
)
{
    const enum d_http_error errors[] =
    {
        D_HTTP_OK,
        D_HTTP_ERROR_ARGUMENT,
        D_HTTP_ERROR_METHOD,
        D_HTTP_ERROR_VERSION,
        D_HTTP_ERROR_STATUS,
        D_HTTP_ERROR_FIELD_NAME,
        D_HTTP_ERROR_FIELD_VALUE,
        D_HTTP_ERROR_INCOMPLETE,
        D_HTTP_ERROR_START_LINE,
        D_HTTP_ERROR_START_LINE_TOO_LONG,
        D_HTTP_ERROR_HEAD_TOO_LARGE,
        D_HTTP_ERROR_LINE_ENDING,
        D_HTTP_ERROR_OBS_FOLD,
        D_HTTP_ERROR_TOO_MANY_FIELDS,
        D_HTTP_ERROR_TARGET,
        D_HTTP_ERROR_HOST
    };
    const size_t count    = sizeof(errors) / sizeof(errors[0]);
    size_t       distinct = 0u;

    // each name, against every later one
    for (size_t i = 0u; i < count; ++i)
    {
        bool unique = (strcmp(d_http_error_name(errors[i]),
                              "unknown error") != 0);

        // no later name the same
        for (size_t j = i + 1u; j < count; ++j)
        {
            unique = (unique) &&
                     (strcmp(d_http_error_name(errors[i]),
                             d_http_error_name(errors[j])) != 0);
        }

        distinct += (unique) ? 1u
                             : 0u;
    }

    return d_assert_standalone(
               (distinct == count) &&
               (strcmp(d_http_error_name((enum d_http_error)99),
                       "unknown error") == 0),
               "d_http_error_name",
               "every error named, and uniquely",
               _counter);
}
