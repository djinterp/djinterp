/*******************************************************************************
* djinterp [net]                                               claude_tests.hpp
*
* Tests of the Claude clients, net/api/claude.hpp and claude_api.hpp, against
* a loopback stand-in for the Messages and Message Batches APIs.
*
*
* path:      /tests/djinterp/net/api/claude_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef DJINTERP_NET_API_CLAUDE_TESTS_HPP
#define DJINTERP_NET_API_CLAUDE_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"          // framework root
#include "../../../../inc/djinterp/net/api/claude_api.hpp"  // under test


NS_DJINTERP
NS_TESTING

// net/api/claude.hpp and claude_api.hpp tests -- each returns true when every
// check held
bool tests_claude_request();
bool tests_claude_send();
bool tests_claude_stream();
bool tests_claude_batches();
bool tests_claude_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_API_CLAUDE_TESTS_HPP
