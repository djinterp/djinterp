/*******************************************************************************
* djinterp [core]                                  observer_tests_connection.cpp
*
*
* path:      /tests/djinterp/core/paradigm/observer/observer_tests_connection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : observer_tests_connection.cpp
//   Connection handles (section 2): the plain connection (a shared alive flag),
// the RAII scoped_connection (move-only, disconnects on destruction, with
// release), and scoped_connections (bulk RAII). Connections are minted directly
// from an observer so the alive flag is real.

// std
#include <cstddef>
// djinterp
#include "observer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_connection_default
  Verifies a default-constructed connection.
  Tests the following:
  - a default connection reports not connected
*/
bool
tests_connection_default()
{
    connection c;

    return (!c.connected());
}

/*
tests_connection_disconnect
  Verifies connection::disconnect flips the live flag.
  Tests the following:
  - a fresh connection from an observer is connected
  - disconnect makes it report not connected
*/
bool
tests_connection_disconnect()
{
    observer<void()> o;
    connection c = o.connect([]() {});

    bool ok = (c.connected());

    c.disconnect();
    ok = ok && (!c.connected());

    return ok;
}

/*
tests_connection_reset
  Verifies connection::reset releases the handle without disconnecting.
  Tests the following:
  - after reset the handle reports not connected
  - the underlying slot stays live (the observer still counts it)
*/
bool
tests_connection_reset()
{
    observer<void()> o;
    connection c = o.connect([]() {});

    c.reset();

    bool ok = true;

    ok = ok && (!c.connected());   // handle no longer controls the slot
    ok = ok && (o.count() == 1);   // slot itself remains live

    return ok;
}

/*
tests_scoped_connection_raii
  Verifies scoped_connection disconnects on destruction.
  Tests the following:
  - the slot is live while the guard is in scope
  - destroying the guard disconnects the slot
*/
bool
tests_scoped_connection_raii()
{
    observer<void()> o;

    {
        scoped_connection sc = o.connect([]() {});
        if (o.count() != 1) { return false; }
    }

    return (o.count() == 0);
}

/*
tests_scoped_connection_move_ctor
  Verifies the scoped_connection move constructor transfers the guard.
  Tests the following:
  - the moved-from guard no longer disconnects
  - the moved-to guard disconnects on destruction
*/
bool
tests_scoped_connection_move_ctor()
{
    observer<void()> o;

    {
        scoped_connection a = o.connect([]() {});
        scoped_connection b = std::move(a);   // b now owns the guard
        if (o.count() != 1) { return false; }
    }   // b disconnects; a (moved-from) is inert

    return (o.count() == 0);
}

/*
tests_scoped_connection_move_assign
  Verifies scoped_connection move assignment disconnects the prior guard.
  Tests the following:
  - move-assigning over a guard disconnects the slot it held
  - the new slot survives until the guard is destroyed
*/
bool
tests_scoped_connection_move_assign()
{
    observer<void()> o;

    connection first = o.connect([]() {});      // tracked below
    connection second = o.connect([]() {});

    scoped_connection guard{ first };
    guard = scoped_connection{ second };        // first must be disconnected

    bool ok = true;

    ok = ok && (!first.connected());   // prior guard's slot dropped
    ok = ok && (second.connected());   // new slot still live

    return ok;
}

/*
tests_scoped_connection_release
  Verifies scoped_connection::release surrenders the guard.
  Tests the following:
  - after release the slot survives the guard's destruction
*/
bool
tests_scoped_connection_release()
{
    observer<void()> o;

    {
        scoped_connection sc = o.connect([]() {});
        connection kept = sc.release();
        (void)kept;
    }   // guard destroyed, but it released -> no disconnect

    return (o.count() == 1);
}

/*
tests_scoped_connection_default
  Verifies a default-constructed scoped_connection is inert.
  Tests the following:
  - a default guard reports not connected and destructs without effect
*/
bool
tests_scoped_connection_default()
{
    scoped_connection sc;

    return (!sc.connected());
}

/*
tests_scoped_connections_add
  Verifies scoped_connections::add tracks connections.
  Tests the following:
  - add grows the tracked size
*/
bool
tests_scoped_connections_add()
{
    observer<void()> o;
    scoped_connections group;

    group.add(o.connect([]() {}));
    group.add(o.connect([]() {}));

    return (group.size() == 2);
}

/*
tests_scoped_connections_plus_equals
  Verifies scoped_connections::operator+= tracks connections.
  Tests the following:
  - operator+= grows the tracked size and chains
*/
bool
tests_scoped_connections_plus_equals()
{
    observer<void()> o;
    scoped_connections group;

    group += o.connect([]() {});
    group += o.connect([]() {});

    return (group.size() == 2);
}

/*
tests_scoped_connections_raii
  Verifies scoped_connections disconnects everything on destruction.
  Tests the following:
  - all tracked slots are live inside the scope
  - destroying the group disconnects them all
*/
bool
tests_scoped_connections_raii()
{
    observer<void()> o;

    {
        scoped_connections group;
        group.add(o.connect([]() {}));
        group.add(o.connect([]() {}));
        group.add(o.connect([]() {}));
        if (o.count() != 3) { return false; }
    }

    return (o.count() == 0);
}


NS_END  // testing
NS_END  // djinterp
