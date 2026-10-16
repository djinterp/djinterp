/*******************************************************************************
* djinterp [core]                                     publisher_tests_broker.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_broker.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_broker.cpp
//   The broker CRTP base (section VI): the subscribe / publish / unsubscribe
// surface and the bookkeeping it inherits from the registry. The base has a
// protected constructor, so it is exercised through the concrete bus - which
// overrides nothing and therefore tests the base's default behaviour exactly.
//
//   The eager/lazy split is the recurring theme: broker::unsubscribe(token),
// unsubscribe_topic() and unsubscribe_all() ERASE entries (and drop the topic
// bucket once empty), whereas subscription::unsubscribe() only flips the shared
// flag - so a lazily revoked entry is skipped by publish and vanishes from the
// live counts while its storage lingers until compact() reclaims it.

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_broker_default_state
  Verifies a freshly constructed broker.
  Tests the following:
  - it reports empty, with no subscribers and no topics
*/
bool
tests_broker_default_state()
{
    string_bus b;

    bool ok = true;

    ok = ok && (b.empty());
    ok = ok && (b.subscriber_count() == 0);
    ok = ok && (b.topic_count() == 0);

    return ok;
}

/*
tests_broker_subscribe_tokens
  Verifies token issuance.
  Tests the following:
  - the first token issued is id 1 (id 0 is reserved for "no subscription")
  - each subsequent registration receives a distinct, increasing id
  - every issued token is valid
*/
bool
tests_broker_subscribe_tokens()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s1 = b.subscribe("a", recorder(&log));
    subscription s2 = b.subscribe("a", recorder(&log));
    subscription s3 = b.subscribe("b", recorder(&log));

    bool ok = true;

    ok = ok && (s1.token().id == 1);       // never 0: that is the null token
    ok = ok && (s2.token().id == 2);
    ok = ok && (s3.token().id == 3);
    ok = ok && (s1.token().valid() && s2.token().valid() && s3.token().valid());
    ok = ok && (s1.token() != s2.token());

    return ok;
}

/*
tests_broker_publish_delivers
  Verifies a publish reaches every live subscriber of the topic.
  Tests the following:
  - both subscribers receive the topic and payload
  - the status is Ok
*/
bool
tests_broker_publish_delivers()
{
    string_bus            b;
    std::vector<delivery> first;
    std::vector<delivery> second;

    subscription s1 = b.subscribe("t", recorder(&first));
    subscription s2 = b.subscribe("t", recorder(&second));

    pubsub_status st = b.publish("t", 21);

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (first.size() == 1 && first[0].topic == "t" && first[0].payload == 21);
    ok = ok && (second.size() == 1 && second[0].payload == 21);

    return ok;
}

/*
tests_broker_publish_order
  Verifies delivery follows registration order.
  Tests the following:
  - subscribers of one topic are invoked oldest-first, into a shared log
*/
bool
tests_broker_publish_order()
{
    string_bus            b;
    std::vector<delivery> log;

    // both handlers append to the same log, tagged by their payload transform
    subscription s1 = b.subscribe("t",
        [&](const std::string& _t, const int& _p) { log.push_back(delivery(_t, _p)); });
    subscription s2 = b.subscribe("t",
        [&](const std::string& _t, const int& _p) { log.push_back(delivery(_t, 100 + _p)); });

    b.publish("t", 5);

    bool ok = true;

    ok = ok && (log.size() == 2);
    ok = ok && (log[0].payload == 5);      // first registered ran first
    ok = ok && (log[1].payload == 105);

    return ok;
}

/*
tests_broker_publish_no_subscribers
  Verifies a publish to a topic nobody subscribes to.
  Tests the following:
  - the status is NoSubscribers
  - no handler runs
*/
bool
tests_broker_publish_no_subscribers()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("known", recorder(&log));

    pubsub_status st = b.publish("unknown", 1);

    bool ok = true;

    ok = ok && (st == DPubSubStatusNoSubscribers);
    ok = ok && (log.empty());

    return ok;
}

/*
tests_broker_publish_topic_isolation
  Verifies a publish reaches only the named topic's subscribers.
  Tests the following:
  - a subscriber of another topic is untouched
*/
bool
tests_broker_publish_topic_isolation()
{
    string_bus            b;
    std::vector<delivery> a_log;
    std::vector<delivery> b_log;

    subscription sa = b.subscribe("a", recorder(&a_log));
    subscription sb = b.subscribe("b", recorder(&b_log));

    b.publish("a", 3);

    bool ok = true;

    ok = ok && (a_log.size() == 1 && a_log[0].payload == 3);
    ok = ok && (b_log.empty());            // "b" saw nothing

    return ok;
}

/*
tests_broker_publish_message
  Verifies the message-envelope publish overload.
  Tests the following:
  - a pubsub_message is routed to its own topic's subscribers
*/
bool
tests_broker_publish_message()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    pubsub_status st = b.publish(pubsub_message<std::string, int>("t", 8));

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (log.size() == 1 && log[0].topic == "t" && log[0].payload == 8);

    return ok;
}

/*
tests_broker_operator_call
  Verifies the call operator aliases publish(topic, payload).
  Tests the following:
  - invoking the broker delivers exactly as publish does
*/
bool
tests_broker_operator_call()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    pubsub_status st = b("t", 4);

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (log.size() == 1 && log[0].payload == 4);

    return ok;
}

/*
tests_broker_unsubscribe_eager
  Verifies eager revocation by token.
  Tests the following:
  - unsubscribe reports that it found the registration
  - the outstanding handle observes the revocation
  - the entry is gone (not merely flagged): the topic bucket is dropped too
  - the handler no longer receives publishes
*/
bool
tests_broker_unsubscribe_eager()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    bool removed = b.unsubscribe(s.token());

    bool ok = true;

    ok = ok && (removed);
    ok = ok && (!s.active());              // the shared flag was flipped
    ok = ok && (b.subscriber_count() == 0);
    ok = ok && (b.topic_count() == 0);     // last entry gone -> bucket dropped

    b.publish("t", 1);
    ok = ok && (log.empty());

    return ok;
}

/*
tests_broker_unsubscribe_absent
  Verifies revoking a token the broker never issued.
  Tests the following:
  - unsubscribe reports false and changes nothing
  - revoking the same token twice reports false the second time
*/
bool
tests_broker_unsubscribe_absent()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    bool ok = (!b.unsubscribe(subscription_token(999)));   // never issued
    ok = ok && (b.subscriber_count() == 1);

    ok = ok && ( b.unsubscribe(s.token()));                // first: found
    ok = ok && (!b.unsubscribe(s.token()));                // second: already gone

    return ok;
}

/*
tests_broker_unsubscribe_lazily_revoked_token
  Verifies the interaction between the two revocation paths. A lazy revoke
  through the handle flips the flag but leaves the entry in place, so an eager
  unsubscribe by the same token still finds it - and only then is the storage
  actually released.
  Tests the following:
  - after a lazy revoke the live counts already read zero
  - unsubscribe(token) nevertheless still reports that it found the entry
  - a second unsubscribe then reports false, the entry now truly being gone
*/
bool
tests_broker_unsubscribe_lazily_revoked_token()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    s.unsubscribe();                       // lazy: flag flipped, entry retained

    bool ok = (b.subscriber_count() == 0); // live counts drop immediately

    ok = ok && ( b.unsubscribe(s.token()));  // the entry is still there to erase
    ok = ok && (!b.unsubscribe(s.token()));  // now it really is gone

    return ok;
}

/*
tests_broker_unsubscribe_topic
  Verifies bulk revocation of one topic.
  Tests the following:
  - unsubscribe_topic reports how many registrations it dropped
  - every handle for that topic observes the revocation
  - other topics are untouched
*/
bool
tests_broker_unsubscribe_topic()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription a1 = b.subscribe("a", recorder(&log));
    subscription a2 = b.subscribe("a", recorder(&log));
    subscription c1 = b.subscribe("c", recorder(&log));

    std::size_t removed = b.unsubscribe_topic("a");

    bool ok = true;

    ok = ok && (removed == 2);
    ok = ok && (!a1.active() && !a2.active());
    ok = ok && (c1.active());              // the other topic survives
    ok = ok && (b.subscriber_count() == 1);
    ok = ok && (b.topic_count() == 1);

    return ok;
}

/*
tests_broker_unsubscribe_topic_absent
  Verifies bulk revocation of a topic nobody subscribes to.
  Tests the following:
  - unsubscribe_topic reports zero and changes nothing
*/
bool
tests_broker_unsubscribe_topic_absent()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("a", recorder(&log));

    std::size_t removed = b.unsubscribe_topic("nonexistent");

    bool ok = true;

    ok = ok && (removed == 0);
    ok = ok && (b.subscriber_count() == 1);
    ok = ok && (s.active());

    return ok;
}

/*
tests_broker_unsubscribe_all
  Verifies bulk revocation across every topic.
  Tests the following:
  - every registration is dropped and every handle observes it
  - the broker is left empty with no topics
*/
bool
tests_broker_unsubscribe_all()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s1 = b.subscribe("a", recorder(&log));
    subscription s2 = b.subscribe("b", recorder(&log));

    b.unsubscribe_all();

    bool ok = true;

    ok = ok && (!s1.active() && !s2.active());
    ok = ok && (b.empty());
    ok = ok && (b.subscriber_count() == 0);
    ok = ok && (b.topic_count() == 0);

    b.publish("a", 1);
    ok = ok && (log.empty());

    return ok;
}

/*
tests_broker_lazy_revoke_skipped
  Verifies publish skips a lazily revoked subscriber.
  Tests the following:
  - the revoked handler is not invoked
  - the surviving handler still receives, and the status is Ok
*/
bool
tests_broker_lazy_revoke_skipped()
{
    string_bus            b;
    std::vector<delivery> dead_log;
    std::vector<delivery> live_log;

    subscription dead = b.subscribe("t", recorder(&dead_log));
    subscription live = b.subscribe("t", recorder(&live_log));

    dead.unsubscribe();                    // lazy revoke

    pubsub_status st = b.publish("t", 6);

    bool ok = true;

    ok = ok && (st == DPubSubStatusOk);
    ok = ok && (dead_log.empty());
    ok = ok && (live_log.size() == 1 && live_log[0].payload == 6);

    return ok;
}

/*
tests_broker_lazy_revoke_all_dead
  Verifies a topic whose subscribers are all lazily revoked. The entries are
  still physically present, but every count reports live entries only.
  Tests the following:
  - the counts report the topic as having no subscribers
  - a publish to it reports NoSubscribers and invokes nobody
*/
bool
tests_broker_lazy_revoke_all_dead()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s1 = b.subscribe("t", recorder(&log));
    subscription s2 = b.subscribe("t", recorder(&log));

    s1.unsubscribe();
    s2.unsubscribe();                      // every entry dead, storage retained

    bool ok = true;

    ok = ok && (b.subscriber_count() == 0);
    ok = ok && (b.subscriber_count("t") == 0);
    ok = ok && (b.topic_count() == 0);     // no topic has a live entry
    ok = ok && (b.empty());
    ok = ok && (!b.has_subscribers("t"));

    pubsub_status st = b.publish("t", 1);

    ok = ok && (st == DPubSubStatusNoSubscribers);
    ok = ok && (log.empty());

    return ok;
}

/*
tests_broker_compact
  Verifies compaction reclaims lazily revoked storage.
  Tests the following:
  - compact reports the number of dead registrations reclaimed
  - the live subscriber survives and still receives publishes
  - compacting again reclaims nothing
*/
bool
tests_broker_compact()
{
    string_bus            b;
    std::vector<delivery> dead_log;
    std::vector<delivery> live_log;

    subscription dead = b.subscribe("t", recorder(&dead_log));
    subscription live = b.subscribe("t", recorder(&live_log));

    dead.unsubscribe();

    std::size_t reclaimed = b.compact();

    bool ok = true;

    ok = ok && (reclaimed == 1);
    ok = ok && (b.compact() == 0);         // nothing left to reclaim
    ok = ok && (b.subscriber_count() == 1);
    ok = ok && (live.active());

    b.publish("t", 2);
    ok = ok && (live_log.size() == 1 && live_log[0].payload == 2);
    ok = ok && (dead_log.empty());

    return ok;
}

/*
tests_broker_subscriber_count
  Verifies the live-subscriber tallies.
  Tests the following:
  - the overall count sums live subscribers across every topic
  - the per-topic count reports only that topic
  - an unknown topic reports zero
*/
bool
tests_broker_subscriber_count()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s1 = b.subscribe("a", recorder(&log));
    subscription s2 = b.subscribe("a", recorder(&log));
    subscription s3 = b.subscribe("b", recorder(&log));

    bool ok = true;

    ok = ok && (b.subscriber_count() == 3);
    ok = ok && (b.subscriber_count("a") == 2);
    ok = ok && (b.subscriber_count("b") == 1);
    ok = ok && (b.subscriber_count("nonexistent") == 0);

    return ok;
}

/*
tests_broker_has_subscribers
  Verifies the per-topic liveness query.
  Tests the following:
  - true for a topic with a live subscriber
  - false for an unknown topic
  - false once that topic's only subscriber is revoked
*/
bool
tests_broker_has_subscribers()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    bool ok = true;

    ok = ok && ( b.has_subscribers("t"));
    ok = ok && (!b.has_subscribers("nonexistent"));

    s.unsubscribe();
    ok = ok && (!b.has_subscribers("t"));

    return ok;
}

/*
tests_broker_topic_count
  Verifies the count of distinct topics with a live subscriber.
  Tests the following:
  - a topic with several subscribers is counted once
  - a topic whose subscribers are all revoked is not counted
*/
bool
tests_broker_topic_count()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription a1 = b.subscribe("a", recorder(&log));
    subscription a2 = b.subscribe("a", recorder(&log));
    subscription c1 = b.subscribe("c", recorder(&log));

    bool ok = (b.topic_count() == 2);      // "a" counted once, plus "c"

    c1.unsubscribe();
    ok = ok && (b.topic_count() == 1);     // "c" has no live subscriber left

    return ok;
}

/*
tests_broker_empty
  Verifies the emptiness predicate.
  Tests the following:
  - a fresh broker is empty
  - a broker with a live subscriber is not
  - revoking every subscriber makes it empty again, even before compaction
*/
bool
tests_broker_empty()
{
    string_bus            b;
    std::vector<delivery> log;

    bool ok = (b.empty());

    subscription s = b.subscribe("t", recorder(&log));
    ok = ok && (!b.empty());

    s.unsubscribe();
    ok = ok && (b.empty());                // live count is zero, storage retained

    return ok;
}

/*
tests_broker_reserve_topics
  Verifies topic pre-allocation is transparent to callers.
  Tests the following:
  - reserving does not change the observable counts
  - subscribe and publish still behave normally afterwards
*/
bool
tests_broker_reserve_topics()
{
    string_bus            b;
    std::vector<delivery> log;

    b.reserve_topics(32);

    bool ok = (b.empty() && b.topic_count() == 0);

    subscription s = b.subscribe("t", recorder(&log));
    b.publish("t", 5);

    ok = ok && (b.subscriber_count() == 1);
    ok = ok && (b.topic_count() == 1);
    ok = ok && (log.size() == 1 && log[0].payload == 5);

    return ok;
}

/*
tests_broker_not_directly_constructible
  Verifies the CRTP base cannot be instantiated on its own - its constructor is
  protected, so only a derived broker may construct it.
  Tests the following:
  - broker<...> is not default-constructible
  - the concrete bus derived from it is
*/
bool
tests_broker_not_directly_constructible()
{
    using base_type = broker<string_bus, std::string, int>;

    static_assert(!std::is_default_constructible<base_type>::value,
                  "the broker base has a protected constructor");
    static_assert(std::is_default_constructible<string_bus>::value,
                  "a concrete bus is constructible");

    return ( !std::is_default_constructible<base_type>::value &&
              std::is_default_constructible<string_bus>::value );
}


NS_END  // testing
NS_END  // djinterp
