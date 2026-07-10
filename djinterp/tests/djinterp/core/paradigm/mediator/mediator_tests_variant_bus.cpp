// djinterp [test] : mediator_tests_variant_bus.cpp
//   The closed-set variant_event_bus (section V, C++17+).  variant_event_bus
// only exists when std::variant is available, so each predicate's body is
// guarded by D_MEDIATOR_HAS_VARIANT and reduces to a vacuous pass on older
// dialects; the declarations remain unconditional so the spec always links.

// std
#include <cstddef>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING

#if D_MEDIATOR_HAS_VARIANT
namespace
{
    // dual_visitor : a visit target callable with each alternative, tallying
    // per-type into caller-owned counters.  Copyable (it holds references), so
    // it survives being captured by value inside subscribe_all.
    struct dual_visitor
    {
        int* pings;
        int* pongs;

        void operator()(const ping&) const { ++(*pings); }
        void operator()(const pong&) const { ++(*pongs); }
    };
}
#endif


/*
tests_variant_bus_subscribe_publish
  Verifies a single-type subscription fires only for its alternative.
  Tests the following:
  - a ping handler fires when a ping is published
  - the same handler stays silent when a pong is published
*/
bool
tests_variant_bus_subscribe_publish()
{
#if D_MEDIATOR_HAS_VARIANT
    variant_event_bus<ping, pong> bus;
    int                           seen = 0;

    bus.subscribe<ping>([&](const ping& _e) { seen += _e.n; });

    bus.publish(ping{ 4 });
    bool ok = (seen == 4);

    bus.publish(pong{ 9 });         // different alternative: no fire
    ok = ok && (seen == 4);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_bus_multiple_types
  Verifies independent per-type handlers each fire for their own alternative.
  Tests the following:
  - publishing ping fires only the ping handler
  - publishing pong fires only the pong handler
*/
bool
tests_variant_bus_multiple_types()
{
#if D_MEDIATOR_HAS_VARIANT
    variant_event_bus<ping, pong> bus;
    int                           pings = 0;
    int                           pongs = 0;

    bus.subscribe<ping>([&](const ping&) { ++pings; });
    bus.subscribe<pong>([&](const pong&) { ++pongs; });

    bus.publish(ping{ 0 });
    bus.publish(pong{ 0 });
    bus.publish(pong{ 0 });

    bool ok = true;

    ok = ok && (pings == 1);
    ok = ok && (pongs == 2);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_bus_subscribe_all
  Verifies subscribe_all delivers every alternative to a visitor handler.
  Tests the following:
  - a visitor sees ping events on its ping overload
  - the same visitor sees pong events on its pong overload
*/
bool
tests_variant_bus_subscribe_all()
{
#if D_MEDIATOR_HAS_VARIANT
    variant_event_bus<ping, pong> bus;
    int                           pings = 0;
    int                           pongs = 0;

    bus.subscribe_all(dual_visitor{ &pings, &pongs });

    bus.publish(ping{ 0 });
    bus.publish(pong{ 0 });
    bus.publish(ping{ 0 });

    bool ok = true;

    ok = ok && (pings == 2);
    ok = ok && (pongs == 1);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_bus_unsubscribe
  Verifies unsubscribe removes the handler for a token.
  Tests the following:
  - handler_count drops after unsubscribe
  - the removed handler no longer fires
*/
bool
tests_variant_bus_unsubscribe()
{
#if D_MEDIATOR_HAS_VARIANT
    variant_event_bus<ping, pong> bus;
    int                           seen = 0;

    subscription_token tok =
        bus.subscribe<ping>([&](const ping&) { ++seen; });

    bus.unsubscribe(tok);

    bool ok = (bus.handler_count() == 0);

    bus.publish(ping{ 0 });
    ok = ok && (seen == 0);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_bus_handler_count
  Verifies handler_count reflects the number of subscriptions.
  Tests the following:
  - count rises with each subscribe (typed and visitor)
*/
bool
tests_variant_bus_handler_count()
{
#if D_MEDIATOR_HAS_VARIANT
    variant_event_bus<ping, pong> bus;

    bool ok = (bus.handler_count() == 0);

    bus.subscribe<ping>([](const ping&) {});
    ok = ok && (bus.handler_count() == 1);

    bus.subscribe<pong>([](const pong&) {});
    ok = ok && (bus.handler_count() == 2);

    bus.subscribe_all([](const auto&) {});
    ok = ok && (bus.handler_count() == 3);

    return ok;
#else
    return true;
#endif
}

/*
tests_variant_bus_clear
  Verifies clear removes every handler.
  Tests the following:
  - handler_count is zero after clear
  - publishing after clear invokes nothing
*/
bool
tests_variant_bus_clear()
{
#if D_MEDIATOR_HAS_VARIANT
    variant_event_bus<ping, pong> bus;
    int                           seen = 0;

    bus.subscribe<ping>([&](const ping&) { ++seen; });
    bus.subscribe<pong>([&](const pong&) { ++seen; });

    bus.clear();

    bool ok = (bus.handler_count() == 0);

    bus.publish(ping{ 0 });
    ok = ok && (seen == 0);

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
