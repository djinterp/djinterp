/*******************************************************************************
* djinterp [core]                              publisher_tests_customization.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_customization.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_customization.cpp
//   The customization points (section VI) and the concrete bus (section VII).
//
//   The three hooks are public, non-virtual members that a derived broker HIDES;
// the base reaches them through its CRTP cast, so a derived definition wins
// without any virtual dispatch. Each hook gets its own derived fixture below:
//     - should_publish : a cheap veto consulted before any delivery. Vetoing
//                        yields DPubSubStatusRejected and touches no handler.
//     - do_deliver     : reshapes delivery of one message to one handler.
//     - do_publish     : replaces routing wholesale, so the registry need not be
//                        consulted at all.
//
//   The bus tests cover the handler-type parameter: the std::function default,
// a raw function pointer, and a custom functor - plus a non-string topic, since
// a topic need only be copy-constructible and equality-comparable.

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // ---- free-function handler, for the raw fn-ptr bus ------------------
    int g_fn_total = 0;

    void
    fn_handler(const std::string& /*_topic*/,
               const int&         _payload)
    {
        g_fn_total += _payload;

        return;
    }

    using fn_ptr_bus = bus<std::string, int, void(*)(const std::string&, const int&)>;


    // ---- custom functor handler ----------------------------------------
    // accumulate: a plain functor (not a std::function) used as the handler type
    // itself, proving the handler parameter accepts any invocable.
    struct accumulate
    {
        int* total;

        explicit accumulate(int* _total = nullptr)
            : total(_total)
        {}

        void
        operator()(const std::string& /*_topic*/,
                   const int&         _payload) const
        {
            if (total != nullptr) { *total += _payload; }

            return;
        }
    };

    using functor_bus = bus<std::string, int, accumulate>;


    // ---- non-string topic ----------------------------------------------
    // int_topic_bus: topics need only be copy-constructible and ==-comparable.
    struct int_accumulate
    {
        int* total;

        explicit int_accumulate(int* _total = nullptr)
            : total(_total)
        {}

        void
        operator()(const int& /*_topic*/,
                   const int& _payload) const
        {
            if (total != nullptr) { *total += _payload; }

            return;
        }
    };

    using int_topic_bus = bus<int, int, int_accumulate>;


    // ---- derived broker: should_publish veto ----------------------------
    // veto_broker: rejects publication of a non-positive payload, and can also
    // be closed wholesale via its `open` flag - covering both a payload-driven
    // and a state-driven veto.
    struct veto_broker
        : broker<veto_broker, std::string, int>
    {
        bool open;

        veto_broker()
            : open(true)
        {}

        D_NODISCARD
        bool
        should_publish(const std::string& /*_topic*/,
                       const int&         _payload)
        {
            return (open && _payload > 0);
        }
    };


    // ---- derived broker: do_deliver override ----------------------------
    // doubling_broker: reshapes per-handler delivery, handing each subscriber
    // twice the published payload. Routing is otherwise the inherited default.
    struct doubling_broker
        : broker<doubling_broker, std::string, int>
    {
        void
        do_deliver(handler_type&      _handler,
                   const std::string& _topic,
                   const int&         _payload)
        {
            _handler(_topic, 2 * _payload);

            return;
        }
    };


    // ---- derived broker: do_publish override ----------------------------
    // counting_broker: replaces routing wholesale - it records the publication
    // and returns a status of its own choosing, never consulting the registry,
    // so no subscriber is ever invoked.
    struct counting_broker
        : broker<counting_broker, std::string, int>
    {
        int published;

        counting_broker()
            : published(0)
        {}

        pubsub_status
        do_publish(const std::string& /*_topic*/,
                   const int&         /*_payload*/)
        {
            ++published;

            return DPubSubStatusTopicNotFound;   // a code the default never emits
        }
    };
}


/*
tests_bus_default_hooks
  Verifies bus inherits every hook default and needs no overrides.
  Tests the following:
  - the default should_publish permits delivery (no Rejected status)
  - the default do_deliver invokes the handler synchronously with the published
    topic and payload, unmodified
  - the default do_publish reports Ok when at least one subscriber received it
*/
bool
tests_bus_default_hooks()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    pubsub_status st = b.publish("t", 11);

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (st != DPubSubStatusRejected);            // nothing vetoed
    ok = ok && (log.size() == 1);
    ok = ok && (log[0].topic == "t" && log[0].payload == 11);   // delivered verbatim

    return ok;
}

/*
tests_bus_typedefs
  Verifies the type aliases bus inherits from the broker base.
  Tests the following:
  - topic_type / payload_type / handler_type name the template arguments, the
    handler defaulting to a std::function
  - token_type, subscription_type, message_type and status_type name the shared
    module types
*/
bool
tests_bus_typedefs()
{
    static_assert(std::is_same<string_bus::topic_type, std::string>::value,
                  "topic_type");
    static_assert(std::is_same<string_bus::payload_type, int>::value,
                  "payload_type");
    static_assert(std::is_same<string_bus::handler_type,
                               std::function<void(const std::string&, const int&)> >::value,
                  "handler defaults to std::function");
    static_assert(std::is_same<string_bus::token_type, subscription_token>::value,
                  "token_type");
    static_assert(std::is_same<string_bus::subscription_type, subscription>::value,
                  "subscription_type");
    static_assert(std::is_same<string_bus::message_type,
                               pubsub_message<std::string, int> >::value,
                  "message_type");
    static_assert(std::is_same<string_bus::status_type, pubsub_status>::value,
                  "status_type");

    return ( std::is_same<string_bus::topic_type, std::string>::value &&
             std::is_same<string_bus::token_type, subscription_token>::value );
}

/*
tests_bus_fn_ptr_handler
  Verifies a bus parameterised on a raw function pointer handler.
  Tests the following:
  - a free function can be registered and is invoked on publish
  - multiple registrations of it each receive the payload
*/
bool
tests_bus_fn_ptr_handler()
{
    fn_ptr_bus b;

    subscription s1 = b.subscribe("t", &fn_handler);
    subscription s2 = b.subscribe("t", &fn_handler);

    g_fn_total = 0;

    pubsub_status st = b.publish("t", 5);

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (b.subscriber_count("t") == 2);
    ok = ok && (g_fn_total == 10);         // both registrations fired

    return ok;
}

/*
tests_bus_functor_handler
  Verifies a bus parameterised on a custom functor handler.
  Tests the following:
  - the functor is stored and invoked directly (no std::function in the way)
  - revoking the subscription stops delivery to it
*/
bool
tests_bus_functor_handler()
{
    functor_bus b;

    int total = 0;

    subscription s = b.subscribe("t", accumulate(&total));

    b.publish("t", 7);

    bool ok = (total == 7);

    s.unsubscribe();
    b.publish("t", 7);

    ok = ok && (total == 7);               // no further delivery

    return ok;
}

/*
tests_bus_nonstring_topic
  Verifies a topic type need only be copy-constructible and ==-comparable.
  Tests the following:
  - an int-keyed bus routes by topic equality
  - a publish to a different topic value does not reach the subscriber
*/
bool
tests_bus_nonstring_topic()
{
    int_topic_bus b;

    int total = 0;

    subscription s = b.subscribe(7, int_accumulate(&total));

    pubsub_status hit  = b.publish(7, 5);
    pubsub_status miss = b.publish(8, 5);   // a different topic value

    bool ok = true;

    ok = ok && (hit == DPubSubStatusOk);
    ok = ok && (miss == DPubSubStatusNoSubscribers);
    ok = ok && (total == 5);                // only the matching topic delivered
    ok = ok && (b.has_subscribers(7) && !b.has_subscribers(8));

    return ok;
}

/*
tests_hook_should_publish_veto
  Verifies a payload-driven veto through the should_publish hook.
  Tests the following:
  - an allowed payload is delivered and reports Ok
  - a vetoed payload reports Rejected and reaches no handler
*/
bool
tests_hook_should_publish_veto()
{
    veto_broker           vb;
    std::vector<delivery> log;

    subscription s = vb.subscribe("t", recorder(&log));

    pubsub_status allowed = vb.publish("t", 5);
    pubsub_status vetoed  = vb.publish("t", -1);   // non-positive -> vetoed

    bool ok = true;

    ok = ok && (allowed == DPubSubStatusOk);
    ok = ok && (vetoed == DPubSubStatusRejected);
    ok = ok && (log.size() == 1 && log[0].payload == 5);   // only the allowed one

    return ok;
}

/*
tests_hook_should_publish_state
  Verifies a broker-state veto through the should_publish hook.
  Tests the following:
  - closing the broker rejects an otherwise valid publication
  - re-opening it restores delivery
  - the subscriber itself is untouched throughout
*/
bool
tests_hook_should_publish_state()
{
    veto_broker           vb;
    std::vector<delivery> log;

    subscription s = vb.subscribe("t", recorder(&log));

    vb.open = false;
    pubsub_status closed = vb.publish("t", 5);

    bool ok = true;

    ok = ok && (closed == DPubSubStatusRejected);
    ok = ok && (log.empty());
    ok = ok && (s.active());               // the registration is unaffected
    ok = ok && (vb.subscriber_count("t") == 1);

    vb.open = true;
    pubsub_status reopened = vb.publish("t", 5);

    ok = ok && (reopened == DPubSubStatusOk);
    ok = ok && (log.size() == 1 && log[0].payload == 5);

    return ok;
}

/*
tests_hook_do_deliver_override
  Verifies a do_deliver override reshapes per-handler delivery while leaving the
  inherited routing intact.
  Tests the following:
  - each subscriber receives the transformed payload
  - routing is otherwise unchanged: every live subscriber is still visited and
    the status is still Ok
*/
bool
tests_hook_do_deliver_override()
{
    doubling_broker       db;
    std::vector<delivery> first;
    std::vector<delivery> second;

    subscription s1 = db.subscribe("t", recorder(&first));
    subscription s2 = db.subscribe("t", recorder(&second));

    pubsub_status st = db.publish("t", 5);

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (first.size() == 1 && first[0].payload == 10);    // doubled
    ok = ok && (second.size() == 1 && second[0].payload == 10);  // every subscriber
    ok = ok && (first[0].topic == "t");                          // topic passes through

    return ok;
}

/*
tests_hook_do_publish_override
  Verifies a do_publish override replaces routing wholesale.
  Tests the following:
  - publish reaches the override (its counter advances)
  - the override's own status code is returned, including one the default routing
    never emits
  - no subscriber is invoked, since the registry is never consulted
  - the message and call-operator overloads route through the same override
*/
bool
tests_hook_do_publish_override()
{
    counting_broker       cb;
    std::vector<delivery> log;

    subscription s = cb.subscribe("t", recorder(&log));

    pubsub_status st = cb.publish("t", 5);

    bool ok = true;

    ok = ok && (cb.published == 1);
    ok = ok && (st == DPubSubStatusTopicNotFound);   // the default never returns this
    ok = ok && (log.empty());                        // delivery bypassed entirely

    // the other publication faces funnel through do_publish too
    cb.publish(pubsub_message<std::string, int>("t", 1));
    cb("t", 1);

    ok = ok && (cb.published == 3);
    ok = ok && (log.empty());
    ok = ok && (cb.subscriber_count("t") == 1);      // the registration still exists

    return ok;
}


NS_END  // testing
NS_END  // djinterp
