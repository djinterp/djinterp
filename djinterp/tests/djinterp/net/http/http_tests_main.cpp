/*******************************************************************************
* djinterp [net]                                             http_tests_main.cpp
*
* The runner for net/http/http.hpp's tests.
*   Announces the suite, which reports each test and the tally itself.
*
*
* path:      /tests/djinterp/net/http/http_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./http_tests.hpp"  // the suite
// std
#include <cstdio>  // std::printf


/*
main
  Nothing here blocks, so unlike tcp's runner there is no watchdog.
*/
int
main()
{
    std::printf("djinterp [net] http C++ layer tests\n");

    return (::djinterp::testing::tests_http_run_all()) ? 0 : 1;
}
