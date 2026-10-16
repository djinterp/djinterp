/*******************************************************************************
* djinterp [test]                                       restd_any_tests_main.cpp
*
* re_std::any test runner main:
*   Walks the registered test-case linked list and reports results.
*
*   USAGE:
*     ./restd_any_tests              - run all tests
*     ./restd_any_tests <suite-name> - run only the named suite
*
* Available suites:
*     construction, observers, retrieval, modifiers, any_cast,
*     make_any, constexpr_tier, std_compat, edge
*
*
* path:      /tests/djinterp/re_std/any/restd_any_tests_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "restd_any_tests.hpp"


int
main(
    int    _argc,
    char** _argv
)
{
    if (_argc > 1)
    {
        return restd_any_tests::run_suite(_argv[1]);
    }

    return restd_any_tests::run_all();
}
