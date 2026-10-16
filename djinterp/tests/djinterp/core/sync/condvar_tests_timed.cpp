/*******************************************************************************
* djinterp [core]                                        condvar_tests_timed.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_timed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                            condvar_tests_timed.cpp
//   Section IV (e): wait_for and wait_until - waiting with a deadline.
//
//   A timed wait has two outcomes and the caller must be able to tell them
// apart: cv_status::timeout (the deadline passed) and cv_status::no_timeout (we
// were woken).  The predicate forms collapse that into a bool - "did the
// condition hold in the end" - and in doing so absorb spurious wakeups, exactly
// as the untimed ones do.  Every one of those four returns is pinned here, in
// both directions, along with the three edge cases that catch people out: a
// predicate that is ALREADY true (the wait must not wait), a ZERO duration (the
// wait must still return, and time out), and a deadline in the PAST (likewise).
//
// These tests can afford to wait on the runner's own thread, unlike the untimed
// ones: every wait here is bounded by construction, so the worst a broken
// condvar
// can do is fail slowly.  The one test that needs a second thread has it
// notify,
// not wait.
//
//   The last test in the file is not about behaviour but about SURFACE:
// std::condition_variable offers wait_until(lock, deadline, predicate), and
// portable_condvar does not forward it.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef channel<exclusive_lock_policy>       exclusive_channel;
    typedef channel<timed_lock_policy>           any_channel;
    typedef std::unique_lock<std::mutex>         exclusive_lock;
    typedef std::unique_lock<std::timed_mutex>   any_lock;
    typedef portable_condvar<exclusive_lock_policy> exclusive_cv;

    // k_long_wait_ms
    //   constant: a wait long enough that an early return is unmistakable.
    D_STATIC D_CONSTEXPR unsigned k_long_wait_ms = k_wait_ms * 2;

    // k_long_floor_ms
    //   constant: the least a wait_for(k_long_wait_ms) may take and still be
    // said to have waited it out.
    D_STATIC D_CONSTEXPR unsigned k_long_floor_ms = (k_long_wait_ms * 3) / 4;

    // delayed_publisher
    // function: sleep, then publish the condition and notify - a producer that
    // arrives while the consumer is already waiting.
    void
    delayed_publisher(
        exclusive_channel* _channel,
        unsigned           _delay_ms
    )
    {
        sleep_ms(_delay_ms);
        _channel->publish_and_notify_all();

        return;
    }

    // delayed_empty_notifier
    //   function: sleep, then notify WITHOUT publishing - the bare notification
    // a predicate wait must refuse to be satisfied by.
    void
    delayed_empty_notifier(
        exclusive_channel* _channel,
        unsigned           _delay_ms
    )
    {
        sleep_ms(_delay_ms);
        _channel->cv.notify_all();

        return;
    }
}

/*
tests_wait_for_times_out
  nobody notifies; the wait expires.
  Tests the following:
  - wait_for(lock, 80ms) returns cv_status::timeout
  - and really waited: at least three quarters of the duration elapsed, so a
    condvar that returned early would be caught
*/
bool
tests_wait_for_times_out()
{
    exclusive_channel            ch;
    exclusive_lock               lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const std::cv_status status =
        ch.cv.wait_for(lock, std::chrono::milliseconds(k_wait_ms));

    const long long elapsed = elapsed_ms(start);

    return ( (status == std::cv_status::timeout) &&
             (elapsed >= static_cast<long long>(k_expired_floor_ms)) );
}

/*
tests_wait_for_returns_no_timeout_when_notified
  a notification inside the window is reported as such.
  Tests the following:
  - a publisher notifies about 10ms into a wait whose deadline is seconds away
  - wait_for returns cv_status::no_timeout
  - it also proves the wait released the mutex: the publisher takes that very
    mutex to publish, and could not have if the waiter still held it
*/
bool
tests_wait_for_returns_no_timeout_when_notified()
{
    exclusive_channel ch;
    std::cv_status    status = std::cv_status::timeout;
    std::thread       publisher;

    {
        // the lock is taken BEFORE the publisher is started: publish() cannot
        // get the mutex until wait_for has released it, so the notification can
        // never arrive before there is a waiter to receive it.  The join comes
        // AFTER the lock is dropped - a publisher still blocked on the mutex
        // could never finish while this thread held it.
        exclusive_lock lock(ch.mutex);

        publisher = std::thread(&delayed_publisher, &ch, 10u);

        status = ch.cv.wait_for(
            lock, std::chrono::milliseconds(k_deadline_ms));
    }

    publisher.join();

    return (status == std::cv_status::no_timeout);
}

/*
tests_wait_for_with_a_predicate_returns_false_on_timeout
  the predicate form reports the CONDITION, not the wakeup.
  Tests the following:
  - the condition is never published, so the predicate is always false
  - wait_for(lock, 80ms, pred) returns FALSE - the condition did not come to
  hold
  - and the full duration elapsed
*/
bool
tests_wait_for_with_a_predicate_returns_false_on_timeout()
{
    exclusive_channel            ch;
    exclusive_lock               lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const bool satisfied =
        ch.cv.wait_for(lock,
                            std::chrono::milliseconds(k_wait_ms),
                            ch.predicate());

    const long long elapsed = elapsed_ms(start);

    return ( (!satisfied) &&
             (elapsed >= static_cast<long long>(k_expired_floor_ms)) &&
             (ch.pred_calls.load() >= 1) );
}

/*
tests_wait_for_with_a_predicate_returns_true_when_satisfied
  ...and TRUE when it does come to hold.
  Tests the following:
  - a publisher publishes about 10ms in
  - wait_for(lock, deadline, pred) returns true
*/
bool
tests_wait_for_with_a_predicate_returns_true_when_satisfied()
{
    exclusive_channel ch;
    bool              satisfied = false;
    std::thread       publisher;

    {
        exclusive_lock lock(ch.mutex);

        publisher = std::thread(&delayed_publisher, &ch, 10u);

        satisfied = ch.cv.wait_for(
            lock,
            std::chrono::milliseconds(k_deadline_ms),
            ch.predicate());
    }

    publisher.join();

    return satisfied;
}

/*
tests_wait_for_with_a_true_predicate_returns_at_once
  a condition that already holds costs no time at all.
  Tests the following:
  - the condition is published before the wait begins
  - wait_for(lock, 80ms, pred) returns true in under half that
  - so a timed wait on an already-satisfied condition is not a sleep
*/
bool
tests_wait_for_with_a_true_predicate_returns_at_once()
{
    exclusive_channel ch;

    ch.publish();

    exclusive_lock               lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const bool satisfied =
        ch.cv.wait_for(lock,
                            std::chrono::milliseconds(k_wait_ms),
                            ch.predicate());

    const long long elapsed = elapsed_ms(start);

    return ( satisfied &&
             (elapsed < static_cast<long long>(k_immediate_ceiling_ms)) );
}

/*
tests_wait_for_with_a_zero_duration_times_out
  a zero-length wait still returns - having waited for nothing.
  Tests the following:
  - wait_for(lock, 0ms) returns cv_status::timeout at once
  - wait_for(lock, 0ms, pred) with a false predicate returns false at once
  - the predicate was still evaluated: a zero duration means "check and leave",
    not "skip the check"
*/
bool
tests_wait_for_with_a_zero_duration_times_out()
{
    exclusive_channel            ch;
    exclusive_lock               lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const std::cv_status status =
        ch.cv.wait_for(lock, std::chrono::milliseconds(0));

    const bool satisfied =
        ch.cv.wait_for(lock,
                            std::chrono::milliseconds(0),
                            ch.predicate());

    const long long elapsed = elapsed_ms(start);

    return ( (status == std::cv_status::timeout)                     &&
             (!satisfied)                                            &&
             (ch.pred_calls.load() >= 1)                        &&
             (elapsed < static_cast<long long>(k_immediate_ceiling_ms)) );
}

/*
tests_wait_for_keeps_waiting_while_the_predicate_is_false
  a bare notification does not end a timed predicate wait, either.
  Tests the following:
  - a notifier notifies - without publishing - about 20ms into a 160ms wait
  - the wait does NOT return there: it re-checks the predicate, finds it false,
    and goes back to waiting for the remainder of its deadline
  - it returns false, having waited at least three quarters of the full 160ms
  The untimed equivalent is tests_wait_ignores_a_notification_while_the_
  predicate_is_false; this is the same contract, with a clock attached, and it
  is
  what keeps a timed wait from reporting success on a spurious wakeup.
*/
bool
tests_wait_for_keeps_waiting_while_the_predicate_is_false()
{
    exclusive_channel ch;
    bool              satisfied = true;
    long long         elapsed   = 0;
    std::thread       notifier;

    {
        exclusive_lock               lock(ch.mutex);
        const test_clock::time_point start = test_clock::now();

        notifier = std::thread(&delayed_empty_notifier, &ch, 20u);

        satisfied = ch.cv.wait_for(
            lock,
            std::chrono::milliseconds(k_long_wait_ms),
            ch.predicate());

        elapsed = elapsed_ms(start);
    }

    notifier.join();

    return ( (!satisfied) &&
             (elapsed >= static_cast<long long>(k_long_floor_ms)) );
}

/*
tests_wait_until_times_out
  the same contract, expressed as a deadline rather than a duration.
  Tests the following:
  - wait_until(lock, now + 80ms) returns cv_status::timeout
  - and at least three quarters of that elapsed
*/
bool
tests_wait_until_times_out()
{
    exclusive_channel            ch;
    exclusive_lock               lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const test_clock::time_point deadline =
        start + std::chrono::milliseconds(k_wait_ms);

    const std::cv_status status = ch.cv.wait_until(lock, deadline);

    const long long elapsed = elapsed_ms(start);

    return ( (status == std::cv_status::timeout) &&
             (elapsed >= static_cast<long long>(k_expired_floor_ms)) );
}

/*
tests_wait_until_with_a_past_deadline_returns_at_once
  a deadline that has already gone by is not an error, and not a wait.
  Tests the following:
  - wait_until(lock, now - 1s) returns cv_status::timeout immediately
  - it does not block, does not throw, and does not wrap around into a very long
    wait - which is what a duration computed as a subtraction of unsigned clocks
    would have done
*/
bool
tests_wait_until_with_a_past_deadline_returns_at_once()
{
    exclusive_channel            ch;
    exclusive_lock               lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const test_clock::time_point already_gone =
        start - std::chrono::seconds(1);

    const std::cv_status status = ch.cv.wait_until(lock, already_gone);

    const long long elapsed = elapsed_ms(start);

    return ( (status == std::cv_status::timeout) &&
             (elapsed < static_cast<long long>(k_immediate_ceiling_ms)) );
}

/*
tests_wait_until_returns_no_timeout_when_notified
  a notification before the deadline is reported as one.
  Tests the following:
  - a publisher notifies about 10ms in; the deadline is seconds away
  - wait_until returns cv_status::no_timeout
*/
bool
tests_wait_until_returns_no_timeout_when_notified()
{
    exclusive_channel ch;
    std::cv_status    status = std::cv_status::timeout;
    std::thread       publisher;

    {
        exclusive_lock lock(ch.mutex);

        publisher = std::thread(&delayed_publisher, &ch, 10u);

        const test_clock::time_point deadline =
            test_clock::now() + std::chrono::milliseconds(k_deadline_ms);

        status = ch.cv.wait_until(lock, deadline);
    }

    publisher.join();

    return (status == std::cv_status::no_timeout);
}

/*
tests_wait_for_on_the_condition_variable_any_path
  the timed waits work on the OTHER branch of the selector too.
  Tests the following:
  - the timed policy (std::timed_mutex -> std::condition_variable_any) times out
    on wait_for, exactly as the exclusive one does
  - and its deadline is honoured to the same tolerance
  Everything above ran through std::condition_variable.  The four policies that
  are not std::mutex-backed run through condition_variable_any instead - a
  different type, a different implementation, and the same contract.  This is
  the
  test that says so.
*/
bool
tests_wait_for_on_the_condition_variable_any_path()
{
    any_channel                  ch;
    any_lock                     lock(ch.mutex);
    const test_clock::time_point start = test_clock::now();

    const std::cv_status status =
        ch.cv.wait_for(lock, std::chrono::milliseconds(k_wait_ms));

    const long long elapsed = elapsed_ms(start);

    const bool it_is_the_any_condvar =
        std::is_same<any_channel::condvar_type,
                     std::condition_variable_any>::value;

    return ( it_is_the_any_condvar                &&
             (status == std::cv_status::timeout)  &&
             (elapsed >= static_cast<long long>(k_expired_floor_ms)) );
}

/*
tests_wait_until_has_no_predicate_overload
  a gap in the wrapper's surface, not in the standard's.
  Tests the following:
  - std::condition_variable and std::condition_variable_any BOTH offer
    wait_until(lock, deadline, predicate) - the form that absorbs spurious
    wakeups up to a deadline
  - portable_condvar does NOT forward it.  Its wait_until takes two arguments
    only, so `cv.wait_until(lock, deadline, pred)` does not compile
  - the workaround is either the duration form (wait_for(lock, d, pred), which
  IS
    forwarded) or native(), which hands back the std condvar with its full
    surface intact
  Pinned as current behaviour.  Forwarding the third overload would be four
  lines; until then, this test is the note that says why the call you expected
  to
  work does not.
*/
bool
tests_wait_until_has_no_predicate_overload()
{
    D_CONSTEXPR bool the_standard_has_it =
        ( has_predicate_wait_until<std::condition_variable,
                                   exclusive_lock,
                                   constant_predicate>::value &&
          has_predicate_wait_until<std::condition_variable_any,
                                   any_lock,
                                   constant_predicate>::value );

    D_CONSTEXPR bool the_wrapper_does_not =
        !has_predicate_wait_until<exclusive_cv,
                                  exclusive_lock,
                                  constant_predicate>::value;

    static_assert(the_standard_has_it,
                  "the std condvars do offer a predicate wait_until");

    return (the_standard_has_it && the_wrapper_does_not);
}


NS_END  // testing
NS_END  // djinterp
