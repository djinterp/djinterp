/*******************************************************************************
* djinterp [core]                                    publisher_tests_message.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_message.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_message.cpp
//   The message envelope (section IV): pubsub_message pairs a topic with its
// payload so a queued or asynchronous broker can store a message between
// publication and delivery. Covers the three constructors and the exported
// type aliases. The copy and move constructors are selected deliberately (const
// lvalues bind only the copy form; std::move'd lvalues select the move form),
// so each is genuinely exercised rather than left to overload resolution.

// std
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    using message = pubsub_message<std::string, int>;
}


/*
tests_message_default
  Verifies the default-constructed message.
  Tests the following:
  - both the topic and the payload are value-initialized
*/
bool
tests_message_default()
{
    message m;

    bool ok = true;

    ok = ok && (m.topic.empty());
    ok = ok && (m.payload == 0);

    return ok;
}

/*
tests_message_copy_ctor
  Verifies construction from const lvalues (the copying overload).
  Tests the following:
  - the topic and payload are copied into the message
  - the sources are left intact
*/
bool
tests_message_copy_ctor()
{
    const std::string topic   = "sensor";
    const int         payload = 21;

    message m(topic, payload);            // const lvalues -> copy overload

    bool ok = true;

    ok = ok && (m.topic == "sensor");
    ok = ok && (m.payload == 21);
    ok = ok && (topic == "sensor");       // source untouched

    return ok;
}

/*
tests_message_move_ctor
  Verifies construction by moving both fields.
  Tests the following:
  - the topic and payload land in the message
  - the moved-from topic really was moved (it is left empty by libstdc++'s
    small-string move; asserted only as "not the original value")
*/
bool
tests_message_move_ctor()
{
    std::string topic   = "a-long-topic-name-defeating-sso-optimisation";
    int         payload = 9;

    message m(std::move(topic), std::move(payload));   // rvalues -> move overload

    bool ok = true;

    ok = ok && (m.topic == "a-long-topic-name-defeating-sso-optimisation");
    ok = ok && (m.payload == 9);
    ok = ok && (topic.empty());          // the source string was moved out of

    return ok;
}

/*
tests_message_typedefs
  Verifies the exported type aliases.
  Tests the following:
  - topic_type and payload_type name the template arguments
  - the members have those types
*/
bool
tests_message_typedefs()
{
    static_assert(std::is_same<message::topic_type, std::string>::value,
                  "topic_type");
    static_assert(std::is_same<message::payload_type, int>::value,
                  "payload_type");
    static_assert(std::is_same<decltype(message().topic), std::string>::value,
                  "topic member");
    static_assert(std::is_same<decltype(message().payload), int>::value,
                  "payload member");

    return ( std::is_same<message::topic_type, std::string>::value &&
             std::is_same<message::payload_type, int>::value );
}


NS_END  // testing
NS_END  // djinterp
