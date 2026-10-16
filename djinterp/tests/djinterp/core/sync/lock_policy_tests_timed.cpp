/*******************************************************************************
* djinterp [core]                                    lock_policy_tests_timed.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_timed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                      lock_policy_tests_timed.cpp
//   Section IV: timed_lock_policy - std::timed_mutex, exclusive with timeouts.
//
//   Like the exclusive policy, but over std::timed_mutex, whose try_lock_for /
// try_lock_until enable timed acquisition; is_timed is true here where it was
// false there.  Reading and writing still use the same exclusive lock (a
// unique_lock<timed_mutex>) - the timeout capability is orthogonal to the
// reader/writer question.  Exists from C++11, the suite's floor.

// std
#include <mutex>
// djinterp
#include "lock_policy_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_timed_aliases
  the policy names std::timed_mutex and its unique_lock.
  Tests the following:
  - mutex_type is std::timed_mutex
  - read_lock_type and write_lock_type are both
    std::unique_lock<std::timed_mutex>
  The timed_mutex is what supplies try_lock_for, which the timed lock guard one
  layer up relies on.
*/
bool
tests_timed_aliases()
{
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<timed_lock_policy::mutex_type,
                       std::timed_mutex>::value                        &&
          std::is_same<timed_lock_policy::read_lock_type,
                       std::unique_lock<std::timed_mutex> >::value     &&
          std::is_same<timed_lock_policy::write_lock_type,
                       std::unique_lock<std::timed_mutex> >::value );

    static_assert(aliases_ok,
                  "timed_lock_policy must name timed_mutex + unique_lock");

    return aliases_ok;
}

/*
tests_timed_read_and_write_are_identical
  reading and writing use the same exclusive (timed) lock.
  Tests the following:
  - read_lock_type and write_lock_type are the identical type
  So the timed policy is exclusive in the same sense as the exclusive policy -
  the timeout support does not introduce reader concurrency.
*/
bool
tests_timed_read_and_write_are_identical()
{
    D_CONSTEXPR bool same =
        std::is_same<timed_lock_policy::read_lock_type,
                     timed_lock_policy::write_lock_type>::value;

    static_assert(same,
                  "timed policy read and write locks must be identical");

    return same;
}

/*
tests_timed_descriptors
  the policy advertises exclusive, TIMED thread-safety.
  Tests the following:
  - is_threadsafe is true
  - is_shared is false
  - is_timed is TRUE - the one descriptor that separates this policy from the
    plain exclusive one
*/
bool
tests_timed_descriptors()
{
    static_assert(timed_lock_policy::is_threadsafe,
                  "timed policy is thread-safe");
    static_assert(!timed_lock_policy::is_shared,
                  "timed policy is not shared");
    static_assert(timed_lock_policy::is_timed,
                  "timed policy IS timed");

    return ( timed_lock_policy::is_threadsafe    &&
             (!timed_lock_policy::is_shared)     &&
             timed_lock_policy::is_timed );
}

/*
tests_timed_level_is_timed
  the policy sits at the `timed` level.
  Tests the following:
  - timed_lock_policy::level is thread_safety_leavel::timed
  - which is strictly above exclusive in the hierarchy, matching that timed is a
    superset of exclusive (it can do everything exclusive can, plus timeouts)
*/
bool
tests_timed_level_is_timed()
{
    static_assert(timed_lock_policy::level == thread_safety_leavel::timed,
                  "timed policy's level must be timed");

    // and it outranks exclusive
    const bool above_exclusive =
        ( static_cast<int>(timed_lock_policy::level) >
          static_cast<int>(exclusive_lock_policy::level) );

    return ( (timed_lock_policy::level == thread_safety_leavel::timed) &&
             above_exclusive );
}


NS_END  // testing
NS_END  // djinterp
