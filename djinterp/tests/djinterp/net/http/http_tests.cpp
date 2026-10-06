/*******************************************************************************
* djinterp [net]                                                  http_tests.cpp
*
* Tests of net/http/http.hpp, HTTP's C++ layer.
*   The C suite checks the semantics against RFC 9110; these check what the
* C++ layer adds: enumerators equal to their C counterparts and web.hpp's
* old names as aliases, results written only on success, negative status
* ints refused, NULs inside std::string refused, and the constants equal to
* http.h's literals.
*
*
* path:      /tests/djinterp/net/http/http_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./http_tests.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
#include <string>   // std::string


NS_DJINTERP
NS_TESTING

namespace net = ::djinterp::net;

NS_INTERNAL

    // tally
    //   struct: checks run and failed across the suite.
    struct tally
    {
        std::size_t run;
        std::size_t failed;
    };

    // counts
    //   function: the suite's one tally.
    static tally&
    counts()
    {
        static tally the_tally = { 0u, 0u };

        return the_tally;
    }

    // check
    //   function: records one check, printing it only when it fails.
    static bool
    check(
        bool        _held,
        const char* _what
    )
    {
        counts().run += 1u;

        // a failure is named
        if (!_held)
        {
            counts().failed += 1u;
            std::printf("    FAIL %s\n",
                        _what);
        }

        return _held;
    }

    // same
    //   function: whether a C string, possibly null, holds a text.
    static bool
    same(
        const char* _text,
        const char* _expected
    )
    {
        return ( (_text != nullptr) &&
                 (std::strcmp(_text,
                              _expected) == 0) );
    }

NS_END  // internal


/*
tests_http_vocabulary
  Tests the following:
  - each enumeration's enumerators equal their C counterparts, both ways
  - web.hpp's old names are aliases of the new ones
*/
bool
tests_http_vocabulary()
{
    bool result = true;

    result = internal::check(
                 ( (net::to_c(net::http_method::other) ==
                    D_HTTP_METHOD_OTHER)                                 &&
                   (net::from_c(D_HTTP_METHOD_PATCH) ==
                    net::http_method::patch)                             &&
                   (net::to_c(net::http_version::http_3) ==
                    D_HTTP_VERSION_3)                                    &&
                   (net::from_c(D_HTTP_STATUS_CLASS_REDIRECTION) ==
                    net::http_status_class::redirection)                 &&
                   (net::to_c(net::http_error::field_value) ==
                    D_HTTP_ERROR_FIELD_VALUE)                            &&
                   (net::to_c(net::http_error::host) ==
                    D_HTTP_ERROR_HOST)                                   &&
                   (static_cast<int>(net::http_status::im_a_teapot) ==
                    D_HTTP_STATUS_IM_A_TEAPOT) ),
                 "enumerators equal their C counterparts") && result;
    result = internal::check(
                 ( (net::http_status::payload_too_large ==
                    net::http_status::content_too_large)              &&
                   (net::http_status::unprocessable_entity ==
                    net::http_status::unprocessable_content)          &&
                   (net::http_status_class::unknown ==
                    net::http_status_class::none)                     &&
                   (net::http_status_class::success ==
                    net::http_status_class::successful) ),
                 "web.hpp's old names are aliases") && result;

    return result;
}

/*
tests_http_methods
  Tests the following:
  - names, and none for an extension method
  - parsing: a registered name, an extension method, a refusal that leaves
    the output as it was
  - safety and idempotence
*/
bool
tests_http_methods()
{
    net::http_method patch     = net::http_method::get;
    net::http_method extension = net::http_method::get;
    net::http_method untouched = net::http_method::trace;
    bool             result    = true;

    const net::http_error parsed  = net::parse_http_method("PATCH",
                                                           patch);
    const net::http_error other   = net::parse_http_method("get",
                                                           extension);
    const net::http_error refused = net::parse_http_method("G T",
                                                           untouched);

    result = internal::check(
                 ( (internal::same(net::http_method_name(
                                       net::http_method::delete_),
                                   "DELETE"))                        &&
                   (net::http_method_name(net::http_method::other) ==
                    nullptr) ),
                 "method names, and none for an extension") && result;
    result = internal::check(
                 ( (parsed == net::http_error::none)             &&
                   (patch == net::http_method::patch)            &&
                   (other == net::http_error::none)              &&
                   (extension == net::http_method::other)        &&
                   (refused == net::http_error::method)          &&
                   (untouched == net::http_method::trace) ),
                 "parsing; a refusal leaves the output alone") && result;
    result = internal::check(
                 ( (net::http_method_is_safe(net::http_method::head))       &&
                   (!net::http_method_is_safe(net::http_method::post))      &&
                   (net::http_method_is_idempotent(net::http_method::put))  &&
                   (!net::http_method_is_idempotent(
                        net::http_method::other)) ),
                 "safety and idempotence") && result;

    return result;
}

/*
tests_http_versions
  Tests the following:
  - names, and none for unknown
  - parsing, the higher-minor rule, and a refusal that leaves the output
*/
bool
tests_http_versions()
{
    net::http_version minor     = net::http_version::unknown;
    net::http_version untouched = net::http_version::http_2;
    bool              result    = true;

    const net::http_error higher  = net::parse_http_version("HTTP/1.7",
                                                            minor);
    const net::http_error refused = net::parse_http_version("HTTP/2.0",
                                                            untouched);

    result = internal::check(
                 ( (internal::same(net::http_version_name(
                                       net::http_version::http_1_0),
                                   "HTTP/1.0"))                        &&
                   (net::http_version_name(net::http_version::unknown) ==
                    nullptr) ),
                 "version names, and none for unknown") && result;
    result = internal::check(
                 ( (higher == net::http_error::none)             &&
                   (minor == net::http_version::http_1_1)        &&
                   (refused == net::http_error::version)         &&
                   (untouched == net::http_version::http_2) ),
                 "parsing; a refusal leaves the output alone") && result;

    return result;
}

/*
tests_http_status
  Tests the following:
  - parsing a code, and a refusal that leaves the output
  - reasons, classes, content, and redirects, by int and by name
  - a negative int is no status code
*/
bool
tests_http_status()
{
    int  found     = 0;
    int  untouched = 7;
    bool result    = true;

    const net::http_error parsed  = net::parse_http_status("404",
                                                           found);
    const net::http_error refused = net::parse_http_status("600",
                                                           untouched);

    result = internal::check(
                 ( (parsed == net::http_error::none)     &&
                   (found == 404)                        &&
                   (refused == net::http_error::status)  &&
                   (untouched == 7) ),
                 "parsing; a refusal leaves the output alone") && result;
    result = internal::check(
                 ( (internal::same(net::http_status_reason(413),
                                   "Content Too Large"))                &&
                   (internal::same(net::http_status_reason(
                                       net::http_status::ok),
                                   "OK"))                               &&
                   (net::http_status_class_of(net::http_status::found) ==
                    net::http_status_class::redirection)                &&
                   (!net::http_status_allows_content(
                        net::http_status::no_content))                  &&
                   (net::http_status_is_redirect(
                        net::http_status::see_other)) ),
                 "reasons, classes, content, and redirects") && result;
    result = internal::check(
                 ( (!net::http_status_is_valid(-200))                     &&
                   (net::http_status_class_of(-404) ==
                    net::http_status_class::none)                         &&
                   (internal::same(net::http_status_reason(-1),
                                   "")) ),
                 "a negative int is no status code") && result;

    return result;
}

/*
tests_http_syntax
  Tests the following:
  - tokens and field values, a NUL inside a std::string refused
  - trimming, and field names compared without case
  - error names
*/
bool
tests_http_syntax()
{
    const std::string with_nul("a\0b",
                               3u);
    bool              result = true;

    result = internal::check(
                 ( (net::http_is_token("Content-Type"))        &&
                   (!net::http_is_token("Content Type"))       &&
                   (net::http_is_field_value("a b"))           &&
                   (!net::http_is_field_value(with_nul)) ),
                 "tokens and values, NULs refused") && result;
    result = internal::check(
                 ( (net::http_trim_whitespace(" \tx y\t ") == "x y")    &&
                   (net::http_trim_whitespace(" \t").empty())           &&
                   (net::http_field_name_equal("ETag",
                                               "etag"))                 &&
                   (!net::http_field_name_equal("ETag",
                                                "Tag")) ),
                 "trimming, and names without case") && result;
    result = internal::check(
                 internal::same(net::http_error_name(net::http_error::status),
                                ::d_http_error_name(D_HTTP_ERROR_STATUS)),
                 "error names are the C foundation's") && result;

    return result;
}

/*
tests_http_names
  Tests the following:
  - the field and media constants are http.h's literals
*/
bool
tests_http_names()
{
    return internal::check(
               ( (internal::same(net::http_field::content_length,
                                 D_HTTP_FIELD_CONTENT_LENGTH))        &&
                 (internal::same(net::http_field::www_authenticate,
                                 "WWW-Authenticate"))                 &&
                 (internal::same(net::http_field::te,
                                 "TE"))                               &&
                 (internal::same(net::http_media::event_stream,
                                 D_HTTP_MEDIA_EVENT_STREAM)) ),
               "the constants are http.h's literals");
}

/*
tests_http_run_all
  Runs each test in order, reporting it, then the tally of checks.
*/
bool
tests_http_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "vocabulary", tests_http_vocabulary },
        { "methods",    tests_http_methods },
        { "versions",   tests_http_versions },
        { "status",     tests_http_status },
        { "syntax",     tests_http_syntax },
        { "names",      tests_http_names }
    };
    bool all = true;

    // each test, reported as it finishes
    for (const entry& test : CASES)
    {
        const bool passed = test.run();

        std::printf("  %s  %s\n",
                    (passed) ? "pass" : "FAIL",
                    test.name);
        all = (passed) && all;
    }

    std::printf("%zu/%zu checks\n",
                internal::counts().run - internal::counts().failed,
                internal::counts().run);

    return all;
}

NS_END  // testing
NS_END  // djinterp
