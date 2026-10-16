/*******************************************************************************
* djinterp [net]                                                  smtp_tests.hpp
*
* djinterp SMTP C++ facade tests.
*   Declarations for the tests of net/smtp.hpp: lifting the kernel's
* vocabulary into owning C++ values; a client session attached to a scripted
* net::connection; and a loopback session between the facade's own client
* and server over STARTTLS with authentication, plus a refused certificate.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

#ifndef DJINTERP_NET_SMTP_SMTP_TESTS_HPP
#define DJINTERP_NET_SMTP_SMTP_TESTS_HPP 1

// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/net/smtp.hpp"  // the facade under test


NS_DJINTERP
NS_TESTING

// net/smtp.hpp tests -- each returns true when every check held
bool tests_smtp_vocabulary();
bool tests_smtp_attach();
bool tests_smtp_loopback();
bool tests_smtp_run_all();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_NET_SMTP_SMTP_TESTS_HPP
