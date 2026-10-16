/*******************************************************************************
* djinterp [net]                                             smtp_tests_main.cpp
*
* djinterp SMTP C++ facade tests -- runner.
*   Runs the facade suite and exits non-zero if any test failed. Link with
* src/djinterp/net/smtp.cpp, the four kernel sources, the C suite's
* smtp_tests_sa_common.c and smtp_tests_sa_session.c (for the certificate
* fixture), the standalone test runtime, -lssl -lcrypto and -pthread.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "./smtp_tests.hpp"
// std
#include <cstdio>  // std::printf


int
main()
{
    std::printf("djinterp [net/smtp] facade tests\n");

    return (::djinterp::testing::tests_smtp_run_all()) ? 0 : 1;
}
