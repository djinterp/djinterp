/*******************************************************************************
* djinterp [net]                                              net_tests_sa_url.c
*
* Standalone tests of net.h's URLs (section 8).
*   Resolution is checked against every example in RFC 3986, section 5.4,
* normal and abnormal, verbatim. The rest checks components, absent against
* empty, refusals and where they point, every host form, recomposition,
* normalization, percent-encoding, and endpoints.
*
*
* path:      /tests/djinterp/net/net_tests_sa_url.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./net_tests_sa.h"  // the suite
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memcmp, strcmp, strlen
// djinterp
#include "../../../inc/djinterp/net/net_url.h"  // the URL API


// d_tests_url_pair
//   struct: an input and the text it must become.
struct d_tests_url_pair
{
    const char* input;
    const char* expected;
};

// d_tests_url_expect
//   struct: the text components a URL must split into.
struct d_tests_url_expect
{
    const char* scheme;
    const char* userinfo;
    const char* host;
    const char* path;
    const char* query;
    const char* fragment;
};

// d_tests_url_endpoint_case
//   struct: a URL and the endpoint it must reach.
struct d_tests_url_endpoint_case
{
    const char* url;
    const char* host;
    d_net_port  port;
};

// d_tests_url_refusal
//   struct: a reference that must be refused, why, and where.
struct d_tests_url_refusal
{
    const char*          text;
    enum d_net_url_error error;
    size_t               at;
};

// RESOLUTIONS
//   constant: RFC 3986, section 5.4, against the base "http://a/b/c/d;p?q".
static const struct d_tests_url_pair RESOLUTIONS[] =
{
    { "g:h",           "g:h" },
    { "g",             "http://a/b/c/g" },
    { "./g",           "http://a/b/c/g" },
    { "g/",            "http://a/b/c/g/" },
    { "/g",            "http://a/g" },
    { "//g",           "http://g" },
    { "?y",            "http://a/b/c/d;p?y" },
    { "g?y",           "http://a/b/c/g?y" },
    { "#s",            "http://a/b/c/d;p?q#s" },
    { "g#s",           "http://a/b/c/g#s" },
    { "g?y#s",         "http://a/b/c/g?y#s" },
    { ";x",            "http://a/b/c/;x" },
    { "g;x",           "http://a/b/c/g;x" },
    { "g;x?y#s",       "http://a/b/c/g;x?y#s" },
    { "",              "http://a/b/c/d;p?q" },
    { ".",             "http://a/b/c/" },
    { "./",            "http://a/b/c/" },
    { "..",            "http://a/b/" },
    { "../",           "http://a/b/" },
    { "../g",          "http://a/b/g" },
    { "../..",         "http://a/" },
    { "../../",        "http://a/" },
    { "../../g",       "http://a/g" },
    { "../../../g",    "http://a/g" },
    { "../../../../g", "http://a/g" },
    { "/./g",          "http://a/g" },
    { "/../g",         "http://a/g" },
    { "g.",            "http://a/b/c/g." },
    { ".g",            "http://a/b/c/.g" },
    { "g..",           "http://a/b/c/g.." },
    { "..g",           "http://a/b/c/..g" },
    { "./../g",        "http://a/b/g" },
    { "./g/.",         "http://a/b/c/g/" },
    { "g/./h",         "http://a/b/c/g/h" },
    { "g/../h",        "http://a/b/c/h" },
    { "g;x=1/./y",     "http://a/b/c/g;x=1/y" },
    { "g;x=1/../y",    "http://a/b/c/y" },
    { "g?y/./x",       "http://a/b/c/g?y/./x" },
    { "g?y/../x",      "http://a/b/c/g?y/../x" },
    { "g#s/./x",       "http://a/b/c/g#s/./x" },
    { "g#s/../x",      "http://a/b/c/g#s/../x" },
    { "http:g",        "http:g" }
};

// REFUSALS
//   constant: malformed references, their errors, and the offsets named.
static const struct d_tests_url_refusal REFUSALS[] =
{
    { "1a:b",                   D_NET_URL_ERROR_SCHEME,     0u  },
    { ":x",                     D_NET_URL_ERROR_SCHEME,     0u  },
    { "http://u^@h",            D_NET_URL_ERROR_USERINFO,   8u  },
    { "http://a@b@c",           D_NET_URL_ERROR_HOST,       10u },
    { "http://h o/",            D_NET_URL_ERROR_HOST,       8u  },
    { "http://[::1",            D_NET_URL_ERROR_IP_LITERAL, 7u  },
    { "http://[1:2:3:4:5:6:7:8:9]/", D_NET_URL_ERROR_IP_LITERAL, 8u },
    { "http://[::1]x/",         D_NET_URL_ERROR_IP_LITERAL, 12u },
    { "http://h:8x/",           D_NET_URL_ERROR_PORT,       10u },
    { "http://h:65536/",        D_NET_URL_ERROR_PORT,       9u  },
    { "http://h/a b",           D_NET_URL_ERROR_PATH,       10u },
    { "http://h/?a b",          D_NET_URL_ERROR_QUERY,      11u },
    { "http://h/#a#b",          D_NET_URL_ERROR_FRAGMENT,   11u },
    { "http://h/%2",            D_NET_URL_ERROR_PERCENT,    9u  },
    { "http://h/%zz",           D_NET_URL_ERROR_PERCENT,    9u  }
};

// IPV6_VALID, IPV6_INVALID
//   constant: bracketed hosts that are, and are not, IPv6 literals.
static const char* const IPV6_VALID[] =
{
    "[::]", "[::1]", "[1::]", "[1:2:3:4:5:6:7:8]", "[1:2:3:4:5:6:7::]",
    "[::ffff:1.2.3.4]", "[1:2:3:4:5:6:1.2.3.4]", "[2001:DB8::1:0:0:1]",
    "[fe80::1%25eth0]"
};
static const char* const IPV6_INVALID[] =
{
    "[:1]", "[1:::2]", "[1::2::3]", "[12345::]", "[1:2:3:4:5:6:7:8:9]",
    "[1.2.3.4]", "[::1.2.3.256]", "[::01.2.3.4]", "[1:2:3:4:5:6:7:1.2.3.4]",
    "[fe80::1%eth0]", "[fe80::1%25]", "[1:2:3:4:5:6:7]"
};

// ROUND_TRIPS
//   constant: references that recompose to themselves, empty parts kept.
static const char* const ROUND_TRIPS[] =
{
    "https://user:pw@example.com:8443/a/b;p?q=1#frag",
    "http://h?", "http://h#", "http://h:", "http://@h", "file:///etc/hosts",
    "//h/p", "/p?q", "p/q", "", "?", "#", "mailto:x@example.com",
    "http://[2001:db8::1]:80/", "http://[v7.abc]/", "urn:isbn:0451450523"
};

// NORMALIZATIONS
//   constant: references and their normal forms (RFC 3986, 6.2).
static const struct d_tests_url_pair NORMALIZATIONS[] =
{
    { "HTTP://User@Example.COM:80/%7Efoo/./bar/../baz?%7a#%7A",
      "http://User@example.com/~foo/baz?z#z" },
    { "https://h:0443",           "https://h/" },
    { "http://h:8080",            "http://h:8080/" },
    { "http://h:08080/",          "http://h:8080/" },
    { "http://[2001:DB8::1]/",    "http://[2001:db8::1]/" },
    { "http://[fe80::A%25Eth0]/", "http://[fe80::a%25Eth0]/" },
    { "http://h/%2f%3a",          "http://h/%2F%3A" },
    { "ftp://h:21",               "ftp://h" },
    { "a/./b/../c",               "a/./b/../c" },
    { "http://h:/",               "http://h/" }
};

// ENDPOINTS, UNREACHABLE
//   constant: URLs and the endpoints they reach; URLs that reach none.
static const struct d_tests_url_endpoint_case ENDPOINTS[] =
{
    { "https://example.com/x",    "example.com",  443u  },
    { "http://[::1]:8080/",       "::1",          8080u },
    { "http://[fe80::1%25eth0]/", "fe80::1%eth0", 80u   },
    { "http://ex%61mple.com",     "example.com",  80u   },
    { "foo://h:9/",               "h",            9u    }
};
static const char* const UNREACHABLE[] =
{
    "mailto:x@example.com", "foo://h/", "http://[v1.x]/",
    "http://%C3%A9.example/"
};

/*
d_tests_url_cstr
  A view of a C string; the net foundation links nothing that makes one.
*/
static struct d_pack_text
d_tests_url_cstr(
    const char* _text
)
{
    const struct d_pack_text view = { _text,
                                      strlen(_text) };

    return view;
}

/*
d_tests_url_parse_cstr
  Parses a C string into a URL.
*/
static enum d_net_url_error
d_tests_url_parse_cstr(
    const char*       _text,
    struct d_net_url* _out,
    size_t*           _at
)
{
    return d_net_url_parse(d_tests_url_cstr(_text),
                           _out,
                           _at);
}

/*
d_tests_url_text_is
  Whether a view holds exactly a C string.
*/
static bool
d_tests_url_text_is(
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
d_tests_url_parts_are
  Whether a parsed URL's text components are exactly those expected.
*/
static bool
d_tests_url_parts_are(
    const struct d_net_url*          _url,
    const struct d_tests_url_expect* _expect
)
{
    return ( (d_tests_url_text_is(_url->scheme,
                                  _expect->scheme))   &&
             (d_tests_url_text_is(_url->userinfo,
                                  _expect->userinfo)) &&
             (d_tests_url_text_is(_url->host,
                                  _expect->host))     &&
             (d_tests_url_text_is(_url->path,
                                  _expect->path))     &&
             (d_tests_url_text_is(_url->query,
                                  _expect->query))    &&
             (d_tests_url_text_is(_url->fragment,
                                  _expect->fragment)) );
}

/*
d_tests_url_kinds_hold
  Whether each host kind is recognized: IPv4, zoned IPv6 with a port, and
IPvFuture.
*/
static bool
d_tests_url_kinds_hold(void)
{
    struct d_net_url ipv4   = { .port = 0u };
    struct d_net_url ipv6   = { .port = 0u };
    struct d_net_url future = { .port = 0u };

    return ( (d_tests_url_parse_cstr("http://10.0.0.1/",
                                     &ipv4,
                                     NULL) == D_NET_URL_OK)        &&
             (ipv4.host_kind == D_NET_URL_HOST_IPV4)               &&
             (d_tests_url_parse_cstr("http://[fe80::1%25en0]:9/",
                                     &ipv6,
                                     NULL) == D_NET_URL_OK)        &&
             (ipv6.host_kind == D_NET_URL_HOST_IPV6)               &&
             (d_tests_url_text_is(ipv6.host,
                                  "fe80::1%25en0"))                &&
             (ipv6.port == 9u)                                     &&
             (d_tests_url_parse_cstr("http://[v7.a:b]/",
                                     &future,
                                     NULL) == D_NET_URL_OK)        &&
             (future.host_kind == D_NET_URL_HOST_IPVFUTURE) );
}

/*
d_tests_url_marks_hold
  Whether absent parts are told from empty ones, and the relative forms
parse: a network-path reference and the empty one.
*/
static bool
d_tests_url_marks_hold(void)
{
    struct d_net_url marked  = { .port = 0u };
    struct d_net_url network = { .port = 0u };
    struct d_net_url empty   = { .port = 0u };

    return ( (d_tests_url_parse_cstr("http://@h:?",
                                     &marked,
                                     NULL) == D_NET_URL_OK) &&
             (marked.has_userinfo)                          &&
             (marked.has_port)                              &&
             (marked.has_query)                             &&
             (!marked.has_fragment)                         &&
             (marked.port_text.length == 0u)                &&
             (d_tests_url_parse_cstr("//h/p",
                                     &network,
                                     NULL) == D_NET_URL_OK) &&
             (network.has_authority)                        &&
             (!d_net_url_is_absolute(&network))             &&
             (d_tests_url_parse_cstr("",
                                     &empty,
                                     NULL) == D_NET_URL_OK) &&
             (!empty.has_authority)                         &&
             (empty.path.length == 0u) );
}

/*
d_tests_sa_net_url_components
  Tests the following:
  - every component of a full URL, without its delimiters
  - each host kind: a name, IPv4, IPv6 with a zone, IPvFuture
  - absent parts told from empty ones
  - relative references: network-path and empty
*/
bool
d_tests_sa_net_url_components(
    struct d_test_counter* _counter
)
{
    const char* const               TEST   = "d_net_url_parse";
    const struct d_tests_url_expect FULL   = { "https",
                                               "user:pw",
                                               "Example.COM",
                                               "/a/b;p",
                                               "q=1",
                                               "f" };
    struct d_net_url                url    = { .port = 0u };
    bool                            result = true;

    const bool full = ( (d_tests_url_parse_cstr(
                             "https://user:pw@Example.COM:8443/a/b;p?q=1#f",
                             &url,
                             NULL) == D_NET_URL_OK)      &&
                        (d_tests_url_parts_are(&url,
                                               &FULL))   &&
                        (url.port == 8443u)              &&
                        (url.host_kind == D_NET_URL_HOST_NAME) &&
                        (d_net_url_is_absolute(&url)) );

    result = d_assert_standalone(full,
                                 TEST,
                                 "a full URL splits into its components",
                                 _counter) && result;
    result = d_assert_standalone(d_tests_url_kinds_hold(),
                                 TEST,
                                 "names, IPv4, zoned IPv6, IPvFuture",
                                 _counter) && result;
    result = d_assert_standalone(d_tests_url_marks_hold(),
                                 TEST,
                                 "absent parts are told from empty ones",
                                 _counter) && result;

    return result;
}

/*
d_tests_url_authority_ok
  Whether an authority parses alone, with the host, port, and userinfo
presence given.
*/
static bool
d_tests_url_authority_ok(
    const char*         _text,
    const char*         _host,
    d_net_port          _port,
    enum d_net_url_host _kind
)
{
    struct d_net_url url = { .port = 0u };

    return ( (d_net_url_parse_authority(d_tests_url_cstr(_text),
                                        &url,
                                        NULL) == D_NET_URL_OK) &&
             (url.has_authority)                               &&
             (d_tests_url_text_is(url.host,
                                  _host))                      &&
             (url.port == _port)                               &&
             (url.host_kind == _kind) );
}

/*
d_tests_url_authority_refused
  Whether an authority is refused with an error, at an offset.
*/
static bool
d_tests_url_authority_refused(
    const char*          _text,
    enum d_net_url_error _error,
    size_t               _at
)
{
    struct d_net_url url   = { .port = 0u };
    size_t           where = 99u;

    return ( (d_net_url_parse_authority(d_tests_url_cstr(_text),
                                        &url,
                                        &where) == _error) &&
             (where == _at)                                &&
             (url.text.data == NULL) );
}

/*
d_tests_sa_net_url_authority
  Tests the following:
  - an authority alone: a name and port, an IPv6 literal, userinfo, and the
    empty authority an HTTP Host may be
  - refusals, each at its offset: a path character, a port out of range,
    an unclosed bracket
*/
bool
d_tests_sa_net_url_authority(
    struct d_test_counter* _counter
)
{
    struct d_net_url user   = { .port = 0u };
    bool             result = true;

    result = d_assert_standalone(
                 (d_tests_url_authority_ok("example.com:8080",
                                           "example.com",
                                           8080u,
                                           D_NET_URL_HOST_NAME))        &&
                 (d_tests_url_authority_ok("[::1]:443",
                                           "::1",
                                           443u,
                                           D_NET_URL_HOST_IPV6))        &&
                 (d_tests_url_authority_ok("",
                                           "",
                                           0u,
                                           D_NET_URL_HOST_NAME))        &&
                 (d_net_url_parse_authority(d_tests_url_cstr("user@h"),
                                            &user,
                                            NULL) == D_NET_URL_OK)      &&
                 (user.has_userinfo)                                    &&
                 (d_tests_url_text_is(user.userinfo,
                                      "user")),
                 "d_net_url_parse_authority",
                 "names, IPv6, userinfo, and the empty authority",
                 _counter) && result;
    result = d_assert_standalone(
                 (d_tests_url_authority_refused("h/x",
                                                D_NET_URL_ERROR_HOST,
                                                1u))                    &&
                 (d_tests_url_authority_refused("h:99999",
                                                D_NET_URL_ERROR_PORT,
                                                2u))                    &&
                 (d_tests_url_authority_refused("[::1",
                                                D_NET_URL_ERROR_IP_LITERAL,
                                                0u)),
                 "d_net_url_parse_authority",
                 "refusals name their error and offset",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_net_url_refusals
  Tests the following:
  - each malformed reference in REFUSALS gets its error, at its offset
  - a refused parse zeroes the result
  - NULL arguments are refused
*/
bool
d_tests_sa_net_url_refusals(
    struct d_test_counter* _counter
)
{
    const char* const        TEST    = "d_net_url_parse refusals";
    const struct d_pack_text NO_TEXT = { NULL,
                                         3u };
    const size_t      count   = sizeof(REFUSALS) / sizeof(REFUSALS[0]);
    struct d_net_url  url     = { .port = 0u };
    size_t            matched = 0u;
    bool              result  = true;

    // the first mismatch, named in the message
    char message[160] = "every refusal names its error and offset";

    // each refusal, error and offset both
    for (size_t i = 0u; i < count; ++i)
    {
        size_t                     at    = 99u;
        const enum d_net_url_error found = d_tests_url_parse_cstr(
                                               REFUSALS[i].text,
                                               &url,
                                               &at);

        // a match, or the first mismatch recorded
        if ( (found == REFUSALS[i].error) &&
             (at == REFUSALS[i].at)       &&
             (url.text.data == NULL) )
        {
            matched += 1u;
        }
        else if (matched == i)
        {
            (void)snprintf(message,
                           sizeof(message),
                           "\"%s\": %s at %zu",
                           REFUSALS[i].text,
                           d_net_url_error_name(found),
                           at);
        }
    }

    result = d_assert_standalone(matched == count,
                                 TEST,
                                 message,
                                 _counter) && result;
    result = d_assert_standalone(
                 (d_net_url_parse(d_tests_url_cstr("x"),
                                  NULL,
                                  NULL) == D_NET_URL_ERROR_ARGUMENT) &&
                 (d_net_url_parse(NO_TEXT,
                                  &url,
                                  NULL) == D_NET_URL_ERROR_ARGUMENT),
                 TEST,
                 "NULL arguments are refused",
                 _counter) && result;

    return result;
}

/*
d_tests_url_count_valid
  How many of a list of hosts, each put in "http://<host>/", parse, and of
kind _kind.
*/
static size_t
d_tests_url_count_valid(
    const char* const*  _hosts,
    size_t              _count,
    enum d_net_url_host _kind
)
{
    size_t valid = 0u;

    // each host, inside a URL
    for (size_t i = 0u; i < _count; ++i)
    {
        struct d_net_url url = { .port = 0u };

        // the text as a URL
        char text[96] = { 0 };

        (void)snprintf(text,
                       sizeof(text),
                       "http://%s/",
                       _hosts[i]);

        // parsed, and of the kind asked
        if ( (d_tests_url_parse_cstr(text,
                                     &url,
                                     NULL) == D_NET_URL_OK) &&
             (url.host_kind == _kind) )
        {
            valid += 1u;
        }
    }

    return valid;
}

/*
d_tests_sa_net_url_hosts
  Tests the following:
  - every literal in IPV6_VALID parses as IPv6, and none in IPV6_INVALID
  - dotted-decimal hosts are IPv4 only when strictly so; otherwise names
*/
bool
d_tests_sa_net_url_hosts(
    struct d_test_counter* _counter
)
{
    const char* const TEST     = "IP literals and IPv4";
    const size_t      valid    = sizeof(IPV6_VALID) / sizeof(IPV6_VALID[0]);
    const size_t      invalid  = sizeof(IPV6_INVALID) /
                                 sizeof(IPV6_INVALID[0]);
    const char* const ipv4[]   = { "192.168.0.1", "0.0.0.0",
                                   "255.255.255.255" };
    const char* const names[]  = { "256.1.1.1", "01.2.3.4", "1.2.3",
                                   "1.2.3.4.5", "example.com" };
    bool              result   = true;

    result = d_assert_standalone(
                 d_tests_url_count_valid(IPV6_VALID,
                                         valid,
                                         D_NET_URL_HOST_IPV6) == valid,
                 TEST,
                 "every valid IPv6 literal parses as IPv6",
                 _counter) && result;
    result = d_assert_standalone(
                 d_tests_url_count_valid(IPV6_INVALID,
                                         invalid,
                                         D_NET_URL_HOST_IPV6) == 0u,
                 TEST,
                 "no malformed IPv6 literal parses",
                 _counter) && result;
    result = d_assert_standalone(
                 (d_tests_url_count_valid(ipv4,
                                          3u,
                                          D_NET_URL_HOST_IPV4) == 3u) &&
                 (d_tests_url_count_valid(names,
                                          5u,
                                          D_NET_URL_HOST_NAME) == 5u),
                 TEST,
                 "strict dotted-decimal is IPv4; the rest are names",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_net_url_format
  Tests the following:
  - each of ROUND_TRIPS recomposes to its own text, empty parts kept
  - a buffer too small reports BUFFER and the exact length
*/
bool
d_tests_sa_net_url_format(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "d_net_url_format";
    const size_t      count  = sizeof(ROUND_TRIPS) / sizeof(ROUND_TRIPS[0]);
    size_t            same   = 0u;
    size_t            length = 0u;
    struct d_net_url  url    = { .port = 0u };
    bool              result = true;

    // the recomposed text
    char text[128] = { 0 };

    // each reference, parsed and recomposed
    for (size_t i = 0u; i < count; ++i)
    {
        // parsed, recomposed, and unchanged
        if ( (d_tests_url_parse_cstr(ROUND_TRIPS[i],
                                     &url,
                                     NULL) == D_NET_URL_OK) &&
             (d_net_url_format(&url,
                               text,
                               sizeof(text),
                               &length) == D_NET_URL_OK) &&
             (strcmp(text,
                     ROUND_TRIPS[i]) == 0) )
        {
            same += 1u;
        }
    }

    const bool sized = ( (d_tests_url_parse_cstr(ROUND_TRIPS[0],
                                                 &url,
                                                 NULL) == D_NET_URL_OK) &&
                         (d_net_url_format(&url,
                                           NULL,
                                           0u,
                                           &length) ==
                          D_NET_URL_ERROR_BUFFER) &&
                         (length == strlen(ROUND_TRIPS[0])) );

    result = d_assert_standalone(same == count,
                                 TEST,
                                 "every reference recomposes to itself",
                                 _counter) && result;
    result = d_assert_standalone(sized,
                                 TEST,
                                 "a sizing call reports the exact length",
                                 _counter) && result;

    return result;
}

/*
d_tests_url_resolve_edges
  Whether a relative base is refused, and a sizing call reports a length
that then suffices for the target.
*/
static bool
d_tests_url_resolve_edges(
    const struct d_net_url* _base
)
{
    struct d_net_url ref    = { .port = 0u };
    size_t           length = 0u;

    // the target
    char target[128] = { 0 };

    return ( (d_tests_url_parse_cstr("../g",
                                     &ref,
                                     NULL) == D_NET_URL_OK)           &&
             (d_net_url_resolve(&ref,
                                &ref,
                                target,
                                sizeof(target),
                                &length) == D_NET_URL_ERROR_BASE)     &&
             (d_net_url_resolve(_base,
                                &ref,
                                NULL,
                                0u,
                                &length) == D_NET_URL_ERROR_BUFFER)   &&
             (length >= strlen("http://a/b/g"))                       &&
             (d_net_url_resolve(_base,
                                &ref,
                                target,
                                length + 1u,
                                &length) == D_NET_URL_OK)             &&
             (strcmp(target,
                     "http://a/b/g") == 0) );
}

/*
d_tests_sa_net_url_resolve
  Tests the following:
  - every example of RFC 3986, section 5.4, normal and abnormal
  - a relative base is refused
  - a buffer too small reports a length that then suffices
*/
bool
d_tests_sa_net_url_resolve(
    struct d_test_counter* _counter
)
{
    const char* const TEST    = "d_net_url_resolve";
    const size_t      count   = sizeof(RESOLUTIONS) / sizeof(RESOLUTIONS[0]);
    struct d_net_url  base    = { .port = 0u };
    struct d_net_url  ref     = { .port = 0u };
    size_t            right   = 0u;
    size_t            length  = 0u;
    bool              result  = true;

    // the target, and the first wrong one, named
    char target[128] = { 0 };
    char message[160] = "all RFC 3986 section 5.4 examples resolve";

    (void)d_tests_url_parse_cstr("http://a/b/c/d;p?q",
                                 &base,
                                 NULL);

    // each example
    for (size_t i = 0u; i < count; ++i)
    {
        // parsed, resolved, and as the RFC says
        if ( (d_tests_url_parse_cstr(RESOLUTIONS[i].input,
                                     &ref,
                                     NULL) == D_NET_URL_OK) &&
             (d_net_url_resolve(&base,
                                &ref,
                                target,
                                sizeof(target),
                                &length) == D_NET_URL_OK) &&
             (strcmp(target,
                     RESOLUTIONS[i].expected) == 0) )
        {
            right += 1u;
        }
        else if (right == i)
        {
            (void)snprintf(message,
                           sizeof(message),
                           "\"%s\" resolved to \"%s\"",
                           RESOLUTIONS[i].input,
                           target);
        }
    }

    result = d_assert_standalone(right == count,
                                 TEST,
                                 message,
                                 _counter) && result;
    result = d_assert_standalone(d_tests_url_resolve_edges(&base),
                                 TEST,
                                 "a relative base is refused; sizing works",
                                 _counter) && result;

    return result;
}

/*
d_tests_url_matches
  How many pairs a function taking a parsed URL maps as expected.
*/
static size_t
d_tests_url_matches(
    const struct d_tests_url_pair* _pairs,
    size_t                         _count
)
{
    size_t same = 0u;

    // each pair, normalized and compared
    for (size_t i = 0u; i < _count; ++i)
    {
        struct d_net_url url    = { .port = 0u };
        size_t           length = 0u;

        // the normal form
        char text[128] = { 0 };

        // parsed, normalized, and as expected
        if ( (d_tests_url_parse_cstr(_pairs[i].input,
                                     &url,
                                     NULL) == D_NET_URL_OK) &&
             (d_net_url_normalize(&url,
                                  text,
                                  sizeof(text),
                                  &length) == D_NET_URL_OK) &&
             (strcmp(text,
                     _pairs[i].expected) == 0) )
        {
            same += 1u;
        }
    }

    return same;
}

/*
d_tests_sa_net_url_normalize
  Tests the following:
  - case, escapes, dot segments, default and empty ports, and the empty
    web path, as NORMALIZATIONS lists them
  - an IPv6 zone keeps its case; a relative reference keeps its dots
*/
bool
d_tests_sa_net_url_normalize(
    struct d_test_counter* _counter
)
{
    const size_t count = sizeof(NORMALIZATIONS) / sizeof(NORMALIZATIONS[0]);

    return d_assert_standalone(d_tests_url_matches(NORMALIZATIONS,
                                                   count) == count,
                               "d_net_url_normalize",
                               "every reference takes its normal form",
                               _counter);
}

/*
d_tests_url_encodes_to
  Whether raw text encodes, for a part, to exactly the expected text.
*/
static bool
d_tests_url_encodes_to(
    const char*         _raw,
    enum d_net_url_part _part,
    const char*         _expected
)
{
    size_t length = 0u;

    // the encoding
    char text[64] = { 0 };

    return ( (d_net_url_encode(d_tests_url_cstr(_raw),
                               _part,
                               text,
                               sizeof(text),
                               &length) == D_NET_URL_OK) &&
             (length == strlen(_expected))               &&
             (strcmp(text,
                     _expected) == 0) );
}

/*
d_tests_url_decodes_to
  Whether text decodes to exactly the expected bytes; with NULL expected,
whether it is refused as a malformed escape.
*/
static bool
d_tests_url_decodes_to(
    const char* _encoded,
    const char* _expected
)
{
    size_t length = 0u;

    // the decoding
    char text[64] = { 0 };

    const enum d_net_url_error found = d_net_url_decode(
                                           d_tests_url_cstr(_encoded),
                                           text,
                                           sizeof(text),
                                           &length);

    return (_expected) ? ( (found == D_NET_URL_OK) &&
                           (strcmp(text,
                                   _expected) == 0) )
                       : (found == D_NET_URL_ERROR_PERCENT);
}

/*
d_tests_url_sizes_exactly
  Whether sizing calls, with no buffer, report the exact lengths.
*/
static bool
d_tests_url_sizes_exactly(void)
{
    size_t encoded = 0u;
    size_t decoded = 0u;

    return ( (d_net_url_encode(d_tests_url_cstr("a b/c?d#e%f"),
                               D_NET_URL_PART_QUERY,
                               NULL,
                               0u,
                               &encoded) == D_NET_URL_ERROR_BUFFER) &&
             (encoded == strlen("a%20b/c?d%23e%25f"))               &&
             (d_net_url_decode(d_tests_url_cstr("%41%42"),
                               NULL,
                               0u,
                               &decoded) == D_NET_URL_ERROR_BUFFER) &&
             (decoded == 2u) );
}

/*
d_tests_sa_net_url_percent
  Tests the following:
  - each part encodes exactly what it must, '%' included, in uppercase
  - decoding reverses encoding, and refuses a malformed escape
  - sizing calls report exact lengths
*/
bool
d_tests_sa_net_url_percent(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "percent-encoding";
    const char* const RAW    = "a b/c?d#e%f";
    bool              result = true;

    const bool encoded  = ( (d_tests_url_encodes_to(RAW,
                                                    D_NET_URL_PART_COMPONENT,
                                                    "a%20b%2Fc%3Fd%23e%25f")) &&
                            (d_tests_url_encodes_to(RAW,
                                                    D_NET_URL_PART_SEGMENT,
                                                    "a%20b%2Fc%3Fd%23e%25f")) &&
                            (d_tests_url_encodes_to(RAW,
                                                    D_NET_URL_PART_PATH,
                                                    "a%20b/c%3Fd%23e%25f"))   &&
                            (d_tests_url_encodes_to(RAW,
                                                    D_NET_URL_PART_QUERY,
                                                    "a%20b/c?d%23e%25f")) );
    const bool reversed = ( (d_tests_url_decodes_to("a%20b%2Fc%3Fd%23e%25f",
                                                    RAW))    &&
                            (d_tests_url_decodes_to("%41%42",
                                                    "AB"))   &&
                            (d_tests_url_decodes_to("%zz",
                                                    NULL))   &&
                            (d_tests_url_decodes_to("%4",
                                                    NULL)) );

    result = d_assert_standalone(encoded,
                                 TEST,
                                 "each part encodes what it must",
                                 _counter) && result;
    result = d_assert_standalone(reversed,
                                 TEST,
                                 "decoding reverses; bad escapes refused",
                                 _counter) && result;
    result = d_assert_standalone(d_tests_url_sizes_exactly(),
                                 TEST,
                                 "sizing calls report exact lengths",
                                 _counter) && result;

    return result;
}

/*
d_tests_url_reaches
  How many of ENDPOINTS reach exactly their host and port, over TCP.
*/
static size_t
d_tests_url_reaches(void)
{
    const size_t count   = sizeof(ENDPOINTS) / sizeof(ENDPOINTS[0]);
    size_t       reached = 0u;

    // each URL, parsed and taken to its endpoint
    for (size_t i = 0u; i < count; ++i)
    {
        struct d_net_url      url      = { .port = 0u };
        struct d_net_endpoint endpoint = { .port = 0u };

        // parsed, reached, and exactly as listed
        if ( (d_tests_url_parse_cstr(ENDPOINTS[i].url,
                                     &url,
                                     NULL) == D_NET_URL_OK)             &&
             (d_net_url_endpoint(&url,
                                 &endpoint) == D_NET_ERROR_NONE)       &&
             (strcmp(endpoint.host,
                     ENDPOINTS[i].host) == 0)                           &&
             (endpoint.port == ENDPOINTS[i].port)                       &&
             (endpoint.protocol == D_NET_PROTOCOL_TCP) )
        {
            reached += 1u;
        }
    }

    return reached;
}

/*
d_tests_url_unreachable
  How many of UNREACHABLE parse and yet have no endpoint.
*/
static size_t
d_tests_url_unreachable(void)
{
    const size_t count   = sizeof(UNREACHABLE) / sizeof(UNREACHABLE[0]);
    size_t       refused = 0u;

    // each URL, parsed and refused an endpoint
    for (size_t i = 0u; i < count; ++i)
    {
        struct d_net_url      url      = { .port = 0u };
        struct d_net_endpoint endpoint = { .port = 0u };

        // parsed, then refused
        if ( (d_tests_url_parse_cstr(UNREACHABLE[i],
                                     &url,
                                     NULL) == D_NET_URL_OK) &&
             (d_net_url_endpoint(&url,
                                 &endpoint) ==
              D_NET_ERROR_ADDRESS_INVALID) )
        {
            refused += 1u;
        }
    }

    return refused;
}

/*
d_tests_sa_net_url_endpoints
  Tests the following:
  - a URL's endpoint: its host, and its port or its scheme's default
  - zones and escaped names decode; brackets go
  - no authority, no port, IPvFuture, and non-ASCII names are refused
  - default ports and TLS schemes, without case
*/
bool
d_tests_sa_net_url_endpoints(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "d_net_url_endpoint";
    bool              result = true;

    const bool schemes = ( (d_net_url_default_port(
                                d_tests_url_cstr("HTTPS")) == 443u)   &&
                           (d_net_url_default_port(
                                d_tests_url_cstr("gopher")) == 0u)    &&
                           (d_net_url_scheme_is_tls(
                                d_tests_url_cstr("Wss")))             &&
                           (!d_net_url_scheme_is_tls(
                                d_tests_url_cstr("http"))) );

    result = d_assert_standalone(
                 d_tests_url_reaches() ==
                 sizeof(ENDPOINTS) / sizeof(ENDPOINTS[0]),
                 TEST,
                 "hosts and ports reach their endpoints",
                 _counter) && result;
    result = d_assert_standalone(
                 d_tests_url_unreachable() ==
                 sizeof(UNREACHABLE) / sizeof(UNREACHABLE[0]),
                 TEST,
                 "URLs naming no reachable host are refused",
                 _counter) && result;
    result = d_assert_standalone(schemes,
                                 TEST,
                                 "default ports and TLS, without case",
                                 _counter) && result;

    return result;
}
