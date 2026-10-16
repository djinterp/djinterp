/*******************************************************************************
* djinterp [core]                                    observer_tests_observer.cpp
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests_observer.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : observer_tests_observer.cpp
//   Tier 2, observer (section 5): dynamic, connection-tracked, unbounded.
// Covers construction, connect + notify, operator+=, disconnect via a handle,
// disconnect_all, size_including_dead accounting, notify skipping dead slots,
// compact freeing dead slots while sparing live handles, reserve, RAII via
// scoped_connection, the memory-safety of a handle that outlives its observer
// (Finding B - the safe aspect only), a raw fn-ptr callable override, and
// multi-argument forwarding.

// std
#include <cstddef>
// djinterp
#include "observer_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // fn_ptr-compatible fixture for the callable-override test.
    int g_obs_sum = 0;
    void obs_bump(int _x) { g_obs_sum += _x; }
}


/*
tests_observer_default_state
  Verifies a default-constructed observer.
  Tests the following:
  - empty, count zero, unbounded capacity (0)
*/
bool
tests_observer_default_state()
{
    observer<void(int)> o;

    bool ok = true;

    ok = ok && (o.empty());
    ok = ok && (o.count() == 0);
    ok = ok && ((observer<void(int)>::capacity) == 0);

    return ok;
}

/*
tests_observer_connect_notify
  Verifies connect returns a live handle and notify invokes the slots.
  Tests the following:
  - connect yields a connected handle and grows the count
  - notify invokes every connected callable
*/
bool
tests_observer_connect_notify()
{
    observer<void(int)> o;
    int sum = 0;

    connection c = o.connect([&](int _x) { sum += _x; });

    bool ok = (c.connected() && o.count() == 1);

    o.notify(5);
    ok = ok && (sum == 5);

    return ok;
}

/*
tests_observer_operator_plus_equals
  Verifies operator+= is shorthand for connect.
  Tests the following:
  - += attaches a callable and returns a live handle
  - the attached callable runs on notify
*/
bool
tests_observer_operator_plus_equals()
{
    observer<void(int)> o;
    int sum = 0;

    connection c = (o += [&](int _x) { sum += _x; });

    bool ok = (c.connected() && o.count() == 1);

    o.notify(8);
    ok = ok && (sum == 8);

    return ok;
}

/*
tests_observer_disconnect_via_connection
  Verifies disconnecting through a handle drops that slot.
  Tests the following:
  - disconnecting one handle leaves the other live
  - notify then invokes only the surviving callable
*/
bool
tests_observer_disconnect_via_connection()
{
    observer<void(int)> o;
    int a = 0;
    int b = 0;

    connection ca = o.connect([&](int _x) { a += _x; });
    connection cb = o.connect([&](int _x) { b += _x; });

    ca.disconnect();

    bool ok = true;

    ok = ok && (!ca.connected() && cb.connected());
    ok = ok && (o.count() == 1);

    o.notify(3);
    ok = ok && (a == 0 && b == 3);

    return ok;
}

/*
tests_observer_disconnect_all
  Verifies disconnect_all detaches every slot and invalidates handles.
  Tests the following:
  - after disconnect_all the count is zero
  - previously live handles report disconnected
*/
bool
tests_observer_disconnect_all()
{
    observer<void(int)> o;

    connection ca = o.connect([](int) {});
    connection cb = o.connect([](int) {});

    o.disconnect_all();

    bool ok = true;

    ok = ok && (o.count() == 0);
    ok = ok && (!ca.connected() && !cb.connected());

    return ok;
}

/*
tests_observer_size_including_dead
  Verifies dead slots linger in storage until compaction.
  Tests the following:
  - disconnecting a handle lowers count but not size_including_dead
*/
bool
tests_observer_size_including_dead()
{
    observer<void(int)> o;

    connection ca = o.connect([](int) {});
    o.connect([](int) {});

    ca.disconnect();

    bool ok = true;

    ok = ok && (o.count() == 1);                 // one live
    ok = ok && (o.size_including_dead() == 2);   // dead slot retained

    return ok;
}

/*
tests_observer_notify_skips_dead
  Verifies notify does not invoke dead slots.
  Tests the following:
  - a disconnected slot's callable is not called on notify
*/
bool
tests_observer_notify_skips_dead()
{
    observer<void(int)> o;
    int a = 0;
    int b = 0;

    connection ca = o.connect([&](int _x) { a += _x; });
    o.connect([&](int _x) { b += _x; });

    ca.disconnect();
    o.notify(10);

    return (a == 0 && b == 10);
}

/*
tests_observer_compact_keeps_live
  Verifies compact frees dead slots but preserves live handles.
  Tests the following:
  - compact reclaims dead storage (size_including_dead drops to the live count)
  - a live handle remains connected and its callable still fires afterwards
*/
bool
tests_observer_compact_keeps_live()
{
    observer<void(int)> o;
    int live = 0;

    connection dead = o.connect([](int) {});
    connection kept = o.connect([&](int _x) { live += _x; });

    dead.disconnect();
    o.compact();

    bool ok = true;

    ok = ok && (o.count() == 1);
    ok = ok && (o.size_including_dead() == 1);   // dead slot reclaimed
    ok = ok && (kept.connected());               // live handle survives compaction

    o.notify(6);
    ok = ok && (live == 6);

    return ok;
}

/*
tests_observer_reserve
  Verifies reserve is a transparent pre-allocation.
  Tests the following:
  - reserve does not change the observed count
  - connections still work after reserving
*/
bool
tests_observer_reserve()
{
    observer<void(int)> o;
    o.reserve(16);

    bool ok = (o.count() == 0);

    int sum = 0;
    o.connect([&](int _x) { sum += _x; });
    o.notify(4);
    ok = ok && (o.count() == 1 && sum == 4);

    return ok;
}

/*
tests_observer_scoped_raii
  Verifies scoped_connection drives observer detachment on scope exit.
  Tests the following:
  - a scoped slot is live in scope and detached after it
  - a separately-held slot is unaffected
*/
bool
tests_observer_scoped_raii()
{
    observer<void(int)> o;
    connection kept;

    {
        scoped_connection sc = o.connect([](int) {});
        kept = o.connect([](int) {});
        if (o.count() != 2) { return false; }
    }   // sc detaches its slot

    bool ok = true;

    ok = ok && (o.count() == 1);
    ok = ok && (kept.connected());

    return ok;
}

/*
tests_observer_connection_outlives_safe
  Verifies a handle that outlives its observer stays memory-safe (Finding B).
  The header claims destruction "invalidates all handles"; in practice the
  handle remains queryable and disconnectable without dangling. This test pins
  only the safe, defined behaviour - not the post-destruction connected() value.
  Tests the following:
  - querying and disconnecting a handle after its observer is gone does not crash
  - disconnect remains idempotent
*/
bool
tests_observer_connection_outlives_safe()
{
    connection c;

    {
        observer<void(int)> o;
        c = o.connect([](int) {});
    }   // observer destroyed; c's shared alive-flag keeps it non-dangling

    // both operations must be safe no-throw no-ops (value intentionally unpinned)
    volatile bool q = c.connected();
    (void)q;
    c.disconnect();
    c.disconnect();                 // idempotent

    return (!c.connected());        // after an explicit disconnect it is false
}

/*
tests_observer_fn_ptr_override
  Verifies an observer parameterised on a raw function pointer callable.
  Tests the following:
  - a fn-ptr observer connects and notifies like the std::function default
*/
bool
tests_observer_fn_ptr_override()
{
    observer<void(int), void(*)(int)> o;

    o.connect(&obs_bump);
    o.connect(&obs_bump);

    g_obs_sum = 0;
    o.notify(5);

    return (o.count() == 2 && g_obs_sum == 10);
}

/*
tests_observer_multi_arg
  Verifies multi-argument forwarding.
  Tests the following:
  - a two-argument signature forwards both arguments to every live slot
*/
bool
tests_observer_multi_arg()
{
    observer<void(int, int)> o;
    int sum = 0;

    o.connect([&](int _a, int _b) { sum += _a + _b; });
    o.connect([&](int _a, int _b) { sum += _a * _b; });

    o.notify(3, 4);      // (3+4) + (3*4) = 19

    return (sum == 19);
}


NS_END  // testing
NS_END  // djinterp
