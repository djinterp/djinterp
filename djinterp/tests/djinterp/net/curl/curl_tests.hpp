/*******************************************************************************
* djinterp [net]                                                 curl_tests.hpp
*
* Tests of the libcurl binding's C++ face, net/curl/curl.hpp, over a loopback
* HTTP server.
*
*
* path:      /tests/djinterp/net/curl/curl_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.12
*******************************************************************************/

#ifndef DJINTERP_NET_CURL_CURL_TESTS_HPP
#define DJINTERP_NET_CURL_CURL_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"       // framework root
#include "../../../../inc/djinterp/net/curl/curl.hpp"  // the layer under test


NS_DJINTERP
NS_TESTING

// net/curl/curl.hpp tests -- each returns true when every check held
bool tests_curl_library();
bool tests_curl_perform();
bool tests_curl_stream();
bool tests_curl_throwing_sink();
bool tests_curl_errors();
bool tests_curl_errors_to_net();
bool tests_curl_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_CURL_CURL_TESTS_HPP
