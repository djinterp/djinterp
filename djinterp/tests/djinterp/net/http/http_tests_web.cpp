/*******************************************************************************
* djinterp [net]                                              http_tests_web.cpp
*
* Checks that web.hpp's HTTP names are net/http/http.hpp's while HTTP moves
* out of web.hpp.
*   At compile time, each old name is the same type as the new one; at run
* time, web.hpp's helpers behave as before: to_string's names, "" for an
* extension method, method_from_string still ignoring case, the old classes
* and reason phrases, and the constant namespaces. C++17 and later, as
* web.hpp is.
*
*
* path:      /tests/djinterp/net/http/http_tests_web.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/web.hpp"  // the aliases under test
// std
#include <cstdio>       // std::printf
#include <cstring>      // std::strcmp
#include <type_traits>  // std::is_same


namespace web = ::djinterp::web;
namespace net = ::djinterp::net;

static_assert(std::is_same<web::http_method, net::http_method>::value,
              "web::http_method must be net::http_method");
static_assert(std::is_same<web::http_version, net::http_version>::value,
              "web::http_version must be net::http_version");
static_assert(std::is_same<web::http_status, net::http_status>::value,
              "web::http_status must be net::http_status");
static_assert(std::is_same<web::status_category,
                           net::http_status_class>::value,
              "web::status_category must be net::http_status_class");

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
main
  Each group of checks, reported, then the tally.
*/
int
main()
{
    web::http_method method = web::http_method::get;

    const bool names    = ( (same(web::to_string(web::http_method::delete_),
                                  "DELETE"))                                &&
                            (same(web::to_string(web::http_method::other),
                                  ""))                                      &&
                            (same(web::to_string(web::http_version::http_1_1),
                                  "HTTP/1.1")) );
    const bool lenient  = ( (web::method_from_string("post",
                                                     method))       &&
                            (method == web::http_method::post)      &&
                            (!web::method_from_string("PROPFIND",
                                                      method)) );
    const bool statuses = ( (web::classify(404) ==
                             web::status_category::client_error)          &&
                            (web::classify(200) ==
                             web::status_category::success)               &&
                            (same(web::reason_phrase(web::http_status::ok),
                                  "OK"))                                  &&
                            (same(web::reason_phrase(404),
                                  "Not Found")) );
    const bool tables   = ( (same(web::header_name::accept,
                                  "Accept"))                          &&
                            (same(web::header_name::content_type,
                                  "Content-Type"))                    &&
                            (same(web::content_type::json,
                                  "application/json")) );
    const int  passed   = (names ? 1 : 0) +
                          (lenient ? 1 : 0) +
                          (statuses ? 1 : 0) +
                          (tables ? 1 : 0);

    std::printf("djinterp [net] web.hpp's HTTP aliases\n");
    std::printf("  %s  names\n  %s  lenient method names\n"
                "  %s  classes and phrases\n  %s  constant tables\n",
                (names) ? "pass" : "FAIL",
                (lenient) ? "pass" : "FAIL",
                (statuses) ? "pass" : "FAIL",
                (tables) ? "pass" : "FAIL");
    std::printf("%d/4 checks\n",
                passed);

    return (passed == 4) ? 0 : 1;
}
