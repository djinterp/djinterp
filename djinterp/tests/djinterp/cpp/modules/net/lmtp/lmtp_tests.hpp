/*******************************************************************************
* djinterp [cpp][net]                                             lmtp_tests.hpp
*
* Tests of the LMTP C++ facade, net/lmtp/lmtp.hpp: its options and names,
* its client and server together over loopback TCP and a unix-domain
* socket, its guards, a scripted server that hangs up partway through the
* final replies, and a real LMTP server where the environment names one.
* It is empty without a 64-bit integer type, as lmtp.hpp is.
*
*
* path:      /tests/djinterp/cpp/modules/net/lmtp/lmtp_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/

#ifndef DJINTERP_NET_LMTP_LMTP_TESTS_HPP
#define DJINTERP_NET_LMTP_LMTP_TESTS_HPP 1

// djinterp
#include "../../../../../../inc/djinterp/cpp/djinterp.hpp"               // framework root
#include "../../../../../../inc/djinterp/cpp/modules/net/lmtp/lmtp.hpp"  // the layer under test


#if (D_INTERNAL_NET_HAS_INT64 == 1)

NS_DJINTERP
NS_TESTING

// net/lmtp/lmtp.hpp tests - each returns true when every check held
bool tests_lmtp_vocabulary();
bool tests_lmtp_loopback();
bool tests_lmtp_local();
bool tests_lmtp_held();
bool tests_lmtp_hang_up();
#if (D_MOVE_ENABLED == 1)
bool tests_lmtp_moves();
#endif  // D_MOVE_ENABLED
bool tests_lmtp_live();
bool tests_lmtp_run_all();

NS_END  // testing
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64


#endif  // DJINTERP_NET_LMTP_LMTP_TESTS_HPP
