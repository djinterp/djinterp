/*******************************************************************************
* djinterp [core]                                lock_policy_tests_exclusive.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_exclusive.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                  lock_policy_tests_exclusive.cpp
//   Section III: exclusive_lock_policy - std::mutex, no reader concurrency.
//
//   The first real policy: it wraps std::mutex, and both its read and write
// lock
// types are std::unique_lock<std::mutex> - so reading and writing take the SAME
// exclusive lock, and there is no reader concurrency.  The section pins the
// aliases, the read==write identity that defines "exclusive", the descriptors
// (thread-safe, not shared, not timed), and the level.  It exists from C++11,
// the suite's floor, so no gating is needed.

// std
#include <mutex>
// djinterp
#include "lock_policy_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_exclusive_aliases
  the policy names std::mutex and its unique_lock.
  Tests the following:
  - mutex_type is std::mutex
  - read_lock_type and write_lock_type are both std::unique_lock<std::mutex>
*/
bool
tests_exclusive_aliases()
{
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<exclusive_lock_policy::mutex_type,
                       std::mutex>::value                              &&
          std::is_same<exclusive_lock_policy::read_lock_type,
                       std::unique_lock<std::mutex> >::value           &&
          std::is_same<exclusive_lock_policy::write_lock_type,
                       std::unique_lock<std::mutex> >::value );

    static_assert(aliases_ok,
                  "exclusive_lock_policy must name std::mutex and unique_lock");

    return aliases_ok;
}

/*
tests_exclusive_read_and_write_are_identical
  reading and writing use the same exclusive lock.
  Tests the following:
  - read_lock_type and write_lock_type are the identical type
  - which is what "exclusive" means here: a reader and a writer contend for one
    lock, so readers do not run concurrently
  This is the defining contrast with the shared policies, where the two differ.
*/
bool
tests_exclusive_read_and_write_are_identical()
{
    D_CONSTEXPR bool same =
        std::is_same<exclusive_lock_policy::read_lock_type,
                     exclusive_lock_policy::write_lock_type>::value;

    static_assert(same,
                  "exclusive policy read and write locks must be identical");

    return same;
}

/*
tests_exclusive_descriptors
  the policy advertises exclusive thread-safety.
  Tests the following:
  - is_threadsafe is true
  - is_shared is false (no reader concurrency)
  - is_timed is false (no timeout support)
  - all usable in a constant expression (the newer policies spell these
    `static constexpr bool`)
*/
bool
tests_exclusive_descriptors()
{
    static_assert(exclusive_lock_policy::is_threadsafe,
                  "exclusive policy is thread-safe");
    static_assert(!exclusive_lock_policy::is_shared,
                  "exclusive policy is not shared");
    static_assert(!exclusive_lock_policy::is_timed,
                  "exclusive policy is not timed");

    return ( exclusive_lock_policy::is_threadsafe    &&
             (!exclusive_lock_policy::is_shared)     &&
             (!exclusive_lock_policy::is_timed) );
}

/*
tests_exclusive_level_is_exclusive
  the policy sits at the `exclusive` level.
  Tests the following:
  - exclusive_lock_policy::level is thread_safety_leavel::exclusive
  So select_lock_policy<exclusive> and this policy's own level agree.
*/
bool
tests_exclusive_level_is_exclusive()
{
    static_assert(
        exclusive_lock_policy::level == thread_safety_leavel::exclusive,
        "exclusive policy's level must be exclusive");

    return (exclusive_lock_policy::level ==
            thread_safety_leavel::exclusive);
}


NS_END  // testing
NS_END  // djinterp
