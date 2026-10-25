/*******************************************************************************
* djinterp [net]                                               client_tests.hpp
*
* Tests of net/client.hpp: the connector contract, connect_with_retry, and the
* client facade.
*
*
* path:      /tests/djinterp/net/client_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_NET_CLIENT_TESTS_HPP
#define DJINTERP_NET_CLIENT_TESTS_HPP 1

// djinterp
#include "../../../inc/djinterp/djinterp.hpp"     // framework root
#include "../../../inc/djinterp/net/client.hpp"   // the layer under test


NS_DJINTERP
NS_TESTING

// net/client.hpp tests -- each returns true when every check held
bool tests_client_retryable();
bool tests_client_retry();
bool tests_client_facade();
bool tests_client_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_CLIENT_TESTS_HPP
