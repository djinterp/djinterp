/*******************************************************************************
* djinterp [net]                                                         tcp.hpp
*
* Forwards to net/tcp/tcp.hpp, the TCP transport's C++ layer.
*   The C++ tcp.hpp that stood here was rebuilt over tcp.h and moved into the
* per-protocol directory; its last content is kept in _retired/relay94_net_tcp.
* This header only keeps "./tcp.hpp" includes compiling until relocate_net.py
* points them at the new path and removes it.
*
*
* path:      /inc/djinterp/net/tcp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.09.28
*******************************************************************************/

#ifndef DJINTERP_NET_TCP_HPP
#define DJINTERP_NET_TCP_HPP 1

// djinterp
#include "./tcp/tcp.hpp"  // the TCP transport's C++ layer


#endif  // DJINTERP_NET_TCP_HPP
