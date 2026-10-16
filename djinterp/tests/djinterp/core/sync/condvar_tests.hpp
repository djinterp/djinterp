/*******************************************************************************
* djinterp [test]                                              condvar_tests.hpp
*
*   Declarations for the condvar.hpp unit-test suite, plus the helper types the
* sections share.
*
*   WHY THE HARNESS IS BUILT FROM std AND NOTHING ELSE:
*   The framework's own concurrency helpers - test_latch, test_barrier,
* test_gate, simultaneous_start (test_sync.hpp) - are implemented ON TOP of
* portable_condvar and exclusive_lock_policy.  They are DOWNSTREAM of the header
* under test.  Orchestrating this suite with them would be circular: a broken
* portable_condvar would break the barrier that was supposed to expose it, and
* the suite could pass, hang, or lie.  So every rendezvous below is built from
* std::atomic + std::thread + std::chrono only - a harness that cannot be broken
* by the thing it is testing.
*
*   WHY NOTHING HANGS:
*   A condvar test that fails is a condvar test that BLOCKS, and a blocked test
* takes the whole runner down with it.  Every waiter here therefore waits on a
* predicate that ORs in an `abort` flag, and every test's last act is rescue() -
* set abort, notify everyone - before it joins.  A broken wait yields a reported
* FAILURE (the deadline elapsed) rather than a hung process.  Spin-waits sleep
* rather than spin hot, so the suite is correct on a single-core box too.
*
*   WHAT THE SUITE PINS THAT THE HEADER DOES NOT SAY:
*   Three facts fell out of testing and are pinned deliberately, each in a named
* test, because code that relies on the header's "same code, locked or not"
* promise will meet them:
*     - no_op_condvar has NO wait_until, so portable_condvar<null_lock_policy>
* ::wait_until does not compile (tests_only_the_real_condvars_have_wait_until);
*     - std::condition_variable has no stop_token overload, so the C++20 wait()
*       does not compile for a std::mutex policy - only for the policies that
*       select condition_variable_any (tests_only_condition_variable_any_takes_
*       a_stop_token);
*     - no_op_condvar::wait(lock, pred) IGNORES the predicate entirely (it is
*       never even invoked), while wait_for(lock, dur, pred) calls it once and
*       returns it (tests_no_op_wait_ignores_its_predicate).
*
*   NOTE ON INCLUDES: this header deliberately does NOT include <mutex>,
* <condition_variable>, <shared_mutex> or <stop_token>.  condvar.hpp claims to
* supply them, and tests_header_provides_the_std_facilities is only an honest
* test of that claim from a TU that could not have gotten them anywhere else.
*
*   PORTABILITY: the suite requires C++11 (the framework's floor, and the gate
* sections III and IV of the header sit behind).  The C++14 / C++17 / C++20
* faces - shared_timed_lock_policy, shared_lock_policy, the stop_token wait -
* are gated where they appear, and their tests degrade to a stated no-op below
* their standard rather than being silently dropped.
*
*
* path:      /tests/djinterp/core/sync/condvar_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      -------
      a. clocks, deadlines, spin-waits
      b. capability detection (SFINAE)
      c.    probes: predicates, locks, callables
            d. the channel: a policy-parameterized rendezvous

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.   thread yield
      c.    II.  hardware concurrency
            d. III. portable_once
            e. IV.  the condvar selector
            f. IV.  the no-op condvar
            g. IV.  portable_condvar's surface
            h. IV.  wait + notify
      i.    IV.  timed waits
            j. IV.  the lock policies
*/

#ifndef DJINTERP_TESTS_CONDVAR_
#define DJINTERP_TESTS_CONDVAR_ 1

// std
// std  (NOT <mutex> / <condition_variable> / <shared_mutex> / <stop_token> -
//       see NOTE ON INCLUDES above)
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/core/sync/condvar.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "condvar_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// a.   clocks, deadlines, spin-waits
//////////////////////////////////////////

// test_clock
//   type: the suite's clock.  steady_clock, never system_clock: a wall-clock
// step would make every timing assertion below a lie.
typedef std::chrono::steady_clock test_clock;

// k_deadline_ms
//   constant: how long a test waits for something that MUST happen before it
// calls the wait broken.  Generous on purpose - it is a liveness bound, not a
// performance one.
D_STATIC D_CONSTEXPR unsigned k_deadline_ms = 3000;

// k_wait_ms
//   constant: the timeout a test hands to wait_for / wait_until when it wants
// the wait to EXPIRE.
D_STATIC D_CONSTEXPR unsigned k_wait_ms = 80;

// k_settle_ms
//   constant: how long a test watches a waiter it believes should still be
// blocked, before concluding that it is.
D_STATIC D_CONSTEXPR unsigned k_settle_ms = 80;

// k_expired_floor_ms
//   constant: the least a wait_for(k_wait_ms) may take and still be said to
// have waited.  Three quarters of the ask - slack for a coarse timer, not for
// a returning-early bug.
D_STATIC D_CONSTEXPR unsigned k_expired_floor_ms = (k_wait_ms * 3) / 4;

// k_immediate_ceiling_ms
//   constant: the most an "it returns at once" path may take.  Half the ask,
// so a wait that actually waited cannot pass as an immediate return.
D_STATIC D_CONSTEXPR unsigned k_immediate_ceiling_ms = k_wait_ms / 2;

// k_waiters
//   constant: how many waiters a multi-waiter test starts.
D_STATIC D_CONSTEXPR int k_waiters = 4;

// sleep_ms
//   function: park this thread for _ms milliseconds.
D_INLINE void
sleep_ms(
    unsigned _ms
)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

    return;
}

// elapsed_ms
//   function: milliseconds since _start, as a plain integer.
D_INLINE long long
elapsed_ms(
    const test_clock::time_point& _start
)
{
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            test_clock::now() - _start).count());
}

// spin_until
//   function: wait for the flag to go true, up to _ms.  Returns false if the
// deadline passed first - which is how a broken wait becomes a FAILED test
// instead of a hung runner.
//
//   It SLEEPS between polls rather than spinning hot: a hot spin on a
// single-core box can starve the very thread it is waiting for.
D_INLINE bool
spin_until(
    const std::atomic<bool>& _flag,
    unsigned                 _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (!_flag.load())
    {
        // check if the deadline has passed
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}

// spin_until_count
//   function: as spin_until, for a counter reaching _target.
D_INLINE bool
spin_until_count(
    const std::atomic<int>& _counter,
    int                     _target,
    unsigned                _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (_counter.load() < _target)
    {
        // check if the deadline has passed
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}


// b.   capability detection (SFINAE)
//////////////////////////////////////////
//   These probe the SELECTED condvar type - std::condition_variable,
// std::condition_variable_any, internal::no_op_condvar - and never
// portable_condvar itself.  That is not a stylistic choice: portable_condvar's
// waits are member TEMPLATES that forward, so a missing member on the wrapped
// condvar is a hard error inside the body, not a substitution failure in the
// immediate context.  A probe aimed at the wrapper would break the build it was
// meant to interrogate; a probe aimed at the wrapped type answers cleanly.

// has_notify
//   trait: the condvar can notify_one() and notify_all().
template<typename _CondVar,
         typename _Enable = void>
struct has_notify : std::false_type
{};

template<typename _CondVar>
struct has_notify<_CondVar,
                  void_t<decltype(std::declval<_CondVar&>().notify_one()),
                         decltype(std::declval<_CondVar&>().notify_all())>>
    : std::true_type
{};

// has_wait
//   trait: the condvar can wait(lock).
template<typename _CondVar,
         typename _Lock,
         typename _Enable = void>
struct has_wait : std::false_type
{};

template<typename _CondVar,
         typename _Lock>
struct has_wait<_CondVar,
                _Lock,
                void_t<decltype(std::declval<_CondVar&>().wait(
                           std::declval<_Lock&>()))>>
    : std::true_type
{};

// has_wait_predicate
//   trait: the condvar can wait(lock, predicate).
template<typename _CondVar,
         typename _Lock,
         typename _Predicate,
         typename _Enable = void>
struct has_wait_predicate : std::false_type
{};

template<typename _CondVar,
         typename _Lock,
         typename _Predicate>
struct has_wait_predicate<_CondVar,
                          _Lock,
                          _Predicate,
                          void_t<decltype(std::declval<_CondVar&>().wait(
                                     std::declval<_Lock&>(),
                                     std::declval<_Predicate>()))>>
    : std::true_type
{};

// has_wait_for
//   trait: the condvar can wait_for(lock, duration).
template<typename _CondVar,
         typename _Lock,
         typename _Enable = void>
struct has_wait_for : std::false_type
{};

template<typename _CondVar,
         typename _Lock>
struct has_wait_for<_CondVar,
                    _Lock,
                    void_t<decltype(std::declval<_CondVar&>().wait_for(
                               std::declval<_Lock&>(),
                               std::declval<
                                   const std::chrono::milliseconds&>()))>>
    : std::true_type
{};

// has_wait_until
//   trait: the condvar can wait_until(lock, time_point).  FALSE for
// no_op_condvar - which is the gap tests_only_the_real_condvars_have_wait_until
// pins.
template<typename _CondVar,
         typename _Lock,
         typename _Enable = void>
struct has_wait_until : std::false_type
{};

template<typename _CondVar,
         typename _Lock>
struct has_wait_until<_CondVar,
                      _Lock,
                      void_t<decltype(std::declval<_CondVar&>().wait_until(
                                 std::declval<_Lock&>(),
                                 std::declval<
                                     const test_clock::time_point&>()))>>
    : std::true_type
{};

// has_predicate_wait_until
//   trait: wait_until(lock, deadline, predicate) - the THREE-argument form.
// std::condition_variable and std::condition_variable_any both have it;
// portable_condvar does NOT forward it, and this is the trait that says so.
//
//   Safe to aim at the WRAPPER, unlike the two-argument probes above: a
// three-argument call does not match portable_condvar's two-parameter
// wait_until
// template at all, so deduction fails in the immediate context and no body is
// ever instantiated.
template<typename _CondVar,
         typename _Lock,
         typename _Predicate,
         typename _Enable = void>
struct has_predicate_wait_until : std::false_type
{};

template<typename _CondVar,
         typename _Lock,
         typename _Predicate>
struct has_predicate_wait_until<_CondVar,
                                _Lock,
                                _Predicate,
                                void_t<decltype(
                                    std::declval<_CondVar&>().wait_until(
                                        std::declval<_Lock&>(),
                                        std::declval<
                                            const test_clock::time_point&>(),
                                        std::declval<_Predicate>()))>>
    : std::true_type
{};

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// has_stop_token_wait
//   trait: the condvar can wait(lock, stop_token, predicate).  TRUE only for
// std::condition_variable_any - which is what makes portable_condvar's C++20
// overload unusable under a std::mutex policy.
template<typename _CondVar,
         typename _Lock,
         typename _Predicate,
         typename _Enable = void>
struct has_stop_token_wait : std::false_type
{};

template<typename _CondVar,
         typename _Lock,
         typename _Predicate>
struct has_stop_token_wait<_CondVar,
                           _Lock,
                           _Predicate,
                           void_t<decltype(std::declval<_CondVar&>().wait(
                                      std::declval<_Lock&>(),
                                      std::declval<std::stop_token>(),
                                      std::declval<_Predicate>()))>>
    : std::true_type
{};

#endif  // C++20


// c.   probes: predicates, locks, callables
//////////////////////////////////////////

// flag_predicate
//   type: the predicate every waiter in the suite waits on.  It is COPYABLE and
// holds only pointers, because a condvar takes its predicate BY VALUE - a
// predicate that owned its counter would count into a copy and report nothing.
//
//   It returns (ready || abort).  The abort arm is the rescue hatch: it lets a
// test release a waiter it can no longer satisfy, so a failure is reported
// rather than hung.
struct flag_predicate
{
    const std::atomic<bool>* ready;
    const std::atomic<bool>* abort;
    std::atomic<int>*        calls;

    flag_predicate(
        const std::atomic<bool>* _ready,
        const std::atomic<bool>* _abort,
        std::atomic<int>*        _calls
    )
        : ready(_ready),
          abort(_abort),
          calls(_calls)
    {}

    bool
    operator()() const
    {
        calls->fetch_add(1);

        return ( ready->load() ||
                 abort->load() );
    }
};

// constant_predicate
//   type: a copyable predicate that always answers the same way, and counts how
// often it was asked.  The no-op condvar's treatment of a FALSE predicate is
// the sharpest thing in the header; this is what observes it.
struct constant_predicate
{
    bool              answer;
    std::atomic<int>* calls;

    constant_predicate(
        bool              _answer,
        std::atomic<int>* _calls
    )
        : answer(_answer),
          calls(_calls)
    {}

    bool
    operator()() const
    {
        calls->fetch_add(1);

        return answer;
    }
};

// recording_lock
//   type: a BasicLockable that records every lock() and unlock() it is given.
// A real condvar UNLOCKS on the way into a wait and re-locks on the way out;
// the no-op condvar does neither, and this is what proves it.
struct recording_lock
{
    int locks;
    int unlocks;

    recording_lock()
        : locks(0),
          unlocks(0)
    {}

    void lock()   { ++locks;   return; }
    void unlock() { ++unlocks; return; }
};

// mock_mutex
//   type: a BasicLockable that is not one of the standard mutexes - the input
// the condvar_selector's PRIMARY template is meant to catch.
struct mock_mutex
{
    void lock()     { return; }
    void unlock()   { return; }
    bool try_lock() { return true; }
};

// call_counter
//   type: a copyable niladic callable that counts its invocations through a
// pointer.  portable_once takes its callable by forwarding reference, but
// std::call_once may copy it; the pointer keeps the count honest either way.
struct call_counter
{
    std::atomic<int>* calls;

    explicit call_counter(
        std::atomic<int>* _calls
    )
        : calls(_calls)
    {}

    void
    operator()() const
    {
        calls->fetch_add(1);

        return;
    }
};


// d.   the channel: a policy-parameterized rendezvous
//////////////////////////////////////////

// channel
//   type: one mutex, one portable_condvar, and the shared state a waiter waits
// on - the whole of the canonical condvar protocol, in one object, for whatever
// lock policy it is given.
//
//   The state is atomic AND published under the lock.  The lock is what closes
// the notify/wait race; the atomics are what let the TEST thread observe the
// waiter without taking part in the protocol it is testing.
template<typename _Policy>
struct channel
{
    typedef _Policy                                policy_type;
    typedef typename _Policy::mutex_type           mutex_type;
    typedef typename _Policy::write_lock_type      lock_type;
    typedef typename _Policy::read_lock_type       read_lock_type;
    typedef typename portable_condvar<_Policy>::condvar_type condvar_type;

    mutex_type                mutex;
    portable_condvar<_Policy> cv;

    std::atomic<bool> ready;       // the condition the waiters wait for
    std::atomic<bool> abort;       // the rescue hatch (see flag_predicate)
    std::atomic<bool> waiting;     // a waiter announced it is about to block
    std::atomic<bool> woke;        // a waiter came back
    std::atomic<int>  wakes;       // how many waiters came back
    std::atomic<int>  pred_calls;  // how often the predicate was asked
    std::atomic<bool> relocked;    // did the waiter hold the lock on return?

    channel()
        : mutex(),
          cv(),
          ready(false),
          abort(false),
          waiting(false),
          woke(false),
          wakes(0),
          pred_calls(0),
          relocked(false)
    {}

    channel(const channel&)            D_DELETE;
    channel& operator=(const channel&) D_DELETE;

    // predicate
    //   the waiters' predicate over this channel's state.
    flag_predicate
    predicate()
    {
        return flag_predicate(&ready, &abort, &pred_calls);
    }

    // publish
    //   sets the condition UNDER THE LOCK.  Publishing outside the lock is the
    // classic lost-wakeup bug; the suite does not commit it, so that the one
    // test that DOES lose a notification loses it on purpose.
    void
    publish()
    {
        lock_type lock(mutex);

        ready.store(true);

        return;
    }

    // publish_and_notify_all / publish_and_notify_one
    //   the producer's half of the protocol: publish, release, notify.
    void
    publish_and_notify_all()
    {
        publish();
        cv.notify_all();

        return;
    }

    void
    publish_and_notify_one()
    {
        publish();
        cv.notify_one();

        return;
    }

    // rescue
    //   the teardown every test runs before joining: release any waiter still
    // blocked, whatever the test concluded.  A failing assertion must not
    // become a hanging runner.
    void
    rescue()
    {
        {
            lock_type lock(mutex);

            abort.store(true);
        }

        cv.notify_all();

        return;
    }
};

// wait_worker
//   function: the canonical consumer, run on a worker thread - take the lock,
// wait on the predicate, come back.  The predicate carries the abort arm, so
// this worker can always be released.
template<typename _Policy>
void
wait_worker(
    channel<_Policy>* _channel
)
{
    typename channel<_Policy>::lock_type lock(_channel->mutex);

    _channel->waiting.store(true);

    _channel->cv.wait(lock, _channel->predicate());

    _channel->wakes.fetch_add(1);
    _channel->woke.store(true);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_inherits_its_guards_from_lock_policy();
bool tests_header_pulls_its_dependencies();
bool tests_header_provides_the_std_facilities();
bool tests_header_defines_no_stray_macros();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_sections_three_and_four_are_gated_to_cpp11();

// b.   I.  thread yield
//////////////////////////////////////////
bool tests_yield_is_a_nullary_void_function();
bool tests_yield_is_callable();
bool tests_yield_does_not_throw();
bool tests_yield_is_not_declared_noexcept();
bool tests_yield_from_many_threads();
bool tests_yield_in_a_spin_loop_makes_progress();
bool tests_yield_has_one_definition_across_tus();

// c.   II. hardware concurrency
//////////////////////////////////////////
bool tests_hardware_returns_unsigned();
bool tests_hardware_is_deterministic();
bool tests_hardware_may_return_zero();
bool tests_hardware_is_plausible();
bool tests_hardware_matches_the_standard();
bool tests_hardware_does_not_throw();
bool tests_hardware_is_callable_from_many_threads();
bool tests_hardware_has_one_definition_across_tus();

// d.   III. portable_once
//////////////////////////////////////////
bool tests_once_is_default_constructible();
bool tests_once_is_not_copyable();
bool tests_once_is_not_movable();
bool tests_once_holds_one_flag_and_nothing_else();
bool tests_once_runs_the_callable();
bool tests_once_runs_it_only_once();
bool tests_once_ignores_a_later_callable();
bool tests_once_runs_exactly_once_under_contention();
bool tests_once_publishes_the_result_to_every_caller();
bool tests_once_forwards_its_arguments();
bool tests_once_forwards_by_reference();
bool tests_once_accepts_any_callable();
bool tests_once_propagates_an_exception();
bool tests_once_retries_after_an_exception();
bool tests_once_instances_are_independent();

// e.   IV. the condvar selector
//////////////////////////////////////////
bool tests_selector_maps_std_mutex_to_condition_variable();
bool tests_selector_maps_no_op_mutex_to_the_no_op_condvar();
bool tests_selector_falls_back_to_condition_variable_any();
bool tests_selector_maps_the_shared_mutexes();
bool tests_selector_primary_accepts_any_type();
bool tests_every_policy_selects_its_condvar();
bool tests_every_condvar_can_notify();
bool tests_every_condvar_can_wait();
bool tests_every_condvar_can_wait_for();
bool tests_only_the_real_condvars_have_wait_until();
bool tests_only_condition_variable_any_takes_a_stop_token();

// f.   IV. the no-op condvar
//////////////////////////////////////////
bool tests_no_op_condvar_is_empty();
bool tests_no_op_notify_is_a_noexcept_no_op();
bool tests_no_op_wait_returns_immediately();
bool tests_no_op_wait_never_touches_the_lock();
bool tests_no_op_wait_ignores_its_predicate();
bool tests_no_op_wait_for_claims_no_timeout();
bool tests_no_op_wait_for_does_not_sleep();
bool tests_no_op_wait_for_with_a_predicate_returns_it();
bool tests_no_op_condvar_lives_in_internal();
bool tests_the_null_policy_condvar_is_free();

// g.   IV. portable_condvar's surface
//////////////////////////////////////////
bool tests_condvar_is_default_constructible();
bool tests_condvar_is_not_copyable();
bool tests_condvar_is_not_movable();
bool tests_condvar_exposes_its_condvar_type();
bool tests_condvar_notify_is_noexcept();
bool tests_condvar_native_returns_the_underlying();
bool tests_condvar_native_has_a_const_overload();
bool tests_condvar_costs_exactly_its_condvar();
bool tests_condvar_has_no_vtable();
bool tests_condvar_notify_without_waiters_is_harmless();
bool tests_condvar_instances_are_independent();

// h.   IV. wait + notify
//////////////////////////////////////////
bool tests_wait_blocks_until_notified();
bool tests_wait_releases_the_mutex_while_blocked();
bool tests_wait_reacquires_the_mutex_before_returning();
bool tests_wait_with_a_predicate_returns_when_it_holds();
bool tests_wait_with_a_true_predicate_never_blocks();
bool tests_wait_ignores_a_notification_while_the_predicate_is_false();
bool tests_notify_one_wakes_a_waiter();
bool tests_notify_all_wakes_every_waiter();
bool tests_notify_one_wakes_at_least_one_of_many();
bool tests_a_notification_with_no_waiter_is_lost();

// i.   IV. timed waits
//////////////////////////////////////////
bool tests_wait_for_times_out();
bool tests_wait_for_returns_no_timeout_when_notified();
bool tests_wait_for_with_a_predicate_returns_false_on_timeout();
bool tests_wait_for_with_a_predicate_returns_true_when_satisfied();
bool tests_wait_for_with_a_true_predicate_returns_at_once();
bool tests_wait_for_with_a_zero_duration_times_out();
bool tests_wait_for_keeps_waiting_while_the_predicate_is_false();
bool tests_wait_until_times_out();
bool tests_wait_until_with_a_past_deadline_returns_at_once();
bool tests_wait_until_returns_no_timeout_when_notified();
bool tests_wait_for_on_the_condition_variable_any_path();
bool tests_wait_until_has_no_predicate_overload();

// j.   IV. the lock policies
//////////////////////////////////////////
bool tests_the_null_policy_never_blocks();
bool tests_the_exclusive_policy_handshake();
bool tests_the_timed_policy_handshake();
bool tests_the_shared_policy_wakes_a_reader();
bool tests_the_shared_timed_policy_handshake();
bool tests_the_default_policy_works();
bool tests_every_policy_yields_a_usable_condvar();
bool tests_the_stop_token_wait_returns_the_predicate();
bool tests_the_stop_token_wait_returns_true_when_satisfied();
bool tests_the_stop_token_wait_returns_at_once_if_already_stopped();


// cross-TU probes
//////////////////////////////////////////
//   Both free functions in the header are `inline`, so every TU that includes
// it gets a definition and the linker must fold them into ONE.  These take the
// address in a DIFFERENT TU from the test that checks it - the only way to
// observe that folding from inside the program.

// yield_address_from_another_tu
//   function: &d_thread_yield, as seen from the hardware-concurrency TU.
void* yield_address_from_another_tu();

// hardware_address_from_another_tu
//   function: &hardware_concurrency, as seen from the thread-yield TU.
void* hardware_address_from_another_tu();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONDVAR_
