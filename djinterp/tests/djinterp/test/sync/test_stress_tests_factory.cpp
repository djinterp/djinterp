/*******************************************************************************
* djinterp [tests]                                 test_stress_tests_factory.cpp
*
*   Section TU: make_stress / make_timed_stress.
*
*   The whole body is gated on D_HS_FACTORY_RETURNABLE, which defaults to 0
* because stress_runner and timed_stress currently delete their copy
* constructors without declaring move constructors - so `return runner;` /
* `return stress;` inside the factories selects a deleted constructor and the
* templates are ill-formed on instantiation.  See the suite header banner for
* the fix; with it applied, build with -DD_HS_FACTORY_RETURNABLE=1 (the
* matching declarations and spec rows are gated on the same macro).
*
*
* path:      /tests/djinterp/test/sync/test_stress_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_stress_tests.hpp"

#if D_HS_FACTORY_RETURNABLE

// std
#include <atomic>
#include <chrono>


NS_DJINTERP
NS_TESTING


bool tests_sfactory_make_stress()
{
    hs_tick sink;

    dt::stress_runner runner =
        dt::make_stress(3, 25, hs_bump_op(sink));

    D_HS_CHECK(runner.thread_count() == 3u);
    D_HS_CHECK(runner.iterations() == 25u);
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 0);   // not run yet

    return true;
}

bool tests_sfactory_make_stress_runs()
{
    hs_tick sink;

    dt::stress_runner runner =
        dt::make_stress(2, 30, hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.success());
    D_HS_CHECK(rep.total_ops == 60u);                             // 2 x 30
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 60);

    return true;
}

bool tests_sfactory_make_timed_stress()
{
    hs_tick sink;

    dt::timed_stress stress =
        dt::make_timed_stress(2, hs_timed_span(), hs_bump_op(sink));

    const dt::stress_report rep = stress.run();

    D_HS_CHECK(rep.success());
    D_HS_CHECK(rep.total_ops > 0u);
    D_HS_CHECK(rep.ops_per_thread.size() == 2u);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_HS_FACTORY_RETURNABLE
