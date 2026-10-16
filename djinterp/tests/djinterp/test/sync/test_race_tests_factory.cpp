/*******************************************************************************
* djinterp [tests]                                   test_race_tests_factory.cpp
*
*   Section TU: make_race_probe.
*
*   Gated on D_HR_FACTORY_RETURNABLE (default 0) because race_probe deletes
* its copy constructor without declaring a move constructor, so the factory's
* `return probe;` is ill-formed on instantiation.  See the suite header banner
* for the one-line fix, then build with -DD_HR_FACTORY_RETURNABLE=1.
*
*
* path:      /tests/djinterp/test/sync/test_race_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_race_tests.hpp"

#if D_HR_FACTORY_RETURNABLE

// std
#include <atomic>


NS_DJINTERP
NS_TESTING


bool tests_rfactory_make_race_probe()
{
    dt::race_probe probe =
        dt::make_race_probe(
            2, 5, 3,
            [](std::size_t, std::size_t) {},
            []() { return true; });

    // the configuration is observable through a run: 3 runs x 2 threads x 5
    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.runs_performed == 3u);
    D_HR_CHECK(rep.total_thread_iterations == 30u);

    return true;
}

bool tests_rfactory_make_and_run()
{
    std::atomic<long> calls(0);

    dt::race_probe probe =
        dt::make_race_probe(
            3, 4, 2,
            [&calls](std::size_t, std::size_t)
            {
                calls.fetch_add(1, std::memory_order_relaxed);
            },
            []() { return true; });

    const dt::race_probe_report rep = probe.run();

    D_HR_CHECK(rep.success());
    D_HR_CHECK(!rep.any_evidence);
    D_HR_CHECK(calls.load(std::memory_order_relaxed) == 24);   // 2 x 3 x 4

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_HR_FACTORY_RETURNABLE
