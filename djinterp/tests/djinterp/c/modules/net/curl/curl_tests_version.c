/*******************************************************************************
* djinterp [c][net]                                         curl_tests_version.c
*
* Tests of the HTTP version a transfer asks for, and the one its response
* came in.
*   Each transfer is a GET of the loopback peer's /hello, which answers once
* and says how it was asked; a test checks what libcurl reported against
* what the peer saw. The tests of HTTP/2 run only where libcurl has it, and
* d_tests_curl_without_http2 checks the other side of that line.
*   HTTP/3 runs over QUIC, which the peer doesn't speak, so its transfer
* goes to a server the environment names: DJINTERP_TEST_CURL_HTTP3_URL, an
* https URL answering 200 over HTTP/3, and DJINTERP_TEST_CURL_HTTP3_VERIFY,
* "0" to accept the certificate a test server made for itself. Without
* both a libcurl that has HTTP/3 and such a server, that test skips;
* d_tests_curl_without_http3 checks a libcurl that lacks it.
*
*
* path:      /tests/djinterp/c/modules/net/curl/curl_tests_version.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./curl_tests.h"  // the suite
// std
#include <stdlib.h>  // getenv, _dupenv_s, free
#include <string.h>  // memcmp, memcpy, strcmp, strlen


// D_TESTS_CURL_TIMEOUT_MS
//   constant: each transfer's limit, so a peer that never answers can't
// hold the suite.
#define D_TESTS_CURL_TIMEOUT_MS 10000L

// dj_tests_curl_take
//   struct: what a transfer delivered: the body, and the x-peer field.
struct dj_tests_curl_take
{
    char   body[32];
    size_t body_size;
    char   peer[8];
};


/*
d_tests_curl_setting
  An environment variable's value in _out, or false when it is unset, empty
or longer than _capacity allows: through _dupenv_s under MSVC, which
deprecates getenv, and getenv elsewhere.
*/
static
bool d_tests_curl_setting(
    const char* _name,
    char*       _out,
    size_t      _capacity
)
{
#if defined(_MSC_VER)
    char*  value  = NULL;
    size_t size   = 0u;
    bool   stored = false;

    // a copy the runtime allocates, freed after
    if ( (_dupenv_s(&value,
                    &size,
                    _name) == 0) &&
         (value != NULL)         &&
         (value[0] != '\0')      &&
         (strlen(value) < _capacity) )
    {
        memcpy(_out,
               value,
               strlen(value) + 1u);
        stored = true;
    }

    free(value);

    return stored;
#else
    const char* const value = getenv(_name);

    // set, not empty, and fitting
    if ( (value == NULL)          ||
         (value[0] == '\0')       ||
         (strlen(value) >= _capacity) )
    {
        return false;
    }

    memcpy(_out,
           value,
           strlen(value) + 1u);

    return true;
#endif  // _MSC_VER
}

/*
d_tests_curl_on_body
  Keeps the body's first bytes; a body longer than the record is refused,
which the tests would see as a SINK failure.
*/
static
bool d_tests_curl_on_body(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_tests_curl_take* const take =
        (struct dj_tests_curl_take*)_context;

    // more than any answer of the peer's
    if (_size > (sizeof(take->body) - 1u - take->body_size))
    {
        return false;
    }

    memcpy(take->body + take->body_size,
           _data,
           _size);
    take->body_size            += _size;
    take->body[take->body_size] = '\0';

    return true;
}

/*
d_tests_curl_on_header
  Keeps x-peer's value; every other field passes.
*/
static
bool d_tests_curl_on_header(
    void*               _context,
    struct dj_pack_text _name,
    struct dj_pack_text _value
)
{
    struct dj_tests_curl_take* const take =
        (struct dj_tests_curl_take*)_context;

    // the peer's field, its value if it fits
    if ( (_name.length == 6u) &&
         (memcmp(_name.data,
                 "x-peer",
                 6u) == 0) &&
         (_value.length < sizeof(take->peer)) )
    {
        memcpy(take->peer,
               _value.data,
               _value.length);
        take->peer[_value.length] = '\0';
    }

    return true;
}

/*
d_tests_curl_get
  GETs _url asking for _version, within D_TESTS_CURL_TIMEOUT_MS and with no
proxy, whatever the environment says; fills _out and _take.
*/
static
enum dj_curl_error d_tests_curl_get(
    const char*                _url,
    enum dj_curl_http_version  _version,
    struct dj_curl_result*     _out,
    struct dj_tests_curl_take* _take
)
{
    struct dj_curl_request request;
    struct dj_curl_options options;
    struct dj_curl_sink    sink;

    memset(&request,
           0,
           sizeof(request));
    memset(_take,
           0,
           sizeof(*_take));
    request.url = _url;
    d_curl_options_init(&options);
    options.timeout_ms   = D_TESTS_CURL_TIMEOUT_MS;
    options.proxy        = "";
    options.http_version = _version;
    sink.body            = d_tests_curl_on_body;
    sink.header          = d_tests_curl_on_header;
    sink.context         = _take;

    return d_curl_perform(&request,
                          &options,
                          &sink,
                          _out);
}

/*
d_tests_curl_has_http2
  Whether this run can speak HTTP/2 through libcurl; the version tests skip
where it can't, with the reason.
*/
static
bool d_tests_curl_has_http2(
    struct dj_test_suite* _suite
)
{
    // no libcurl, or one without HTTP/2
    if (!d_curl_supports(D_CURL_FEATURE_HTTP2))
    {
        (void)d_test_suite_skip(_suite,
                                (d_curl_version() == NULL)
                                    ? "the build has no libcurl"
                                    : "this libcurl has no HTTP/2");

        return false;
    }

    return true;
}

/*
d_tests_curl_exchange
  The common shape of a version test: a peer that speaks HTTP/2 or not, one
GET asking for _version, and the peer joined. Returns false, with a failed
check, if the peer couldn't start.
*/
static
bool d_tests_curl_exchange(
    struct dj_test_suite*      _suite,
    const char*                _test,
    bool                       _speaks_h2,
    enum dj_curl_http_version  _version,
    struct dj_tests_curl_peer* _peer,
    struct dj_curl_result*     _out,
    struct dj_tests_curl_take* _take
)
{
    const bool started = d_tests_curl_peer_start(_peer,
                                                 _speaks_h2);

    // a peer, or nothing to test against
    if (!D_TESTS_NET_ASSERT(started,
                            _test,
                            "the loopback peer starts",
                            _suite))
    {
        return false;
    }

    (void)d_tests_curl_get(_peer->url,
                           _version,
                           _out,
                           _take);
    d_tests_curl_peer_finish(_peer);

    return true;
}


/*
d_tests_curl_options_default
  Tests the following:
  - d_curl_options_init leaves the HTTP version to libcurl
*/
bool d_tests_curl_options_default(
    struct dj_test_suite* _suite
)
{
    struct dj_curl_options options;

    memset(&options,
           0xA5,
           sizeof(options));
    d_curl_options_init(&options);

    return D_TESTS_NET_ASSERT(options.http_version == D_CURL_HTTP_DEFAULT,
                              "d_curl_options_init",
                              "the HTTP version defaults to libcurl's choice",
                              _suite);
}

/*
d_tests_curl_version_refused
  Tests the following:
  - an http_version the enum doesn't list is INVALID_ARGUMENT, with or
    without libcurl, and nothing is sent: the URL's port has no listener
  - the refusal's result reports no status and no version
*/
bool d_tests_curl_version_refused(
    struct dj_test_suite* _suite
)
{
    const char* const     TEST    = "d_curl_perform";
    struct dj_curl_request request;
    struct dj_curl_options options;
    struct dj_curl_result  result;
    bool                  passed  = true;

    memset(&request,
           0,
           sizeof(request));
    memset(&result,
           0xA5,
           sizeof(result));
    request.url = "http://127.0.0.1:9/hello";
    d_curl_options_init(&options);
    options.http_version = (enum dj_curl_http_version)99;

    passed = D_TESTS_NET_ASSERT(d_curl_perform(&request,
                                               &options,
                                               NULL,
                                               &result) ==
                                D_CURL_ERROR_INVALID_ARGUMENT,
                                TEST,
                                "an unlisted HTTP version is refused",
                                _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status ==
                                  D_CURL_ERROR_INVALID_ARGUMENT) &&
                                 (result.code == 0)              &&
                                 (result.http_status == 0)       &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_UNKNOWN),
                                 TEST,
                                 "the refusal reports no status or version",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_without_http2
  Tests the following:
  - without libcurl, a transfer asking for HTTP/2 is UNSUPPORTED
  - with a libcurl lacking HTTP/2, it is UNSUPPORTED_PROTOCOL, before a byte
    is sent
  - with one that has it, there is nothing to check here; the version
    tests cover it, and this test skips
*/
bool d_tests_curl_without_http2(
    struct dj_test_suite* _suite
)
{
    const char* const        TEST = "d_curl_perform";
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;

    // a libcurl that speaks HTTP/2: the other tests' case
    if (d_curl_supports(D_CURL_FEATURE_HTTP2))
    {
        return d_test_suite_skip(_suite,
                                 "this libcurl has HTTP/2");
    }

    // no libcurl at all
    if (d_curl_version() == NULL)
    {
        return D_TESTS_NET_ASSERT(d_tests_curl_get(
                                      "http://127.0.0.1:9/hello",
                                      D_CURL_HTTP_2,
                                      &result,
                                      &take) == D_CURL_ERROR_UNSUPPORTED,
                                  TEST,
                                  "without libcurl, HTTP/2 is unsupported",
                                  _suite);
    }

    return D_TESTS_NET_ASSERT(d_tests_curl_get(
                                  "http://127.0.0.1:9/hello",
                                  D_CURL_HTTP_2,
                                  &result,
                                  &take) ==
                              D_CURL_ERROR_UNSUPPORTED_PROTOCOL,
                              TEST,
                              "a libcurl without HTTP/2 refuses to ask",
                              _suite);
}

/*
d_tests_curl_prior_knowledge
  Tests the following:
  - HTTP_2_PRIOR_KNOWLEDGE in the clear opens with HTTP/2's preface, no
    HTTP/1 request first
  - the answer arrives whole over HTTP/2: status 200, the peer's field and
    body
  - the result says HTTP/2
*/
bool d_tests_curl_prior_knowledge(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_2_PRIOR_KNOWLEDGE";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // where libcurl can't speak HTTP/2
    if (!d_tests_curl_has_http2(_suite))
    {
        return true;
    }

    if (!d_tests_curl_exchange(_suite, TEST, true,
                               D_CURL_HTTP_2_PRIOR_KNOWLEDGE,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK) &&
                                 (result.http_status == 200),
                                 TEST,
                                 "the transfer completes with a 200",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (peer.saw_preface)  &&
                                 (!peer.saw_upgrade) &&
                                 (peer.answered_h2)  &&
                                 (peer.outcome == D_NET_OK),
                                 TEST,
                                 "the peer saw HTTP/2 from the first byte",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT(result.http_version == D_HTTP_VERSION_2,
                                TEST,
                                "the result says HTTP/2",
                                _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (strcmp(take.body,
                                         "hello h2") == 0) &&
                                 (strcmp(take.peer,
                                         "h2") == 0),
                                 TEST,
                                 "the body and the field arrive",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_upgrade
  Tests the following:
  - HTTP_2 in the clear sends an HTTP/1.1 request asking to upgrade to h2c
  - a server that takes the upgrade answers it over HTTP/2, and the result
    says HTTP/2
*/
bool d_tests_curl_upgrade(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_2, upgraded";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // where libcurl can't speak HTTP/2
    if (!d_tests_curl_has_http2(_suite))
    {
        return true;
    }

    if (!d_tests_curl_exchange(_suite, TEST, true, D_CURL_HTTP_2,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK) &&
                                 (result.http_status == 200),
                                 TEST,
                                 "the transfer completes with a 200",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (!peer.saw_preface) &&
                                 (peer.saw_upgrade)  &&
                                 (strcmp(peer.version,
                                         "HTTP/1.1") == 0) &&
                                 (peer.answered_h2),
                                 TEST,
                                 "an HTTP/1.1 request asked to upgrade",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.http_version == D_HTTP_VERSION_2) &&
                                 (strcmp(take.body,
                                         "hello h2") == 0),
                                 TEST,
                                 "the answer came over HTTP/2",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_upgrade_declined
  Tests the following:
  - HTTP_2 in the clear, to a server that speaks only HTTP/1.1, falls back:
    the transfer completes over HTTP/1.1, and the result says so
*/
bool d_tests_curl_upgrade_declined(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_2, declined";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // where libcurl can't speak HTTP/2
    if (!d_tests_curl_has_http2(_suite))
    {
        return true;
    }

    if (!d_tests_curl_exchange(_suite, TEST, false, D_CURL_HTTP_2,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (peer.saw_upgrade)  &&
                                 (!peer.answered_h2),
                                 TEST,
                                 "the upgrade was asked, and declined",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK)            &&
                                 (result.http_status == 200)             &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_1_1)                    &&
                                 (strcmp(take.body,
                                         "hello h1") == 0),
                                 TEST,
                                 "the transfer completes over HTTP/1.1",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_http1_only
  Tests the following:
  - HTTP_1_1, to a server that speaks HTTP/2, never asks for it: no
    preface, no upgrade, and the result says HTTP/1.1
*/
bool d_tests_curl_http1_only(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_1_1";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // without libcurl there is no transfer to look at
    if (d_curl_version() == NULL)
    {
        return d_test_suite_skip(_suite,
                                 "the build has no libcurl");
    }

    if (!d_tests_curl_exchange(_suite, TEST, true, D_CURL_HTTP_1_1,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (!peer.saw_preface)                &&
                                 (!peer.saw_upgrade)                &&
                                 (strcmp(peer.version,
                                         "HTTP/1.1") == 0),
                                 TEST,
                                 "a plain HTTP/1.1 request",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK)            &&
                                 (result.http_status == 200)             &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_1_1)                    &&
                                 (strcmp(take.peer,
                                         "h1") == 0),
                                 TEST,
                                 "the result says HTTP/1.1",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_http1_0
  Tests the following:
  - HTTP_1_0 sends an HTTP/1.0 request, and the result says HTTP/1.0 for
    the HTTP/1.0 answer
*/
bool d_tests_curl_http1_0(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_1_0";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // without libcurl there is no transfer to look at
    if (d_curl_version() == NULL)
    {
        return d_test_suite_skip(_suite,
                                 "the build has no libcurl");
    }

    if (!d_tests_curl_exchange(_suite, TEST, true, D_CURL_HTTP_1_0,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT(strcmp(peer.version,
                                       "HTTP/1.0") == 0,
                                TEST,
                                "an HTTP/1.0 request",
                                _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK)            &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_1_0)                    &&
                                 (strcmp(take.body,
                                         "hello h1") == 0),
                                 TEST,
                                 "the result says HTTP/1.0",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_tls_only_in_clear
  Tests the following:
  - HTTP_2_TLS over a URL in the clear asks for nothing: HTTP/1.1, no
    upgrade, though the server would take one
*/
bool d_tests_curl_tls_only_in_clear(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_2_TLS, in the clear";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // where libcurl can't speak HTTP/2
    if (!d_tests_curl_has_http2(_suite))
    {
        return true;
    }

    if (!d_tests_curl_exchange(_suite, TEST, true, D_CURL_HTTP_2_TLS,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (!peer.saw_preface) &&
                                 (!peer.saw_upgrade),
                                 TEST,
                                 "no HTTP/2 asked for in the clear",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK) &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_1_1),
                                 TEST,
                                 "the result says HTTP/1.1",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_default_in_clear
  Tests the following:
  - DEFAULT over a URL in the clear is HTTP/1.1 with no upgrade, libcurl's
    own choice from 7.62
*/
bool d_tests_curl_default_in_clear(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "DEFAULT, in the clear";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // without libcurl there is no transfer to look at
    if (d_curl_version() == NULL)
    {
        return d_test_suite_skip(_suite,
                                 "the build has no libcurl");
    }

    if (!d_tests_curl_exchange(_suite, TEST, true, D_CURL_HTTP_DEFAULT,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (!peer.saw_preface) &&
                                 (!peer.saw_upgrade),
                                 TEST,
                                 "no HTTP/2 asked for in the clear",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status == D_CURL_OK) &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_1_1),
                                 TEST,
                                 "the result says HTTP/1.1",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_prior_knowledge_refused
  Tests the following:
  - HTTP_2_PRIOR_KNOWLEDGE to a server that speaks only HTTP/1.1 fails: the
    server answers HTTP/2's preface as a bad HTTP/1 request, and libcurl,
    expecting frames, gives up
  - the failure reports no HTTP version
*/
bool d_tests_curl_prior_knowledge_refused(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_2_PRIOR_KNOWLEDGE, refused";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // where libcurl can't speak HTTP/2
    if (!d_tests_curl_has_http2(_suite))
    {
        return true;
    }

    if (!d_tests_curl_exchange(_suite, TEST, false,
                               D_CURL_HTTP_2_PRIOR_KNOWLEDGE,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (peer.saw_preface) &&
                                 (!peer.answered_h2),
                                 TEST,
                                 "the preface reached an HTTP/1 server",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (result.status != D_CURL_OK) &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_UNKNOWN),
                                 TEST,
                                 "the transfer fails, with no version",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_without_http3
  Tests the following:
  - without libcurl, a transfer asking for HTTP/3 is UNSUPPORTED
  - with a libcurl lacking HTTP/3, HTTP_3 and HTTP_3_ONLY are each
    UNSUPPORTED_PROTOCOL, before a byte is sent
  - with one that has it, this test skips; d_tests_curl_http3_server
    covers it
*/
bool d_tests_curl_without_http3(
    struct dj_test_suite* _suite
)
{
    const char* const        TEST = "d_curl_perform";
    const char* const        URL  = "https://127.0.0.1:9/hello";
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                     passed = true;

    // a libcurl that speaks HTTP/3: the other test's case
    if (d_curl_supports(D_CURL_FEATURE_HTTP3))
    {
        return d_test_suite_skip(_suite,
                                 "this libcurl has HTTP/3");
    }

    // no libcurl at all
    if (d_curl_version() == NULL)
    {
        return D_TESTS_NET_ASSERT(d_tests_curl_get(URL,
                                                   D_CURL_HTTP_3,
                                                   &result,
                                                   &take) ==
                                  D_CURL_ERROR_UNSUPPORTED,
                                  TEST,
                                  "without libcurl, HTTP/3 is unsupported",
                                  _suite);
    }

    passed = D_TESTS_NET_ASSERT(d_tests_curl_get(URL,
                                                 D_CURL_HTTP_3,
                                                 &result,
                                                 &take) ==
                                D_CURL_ERROR_UNSUPPORTED_PROTOCOL,
                                TEST,
                                "a libcurl without HTTP/3 refuses HTTP_3",
                                _suite) && passed;
    passed = D_TESTS_NET_ASSERT(d_tests_curl_get(URL,
                                                 D_CURL_HTTP_3_ONLY,
                                                 &result,
                                                 &take) ==
                                D_CURL_ERROR_UNSUPPORTED_PROTOCOL,
                                TEST,
                                "and HTTP_3_ONLY",
                                _suite) && passed;

    return passed;
}

/*
d_tests_curl_http3_server
  Tests the following, against the server DJINTERP_TEST_CURL_HTTP3_URL
names:
  - HTTP_3_ONLY gets its 200 over HTTP/3, and the result says HTTP/3
  - HTTP_3, which may fall back, gets it over HTTP/3 too from a server that
    answers QUIC
*/
bool d_tests_curl_http3_server(
    struct dj_test_suite* _suite
)
{
    const char* const        TEST = "HTTP_3, to a named server";
    char                     url[512];
    char                     verify[8];
    struct dj_curl_request    request;
    struct dj_curl_options    options;
    struct dj_curl_result     only;
    struct dj_curl_result     either;
    bool                     passed = true;

    // a libcurl with HTTP/3, and a server to ask
    if (!d_curl_supports(D_CURL_FEATURE_HTTP3))
    {
        return d_test_suite_skip(_suite,
                                 (d_curl_version() == NULL)
                                     ? "the build has no libcurl"
                                     : "this libcurl has no HTTP/3");
    }

    if (!d_tests_curl_setting("DJINTERP_TEST_CURL_HTTP3_URL",
                              url,
                              sizeof(url)))
    {
        return d_test_suite_skip(_suite,
                                 "DJINTERP_TEST_CURL_HTTP3_URL names no "
                                 "HTTP/3 server");
    }

    memset(&request,
           0,
           sizeof(request));
    request.url = url;
    d_curl_options_init(&options);
    options.timeout_ms   = D_TESTS_CURL_TIMEOUT_MS;
    options.proxy        = "";
    options.verify_tls   = !( (d_tests_curl_setting(
                                   "DJINTERP_TEST_CURL_HTTP3_VERIFY",
                                   verify,
                                   sizeof(verify))) &&
                              (strcmp(verify,
                                      "0") == 0) );
    options.http_version = D_CURL_HTTP_3_ONLY;
    (void)d_curl_perform(&request,
                         &options,
                         NULL,
                         &only);
    options.http_version = D_CURL_HTTP_3;
    (void)d_curl_perform(&request,
                         &options,
                         NULL,
                         &either);

    passed = D_TESTS_NET_ASSERT( (only.status == D_CURL_OK)  &&
                                 (only.http_status == 200)   &&
                                 (only.http_version ==
                                  D_HTTP_VERSION_3),
                                 TEST,
                                 "HTTP_3_ONLY gets a 200 over HTTP/3",
                                 _suite) && passed;
    passed = D_TESTS_NET_ASSERT( (either.status == D_CURL_OK) &&
                                 (either.http_version ==
                                  D_HTTP_VERSION_3),
                                 TEST,
                                 "HTTP_3 gets it over HTTP/3 too",
                                 _suite) && passed;

    return passed;
}

/*
d_tests_curl_http3_in_clear
  Tests the following, where libcurl has HTTP/3:
  - HTTP_3 over a URL in the clear is HTTP/1.1: QUIC is TLS's own, so there
    is nothing to race, and nothing HTTP/2's is asked for either
  - HTTP_3_ONLY over a URL in the clear fails before any connection: the
    peer never sees a request
*/
bool d_tests_curl_http3_in_clear(
    struct dj_test_suite* _suite
)
{
    const char* const         TEST = "HTTP_3, in the clear";
    struct dj_tests_curl_peer peer;
    struct dj_curl_result     result;
    struct dj_tests_curl_take take;
    bool                      passed = true;

    // where libcurl can't speak HTTP/3
    if (!d_curl_supports(D_CURL_FEATURE_HTTP3))
    {
        return d_test_suite_skip(_suite,
                                 (d_curl_version() == NULL)
                                     ? "the build has no libcurl"
                                     : "this libcurl has no HTTP/3");
    }

    if (!d_tests_curl_exchange(_suite, TEST, true, D_CURL_HTTP_3,
                               &peer, &result, &take))
    {
        return false;
    }

    passed = D_TESTS_NET_ASSERT( (!peer.saw_preface)               &&
                                 (!peer.saw_upgrade)               &&
                                 (result.status == D_CURL_OK)      &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_1_1),
                                 TEST,
                                 "HTTP_3 in the clear is HTTP/1.1",
                                 _suite) && passed;

    // HTTP_3_ONLY: no connection is made, so the peer's wait is cut short
    const bool started = d_tests_curl_peer_start(&peer,
                                                 true);

    if (started)
    {
        (void)d_tests_curl_get(peer.url,
                               D_CURL_HTTP_3_ONLY,
                               &result,
                               &take);
        d_tcp_listener_close(&peer.listener);
        d_tests_curl_peer_finish(&peer);
    }

    passed = D_TESTS_NET_ASSERT( (started)                            &&
                                 (result.status != D_CURL_OK)         &&
                                 (result.http_version ==
                                  D_HTTP_VERSION_UNKNOWN)             &&
                                 (peer.version[0] == '\0')            &&
                                 (!peer.saw_preface),
                                 TEST,
                                 "HTTP_3_ONLY in the clear fails, unsent",
                                 _suite) && passed;

    return passed;
}
