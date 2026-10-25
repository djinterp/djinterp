/*******************************************************************************
* djinterp [net]                                          claude_tests_main.cpp
*
* Runs the Claude clients' tests; without libcurl there are none to run.
*
*
* path:      /tests/djinterp/net/api/claude_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./claude_tests.hpp"  // the suite
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
    std::printf("djinterp [net] claude client tests\n");

    // a build without libcurl makes no calls
    if (!::djinterp::net::curl::available())
    {
        std::printf("  no libcurl: nothing to test\n");

        return 0;
    }

    return (::djinterp::testing::tests_claude_run_all()) ? 0 : 1;
}
