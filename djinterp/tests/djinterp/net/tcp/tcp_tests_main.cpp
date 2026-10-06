/*******************************************************************************
* djinterp [net]                                              tcp_tests_main.cpp
*
* Entry point for the TCP transport's C++ tests.
*   A watchdog alarm ends a run that hangs -- an accept that never woke --
* rather than letting it wait forever.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./tcp_tests.hpp"  // the suite
// std
#include <cstdio>    // std::printf
// posix
#include <unistd.h>  // alarm


/*
main
  Arms the watchdog and announces the suite, which reports each test and
the tally itself.
*/
int
main()
{
    (void)::alarm(60u);
    std::printf("djinterp [net] tcp C++ layer tests\n");

    return (::djinterp::testing::tests_tcp_run_all()) ? 0 : 1;
}
