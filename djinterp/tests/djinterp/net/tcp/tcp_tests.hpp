/*******************************************************************************
* djinterp [net]                                                   tcp_tests.hpp
*
* Tests of the TCP transport's C++ layer.
*   socket_connection, tcp_connector, and tcp_acceptor over real loopback
* sockets, net.hpp's stream templates over them, and at compile time the
* shapes the consumers rely on: connect and accept return open_result, the
* native handle is an int under BSD sockets, and the moves are noexcept.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

#ifndef DJINTERP_NET_TCP_TCP_TESTS_HPP
#define DJINTERP_NET_TCP_TCP_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"     // framework root
#include "../../../../inc/djinterp/net/tcp/tcp.hpp"  // the layer under test


NS_DJINTERP
NS_TESTING

// net/tcp/tcp.hpp tests -- each returns true when every check held
bool tests_tcp_options();
bool tests_tcp_loopback();
bool tests_tcp_streams();
bool tests_tcp_moves();
bool tests_tcp_adopt();
bool tests_tcp_accept_wake();
bool tests_tcp_unix();
bool tests_tcp_errors();
bool tests_tcp_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_TCP_TCP_TESTS_HPP
