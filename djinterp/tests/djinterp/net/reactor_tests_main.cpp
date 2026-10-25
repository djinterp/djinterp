/*******************************************************************************
* djinterp [net]                                         reactor_tests_main.cpp
*
* Runs net/reactor.hpp's tests; without epoll there are none.
*
*
* path:      /tests/djinterp/net/reactor_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./reactor_tests.hpp"  // the suite
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
    std::printf("djinterp [net] reactor tests\n");

#if D_ENV_NET_HAS_EPOLL
    return (::djinterp::testing::tests_reactor_run_all()) ? 0 : 1;
#else
    std::printf("  no epoll: nothing to test\n");

    return 0;
#endif  // D_ENV_NET_HAS_EPOLL
}
