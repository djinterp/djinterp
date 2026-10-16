/*******************************************************************************
* djinterp [core]                                    observer_tests_delegate.cpp
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests_delegate.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : observer_tests_delegate.cpp
//   Tier 0, delegate (section 3): a single callable slot defaulting to a raw
// function pointer. Covers construction, connect/replace, disconnect, the void
// and non-void notify paths (including the R{} fallback when unconnected),
// operator(), get, capacity, a std::function override for captures, multi-arg
// forwarding, and the observer_ptr alias (section 7).

// std
#include <cstddef>
#include <functional>
#include <type_traits>
// djinterp
#include "observer_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // fn_ptr-compatible fixtures (the delegate default stores a raw fn ptr).
    int g_sum = 0;

    void bump(int _x)          { g_sum += _x; }
    void bump_twice(int _x)    { g_sum += 2 * _x; }
    int  doubler(int _x)       { return 2 * _x; }
    void add_pair(int _a, int _b) { g_sum += _a + _b; }
}


/*
tests_delegate_default_state
  Verifies a default-constructed delegate.
  Tests the following:
  - unconnected on construction, count zero
*/
bool
tests_delegate_default_state()
{
    delegate<void(int)> d;

    return (!d.connected() && d.count() == 0);
}

/*
tests_delegate_connect_replace
  Verifies connect attaches and replaces the callable.
  Tests the following:
  - connect makes the delegate connected with count one
  - a second connect replaces the callable (still count one)
*/
bool
tests_delegate_connect_replace()
{
    delegate<void(int)> d;

    d.connect(&bump);
    bool ok = (d.connected() && d.count() == 1);

    g_sum = 0;
    d.connect(&bump_twice);      // replaces
    d.notify(5);
    ok = ok && (d.count() == 1);
    ok = ok && (g_sum == 10);    // bump_twice ran, not bump

    return ok;
}

/*
tests_delegate_disconnect
  Verifies disconnect clears the callable.
  Tests the following:
  - disconnect makes the delegate unconnected with count zero
*/
bool
tests_delegate_disconnect()
{
    delegate<void(int)> d(&bump);

    d.disconnect();

    return (!d.connected() && d.count() == 0);
}

/*
tests_delegate_disconnect_all
  Verifies disconnect_all is an alias for disconnect.
  Tests the following:
  - disconnect_all clears the callable
*/
bool
tests_delegate_disconnect_all()
{
    delegate<void(int)> d(&bump);

    d.disconnect_all();

    return (!d.connected());
}

/*
tests_delegate_notify_void
  Verifies the void notify path.
  Tests the following:
  - notify invokes the callable with the argument
  - notify on an unconnected delegate is a safe no-op
*/
bool
tests_delegate_notify_void()
{
    delegate<void(int)> d;

    g_sum = 0;
    d.notify(9);                 // unconnected: no-op
    bool ok = (g_sum == 0);

    d.connect(&bump);
    d.notify(9);
    ok = ok && (g_sum == 9);

    return ok;
}

/*
tests_delegate_notify_nonvoid
  Verifies the non-void notify path and its R{} fallback.
  Tests the following:
  - an unconnected delegate returns a value-initialized result
  - a connected delegate returns the callable's result
*/
bool
tests_delegate_notify_nonvoid()
{
    delegate<int(int)> d;

    bool ok = (d.notify(21) == 0);   // R{} == 0 when unconnected

    d.connect(&doubler);
    ok = ok && (d.notify(21) == 42);

    return ok;
}

/*
tests_delegate_operator_call
  Verifies operator() aliases notify.
  Tests the following:
  - invoking the delegate via operator() runs the callable
*/
bool
tests_delegate_operator_call()
{
    delegate<void(int)> d(&bump);

    g_sum = 0;
    d(7);

    return (g_sum == 7);
}

/*
tests_delegate_get
  Verifies get exposes the stored callable.
  Tests the following:
  - get is null before connect and non-null after
*/
bool
tests_delegate_get()
{
    delegate<void(int)> d;

    bool ok = (d.get() == nullptr);

    d.connect(&bump);
    ok = ok && (d.get() != nullptr);

    return ok;
}

/*
tests_delegate_implicit_ctor
  Verifies construction directly from a callable.
  Tests the following:
  - a delegate built from a callable is connected
*/
bool
tests_delegate_implicit_ctor()
{
    delegate<void(int)> d(&bump);

    return (d.connected());
}

/*
tests_delegate_capacity
  Verifies the delegate's static capacity.
  Tests the following:
  - capacity is one
*/
bool
tests_delegate_capacity()
{
    static_assert(delegate<void(int)>::capacity == 1, "single slot");

    return (delegate<void(int)>::capacity == 1);
}

/*
tests_delegate_stdfunction_captures
  Verifies a std::function override supports capturing callables.
  Tests the following:
  - a capturing lambda stored via std::function is invoked on notify
*/
bool
tests_delegate_stdfunction_captures()
{
    int local = 0;

    delegate<void(int), std::function<void(int)> > d;
    d.connect([&](int _x) { local += _x; });
    d.notify(4);
    d(6);

    return (local == 10);
}

/*
tests_delegate_multi_arg
  Verifies multi-argument forwarding.
  Tests the following:
  - a two-argument signature forwards both arguments to the callable
*/
bool
tests_delegate_multi_arg()
{
    delegate<void(int, int)> d(&add_pair);

    g_sum = 0;
    d.notify(3, 4);

    return (g_sum == 7);
}

/*
tests_observer_ptr_alias
  Verifies the observer_ptr convenience alias (section 7).
  Tests the following:
  - observer_ptr<Sig> is exactly delegate<Sig>
  - it behaves as a single-slot delegate
*/
bool
tests_observer_ptr_alias()
{
    static_assert(std::is_same<observer_ptr<void(int)>, delegate<void(int)> >::value,
                  "observer_ptr is delegate");

    observer_ptr<void(int)> d(&bump);
    g_sum = 0;
    d.notify(5);

    return ( std::is_same<observer_ptr<void(int)>, delegate<void(int)> >::value &&
             g_sum == 5 );
}


NS_END  // testing
NS_END  // djinterp
