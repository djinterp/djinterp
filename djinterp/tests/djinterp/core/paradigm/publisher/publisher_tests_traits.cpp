/*******************************************************************************
* djinterp [core]                                     publisher_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_traits.cpp
//   The conformance traits (section VIII) and the C++20 concepts (section IX).
// These live at djinterp:: top level rather than in a traits sub-namespace,
// matching the flat convention the paradigm headers use.
//
//   Each SFINAE detector is exercised against a conforming broker (the bus) and
// against a type that cannot possibly satisfy it (int). is_subscription draws a
// finer distinction worth pinning: subscription and scoped_subscription both
// expose active() + unsubscribe(), so both satisfy it, but scoped_subscriptions
// - which offers only unsubscribe_all() and no active() - does NOT, even though
// it is very much part of the handle family.

// std
#include <string>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_trait_has_subscribe
  Verifies pubsub_has_subscribe detects subscribe(topic, handler).
  Tests the following:
  - true for a bus, false for a non-broker type
*/
bool
tests_trait_has_subscribe()
{
    static_assert( pubsub_has_subscribe<string_bus>::value, "bus subscribes");
    static_assert(!pubsub_has_subscribe<int>::value,        "int does not");

    return ( pubsub_has_subscribe<string_bus>::value &&
            !pubsub_has_subscribe<int>::value );
}

/*
tests_trait_has_publish
  Verifies pubsub_has_publish detects publish(topic, payload).
  Tests the following:
  - true for a bus, false for a non-broker type
*/
bool
tests_trait_has_publish()
{
    static_assert( pubsub_has_publish<string_bus>::value, "bus publishes");
    static_assert(!pubsub_has_publish<int>::value,        "int does not");

    return ( pubsub_has_publish<string_bus>::value &&
            !pubsub_has_publish<int>::value );
}

/*
tests_trait_has_unsubscribe
  Verifies pubsub_has_unsubscribe detects unsubscribe(token).
  Tests the following:
  - true for a bus, false for a non-broker type
*/
bool
tests_trait_has_unsubscribe()
{
    static_assert( pubsub_has_unsubscribe<string_bus>::value, "bus unsubscribes");
    static_assert(!pubsub_has_unsubscribe<int>::value,        "int does not");

    return ( pubsub_has_unsubscribe<string_bus>::value &&
            !pubsub_has_unsubscribe<int>::value );
}

/*
tests_trait_is_broker
  Verifies the composite broker protocol trait.
  Tests the following:
  - a bus satisfies all three requirements and so is a broker
  - a non-broker type is not
  - a subscription handle - which has unsubscribe() but neither subscribe nor
    publish - is not mistaken for a broker
*/
bool
tests_trait_is_broker()
{
    static_assert( is_broker<string_bus>::value, "bus is a broker");
    static_assert(!is_broker<int>::value,        "int is not");

    bool ok = true;

    ok = ok && ( is_broker<string_bus>::value);
    ok = ok && (!is_broker<int>::value);
    ok = ok && (!is_broker<subscription>::value);   // a handle is not a broker

    return ok;
}

/*
tests_trait_is_broker_v
  Verifies the is_broker_v alias.
  Tests the following:
  - the _v alias mirrors is_broker<>::value for conforming and non-conforming
*/
bool
tests_trait_is_broker_v()
{
    bool ok = true;

    ok = ok && (is_broker_v<string_bus> == is_broker<string_bus>::value);
    ok = ok && (is_broker_v<int> == is_broker<int>::value);
    ok = ok && ( is_broker_v<string_bus>);
    ok = ok && (!is_broker_v<int>);

    return ok;
}

/*
tests_trait_is_subscription
  Verifies the subscription-handle protocol trait (active + unsubscribe).
  Tests the following:
  - a subscription satisfies it
  - a scoped_subscription satisfies it too (it forwards both members)
  - scoped_subscriptions does NOT: it exposes unsubscribe_all(), not
    unsubscribe(), and has no active()
  - an unrelated type does not
*/
bool
tests_trait_is_subscription()
{
    static_assert( is_subscription<subscription>::value,         "handle");
    static_assert( is_subscription<scoped_subscription>::value,  "RAII guard");
    static_assert(!is_subscription<scoped_subscriptions>::value, "the group is not");
    static_assert(!is_subscription<int>::value,                  "int is not");

    bool ok = true;

    ok = ok && ( is_subscription<subscription>::value);
    ok = ok && ( is_subscription<scoped_subscription>::value);
    ok = ok && (!is_subscription<scoped_subscriptions>::value);
    ok = ok && (!is_subscription<int>::value);
    ok = ok && (!is_subscription<string_bus>::value);   // a broker is not a handle

    return ok;
}

/*
tests_trait_is_subscription_v
  Verifies the is_subscription_v alias.
  Tests the following:
  - the _v alias mirrors is_subscription<>::value across the handle family
*/
bool
tests_trait_is_subscription_v()
{
    bool ok = true;

    ok = ok && (is_subscription_v<subscription> ==
                is_subscription<subscription>::value);
    ok = ok && (is_subscription_v<scoped_subscriptions> ==
                is_subscription<scoped_subscriptions>::value);
    ok = ok && ( is_subscription_v<subscription>);
    ok = ok && (!is_subscription_v<int>);

    return ok;
}

/*
tests_concept_broker_type
  Verifies the C++20 broker_type concept.
  Tests the following:
  - satisfied by a bus and not by a non-broker type (C++20+)
*/
bool
tests_concept_broker_type()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert( broker_type<string_bus>, "bus conforms");
    static_assert(!broker_type<int>,        "int does not");

    return (broker_type<string_bus> && !broker_type<int>);
#else
    return true;
#endif
}

/*
tests_concept_handler_for
  Verifies the C++20 handler_for concept.
  Tests the following:
  - satisfied by a callable invocable as void(topic, payload) - both the shared
    recorder functor and a lambda
  - not satisfied by a non-invocable type (C++20+)
*/
bool
tests_concept_handler_for()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    auto lam = [](const std::string&, const int&) {};

    static_assert( handler_for<recorder, std::string, int>,          "functor");
    static_assert( handler_for<decltype(lam), std::string, int>,     "lambda");
    static_assert(!handler_for<int, std::string, int>,               "int is not");

    return ( handler_for<recorder, std::string, int> &&
             handler_for<decltype(lam), std::string, int> &&
            !handler_for<int, std::string, int> );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
