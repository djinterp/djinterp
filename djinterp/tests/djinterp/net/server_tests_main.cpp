/*******************************************************************************
* djinterp [net]                                          server_tests_main.cpp
*
* Runs net/server.hpp's tests.
*
*
* path:      /tests/djinterp/net/server_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.05
*******************************************************************************/
#include "./server_tests.hpp"  // the suite
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
    std::printf("djinterp [net] server.hpp tests\n");

    return (::djinterp::testing::tests_server_run_all()) ? 0 : 1;
}
