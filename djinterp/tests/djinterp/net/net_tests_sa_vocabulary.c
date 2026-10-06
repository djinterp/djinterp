/*******************************************************************************
* djinterp [net]                                       net_tests_sa_vocabulary.c
*
* Tests of the net foundation's pure functions.
*   Error phrases, result helpers, port and endpoint parsing and formatting,
* and the frame header codec. Nothing here touches a connection.
*
*
* path:      /tests/djinterp/net/net_tests_sa_vocabulary.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "./net_tests_sa.h"  // corresponding header
// std
#include <stdint.h>  // SIZE_MAX, uint32_t
#include <string.h>  // memcmp, memset, strcmp, strlen


/*
d_tests_net_text
  Spans a NUL-terminated literal.
*/
static struct d_pack_text
d_tests_net_text(
    const char* _text
)
{
    const struct d_pack_text text = { _text, strlen(_text) };

    return text;
}

/*
d_tests_net_text_is
  Tells whether a span holds exactly the given literal.
*/
static bool
d_tests_net_text_is(
    struct d_pack_text _text,
    const char*        _expected
)
{
    const size_t length = strlen(_expected);

    return ( (_text.length == length) &&
             (memcmp(_text.data,
                     _expected,
                     length) == 0) );
}

/*
d_tests_net_phrase_is
  Tells whether an error's phrase is the given one.
*/
static bool
d_tests_net_phrase_is(
    enum d_net_error _error,
    const char*      _expected
)
{
    return (strcmp(d_net_error_string(_error),
                   _expected) == 0);
}

/*
d_tests_net_fill_host
  Fills a buffer with `_length` copies of 'a' and a terminator.
*/
static void
d_tests_net_fill_host(
    char*  _buffer,
    size_t _length
)
{
    memset(_buffer,
           'a',
           _length);
    _buffer[_length] = '\0';

    return;
}

/*
d_tests_sa_net_errors
  Tests the following:
  - every error has a phrase, and no two share one
  - the phrases are the ones the C++ layer has always returned
  - a value outside the enum reads as "unknown error"
*/
bool
d_tests_sa_net_errors(
    struct d_test_counter* _counter
)
{
    const char* const TEST     = "d_net_error_string";
    bool              distinct = true;
    bool              result   = true;

    // compare every pair of enumerators' phrases
    for (int i = 0; i <= (int)D_NET_ERROR_UNKNOWN; ++i)
    {
        for (int j = i + 1; j <= (int)D_NET_ERROR_UNKNOWN; ++j)
        {
            const char* const a = d_net_error_string((enum d_net_error)i);
            const char* const b = d_net_error_string((enum d_net_error)j);

            distinct = ( (distinct) &&
                         (a != NULL) &&
                         (b != NULL) &&
                         (strcmp(a,
                                 b) != 0) );
        }
    }

    const bool phrases = ( (d_tests_net_phrase_is(D_NET_ERROR_NONE,
                                                  "none")) &&
                           (d_tests_net_phrase_is(D_NET_ERROR_WOULD_BLOCK,
                                                  "would block")) &&
                           (d_tests_net_phrase_is(
                                D_NET_ERROR_TOO_MANY_OPEN_FILES,
                                "too many open files")) &&
                           (d_tests_net_phrase_is(D_NET_ERROR_UNKNOWN,
                                                  "unknown error")) );
    const bool outside = d_tests_net_phrase_is((enum d_net_error)99,
                                               "unknown error");

    result = d_assert_standalone(distinct,
                                 TEST,
                                 "every error has a phrase of its own",
                                 _counter) && result;
    result = d_assert_standalone(phrases,
                                 TEST,
                                 "the phrases are the C++ layer's",
                                 _counter) && result;
    result = d_assert_standalone(outside,
                                 TEST,
                                 "a value outside the enum is unknown",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_net_results
  Tests the following:
  - bytes without an error are ok, and not the end of the stream
  - nothing moved without an error is the end of the stream
  - bytes that came with an error keep their count, and are not ok
*/
bool
d_tests_sa_net_results(
    struct d_test_counter* _counter
)
{
    const char* const            TEST  = "d_net_io_result";
    const struct d_net_io_result moved =
        d_net_io_result_make(5u,
                             D_NET_ERROR_NONE);
    const struct d_net_io_result ended =
        d_net_io_result_make(0u,
                             D_NET_ERROR_NONE);
    const struct d_net_io_result torn  =
        d_net_io_result_make(3u,
                             D_NET_ERROR_CONNECTION_RESET);
    bool                         result = true;

    const bool ok  = ( (moved.count == 5u) &&
                       (d_net_io_result_ok(moved)) &&
                       (!d_net_io_result_eof(moved)) );
    const bool end = ( (d_net_io_result_ok(ended)) &&
                       (d_net_io_result_eof(ended)) );
    const bool err = ( (torn.count == 3u) &&
                       (torn.error == D_NET_ERROR_CONNECTION_RESET) &&
                       (!d_net_io_result_ok(torn)) &&
                       (!d_net_io_result_eof(torn)) );

    result = d_assert_standalone(ok,
                                 TEST,
                                 "bytes without an error are ok",
                                 _counter) && result;
    result = d_assert_standalone(end,
                                 TEST,
                                 "nothing moved is the end of the stream",
                                 _counter) && result;
    result = d_assert_standalone(err,
                                 TEST,
                                 "bytes with an error are counted, not ok",
                                 _counter) && result;

    return result;
}

/*
d_tests_net_port_format
  The formatting half of d_tests_sa_net_ports: ports measure without a
buffer, and format all or nothing.
*/
static bool
d_tests_net_port_format(
    struct d_test_counter* _counter
)
{
    const char* const TEST    = "d_net_port_format";
    char              text[8] = "#######";

    const bool format = ( (d_net_port_format(65535u,
                                             NULL,
                                             0u) == 5u) &&
                          (d_net_port_format(65535u,
                                             text,
                                             5u) == 5u) &&
                          (text[0] == '\0') &&
                          (d_net_port_format(65535u,
                                             text,
                                             6u) == 5u) &&
                          (strcmp(text,
                                  "65535") == 0) );

    return d_assert_standalone(format,
                               TEST,
                               "ports format all or nothing, and measure",
                               _counter);
}

/*
d_tests_sa_net_ports
  Tests the following:
  - decimal ports parse across the whole range, with leading zeros
  - malformed or oversized text is refused, leaving the output alone
  - NULL arguments are refused
  - ports format all or nothing (d_tests_net_port_format)
*/
bool
d_tests_sa_net_ports(
    struct d_test_counter* _counter
)
{
    static const char* const BAD[] =
    {
        "", "65536", "8a", "-1", " 80", "+80", "99999999999999999999"
    };
    const char* const        TEST      = "d_net_port";
    const struct d_pack_text null_text = { NULL, 3u };
    d_net_port               port      = 7u;
    bool                     refused   = true;
    bool                     result    = true;

    const bool parsed = ( (d_net_port_parse(d_tests_net_text("00080"),
                                            &port) == D_NET_ERROR_NONE) &&
                          (port == 80u) &&
                          (d_net_port_parse(d_tests_net_text("65535"),
                                            &port) == D_NET_ERROR_NONE) &&
                          (port == 65535u) &&
                          (d_net_port_parse(d_tests_net_text("0"),
                                            &port) == D_NET_ERROR_NONE) &&
                          (port == 0u) );

    // every malformed text is refused, and the output keeps its value
    for (size_t i = 0u; i < (sizeof(BAD) / sizeof(BAD[0])); ++i)
    {
        port    = 7u;
        refused = ( (refused) &&
                    (d_net_port_parse(d_tests_net_text(BAD[i]),
                                      &port) ==
                     D_NET_ERROR_ADDRESS_INVALID) &&
                    (port == 7u) );
    }

    const bool nulls = ( (d_net_port_parse(d_tests_net_text("80"),
                                           NULL) ==
                          D_NET_ERROR_INVALID_ARGUMENT) &&
                         (d_net_port_parse(null_text,
                                           &port) ==
                          D_NET_ERROR_INVALID_ARGUMENT) );

    result = d_assert_standalone(parsed,
                                 TEST,
                                 "ports parse from 0 to 65535",
                                 _counter) && result;
    result = d_assert_standalone(refused,
                                 TEST,
                                 "malformed ports are refused untouched",
                                 _counter) && result;
    result = d_assert_standalone(nulls,
                                 TEST,
                                 "NULL arguments are refused",
                                 _counter) && result;

    return d_tests_net_port_format(_counter) && result;
}

/*
d_tests_net_endpoint_equality
  The comparison half of d_tests_sa_net_endpoint_records: a copy equals its
original, NULL equals only NULL, and set_local refuses an empty path, then
makes the copy a different endpoint.
*/
static bool
d_tests_net_endpoint_equality(
    struct d_test_counter*       _counter,
    const struct d_net_endpoint* _endpoint
)
{
    const char* const     TEST  = "d_net_endpoint_equal";
    struct d_net_endpoint other = *_endpoint;

    const bool equal = ( (d_net_endpoint_equal(_endpoint,
                                               &other)) &&
                         (d_net_endpoint_equal(NULL,
                                               NULL)) &&
                         (!d_net_endpoint_equal(_endpoint,
                                                NULL)) &&
                         (d_net_endpoint_set_local(&other,
                                                   d_tests_net_text("")) ==
                          D_NET_ERROR_ADDRESS_INVALID) &&
                         (d_net_endpoint_set_local(&other,
                                                   d_tests_net_text("/d.s")) ==
                          D_NET_ERROR_NONE) &&
                         (d_net_endpoint_is_unix(&other)) &&
                         (other.port == 0u) &&
                         (!d_net_endpoint_equal(_endpoint,
                                                &other)) );

    return d_assert_standalone(equal,
                               TEST,
                               "set_local stores paths; equal compares",
                               _counter);
}

/*
d_tests_sa_net_endpoint_records
  Tests the following:
  - a fresh endpoint is empty, on port 0, over TCP
  - set stores its fields, and accepts an empty host
  - an overlong host, a NUL, an unknown protocol, and NULL are refused, and
    a refused set leaves the record as it was
  - set_local stores a path, and refuses an empty one
  - equal compares all three fields (d_tests_net_endpoint_equality)
*/
bool
d_tests_sa_net_endpoint_records(
    struct d_test_counter* _counter
)
{
    const char* const        TEST     = "d_net_endpoint_set";
    const struct d_pack_text with_nul = { "a\0b", 3u };
    struct d_net_endpoint    endpoint = { .port = 0u };
    bool                     result   = true;

    // one byte more than an endpoint holds
    char too_long[D_NET_HOST_MAX + 2u] = { 0 };

    d_tests_net_fill_host(too_long,
                          D_NET_HOST_MAX + 1u);
    d_net_endpoint_init(&endpoint);

    const bool stored = ( (endpoint.host[0] == '\0') &&
                          (endpoint.port == 0u) &&
                          (endpoint.protocol == D_NET_PROTOCOL_TCP) &&
                          (d_net_endpoint_set(&endpoint,
                                              d_tests_net_text(""),
                                              0u,
                                              D_NET_PROTOCOL_UDP) ==
                           D_NET_ERROR_NONE) &&
                          (d_net_endpoint_set(&endpoint,
                                              d_tests_net_text("a.example"),
                                              443u,
                                              D_NET_PROTOCOL_TCP) ==
                           D_NET_ERROR_NONE) &&
                          (strcmp(endpoint.host,
                                  "a.example") == 0) &&
                          (endpoint.port == 443u) );
    const bool refused = ( (d_net_endpoint_set(&endpoint,
                                               d_tests_net_text(too_long),
                                               1u,
                                               D_NET_PROTOCOL_TCP) ==
                            D_NET_ERROR_ADDRESS_INVALID) &&
                           (d_net_endpoint_set(&endpoint,
                                               with_nul,
                                               1u,
                                               D_NET_PROTOCOL_TCP) ==
                            D_NET_ERROR_ADDRESS_INVALID) &&
                           (d_net_endpoint_set(&endpoint,
                                               d_tests_net_text("h"),
                                               1u,
                                               (enum d_net_protocol)7) ==
                            D_NET_ERROR_INVALID_ARGUMENT) &&
                           (strcmp(endpoint.host,
                                   "a.example") == 0) &&
                           (endpoint.port == 443u) );

    result = d_assert_standalone(stored,
                                 TEST,
                                 "records start empty; set stores a host",
                                 _counter) && result;
    result = d_assert_standalone(refused,
                                 TEST,
                                 "a refused set leaves the record alone",
                                 _counter) && result;

    return d_tests_net_endpoint_equality(_counter,
                                         &endpoint) && result;
}

/*
d_tests_net_parse_records
  The record half of d_tests_sa_net_endpoint_parse: parse fills a record,
refuses a unix-domain protocol, and refuses a host the record cannot hold,
leaving the record as it was.
*/
static bool
d_tests_net_parse_records(
    struct d_test_counter* _counter
)
{
    const char* const     TEST     = "d_net_endpoint_parse";
    struct d_net_endpoint endpoint = { .port = 0u };
    bool                  result   = true;

    // an overlong host, a colon, a port digit
    char too_long[D_NET_HOST_MAX + 4u] = { 0 };

    d_tests_net_fill_host(too_long,
                          D_NET_HOST_MAX + 1u);
    too_long[D_NET_HOST_MAX + 1u] = ':';
    too_long[D_NET_HOST_MAX + 2u] = '1';
    too_long[D_NET_HOST_MAX + 3u] = '\0';
    d_net_endpoint_init(&endpoint);

    const bool filled = ( (d_net_endpoint_parse(d_tests_net_text("[::1]:53"),
                                                D_NET_PROTOCOL_UDP,
                                                &endpoint) ==
                           D_NET_ERROR_NONE) &&
                          (strcmp(endpoint.host,
                                  "::1") == 0) &&
                          (endpoint.port == 53u) &&
                          (endpoint.protocol == D_NET_PROTOCOL_UDP) );
    const bool refused = ( (d_net_endpoint_parse(d_tests_net_text("h:1"),
                                                 D_NET_PROTOCOL_UNIX,
                                                 &endpoint) ==
                            D_NET_ERROR_INVALID_ARGUMENT) &&
                           (d_net_endpoint_parse(d_tests_net_text(too_long),
                                                 D_NET_PROTOCOL_TCP,
                                                 &endpoint) ==
                            D_NET_ERROR_ADDRESS_INVALID) &&
                           (strcmp(endpoint.host,
                                   "::1") == 0) );

    result = d_assert_standalone(filled,
                                 TEST,
                                 "parse fills a record",
                                 _counter) && result;
    result = d_assert_standalone(refused,
                                 TEST,
                                 "parse refuses what a record cannot be",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_net_endpoint_parse
  Tests the following:
  - names, bracketed IPv6 literals, and zones split into borrowed parts
  - every malformed form is refused, leaving the outputs alone
  - NULL outputs are refused
  - parse fills records, within their limits (d_tests_net_parse_records)
*/
bool
d_tests_sa_net_endpoint_parse(
    struct d_test_counter* _counter
)
{
    static const char* const BAD[] =
    {
        "", "example.com", "example.com:", ":80", "[::1]", "[::1]80",
        "[::1]:", "[]:80", "::1:80", "[::1", "h:0", "h:65536", "h:8x"
    };
    const char* const        TEST     = "d_net_endpoint_parse_parts";
    const struct d_pack_text with_nul = { "a\0b:80", 6u };
    const struct d_pack_text zoned    = d_tests_net_text("[fe80::1%e0]:443");
    struct d_pack_text       host     = { NULL, 0u };
    d_net_port               port     = 0u;
    bool                     refused  = true;
    bool                     result   = true;

    const bool split = ( (d_net_endpoint_parse_parts(
                              d_tests_net_text("example.com:80"),
                              &host,
                              &port) == D_NET_ERROR_NONE) &&
                         (d_tests_net_text_is(host,
                                              "example.com")) &&
                         (port == 80u) &&
                         (d_net_endpoint_parse_parts(zoned,
                                                     &host,
                                                     &port) ==
                          D_NET_ERROR_NONE) &&
                         (d_tests_net_text_is(host,
                                              "fe80::1%e0")) &&
                         (host.data == zoned.data + 1) &&
                         (port == 443u) );

    // every malformed form is refused, and the outputs keep their values
    for (size_t i = 0u; i < (sizeof(BAD) / sizeof(BAD[0])); ++i)
    {
        refused = ( (refused) &&
                    (d_net_endpoint_parse_parts(d_tests_net_text(BAD[i]),
                                                &host,
                                                &port) ==
                     D_NET_ERROR_ADDRESS_INVALID) &&
                    (port == 443u) );
    }

    const bool guarded = ( (d_net_endpoint_parse_parts(with_nul,
                                                       &host,
                                                       &port) ==
                            D_NET_ERROR_ADDRESS_INVALID) &&
                           (d_net_endpoint_parse_parts(d_tests_net_text("h:1"),
                                                       NULL,
                                                       &port) ==
                            D_NET_ERROR_INVALID_ARGUMENT) );

    result = d_assert_standalone(split,
                                 TEST,
                                 "names and bracketed literals split",
                                 _counter) && result;
    result = d_assert_standalone( ( (refused) &&
                                    (guarded) ),
                                  TEST,
                                  "malformed text is refused untouched",
                                  _counter) && result;

    return d_tests_net_parse_records(_counter) && result;
}

/*
d_tests_net_format_edges
  The edge half of d_tests_sa_net_endpoint_format: paths format bare, NULL
formats as nothing, and the longest endpoint's text is exactly
D_NET_ENDPOINT_TEXT_MAX long.
*/
static bool
d_tests_net_format_edges(
    struct d_test_counter* _counter
)
{
    const char* const     TEST     = "d_net_endpoint_format";
    struct d_net_endpoint endpoint = { .port = 0u };
    bool                  result   = true;

    // room for the longest text an endpoint formats to
    char text[D_NET_ENDPOINT_TEXT_MAX + 1u] = { 0 };

    // the longest host an endpoint holds, made an IPv6-like literal
    char host[D_NET_HOST_MAX + 1u] = { 0 };

    d_tests_net_fill_host(host,
                          D_NET_HOST_MAX);
    host[1] = ':';
    (void)d_net_endpoint_set_local(&endpoint,
                                   d_tests_net_text("/run/d.sock"));

    const bool local = ( (d_net_endpoint_format(&endpoint,
                                                text,
                                                sizeof(text)) == 11u) &&
                         (strcmp(text,
                                 "/run/d.sock") == 0) &&
                         (d_net_endpoint_format(NULL,
                                                text,
                                                sizeof(text)) == 0u) &&
                         (text[0] == '\0') );
    const bool longest = (d_net_endpoint_format_parts(d_tests_net_text(host),
                                                      65535u,
                                                      D_NET_PROTOCOL_TCP,
                                                      text,
                                                      sizeof(text)) ==
                          D_NET_ENDPOINT_TEXT_MAX);

    result = d_assert_standalone(local,
                                 TEST,
                                 "paths format bare; NULL formats nothing",
                                 _counter) && result;
    result = d_assert_standalone(longest,
                                 TEST,
                                 "the text limit holds the longest endpoint",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_net_endpoint_format
  Tests the following:
  - names format as host:port, colon hosts in brackets, paths bare
  - formatting measures without a buffer, and is all or nothing
  - paths, NULL, and the longest endpoint (d_tests_net_format_edges)
  - what is formatted parses back to the same parts
*/
bool
d_tests_sa_net_endpoint_format(
    struct d_test_counter* _counter
)
{
    const char* const     TEST     = "d_net_endpoint_format";
    struct d_net_endpoint endpoint = { .port = 0u };
    struct d_pack_text    back     = { NULL, 0u };
    d_net_port            port     = 0u;
    bool                  result   = true;

    // room for the longest text an endpoint formats to
    char text[D_NET_ENDPOINT_TEXT_MAX + 1u] = { 0 };

    (void)d_net_endpoint_parse(d_tests_net_text("[::1]:8080"),
                               D_NET_PROTOCOL_TCP,
                               &endpoint);

    const bool plain = ( (d_net_endpoint_format(&endpoint,
                                                NULL,
                                                0u) == 10u) &&
                         (d_net_endpoint_format(&endpoint,
                                                text,
                                                10u) == 10u) &&
                         (text[0] == '\0') &&
                         (d_net_endpoint_format(&endpoint,
                                                text,
                                                11u) == 10u) &&
                         (strcmp(text,
                                 "[::1]:8080") == 0) &&
                         (d_net_endpoint_parse_parts(d_tests_net_text(text),
                                                     &back,
                                                     &port) ==
                          D_NET_ERROR_NONE) &&
                         (d_tests_net_text_is(back,
                                              "::1")) &&
                         (port == 8080u) );

    result = d_assert_standalone(plain,
                                 TEST,
                                 "brackets, measures, and parses back",
                                 _counter) && result;

    return d_tests_net_format_edges(_counter) && result;
}

/*
d_tests_sa_net_frame_codec
  Tests the following:
  - headers encode big-endian, and decode to what was encoded
  - the check allows a payload up to its ceiling and no further
  - the check refuses what a 32-bit header cannot carry, whatever the
    ceiling, where size_t can hold such a size
  - NULL headers are ignored, and decode as 0
*/
bool
d_tests_sa_net_frame_codec(
    struct d_test_counter* _counter
)
{
    const char* const   TEST        = "d_net_frame_header";
    const unsigned char expected[4] = { 0x01u, 0x02u, 0x03u, 0x04u };
    const size_t        beyond      = (size_t)D_NET_FRAME_LENGTH_MAX;
    unsigned char       header[4]   = { 0u, 0u, 0u, 0u };
    bool                wide        = true;
    bool                result      = true;

    d_net_frame_header_encode(0x01020304u,
                              header);

    const bool order = ( (memcmp(header,
                                 expected,
                                 sizeof(header)) == 0) &&
                         (d_net_frame_header_decode(header) == 0x01020304u) );

    d_net_frame_header_encode(0xFFFFFFFFu,
                              header);
    d_net_frame_header_encode(0u,
                              NULL);

    const bool edges = ( (d_net_frame_header_decode(header) == 0xFFFFFFFFu) &&
                         (d_net_frame_header_decode(NULL) == 0u) );
    const bool check = ( (d_net_frame_check(10u,
                                            10u) == D_NET_ERROR_NONE) &&
                         (d_net_frame_check(0u,
                                            0u) == D_NET_ERROR_NONE) &&
                         (d_net_frame_check(11u,
                                            10u) ==
                          D_NET_ERROR_MESSAGE_TOO_LARGE) );

    // only where size_t outranges the header can a size exceed it
    if (beyond < SIZE_MAX)
    {
        wide = (d_net_frame_check(beyond + 1u,
                                  SIZE_MAX) == D_NET_ERROR_MESSAGE_TOO_LARGE);
    }

    result = d_assert_standalone( ( (order) &&
                                    (edges) ),
                                  TEST,
                                  "headers are big-endian and round-trip",
                                  _counter) && result;
    result = d_assert_standalone(check,
                                 TEST,
                                 "the check holds payloads to the ceiling",
                                 _counter) && result;
    result = d_assert_standalone(wide,
                                 TEST,
                                 "the check holds payloads to 32 bits",
                                 _counter) && result;

    return result;
}
