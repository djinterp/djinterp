/*******************************************************************************
* djinterp [core]                                  publisher_tests_factories.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_factories.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_factories.cpp
//   The factories (section X): make_bus builds a default synchronous bus for a
// given topic/payload pair (optionally with an explicit handler type), and
// make_message builds a topic/payload envelope with the argument types deduced.

// std
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
    // tally: a plain functor handler, used to prove make_bus honours an
    // explicit handler type rather than always defaulting to std::function.
    struct tally
    {
        int* total;

        explicit tally(int* _total = nullptr)
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
}


/*
tests_make_bus
  Verifies make_bus yields a usable synchronous bus.
  Tests the following:
  - the returned object is exactly bus<Topic, Payload> with the default handler
  - it starts empty and then subscribes, publishes, and delivers normally
*/
bool
tests_make_bus()
{
    auto b = make_bus<std::string, int>();

    static_assert(std::is_same<decltype(b), string_bus>::value,
                  "make_bus yields the default bus type");

    std::vector<delivery> log;

    bool ok = (b.empty());

    subscription s = b.subscribe("t", recorder(&log));

    pubsub_status st = b.publish("t", 6);

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (b.subscriber_count("t") == 1);
    ok = ok && (log.size() == 1 && log[0].topic == "t" && log[0].payload == 6);

    return ok;
}

/*
tests_make_bus_custom_handler
  Verifies make_bus honours an explicit handler type.
  Tests the following:
  - the returned bus carries the requested handler type, not the std::function
    default
  - the custom handler is invoked on publish
*/
bool
tests_make_bus_custom_handler()
{
    auto b = make_bus<std::string, int, tally>();

    static_assert(std::is_same<decltype(b)::handler_type, tally>::value,
                  "the explicit handler type is honoured");

    int total = 0;

    subscription s = b.subscribe("t", tally(&total));

    b.publish("t", 4);
    b.publish("t", 3);

    return (total == 7);
}

/*
tests_make_message
  Verifies make_message builds an envelope with deduced argument types.
  Tests the following:
  - the topic and payload land in the message
  - the deduced type is pubsub_message<Topic, Payload>
  - the resulting message can be published through the matching overload
*/
bool
tests_make_message()
{
    auto m = make_message(std::string("t"), 3);

    static_assert(std::is_same<decltype(m),
                               pubsub_message<std::string, int> >::value,
                  "make_message deduces its topic and payload types");

    bool ok = true;

    ok = ok && (m.topic == "t");
    ok = ok && (m.payload == 3);

    // the envelope routes through the message publish overload
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    pubsub_status st = b.publish(m);

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (log.size() == 1 && log[0].payload == 3);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
