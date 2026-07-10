// djinterp [test] : mediator_tests_event_bus.cpp
//   The type-indexed publish/subscribe event bus (section IV): subscription
// tokens, subscribe, both publish overloads (const-lvalue and rvalue), the
// pre-wrapped notify() path, handler_count / handler_count_for, unsubscribe,
// and clear.  Handlers are lambdas that accumulate into test-local counters.

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_subscription_token_type
  Verifies the subscription_token alias.
  Tests the following:
  - subscription_token is std::size_t
*/
bool
tests_subscription_token_type()
{
    static_assert(std::is_same<subscription_token, std::size_t>::value,
                  "subscription_token is std::size_t");

    return std::is_same<subscription_token, std::size_t>::value;
}

/*
tests_event_bus_subscribe_publish
  Verifies a subscribed handler fires with the correct payload on publish.
  Tests the following:
  - subscribe registers exactly one handler
  - publishing that event type invokes the handler with the payload
*/
bool
tests_event_bus_subscribe_publish()
{
    event_bus bus;
    int       seen = 0;

    bus.subscribe<ping>([&](const ping& _e) { seen += _e.n; });

    bool ok = (bus.handler_count() == 1);

    bus.publish(ping{ 5 });
    ok = ok && (seen == 5);

    return ok;
}

/*
tests_event_bus_type_filtering
  Verifies dispatch is keyed on the event type identity.
  Tests the following:
  - a ping handler and a pong handler coexist
  - publishing ping fires only the ping handler
  - publishing pong fires only the pong handler
*/
bool
tests_event_bus_type_filtering()
{
    event_bus bus;
    int       pings = 0;
    int       pongs = 0;

    bus.subscribe<ping>([&](const ping&) { ++pings; });
    bus.subscribe<pong>([&](const pong&) { ++pongs; });

    bus.publish(ping{ 1 });
    bus.publish(ping{ 1 });
    bus.publish(pong{ 1 });

    bool ok = true;

    ok = ok && (pings == 2);
    ok = ok && (pongs == 1);

    return ok;
}

/*
tests_event_bus_multiple_handlers
  Verifies that every handler registered for a type is invoked.
  Tests the following:
  - three ping handlers all fire on a single publish
  - handler_count_for<ping> reports three
*/
bool
tests_event_bus_multiple_handlers()
{
    event_bus bus;
    int       total = 0;

    bus.subscribe<ping>([&](const ping& _e) { total += _e.n; });
    bus.subscribe<ping>([&](const ping& _e) { total += _e.n; });
    bus.subscribe<ping>([&](const ping& _e) { total += _e.n; });

    bool ok = (bus.handler_count_for<ping>() == 3);

    bus.publish(ping{ 10 });
    ok = ok && (total == 30);

    return ok;
}

/*
tests_event_bus_unsubscribe
  Verifies unsubscribe removes exactly the handler for a given token.
  Tests the following:
  - the removed handler no longer fires
  - a sibling handler continues to fire
  - handler_count drops by one
*/
bool
tests_event_bus_unsubscribe()
{
    event_bus bus;
    int       a = 0;
    int       b = 0;

    subscription_token tok = bus.subscribe<ping>([&](const ping&) { ++a; });
    bus.subscribe<ping>([&](const ping&) { ++b; });

    bus.unsubscribe(tok);

    bool ok = (bus.handler_count() == 1);

    bus.publish(ping{ 0 });
    ok = ok && (a == 0);
    ok = ok && (b == 1);

    return ok;
}

/*
tests_event_bus_publish_move
  Verifies the rvalue publish overload.
  Tests the following:
  - publishing a temporary invokes the matching handler with the payload
*/
bool
tests_event_bus_publish_move()
{
    event_bus bus;
    int       seen = 0;

    bus.subscribe<ping>([&](const ping& _e) { seen = _e.n; });

    bus.publish(ping{ 77 });        // rvalue

    return (seen == 77);
}

/*
tests_event_bus_publish_lvalue
  Verifies the const-lvalue publish overload.
  Tests the following:
  - publishing a named const value invokes the matching handler
*/
bool
tests_event_bus_publish_lvalue()
{
    event_bus bus;
    int       seen = 0;

    bus.subscribe<ping>([&](const ping& _e) { seen = _e.n; });

    const ping e{ 33 };
    bus.publish(e);                 // const lvalue

    return (seen == 33);
}

/*
tests_event_bus_notify
  Verifies notify() dispatches a pre-wrapped compat::any by its stored type id.
  Tests the following:
  - a ping wrapped in compat::any is delivered to the ping handler
*/
bool
tests_event_bus_notify()
{
    event_bus bus;
    int       seen = 0;

    bus.subscribe<ping>([&](const ping& _e) { seen = _e.n; });

    compat::any wrapped(ping{ 55 });
    bus.notify(wrapped);

    return (seen == 55);
}

/*
tests_event_bus_notify_type_gated
  Verifies notify() only reaches handlers whose event type matches the any.
  Tests the following:
  - notifying with a pong any does not fire a ping handler
  - notifying with a ping any does fire the ping handler
*/
bool
tests_event_bus_notify_type_gated()
{
    event_bus bus;
    int       pings = 0;

    bus.subscribe<ping>([&](const ping&) { ++pings; });

    bus.notify(compat::any(pong{ 1 }));     // wrong type: no fire
    bool ok = (pings == 0);

    bus.notify(compat::any(ping{ 1 }));     // right type: fire
    ok = ok && (pings == 1);

    return ok;
}

/*
tests_event_bus_handler_counts
  Verifies handler_count and handler_count_for across a mixed handler set.
  Tests the following:
  - handler_count reports the grand total across types
  - handler_count_for reports the per-type total
  - handler_count_for is zero for an unsubscribed type
*/
bool
tests_event_bus_handler_counts()
{
    event_bus bus;

    bus.subscribe<ping>([](const ping&) {});
    bus.subscribe<ping>([](const ping&) {});
    bus.subscribe<pong>([](const pong&) {});

    bool ok = true;

    ok = ok && (bus.handler_count() == 3);
    ok = ok && (bus.handler_count_for<ping>() == 2);
    ok = ok && (bus.handler_count_for<pong>() == 1);
    ok = ok && (bus.handler_count_for<tick>() == 0);

    return ok;
}

/*
tests_event_bus_clear
  Verifies clear removes every handler.
  Tests the following:
  - handler_count is zero after clear
  - publishing after clear invokes nothing
*/
bool
tests_event_bus_clear()
{
    event_bus bus;
    int       seen = 0;

    bus.subscribe<ping>([&](const ping&) { ++seen; });
    bus.subscribe<pong>([&](const pong&) { ++seen; });

    bus.clear();

    bool ok = (bus.handler_count() == 0);

    bus.publish(ping{ 1 });
    bus.publish(pong{ 1 });
    ok = ok && (seen == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
