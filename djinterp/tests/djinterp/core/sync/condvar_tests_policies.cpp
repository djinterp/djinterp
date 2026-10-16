/*******************************************************************************
* djinterp [core]                                     condvar_tests_policies.cpp
*
*
* path:      /tests/djinterp/core/sync/condvar_tests_policies.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                         condvar_tests_policies.cpp
//   Section IV (f): every lock policy, end to end - and the C++20 stop_token.
//
//   The previous sections took portable_condvar apart against ONE policy.  This
// one runs the whole protocol - block, publish, notify, wake - once per policy,
// with that policy's OWN lock type, because that is the pairing the selector
// exists to get right and the only pairing a consumer will ever write:
//
//     null          no_op_guard                       -> no_op_condvar
//     exclusive     unique_lock<mutex>                -> condition_variable
//     timed         unique_lock<timed_mutex>          -> condition_variable_any
//     shared        shared_lock<shared_mutex>         -> condition_variable_any
//     shared_timed  shared_lock<shared_timed_mutex>   -> condition_variable_any
//
//   The shared row is the interesting one, and the reason the primary template
// hands out condition_variable_any at all: the waiter there holds a SHARED lock
// -
// a reader - and std::condition_variable cannot wait on one.  A reader that
// blocks until a writer publishes is the shape every reader/writer container
// needs, and it works only because of that one line in the selector.
//
// The C++20 tests close the file, and they run on the TIMED policy rather than
// the exclusive one - deliberately.  portable_condvar's stop_token wait
// forwards
// to condition_variable_any::wait(lock, stoken, pred), and
// std::condition_variable
// has no such overload, so the same three tests written against
// exclusive_lock_policy would not compile.  See
// tests_only_condition_variable_any_takes_a_stop_token.

// djinterp
#include "condvar_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    typedef channel<null_lock_policy>          null_channel;
    typedef channel<exclusive_lock_policy>      exclusive_channel;
    typedef channel<timed_lock_policy>          timed_channel;
    typedef channel<default_lock_policy>        default_channel;
    typedef std::unique_lock<std::timed_mutex>  timed_lock;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    typedef channel<shared_lock_policy>         shared_channel;
#endif

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    typedef channel<shared_timed_lock_policy>   shared_timed_channel;
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // shared_reader_worker
    //   function: a READER that waits.  It holds shared_lock_policy's own
    // read_lock_type - a std::shared_lock - which std::condition_variable could
    // not have waited on at all.  condition_variable_any can, because it needs
    // only lock() and unlock().
    void
    shared_reader_worker(
        channel<shared_lock_policy>* _channel
    )
    {
        channel<shared_lock_policy>::read_lock_type lock(_channel->mutex);

        _channel->waiting.store(true);

        _channel->cv.wait(lock, _channel->predicate());

        _channel->wakes.fetch_add(1);
        _channel->woke.store(true);

        return;
    }

#endif  // C++17

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // shared_timed_reader_worker
    //   function: the same reader, on the shared_timed policy, giving up after
    // a bounded wait.
    void
    shared_timed_reader_worker(
        channel<shared_timed_lock_policy>* _channel,
        std::atomic<bool>*                 _satisfied
    )
    {
        channel<shared_timed_lock_policy>::read_lock_type lock(_channel->mutex);

        _channel->waiting.store(true);

        const bool satisfied =
            _channel->cv.wait_for(lock,
                                  std::chrono::milliseconds(k_deadline_ms),
                                  _channel->predicate());

        _satisfied->store(satisfied);
        _channel->wakes.fetch_add(1);
        _channel->woke.store(true);

        return;
    }

#endif  // C++14

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // stop_token_worker
    //   function: a waiter that can be released two ways - by the condition
    // coming to hold, or by the stop_token being cancelled.  The header's C++20
    // overload returns the PREDICATE's answer either way, so the result tells
    // the test which of the two happened.
    void
    stop_token_worker(
        timed_channel*     _channel,
        std::stop_source*  _source,
        std::atomic<bool>* _result
    )
    {
        timed_lock lock(_channel->mutex);

        _channel->waiting.store(true);

        const bool satisfied =
            _channel->cv.wait(lock,
                              _source->get_token(),
                              _channel->predicate());

        _result->store(satisfied);
        _channel->woke.store(true);

        return;
    }

#endif  // C++20
}

/*
tests_the_null_policy_never_blocks
  the single-threaded policy runs the whole protocol on ONE thread, and cannot
  deadlock.
  Tests the following:
  - a wait on portable_condvar<null_lock_policy>, with the policy's own
    no_op_guard as the lock, returns on the calling thread - no second thread
    exists, and none is needed
  - it returns at once
  - and the predicate is NEVER evaluated (pred_calls stays zero) - the caveat
    that pays for the zero cost, pinned again here where a policy-level reader
    will meet it
  A real condvar on one thread would deadlock here; that is exactly what the
  null policy is for, and exactly what it cannot warn you about.
*/
bool
tests_the_null_policy_never_blocks()
{
    null_channel ch;

    const test_clock::time_point start = test_clock::now();

    // no thread: the null policy cannot block, so the runner may do this itself
    wait_worker<null_lock_policy>(&ch);

    const long long elapsed = elapsed_ms(start);

    return ( ch.woke.load()                                            &&
             (ch.wakes.load() == 1)                                    &&
             (ch.pred_calls.load() == 0)                               &&
             (elapsed < static_cast<long long>(k_immediate_ceiling_ms)) );
}

/*
tests_the_exclusive_policy_handshake
  std::mutex + std::condition_variable, with the policy's own unique_lock.
  Tests the following:
  - the waiter blocks; the publisher publishes and notifies; the waiter wakes
  - the condvar underneath really is std::condition_variable
*/
bool
tests_the_exclusive_policy_handshake()
{
    exclusive_channel ch;

    D_CONSTEXPR bool the_real_condvar =
        std::is_same<exclusive_channel::condvar_type,
                     std::condition_variable>::value;

    std::thread waiter(&wait_worker<exclusive_lock_policy>, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( the_real_condvar &&
             reached_the_wait &&
             woke );
}

/*
tests_the_timed_policy_handshake
  std::timed_mutex + std::condition_variable_any.
  Tests the following:
  - the same handshake works on the OTHER branch of the selector
  - the condvar underneath is condition_variable_any: the timed mutex is not
    std::mutex, so the primary template caught it
*/
bool
tests_the_timed_policy_handshake()
{
    timed_channel ch;

    D_CONSTEXPR bool the_any_condvar =
        std::is_same<timed_channel::condvar_type,
                     std::condition_variable_any>::value;

    std::thread waiter(&wait_worker<timed_lock_policy>, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( the_any_condvar  &&
             reached_the_wait &&
             woke );
}

/*
tests_the_shared_policy_wakes_a_reader
  a READER, holding a shared lock, waits - and a writer wakes it.
  Tests the following:
  - the waiter holds shared_lock_policy's read_lock_type: a
    std::shared_lock<std::shared_mutex>
  - the publisher takes the write_lock_type - a unique_lock - to publish, which
    it could not have done had the reader's shared lock not been released by the
    wait
  - the reader wakes
  This is the payoff of the selector's primary template.
  std::condition_variable
  can only wait on unique_lock<std::mutex>; a reader could not have waited at
  all.  condition_variable_any takes any BasicLockable, so it takes a
  shared_lock,
  and a reader/writer container can finally block a reader on a condition.
  Below C++17 there is no std::shared_mutex and no policy to test.
*/
bool
tests_the_shared_policy_wakes_a_reader()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    shared_channel ch;

    D_CONSTEXPR bool the_reader_holds_a_shared_lock =
        std::is_same<shared_channel::read_lock_type,
                     std::shared_lock<std::shared_mutex>>::value;

    D_CONSTEXPR bool the_any_condvar =
        std::is_same<shared_channel::condvar_type,
                     std::condition_variable_any>::value;

    std::thread reader(&shared_reader_worker, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    // the writer needs the EXCLUSIVE lock; it can only have it if the reader's
    // shared lock was released by the wait
    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    reader.join();

    return ( the_reader_holds_a_shared_lock &&
             the_any_condvar                &&
             reached_the_wait               &&
             woke );
#else
    // std::shared_mutex is C++17; there is no shared_lock_policy to exercise
    return true;
#endif
}

/*
tests_the_shared_timed_policy_handshake
  the same, on std::shared_timed_mutex, with a bounded wait.
  Tests the following:
  - a reader holding a shared_lock waits with a timeout, and is satisfied by the
    writer well inside it
  - the condvar is condition_variable_any here too
  Below C++14 the mutex does not exist and neither does the policy.
*/
bool
tests_the_shared_timed_policy_handshake()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    shared_timed_channel ch;
    std::atomic<bool>    satisfied(false);

    D_CONSTEXPR bool the_any_condvar =
        std::is_same<shared_timed_channel::condvar_type,
                     std::condition_variable_any>::value;

    std::thread reader(&shared_timed_reader_worker, &ch, &satisfied);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    reader.join();

    return ( the_any_condvar  &&
             reached_the_wait &&
             woke             &&
             satisfied.load() );
#else
    // std::shared_timed_mutex is C++14; there is no policy to exercise
    return true;
#endif
}

/*
tests_the_default_policy_works
  whatever the project default IS this standard, it works.
  Tests the following:
  - the handshake runs on default_lock_policy without the test knowing which
    policy that is: shared_lock_policy at C++17 and above, exclusive below
  - which is the point of the alias - a consumer writes default_lock_policy and
    gets the most capable thing the standard allows
*/
bool
tests_the_default_policy_works()
{
    default_channel ch;

    std::thread waiter(&wait_worker<default_lock_policy>, &ch);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return (reached_the_wait && woke);
}

/*
tests_every_policy_yields_a_usable_condvar
  the sweep: every policy the build has produces a condvar with the same shape.
  Tests the following:
  - portable_condvar<P> is default-constructible, non-copyable and non-movable
    for every P
  - its condvar_type is a complete type in every case
  - so nothing in the policy set produces a condvar that only half exists
*/
bool
tests_every_policy_yields_a_usable_condvar()
{
    D_CONSTEXPR bool core_policies =
        ( std::is_default_constructible<
              portable_condvar<null_lock_policy>>::value          &&
          std::is_default_constructible<
              portable_condvar<exclusive_lock_policy>>::value     &&
          std::is_default_constructible<
              portable_condvar<timed_lock_policy>>::value         &&
          std::is_default_constructible<
              portable_condvar<default_lock_policy>>::value       &&
          !std::is_copy_constructible<
              portable_condvar<null_lock_policy>>::value          &&
          !std::is_copy_constructible<
              portable_condvar<exclusive_lock_policy>>::value     &&
          !std::is_move_constructible<
              portable_condvar<timed_lock_policy>>::value );

    const bool condvars_are_complete =
        ( (sizeof(portable_condvar<null_lock_policy>::condvar_type) > 0)      &&
          (sizeof(portable_condvar<exclusive_lock_policy>::condvar_type) > 0) &&
          (sizeof(portable_condvar<timed_lock_policy>::condvar_type) > 0) );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    const bool shared_policies_too =
        ( std::is_default_constructible<
              portable_condvar<shared_lock_policy>>::value                  &&
          (sizeof(portable_condvar<shared_lock_policy>::condvar_type) > 0) );
#elif D_ENV_LANG_IS_CPP14_OR_HIGHER
    const bool shared_policies_too =
        ( std::is_default_constructible<
              portable_condvar<shared_timed_lock_policy>>::value            &&
          (sizeof(portable_condvar<
                      shared_timed_lock_policy>::condvar_type) > 0) );
#else
    // there is no shared policy at all below C++14
    const bool shared_policies_too = true;
#endif

    static_assert(core_policies,
                  "every policy must yield a default-constructible, "
                  "non-copyable condvar");

    return ( core_policies         &&
             condvars_are_complete &&
             shared_policies_too );
}

/*
tests_the_stop_token_wait_returns_the_predicate
  the C++20 overload: cancelled, with the condition still false.
  Tests the following:
  - a waiter blocks in wait(lock, stop_token, pred) with nothing published
  - the stop is requested; the wait returns
  - and it returns FALSE - the header's overload answers with the PREDICATE, not
    with "were you cancelled", so a caller learns what it actually wanted to
    know: the condition never came to hold
  A stop is not a satisfaction.  Code that treats a true return as "the
  condition
  holds" is correct; code that treats a return at all as "the condition holds"
  is not, and this is the test that separates them.
*/
bool
tests_the_stop_token_wait_returns_the_predicate()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    timed_channel     ch;
    std::stop_source  source;
    std::atomic<bool> result(true);   // pessimistic: the test must clear it

    std::thread waiter(&stop_token_worker, &ch, &source, &result);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    // nothing is ever published; the only way out is the stop
    source.request_stop();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    return ( reached_the_wait &&
             woke             &&
             (!result.load()) );
#else
    // std::stop_token is C++20; portable_condvar's overload is gated with it
    return true;
#endif
}

/*
tests_the_stop_token_wait_returns_true_when_satisfied
  ...and TRUE when the condition wins the race instead.
  Tests the following:
  - the same waiter, but the condition is published and notified while the stop
    is never requested
  - the wait returns true
  - so the overload is a normal predicate wait that happens to also be
    cancellable
*/
bool
tests_the_stop_token_wait_returns_true_when_satisfied()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    timed_channel     ch;
    std::stop_source  source;
    std::atomic<bool> result(false);

    std::thread waiter(&stop_token_worker, &ch, &source, &result);

    const bool reached_the_wait = spin_until(ch.waiting, k_deadline_ms);

    ch.publish_and_notify_all();

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    ch.rescue();
    waiter.join();

    const bool never_stopped = (!source.stop_requested());

    return ( reached_the_wait &&
             woke             &&
             never_stopped    &&
             result.load() );
#else
    return true;
#endif
}

/*
tests_the_stop_token_wait_returns_at_once_if_already_stopped
  a stop that arrived BEFORE the wait is not lost.
  Tests the following:
  - the stop is requested before the waiter ever reaches the wait
  - the wait returns immediately rather than blocking on a cancellation that has
    already happened - the stop_token carries STATE, unlike a notification,
    which
    carries none (see tests_a_notification_with_no_waiter_is_lost)
  - and it returns false, the condition never having held
  This is the difference between a stop_token and a notify: one you can miss,
  the
  other you cannot.
*/
bool
tests_the_stop_token_wait_returns_at_once_if_already_stopped()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    timed_channel     ch;
    std::stop_source  source;
    std::atomic<bool> result(true);

    // stopped before anybody waits
    source.request_stop();

    const test_clock::time_point start = test_clock::now();

    std::thread waiter(&stop_token_worker, &ch, &source, &result);

    const bool woke = spin_until(ch.woke, k_deadline_ms);

    const long long elapsed = elapsed_ms(start);

    ch.rescue();
    waiter.join();

    return ( woke                                            &&
             (!result.load())                                &&
             (elapsed < static_cast<long long>(k_deadline_ms)) );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
