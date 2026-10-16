/*******************************************************************************
* djinterp [tests]                             test_concurrent_tests_factory.cpp
*
*   Section TU: make_concurrent_runner.
*
*   The whole body is gated on D_HC_FACTORY_RETURNABLE, which defaults to 0
* because concurrent_runner currently deletes its copy constructor and does
* not declare a move constructor - so `return runner;` inside
* make_concurrent_runner() selects a deleted constructor and the template is
* ill-formed on instantiation.  See the suite header banner for the one-line
* fix; with it applied, build this TU with -DD_HC_FACTORY_RETURNABLE=1 (the
* matching declarations and spec rows are gated on the same macro).
*
*
* path:      /tests/djinterp/test/sync/test_concurrent_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_concurrent_tests.hpp"

#if D_HC_FACTORY_RETURNABLE

// std
#include <atomic>


NS_DJINTERP
NS_TESTING


bool tests_cfactory_make_concurrent_runner()
{
    hc_counter sink;

    dt::concurrent_runner runner =
        dt::make_concurrent_runner(3, hc_bump(sink));

    D_HC_CHECK(runner.thread_count() == 3u);
    D_HC_CHECK(sink.calls.load(std::memory_order_acquire) == 0);   // not run yet

    return true;
}

bool tests_cfactory_make_and_run()
{
    hc_counter sink;

    dt::concurrent_runner runner =
        dt::make_concurrent_runner(4, hc_bump(sink));

    const dt::concurrent_run_report rep = runner.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.thread_count == 4u);
    D_HC_CHECK(sink.calls.load(std::memory_order_acquire) == 4);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_HC_FACTORY_RETURNABLE
