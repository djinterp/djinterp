/*******************************************************************************
* djinterp [net]                                          http_tests_helpers.cpp
*
* Checks the string helpers http.hpp and net_url.hpp keep for HTTP and URLs:
* to_string's names, "" for an extension method, method_from_string ignoring
* case, every method and scheme token found by method_from_string and
* scheme_from_string, the status classes and reason phrases, and the constant
* namespaces. Header names match without regard to case, and split_url reads
* URLs as RFC 3986 does (register item 100). C++98 and later.
*
*
* path:      /tests/djinterp/net/http/http_tests_helpers.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/http/http.hpp"  // HTTP helpers
#include "../../../../inc/djinterp/net/net_url.hpp"    // URL helpers
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
#include <string>   // std::string


namespace net = ::djinterp::net;

/*
same
  Whether two C strings hold the same text.
*/
static bool
same(
    const char* _text,
    const char* _expected
)
{
    return (std::strcmp(_text,
                        _expected) == 0);
}

/*
every_method
  Whether each method's token, in any mix of case, yields that method.
*/
static bool
every_method()
{
    static const struct
    {
        const char*             token;
        net::http_method::value method;
    } cases[] =
    {
        { "GET",     net::http_method::get     },
        { "head",    net::http_method::head    },
        { "Post",    net::http_method::post    },
        { "pUT",     net::http_method::put     },
        { "delete",  net::http_method::delete_ },
        { "PATCH",   net::http_method::patch   },
        { "options", net::http_method::options },
        { "TRACE",   net::http_method::trace   },
        { "connect", net::http_method::connect }
    };

    // each must be found, and must name its own method
    for (std::size_t i = 0; i < (sizeof(cases) / sizeof(cases[0])); ++i)
    {
        net::http_method::value method = net::http_method::other;

        if ( (!net::method_from_string(cases[i].token,
                                       method)) ||
             (method != cases[i].method) )
        {
            return false;
        }
    }

    return true;
}

/*
every_scheme
  Whether each scheme's token, in any mix of case, yields that scheme, and
other tokens yield unknown.
*/
static bool
every_scheme()
{
    static const struct
    {
        const char*            token;
        net::url_scheme::value scheme;
    } cases[] =
    {
        { "http",   net::url_scheme::http    },
        { "HTTPS",  net::url_scheme::https   },
        { "ws",     net::url_scheme::ws      },
        { "WsS",    net::url_scheme::wss     },
        { "ftp",    net::url_scheme::ftp     },
        { "Ftps",   net::url_scheme::ftps    },
        { "FILE",   net::url_scheme::file    },
        { "data",   net::url_scheme::data    },
        { "gopher", net::url_scheme::unknown },
        { "",       net::url_scheme::unknown }
    };

    // each must name its own scheme
    for (std::size_t i = 0; i < (sizeof(cases) / sizeof(cases[0])); ++i)
    {
        if (net::scheme_from_string(cases[i].token) != cases[i].scheme)
        {
            return false;
        }
    }

    return true;
}

/*
header_lookup
  Whether header names match in any mix of ASCII case and only so, and
set_header replaces the first match in place or appends.
*/
static bool
header_lookup()
{
    net::header_list list;

    list.push_back(net::header_field("Content-Type",
                                     "text/plain"));
    list.push_back(net::header_field("x-api-key",
                                     "k"));
    net::set_header(list,
                    "CONTENT-TYPE",
                    "application/json");
    net::set_header(list,
                    "Accept",
                    "*/*");

    return ( (list.size() == 3u)                                       &&
             (list[0].first == "Content-Type")                         &&
             (net::header_value(list,
                                "content-type") == "application/json") &&
             (net::has_header(list,
                              "X-API-KEY"))                            &&
             (!net::has_header(list,
                               "Content_Type"))                        &&
             (net::find_header(list,
                               "accept") == &list[2]) );
}

/*
split_case
  A URL split_url accepts, and the parts it must give.
*/
struct split_case
{
    const char* url;
    const char* userinfo;
    const char* host;
    unsigned    port;
    const char* path;
};

/*
split_one
  Whether split_url accepts a case's URL and splits it into the case's parts.
*/
static bool
split_one(
    const split_case& _case
)
{
    net::url_parts parts;

    return ( (net::split_url(_case.url,
                             parts))                           &&
             (parts.userinfo == _case.userinfo)                &&
             (parts.host == _case.host)                        &&
             (static_cast<unsigned>(parts.port) == _case.port) &&
             (parts.path == _case.path) );
}

/*
split_urls
  Whether split_url reads IP literals and zones, userinfo, ports, queries
and fragments as RFC 3986 does, and refuses, leaving its output alone, a
URL that does not parse or has no authority.
*/
static bool
split_urls()
{
    static const split_case cases[] =
    {
        { "http://[::1]/",             "",        "::1",            0u,  "/"  },
        { "http://[fe80::1%25eth0]/x", "",        "fe80::1%25eth0", 0u,  "/x" },
        { "http://user:pw@h/",         "user:pw", "h",              0u,  "/"  },
        { "http://user:pw@h:81/",      "user:pw", "h",              81u, "/"  },
        { "http://h",                  "",        "h",              0u,  ""   }
    };
    static const char* const refused[] =
    {
        "http://h:18446744073709551617/",
        "http://h:4294967377/",
        "mailto:x@y.org",
        "/relative",
        "//h/network-path",
        "http://[::1/"
    };
    net::url_parts parts;
    bool           all = true;

    // each URL splits into its parts
    for (std::size_t i = 0; i < (sizeof(cases) / sizeof(cases[0])); ++i)
    {
        all = (split_one(cases[i])) && all;
    }

    // the scheme, query and fragment, still as written
    all = ( (net::split_url("HTTPS://Ex.com:8443/a/b?q=1#f",
                            parts))   &&
            (parts.scheme == "HTTPS") &&
            (parts.query == "q=1")    &&
            (parts.fragment == "f") ) && all;
    parts.host = "untouched";

    // what does not parse is refused, and the parts are left alone
    for (std::size_t i = 0; i < (sizeof(refused) / sizeof(refused[0])); ++i)
    {
        all = (!net::split_url(refused[i],
                               parts)) && all;
    }

    return ( (all) &&
             (parts.host == "untouched") );
}

/*
main
  Each group of checks, reported, then the tally.
*/
int
main()
{
    net::http_method::value method = net::http_method::get;

    const bool names    = ( (same(net::to_string(net::http_method::delete_),
                                  "DELETE")) &&
                            (same(net::to_string(net::http_method::other),
                                  ""))       &&
                            (same(net::to_string(net::http_version::http_1_1),
                                  "HTTP/1.1")) );
    const bool lenient  = ( (net::method_from_string("post",
                                                     method))  &&
                            (method == net::http_method::post) &&
                            (!net::method_from_string("PROPFIND",
                                                      method)) );
    const bool statuses = ( (net::classify(404) ==
                             net::http_status_class::client_error) &&
                            (net::classify(200) ==
                             net::http_status_class::successful)   &&
                            (same(net::reason_phrase(net::http_status::ok),
                                  "OK"))                           &&
                            (same(net::reason_phrase(404),
                                  "Not Found")) );
    const bool tables   = ( (same(net::http_field::accept,
                                  "Accept"))       &&
                            (same(net::http_field::content_type,
                                  "Content-Type")) &&
                            (same(net::http_media::json,
                                  "application/json")) );
    const bool tokens   = ( (every_method()) &&
                            (every_scheme()) );
    const bool headers  = header_lookup();
    const bool urls     = split_urls();
    const int  passed   = (names ? 1 : 0)    +
                          (lenient ? 1 : 0)  +
                          (tokens ? 1 : 0)   +
                          (statuses ? 1 : 0) +
                          (tables ? 1 : 0)   +
                          (headers ? 1 : 0)  +
                          (urls ? 1 : 0);

    std::printf("djinterp [net] HTTP and URL helpers\n");
    std::printf("  %s  names\n  %s  lenient method names\n"
                "  %s  every method and scheme token\n"
                "  %s  classes and phrases\n  %s  constant tables\n"
                "  %s  header lookup\n  %s  URL splitting\n",
                (names) ? "pass" : "FAIL",
                (lenient) ? "pass" : "FAIL",
                (tokens) ? "pass" : "FAIL",
                (statuses) ? "pass" : "FAIL",
                (tables) ? "pass" : "FAIL",
                (headers) ? "pass" : "FAIL",
                (urls) ? "pass" : "FAIL");
    std::printf("%d/7 checks\n",
                passed);

    return (passed == 7) ? 0 : 1;
}
