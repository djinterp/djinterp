/*******************************************************************************
* djinterp [core]                                        condvar_tests_no_op.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_no_op.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                           condvar_tests_no_op.cpp
//   Section IV (b): internal::no_op_condvar - the condvar for a program with no
// other threads.
//
//   The header's promise is that null_lock_policy lets the same source compile
// and run single-threaded at zero cost.  For locks that is exactly true.  For a
// CONDITION VARIABLE it cannot be, and the no-op's job is to fail as quietly as
// possible.  What it actually does is worth writing down, because none of it is
// what a reader of `cv.wait(lock, pred)` expects:
//
//     wait(lock)             returns at once.  The lock is not released.
//     wait(lock, pred)       returns at once, and NEVER CALLS pred.  Not even
//                            once.  A false condition is not detected, not
//                            asserted, not reported - the caller simply
//                            proceeds as if the condition held.
//     wait_for(lock, d)      returns at once with cv_status::no_timeout - it
//                            claims to have been NOTIFIED, having waited for
//                            nothing.
//     wait_for(lock, d, p)   calls p exactly once and returns what it said.
//                            The only overload that looks at the predicate.
//
//   The source itself notes the gap - "if pred is false, it will never become
// true ... this is a programming error.  In debug builds, assert." - but there
// is no assert, in any build.  Every one of those behaviours is pinned below,
// as behaviour, so that a change to any of them shows up as a failing test
// rather than as a silent change of meaning in single-threaded builds.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef internal::no_op_condvar null_condvar;
}

/*
tests_no_op_condvar_is_empty
  the no-op costs nothing.
  Tests the following:
  - it is an empty class: one byte, no members, no vtable
  - it is default-constructible and trivially copyable
  - unlike portable_condvar, which deletes its copy, the no-op itself is
    copyable - it has no state to copy
*/
bool
tests_no_op_condvar_is_empty()
{
    D_CONSTEXPR bool empty =
        ( std::is_empty<null_condvar>::value          &&
          std::is_class<null_condvar>::value          &&
          !std::is_polymorphic<null_condvar>::value );

    const bool one_byte = (sizeof(null_condvar) == 1);

    D_CONSTEXPR bool constructible =
        ( std::is_default_constructible<null_condvar>::value &&
          std::is_copy_constructible<null_condvar>::value );

    static_assert(empty, "the no-op condvar must be an empty class");

    return ( empty         &&
             one_byte      &&
             constructible );
}

/*
tests_no_op_notify_is_a_noexcept_no_op
  notifying nobody is free and cannot fail.
  Tests the following:
  - notify_one and notify_all return void and are noexcept
  - calling them repeatedly on a fresh no-op condvar does nothing observable and
    does not throw
*/
bool
tests_no_op_notify_is_a_noexcept_no_op()
{
    null_condvar cv;
    int          i = 0;

    D_CONSTEXPR bool returns_void =
        ( std::is_same<decltype(std::declval<null_condvar&>().notify_one()),
                       void>::value &&
          std::is_same<decltype(std::declval<null_condvar&>().notify_all()),
                       void>::value );

    D_CONSTEXPR bool is_noexcept =
        ( noexcept(std::declval<null_condvar&>().notify_one()) &&
          noexcept(std::declval<null_condvar&>().notify_all()) );

    static_assert(returns_void && is_noexcept,
                  "the no-op condvar's notifies must be noexcept and void");

    for (i = 0; i < 100; ++i)
    {
        cv.notify_one();
        cv.notify_all();
    }

    return (returns_void && is_noexcept);
}

/*
tests_no_op_wait_returns_immediately
  a wait with no other threads to wait for cannot block, and does not.
  Tests the following:
  - wait(lock) returns at once - far inside the ceiling a real wait would blow
    through
  - it does not deadlock, which is the one thing a single-threaded wait on a
    real condvar WOULD do
*/
bool
tests_no_op_wait_returns_immediately()
{
    null_condvar   cv;
    recording_lock lock;

    const test_clock::time_point start = test_clock::now();

    cv.wait(lock);

    const long long elapsed = elapsed_ms(start);

    return (elapsed < static_cast<long long>(k_immediate_ceiling_ms));
}

/*
tests_no_op_wait_never_touches_the_lock
  the no-op does not release the mutex - because there is no mutex.
  Tests the following:
  - a recording lock passed to wait(), wait(pred) and wait_for() comes back with
    zero lock() calls and zero unlock() calls
  - a REAL condvar unlocks on the way in and re-locks on the way out (see
    tests_wait_releases_the_mutex_while_blocked); the no-op does neither
  - which is sound only because null_lock_policy's lock was already a no-op.
    Hand the no-op condvar a real lock - it takes any type - and it will hold it
    right through the "wait".
*/
bool
tests_no_op_wait_never_touches_the_lock()
{
    null_condvar     cv;
    recording_lock   lock;
    std::atomic<int> predicate_calls(0);

    cv.wait(lock);
    cv.wait(lock, constant_predicate(false, &predicate_calls));
    (void)cv.wait_for(lock, std::chrono::milliseconds(k_wait_ms));
    (void)cv.wait_for(lock,
                      std::chrono::milliseconds(k_wait_ms),
                      constant_predicate(true, &predicate_calls));

    return ( (lock.locks   == 0) &&
             (lock.unlocks == 0) );
}

/*
tests_no_op_wait_ignores_its_predicate
  the sharpest edge in the header: wait(lock, pred) does not look at pred.
  Tests the following:
  - a predicate that answers FALSE is handed to wait(lock, pred)
  - the wait returns anyway, at once
  - and the predicate is invoked ZERO times.  Not evaluated-and-ignored:
    never called.
  A real condvar would block here forever (correctly - the condition is false
  and nobody will ever notify).  The no-op instead lets the caller through, and
  the caller proceeds as though its condition held.  The source acknowledges
  this and says "in debug builds, assert" - but nothing asserts, in any build.
  This test exists so that the day someone adds that assert, or makes the
  predicate be evaluated, it is a visible change and not a silent one.
*/
bool
tests_no_op_wait_ignores_its_predicate()
{
    null_condvar     cv;
    recording_lock   lock;
    std::atomic<int> predicate_calls(0);

    const test_clock::time_point start = test_clock::now();

    cv.wait(lock, constant_predicate(false, &predicate_calls));

    const long long elapsed = elapsed_ms(start);

    const bool returned_at_once =
        (elapsed < static_cast<long long>(k_immediate_ceiling_ms));

    const bool predicate_was_never_asked = (predicate_calls.load() == 0);

    return (returned_at_once && predicate_was_never_asked);
}

/*
tests_no_op_wait_for_claims_no_timeout
  the no-op's timed wait reports that it was NOTIFIED.
  Tests the following:
  - wait_for(lock, 80ms) returns std::cv_status::no_timeout
  - although nothing notified it, and nothing could have
  A caller looping `while (cv.wait_for(lock, d) == cv_status::timeout)`
  therefore
  exits the loop immediately under the null policy - which is the intended
  outcome, but arrived at by a claim that is not true.  Pinned as behaviour.
*/
bool
tests_no_op_wait_for_claims_no_timeout()
{
    null_condvar   cv;
    recording_lock lock;

    const std::cv_status status =
        cv.wait_for(lock, std::chrono::milliseconds(k_wait_ms));

    return (status == std::cv_status::no_timeout);
}

/*
tests_no_op_wait_for_does_not_sleep
  ...and it does not wait out the duration, either.
  Tests the following:
  - wait_for(lock, 80ms) returns in well under half that
  - so the null policy pays no latency for a timed wait, and a program that used
    wait_for as a sleep would find it does not sleep
*/
bool
tests_no_op_wait_for_does_not_sleep()
{
    null_condvar   cv;
    recording_lock lock;

    const test_clock::time_point start = test_clock::now();

    (void)cv.wait_for(lock, std::chrono::milliseconds(k_wait_ms));

    const long long elapsed = elapsed_ms(start);

    return (elapsed < static_cast<long long>(k_immediate_ceiling_ms));
}

/*
tests_no_op_wait_for_with_a_predicate_returns_it
  the one overload that does look at the predicate - and the asymmetry is the
  point.
  Tests the following:
  - wait_for(lock, d, pred) invokes pred EXACTLY ONCE and returns what it said
  - a true predicate gives true; a false one gives false, at once, without
    waiting out the duration
  - compare tests_no_op_wait_ignores_its_predicate: the untimed wait(lock, pred)
    never calls the predicate at all.  Two overloads, two different treatments
    of the same argument, in the same class.
*/
bool
tests_no_op_wait_for_with_a_predicate_returns_it()
{
    null_condvar     cv;
    recording_lock   lock;
    std::atomic<int> true_calls(0);
    std::atomic<int> false_calls(0);

    const bool said_true =
        cv.wait_for(lock,
                    std::chrono::milliseconds(k_wait_ms),
                    constant_predicate(true, &true_calls));

    const test_clock::time_point start = test_clock::now();

    const bool said_false =
        cv.wait_for(lock,
                    std::chrono::milliseconds(k_wait_ms),
                    constant_predicate(false, &false_calls));

    const long long elapsed = elapsed_ms(start);

    return ( said_true                     &&
             (!said_false)                 &&
             (true_calls.load()  == 1)     &&
             (false_calls.load() == 1)     &&
             (elapsed < static_cast<long long>(k_immediate_ceiling_ms)) );
}

/*
tests_no_op_condvar_lives_in_internal
  it is an implementation detail, and it is spelled like one.
  Tests the following:
  - no_op_condvar is ::djinterp::internal::no_op_condvar, not a public name
  - the selector that hands it out is internal too
  - a consumer reaches it only through portable_condvar<null_lock_policy>, which
    is how it should be reached
*/
bool
tests_no_op_condvar_lives_in_internal()
{
    D_CONSTEXPR bool in_internal =
        std::is_same<internal::no_op_condvar,
                     ::djinterp::internal::no_op_condvar>::value;

    D_CONSTEXPR bool reached_through_the_policy =
        std::is_same<portable_condvar<null_lock_policy>::condvar_type,
                     ::djinterp::internal::no_op_condvar>::value;

    static_assert(in_internal && reached_through_the_policy,
                  "the no-op condvar must live in djinterp::internal");

    return (in_internal && reached_through_the_policy);
}

/*
tests_the_null_policy_condvar_is_free
  the whole point of the null policy, end to end.
  Tests the following:
  - portable_condvar<null_lock_policy> is ONE BYTE - the empty no-op condvar is
    its only member, and nothing else is paid for
  - notify, wait and wait_for all go through the wrapper and return at once,
    with the policy's own lock type (no_op_guard) as the lock
  - so single-threaded code that was written against the condvar protocol still
    compiles, still runs, and costs nothing - subject to the predicate caveat
    two tests up, which is the price of the zero
*/
bool
tests_the_null_policy_condvar_is_free()
{
    const bool costs_one_byte =
        (sizeof(portable_condvar<null_lock_policy>) == 1);

    portable_condvar<null_lock_policy> cv;
    no_op_mutex                        mutex;
    no_op_guard                        lock(mutex);
    std::atomic<int>                   predicate_calls(0);

    const test_clock::time_point start = test_clock::now();

    cv.notify_one();
    cv.notify_all();
    cv.wait(lock);
    cv.wait(lock, constant_predicate(true, &predicate_calls));

    const std::cv_status status =
        cv.wait_for(lock, std::chrono::milliseconds(k_wait_ms));

    const bool satisfied =
        cv.wait_for(lock,
                    std::chrono::milliseconds(k_wait_ms),
                    constant_predicate(true, &predicate_calls));

    const long long elapsed = elapsed_ms(start);

    return ( costs_one_byte                        &&
             (status == std::cv_status::no_timeout) &&
             satisfied                             &&
             (elapsed < static_cast<long long>(k_immediate_ceiling_ms)) );
}


NS_END  // testing
NS_END  // djinterp
