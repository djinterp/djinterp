/*******************************************************************************
* djinterp [net]                                          client_tests_main.cpp
*
* Runs net/client.hpp's tests.
*
*
* path:      /tests/djinterp/net/client_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./client_tests.hpp"  // the suite
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
    std::printf("djinterp [net] client.hpp tests\n");

    return (::djinterp::testing::tests_client_run_all()) ? 0 : 1;
}
