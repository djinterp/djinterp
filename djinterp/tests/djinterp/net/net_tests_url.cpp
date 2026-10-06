/*******************************************************************************
* djinterp [net]                                               net_tests_url.cpp
*
* Tests of net/net_url.hpp, the C++ layer over net/net_url.h.
*   Values are checked against the C foundation, whose own suite checks them
* against RFC 3986. What is new in C++ is ownership, so that is checked
* directly: every view a url hands out must point into its own text, after
* copies and moves of a short text, held in a string's inline buffer, and of
* a long one, held on the heap. Texts over 256 bytes take the producers'
* second-call path.
*
*
* path:      /tests/djinterp/net/net_tests_url.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./net_tests.hpp"  // corresponding header
// std
#include <cstddef>     // std::size_t
#include <functional>  // std::less
#include <string>      // std::string
#include <utility>     // std::move
// djinterp
#include "../../../inc/djinterp/net/net_url.hpp"  // the layer under test


NS_DJINTERP
NS_TESTING

namespace net = ::djinterp::net;

NS_INTERNAL

    // same
    //   function: whether a view holds exactly a string's characters.
    static bool
    same(
        net::url::view_type _view,
        const std::string&  _text
    )
    {
        return ( (_view.size() == _text.size()) &&
                 ( (_text.empty()) ||
                   (_text.compare(0u,
                                  _text.size(),
                                  _view.data(),
                                  _view.size()) == 0) ) );
    }

    // owns
    //   function: whether every present view of a url lies inside its own
    // text; absent views point nowhere.
    static bool
    owns(
        const net::url& _url
    )
    {
        const std::less<const char*> before;
        const char* const            begin = _url.str().data();
        const char* const            end   = begin + _url.str().size();
        const net::url::view_type    views[] =
        {
            _url.scheme(),
            _url.userinfo(),
            _url.host(),
            _url.port_text(),
            _url.path(),
            _url.query(),
            _url.fragment()
        };

        // each view, bounded by the url's text
        for (const net::url::view_type& view : views)
        {
            // a present view outside the text
            if ( (view.data() != nullptr)             &&
                 ( (before(view.data(),
                           begin)) ||
                   (before(end,
                           view.data() + view.size())) ) )
            {
                return false;
            }
        }

        return true;
    }

    // intact
    //   function: whether a url holds a text, owns its views, and has the
    // query "q" and the fragment "f" every ownership text ends with.
    static bool
    intact(
        const net::url&    _url,
        const std::string& _text
    )
    {
        return ( (_url.str() == _text)      &&
                 (owns(_url))               &&
                 (same(_url.query(),
                       "q"))                &&
                 (same(_url.fragment(),
                       "f")) );
    }

NS_END  // internal


/*
tests_net_url_vocabulary
  Tests the following:
  - enumerators equal their C counterparts, converted either way
  - error names are the C foundation's
*/
bool
tests_net_url_vocabulary()
{
    bool result = true;

    result = internal::check(
                 ( (net::to_c(net::url_error::buffer) ==
                    D_NET_URL_ERROR_BUFFER)                          &&
                   (net::from_c(D_NET_URL_ERROR_PORT) ==
                    net::url_error::port)                            &&
                   (net::to_c(net::url_host::ipv_future) ==
                    D_NET_URL_HOST_IPVFUTURE)                        &&
                   (net::from_c(D_NET_URL_HOST_IPV4) ==
                    net::url_host::ipv4)                             &&
                   (net::from_c(D_NET_URL_PART_SEGMENT) ==
                    net::url_part::segment) ),
                 "url enumerators equal their C counterparts") && result;
    result = internal::check(
                 std::string(net::url_error_name(net::url_error::percent)) ==
                 ::d_net_url_error_name(D_NET_URL_ERROR_PERCENT),
                 "error names are the C foundation's") && result;

    return result;
}

/*
tests_net_url_parse
  Tests the following:
  - a parsed url's components, as views into its own text
  - presence, host kind, effective port, and TLS
  - a refusal's error and position, with the empty reference as its url
*/
bool
tests_net_url_parse()
{
    const net::url_result full   = net::url::parse(
                                       "https://user@Example.COM/a/b?q#f");
    const net::url_result bad    = net::url::parse("http://h:99999/");
    const net::url&       parsed = full.value;
    bool                  result = true;

    result = internal::check(
                 ( (full.ok())                        &&
                   (internal::same(parsed.scheme(),
                                   "https"))          &&
                   (internal::same(parsed.userinfo(),
                                   "user"))           &&
                   (internal::same(parsed.host(),
                                   "Example.COM"))    &&
                   (internal::same(parsed.path(),
                                   "/a/b"))           &&
                   (internal::owns(parsed)) ),
                 "a url's components are views into its own text") &&
             result;
    result = internal::check(
                 ( (parsed.has_userinfo())                 &&
                   (!parsed.has_port())                    &&
                   (parsed.effective_port() == 443u)       &&
                   (parsed.is_tls())                       &&
                   (parsed.is_absolute())                  &&
                   (parsed.host_kind() == net::url_host::name) ),
                 "presence, kind, effective port, and TLS") && result;
    result = internal::check(
                 ( (!bad.ok())                             &&
                   (bad.error == net::url_error::port)     &&
                   (bad.position == 9u)                    &&
                   (bad.value.str().empty())               &&
                   (bad.value.path().empty()) ),
                 "a refusal says why and where, with an empty url") &&
             result;

    return result;
}

/*
tests_net_url_ownership
  Tests the following:
  - copy and move construction and assignment, of a short text and of a
    heap-sized one, rebase every view onto the new url's own text
  - a copy outlives its source
  - a moved-from url is the empty reference; self-assignment changes
    nothing
*/
bool
tests_net_url_ownership()
{
    const std::string texts[] =
    {
        "http://h/p?q#f",
        "http://example.com/" + std::string(300u,
                                            'a') + "?q#f"
    };
    bool result = true;

    // an inline-buffer text, then a heap one
    for (const std::string& text : texts)
    {
        net::url copied;
        net::url moved;

        // the source dies before its copies are read
        {
            net::url source = net::url::parse(text).value;

            copied = source;
            moved  = std::move(source);

            result = internal::check(
                         ( (source.str().empty()) &&
                           (internal::owns(source)) ),
                         "a moved-from url is the empty reference") &&
                     result;
        }

        net::url  constructed(std::move(moved));
        net::url  duplicate(copied);
        net::url& alias = duplicate;

        duplicate = std::move(alias);

        result = internal::check(
                     ( (internal::intact(copied,
                                         text))      &&
                       (internal::intact(constructed,
                                         text))      &&
                       (internal::intact(duplicate,
                                         text)) ),
                     "copies and moves own their views") && result;
    }

    return result;
}

/*
tests_net_url_resolution
  Tests the following:
  - resolution against a base, including a target over 256 bytes
  - a relative base is refused
  - a normal form, parsed as a url of its own
*/
bool
tests_net_url_resolution()
{
    const std::string     tail    = std::string(300u,
                                                'x');
    const net::url        base    = net::url::parse(
                                        "http://a/b/c/d;p?q").value;
    const net::url_result near    = base.resolve(
                                        net::url::parse("../g").value);
    const net::url_result far     = base.resolve(
                                        net::url::parse("g/../../" +
                                                        tail).value);
    const net::url_result refused = net::url::parse("g").value.resolve(base);
    const net::url_result normal  = net::url::parse(
                                        "HTTP://Example.COM:80/%7Ea/./b")
                                        .value.normalized();
    bool                  result  = true;

    result = internal::check(
                 ( (near.ok())                              &&
                   (near.value.str() == "http://a/b/g")     &&
                   (internal::owns(near.value))             &&
                   (far.ok())                               &&
                   (far.value.str() == "http://a/b/" + tail) ),
                 "targets resolve, at any length") && result;
    result = internal::check(
                 ( (!refused.ok()) &&
                   (refused.error == net::url_error::base) ),
                 "a relative base is refused") && result;
    result = internal::check(
                 ( (normal.ok()) &&
                   (normal.value.str() == "http://example.com/~a/b") ),
                 "a normal form is a url of its own") && result;

    return result;
}

/*
tests_net_url_encoding
  Tests the following:
  - each part encodes what it must, including output over 256 bytes
  - decoding reverses encoding, keeps interior NULs, and refuses a
    malformed escape
*/
bool
tests_net_url_encoding()
{
    const std::string          raw     = "a b/c?d#e%f";
    const std::string          spaces(300u,
                                      ' ');
    const net::url_text_result nul     = net::url_decode("x%00y");
    const net::url_text_result bad     = net::url_decode("%zz");
    const std::string          encoded = net::url_encode(
                                             spaces,
                                             net::url_part::component);
    bool                       result  = true;

    result = internal::check(
                 ( (net::url_encode(raw,
                                    net::url_part::component) ==
                    "a%20b%2Fc%3Fd%23e%25f")             &&
                   (net::url_encode(raw,
                                    net::url_part::path) ==
                    "a%20b/c%3Fd%23e%25f")               &&
                   (encoded.size() == 900u) ),
                 "each part encodes what it must") && result;
    result = internal::check(
                 ( (net::url_decode(encoded).text == spaces)     &&
                   (nul.ok())                                    &&
                   (nul.text == std::string("x\0y",
                                            3u))                 &&
                   (!bad.ok())                                   &&
                   (bad.error == net::url_error::percent)        &&
                   (bad.text.empty()) ),
                 "decoding reverses, keeps NULs, refuses bad escapes") &&
             result;

    return result;
}

/*
tests_net_url_endpoints
  Tests the following:
  - a URL's endpoint, by its scheme's default port or its own, with an
    IPv6 zone decoded
  - a URL naming no host is refused, and the output left untouched
  - default ports and TLS schemes
*/
bool
tests_net_url_endpoints()
{
    net::endpoint web;
    net::endpoint zoned;
    net::endpoint untouched("keep",
                            7u);
    bool          result = true;

    const net::io_error reached = net::url_endpoint(
                                      net::url::parse(
                                          "https://example.com/x").value,
                                      web);
    const net::io_error scoped  = net::url_endpoint(
                                      net::url::parse(
                                          "http://[fe80::1%25eth0]:8080/")
                                          .value,
                                      zoned);
    const net::io_error refused = net::url_endpoint(
                                      net::url::parse(
                                          "mailto:x@example.com").value,
                                      untouched);

    result = internal::check(
                 ( (reached == net::io_error::none)   &&
                   (web.host == "example.com")        &&
                   (web.port == 443u)                 &&
                   (web.proto == net::protocol::tcp)  &&
                   (scoped == net::io_error::none)    &&
                   (zoned.host == "fe80::1%eth0")     &&
                   (zoned.port == 8080u) ),
                 "URLs reach their endpoints") && result;
    result = internal::check(
                 ( (refused == net::io_error::address_invalid) &&
                   (untouched.host == "keep")                  &&
                   (untouched.port == 7u) ),
                 "a URL naming no host is refused, output untouched") &&
             result;
    result = internal::check(
                 ( (net::url_default_port("HTTPS") == 443u) &&
                   (net::url_scheme_is_tls("wss"))          &&
                   (!net::url_scheme_is_tls("http")) ),
                 "default ports and TLS schemes") && result;

    return result;
}

NS_END  // testing
NS_END  // djinterp
