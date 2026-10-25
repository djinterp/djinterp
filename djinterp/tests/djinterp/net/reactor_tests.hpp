/*******************************************************************************
* djinterp [net]                                              reactor_tests.hpp
*
* Tests of net/reactor.hpp: the reactor, and an event_server echoing over
* loopback.
*
*
* path:      /tests/djinterp/net/reactor_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_NET_REACTOR_TESTS_HPP
#define DJINTERP_NET_REACTOR_TESTS_HPP 1

// djinterp
#include "../../../inc/djinterp/djinterp.hpp"     // framework root
#include "../../../inc/djinterp/net/reactor.hpp"  // the layer under test


#if D_ENV_NET_HAS_EPOLL

NS_DJINTERP
NS_TESTING

// net/reactor.hpp tests -- each returns true when every check held
bool tests_reactor_mask();
bool tests_reactor_dispatch();
bool tests_reactor_echo();
bool tests_reactor_run_all();

NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_NET_HAS_EPOLL


#endif  // DJINTERP_NET_REACTOR_TESTS_HPP
