/*******************************************************************************
* djinterp [cpp][container]                         node_probes_tests_runner.cpp
*
*   The runner for node_probes.hpp's suite. The suite is compile time: every
* answer is a static_assert in node_probes_tests_answers.cpp, so a build at a
* level is a pass at that level. The runner reports how many answers were
* checked, and at which level.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/node_probes/node_probes_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
// std
#include <cstdio>  // std::printf
// djinterp
#include "node_probes_tests.hpp"  // via INCLUDES (tests dir)


/*
main
  The runner: counts the answers asserted, and prints them with the level.
*/
int
main()
{
    const int answers = ::djinterp::testing::node_probes_tests_answers();

    std::printf("node_probes.hpp unit tests: %d answers asserted at compile "
                "time, __cplusplus %ld\n",
                answers,
                static_cast<long>(__cplusplus));

    return (answers > 0) ? 0 : 1;
}
