/*******************************************************************************
* djinterp [net]                                           http_tests_sa_http1.c
*
* Standalone tests of http1.h: HTTP/1.1 message heads.
*   Every refusal the header names has a case, each expecting its own error.
* Every case is also fed one byte at a time, and must reach the verdict and
* the length its whole parse does: the resumable scan's promise.
*
*
* path:      /tests/djinterp/net/http/http_tests_sa_http1.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./http_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp, memset, strlen
// djinterp
#include "../../../../inc/djinterp/net/http/http1.h"  // the module


// d_tests_http1_case
//   struct: a head's text, and the error its parse must give.
struct d_tests_http1_case
{
    const char*       text;
    enum d_http_error error;
};

// REQUESTS
//   constant: request heads, accepted and refused.
static const struct d_tests_http1_case REQUESTS[] =
{
    { "GET / HTTP/1.1\r\nHost: a\r\n\r\n",                 D_HTTP_OK },
    { "\r\n\r\nGET / HTTP/1.1\r\nHost: a\r\n\r\n",         D_HTTP_OK },
    { "GET / HTTP/1.0\r\n\r\n",                            D_HTTP_OK },
    { "OPTIONS * HTTP/1.1\r\nHost: a\r\n\r\n",             D_HTTP_OK },
    { "CONNECT a.example:443 HTTP/1.1\r\nHost: a\r\n\r\n", D_HTTP_OK },
    { "GET http://a/x HTTP/1.1\r\nHost: a\r\n\r\n",        D_HTTP_OK },
    { "PROPFIND /x HTTP/1.1\r\nHost: a\r\n\r\n",           D_HTTP_OK },
    { "GET / HTTP/1.1\r\nHost:\r\n\r\n",                   D_HTTP_OK },
    { "GET / HTTP/1.1\r\n\r\n",                            D_HTTP_ERROR_HOST },
    { "GET / HTTP/1.1\r\nHost: a\r\nHOST: b\r\n\r\n",      D_HTTP_ERROR_HOST },
    { "GET / HTTP/1.1\r\nHost: u@a\r\n\r\n",               D_HTTP_ERROR_HOST },
    { "GET / HTTP/1.1\nHost: a\r\n\r\n",
      D_HTTP_ERROR_LINE_ENDING },
    { "GET / HTTP/1.1\r\nHost: a\rb\r\n\r\n",
      D_HTTP_ERROR_LINE_ENDING },
    { "GET /\r HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_LINE_ENDING },
    { "GET / HTTP/1.1\r\nHost: a\r\n X: y\r\n\r\n",
      D_HTTP_ERROR_OBS_FOLD },
    { "GET / HTTP/1.1\r\nHost : a\r\n\r\n",
      D_HTTP_ERROR_FIELD_NAME },
    { "GET / HTTP/1.1\r\nHost a\r\n\r\n",
      D_HTTP_ERROR_FIELD_NAME },
    { "GET / HTTP/1.1\r\nHost: a\x01\r\n\r\n",
      D_HTTP_ERROR_FIELD_VALUE },
    { "GET  / HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_START_LINE },
    { "GET / HTTP/1.1 \r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_START_LINE },
    { "GET /\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_START_LINE },
    { "G@T / HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_METHOD },
    { "GET / HTTP/2.0\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_VERSION },
    { "GET x HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_TARGET },
    { "GET /a#f HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_TARGET },
    { "GET * HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_TARGET },
    { "GET //a HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_TARGET },
    { "CONNECT /x HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_TARGET },
    { "CONNECT a.example HTTP/1.1\r\nHost: a\r\n\r\n",
      D_HTTP_ERROR_TARGET },
    { "GET / HTTP/1.1\r\nHost: a\r\n",
      D_HTTP_ERROR_INCOMPLETE },
    { "GET / HTT",
      D_HTTP_ERROR_INCOMPLETE }
};

// RESPONSES
//   constant: response heads, accepted and refused.
static const struct d_tests_http1_case RESPONSES[] =
{
    { "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", D_HTTP_OK },
    { "HTTP/1.1 204 \r\n\r\n",                        D_HTTP_OK },
    { "HTTP/1.1 204\r\n\r\n",                         D_HTTP_OK },
    { "HTTP/1.0 404 Not Found\r\n\r\n",               D_HTTP_OK },
    { "HTTP/1.1 200 Very\tOK\r\n\r\n",                D_HTTP_OK },
    { "\r\nHTTP/1.1 200 OK\r\n\r\n",                  D_HTTP_ERROR_START_LINE },
    { "HTTP/1.1 2000 OK\r\n\r\n",                     D_HTTP_ERROR_START_LINE },
    { "HTTP/1.1 200 OK\x01\r\n\r\n",                  D_HTTP_ERROR_START_LINE },
    { "HTTP/2 200 OK\r\n\r\n",                        D_HTTP_ERROR_START_LINE },
    { "HTTP/1.x 200 OK\r\n\r\n",                      D_HTTP_ERROR_VERSION },
    { "HTTP/1.1 600 X\r\n\r\n",                       D_HTTP_ERROR_STATUS },
    { "HTTP/1.1 200 OK\r\nX: y\r\n z\r\n\r\n",        D_HTTP_ERROR_OBS_FOLD },
    { "HTTP/1.1 200 OK\r\nX: y\n\r\n",
      D_HTTP_ERROR_LINE_ENDING },
    { "HTTP/1.1 200 OK\r\n",                          D_HTTP_ERROR_INCOMPLETE }
};

/*
d_tests_http1_text
  A view of a C string.
*/
static struct d_pack_text
d_tests_http1_text(
    const char* _text
)
{
    const struct d_pack_text view = { _text,
                                      strlen(_text) };

    return view;
}

/*
d_tests_http1_request
  Parses a request head whole, into a fresh parser and eight fields.
*/
static enum d_http_error
d_tests_http1_request(
    struct d_pack_text      _text,
    struct d_http1_request* _out,
    struct d_http1_field*   _items,
    size_t*                 _consumed
)
{
    struct d_http1_parser parser = { .line = 0u };

    d_http1_parser_init(&parser);
    memset(_out,
           0,
           sizeof(*_out));
    _out->fields.items    = _items;
    _out->fields.capacity = 8u;

    return d_http1_parse_request(&parser,
                                 _text,
                                 _out,
                                 _consumed);
}

/*
d_tests_http1_response
  Parses a response head whole, into a fresh parser and eight fields.
*/
static enum d_http_error
d_tests_http1_response(
    struct d_pack_text       _text,
    struct d_http1_response* _out,
    struct d_http1_field*    _items,
    size_t*                  _consumed
)
{
    struct d_http1_parser parser = { .line = 0u };

    d_http1_parser_init(&parser);
    memset(_out,
           0,
           sizeof(*_out));
    _out->fields.items    = _items;
    _out->fields.capacity = 8u;

    return d_http1_parse_response(&parser,
                                  _text,
                                  _out,
                                  _consumed);
}

/*
d_tests_http1_right
  How many cases of a table parse as listed, as requests or as responses.
*/
static size_t
d_tests_http1_right(
    const struct d_tests_http1_case* _cases,
    size_t                           _count,
    bool                             _requests
)
{
    size_t right = 0u;

    // each case, whole
    for (size_t i = 0u; i < _count; ++i)
    {
        struct d_http1_field    items[8];
        struct d_http1_request  request;
        struct d_http1_response response;
        size_t                  used  = 0u;
        const struct d_pack_text text = d_tests_http1_text(_cases[i].text);

        const enum d_http_error found = (_requests)
            ? d_tests_http1_request(text,
                                    &request,
                                    items,
                                    &used)
            : d_tests_http1_response(text,
                                     &response,
                                     items,
                                     &used);

        right += (found == _cases[i].error) ? 1u
                                            : 0u;
    }

    return right;
}

/*
d_tests_sa_http1_heads
  Tests the following:
  - every case of REQUESTS and RESPONSES parses as listed, whole
*/
bool
d_tests_sa_http1_heads(
    struct d_test_counter* _counter
)
{
    const size_t requests  = sizeof(REQUESTS) / sizeof(REQUESTS[0]);
    const size_t responses = sizeof(RESPONSES) / sizeof(RESPONSES[0]);
    bool         result    = true;

    result = d_assert_standalone(
                 d_tests_http1_right(REQUESTS,
                                     requests,
                                     true) == requests,
                 "d_http1_parse_request",
                 "every request case gives its error",
                 _counter) && result;
    result = d_assert_standalone(
                 d_tests_http1_right(RESPONSES,
                                     responses,
                                     false) == responses,
                 "d_http1_parse_response",
                 "every response case gives its error",
                 _counter) && result;

    return result;
}

/*
d_tests_http1_text_is
  Whether a view holds exactly a C string.
*/
static bool
d_tests_http1_text_is(
    struct d_pack_text _text,
    const char*        _expected
)
{
    const size_t length = strlen(_expected);

    return ( (_text.length == length) &&
             ( (length == 0u) ||
               (memcmp(_text.data,
                       _expected,
                       length) == 0) ) );
}

/*
d_tests_http1_forms_hold
  Whether CONNECT's target is authority-form, and an extension method keeps
its name.
*/
static bool
d_tests_http1_forms_hold(void)
{
    struct d_http1_field   items[8];
    struct d_http1_request request;
    size_t                 used = 0u;

    const bool connect   = ( (d_tests_http1_request(
                                  d_tests_http1_text(REQUESTS[4].text),
                                  &request,
                                  items,
                                  &used) == D_HTTP_OK) &&
                             (request.target_form ==
                              D_HTTP1_TARGET_AUTHORITY) );
    const bool extension = ( (d_tests_http1_request(
                                  d_tests_http1_text(REQUESTS[6].text),
                                  &request,
                                  items,
                                  &used) == D_HTTP_OK)                 &&
                             (request.method == D_HTTP_METHOD_OTHER)   &&
                             (d_tests_http1_text_is(request.method_name,
                                                    "PROPFIND")) );

    return ( (connect) &&
             (extension) );
}

/*
d_tests_sa_http1_parts
  Tests the following:
  - a request's method, name, target, form, version, and trimmed fields;
    its length stops where a pipelined request begins
  - an extension method keeps its name; each target form is recognized
  - a response's version, status, reason, and fields
*/
bool
d_tests_sa_http1_parts(
    struct d_test_counter* _counter
)
{
    const char* const       text     = "GET /a?b HTTP/1.1\r\nHost: h\r\n"
                                       "X-Y:  z  \r\n\r\nGET /next";
    struct d_http1_field    items[8];
    struct d_http1_request  request;
    struct d_http1_response response;
    size_t                  used     = 0u;
    bool                    result   = true;

    const bool request_ok = ( (d_tests_http1_request(
                                   d_tests_http1_text(text),
                                   &request,
                                   items,
                                   &used) == D_HTTP_OK)                  &&
                              (used == strlen(text) - strlen("GET /next")) &&
                              (request.method == D_HTTP_METHOD_GET)       &&
                              (d_tests_http1_text_is(request.target,
                                                     "/a?b"))             &&
                              (request.target_form ==
                               D_HTTP1_TARGET_ORIGIN)                     &&
                              (request.version == D_HTTP_VERSION_1_1)     &&
                              (request.fields.count == 2u)                &&
                              (d_tests_http1_text_is(items[1].value,
                                                     "z")) );
    const bool response_ok = ( (d_tests_http1_response(
                                    d_tests_http1_text(RESPONSES[3].text),
                                    &response,
                                    items,
                                    &used) == D_HTTP_OK)                 &&
                               (response.version == D_HTTP_VERSION_1_0)  &&
                               (response.status == 404u)                 &&
                               (d_tests_http1_text_is(response.reason,
                                                      "Not Found"))      &&
                               (response.fields.count == 0u) );

    result = d_assert_standalone(request_ok,
                                 "d_http1_parse_request",
                                 "a request's parts, before pipelined bytes",
                                 _counter) && result;
    result = d_assert_standalone(d_tests_http1_forms_hold(),
                                 "d_http1_parse_request",
                                 "target forms, and an extension's name",
                                 _counter) && result;
    result = d_assert_standalone(response_ok,
                                 "d_http1_parse_response",
                                 "a response's parts",
                                 _counter) && result;

    return result;
}

/*
d_tests_http1_fed
  Whether a case fed one byte at a time, into one parser, reaches the
verdict and length its whole parse does, answering INCOMPLETE until then.
*/
static bool
d_tests_http1_fed(
    const struct d_tests_http1_case* _case,
    bool                             _requests
)
{
    const struct d_pack_text all    = d_tests_http1_text(_case->text);
    struct d_http1_parser    parser = { .line = 0u };
    struct d_http1_field     items[8];
    struct d_http1_request   request;
    struct d_http1_response  response;
    size_t                   whole  = 0u;
    size_t                   used   = 0u;
    enum d_http_error        found  = D_HTTP_ERROR_INCOMPLETE;

    const enum d_http_error expected = (_requests)
        ? d_tests_http1_request(all,
                                &request,
                                items,
                                &whole)
        : d_tests_http1_response(all,
                                 &response,
                                 items,
                                 &whole);

    d_http1_parser_init(&parser);
    request.fields  = (struct d_http1_fields){ items, 8u, 0u };
    response.fields = (struct d_http1_fields){ items, 8u, 0u };

    // one more byte each time, until a verdict
    for (size_t n = 0u; ( (n <= all.length) &&
                          (found == D_HTTP_ERROR_INCOMPLETE) ); ++n)
    {
        const struct d_pack_text part = { all.data,
                                          n };

        found = (_requests) ? d_http1_parse_request(&parser,
                                                    part,
                                                    &request,
                                                    &used)
                            : d_http1_parse_response(&parser,
                                                     part,
                                                     &response,
                                                     &used);
    }

    return ( (found == expected) &&
             ( (found != D_HTTP_OK) ||
               (used == whole) ) );
}

/*
d_tests_sa_http1_incremental
  Tests the following:
  - every case of both tables, fed a byte at a time, reaches the verdict
    and the length its whole parse does
*/
bool
d_tests_sa_http1_incremental(
    struct d_test_counter* _counter
)
{
    const size_t requests  = sizeof(REQUESTS) / sizeof(REQUESTS[0]);
    const size_t responses = sizeof(RESPONSES) / sizeof(RESPONSES[0]);
    size_t       agreed    = 0u;

    // each request case, then each response case
    for (size_t i = 0u; i < requests; ++i)
    {
        agreed += (d_tests_http1_fed(&REQUESTS[i],
                                     true)) ? 1u
                                            : 0u;
    }

    // each response case
    for (size_t i = 0u; i < responses; ++i)
    {
        agreed += (d_tests_http1_fed(&RESPONSES[i],
                                     false)) ? 1u
                                             : 0u;
    }

    return d_assert_standalone(agreed == requests + responses,
                               "d_http1_parse_request",
                               "a byte at a time agrees with the whole",
                               _counter);
}

// d_tests_http1_limit_case
//   struct: a request, the limits and field room to parse it with, and the
// error that must give. "GET /abc HTTP/1.0" is 17 bytes, and its head 27.
struct d_tests_http1_limit_case
{
    const char*       text;
    size_t            start_line_max;
    size_t            head_max;
    size_t            capacity;
    enum d_http_error error;
};

// LIMITS
//   constant: each limit at its bound and one past it, ended and open.
static const struct d_tests_http1_limit_case LIMITS[] =
{
    { "GET /abc HTTP/1.0\r\nX: y\r\n\r\n",  17u, 1024u, 8u, D_HTTP_OK },
    { "GET /abc HTTP/1.0\r\nX: y\r\n\r\n",  16u, 1024u, 8u,
      D_HTTP_ERROR_START_LINE_TOO_LONG },
    { "GET /abcdefghijklmnop",               64u, 1024u, 8u,
      D_HTTP_ERROR_INCOMPLETE },
    { "GET /abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijk",
      64u, 1024u, 8u, D_HTTP_ERROR_START_LINE_TOO_LONG },
    { "GET /abc HTTP/1.0\r\nX: y\r\n\r\n",  64u, 27u, 8u, D_HTTP_OK },
    { "GET /abc HTTP/1.0\r\nX: y\r\n\r\n",  64u, 26u, 8u,
      D_HTTP_ERROR_HEAD_TOO_LARGE },
    { "GET / HTTP/1.0\r\nX: yyyyyyyyyy",      64u, 20u, 8u,
      D_HTTP_ERROR_HEAD_TOO_LARGE },
    { "GET /abc HTTP/1.0\r\nX: y\r\n\r\n",  64u, 1024u, 0u,
      D_HTTP_ERROR_TOO_MANY_FIELDS },
    { "GET / HTTP/1.0\r\n\r\n",              64u, 1024u, 0u, D_HTTP_OK },
    { "GET /abcdef\n",                        4u, 1024u, 8u,
      D_HTTP_ERROR_START_LINE_TOO_LONG }
};

/*
d_tests_http1_limited
  A limit case's request parsed whole with its limits and field room.
*/
static enum d_http_error
d_tests_http1_limited(
    const struct d_tests_http1_limit_case* _case
)
{
    struct d_http1_parser  parser  = { .line = 0u };
    struct d_http1_field   items[8];
    struct d_http1_request request = { .fields = { items,
                                                   _case->capacity,
                                                   0u } };
    size_t                 used    = 0u;

    d_http1_parser_init(&parser);
    parser.start_line_max = _case->start_line_max;
    parser.head_max       = _case->head_max;

    return d_http1_parse_request(&parser,
                                 d_tests_http1_text(_case->text),
                                 &request,
                                 &used);
}

/*
d_tests_http1_limited_fed
  A limit case's request fed one byte at a time, with its limits: the
verdict it reaches.
*/
static enum d_http_error
d_tests_http1_limited_fed(
    const struct d_tests_http1_limit_case* _case
)
{
    const struct d_pack_text all     = d_tests_http1_text(_case->text);
    struct d_http1_parser    parser  = { .line = 0u };
    struct d_http1_field     items[8];
    struct d_http1_request   request = { .fields = { items,
                                                     _case->capacity,
                                                     0u } };
    size_t                   used    = 0u;
    enum d_http_error        found   = D_HTTP_ERROR_INCOMPLETE;

    d_http1_parser_init(&parser);
    parser.start_line_max = _case->start_line_max;
    parser.head_max       = _case->head_max;

    // one more byte each time, until a verdict
    for (size_t n = 0u; ( (n <= all.length) &&
                          (found == D_HTTP_ERROR_INCOMPLETE) ); ++n)
    {
        const struct d_pack_text part = { all.data,
                                          n };

        found = d_http1_parse_request(&parser,
                                      part,
                                      &request,
                                      &used);
    }

    return found;
}

/*
d_tests_sa_http1_limits
  Tests the following:
  - a start line over its limit, ended or still open, and one exactly at it
  - a head over its limit, ended or still open, and one exactly at it
  - more fields than room; no room and no fields is fine
  - each case fed a byte at a time reaches the same verdict; the last case
    is the fuzzer's find, a long start line ending in a bare LF, where the
    start line's limit comes first in the bytes
*/
bool
d_tests_sa_http1_limits(
    struct d_test_counter* _counter
)
{
    const size_t count = sizeof(LIMITS) / sizeof(LIMITS[0]);
    size_t       right = 0u;

    // each case, with its own limits
    for (size_t i = 0u; i < count; ++i)
    {
        right += ( (d_tests_http1_limited(&LIMITS[i]) == LIMITS[i].error) &&
                   (d_tests_http1_limited_fed(&LIMITS[i]) ==
                    LIMITS[i].error) ) ? 1u
                                       : 0u;
    }

    return d_assert_standalone(right == count,
                               "d_http1_parse_request",
                               "every limit, at its bound and past it",
                               _counter);
}

/*
d_tests_sa_http1_lookup
  Tests the following:
  - a field found without case; the next of a repeated one; none missing
  - NULL arguments refused
*/
bool
d_tests_sa_http1_lookup(
    struct d_test_counter* _counter
)
{
    struct d_http1_field   items[8];
    struct d_http1_request request;
    struct d_http1_parser  parser = { .line = 0u };
    size_t                 used   = 0u;
    bool                   result = true;

    (void)d_tests_http1_request(
        d_tests_http1_text("GET / HTTP/1.1\r\nHost: h\r\nAccept: a\r\n"
                           "accept: b\r\n\r\n"),
        &request,
        items,
        &used);
    d_http1_parser_init(&parser);

    result = d_assert_standalone(
                 (d_http1_fields_index(&request.fields,
                                       d_tests_http1_text("ACCEPT"),
                                       0u) == 1u)                     &&
                 (d_http1_fields_index(&request.fields,
                                       d_tests_http1_text("Accept"),
                                       2u) == 2u)                     &&
                 (d_http1_fields_index(&request.fields,
                                       d_tests_http1_text("Cookie"),
                                       0u) == request.fields.count),
                 "d_http1_fields_index",
                 "found without case, repeated, and missing",
                 _counter) && result;
    result = d_assert_standalone(
                 (d_http1_parse_request(NULL,
                                        d_tests_http1_text("x"),
                                        &request,
                                        &used) ==
                  D_HTTP_ERROR_ARGUMENT)                              &&
                 (d_http1_parse_response(&parser,
                                         d_tests_http1_text("x"),
                                         NULL,
                                         &used) ==
                  D_HTTP_ERROR_ARGUMENT),
                 "d_http1_parse_request",
                 "NULL arguments are refused",
                 _counter) && result;

    return result;
}
