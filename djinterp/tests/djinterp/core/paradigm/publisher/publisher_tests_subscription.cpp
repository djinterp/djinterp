/*******************************************************************************
* djinterp [core]                               publisher_tests_subscription.cpp
*
*
* path:      /tests/djinterp/core/paradigm/publisher/publisher_tests_subscription.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : publisher_tests_subscription.cpp
//   The subscription handles (section III). The crux here is the revocation
// model: subscription::unsubscribe() is a LAZY revoke (flips the shared flag;
// the entry lingers until compact()), subscription::release() detaches WITHOUT
// revoking (the registration keeps receiving), copies share one flag, and a
// handle never dangles because the flag is shared rather than a back-pointer to
// the broker. scoped_subscription adds move-only RAII on top;
// scoped_subscriptions does the same for a whole group.

// std
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "publisher_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // move_assign
    //   helper: performs a move assignment through two references. Calling it
    // with the same object twice exercises scoped_subscription's run-time
    // self-move guard without the aliasing being visible at the call site.
    void
    move_assign(
        scoped_subscription& _lhs,
        scoped_subscription& _rhs
    )
    {
        _lhs = std::move(_rhs);

        return;
    }
}


/*
tests_subscription_default
  Verifies a default-constructed subscription.
  Tests the following:
  - it is inactive and carries the null token
*/
bool
tests_subscription_default()
{
    subscription s;

    bool ok = true;

    ok = ok && (!s.active());
    ok = ok && (!s.token().valid());

    return ok;
}

/*
tests_subscription_active
  Verifies a handle returned by subscribe is live.
  Tests the following:
  - the fresh handle reports active
*/
bool
tests_subscription_active()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    return s.active();
}

/*
tests_subscription_token_accessor
  Verifies the handle carries its registration token.
  Tests the following:
  - the handle's token is valid
  - the broker can revoke by that token, proving it identifies the registration
*/
bool
tests_subscription_token_accessor()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    bool ok = (s.token().valid());

    ok = ok && (b.unsubscribe(s.token()));   // the token really names the entry

    return ok;
}

/*
tests_subscription_unsubscribe
  Verifies the lazy revoke through a handle.
  Tests the following:
  - the handle reports inactive afterwards
  - the broker no longer counts the subscriber as live
*/
bool
tests_subscription_unsubscribe()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    s.unsubscribe();

    bool ok = true;

    ok = ok && (!s.active());
    ok = ok && (b.subscriber_count() == 0);
    ok = ok && (b.subscriber_count("t") == 0);

    return ok;
}

/*
tests_subscription_unsubscribe_idempotent
  Verifies repeated revocation is safe.
  Tests the following:
  - calling unsubscribe twice leaves the handle inactive and does not throw
*/
bool
tests_subscription_unsubscribe_idempotent()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription s = b.subscribe("t", recorder(&log));

    s.unsubscribe();
    s.unsubscribe();      // must be a safe no-op

    return (!s.active());
}

/*
tests_subscription_default_unsubscribe_safe
  Verifies revoking / releasing a null handle is a safe no-op.
  Tests the following:
  - unsubscribe and release on a default handle neither crash nor activate it
*/
bool
tests_subscription_default_unsubscribe_safe()
{
    subscription s;

    s.unsubscribe();      // no shared flag held: must be a no-op
    s.release();
    s.unsubscribe();

    return (!s.active());
}

/*
tests_subscription_release
  Verifies release detaches the handle WITHOUT revoking the registration.
  Tests the following:
  - the released handle reports inactive
  - the handle's token is retained (only the flag is dropped)
  - the registration stays live and still receives publishes
*/
bool
tests_subscription_release()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription       s   = b.subscribe("t", recorder(&log));
    subscription_token tok = s.token();

    s.release();

    bool ok = true;

    ok = ok && (!s.active());              // this handle no longer observes it
    ok = ok && (s.token() == tok);         // but the token is kept
    ok = ok && (b.subscriber_count("t") == 1);   // the registration lives on

    b.publish("t", 7);
    ok = ok && (log.size() == 1 && log[0].payload == 7);   // and still receives

    return ok;
}

/*
tests_subscription_copies_share_flag
  Verifies copies of a handle share one liveness flag.
  Tests the following:
  - both copies start active
  - revoking through either copy deactivates both
*/
bool
tests_subscription_copies_share_flag()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription original = b.subscribe("t", recorder(&log));
    subscription copy     = original;

    bool ok = (original.active() && copy.active());

    copy.unsubscribe();                    // revoke via the copy

    ok = ok && (!original.active());       // the original sees it too
    ok = ok && (!copy.active());
    ok = ok && (b.subscriber_count() == 0);

    return ok;
}

/*
tests_subscription_outlives_broker
  Verifies a handle that outlives its broker never dangles. The shared flag (as
  opposed to a back-pointer) is what makes this safe: the header promises the
  handle stays safe to query and to revoke whichever object dies first. This
  test pins that documented guarantee; it deliberately does not pin the value of
  active() after destruction, which the header does not specify.
  Tests the following:
  - querying a handle after its broker is destroyed is safe
  - revoking it afterwards is safe and leaves it inactive
  - revoking twice remains safe
*/
bool
tests_subscription_outlives_broker()
{
    subscription s;

    {
        string_bus            b;
        std::vector<delivery> log;

        s = b.subscribe("t", recorder(&log));

        if (!s.active()) { return false; }
    }   // broker (and its registry) destroyed; s still holds the shared flag

    // both operations must be safe on a handle whose broker is gone
    volatile bool queried = s.active();
    (void)queried;

    s.unsubscribe();
    s.unsubscribe();                       // idempotent

    return (!s.active());
}

/*
tests_scoped_subscription_default
  Verifies a default-constructed guard is inert.
  Tests the following:
  - it reports inactive and destructs without effect
*/
bool
tests_scoped_subscription_default()
{
    scoped_subscription g;

    return (!g.active());
}

/*
tests_scoped_subscription_raii
  Verifies the guard revokes on destruction.
  Tests the following:
  - the registration is live while the guard is in scope
  - leaving the scope revokes it
  - an unguarded sibling registration survives
*/
bool
tests_scoped_subscription_raii()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription unguarded;

    {
        scoped_subscription g = b.subscribe("t", recorder(&log));   // implicit adopt
        unguarded = b.subscribe("t", recorder(&log));

        if (b.subscriber_count("t") != 2 || !g.active()) { return false; }
    }   // g revokes its registration

    bool ok = true;

    ok = ok && (b.subscriber_count("t") == 1);
    ok = ok && (unguarded.active());

    return ok;
}

/*
tests_scoped_subscription_move_ctor
  Verifies the move constructor transfers the revoke obligation.
  Tests the following:
  - the registration survives the move itself
  - destroying the moved-to guard revokes it
  - the moved-from guard is inert (it does not double-revoke)
*/
bool
tests_scoped_subscription_move_ctor()
{
    string_bus            b;
    std::vector<delivery> log;

    scoped_subscription source = b.subscribe("t", recorder(&log));

    {
        scoped_subscription sink = std::move(source);

        if (b.subscriber_count("t") != 1 || !sink.active()) { return false; }
    }   // sink revokes

    bool ok = true;

    ok = ok && (b.subscriber_count("t") == 0);
    ok = ok && (!source.active());        // moved-from guard holds nothing

    return ok;
}

/*
tests_scoped_subscription_move_assign
  Verifies move assignment revokes the guard's prior registration.
  Tests the following:
  - the previously held registration is revoked on assignment
  - the newly adopted registration stays live
*/
bool
tests_scoped_subscription_move_assign()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription first  = b.subscribe("a", recorder(&log));
    subscription second = b.subscribe("b", recorder(&log));

    scoped_subscription g{ first };
    g = scoped_subscription{ second };    // must revoke `first`

    bool ok = true;

    ok = ok && (!first.active());
    ok = ok && (second.active());
    ok = ok && (g.active());

    return ok;
}

/*
tests_scoped_subscription_self_move
  Verifies the self-move-assignment guard. The assignment is routed through a
  helper taking two references, so the aliasing is only discoverable at run time
  - which is both the realistic scenario the guard exists for and what keeps the
  compiler's -Wself-move diagnostic from firing on a deliberate test.
  Tests the following:
  - self-move-assigning leaves the registration live (it is not revoked)
*/
bool
tests_scoped_subscription_self_move()
{
    string_bus            b;
    std::vector<delivery> log;

    scoped_subscription g = b.subscribe("t", recorder(&log));

    move_assign(g, g);                    // guarded self-move

    bool ok = true;

    ok = ok && (g.active());
    ok = ok && (b.subscriber_count("t") == 1);

    return ok;
}

/*
tests_scoped_subscription_release
  Verifies release surrenders the revoke obligation.
  Tests the following:
  - the returned subscription is still live
  - destroying the released guard does not revoke the registration
*/
bool
tests_scoped_subscription_release()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription kept;

    {
        scoped_subscription g = b.subscribe("t", recorder(&log));
        kept = g.release();

        if (!kept.active()) { return false; }
    }   // g destructs, but it released -> no revoke

    bool ok = true;

    ok = ok && (kept.active());
    ok = ok && (b.subscriber_count("t") == 1);

    return ok;
}

/*
tests_scoped_subscription_unsubscribe
  Verifies early revocation through the guard.
  Tests the following:
  - unsubscribe revokes the held registration before the guard dies
*/
bool
tests_scoped_subscription_unsubscribe()
{
    string_bus            b;
    std::vector<delivery> log;

    scoped_subscription g = b.subscribe("t", recorder(&log));

    g.unsubscribe();

    bool ok = true;

    ok = ok && (!g.active());
    ok = ok && (b.subscriber_count("t") == 0);

    return ok;
}

/*
tests_scoped_subscription_move_only
  Verifies the guard's special-member profile.
  Tests the following:
  - scoped_subscription is move-constructible and move-assignable
  - it is neither copy-constructible nor copy-assignable
  - a plain subscription, by contrast, is copyable by design
*/
bool
tests_scoped_subscription_move_only()
{
    static_assert( std::is_move_constructible<scoped_subscription>::value, "movable");
    static_assert( std::is_move_assignable<scoped_subscription>::value,    "move-assignable");
    static_assert(!std::is_copy_constructible<scoped_subscription>::value, "not copyable");
    static_assert(!std::is_copy_assignable<scoped_subscription>::value,    "not copy-assignable");
    static_assert( std::is_copy_constructible<subscription>::value,        "subscription copies");

    bool ok = true;

    ok = ok && ( std::is_move_constructible<scoped_subscription>::value);
    ok = ok && (!std::is_copy_constructible<scoped_subscription>::value);
    ok = ok && ( std::is_copy_constructible<subscription>::value);

    return ok;
}

/*
tests_scoped_subscriptions_default
  Verifies a default-constructed collection.
  Tests the following:
  - it is empty with size zero
*/
bool
tests_scoped_subscriptions_default()
{
    scoped_subscriptions g;

    return (g.empty() && g.size() == 0);
}

/*
tests_scoped_subscriptions_add
  Verifies add tracks a subscription.
  Tests the following:
  - add grows the collection and leaves the registration live
*/
bool
tests_scoped_subscriptions_add()
{
    string_bus            b;
    std::vector<delivery> log;

    scoped_subscriptions g;
    g.add(b.subscribe("a", recorder(&log)));
    g.add(b.subscribe("b", recorder(&log)));

    bool ok = true;

    ok = ok && (g.size() == 2);
    ok = ok && (!g.empty());
    ok = ok && (b.subscriber_count() == 2);

    return ok;
}

/*
tests_scoped_subscriptions_plus_equals
  Verifies operator+= tracks a subscription.
  Tests the following:
  - += grows the collection like add
*/
bool
tests_scoped_subscriptions_plus_equals()
{
    string_bus            b;
    std::vector<delivery> log;

    scoped_subscriptions g;
    g += b.subscribe("a", recorder(&log));
    g += b.subscribe("a", recorder(&log));

    bool ok = true;

    ok = ok && (g.size() == 2);
    ok = ok && (b.subscriber_count("a") == 2);

    return ok;
}

/*
tests_scoped_subscriptions_raii
  Verifies the collection revokes everything on destruction.
  Tests the following:
  - all held registrations are live inside the scope
  - destroying the collection revokes them all
  - an untracked registration survives
*/
bool
tests_scoped_subscriptions_raii()
{
    string_bus            b;
    std::vector<delivery> log;

    subscription untracked = b.subscribe("keep", recorder(&log));

    {
        scoped_subscriptions g;
        g.add(b.subscribe("a", recorder(&log)));
        g.add(b.subscribe("a", recorder(&log)));
        g.add(b.subscribe("b", recorder(&log)));

        if (b.subscriber_count() != 4) { return false; }
    }   // g revokes its three

    bool ok = true;

    ok = ok && (b.subscriber_count() == 1);
    ok = ok && (untracked.active());

    return ok;
}

/*
tests_scoped_subscriptions_unsubscribe_all
  Verifies explicit bulk revocation.
  Tests the following:
  - unsubscribe_all revokes every held registration
  - the collection is emptied
*/
bool
tests_scoped_subscriptions_unsubscribe_all()
{
    string_bus            b;
    std::vector<delivery> log;

    scoped_subscriptions g;
    g.add(b.subscribe("a", recorder(&log)));
    g.add(b.subscribe("b", recorder(&log)));

    g.unsubscribe_all();

    bool ok = true;

    ok = ok && (g.empty() && g.size() == 0);
    ok = ok && (b.subscriber_count() == 0);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
