// djinterp [test] : mediator_tests_dispatch.cpp
//   The stateless dispatch policies (section VIII).  Each is a struct with a
// single static dispatch() over a handler range and an event.  The three
// differ in their gating: broadcast_policy delivers to all; targeted_policy
// applies a predicate to each HANDLER; filtered_policy applies a predicate to
// the EVENT and then delivers to all or none.

// std
#include <functional>
#include <vector>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // id_handler : a handler that carries an identity so a targeting predicate
    // can select it, and increments a caller-owned counter when invoked.
    struct id_handler
    {
        int  id;
        int* counter;

        void operator()(const ping&) const { ++(*counter); }
    };
}


/*
tests_broadcast_policy
  Verifies broadcast_policy delivers the event to every handler.
  Tests the following:
  - all handlers in the range are invoked exactly once
*/
bool
tests_broadcast_policy()
{
    int count = 0;

    std::vector<std::function<void(const ping&)>> handlers;
    handlers.push_back([&](const ping&) { ++count; });
    handlers.push_back([&](const ping&) { ++count; });
    handlers.push_back([&](const ping&) { ++count; });

    broadcast_policy::dispatch(handlers, ping{ 1 });

    return (count == 3);
}

/*
tests_targeted_policy
  Verifies targeted_policy delivers only to handlers matching the predicate.
  Tests the following:
  - handlers for which the predicate is true are invoked
  - handlers for which the predicate is false are skipped
*/
bool
tests_targeted_policy()
{
    int hit_0 = 0;
    int hit_1 = 0;

    std::vector<id_handler> handlers =
    {
        id_handler{ 0, &hit_0 },
        id_handler{ 1, &hit_1 },
        id_handler{ 1, &hit_1 }
    };

    auto pred = [](const id_handler& _h) { return (_h.id == 1); };

    targeted_policy::dispatch(handlers, ping{ 1 }, pred);

    bool ok = true;

    ok = ok && (hit_0 == 0);
    ok = ok && (hit_1 == 2);

    return ok;
}

/*
tests_filtered_policy
  Verifies filtered_policy delivers to all handlers iff the event passes.
  Tests the following:
  - a passing event reaches every handler
  - a failing event reaches no handler
*/
bool
tests_filtered_policy()
{
    int count = 0;

    std::vector<std::function<void(const ping&)>> handlers;
    handlers.push_back([&](const ping&) { ++count; });
    handlers.push_back([&](const ping&) { ++count; });

    auto passes = [](const ping& _p) { return (_p.n > 0); };

    filtered_policy::dispatch(handlers, ping{ 5 }, passes);   // passes -> all
    bool ok = (count == 2);

    count = 0;
    filtered_policy::dispatch(handlers, ping{ -1 }, passes);  // fails -> none
    ok = ok && (count == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
