/*******************************************************************************
* djinterp [net]                                                          curl.c
*
*   Definitions for curl.h: the libcurl binding, over libcurl's easy
* interface, one handle per transfer.
*   Four mechanisms carry it. A write callback is always installed, because
* libcurl without one writes the body to stdout; a sink without a body
* callback discards instead. The header callback parses each field as the
* C++ web layer always has -- split at the first colon, the value's blanks
* trimmed -- and skips status lines by their "HTTP/" prefix, which no field
* name can begin with. Request header fields are checked for colons and line
* breaks before libcurl sees them, so no field can smuggle in another. And
* CURLOPT_NOSIGNAL is always set, so libcurl never installs a SIGALRM
* handler for its timeouts, which a multi-threaded program cannot survive.
*   Everything that needs libcurl is selected per function: a build without
* it gets stubs reporting D_CURL_STATUS_UNSUPPORTED, after the same argument
* checks, so the two builds agree on every call that libcurl never sees.
*
*
* path:      /src/djinterp/net/curl/curl.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "../../../../inc/djinterp/net/curl/curl.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t
#include <stdlib.h>   // malloc, free
#include <string.h>   // memchr, memcpy, strcmp, strncmp
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"                // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"        // d_pack_text
#include "../../../../inc/djinterp/config/net/curl/cfg_curl.h"  // D_INTERNAL_CURL

#if (D_INTERNAL_CURL == 1)
    // libcurl
    #include <curl/curl.h>  // CURL, curl_easy_*, curl_slist_*
#endif  // D_INTERNAL_CURL


/*
d_curl_status_name
  A switch over every status; the default catches values the enum does not
list.
*/
const char*
d_curl_status_name(
    enum d_curl_status _status
)
{
    switch (_status)
    {
        case D_CURL_STATUS_OK:
            return "ok";

        case D_CURL_STATUS_INVALID_ARGUMENT:
            return "invalid argument";

        case D_CURL_STATUS_UNSUPPORTED:
            return "unsupported";

        case D_CURL_STATUS_UNSUPPORTED_PROTOCOL:
            return "unsupported protocol";

        case D_CURL_STATUS_COULD_NOT_RESOLVE_HOST:
            return "could not resolve host";

        case D_CURL_STATUS_COULD_NOT_CONNECT:
            return "could not connect";

        case D_CURL_STATUS_TIMED_OUT:
            return "timed out";

        case D_CURL_STATUS_TLS_ERROR:
            return "TLS error";

        case D_CURL_STATUS_TOO_MANY_REDIRECTS:
            return "too many redirects";

        case D_CURL_STATUS_WRITE_ERROR:
            return "write error";

        case D_CURL_STATUS_READ_ERROR:
            return "read error";

        case D_CURL_STATUS_CANCELED:
            return "canceled";

        case D_CURL_STATUS_OUT_OF_MEMORY:
            return "out of memory";

        case D_CURL_STATUS_UNKNOWN:
            return "unknown";

        default:
            return "invalid status";
    }
}

/*
d_curl_options_init
  The defaults curl.h lists, each assigned explicitly.
*/
void
d_curl_options_init(
    struct d_curl_options* _options
)
{
    // nothing to fill
    if (_options == NULL)
    {
        return;
    }

    _options->timeout_ms         = 0;
    _options->connect_timeout_ms = 0;
    _options->follow_redirects   = true;
    _options->max_redirects      = D_CURL_MAX_REDIRECTS_DEFAULT;
    _options->verify_tls         = true;
    _options->verbose            = false;
    _options->accept_encoding    = "";
    _options->user_agent         = NULL;
    _options->proxy              = NULL;

    return;
}

/*
d_internal_curl_span_ok
  A span is well-formed when its pointer is set or its length is zero.
*/
static bool
d_internal_curl_span_ok(
    const void* _data,
    size_t      _length
)
{
    return ( (_data != NULL) ||
             (_length == 0) );
}

/*
d_internal_curl_clean
  Whether text holds none of the bytes in _banned, nor a NUL, which would end
the field early in the copy libcurl keeps.
*/
static bool
d_internal_curl_clean(
    struct d_pack_text _text,
    const char*        _banned
)
{
    // empty text holds nothing
    if (_text.length == 0)
    {
        return true;
    }

    // a NUL would cut the field short
    if (memchr(_text.data,
               '\0',
               _text.length) != NULL)
    {
        return false;
    }

    // each banned byte in turn
    for (const char* ban = _banned; *ban != '\0'; ban++)
    {
        if (memchr(_text.data,
                   *ban,
                   _text.length) != NULL)
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_curl_field_ok
  A request header field is safe to send when its name is non-empty and
holds no colon, and neither part holds a line break or a NUL: each would let
the field end early and smuggle in another, or be cut short.
*/
static bool
d_internal_curl_field_ok(
    const struct d_curl_header* _field
)
{
    return ( (_field->name.length > 0)                              &&
             (_field->name.data != NULL)                            &&
             (d_internal_curl_span_ok(_field->value.data,
                                      _field->value.length))        &&
             (d_internal_curl_clean(_field->name,
                                    ":\r\n"))                        &&
             (d_internal_curl_clean(_field->value,
                                    "\r\n")) );
}

/*
d_internal_curl_request_ok
  Checks a request as d_curl_perform documents, before any library call, so
builds with and without libcurl reject the same requests.
*/
static bool
d_internal_curl_request_ok(
    const struct d_curl_request* _request
)
{
    // the request, its URL, and its spans
    if ( (_request == NULL)                                   ||
         (_request->url == NULL)                              ||
         (!d_internal_curl_span_ok(_request->headers,
                                   _request->header_count))   ||
         (!d_internal_curl_span_ok(_request->body.data,
                                   _request->body.size)) )
    {
        return false;
    }

    // every header field
    for (size_t i = 0; i < _request->header_count; i++)
    {
        if (!d_internal_curl_field_ok(&_request->headers[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_curl_report
  Stores a result where the caller asked for one, and returns its status.
*/
static enum d_curl_status
d_internal_curl_report(
    struct d_curl_result*       _out,
    const struct d_curl_result* _result
)
{
    // the caller may not want the details
    if (_out != NULL)
    {
        *_out = *_result;
    }

    return _result->status;
}

#if (D_INTERNAL_CURL == 1)

// D_INTERNAL_CURL_BIT_<FEATURE>
//   macro: libcurl's feature bit for each d_curl_feature its headers may
// predate: the bit where the headers name it, 0 where they do not, which
// d_curl_supports reads as unsupported. The rest date from before the
// binding's 7.21.6 floor.
#if defined(CURL_VERSION_HTTP2)
    #define D_INTERNAL_CURL_BIT_HTTP2      CURL_VERSION_HTTP2
#else
    #define D_INTERNAL_CURL_BIT_HTTP2      0
#endif  // CURL_VERSION_HTTP2
#if defined(CURL_VERSION_HTTP3)
    #define D_INTERNAL_CURL_BIT_HTTP3      CURL_VERSION_HTTP3
#else
    #define D_INTERNAL_CURL_BIT_HTTP3      0
#endif  // CURL_VERSION_HTTP3
#if defined(CURL_VERSION_BROTLI)
    #define D_INTERNAL_CURL_BIT_BROTLI     CURL_VERSION_BROTLI
#else
    #define D_INTERNAL_CURL_BIT_BROTLI     0
#endif  // CURL_VERSION_BROTLI
#if defined(CURL_VERSION_ZSTD)
    #define D_INTERNAL_CURL_BIT_ZSTD       CURL_VERSION_ZSTD
#else
    #define D_INTERNAL_CURL_BIT_ZSTD       0
#endif  // CURL_VERSION_ZSTD
#if defined(CURL_VERSION_THREADSAFE)
    #define D_INTERNAL_CURL_BIT_THREADSAFE CURL_VERSION_THREADSAFE
#else
    #define D_INTERNAL_CURL_BIT_THREADSAFE 0
#endif  // CURL_VERSION_THREADSAFE

/*
d_internal_curl_status
  Maps libcurl's codes to the binding's categories exactly as the C++ web
layer's to_transport_error does, so a failure reads the same through
either.
*/
static enum d_curl_status
d_internal_curl_status(
    CURLcode _code
)
{
    switch (_code)
    {
        case CURLE_OK:
            return D_CURL_STATUS_OK;

        case CURLE_UNSUPPORTED_PROTOCOL:
        case CURLE_URL_MALFORMAT:
            return D_CURL_STATUS_UNSUPPORTED_PROTOCOL;

        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_COULDNT_RESOLVE_PROXY:
            return D_CURL_STATUS_COULD_NOT_RESOLVE_HOST;

        case CURLE_COULDNT_CONNECT:
            return D_CURL_STATUS_COULD_NOT_CONNECT;

        case CURLE_OPERATION_TIMEDOUT:
            return D_CURL_STATUS_TIMED_OUT;

        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_PEER_FAILED_VERIFICATION:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CIPHER:
        case CURLE_USE_SSL_FAILED:
            return D_CURL_STATUS_TLS_ERROR;

        case CURLE_TOO_MANY_REDIRECTS:
            return D_CURL_STATUS_TOO_MANY_REDIRECTS;

        case CURLE_WRITE_ERROR:
            return D_CURL_STATUS_WRITE_ERROR;

        case CURLE_READ_ERROR:
            return D_CURL_STATUS_READ_ERROR;

        case CURLE_ABORTED_BY_CALLBACK:
            return D_CURL_STATUS_CANCELED;

        case CURLE_OUT_OF_MEMORY:
            return D_CURL_STATUS_OUT_OF_MEMORY;

        default:
            return D_CURL_STATUS_UNKNOWN;
    }
}

/*
d_internal_curl_on_body
  libcurl's write callback. Pieces go to the sink's body callback, or
nowhere. A refusal answers 0, which libcurl turns into CURLE_WRITE_ERROR.
*/
static size_t
d_internal_curl_on_body(
    char*  _data,
    size_t _size,
    size_t _count,
    void*  _sink
)
{
    const struct d_curl_sink* const sink  = _sink;
    const size_t                    total = _size * _count;

    // nothing to deliver, or no one to deliver it to
    if ( (total == 0)        ||
         (sink == NULL)      ||
         (sink->body == NULL) )
    {
        return total;
    }

    return (sink->body(sink->context,
                       _data,
                       total)) ? total : 0;
}

/*
d_internal_curl_split
  Splits one header line into a field's name and value: the name is
everything before the first colon, as sent; the value is everything after,
without its line ending or surrounding blanks. False for a line without a
colon -- the blank line ending a block -- and for a status line, which begins
"HTTP/" as no field name can, since "/" is not a token character.
*/
static bool
d_internal_curl_split(
    const char*         _line,
    size_t              _length,
    struct d_pack_text* _name,
    struct d_pack_text* _value
)
{
    size_t end = _length;

    // the line's own CR and LF
    while ( (end > 0) &&
            ( (_line[end - 1] == '\r') ||
              (_line[end - 1] == '\n') ) )
    {
        end--;
    }

    const char* const colon = memchr(_line,
                                     ':',
                                     end);

    // a status line, or no field at all
    if ( (colon == NULL) ||
         ( (end >= 5) &&
           (strncmp(_line,
                    "HTTP/",
                    5) == 0) ) )
    {
        return false;
    }

    size_t first = (size_t)(colon - _line) + 1;
    size_t last  = end;

    // blanks around the value
    while ( (first < last) &&
            ( (_line[first] == ' ') ||
              (_line[first] == '\t') ) )
    {
        first++;
    }

    while ( (last > first) &&
            ( (_line[last - 1] == ' ') ||
              (_line[last - 1] == '\t') ) )
    {
        last--;
    }

    _name->data    = _line;
    _name->length  = (size_t)(colon - _line);
    _value->data   = _line + first;
    _value->length = last - first;

    return true;
}

/*
d_internal_curl_on_header
  libcurl's header callback: one call per line, fields parsed by
d_internal_curl_split and handed to the sink's header callback. Lines that
are not fields pass without a call. A refusal answers 0, which libcurl turns
into CURLE_WRITE_ERROR.
*/
static size_t
d_internal_curl_on_header(
    char*  _line,
    size_t _size,
    size_t _count,
    void*  _sink
)
{
    const struct d_curl_sink* const sink  = _sink;
    const size_t                    total = _size * _count;
    struct d_pack_text              name  = { NULL, 0 };
    struct d_pack_text              value = { NULL, 0 };

    // no one to deliver to, or nothing that is a field
    if ( (sink == NULL)                       ||
         (sink->header == NULL)               ||
         (!d_internal_curl_split(_line,
                                 total,
                                 &name,
                                 &value)) )
    {
        return total;
    }

    return (sink->header(sink->context,
                         name,
                         value)) ? total : 0;
}

/*
d_internal_curl_append
  Appends one field to a list as "Name: value", the form libcurl takes.
libcurl copies the line, so it is freed at once. NULL if an allocation
fails, the list then untouched for the caller to free.
*/
static struct curl_slist*
d_internal_curl_append(
    struct curl_slist*          _list,
    const struct d_curl_header* _field
)
{
    const size_t name_length  = _field->name.length;
    const size_t value_length = _field->value.length;

    // a line too long to measure
    if (value_length > (SIZE_MAX - 3 - name_length))
    {
        return NULL;
    }

    char* const line = malloc(name_length + value_length + 3);

    // no memory for the line
    if (line == NULL)
    {
        return NULL;
    }

    memcpy(line,
           _field->name.data,
           name_length);
    line[name_length]     = ':';
    line[name_length + 1] = ' ';

    // an empty value has no bytes to copy
    if (value_length > 0)
    {
        memcpy(line + name_length + 2,
               _field->value.data,
               value_length);
    }

    line[name_length + 2 + value_length] = '\0';

    struct curl_slist* const next = curl_slist_append(_list,
                                                      line);

    free(line);

    return next;
}

/*
d_internal_curl_fields
  Builds libcurl's list of a request's header fields. NULL both for a request
without fields and after a failed allocation, so *_out_status tells the two
apart.
*/
static struct curl_slist*
d_internal_curl_fields(
    const struct d_curl_request* _request,
    enum d_curl_status*          _out_status
)
{
    struct curl_slist* list = NULL;

    *_out_status = D_CURL_STATUS_OK;

    // each field in order
    for (size_t i = 0; i < _request->header_count; i++)
    {
        struct curl_slist* const next =
            d_internal_curl_append(list,
                                   &_request->headers[i]);

        // a failed append leaves the list for this function to free
        if (next == NULL)
        {
            curl_slist_free_all(list);
            *_out_status = D_CURL_STATUS_OUT_OF_MEMORY;

            return NULL;
        }

        list = next;
    }

    return list;
}

/*
d_internal_curl_then_long
  Sets a long option unless an earlier setting failed, so that a chain of
settings reports its first failure.
*/
static CURLcode
d_internal_curl_then_long(
    CURL*      _easy,
    CURLcode   _prior,
    CURLoption _option,
    long       _value
)
{
    return (_prior != CURLE_OK) ? _prior
                                : curl_easy_setopt(_easy,
                                                   _option,
                                                   _value);
}

/*
d_internal_curl_then_text
  Sets a text option unless an earlier setting failed; libcurl copies the
text.
*/
static CURLcode
d_internal_curl_then_text(
    CURL*       _easy,
    CURLcode    _prior,
    CURLoption  _option,
    const char* _text
)
{
    return (_prior != CURLE_OK) ? _prior
                                : curl_easy_setopt(_easy,
                                                   _option,
                                                   _text);
}

/*
d_internal_curl_configure_connection
  How the connection behaves: no signals, the two timeouts (0 keeps
libcurl's), verbosity, and the proxy when the options name one.
*/
static CURLcode
d_internal_curl_configure_connection(
    CURL*                        _easy,
    const struct d_curl_options* _options
)
{
    CURLcode code = curl_easy_setopt(_easy,
                                     CURLOPT_NOSIGNAL,
                                     1L);

    code = d_internal_curl_then_long(_easy,
                                     code,
                                     CURLOPT_TIMEOUT_MS,
                                     _options->timeout_ms);
    code = d_internal_curl_then_long(_easy,
                                     code,
                                     CURLOPT_CONNECTTIMEOUT_MS,
                                     _options->connect_timeout_ms);
    code = d_internal_curl_then_long(_easy,
                                     code,
                                     CURLOPT_VERBOSE,
                                     (_options->verbose) ? 1L : 0L);

    // "" disables proxies, whatever the environment says; NULL keeps it
    if (_options->proxy != NULL)
    {
        code = d_internal_curl_then_text(_easy,
                                         code,
                                         CURLOPT_PROXY,
                                         _options->proxy);
    }

    return code;
}

/*
d_internal_curl_configure_policy
  What the transfer accepts: redirects, TLS verification of the peer and of
its name, the User-Agent, and the codings to decode.
*/
static CURLcode
d_internal_curl_configure_policy(
    CURL*                        _easy,
    const struct d_curl_options* _options
)
{
    const char* const agent = (_options->user_agent != NULL)
                                  ? _options->user_agent
                                  : D_CURL_USER_AGENT_DEFAULT;
    CURLcode          code  = curl_easy_setopt(
                                  _easy,
                                  CURLOPT_FOLLOWLOCATION,
                                  (_options->follow_redirects) ? 1L : 0L);

    code = d_internal_curl_then_long(_easy,
                                     code,
                                     CURLOPT_MAXREDIRS,
                                     _options->max_redirects);
    code = d_internal_curl_then_long(_easy,
                                     code,
                                     CURLOPT_SSL_VERIFYPEER,
                                     (_options->verify_tls) ? 1L : 0L);
    code = d_internal_curl_then_long(_easy,
                                     code,
                                     CURLOPT_SSL_VERIFYHOST,
                                     (_options->verify_tls) ? 2L : 0L);
    code = d_internal_curl_then_text(_easy,
                                     code,
                                     CURLOPT_USERAGENT,
                                     agent);

    // NULL leaves decoding off; "" accepts every coding libcurl knows
    if (_options->accept_encoding != NULL)
    {
        code = d_internal_curl_then_text(_easy,
                                         code,
                                         CURLOPT_ACCEPT_ENCODING,
                                         _options->accept_encoding);
    }

    return code;
}

/*
d_internal_curl_configure_request
  What to send: the URL, the method, the body, and the header fields. HEAD
goes through CURLOPT_NOBODY, as libcurl asks, so it waits for no body; any
other named method is sent as given. A body is copied, its size set first so
that binary bodies survive.
*/
static CURLcode
d_internal_curl_configure_request(
    CURL*                        _easy,
    const struct d_curl_request* _request,
    struct curl_slist*           _fields
)
{
    const char* const method = _request->method;
    CURLcode          code   = curl_easy_setopt(_easy,
                                                CURLOPT_URL,
                                                _request->url);

    // HEAD asks for no body; other methods go as named
    if ( (method != NULL) &&
         (strcmp(method,
                 "HEAD") == 0) )
    {
        code = d_internal_curl_then_long(_easy,
                                         code,
                                         CURLOPT_NOBODY,
                                         1L);
    }
    else if (method != NULL)
    {
        code = d_internal_curl_then_text(_easy,
                                         code,
                                         CURLOPT_CUSTOMREQUEST,
                                         method);
    }

    // the body, copied
    if ( (code == CURLE_OK) &&
         (_request->body.size > 0) )
    {
        code = curl_easy_setopt(_easy,
                                CURLOPT_POSTFIELDSIZE_LARGE,
                                (curl_off_t)_request->body.size);
        code = (code == CURLE_OK) ? curl_easy_setopt(_easy,
                                                     CURLOPT_COPYPOSTFIELDS,
                                                     _request->body.data)
                                  : code;
    }

    // the fields, when there are any
    if ( (code == CURLE_OK) &&
         (_fields != NULL) )
    {
        code = curl_easy_setopt(_easy,
                                CURLOPT_HTTPHEADER,
                                _fields);
    }

    return code;
}

/*
d_internal_curl_configure_sink
  Where the response goes. Both callbacks are always installed: without a
write callback libcurl would print the body to stdout.
*/
static CURLcode
d_internal_curl_configure_sink(
    CURL*                     _easy,
    const struct d_curl_sink* _sink
)
{
    CURLcode code = curl_easy_setopt(_easy,
                                     CURLOPT_WRITEFUNCTION,
                                     d_internal_curl_on_body);

    code = (code == CURLE_OK) ? curl_easy_setopt(_easy,
                                                 CURLOPT_WRITEDATA,
                                                 _sink)
                              : code;
    code = (code == CURLE_OK) ? curl_easy_setopt(_easy,
                                                 CURLOPT_HEADERFUNCTION,
                                                 d_internal_curl_on_header)
                              : code;
    code = (code == CURLE_OK) ? curl_easy_setopt(_easy,
                                                 CURLOPT_HEADERDATA,
                                                 _sink)
                              : code;

    return code;
}

/*
d_internal_curl_transfer
  Runs one validated request on a fresh easy handle and fills _result. The
response code is read even after a failure, since a sink may end a transfer
whose response had already begun.
*/
static void
d_internal_curl_transfer(
    const struct d_curl_request* _request,
    const struct d_curl_options* _options,
    const struct d_curl_sink*    _sink,
    struct d_curl_result*        _result
)
{
    CURL* const easy = curl_easy_init();

    // libcurl could not allocate a handle
    if (easy == NULL)
    {
        _result->status = D_CURL_STATUS_OUT_OF_MEMORY;

        return;
    }

    enum d_curl_status       status = D_CURL_STATUS_OK;
    struct curl_slist* const fields = d_internal_curl_fields(_request,
                                                             &status);
    CURLcode                 code   =
        d_internal_curl_configure_connection(easy,
                                             _options);

    code = (code == CURLE_OK) ? d_internal_curl_configure_policy(easy,
                                                                 _options)
                              : code;
    code = (code == CURLE_OK) ? d_internal_curl_configure_request(easy,
                                                                  _request,
                                                                  fields)
                              : code;
    code = (code == CURLE_OK) ? d_internal_curl_configure_sink(easy,
                                                               _sink)
                              : code;

    // a transfer only when every setting took and the fields were built
    if ( (code == CURLE_OK) &&
         (status == D_CURL_STATUS_OK) )
    {
        code = curl_easy_perform(easy);
    }

    long http_status = 0;

    (void)curl_easy_getinfo(easy,
                            CURLINFO_RESPONSE_CODE,
                            &http_status);
    _result->status      = (status != D_CURL_STATUS_OK)
                               ? status
                               : d_internal_curl_status(code);
    _result->code        = (int)code;
    _result->http_status = http_status;
    curl_slist_free_all(fields);
    curl_easy_cleanup(easy);

    return;
}

/*
d_internal_curl_feature_bit
  libcurl's bit for a feature, 0 for one the headers predate or the enum
does not list.
*/
static int
d_internal_curl_feature_bit(
    enum d_curl_feature _feature
)
{
    switch (_feature)
    {
        case D_CURL_FEATURE_SSL:
            return CURL_VERSION_SSL;

        case D_CURL_FEATURE_HTTP2:
            return D_INTERNAL_CURL_BIT_HTTP2;

        case D_CURL_FEATURE_HTTP3:
            return D_INTERNAL_CURL_BIT_HTTP3;

        case D_CURL_FEATURE_IPV6:
            return CURL_VERSION_IPV6;

        case D_CURL_FEATURE_LIBZ:
            return CURL_VERSION_LIBZ;

        case D_CURL_FEATURE_BROTLI:
            return D_INTERNAL_CURL_BIT_BROTLI;

        case D_CURL_FEATURE_ZSTD:
            return D_INTERNAL_CURL_BIT_ZSTD;

        case D_CURL_FEATURE_ASYNCH_DNS:
            return CURL_VERSION_ASYNCHDNS;

        case D_CURL_FEATURE_THREADSAFE:
            return D_INTERNAL_CURL_BIT_THREADSAFE;

        default:
            return 0;
    }
}

/*
d_curl_code_message
  libcurl's own text for its code.
*/
const char*
d_curl_code_message(
    int _code
)
{
    return curl_easy_strerror((CURLcode)_code);
}

/*
d_curl_version
  The version text of the libcurl loaded at run time, which may be newer than
the headers the build used.
*/
const char*
d_curl_version(void)
{
    const curl_version_info_data* const info =
        curl_version_info(CURLVERSION_NOW);

    return (info != NULL) ? info->version : NULL;
}

/*
d_curl_version_number
  The loaded libcurl's version as 0xMMmmpp.
*/
uint32_t
d_curl_version_number(void)
{
    const curl_version_info_data* const info =
        curl_version_info(CURLVERSION_NOW);

    return (info != NULL) ? (uint32_t)info->version_num : 0;
}

/*
d_curl_supports
  Tests the loaded libcurl's feature bits. A feature without a bit in these
headers reads as unsupported, however new the loaded library is.
*/
bool
d_curl_supports(
    enum d_curl_feature _feature
)
{
    const int                           bit  =
        d_internal_curl_feature_bit(_feature);
    const curl_version_info_data* const info =
        curl_version_info(CURLVERSION_NOW);

    return ( (bit != 0)     &&
             (info != NULL) &&
             ((info->features & bit) != 0) );
}

/*
d_curl_global_init
  libcurl counts its own initializations, so each call is simply passed on.
*/
enum d_curl_status
d_curl_global_init(void)
{
    return d_internal_curl_status(curl_global_init(CURL_GLOBAL_ALL));
}

/*
d_curl_global_cleanup
  Balances one d_curl_global_init.
*/
void
d_curl_global_cleanup(void)
{
    curl_global_cleanup();

    return;
}

/*
d_curl_perform
  Validates, fills in the default options where none are given, and runs the
transfer.
*/
enum d_curl_status
d_curl_perform(
    const struct d_curl_request* _request,
    const struct d_curl_options* _options,
    const struct d_curl_sink*    _sink,
    struct d_curl_result*        _out
)
{
    // parameter validation
    if (!d_internal_curl_request_ok(_request))
    {
        const struct d_curl_result refused =
        {
            D_CURL_STATUS_INVALID_ARGUMENT,
            0,
            0
        };

        return d_internal_curl_report(_out,
                                      &refused);
    }

    struct d_curl_options defaults = { .timeout_ms = 0 };
    struct d_curl_result  result   = { D_CURL_STATUS_OK, 0, 0 };

    d_curl_options_init(&defaults);
    d_internal_curl_transfer(_request,
                             (_options != NULL) ? _options : &defaults,
                             _sink,
                             &result);

    return d_internal_curl_report(_out,
                                  &result);
}

#else

/*
d_curl_code_message
  Without libcurl there is no text to give.
*/
const char*
d_curl_code_message(
    int _code
)
{
    (void)_code;

    return NULL;
}

/*
d_curl_version
  Without libcurl there is no version.
*/
const char*
d_curl_version(void)
{
    return NULL;
}

/*
d_curl_version_number
  Without libcurl the version is 0.
*/
uint32_t
d_curl_version_number(void)
{
    return 0;
}

/*
d_curl_supports
  Without libcurl nothing is supported.
*/
bool
d_curl_supports(
    enum d_curl_feature _feature
)
{
    (void)_feature;

    return false;
}

/*
d_curl_global_init
  Without libcurl there is nothing to initialize.
*/
enum d_curl_status
d_curl_global_init(void)
{
    return D_CURL_STATUS_UNSUPPORTED;
}

/*
d_curl_global_cleanup
  Without libcurl there is nothing to release.
*/
void
d_curl_global_cleanup(void)
{
    return;
}

/*
d_curl_perform
  Checks the arguments as the libcurl build does, then reports UNSUPPORTED.
*/
enum d_curl_status
d_curl_perform(
    const struct d_curl_request* _request,
    const struct d_curl_options* _options,
    const struct d_curl_sink*    _sink,
    struct d_curl_result*        _out
)
{
    const struct d_curl_result result =
    {
        (d_internal_curl_request_ok(_request))
            ? D_CURL_STATUS_UNSUPPORTED
            : D_CURL_STATUS_INVALID_ARGUMENT,
        0,
        0
    };

    (void)_options;
    (void)_sink;

    return d_internal_curl_report(_out,
                                  &result);
}

#endif  // D_INTERNAL_CURL
