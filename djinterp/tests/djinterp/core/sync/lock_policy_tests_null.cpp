/*******************************************************************************
* djinterp [core]                                     lock_policy_tests_null.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_null.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       lock_policy_tests_null.cpp
//   Section II: the null policy - no_op_mutex, no_op_guard, null_lock_policy.
//
//   The null policy is the zero-cost default: it lets the same generic code run
// single-threaded with no locking overhead, by substituting a mutex and guard
// whose every operation compiles to nothing.  This is the only part of
// lock_policy.hpp with any run-time surface at all - no_op_mutex's operations -
// so the section exercises those (they must be callable and try_lock must
// always
// succeed), and then pins the two structural facts that matter downstream: the
// guard is non-copyable AND non-movable (the root cause of the lock_guard
// suite's null-policy move finding), and the policy wires the no-op types into
// the read/write/mutex aliases with all descriptors false.

// djinterp
#include "lock_policy_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_no_op_mutex_operations_are_callable
  every no_op_mutex operation exists and runs.
  Tests the following:
  - lock(), unlock(), lock_shared() and unlock_shared() are all callable and do
    nothing observable (they compile to no-ops)
  - so the null mutex presents the full mutex-and-shared-mutex surface a policy
    guard might touch, without any of it costing anything
  The test simply calls each; completing without error IS the assertion.
*/
bool
tests_no_op_mutex_operations_are_callable()
{
    no_op_mutex mutex;

    mutex.lock();
    mutex.unlock();
    mutex.lock_shared();
    mutex.unlock_shared();

    // nothing to observe - these are no-ops by design
    return true;
}

/*
tests_no_op_mutex_try_lock_always_succeeds
  the null mutex never refuses a try-lock.
  Tests the following:
  - try_lock() returns true
  - a SECOND try_lock() also returns true - there is no state, so there is no
    contention to fail against
  This is the null policy's promise (it always "acquires", so single-threaded
  code proceeds) and its hazard (it provides no real mutual exclusion), pinned
  together.  It is the source of the same observation the lock_guard suite makes
  about scoped_try_lock<null_lock_policy>.
*/
bool
tests_no_op_mutex_try_lock_always_succeeds()
{
    no_op_mutex mutex;

    const bool first  = mutex.try_lock();
    const bool second = mutex.try_lock();

    return (first && second);
}

/*
tests_no_op_guard_is_constructible_from_the_mutex
  the guard takes a mutex reference, for interface parity with real guards.
  Tests the following:
  - no_op_guard is constructible from a no_op_mutex& (its explicit constructor)
  - it is NOT constructible from nothing: the mutex-reference constructor is the
    only way to make one, matching how a real RAII guard is handed its mutex
  So generic guard-construction code that writes `guard_type g(mutex);`
  works for the null guard exactly as for a real one.
*/
bool
tests_no_op_guard_is_constructible_from_the_mutex()
{
    D_CONSTEXPR bool from_mutex =
        std::is_constructible<no_op_guard, no_op_mutex&>::value;

    D_CONSTEXPR bool not_default =
        !std::is_default_constructible<no_op_guard>::value;

    static_assert(from_mutex,
                  "no_op_guard must be constructible from its mutex");

    return (from_mutex && not_default);
}

/*
tests_no_op_guard_is_not_copyable
  the guard cannot be copied.
  Tests the following:
  - no_op_guard is not copy-constructible or copy-assignable (D_DELETE)
  - it mirrors a real lock guard, which is non-copyable because two guards over
    one lock would both release it
*/
bool
tests_no_op_guard_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<no_op_guard>::value &&
          !std::is_copy_assignable<no_op_guard>::value );

    static_assert(no_copy, "no_op_guard must not be copyable");

    return no_copy;
}

/*
tests_no_op_guard_is_not_movable
  the guard cannot be moved either - and this has downstream consequences.
  Tests the following:
  - no_op_guard is neither move-constructible nor move-assignable
  - because it deletes its copy operations AND declares a destructor, so the
    compiler generates no implicit move
  This is the ROOT CAUSE of a finding one layer up: because no_op_guard is
  non-movable, scoped_read_lock<null_lock_policy> and
  scoped_write_lock<null_lock_policy> are non-movable too (their `= default`
  move is implicitly deleted), even though the same guards move fine for every
  policy.  Pinning it here explains the surprise there.
*/
bool
tests_no_op_guard_is_not_movable()
{
    D_CONSTEXPR bool not_movable =
        ( !std::is_move_constructible<no_op_guard>::value &&
          !std::is_move_assignable<no_op_guard>::value );

    static_assert(not_movable, "no_op_guard must not be movable");

    return not_movable;
}

/*
tests_null_policy_aliases
  the null policy names the no-op types.
  Tests the following:
  - null_lock_policy::mutex_type is no_op_mutex
  - null_lock_policy::read_lock_type and write_lock_type are both no_op_guard
  So a guard built over this policy holds no-op machinery, which the optimizer
  erases.
*/
bool
tests_null_policy_aliases()
{
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<null_lock_policy::mutex_type, no_op_mutex>::value      &&
          std::is_same<null_lock_policy::read_lock_type, no_op_guard>::value  &&
          std::is_same<null_lock_policy::write_lock_type,
                       no_op_guard>::value );

    static_assert(aliases_ok,
                  "null_lock_policy must name the no-op types");

    return aliases_ok;
}

/*
tests_null_policy_read_and_write_are_the_same_guard
  under the null policy, reading and writing use the same (no-op) guard.
  Tests the following:
  - read_lock_type and write_lock_type are the identical type
  - which they must be: with no real locking, there is no reader/writer
    distinction to make
  This mirrors the exclusive policy (where read and write are also identical, an
  exclusive lock) and contrasts with the shared policies (where they differ).
*/
bool
tests_null_policy_read_and_write_are_the_same_guard()
{
    D_CONSTEXPR bool same =
        std::is_same<null_lock_policy::read_lock_type,
                     null_lock_policy::write_lock_type>::value;

    static_assert(same,
                  "null policy read and write guards must be identical");

    return same;
}

/*
tests_null_policy_descriptors_are_all_false
  the null policy advertises no guarantees.
  Tests the following:
  - is_threadsafe, is_shared and is_timed are all false
  - and they are usable in a constant expression (static_assert), confirming
    they are compile-time constants - the header spells them `static const
    bool`,
    the C++98-friendly form, rather than the `static constexpr bool` the newer
    policies use
  So a trait querying this policy learns it offers no thread-safety, no sharing
  and no timing - exactly the single-threaded contract.
*/
bool
tests_null_policy_descriptors_are_all_false()
{
    static_assert(!null_lock_policy::is_threadsafe,
                  "null policy is not thread-safe");
    static_assert(!null_lock_policy::is_shared,
                  "null policy is not shared");
    static_assert(!null_lock_policy::is_timed,
                  "null policy is not timed");

    return ( (!null_lock_policy::is_threadsafe) &&
             (!null_lock_policy::is_shared)     &&
             (!null_lock_policy::is_timed) );
}

/*
tests_null_policy_level_is_none
  the null policy sits at the bottom of the hierarchy.
  Tests the following:
  - null_lock_policy::level is thread_safety_leavel::none
  - the member exists (it is gated to C++11+, and the suite floors at C++11) and
    is usable in a constant expression
  So select_lock_policy<none> and this policy's own level agree - the round trip
  the selector section checks from the other side.
*/
bool
tests_null_policy_level_is_none()
{
    static_assert(null_lock_policy::level == thread_safety_leavel::none,
                  "null policy's level must be none");

    return (null_lock_policy::level == thread_safety_leavel::none);
}


NS_END  // testing
NS_END  // djinterp
