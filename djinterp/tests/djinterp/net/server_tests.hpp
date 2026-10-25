/*******************************************************************************
* djinterp [net]                                               server_tests.hpp
*
* Tests of net/server.hpp: the HTTP/1.1 server, over loopback.
*
*
* path:      /tests/djinterp/net/server_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.14
*******************************************************************************/

#ifndef DJINTERP_NET_SERVER_TESTS_HPP
#define DJINTERP_NET_SERVER_TESTS_HPP 1

// djinterp
#include "../../../inc/djinterp/djinterp.hpp"     // framework root
#include "../../../inc/djinterp/net/net_common.h"  // D_INTERNAL_NET_HAS_INT64
#include "../../../inc/djinterp/net/server.hpp"   // the layer under test


NS_DJINTERP
NS_TESTING

// net/server.hpp tests -- each returns true when every check held
bool tests_server_requests();
bool tests_server_refusals();
bool tests_server_concurrency();
#if (D_INTERNAL_NET_HAS_INT64 == 1)
bool tests_server_https();
#endif  // D_INTERNAL_NET_HAS_INT64
bool tests_server_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_SERVER_TESTS_HPP
