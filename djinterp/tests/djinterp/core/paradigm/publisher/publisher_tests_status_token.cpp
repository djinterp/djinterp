/*******************************************************************************
* djinterp [core]                               publisher_tests_status_token.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_status_token.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_status_token.cpp
//   Status codes (section I) and the subscription token (section II): the
// standard publish status constants and the opaque, copyable registration id
// whose zero value is reserved to mean "no subscription".

// std
#include <cstdint>
#include <string>
#include <type_traits>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_status_codes
  Verifies the standard publish status constants.
  Tests the following:
  - Ok / NoSubscribers / TopicNotFound / TokenNotFound / Rejected take their
    documented values
  - the user base sits above every reserved code
*/
bool
tests_status_codes()
{
    bool ok = true;

    ok = ok && (DPubSubStatusOk == 0);
    ok = ok && (DPubSubStatusNoSubscribers == 1);
    ok = ok && (DPubSubStatusTopicNotFound == 2);
    ok = ok && (DPubSubStatusTokenNotFound == 3);
    ok = ok && (DPubSubStatusRejected == 4);
    ok = ok && (DPubSubStatusUserBase == 64);
    ok = ok && (DPubSubStatusUserBase > DPubSubStatusRejected);

    return ok;
}

/*
tests_status_default_publish_codes
  Verifies which status codes the default routing actually emits. TopicNotFound
  and TokenNotFound are standard codes reserved for derived brokers; the default
  do_publish never produces them - an unknown topic is reported as
  NoSubscribers, not TopicNotFound.
  Tests the following:
  - a delivered publish reports Ok
  - a publish to an unknown topic reports NoSubscribers (not TopicNotFound)
*/
bool
tests_status_default_publish_codes()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("known", recorder(&log));

    bool ok = true;

    ok = ok && (b.publish("known", 1) == DPubSubStatusOk);
    ok = ok && (b.publish("unknown", 1) == DPubSubStatusNoSubscribers);
    ok = ok && (b.publish("unknown", 1) != DPubSubStatusTopicNotFound);

    return ok;
}

/*
tests_token_default
  Verifies the default-constructed token.
  Tests the following:
  - the default token carries id zero (the reserved "no subscription" value)
*/
bool
tests_token_default()
{
    subscription_token t;

    return (t.id == 0);
}

/*
tests_token_explicit
  Verifies the explicit-id constructor.
  Tests the following:
  - the supplied id is stored verbatim
*/
bool
tests_token_explicit()
{
    subscription_token t(42);

    return (t.id == 42);
}

/*
tests_token_valid
  Verifies the validity predicate.
  Tests the following:
  - a default (zero-id) token is invalid
  - a non-zero token is valid
*/
bool
tests_token_valid()
{
    subscription_token null_tok;
    subscription_token real_tok(1);

    return (!null_tok.valid() && real_tok.valid());
}

/*
tests_token_equality
  Verifies the friend comparison operators.
  Tests the following:
  - tokens with the same id compare equal and not unequal
  - tokens with different ids compare unequal and not equal
*/
bool
tests_token_equality()
{
    subscription_token a(5);
    subscription_token b(5);
    subscription_token c(6);

    bool ok = true;

    ok = ok && (a == b);
    ok = ok && (!(a != b));
    ok = ok && (a != c);
    ok = ok && (!(a == c));

    return ok;
}

/*
tests_token_id_type
  Verifies the token's id type.
  Tests the following:
  - id_type is a 64-bit unsigned integer
  - the id member has that type
*/
bool
tests_token_id_type()
{
    static_assert(std::is_same<subscription_token::id_type, std::uint64_t>::value,
                  "id_type is uint64_t");
    static_assert(std::is_same<decltype(subscription_token().id),
                               subscription_token::id_type>::value,
                  "the id member uses id_type");

    return std::is_same<subscription_token::id_type, std::uint64_t>::value;
}


NS_END  // testing
NS_END  // djinterp
