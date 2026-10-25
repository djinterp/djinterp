/*******************************************************************************
* djinterp [net]                                                   tls_tests.hpp
*
* Tests of the TLS layer, net/tls/tls.hpp.
*   Each test runs a real handshake over loopback TCP, against a certificate
* the suite carries, in the SSL kernel's OpenSSL-family engine; without the
* engine the suite says it skipped. It is empty without a 64-bit integer
* type, as tls.hpp is.
*
*
* path:      /tests/djinterp/net/tls/tls_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/

#ifndef DJINTERP_NET_TLS_TLS_TESTS_HPP
#define DJINTERP_NET_TLS_TLS_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"     // framework root
#include "../../../../inc/djinterp/net/tls/tls.hpp"  // the layer under test


#if (D_INTERNAL_NET_HAS_INT64 == 1)

NS_DJINTERP
NS_TESTING

// net/tls/tls.hpp tests -- each returns true when every check held;
// tests_tls_engine says whether there is an engine to run them in
bool tests_tls_engine();
bool tests_tls_context();
bool tests_tls_prepare();
bool tests_tls_settings();
bool tests_tls_naming();
bool tests_tls_remaking();
bool tests_tls_identity();
bool tests_tls_adopt();
bool tests_tls_loopback();
bool tests_tls_hostname();
bool tests_tls_untrusted();
bool tests_tls_verify_off();
bool tests_tls_versions();
bool tests_tls_closing();
bool tests_tls_truncation();
bool tests_tls_outlives();
bool tests_tls_alpn();
bool tests_tls_upgrade();
#if (D_MOVE_ENABLED == 1)
bool tests_tls_moves();
bool tests_tls_moving();
#endif  // D_MOVE_ENABLED
bool tests_tls_run_all();

NS_END  // testing
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64


#endif  // DJINTERP_NET_TLS_TLS_TESTS_HPP
