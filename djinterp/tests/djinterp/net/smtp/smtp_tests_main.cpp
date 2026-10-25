/*******************************************************************************
* djinterp [net]                                            smtp_tests_main.cpp
*
* Runs the SMTP C++ facade's tests; without a 64-bit integer type there
* are none, and it says so.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./smtp_tests.hpp"  // the suite
// std
#include <csignal>   // std::signal, SIGPIPE
#include <cstdio>    // std::printf
// posix
#include <unistd.h>  // alarm


/*
main
  Arms the watchdog, ignores SIGPIPE for the sockets, and announces the
suite, which reports each test and the tally itself.
*/
int
main()
{
    (void)::alarm(60u);
    (void)std::signal(SIGPIPE,
                      SIG_IGN);
    std::printf("djinterp [net] smtp C++ facade tests\n");

#if (D_INTERNAL_NET_HAS_INT64 == 1)
    return (::djinterp::testing::tests_smtp_run_all()) ? 0 : 1;
#else
    std::printf("  no 64-bit integer type: nothing to test\n");

    return 0;
#endif  // D_INTERNAL_NET_HAS_INT64
}
