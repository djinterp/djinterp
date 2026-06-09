/******************************************************************************
* djinterp [paradigm]                                               pubsub.hpp
*
* Publish-subscribe paradigm foundation:
*   A publish-subscribe system decouples the senders of messages
* (publishers) from their receivers (subscribers) by routing every
* message through an intermediary - the broker - keyed by a topic.
* Publishers emit a message on a topic without knowing whether, or
* by whom, it is consumed; subscribers register interest in a topic
* without knowing who, if anyone, produces for it.  This is the key
* difference from the observer paradigm, where a subject holds its
* observers directly: here neither side references the other.
*
*   This module provides the common, type-agnostic substrate shared
* by every concrete publish-subscribe variant - synchronous buses,
* queued/asynchronous dispatchers, priority delivery, filtered or
* wildcard routing, retained-value channels, and so on.  The shared
* machinery is factored into three reusable pieces:
*
*     1. subscription        RAII handle for a single registration.
*     2. subscriber_registry topic -> subscribers storage + dispatch.
*     3. broker              CRTP base wiring subscribe/publish around
*                            the registry, with the delivery step left
*                            as a customization point for derived types.
*
*   A derived module customizes only how each message is delivered by
* overriding the public do_publish / do_deliver hooks; everything else
* - token generation, registration, revocation, bookkeeping, and the
* topic-lookup loop - is inherited.  The included bus<> is the
* batteries-included synchronous default and needs no overrides at all.
*
*   The topic type need only be copy-constructible and equality-
* comparable (operator==).  The payload type need only be copyable.
* The handler type need only be invocable as void(topic, payload);
* it defaults to std::function but may be overridden with a raw
* function pointer or any custom functor for tighter control.
*
* Contents:
*   - pubsub_status            delivery outcome codes
*   - subscription_token       opaque per-broker registration id
*   - subscription             RAII handle (lazy revoke)
*   - scoped_subscription      single-handle RAII guard (move-only)
*   - scoped_subscriptions     multi-handle RAII guard
*   - pubsub_message           topic + payload envelope
*   - subscriber_registry      shared topic -> subscribers store
*   - broker                   CRTP base for all broker variants
*   - bus                      synchronous in-process broker
*   - pubsub_has_*, is_broker  conformance traits
*   - broker_type, handler_for C++20 concepts
*   - make_bus                 factory
*
* Usage:
*   // a string-keyed bus carrying integer payloads
*   bus<std::string, int> b;
*
*   // subscribe; keep the returned handle alive to stay subscribed
*   subscription s = b.subscribe("temperature",
*       [](const std::string& _topic, const int& _value)
*       {
*           // react to the value
*       });
*
*   // publish to all current subscribers of the topic
*   b.publish("temperature", 21);
*
*   // revoke explicitly, or let the handle's destructor do it
*   s.unsubscribe();
*
*
* path:      /inc/djinterp/core/paradigm/pubsub/pubsub.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.08
******************************************************************************/

#ifndef DJINTERP_PARADIGM_PUBSUB_
#define DJINTERP_PARADIGM_PUBSUB_ 1

// std
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   STATUS CODES                                        ///
///////////////////////////////////////////////////////////////////////////////

// pubsub_status
//   typedef: classifies the outcome of a publish operation.
typedef std::int32_t pubsub_status;

// DPubSubStatus*
//   constants: standard publish status codes.  Derived brokers may
// define additional codes at or above DPubSubStatusUserBase.
constexpr pubsub_status DPubSubStatusOk             =  0;
constexpr pubsub_status DPubSubStatusNoSubscribers  =  1;
constexpr pubsub_status DPubSubStatusTopicNotFound  =  2;
constexpr pubsub_status DPubSubStatusTokenNotFound  =  3;
constexpr pubsub_status DPubSubStatusRejected       =  4;
constexpr pubsub_status DPubSubStatusUserBase       = 64;


///////////////////////////////////////////////////////////////////////////////
///                II.  SUBSCRIPTION TOKEN                                  ///
///////////////////////////////////////////////////////////////////////////////

// subscription_token
//   struct: an opaque, copyable identifier for a single registration.
// Tokens are unique within the broker that issued them.  The id 0 is
// reserved to mean "no subscription" (a default-constructed token).
struct subscription_token
{
    using id_type = std::uint64_t;

    id_type id;

    // subscription_token (default)
    //   constructs a null token (id == 0).
    subscription_token() D_NOEXCEPT
        : id(0)
    {}

    // subscription_token (id)
    //   constructs a token wrapping an explicit id.
    explicit subscription_token(
        id_type _id
    ) D_NOEXCEPT
        : id(_id)
    {}

    // valid
    //   returns true if this token refers to a real registration.
    D_NODISCARD
    bool
    valid() const D_NOEXCEPT
    {
        return (id != 0);
    }

    // operator==
    //   returns true if both tokens carry the same id.
    friend bool
    operator==(
        const subscription_token& _a,
        const subscription_token& _b
    ) D_NOEXCEPT
    {
        return (_a.id == _b.id);
    }

    // operator!=
    //   returns true if the tokens differ.
    friend bool
    operator!=(
        const subscription_token& _a,
        const subscription_token& _b
    ) D_NOEXCEPT
    {
        return !(_a == _b);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. SUBSCRIPTION HANDLES                                ///
///////////////////////////////////////////////////////////////////////////////
//
//   A subscription owns a shared "alive" flag that is also held by the
//   matching entry in the broker's registry.  Marking the flag false
//   deactivates the registration: the broker skips it on the next
//   publish and reclaims its storage on compact().  Because the flag is
//   shared (not a back-pointer to the broker), a subscription never
//   dangles - it remains safe to query and revoke whether the broker is
//   destroyed first or the handle is.
//
//   This is a lazy revoke: the registry entry is removed opportunistically
//   rather than immediately.  Callers that hold the broker and want eager
//   removal can instead call broker::unsubscribe(token).
//

// subscription
//   class: a movable, copyable handle to one registration.  Copies
// share the same underlying flag, so revoking through any copy
// deactivates the registration for all of them.
class subscription
{
public:
    using token_type = subscription_token;

    // subscription (default)
    //   constructs an inactive handle bound to no registration.
    subscription() D_NOEXCEPT
        : m_alive(),
          m_token()
    {}

    // subscription (flag + token)
    //   constructs a handle from a shared flag and its token.  Used by
    // broker::subscribe; not normally called directly.
    subscription(
        std::shared_ptr<bool> _alive,
        token_type            _token
    ) D_NOEXCEPT
        : m_alive(std::move(_alive)),
          m_token(_token)
    {}

    // active
    //   returns true if the registration is still live.
    D_NODISCARD
    bool
    active() const D_NOEXCEPT
    {
        return (m_alive && *m_alive);
    }

    // token
    //   returns the token identifying this registration.
    D_NODISCARD
    const token_type&
    token() const D_NOEXCEPT
    {
        return m_token;
    }

    // unsubscribe
    //   deactivates the registration.  The broker will skip it on the
    // next publish and may reclaim its storage on compact().  Safe to
    // call more than once and safe after the broker is destroyed.
    void
    unsubscribe() D_NOEXCEPT
    {
        // flip the shared flag if we still hold one.
        if (m_alive)
        {
            *m_alive = false;
        }

        return;
    }

    // release
    //   detaches this handle from the registration without deactivating
    // it.  The registration stays live, but this handle can no longer
    // observe or revoke it.
    void
    release() D_NOEXCEPT
    {
        m_alive.reset();

        return;
    }

private:
    std::shared_ptr<bool> m_alive;
    token_type            m_token;
};


// scoped_subscription
//   class: RAII guard that revokes its subscription on destruction.
// Move-only - moving transfers the obligation to revoke.
class scoped_subscription
{
public:
    // scoped_subscription (default)
    //   constructs an empty guard.
    scoped_subscription() D_NOEXCEPT
        : m_sub()
    {}

    // scoped_subscription (adopt)
    //   adopts a subscription, taking responsibility for revoking it.
    // Implicit by design, so a subscription can be assigned directly to
    // a scoped guard.
    scoped_subscription(
        subscription _sub
    ) D_NOEXCEPT
        : m_sub(std::move(_sub))
    {}

    // ~scoped_subscription
    //   revokes the held subscription.
    ~scoped_subscription()
    {
        m_sub.unsubscribe();
    }

    // scoped_subscription (move)
    //   transfers the revoke obligation from another guard.
    scoped_subscription(
        scoped_subscription&& _other
    ) D_NOEXCEPT
        : m_sub(std::move(_other.m_sub))
    {
        // the moved-from guard must no longer revoke anything.
        _other.m_sub.release();
    }

    // operator= (move)
    //   revokes any current subscription, then adopts _other's.
    scoped_subscription&
    operator=(
        scoped_subscription&& _other
    ) D_NOEXCEPT
    {
        // guard against self-move.
        if (this != &_other)
        {
            m_sub.unsubscribe();
            m_sub = std::move(_other.m_sub);
            _other.m_sub.release();
        }

        return *this;
    }

    // no copying - a registration may be revoked by exactly one guard.
    scoped_subscription(const scoped_subscription&)            = delete;
    scoped_subscription& operator=(const scoped_subscription&) = delete;

    // active
    //   returns true if the held subscription is still live.
    D_NODISCARD
    bool
    active() const D_NOEXCEPT
    {
        return m_sub.active();
    }

    // unsubscribe
    //   revokes the held subscription early.
    void
    unsubscribe() D_NOEXCEPT
    {
        m_sub.unsubscribe();

        return;
    }

    // release
    //   surrenders the revoke obligation and returns the underlying
    // subscription.  The registration stays live; this guard will no
    // longer revoke it on destruction.
    D_NODISCARD
    subscription
    release() D_NOEXCEPT
    {
        subscription out = std::move(m_sub);
        m_sub.release();

        return out;
    }

private:
    subscription m_sub;
};


// scoped_subscriptions
//   class: holds any number of subscriptions and revokes them all on
// destruction.  Useful when one owner manages many registrations.
class scoped_subscriptions
{
public:
    using size_type = std::size_t;

    // scoped_subscriptions (default)
    //   constructs an empty collection.
    scoped_subscriptions()
        : m_subs()
    {}

    // ~scoped_subscriptions
    //   revokes every held subscription.
    ~scoped_subscriptions()
    {
        unsubscribe_all();
    }

    // add
    //   adds a subscription to the collection.
    void
    add(
        subscription _sub
    )
    {
        m_subs.push_back(std::move(_sub));

        return;
    }

    // operator+=
    //   shorthand for add().
    scoped_subscriptions&
    operator+=(
        subscription _sub
    )
    {
        m_subs.push_back(std::move(_sub));

        return *this;
    }

    // unsubscribe_all
    //   revokes every held subscription and empties the collection.
    void
    unsubscribe_all() D_NOEXCEPT
    {
        // revoke each registration in turn.
        for (subscription& s : m_subs)
        {
            s.unsubscribe();
        }

        m_subs.clear();

        return;
    }

    // size
    //   returns the number of held subscriptions.
    D_NODISCARD
    size_type
    size() const D_NOEXCEPT
    {
        return m_subs.size();
    }

    // empty
    //   returns true if the collection holds no subscriptions.
    D_NODISCARD
    bool
    empty() const D_NOEXCEPT
    {
        return m_subs.empty();
    }

private:
    std::vector<subscription> m_subs;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  MESSAGE ENVELOPE                                    ///
///////////////////////////////////////////////////////////////////////////////

// pubsub_message
//   struct: a self-describing message pairing a topic with its payload.
// Convenient for queued or asynchronous brokers that need to store
// messages between publication and delivery.
template<typename _Topic,
         typename _Payload>
struct pubsub_message
{
    using topic_type   = _Topic;
    using payload_type = _Payload;

    _Topic   topic;
    _Payload payload;

    // pubsub_message (default)
    //   constructs a value-initialized message.
    pubsub_message()
        : topic  (),
          payload()
    {}

    // pubsub_message (copy fields)
    //   constructs a message from a topic and payload.
    pubsub_message(
        const _Topic&   _topic,
        const _Payload& _payload
    )
        : topic  (_topic),
          payload(_payload)
    {}

    // pubsub_message (move fields)
    //   constructs a message by moving a topic and payload.
    pubsub_message(
        _Topic&&   _topic,
        _Payload&& _payload
    )
        : topic  (std::move(_topic)),
          payload(std::move(_payload))
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                V.   SUBSCRIBER REGISTRY                                 ///
///////////////////////////////////////////////////////////////////////////////

// subscriber_registry
//   class: the shared storage core of every broker.  Maps each topic to
// an ordered list of (token, handler, alive-flag) entries and provides
// registration, revocation, bookkeeping, and a generic dispatch loop.
//
//   Topics are stored in insertion order and matched with operator==,
// so the topic type need only be copy-constructible and equality-
// comparable; no ordering or hashing is required.  The registry is
// payload-agnostic: it knows how to find subscribers for a topic and
// hand each live handler to a caller-supplied function, but it has no
// notion of how delivery is performed.  That keeps it reusable across
// every delivery policy a derived broker might implement.
template<typename _Topic,
         typename _Handler>
class subscriber_registry
{
public:
    using topic_type   = _Topic;
    using handler_type = _Handler;
    using token_type   = subscription_token;
    using size_type    = std::size_t;

private:
    // entry
    //   struct: a single subscriber registration within one topic.
    struct entry
    {
        token_type            token;
        handler_type          handler;
        std::shared_ptr<bool> alive;

        entry(
            const token_type&     _token,
            handler_type          _handler,
            std::shared_ptr<bool> _alive
        )
            : token  (_token),
              handler(std::move(_handler)),
              alive  (std::move(_alive))
        {}
    };

    // bucket
    //   struct: all subscribers registered against one topic value.
    struct bucket
    {
        topic_type         topic;
        std::vector<entry> entries;

        explicit bucket(
            const topic_type& _topic
        )
            : topic  (_topic),
              entries()
        {}
    };

public:
    // subscriber_registry (default)
    //   constructs an empty registry.
    subscriber_registry()
        : m_buckets()
    {}

    // --------------------------------------------------------
    //  registration
    // --------------------------------------------------------

    // add
    //   registers a handler under _topic, identified by _token and
    // governed by the shared flag _alive.  A new topic bucket is created
    // on first use.
    void
    add(
        const topic_type&     _topic,
        const token_type&     _token,
        handler_type          _handler,
        std::shared_ptr<bool> _alive
    )
    {
        bucket* b = find_bucket(_topic);

        // create the bucket the first time a topic is seen.
        if (!b)
        {
            m_buckets.emplace_back(_topic);
            b = &m_buckets.back();
        }

        b->entries.emplace_back(_token,
                                std::move(_handler),
                                std::move(_alive));

        return;
    }

    // remove
    //   eagerly removes the registration identified by _token, marking
    // its flag inactive first so any outstanding handle observes the
    // change.  Returns true if a matching registration was found.
    bool
    remove(
        const token_type& _token
    )
    {
        // scan every bucket for the matching token.
        for (size_type b = 0; b < m_buckets.size(); ++b)
        {
            std::vector<entry>& es = m_buckets[b].entries;

            for (auto it = es.begin(); it != es.end(); ++it)
            {
                if (it->token == _token)
                {
                    // deactivate, then erase the entry.
                    if (it->alive)
                    {
                        *it->alive = false;
                    }

                    es.erase(it);

                    // drop the bucket once its last entry is gone.
                    if (es.empty())
                    {
                        m_buckets.erase(m_buckets.begin() + b);
                    }

                    return true;
                }
            }
        }

        return false;
    }

    // remove_topic
    //   eagerly removes every registration for _topic, deactivating each
    // flag.  Returns the number of registrations removed.
    size_type
    remove_topic(
        const topic_type& _topic
    )
    {
        size_type removed = 0;

        // find and drop the whole bucket for this topic.
        for (size_type b = 0; b < m_buckets.size(); ++b)
        {
            if (m_buckets[b].topic == _topic)
            {
                std::vector<entry>& es = m_buckets[b].entries;

                // deactivate each flag before discarding the bucket.
                for (entry& e : es)
                {
                    if (e.alive)
                    {
                        *e.alive = false;
                    }
                }

                removed = es.size();
                m_buckets.erase(m_buckets.begin() + b);

                return removed;
            }
        }

        return removed;
    }

    // clear
    //   removes every registration in the registry, deactivating all
    // flags so outstanding handles observe the change.
    void
    clear()
    {
        // deactivate every flag, across every topic.
        for (bucket& b : m_buckets)
        {
            for (entry& e : b.entries)
            {
                if (e.alive)
                {
                    *e.alive = false;
                }
            }
        }

        m_buckets.clear();

        return;
    }

    // compact
    //   reclaims storage for deactivated (lazily revoked) registrations
    // and drops any topic left with no live subscribers.  Returns the
    // number of dead registrations reclaimed.
    size_type
    compact()
    {
        size_type reclaimed = 0;

        // walk buckets back-to-front so erasures don't shift the cursor.
        for (size_type i = m_buckets.size(); i-- > 0; )
        {
            std::vector<entry>& es = m_buckets[i].entries;

            const size_type before = es.size();

            // erase entries whose flag is dead.
            es.erase(
                std::remove_if(
                    es.begin(),
                    es.end(),
                    [](const entry& _e)
                    {
                        return !(_e.alive && *_e.alive);
                    }),
                es.end());

            reclaimed += (before - es.size());

            // drop the bucket if it is now empty.
            if (es.empty())
            {
                m_buckets.erase(m_buckets.begin() + i);
            }
        }

        return reclaimed;
    }

    // --------------------------------------------------------
    //  dispatch
    // --------------------------------------------------------

    // dispatch
    //   invokes _fn(handler) for each live subscriber of _topic, in
    // registration order, skipping deactivated entries.  Returns the
    // number of handlers visited.  The registry performs no delivery
    // itself - the supplied function decides what "deliver" means.
    template<typename _Fn>
    size_type
    dispatch(
        const topic_type& _topic,
        _Fn               _fn
    )
    {
        bucket* b = find_bucket(_topic);

        // an unknown topic has no subscribers.
        if (!b)
        {
            return 0;
        }

        size_type delivered = 0;

        // visit each live handler in order.
        for (entry& e : b->entries)
        {
            if (e.alive && *e.alive)
            {
                _fn(e.handler);
                ++delivered;
            }
        }

        return delivered;
    }

    // --------------------------------------------------------
    //  bookkeeping
    // --------------------------------------------------------

    // subscriber_count
    //   returns the number of live subscribers across all topics.
    D_NODISCARD
    size_type
    subscriber_count() const D_NOEXCEPT
    {
        size_type n = 0;

        // tally live entries in every bucket.
        for (const bucket& b : m_buckets)
        {
            for (const entry& e : b.entries)
            {
                if (e.alive && *e.alive)
                {
                    ++n;
                }
            }
        }

        return n;
    }

    // subscriber_count (topic)
    //   returns the number of live subscribers for _topic.
    D_NODISCARD
    size_type
    subscriber_count(
        const topic_type& _topic
    ) const
    {
        const bucket* b = find_bucket(_topic);

        // an unknown topic has no subscribers.
        if (!b)
        {
            return 0;
        }

        size_type n = 0;

        // tally live entries in the topic's bucket.
        for (const entry& e : b->entries)
        {
            if (e.alive && *e.alive)
            {
                ++n;
            }
        }

        return n;
    }

    // has_subscribers
    //   returns true if _topic has at least one live subscriber.
    D_NODISCARD
    bool
    has_subscribers(
        const topic_type& _topic
    ) const
    {
        return (subscriber_count(_topic) > 0);
    }

    // topic_count
    //   returns the number of distinct topics that currently have at
    // least one live subscriber.
    D_NODISCARD
    size_type
    topic_count() const D_NOEXCEPT
    {
        size_type n = 0;

        // count buckets holding any live entry.
        for (const bucket& b : m_buckets)
        {
            for (const entry& e : b.entries)
            {
                if (e.alive && *e.alive)
                {
                    ++n;

                    break;
                }
            }
        }

        return n;
    }

    // empty
    //   returns true if the registry holds no live subscribers.
    D_NODISCARD
    bool
    empty() const D_NOEXCEPT
    {
        return (subscriber_count() == 0);
    }

    // reserve_topics
    //   pre-allocates storage for _count distinct topics.
    void
    reserve_topics(
        size_type _count
    )
    {
        m_buckets.reserve(_count);

        return;
    }

private:
    // find_bucket
    //   returns the bucket for _topic, or nullptr if none exists.
    D_NODISCARD
    bucket*
    find_bucket(
        const topic_type& _topic
    )
    {
        // linear search by topic equality.
        for (bucket& b : m_buckets)
        {
            if (b.topic == _topic)
            {
                return &b;
            }
        }

        return nullptr;
    }

    // find_bucket (const)
    //   const overload of find_bucket.
    D_NODISCARD
    const bucket*
    find_bucket(
        const topic_type& _topic
    ) const
    {
        // linear search by topic equality.
        for (const bucket& b : m_buckets)
        {
            if (b.topic == _topic)
            {
                return &b;
            }
        }

        return nullptr;
    }

    std::vector<bucket> m_buckets;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  BROKER (CRTP BASE)                                  ///
///////////////////////////////////////////////////////////////////////////////

// broker
//   class: generic CRTP base for every publish-subscribe broker.  Owns
// the subscriber registry and implements the full subscribe / publish /
// unsubscribe surface, leaving only the delivery policy to the derived
// type.
//
//   The type triad (_Topic, _Payload, _Handler) is supplied as template
// parameters; the derived class inherits the resulting typedefs rather
// than redeclaring them.  The handler defaults to a std::function
// invocable as void(topic, payload) but may be any invocable type.
//
//   CUSTOMIZATION POINTS:
//   A derived broker changes behaviour by hiding one of the public do_*
// hooks below.  They are public, not protected, so that this base can
// dispatch to a derived override through the CRTP cast; calling them
// directly is not intended.
//
//     - should_publish(topic, payload) -> bool
//         cheap veto consulted before any delivery.  Default: true.
//     - do_deliver(handler, topic, payload) -> void
//         delivers one message to one handler.  Default: invoke the
//         handler synchronously.  Override for queuing, error
//         isolation, transformation, and the like.
//     - do_publish(topic, payload) -> pubsub_status
//         routes one message to its subscribers.  Default: consult
//         should_publish, then dispatch every live subscriber through
//         do_deliver.  Override to change routing wholesale (priority
//         ordering, wildcard matching, retained replay, and so on).
//
//   A derived type that overrides nothing - as bus<> does - is a
// synchronous, immediate, single-threaded broker.
//
//   RE-ENTRANCY: the default routing does not snapshot the subscriber
// list, so a handler that subscribes to or unsubscribes from the same
// broker mid-delivery invokes undefined behaviour under the default
// policy.  Derived policies that require re-entrant mutation should
// override do_publish to dispatch against a snapshot.
template<typename _Derived,
         typename _Topic,
         typename _Payload,
         typename _Handler = std::function<void(const _Topic&, const _Payload&)>>
class broker
{
public:
    using derived_type      = _Derived;
    using topic_type        = _Topic;
    using payload_type      = _Payload;
    using handler_type      = _Handler;
    using token_type        = subscription_token;
    using subscription_type = subscription;
    using message_type      = pubsub_message<_Topic, _Payload>;
    using registry_type     = subscriber_registry<_Topic, _Handler>;
    using status_type       = pubsub_status;
    using size_type         = std::size_t;

    // --------------------------------------------------------
    //  subscription surface
    // --------------------------------------------------------

    // subscribe
    //   registers _handler against _topic and returns a subscription
    // handle.  The registration stays live until the handle (or any
    // copy of it) revokes it, or until unsubscribe / unsubscribe_topic /
    // unsubscribe_all is called on the broker.
    D_NODISCARD
    subscription
    subscribe(
        const topic_type& _topic,
        handler_type      _handler
    )
    {
        // issue a fresh token and a shared liveness flag.
        token_type            tok(m_next_id++);
        std::shared_ptr<bool> alive = std::make_shared<bool>(true);

        m_registry.add(_topic, tok, std::move(_handler), alive);

        return subscription(std::move(alive), tok);
    }

    // unsubscribe
    //   eagerly revokes the registration identified by _token, removing
    // it from the registry.  Returns true if it was found.
    bool
    unsubscribe(
        const token_type& _token
    )
    {
        return m_registry.remove(_token);
    }

    // unsubscribe_topic
    //   eagerly revokes every registration for _topic.  Returns the
    // number removed.
    size_type
    unsubscribe_topic(
        const topic_type& _topic
    )
    {
        return m_registry.remove_topic(_topic);
    }

    // unsubscribe_all
    //   eagerly revokes every registration across all topics.
    void
    unsubscribe_all()
    {
        m_registry.clear();

        return;
    }

    // --------------------------------------------------------
    //  publication surface
    // --------------------------------------------------------

    // publish
    //   routes _payload to every live subscriber of _topic and returns
    // the delivery status, delegating to the derived do_publish.
    status_type
    publish(
        const topic_type&   _topic,
        const payload_type& _payload
    )
    {
        return self().do_publish(_topic, _payload);
    }

    // publish (message)
    //   routes a self-describing message to its topic's subscribers.
    status_type
    publish(
        const message_type& _message
    )
    {
        return self().do_publish(_message.topic, _message.payload);
    }

    // operator()
    //   alias for publish(topic, payload).
    status_type
    operator()(
        const topic_type&   _topic,
        const payload_type& _payload
    )
    {
        return self().do_publish(_topic, _payload);
    }

    // --------------------------------------------------------
    //  customization points (default implementations)
    // --------------------------------------------------------

    // should_publish
    //   hook: cheap veto consulted at the top of the default do_publish.
    // Returns true to allow delivery.  Override to gate publication on
    // topic, payload, or broker state.
    D_NODISCARD
    bool
    should_publish(
        const topic_type&   /*_topic*/,
        const payload_type& /*_payload*/
    )
    {
        return true;
    }

    // do_deliver
    //   hook: delivers one message to one handler.  The default invokes
    // the handler synchronously.  Override to queue, isolate exceptions,
    // transform, or otherwise reshape per-handler delivery.
    void
    do_deliver(
        handler_type&       _handler,
        const topic_type&   _topic,
        const payload_type& _payload
    )
    {
        _handler(_topic, _payload);

        return;
    }

    // do_publish
    //   hook: routes one message to the subscribers of _topic.  The
    // default consults should_publish, then dispatches every live
    // subscriber through do_deliver, reporting whether anyone received
    // the message.  Override to change routing wholesale.
    status_type
    do_publish(
        const topic_type&   _topic,
        const payload_type& _payload
    )
    {
        // honour the veto hook before touching the registry.
        if (!self().should_publish(_topic, _payload))
        {
            return DPubSubStatusRejected;
        }

        // hand each live subscriber to the (possibly overridden) deliverer.
        const size_type delivered = m_registry.dispatch(
            _topic,
            [&](handler_type& _h)
            {
                self().do_deliver(_h, _topic, _payload);
            });

        return (delivered > 0)
               ? DPubSubStatusOk
               : DPubSubStatusNoSubscribers;
    }

    // --------------------------------------------------------
    //  bookkeeping
    // --------------------------------------------------------

    // subscriber_count
    //   returns the number of live subscribers across all topics.
    D_NODISCARD
    size_type
    subscriber_count() const D_NOEXCEPT
    {
        return m_registry.subscriber_count();
    }

    // subscriber_count (topic)
    //   returns the number of live subscribers for _topic.
    D_NODISCARD
    size_type
    subscriber_count(
        const topic_type& _topic
    ) const
    {
        return m_registry.subscriber_count(_topic);
    }

    // has_subscribers
    //   returns true if _topic has at least one live subscriber.
    D_NODISCARD
    bool
    has_subscribers(
        const topic_type& _topic
    ) const
    {
        return m_registry.has_subscribers(_topic);
    }

    // topic_count
    //   returns the number of distinct topics with live subscribers.
    D_NODISCARD
    size_type
    topic_count() const D_NOEXCEPT
    {
        return m_registry.topic_count();
    }

    // empty
    //   returns true if the broker has no live subscribers.
    D_NODISCARD
    bool
    empty() const D_NOEXCEPT
    {
        return m_registry.empty();
    }

    // compact
    //   reclaims storage for lazily revoked registrations.  Returns the
    // number of dead registrations reclaimed.
    size_type
    compact()
    {
        return m_registry.compact();
    }

    // reserve_topics
    //   pre-allocates storage for _count distinct topics.
    void
    reserve_topics(
        size_type _count
    )
    {
        m_registry.reserve_topics(_count);

        return;
    }

protected:
    // broker (default)
    //   constructs an empty broker.  Protected so that broker cannot be
    // instantiated directly; only derived types may construct it.
    broker()
        : m_registry(),
          m_next_id (1)
    {}

    // ~broker
    //   protected, non-virtual: brokers are used through their concrete
    // (derived) type, not through broker* base pointers.
    ~broker() = default;

    // registry
    //   accessor for derived overrides that need direct registry access.
    D_NODISCARD
    registry_type&
    registry() D_NOEXCEPT
    {
        return m_registry;
    }

    // registry (const)
    //   const overload of registry().
    D_NODISCARD
    const registry_type&
    registry() const D_NOEXCEPT
    {
        return m_registry;
    }

private:
    // self
    //   CRTP cast helper: this object viewed as the derived type.
    D_NODISCARD
    derived_type&
    self() D_NOEXCEPT
    {
        return *static_cast<derived_type*>(this);
    }

    // self (const)
    //   const overload of self().
    D_NODISCARD
    const derived_type&
    self() const D_NOEXCEPT
    {
        return *static_cast<const derived_type*>(this);
    }

    registry_type       m_registry;
    token_type::id_type m_next_id;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. BUS (DEFAULT CONCRETE BROKER)                       ///
///////////////////////////////////////////////////////////////////////////////

// bus
//   class: the batteries-included broker.  Synchronous, immediate,
// single-threaded, in-process delivery with unlimited subscribers per
// topic.  Inherits every customization point's default, so it needs no
// overrides of its own; it exists to give callers an immediately usable
// publish-subscribe object and to serve as the canonical example of a
// concrete broker.
template<typename _Topic,
         typename _Payload,
         typename _Handler = std::function<void(const _Topic&, const _Payload&)>>
class bus
    : public broker<bus<_Topic, _Payload, _Handler>,
                    _Topic,
                    _Payload,
                    _Handler>
{
public:
    using base_type = broker<bus<_Topic, _Payload, _Handler>,
                             _Topic,
                             _Payload,
                             _Handler>;

    using topic_type        = typename base_type::topic_type;
    using payload_type      = typename base_type::payload_type;
    using handler_type      = typename base_type::handler_type;
    using token_type        = typename base_type::token_type;
    using subscription_type = typename base_type::subscription_type;
    using message_type      = typename base_type::message_type;
    using status_type       = typename base_type::status_type;
    using size_type         = typename base_type::size_type;

    // bus (default)
    //   constructs an empty synchronous bus.
    bus()
        : base_type()
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                VIII. CONFORMANCE TRAITS                                 ///
///////////////////////////////////////////////////////////////////////////////
//
//   These traits live in djinterp:: (top-level), matching the flat-
//   namespace convention used throughout the paradigm headers, and are
//   defined here so generic code can discover broker capabilities at
//   compile time without coupling to a concrete broker type.
//

// pubsub_has_subscribe
//   trait: detects a callable `subscribe(topic_type, handler_type)`.
// Primary template is std::false_type; the specialization succeeds when
// the expression is well-formed.
template<typename _Type,
         typename = void>
struct pubsub_has_subscribe : std::false_type
{};

template<typename _Type>
struct pubsub_has_subscribe<_Type, std::void_t<
    decltype(
        std::declval<_Type&>().subscribe(
            std::declval<const typename _Type::topic_type&>(),
            std::declval<typename _Type::handler_type>()))
    >> : std::true_type
{};


// pubsub_has_publish
//   trait: detects a callable `publish(topic_type, payload_type)`.
template<typename _Type,
         typename = void>
struct pubsub_has_publish : std::false_type
{};

template<typename _Type>
struct pubsub_has_publish<_Type, std::void_t<
    decltype(
        std::declval<_Type&>().publish(
            std::declval<const typename _Type::topic_type&>(),
            std::declval<const typename _Type::payload_type&>()))
    >> : std::true_type
{};


// pubsub_has_unsubscribe
//   trait: detects a callable `unsubscribe(token_type)`.
template<typename _Type,
         typename = void>
struct pubsub_has_unsubscribe : std::false_type
{};

template<typename _Type>
struct pubsub_has_unsubscribe<_Type, std::void_t<
    decltype(
        std::declval<_Type&>().unsubscribe(
            std::declval<const typename _Type::token_type&>()))
    >> : std::true_type
{};


// is_broker
//   trait: composite - true iff _Type satisfies the broker protocol
// (subscribe, publish, and unsubscribe).
template<typename _Type>
struct is_broker
{
    static constexpr bool value =
        ( pubsub_has_subscribe  <_Type>::value &&
          pubsub_has_publish    <_Type>::value &&
          pubsub_has_unsubscribe<_Type>::value );
};

// is_broker_v
//   value: convenience alias for is_broker<_Type>::value.
template<typename _Type>
constexpr bool is_broker_v = is_broker<_Type>::value;


// is_subscription
//   trait: detects the subscription-handle protocol (active + token).
template<typename _Type,
         typename = void>
struct is_subscription : std::false_type
{};

template<typename _Type>
struct is_subscription<_Type, std::void_t<
    decltype(static_cast<bool>(std::declval<const _Type&>().active())),
    decltype(std::declval<_Type&>().unsubscribe())
    >> : std::true_type
{};

// is_subscription_v
//   value: convenience alias for is_subscription<_Type>::value.
template<typename _Type>
constexpr bool is_subscription_v = is_subscription<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                IX.  C++20 CONCEPTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )

// broker_type
//   concept: constrains types that satisfy the broker protocol - the
// subscribe, publish, and unsubscribe member functions.
template<typename _Type>
concept broker_type = is_broker<_Type>::value;

// handler_for
//   concept: constrains callables usable as a subscriber for the given
// topic and payload types, i.e. invocable as void(topic, payload).
template<typename _Handler,
         typename _Topic,
         typename _Payload>
concept handler_for = requires(_Handler           _h,
                               const _Topic&      _t,
                               const _Payload&    _p)
{
    _h(_t, _p);
};

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                X.   FACTORIES                                           ///
///////////////////////////////////////////////////////////////////////////////

// make_bus
//   factory: creates a default synchronous bus for the given topic and
// payload types.
template<typename _Topic,
         typename _Payload,
         typename _Handler = std::function<void(const _Topic&, const _Payload&)>>
D_NODISCARD
bus<_Topic, _Payload, _Handler>
make_bus()
{
    return bus<_Topic, _Payload, _Handler>();
}

// make_message
//   factory: creates a message envelope from a topic and payload.
template<typename _Topic,
         typename _Payload>
D_NODISCARD
pubsub_message<_Topic, _Payload>
make_message(
    const _Topic&   _topic,
    const _Payload& _payload
)
{
    return pubsub_message<_Topic, _Payload>(_topic, _payload);
}


NS_END  // djinterp


#endif  // DJINTERP_PARADIGM_PUBSUB_
