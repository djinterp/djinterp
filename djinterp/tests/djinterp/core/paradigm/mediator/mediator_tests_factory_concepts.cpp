// djinterp [test] : mediator_tests_factory_concepts.cpp
//   Two dialect-gated layers.  The convenience factories (section IX, C++14+):
// make_event_bus, make_signal_hub, subscribe_many, unsubscribe_all — bodies
// guarded by D_ENV_LANG_IS_CPP14_OR_HIGHER.  The concept-constrained interfaces
// (section X, C++20+): mediator_for, colleague_of, handler_for,
// constrained_subscribe, constrained_connect — bodies guarded by
// D_MEDIATOR_HAS_CONCEPTS.  Each guarded body reduces to a vacuous pass on a
// dialect that lacks the feature, so every predicate is declared and defined at
// all supported language levels and the spec always links.

// std
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
#include "mediator_tests.hpp"


NS_DJINTERP
NS_TESTING

#if D_MEDIATOR_HAS_CONCEPTS
namespace
{
    // probe_med / probe_colleague : a minimal mediator/colleague pair that
    // satisfies colleague_of — set_mediator(probe_med*) plus a single-argument
    // on_event(const any&).
    struct probe_med
    {
    };

    struct probe_colleague
    {
        void set_mediator(probe_med*) {}
        void on_event(const compat::any&) {}
    };

    // ping_handler / int_handler : callables used to drive handler_for both
    // true and false.
    struct ping_handler
    {
        void operator()(const ping&) const {}
    };

    struct int_handler
    {
        void operator()(int) const {}
    };
}
#endif


/*
tests_make_event_bus
  Verifies make_event_bus yields a working event_bus.
  Tests the following:
  - the factory's result type is event_bus
  - the returned bus subscribes and publishes normally
*/
bool
tests_make_event_bus()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    auto bus = make_event_bus();

    static_assert(std::is_same<decltype(bus), event_bus>::value,
                  "make_event_bus returns event_bus");

    int seen = 0;
    bus.subscribe<ping>([&](const ping& _e) { seen = _e.n; });
    bus.publish(ping{ 21 });

    return (seen == 21);
#else
    return true;
#endif
}

/*
tests_make_signal_hub
  Verifies make_signal_hub yields a working signal_hub.
  Tests the following:
  - the factory's result type is signal_hub
  - the returned hub connects and emits normally
*/
bool
tests_make_signal_hub()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    auto hub = make_signal_hub();

    static_assert(std::is_same<decltype(hub), signal_hub>::value,
                  "make_signal_hub returns signal_hub");

    int seen = 0;
    hub.connect(signal_id("chan"), [&](const compat::any& _e) { seen = _e.get<int>(); });
    hub.emit("chan", compat::any(21));

    return (seen == 21);
#else
    return true;
#endif
}

/*
tests_subscribe_many
  Verifies subscribe_many registers a batch of handlers and returns a token
  per handler.
  Tests the following:
  - the returned vector has one token per supplied handler
  - handler_count_for reflects the batch size
  - a single publish fires every handler in the batch
*/
bool
tests_subscribe_many()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    event_bus bus;
    int       total = 0;

    std::vector<subscription_token> tokens =
        subscribe_many<ping>(
            bus,
            [&](const ping& _e) { total += _e.n; },
            [&](const ping& _e) { total += _e.n; },
            [&](const ping& _e) { total += _e.n; });

    bool ok = true;

    ok = ok && (tokens.size() == 3);
    ok = ok && (bus.handler_count_for<ping>() == 3);

    bus.publish(ping{ 10 });
    ok = ok && (total == 30);

    return ok;
#else
    return true;
#endif
}

/*
tests_unsubscribe_all
  Verifies unsubscribe_all removes a whole batch of tokens.
  Tests the following:
  - handler_count is zero after unsubscribing the returned batch
  - a subsequent publish fires nothing
*/
bool
tests_unsubscribe_all()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    event_bus bus;
    int       total = 0;

    std::vector<subscription_token> tokens =
        subscribe_many<ping>(
            bus,
            [&](const ping&) { ++total; },
            [&](const ping&) { ++total; });

    unsubscribe_all(bus, tokens);

    bool ok = (bus.handler_count() == 0);

    bus.publish(ping{ 1 });
    ok = ok && (total == 0);

    return ok;
#else
    return true;
#endif
}

/*
tests_mediator_for
  Verifies the mediator_for concept (a notify(const any&) surface).
  Tests the following:
  - satisfied by event_bus (has notify)
  - not satisfied by int
  - not satisfied by concrete_mediator, which exposes receive() rather than
    notify() (edge case)
*/
bool
tests_mediator_for()
{
#if D_MEDIATOR_HAS_CONCEPTS
    static_assert( mediator_for<event_bus>,          "event_bus mediates");
    static_assert(!mediator_for<int>,                "int does not");
    static_assert(!mediator_for<concrete_mediator>,  "receive() != notify()");

    bool ok = true;

    ok = ok && ( mediator_for<event_bus>);
    ok = ok && (!mediator_for<int>);
    ok = ok && (!mediator_for<concrete_mediator>);

    return ok;
#else
    return true;
#endif
}

/*
tests_colleague_of
  Verifies the colleague_of concept (set_mediator + single-arg on_event).
  Tests the following:
  - satisfied by a matching mediator/colleague pair
  - not satisfied by an unrelated type (int)
*/
bool
tests_colleague_of()
{
#if D_MEDIATOR_HAS_CONCEPTS
    static_assert( colleague_of<probe_colleague, probe_med>, "matching pair");
    static_assert(!colleague_of<int, probe_med>,             "int is not one");

    bool ok = true;

    ok = ok && ( colleague_of<probe_colleague, probe_med>);
    ok = ok && (!colleague_of<int, probe_med>);

    return ok;
#else
    return true;
#endif
}

/*
tests_handler_for
  Verifies the handler_for concept (invocability with const _Event&).
  Tests the following:
  - satisfied when the callable accepts the event type
  - not satisfied when the callable's parameter is incompatible
  - not satisfied when invoked against an unrelated event type
*/
bool
tests_handler_for()
{
#if D_MEDIATOR_HAS_CONCEPTS
    static_assert( handler_for<ping_handler, ping>, "ping handler for ping");
    static_assert(!handler_for<int_handler, ping>,  "int handler not for ping");
    static_assert(!handler_for<ping_handler, pong>, "ping handler not for pong");

    bool ok = true;

    ok = ok && ( handler_for<ping_handler, ping>);
    ok = ok && (!handler_for<int_handler, ping>);
    ok = ok && (!handler_for<ping_handler, pong>);

    return ok;
#else
    return true;
#endif
}

/*
tests_constrained_subscribe
  Verifies constrained_subscribe subscribes a concept-satisfying handler.
  Tests the following:
  - the returned handler fires on publish of the constrained event type
*/
bool
tests_constrained_subscribe()
{
#if D_MEDIATOR_HAS_CONCEPTS
    event_bus bus;
    int       seen = 0;

    constrained_subscribe<ping>(bus, [&](const ping& _e) { seen = _e.n; });

    bus.publish(ping{ 44 });

    return (seen == 44);
#else
    return true;
#endif
}

/*
tests_constrained_connect
  Verifies constrained_connect connects a concept-satisfying handler.
  Tests the following:
  - the returned handler fires on emit of the connected channel
*/
bool
tests_constrained_connect()
{
#if D_MEDIATOR_HAS_CONCEPTS
    signal_hub hub;
    int        seen = 0;

    constrained_connect(hub, "chan",
                        [&](const compat::any& _e) { seen = _e.get<int>(); });

    hub.emit("chan", compat::any(44));

    return (seen == 44);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
