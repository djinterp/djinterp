/*******************************************************************************
* djinterp [net]                                            curl_tests_main.cpp
*
* Runs the libcurl binding's C++ tests.
*
*
* path:      /tests/djinterp/net/curl/curl_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./curl_tests.hpp"  // the suite
// std
#include <csignal>   // std::signal, SIGPIPE
#include <cstdio>    // std::printf
// posix
#include <unistd.h>  // alarm


/*
main
  Arms the watchdog, ignores SIGPIPE for the sockets, and announces the
suite, which reports each test and the tally itself; without libcurl there
is nothing to test.
*/
int
main()
{
    (void)::alarm(60u);
    (void)std::signal(SIGPIPE,
                      SIG_IGN);
    std::printf("djinterp [net] curl C++ layer tests\n");

    // a build without libcurl has no transfers to test
    if (!::djinterp::net::curl::available())
    {
        std::printf("  no libcurl: nothing to test\n");

        return 0;
    }

    return (::djinterp::testing::tests_curl_run_all()) ? 0 : 1;
}
