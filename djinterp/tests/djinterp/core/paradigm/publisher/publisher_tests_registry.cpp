/*******************************************************************************
* djinterp [core]                                   publisher_tests_registry.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_registry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_registry.cpp
//   The subscriber registry (section V): the shared topic -> subscribers store
// that every broker owns. Tested directly - without a broker wrapped around it -
// by driving add() with hand-made tokens and liveness flags, so the lazy-revoke
// paths (flag flipped behind the registry's back) can be exercised precisely.
//
//   Two structural rules get particular attention: a topic bucket is dropped as
// soon as its last entry goes (whether via remove, remove_topic, or compact),
// and every count reports LIVE entries only - so an all-dead bucket reads as
// zero subscribers and zero topics even while its storage is still present.

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    using handler = std::function<void(const std::string&, const int&)>;

    // flag
    //   helper: a fresh, live shared liveness flag.
    std::shared_ptr<bool>
    flag()
    {
        return std::make_shared<bool>(true);
    }

    // deliver_to
    //   helper: the dispatch function object a broker would supply - invokes
    // each visited handler with the given topic and payload.
    struct deliver_to
    {
        std::string topic;
        int         payload;

        void operator()(handler& _h) const { _h(topic, payload); }
    };
}


/*
tests_registry_default
  Verifies a default-constructed registry.
  Tests the following:
  - it is empty, with no subscribers and no topics
*/
bool
tests_registry_default()
{
    string_registry r;

    bool ok = true;

    ok = ok && (r.empty());
    ok = ok && (r.subscriber_count() == 0);
    ok = ok && (r.topic_count() == 0);

    return ok;
}

/*
tests_registry_add
  Verifies registration, including bucket creation on first use of a topic.
  Tests the following:
  - adding to a new topic creates its bucket
  - adding a second entry to the same topic reuses that bucket
  - counts reflect the registrations
*/
bool
tests_registry_add()
{
    string_registry       r;
    std::vector<delivery> log;

    r.add("a", subscription_token(1), recorder(&log), flag());
    r.add("a", subscription_token(2), recorder(&log), flag());
    r.add("b", subscription_token(3), recorder(&log), flag());

    bool ok = true;

    ok = ok && (r.subscriber_count() == 3);
    ok = ok && (r.subscriber_count("a") == 2);
    ok = ok && (r.subscriber_count("b") == 1);
    ok = ok && (r.topic_count() == 2);      // two distinct buckets
    ok = ok && (!r.empty());

    return ok;
}

/*
tests_registry_dispatch
  Verifies the dispatch loop visits every live subscriber of a topic in
  registration order.
  Tests the following:
  - dispatch returns the number of handlers visited
  - each handler receives the topic and payload the caller supplied
  - handlers are visited oldest-first
*/
bool
tests_registry_dispatch()
{
    string_registry       r;
    std::vector<delivery> first;
    std::vector<delivery> second;

    r.add("a", subscription_token(1), recorder(&first), flag());
    r.add("a", subscription_token(2), recorder(&second), flag());

    std::size_t visited = r.dispatch("a", deliver_to{ "a", 5 });

    bool ok = true;

    ok = ok && (visited == 2);
    ok = ok && (first.size() == 1 && first[0].topic == "a" && first[0].payload == 5);
    ok = ok && (second.size() == 1 && second[0].payload == 5);

    return ok;
}

/*
tests_registry_dispatch_unknown_topic
  Verifies dispatch on a topic with no bucket.
  Tests the following:
  - dispatch reports zero handlers visited and invokes nothing
*/
bool
tests_registry_dispatch_unknown_topic()
{
    string_registry       r;
    std::vector<delivery> log;

    r.add("a", subscription_token(1), recorder(&log), flag());

    std::size_t visited = r.dispatch("nonexistent", deliver_to{ "nonexistent", 1 });

    bool ok = true;

    ok = ok && (visited == 0);
    ok = ok && (log.empty());

    return ok;
}

/*
tests_registry_dispatch_skips_dead
  Verifies dispatch skips deactivated entries.
  Tests the following:
  - flipping an entry's flag excludes it from the visit
  - the surviving entry is still visited
*/
bool
tests_registry_dispatch_skips_dead()
{
    string_registry       r;
    std::vector<delivery> dead_log;
    std::vector<delivery> live_log;

    std::shared_ptr<bool> dead = flag();

    r.add("a", subscription_token(1), recorder(&dead_log), dead);
    r.add("a", subscription_token(2), recorder(&live_log), flag());

    *dead = false;                          // lazy revoke, behind the registry's back

    std::size_t visited = r.dispatch("a", deliver_to{ "a", 3 });

    bool ok = true;

    ok = ok && (visited == 1);
    ok = ok && (dead_log.empty());
    ok = ok && (live_log.size() == 1 && live_log[0].payload == 3);

    return ok;
}

/*
tests_registry_remove
  Verifies eager removal by token.
  Tests the following:
  - remove reports that it found the registration
  - the entry's flag is deactivated so outstanding handles observe it
  - the entry is gone from the counts
*/
bool
tests_registry_remove()
{
    string_registry       r;
    std::vector<delivery> log;

    std::shared_ptr<bool> alive = flag();

    r.add("a", subscription_token(1), recorder(&log), alive);
    r.add("a", subscription_token(2), recorder(&log), flag());

    bool removed = r.remove(subscription_token(1));

    bool ok = true;

    ok = ok && (removed);
    ok = ok && (*alive == false);          // flag deactivated before erasure
    ok = ok && (r.subscriber_count("a") == 1);

    return ok;
}

/*
tests_registry_remove_absent
  Verifies removal of an unknown token.
  Tests the following:
  - remove reports false and changes nothing
*/
bool
tests_registry_remove_absent()
{
    string_registry       r;
    std::vector<delivery> log;

    r.add("a", subscription_token(1), recorder(&log), flag());

    bool removed = r.remove(subscription_token(99));

    bool ok = true;

    ok = ok && (!removed);
    ok = ok && (r.subscriber_count() == 1);

    return ok;
}

/*
tests_registry_remove_drops_empty_bucket
  Verifies a topic bucket is dropped once its last entry is removed.
  Tests the following:
  - removing the sole entry for a topic drops the topic itself
  - a sibling topic is unaffected
*/
bool
tests_registry_remove_drops_empty_bucket()
{
    string_registry       r;
    std::vector<delivery> log;

    r.add("solo",  subscription_token(1), recorder(&log), flag());
    r.add("other", subscription_token(2), recorder(&log), flag());

    bool ok = (r.topic_count() == 2);

    r.remove(subscription_token(1));

    ok = ok && (r.topic_count() == 1);              // "solo" gone entirely
    ok = ok && (!r.has_subscribers("solo"));
    ok = ok && (r.has_subscribers("other"));        // sibling intact

    return ok;
}

/*
tests_registry_remove_topic
  Verifies bulk removal of a topic.
  Tests the following:
  - remove_topic reports how many registrations it dropped
  - every dropped entry's flag is deactivated
  - other topics survive
*/
bool
tests_registry_remove_topic()
{
    string_registry       r;
    std::vector<delivery> log;

    std::shared_ptr<bool> a1 = flag();
    std::shared_ptr<bool> a2 = flag();

    r.add("a", subscription_token(1), recorder(&log), a1);
    r.add("a", subscription_token(2), recorder(&log), a2);
    r.add("b", subscription_token(3), recorder(&log), flag());

    std::size_t removed = r.remove_topic("a");

    bool ok = true;

    ok = ok && (removed == 2);
    ok = ok && (*a1 == false && *a2 == false);      // every flag deactivated
    ok = ok && (r.subscriber_count() == 1);
    ok = ok && (r.topic_count() == 1);
    ok = ok && (r.has_subscribers("b"));

    return ok;
}

/*
tests_registry_remove_topic_absent
  Verifies bulk removal of an unknown topic.
  Tests the following:
  - remove_topic reports zero and changes nothing
*/
bool
tests_registry_remove_topic_absent()
{
    string_registry       r;
    std::vector<delivery> log;

    r.add("a", subscription_token(1), recorder(&log), flag());

    std::size_t removed = r.remove_topic("nonexistent");

    bool ok = true;

    ok = ok && (removed == 0);
    ok = ok && (r.subscriber_count() == 1);

    return ok;
}

/*
tests_registry_clear
  Verifies clear drops every registration across every topic.
  Tests the following:
  - the registry is left empty with no topics
  - every flag is deactivated so outstanding handles observe it
*/
bool
tests_registry_clear()
{
    string_registry       r;
    std::vector<delivery> log;

    std::shared_ptr<bool> a = flag();
    std::shared_ptr<bool> b = flag();

    r.add("a", subscription_token(1), recorder(&log), a);
    r.add("b", subscription_token(2), recorder(&log), b);

    r.clear();

    bool ok = true;

    ok = ok && (r.empty());
    ok = ok && (r.topic_count() == 0);
    ok = ok && (*a == false && *b == false);

    return ok;
}

/*
tests_registry_compact
  Verifies compaction reclaims lazily revoked entries.
  Tests the following:
  - compact reports how many dead registrations it reclaimed
  - the live registration survives and is still dispatched to
*/
bool
tests_registry_compact()
{
    string_registry       r;
    std::vector<delivery> dead_log;
    std::vector<delivery> live_log;

    std::shared_ptr<bool> dead = flag();

    r.add("a", subscription_token(1), recorder(&dead_log), dead);
    r.add("a", subscription_token(2), recorder(&live_log), flag());

    *dead = false;                          // lazy revoke

    std::size_t reclaimed = r.compact();

    bool ok = true;

    ok = ok && (reclaimed == 1);
    ok = ok && (r.subscriber_count("a") == 1);
    ok = ok && (r.topic_count() == 1);       // the topic still has a live entry

    std::size_t visited = r.dispatch("a", deliver_to{ "a", 4 });
    ok = ok && (visited == 1);
    ok = ok && (live_log.size() == 1 && live_log[0].payload == 4);

    return ok;
}

/*
tests_registry_compact_drops_dead_bucket
  Verifies compaction drops a topic whose entries are all dead.
  Tests the following:
  - an all-dead topic already reads as zero subscribers and zero topics
  - compact reclaims both entries and drops the bucket, leaving siblings intact
*/
bool
tests_registry_compact_drops_dead_bucket()
{
    string_registry       r;
    std::vector<delivery> log;

    std::shared_ptr<bool> d1 = flag();
    std::shared_ptr<bool> d2 = flag();

    r.add("dead", subscription_token(1), recorder(&log), d1);
    r.add("dead", subscription_token(2), recorder(&log), d2);
    r.add("live", subscription_token(3), recorder(&log), flag());

    *d1 = false;
    *d2 = false;                            // the whole bucket is dead

    bool ok = true;

    ok = ok && (r.subscriber_count("dead") == 0);   // counts see only live entries
    ok = ok && (r.topic_count() == 1);              // "dead" no longer counts

    std::size_t reclaimed = r.compact();

    ok = ok && (reclaimed == 2);
    ok = ok && (r.subscriber_count() == 1);
    ok = ok && (r.topic_count() == 1);
    ok = ok && (r.has_subscribers("live"));

    return ok;
}

/*
tests_registry_subscriber_count
  Verifies the live-subscriber tallies.
  Tests the following:
  - the overall count sums live entries across every topic
  - the per-topic count reports only that topic
  - an unknown topic reports zero
*/
bool
tests_registry_subscriber_count()
{
    string_registry       r;
    std::vector<delivery> log;

    r.add("a", subscription_token(1), recorder(&log), flag());
    r.add("a", subscription_token(2), recorder(&log), flag());
    r.add("b", subscription_token(3), recorder(&log), flag());

    bool ok = true;

    ok = ok && (r.subscriber_count() == 3);
    ok = ok && (r.subscriber_count("a") == 2);
    ok = ok && (r.subscriber_count("b") == 1);
    ok = ok && (r.subscriber_count("nonexistent") == 0);

    return ok;
}

/*
tests_registry_has_subscribers
  Verifies the per-topic liveness query.
  Tests the following:
  - true for a topic with a live entry
  - false for an unknown topic
  - false once the topic's only entry is lazily revoked
*/
bool
tests_registry_has_subscribers()
{
    string_registry       r;
    std::vector<delivery> log;

    std::shared_ptr<bool> alive = flag();

    r.add("a", subscription_token(1), recorder(&log), alive);

    bool ok = true;

    ok = ok && ( r.has_subscribers("a"));
    ok = ok && (!r.has_subscribers("nonexistent"));

    *alive = false;
    ok = ok && (!r.has_subscribers("a"));   // dead entries do not count

    return ok;
}

/*
tests_registry_topic_count
  Verifies the count of distinct topics holding a live entry.
  Tests the following:
  - each distinct topic with a live entry is counted once
  - a topic whose entries are all dead is not counted
*/
bool
tests_registry_topic_count()
{
    string_registry       r;
    std::vector<delivery> log;

    std::shared_ptr<bool> dead = flag();

    r.add("a", subscription_token(1), recorder(&log), flag());
    r.add("a", subscription_token(2), recorder(&log), flag());   // same topic
    r.add("b", subscription_token(3), recorder(&log), dead);

    bool ok = (r.topic_count() == 2);       // "a" counted once, plus "b"

    *dead = false;
    ok = ok && (r.topic_count() == 1);      // "b" has no live entry left

    return ok;
}

/*
tests_registry_empty
  Verifies the emptiness predicate.
  Tests the following:
  - a fresh registry is empty
  - a registry with a live entry is not
  - lazily revoking every entry makes it empty again, even before compaction
*/
bool
tests_registry_empty()
{
    string_registry       r;
    std::vector<delivery> log;

    bool ok = (r.empty());

    std::shared_ptr<bool> alive = flag();
    r.add("a", subscription_token(1), recorder(&log), alive);
    ok = ok && (!r.empty());

    *alive = false;
    ok = ok && (r.empty());                 // no LIVE entries, though storage remains

    return ok;
}

/*
tests_registry_reserve_topics
  Verifies topic pre-allocation is transparent to callers.
  Tests the following:
  - reserving does not change the observable counts
  - registration and dispatch still behave normally afterwards
*/
bool
tests_registry_reserve_topics()
{
    string_registry       r;
    std::vector<delivery> log;

    r.reserve_topics(32);

    bool ok = (r.empty() && r.topic_count() == 0);

    r.add("a", subscription_token(1), recorder(&log), flag());

    std::size_t visited = r.dispatch("a", deliver_to{ "a", 2 });

    ok = ok && (r.subscriber_count() == 1);
    ok = ok && (visited == 1);
    ok = ok && (log.size() == 1 && log[0].payload == 2);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
