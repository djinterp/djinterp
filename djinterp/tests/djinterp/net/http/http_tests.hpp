/*******************************************************************************
* djinterp [net]                                                  http_tests.hpp
*
* Tests of net/http/http.hpp, HTTP's C++ layer.
*   Each test returns true when every check held; the suite reports each test
* and the tally.
*
*
* path:      /tests/djinterp/net/http/http_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

#ifndef DJINTERP_NET_HTTP_HTTP_TESTS_HPP
#define DJINTERP_NET_HTTP_HTTP_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"       // framework root
#include "../../../../inc/djinterp/net/http/http.hpp"  // the layer under test


NS_DJINTERP
NS_TESTING

// net/http/http.hpp tests -- each returns true when every check held
bool tests_http_vocabulary();
bool tests_http_methods();
bool tests_http_versions();
bool tests_http_status();
bool tests_http_syntax();
bool tests_http_names();
bool tests_http_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_HTTP_HTTP_TESTS_HPP
