/*******************************************************************************
* djinterp [net]                                              net_tests_main.cpp
*
* Entry point for the net foundation C++ tests.
*   Runs the suite and reports its tally. Exits 0 only if every test passed.
*
*
* path:      /tests/djinterp/net/net_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "./net_tests.hpp"  // the suite
// std
#include <cstdio>  // std::printf


/*
main
  Announces the suite, which reports each test and the tally itself.
*/
int
main()
{
    std::printf("djinterp [net] C++ layer tests\n");

    return (::djinterp::testing::tests_net_run_all()) ? 0 : 1;
}
