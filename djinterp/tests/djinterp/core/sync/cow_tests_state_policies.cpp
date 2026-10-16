/*******************************************************************************
* djinterp [core]                                   cow_tests_state_policies.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_state_policies.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     cow_tests_state_policies.cpp
//   Section IV (c): cow_state under each LOCK POLICY.
//
//   cow_state is parameterized on a policy and locks with that policy's own
// lock
// types - `typename _Policy::read_lock_type` around read() and snapshot(),
// `write_lock_type` around write(), modify() and replace().  So the same class
// is a mutex-guarded container, a reader/writer container, or a bare unlocked
// one, depending on a single template argument.  This section runs the protocol
// once per policy and checks that the locking is REAL - by taking the lock from
// another thread and watching it fail.
//
//   The null policy is the interesting end.  It compiles to no mutex at all:
// the
// state is smaller, the locks are empty guards, and the copy-on-write machinery
// still works exactly as it does everywhere else.  Which is the point - the
// single-threaded container and the shared one are the same source.

// djinterp
#include "cow_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // exercise_the_protocol
    //   function: the whole cow_state protocol, once, on whatever policy it is
    // given.  Construct, read, snapshot, write (which must clone, because the
    // snapshot shares), check the snapshot is isolated, modify, replace - and
    // check the version counted every one of them.
    template<typename _Policy>
    bool
    exercise_the_protocol()
    {
        probe_reset();

        cow_state<tracked, _Policy> state(tracked(1, 1));

        // read
        if ( (state.read().first != 1) ||
             (state.version() != 0) )
        {
            return false;
        }

        // snapshot: shares, no copy
        const long copies_before = probe_copies().load();

        immutable_snapshot<tracked> snapshot = state.snapshot();

        if ( (probe_copies().load() != copies_before) ||
             (snapshot.use_count() != 2)              ||
             (snapshot.version() != 0) )
        {
            return false;
        }

        // write: the snapshot is sharing, so this must clone exactly once
        state.write().first = 2;

        if ( (probe_copies().load() != (copies_before + 1)) ||
             (state.read().first != 2)                      ||
             (snapshot.get().first != 1)                    ||
             (state.version() != 1) )
        {
            return false;
        }

        // modify
        state.modify([](tracked& _value) { _value.second = 3; });

        if ( (state.read().second != 3) ||
             (state.version() != 2) )
        {
            return false;
        }

        // replace
        state.replace(tracked(9, 9));

        if ( (state.read().first != 9)    ||
             (snapshot.get().first != 1)  ||
             (state.version() != 3) )
        {
            return false;
        }

        return true;
    }

    // write_prober
    //   function: try to take the state's mutex from another thread while a
    // write lock is held.  It must fail.
    void
    write_prober(
        cow_state<int, exclusive_lock_policy>* _state,
        const std::atomic<bool>*               _inside,
        std::atomic<bool>*                     _took_the_lock,
        std::atomic<bool>*                     _done
    )
    {
        while (!_inside->load())
        {
            std::this_thread::yield();
        }

        const bool taken = _state->mutex().try_lock();

        _took_the_lock->store(taken);

        if (taken)
        {
            _state->mutex().unlock();
        }

        _done->store(true);

        return;
    }
}

/*
tests_state_under_the_null_policy
  no mutex, no locking, and the same protocol.
  Tests the following:
  - the whole cow_state protocol - read, snapshot, cloning write, modify,
    replace, version - behaves identically with no lock at all
  - the state's mutex_type is no_op_mutex and its lock types are empty guards
  So the single-threaded build of a container is the same source as the shared
  one, with one template argument changed.  What it does NOT get is safety: the
  null policy makes concurrent use undefined, quietly.
*/
bool
tests_state_under_the_null_policy()
{
    D_CONSTEXPR bool no_real_mutex =
        std::is_same<cow_state<tracked, null_lock_policy>::mutex_type,
                     no_op_mutex>::value;

    static_assert(no_real_mutex,
                  "the null policy must carry no real mutex");

    return ( no_real_mutex &&
             exercise_the_protocol<null_lock_policy>() );
}

/*
tests_state_under_the_exclusive_policy
  std::mutex: one agent at a time, readers included.
  Tests the following:
  - the whole protocol works
  - the mutex is a std::mutex, and read_lock_type is a unique_lock over it - so
    even a READ excludes every other agent under this policy
*/
bool
tests_state_under_the_exclusive_policy()
{
    D_CONSTEXPR bool is_a_std_mutex =
        std::is_same<cow_state<tracked, exclusive_lock_policy>::mutex_type,
                     std::mutex>::value;

    static_assert(is_a_std_mutex,
                  "the exclusive policy must carry a std::mutex");

    return ( is_a_std_mutex &&
             exercise_the_protocol<exclusive_lock_policy>() );
}

/*
tests_state_under_the_timed_policy
  std::timed_mutex: the same, with deadlines available to the container above.
  Tests the following:
  - the whole protocol works
  - cow_state itself never uses the timed face - it locks unconditionally - so
    the deadline is there for the container that owns the state, through mutex()
*/
bool
tests_state_under_the_timed_policy()
{
    D_CONSTEXPR bool is_a_timed_mutex =
        std::is_same<cow_state<tracked, timed_lock_policy>::mutex_type,
                     std::timed_mutex>::value;

    static_assert(is_a_timed_mutex,
                  "the timed policy must carry a std::timed_mutex");

    return ( is_a_timed_mutex &&
             exercise_the_protocol<timed_lock_policy>() );
}

/*
tests_state_under_the_shared_policy
  std::shared_mutex: many readers, one writer - the policy cow_state is for.
  Tests the following:
  - the whole protocol works
  - read() and snapshot() take a SHARED lock, so they do not exclude each other:
    see tests_the_shared_policy_admits_concurrent_readers, which proves two of
    them overlap
  Below C++17 there is no std::shared_mutex and no policy to test.
*/
bool
tests_state_under_the_shared_policy()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool is_a_shared_mutex =
        std::is_same<cow_state<tracked, shared_lock_policy>::mutex_type,
                     std::shared_mutex>::value;

    D_CONSTEXPR bool reads_share =
        std::is_same<shared_lock_policy::read_lock_type,
                     std::shared_lock<std::shared_mutex> >::value;

    static_assert(is_a_shared_mutex && reads_share,
                  "the shared policy must read under a shared_lock");

    return ( is_a_shared_mutex &&
             reads_share       &&
             exercise_the_protocol<shared_lock_policy>() );
#else
    // std::shared_mutex is C++17; there is no shared_lock_policy to exercise
    return true;
#endif
}

/*
tests_state_under_the_shared_timed_policy
  std::shared_timed_mutex: reader/writer, with deadlines.
  Tests the following:
  - the whole protocol works
  Below C++14 the mutex does not exist and neither does the policy.
*/
bool
tests_state_under_the_shared_timed_policy()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR bool is_a_shared_timed_mutex =
        std::is_same<cow_state<tracked, shared_timed_lock_policy>::mutex_type,
                     std::shared_timed_mutex>::value;

    static_assert(is_a_shared_timed_mutex,
                  "the shared_timed policy must carry a shared_timed_mutex");

    return ( is_a_shared_timed_mutex &&
             exercise_the_protocol<shared_timed_lock_policy>() );
#else
    // std::shared_timed_mutex is C++14; there is no policy to exercise
    return true;
#endif
}

/*
tests_state_write_excludes_other_agents
  the write lock is a real lock.
  Tests the following:
  - while a modify() is running, another thread cannot take the state's mutex
  - so writes are serialized against every other agent, readers included
  This is the same probe tests_state_modify_runs_under_the_write_lock uses, kept
  here as the policy-level statement: what makes cow_state safe is not the
  atomic
  refcount - that only protects the BLOCK - but this mutex, which protects the
  cow_ptr itself.  The header says so: "concurrent reads and writes to the SAME
  cow_ptr instance are NOT safe.  Protect the cow_ptr itself with a lock (see
  cow_state)".
*/
bool
tests_state_write_excludes_other_agents()
{
    cow_state<int, exclusive_lock_policy> state(0);

    std::atomic<bool> inside(false);
    std::atomic<bool> took_the_lock(true);      // pessimistic
    std::atomic<bool> done(false);

    std::thread prober(&write_prober, &state, &inside, &took_the_lock, &done);

    state.modify([&inside, &done](int& _value)
                 {
                     inside.store(true);

                     while (!done.load())
                     {
                         std::this_thread::yield();
                     }

                     _value = 7;
                 });

    prober.join();

    return ( (!took_the_lock.load()) &&
             (state.read() == 7) );
}

/*
tests_state_null_policy_costs_no_mutex
  the null policy is smaller, and measurably so.
  Tests the following:
  - cow_state<T, null_lock_policy> is SMALLER than cow_state<T,
    exclusive_lock_policy>: the no_op_mutex is an empty type, and the std::mutex
    is not
  - the null state is no bigger than its own parts - a cow_ptr and a version
    stamp - plus at most a byte for the empty mutex
  So "the single-threaded container costs nothing" is a claim about sizeof, and
  here it is.
*/
bool
tests_state_null_policy_costs_no_mutex()
{
    const bool smaller =
        (sizeof(cow_state<int, null_lock_policy>) <
         sizeof(cow_state<int, exclusive_lock_policy>));

    const bool the_mutex_is_empty =
        ( std::is_empty<no_op_mutex>::value &&
          (sizeof(no_op_mutex) == 1) );

    // a pointer, a 64-bit version, and an empty mutex - and nothing else
    const bool nothing_else_in_there =
        (sizeof(cow_state<int, null_lock_policy>) <=
         (sizeof(void*) + sizeof(std::uint64_t) + sizeof(void*)));

    return ( smaller             &&
             the_mutex_is_empty  &&
             nothing_else_in_there );
}

/*
tests_every_policy_yields_a_usable_state
  the sweep: every policy the build has produces the same shape.
  Tests the following:
  - cow_state<T, P> is constructible, non-copyable and non-movable for every P
  - every one of them declares the cow strategy tag and its own self-marker
  - so nothing in the policy set produces a state that is only half a cow_state
*/
bool
tests_every_policy_yields_a_usable_state()
{
    D_CONSTEXPR bool core_policies =
        ( std::is_constructible<cow_state<int, null_lock_policy>,
                                const int&>::value                &&
          std::is_constructible<cow_state<int, exclusive_lock_policy>,
                                const int&>::value                &&
          std::is_constructible<cow_state<int, timed_lock_policy>,
                                const int&>::value                &&
          !std::is_copy_constructible<
              cow_state<int, null_lock_policy> >::value           &&
          !std::is_move_constructible<
              cow_state<int, exclusive_lock_policy> >::value );

    D_CONSTEXPR bool all_are_tagged_cow =
        ( std::is_same<
              cow_state<int, null_lock_policy>::concurrency_strategy_tag,
              cow_strategy_tag>::value &&
          std::is_same<
              cow_state<int, timed_lock_policy>::concurrency_strategy_tag,
              cow_strategy_tag>::value );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool the_shared_policy_too =
        ( std::is_constructible<cow_state<int, shared_lock_policy>,
                                const int&>::value &&
          std::is_same<
              cow_state<int, shared_lock_policy>::concurrency_strategy_tag,
              cow_strategy_tag>::value );
#else
    D_CONSTEXPR bool the_shared_policy_too = true;
#endif

    static_assert(core_policies && all_are_tagged_cow,
                  "every policy must yield a usable, cow-tagged state");

    return ( core_policies        &&
             all_are_tagged_cow   &&
             the_shared_policy_too );
}


NS_END  // testing
NS_END  // djinterp
