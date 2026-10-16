/*******************************************************************************
* djinterp [core]                                       observer_tests_event.cpp
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests_event.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : observer_tests_event.cpp
//   Tier 1, event (section 4): a fixed-capacity inline array of callables keyed
// by slot id. Covers construction, connect (ascending ids, no_slot when full),
// disconnect and first-null reuse, disconnect_all, the occupancy predicates,
// notify ordering, direct at() access, compact defragmentation, operator(), a
// std::function override, and multi-argument forwarding.

// std
#include <cstddef>
#include <functional>
// djinterp
#include "observer_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    int g_sum = 0;

    void bump(int _x)          { g_sum += _x; }
    void add_pair(int _a, int _b) { g_sum += _a + _b; }
}


/*
tests_event_default_state
  Verifies a default-constructed event.
  Tests the following:
  - empty, not full, count zero, capacity as specified
*/
bool
tests_event_default_state()
{
    event<void(int), 3> e;

    bool ok = true;

    ok = ok && (e.empty());
    ok = ok && (!e.full());
    ok = ok && (e.count() == 0);
    ok = ok && ((event<void(int), 3>::capacity) == 3);

    return ok;
}

/*
tests_event_connect_slot_ids
  Verifies connect returns ascending slot ids.
  Tests the following:
  - the first three connects yield ids 0, 1, 2
  - count reflects the connections
*/
bool
tests_event_connect_slot_ids()
{
    event<void(int), 4> e;

    slot_id a = e.connect(&bump);
    slot_id b = e.connect(&bump);
    slot_id c = e.connect(&bump);

    bool ok = true;

    ok = ok && (a == 0 && b == 1 && c == 2);
    ok = ok && (e.count() == 3);

    return ok;
}

/*
tests_event_connect_full_no_slot
  Verifies connect returns no_slot when the event is full.
  Tests the following:
  - filling every slot makes full() true
  - a further connect returns no_slot and does not change the count
*/
bool
tests_event_connect_full_no_slot()
{
    event<void(int), 2> e;

    e.connect(&bump);
    e.connect(&bump);

    bool ok = (e.full());

    slot_id overflow = e.connect(&bump);
    ok = ok && (overflow == no_slot);
    ok = ok && (e.count() == 2);

    return ok;
}

/*
tests_event_disconnect_and_reuse
  Verifies disconnect frees a slot and connect reuses the first null.
  Tests the following:
  - disconnecting a middle slot drops the count and that slot's liveness
  - the next connect reuses exactly the freed slot
*/
bool
tests_event_disconnect_and_reuse()
{
    event<void(int), 3> e;

    e.connect(&bump);
    slot_id mid = e.connect(&bump);
    e.connect(&bump);

    e.disconnect(mid);

    bool ok = true;

    ok = ok && (e.count() == 2);
    ok = ok && (!e.slot_connected(mid));

    slot_id reused = e.connect(&bump);
    ok = ok && (reused == mid);          // first null slot reused
    ok = ok && (e.count() == 3);

    return ok;
}

/*
tests_event_disconnect_all
  Verifies disconnect_all nulls every slot.
  Tests the following:
  - after disconnect_all the event is empty
*/
bool
tests_event_disconnect_all()
{
    event<void(int), 3> e;

    e.connect(&bump);
    e.connect(&bump);

    e.disconnect_all();

    return (e.empty() && e.count() == 0);
}

/*
tests_event_slot_connected
  Verifies per-slot liveness reporting.
  Tests the following:
  - an occupied slot reports connected
  - an out-of-range slot id reports not connected
*/
bool
tests_event_slot_connected()
{
    event<void(int), 2> e;
    slot_id a = e.connect(&bump);

    bool ok = true;

    ok = ok && (e.slot_connected(a));
    ok = ok && (!e.slot_connected(1));    // unused slot
    ok = ok && (!e.slot_connected(99));   // out of range

    return ok;
}

/*
tests_event_full_empty
  Verifies the full / empty occupancy predicates.
  Tests the following:
  - empty is true only when the count is zero
  - full is true only when every slot is occupied
*/
bool
tests_event_full_empty()
{
    event<void(int), 2> e;

    bool ok = (e.empty() && !e.full());

    e.connect(&bump);
    ok = ok && (!e.empty() && !e.full());

    e.connect(&bump);
    ok = ok && (!e.empty() && e.full());

    return ok;
}

/*
tests_event_notify_order
  Verifies notify invokes live slots front to back.
  Tests the following:
  - every occupied slot is invoked with the argument
*/
bool
tests_event_notify_order()
{
    event<void(int), 3> e;
    e.connect(&bump);
    e.connect(&bump);
    e.connect(&bump);

    g_sum = 0;
    e.notify(4);          // 3 slots each add 4

    return (g_sum == 12);
}

/*
tests_event_at
  Verifies direct slot access via at().
  Tests the following:
  - at(id) exposes the stored callable at that slot
*/
bool
tests_event_at()
{
    event<void(int), 2> e;
    slot_id a = e.connect(&bump);

    bool ok = true;

    ok = ok && (e.at(a) != nullptr);   // occupied slot
    ok = ok && (e.at(1) == nullptr);   // empty slot

    return ok;
}

/*
tests_event_compact
  Verifies compact defragments live slots to the front.
  Tests the following:
  - after freeing interior slots, compact packs the survivors from slot 0
  - the live count is preserved
*/
bool
tests_event_compact()
{
    event<void(int), 5> e;
    e.connect(&bump);            // 0
    slot_id b = e.connect(&bump); // 1
    e.connect(&bump);            // 2
    slot_id d = e.connect(&bump); // 3

    e.disconnect(b);
    e.disconnect(d);             // holes at 1 and 3

    e.compact();

    bool ok = true;

    ok = ok && (e.count() == 2);
    ok = ok && (e.slot_connected(0));
    ok = ok && (e.slot_connected(1));    // survivors packed forward
    ok = ok && (!e.slot_connected(2));

    return ok;
}

/*
tests_event_operator_call
  Verifies operator() aliases notify.
  Tests the following:
  - invoking the event via operator() runs every live slot
*/
bool
tests_event_operator_call()
{
    event<void(int), 2> e;
    e.connect(&bump);
    e.connect(&bump);

    g_sum = 0;
    e(5);

    return (g_sum == 10);
}

/*
tests_event_stdfunction
  Verifies a std::function override supports capturing callables.
  Tests the following:
  - capturing lambdas stored in the slots are invoked on notify
*/
bool
tests_event_stdfunction()
{
    int local = 0;

    event<void(int), 3, std::function<void(int)> > e;
    e.connect([&](int _x) { local += _x; });
    e.connect([&](int _x) { local += 10 * _x; });
    e.notify(1);

    return (local == 11);
}

/*
tests_event_multi_arg
  Verifies multi-argument forwarding.
  Tests the following:
  - a two-argument signature forwards both arguments to every slot
*/
bool
tests_event_multi_arg()
{
    event<void(int, int), 2> e;
    e.connect(&add_pair);
    e.connect(&add_pair);

    g_sum = 0;
    e.notify(3, 4);      // 2 slots each add 7

    return (g_sum == 14);
}


NS_END  // testing
NS_END  // djinterp
