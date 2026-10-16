/*******************************************************************************
* djinterp [tests]                               test_deadlock_tests_factory.cpp
*
*   Section TU: make_deadlock_watchdog.
*
*   Gated on D_HD_FACTORY_RETURNABLE (default 0): the factory returns a named
* local deadlock_watchdog, whose copy constructor is deleted and which
* declares no move constructor, so the return is ill-formed on instantiation.
* Note that a defaulted move would ALSO be deleted here, because the class
* holds a std::mutex and a std::condition_variable; the practical fix is to
* rewrite the factory body to return a prvalue.  See the suite header banner.
*
*
* path:      /tests/djinterp/test/sync/test_deadlock_tests_factory.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_deadlock_tests.hpp"

#if D_HD_FACTORY_RETURNABLE

// std
#include <chrono>
#include <string>


NS_DJINTERP
NS_TESTING


bool tests_dfactory_make_deadlock_watchdog()
{
    dt::deadlock_watchdog wd =
        dt::make_deadlock_watchdog(std::chrono::milliseconds(400));

    // configured but not yet armed
    D_HD_CHECK(!wd.armed());
    D_HD_CHECK(!wd.fired());
    D_HD_CHECK(wd.report().configured_timeout ==
               std::chrono::nanoseconds(
                   std::chrono::milliseconds(400)));

    return true;
}

bool tests_dfactory_make_with_description()
{
    dt::deadlock_watchdog wd =
        dt::make_deadlock_watchdog(
            hd_fire_timeout(),
            std::string("factory built"));

    wd.arm();
    hd_await_fire();
    wd.disarm();

    D_HD_CHECK(wd.fired());
    D_HD_CHECK(wd.report().event_count() == 1u);
    D_HD_CHECK(wd.report().events[0].description == "factory built");

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_HD_FACTORY_RETURNABLE
