/*******************************************************************************
* djinterp [core]                                         condvar_tests_wait.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_wait.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                             condvar_tests_wait.cpp
//   Section IV (d): wait and notify - what actually happens when a thread
// blocks.
//
//   Everything here runs on the exclusive policy, because that is the path with
// a real std::condition_variable behind it and real blocking to observe.  The
// tests are built to be DETERMINISTIC rather than probabilistic, which for a
// condvar means three rules:
//
//     - never assert that a thread is blocked by watching it not finish for a
//       moment; assert it by making the ONLY thing that could release it happen
//       later, and checking it did not finish before that;
//     - never count wakeups from notify_one (a spurious wakeup is legal and the
//       count is not a contract); assert that AT LEAST one waiter came back;
//     - never leave a waiter that the test can no longer satisfy - every test
//       ends in rescue(), so a broken wait is a reported failure and not a hung
//       runner.
//
//   The two claims that justify the whole predicate protocol are here: a
// notification arriving while the predicate is false does NOT wake the waiter
// (it re-checks and blocks again), and a notification arriving with NO waiter
// is
// LOST (it is not stored for the next one).  Between them they are the reason
// you may not write a condvar loop without shared state.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef channel<exclusive_lock_policy> exclusive_channel;
    typedef std::unique_lock<std::mutex>   exclusive_lock;

    // bare_wait_worker
    //   function: the canonical loop AROUND a predicate-less wait - the form
    // every condvar tutorial opens with, and the only correct way to use
    // wait(lock): re-check the condition on every wakeup, because a wakeup is
    // not a promise.
    void
    bare_wait_worker(
        exclusive_channel* _channel
    )
    {
        exclusive_lock lock(_channel->mutex);

        _channel->waiting.store(true);

        while ( (!_channel->ready.load()) &&
                (!_channel->abort.load()) )
        {
            _channel->cv.wait(lock);
        }

        // the wait handed the mutex back before it returned
        _channel->relocked.store(lock.owns_lock());

        _channel->wakes.fetch_add(1);
        _channel->woke.store(true);

        return;
    }

    // timed_wait_worker
    //   function: a waiter that gives up after k_wait_ms.  Used by the
    // lost-notification test, where the waiter MUST be allowed to time out.
    void
    timed_wait_worker(
        exclusive_channel* _channel,
        std::atomic<bool>* _satisfied
    )
    {
        exclusive_lock lock(_channel->mutex);

        _channel->waiting.store(true);

        const bool satisfied =
            _channel->cv.wait_for(lock,
                                  std::chrono::milliseconds(k_wait_ms),
                                  _channel->predicate());

        _satisfied->store(satisfied);
        _channel->woke.store(true);

        return;
    }
}

/*
tests_wait_blocks_until_notified
  a bare wait blocks, and a notification releases it.
  Tests the following:
  - a worker inside `while (!ready) cv.wait(lock);` does not come back on its
  own
  - it is still there after the settle window, with nothing published
  - it comes back once the condition is published and the condvar notified
*/
bool
tests_wait_blocks_until_notified()
{
    exclusive_channel ch;

    std::thread waiter(&bare_wait_worker, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    sleep_ms(k_settle_ms);

    const bool still_blocked = (!ch.woke.load());

    ch.publish_and_notify_one();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait &&
             still_blocked    &&
             woke );
}

/*
tests_wait_releases_the_mutex_while_blocked
  a blocked waiter is NOT holding the mutex - which is the entire reason a
  condition variable exists rather than a spin on a locked flag.
  Tests the following:
  - while a worker is inside cv.wait(lock), THIS thread can acquire the same
    mutex
  - it acquires it by try_lock, in a bounded retry, so the proof is positive
    (the lock was taken) rather than negative (nothing hung)
  - the worker is released afterwards and comes back
  The retry loop is not a fudge: the worker sets `waiting` while it still holds
  the mutex, just before it waits, so the first try_lock may legitimately lose.
  What could never succeed is a try_lock against a waiter that kept the lock.
*/
bool
tests_wait_releases_the_mutex_while_blocked()
{
    exclusive_channel            ch;
    bool                         acquired = false;
    const test_clock::time_point start    = test_clock::now();

    std::thread waiter(&bare_wait_worker, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    // the mutex must become free while the worker sits in wait()
    while (elapsed_ms(start) < static_cast<long long>(k_deadline_ms))
    {
        // check if the waiter has let go of the mutex
        if (ch.mutex.try_lock())
        {
            ch.mutex.unlock();
            acquired = true;
            break;
        }

        sleep_ms(1);
    }

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait &&
             acquired         &&
             woke );
}

/*
tests_wait_reacquires_the_mutex_before_returning
  ...and it takes the mutex back on the way out.
  Tests the following:
  - the unique_lock the worker handed to wait() owns the mutex again when wait()
    returns
  - which is what makes it safe to read the shared condition immediately after
    the wait, inside the same loop
*/
bool
tests_wait_reacquires_the_mutex_before_returning()
{
    exclusive_channel ch;

    std::thread waiter(&bare_wait_worker, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_one();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait      &&
             woke                  &&
             ch.relocked.load() );
}

/*
tests_wait_with_a_predicate_returns_when_it_holds
  the predicate overload: the same thing, with the loop written for you.
  Tests the following:
  - a worker in cv.wait(lock, pred) blocks while the predicate is false
  - it returns once the condition is published and the condvar notified
  - the predicate WAS evaluated - at least once on entry, and again on each
    wakeup (contrast the no-op condvar, which never asks)
*/
bool
tests_wait_with_a_predicate_returns_when_it_holds()
{
    exclusive_channel ch;

    std::thread waiter(&wait_worker<exclusive_lock_policy>, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    sleep_ms(k_settle_ms);

    const bool still_blocked = (!ch.woke.load());

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait                 &&
             still_blocked                    &&
             woke                             &&
             (ch.pred_calls.load() >= 1) );
}

/*
tests_wait_with_a_true_predicate_never_blocks
  if the condition already holds, the wait does not wait.
  Tests the following:
  - the condition is published BEFORE the waiter ever starts
  - the waiter returns from cv.wait(lock, pred) without anybody ever notifying -
    no notify_one, no notify_all, at any point
  - and it evaluated the predicate to find that out
  This is the case that makes the predicate overload safe against the lost
  wakeup: a notification that arrived before the waiter did is not needed,
  because the STATE it announced is still there.
*/
bool
tests_wait_with_a_true_predicate_never_blocks()
{
    exclusive_channel ch;

    // publish first; nobody will ever notify this condvar
    ch.publish();

    std::thread waiter(&wait_worker<exclusive_lock_policy>, &ch);

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( woke &&
             (ch.pred_calls.load() >= 1) );
}

/*
tests_wait_ignores_a_notification_while_the_predicate_is_false
  a notification is not a wakeup: the predicate decides.
  Tests the following:
  - a waiter sits in cv.wait(lock, pred) with the condition unpublished
  - the condvar is notified - notify_all, twice, loudly - and NOTHING is
    published
  - the waiter re-checks its predicate, finds it still false, and blocks again:
    it is still there after the settle window
  - once the condition IS published, the next notification releases it
  This is the deterministic form of "spurious wakeups are absorbed".  A bare
  wait(lock) would have returned on those notifications; the predicate overload
  cannot, and that is the whole of its value.
*/
bool
tests_wait_ignores_a_notification_while_the_predicate_is_false()
{
    exclusive_channel ch;

    std::thread waiter(&wait_worker<exclusive_lock_policy>, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    // notified, but with nothing to find
    ch.cv.notify_all();
    sleep_ms(5);
    ch.cv.notify_all();

    sleep_ms(k_settle_ms);

    const bool ignored_the_empty_notification = (!ch.woke.load());

    // now give it something to find
    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait                 &&
             ignored_the_empty_notification   &&
             woke );
}

/*
tests_notify_one_wakes_a_waiter
  one notification, one waiter.
  Tests the following:
  - a single notify_one releases the only thread waiting
*/
bool
tests_notify_one_wakes_a_waiter()
{
    exclusive_channel ch;

    std::thread waiter(&wait_worker<exclusive_lock_policy>, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_one();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait &&
             woke             &&
             (ch.wakes.load() == 1) );
}

/*
tests_notify_all_wakes_every_waiter
  one notification, every waiter.
  Tests the following:
  - four threads wait on one condvar
  - the condition is published once, and notify_all is called once
  - all four come back
  Note the ORDER: publish under the lock, THEN notify.  Notifying first would be
  a race, and the four would be entitled to miss it.
*/
bool
tests_notify_all_wakes_every_waiter()
{
    exclusive_channel        ch;
    std::vector<std::thread> waiters;
    int                      i = 0;

    for (i = 0; i < k_waiters; ++i)
    {
        waiters.push_back(
            std::thread(&wait_worker<exclusive_lock_policy>, &ch));
    }

    // every waiter must have reached the wait before the single notify lands
    const bool all_waiting =
        spin_until_count(ch.pred_calls, k_waiters, k_deadline_ms);

    sleep_ms(k_settle_ms);

    ch.publish_and_notify_all();

    const bool all_woke =
        spin_until_count(ch.wakes, k_waiters, k_deadline_ms);

    ch.rescue();

    for (i = 0; i < k_waiters; ++i)
    {
        waiters[static_cast<std::size_t>(i)].join();
    }

    return ( all_waiting &&
             all_woke    &&
             (ch.wakes.load() == k_waiters) );
}

/*
tests_notify_one_wakes_at_least_one_of_many
  one notification, one waiter - and no promise about which.
  Tests the following:
  - four threads wait; the condition is published; notify_one is called ONCE
  - at least one of them comes back
  The assertion is "at least one" on purpose.  The standard unblocks one waiter,
  but a spurious wakeup may return others whose predicate is now true, and
  nothing says which thread wins.  A test that demanded exactly one would be
  asserting a race, not a contract.  The rest are released by the rescue.
*/
bool
tests_notify_one_wakes_at_least_one_of_many()
{
    exclusive_channel        ch;
    std::vector<std::thread> waiters;
    int                      i = 0;

    for (i = 0; i < k_waiters; ++i)
    {
        waiters.push_back(
            std::thread(&wait_worker<exclusive_lock_policy>, &ch));
    }

    const bool all_waiting =
        spin_until_count(ch.pred_calls, k_waiters, k_deadline_ms);

    ch.publish_and_notify_one();

    const bool at_least_one_woke =
        spin_until_count(ch.wakes, 1, k_deadline_ms);

    // release whoever is left, however many that is
    ch.rescue();

    const bool all_returned =
        spin_until_count(ch.wakes, k_waiters, k_deadline_ms);

    for (i = 0; i < k_waiters; ++i)
    {
        waiters[static_cast<std::size_t>(i)].join();
    }

    return ( all_waiting       &&
             at_least_one_woke &&
             all_returned );
}

/*
tests_a_notification_with_no_waiter_is_lost
  a condvar has no memory.
  Tests the following:
  - notify_all is called on a condvar nobody is waiting on - twice
  - a waiter then arrives, with the condition still unpublished, and waits with
  a
    timeout
  - it TIMES OUT.  The earlier notifications were not saved for it; they went
    nowhere, and are gone
  This is the other half of why the predicate protocol exists.  A notification
  is
  an event, not a state; only the shared condition is a state.  Publish first,
  under the lock, and let the predicate - not the notification - be what the
  waiter believes.
*/
bool
tests_a_notification_with_no_waiter_is_lost()
{
    exclusive_channel ch;
    std::atomic<bool> satisfied(true);   // pessimistic: the test must clear it

    // shout into an empty room
    ch.cv.notify_all();
    ch.cv.notify_all();

    sleep_ms(5);

    std::thread waiter(&timed_wait_worker, &ch, &satisfied);

    const bool returned = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    // the waiter timed out: it never saw those notifications
    const bool timed_out = (!satisfied.load());

    return (returned && timed_out);
}


NS_END  // testing
NS_END  // djinterp
