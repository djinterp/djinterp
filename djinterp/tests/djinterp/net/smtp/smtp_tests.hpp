/*******************************************************************************
* djinterp [net]                                                  smtp_tests.hpp
*
* Tests of the SMTP C++ facade, net/smtp/smtp.hpp, over a loopback server --
* in plaintext, by STARTTLS and in TLS from the first byte -- and over a
* scripted net::connection. It is empty without a 64-bit integer type, as
* smtp.hpp is.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/

#ifndef DJINTERP_NET_SMTP_SMTP_TESTS_HPP
#define DJINTERP_NET_SMTP_SMTP_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"       // framework root
#include "../../../../inc/djinterp/net/smtp/smtp.hpp"  // the layer under test
#if (D_MOVE_ENABLED == 1)
    // std
    #include <type_traits>  // std::is_same
#endif  // D_MOVE_ENABLED


#if (D_INTERNAL_NET_HAS_INT64 == 1)

NS_DJINTERP
NS_TESTING

// net/smtp/smtp.hpp tests -- each returns true when every check held
bool tests_smtp_vocabulary();
bool tests_smtp_lifting();
bool tests_smtp_delivery();
bool tests_smtp_attach();
bool tests_smtp_script();
bool tests_smtp_send_once();
bool tests_smtp_listen();
bool tests_smtp_starttls();
bool tests_smtp_tls_paths();
#if (D_MOVE_ENABLED == 1)
bool tests_smtp_moves();
#endif  // D_MOVE_ENABLED
bool tests_smtp_run_all();

NS_END  // testing
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64


#endif  // DJINTERP_NET_SMTP_SMTP_TESTS_HPP
