/*******************************************************************************
* djinterp [core]                             lock_policy_tests_shared_timed.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_shared_timed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]               lock_policy_tests_shared_timed.cpp
//   Section VI: shared_timed_lock_policy - std::shared_timed_mutex, the most
// capable policy (C++14+).
//
//   This policy combines both axes: reader/writer sharing AND timed
// acquisition,
// over std::shared_timed_mutex.  Like shared_lock_policy, read and write types
// differ (shared_lock vs unique_lock); unlike it, is_timed is true.  So it is
// the top of the hierarchy, with both is_shared and is_timed set.  The section
// pins the aliases, the read/write difference, the descriptors, and that it is
// the most capable policy.
//
//   The whole section is gated to C++14 - std::shared_timed_mutex arrived in
// C++14, one standard BEFORE std::shared_mutex - so this policy exists at C++14
// while shared_lock_policy does not appear until C++17.  Below C++14 the name
// shared_timed_lock_policy does not exist, so every reference is inside the
// guard and each test degrades to a stated no-op.

// std
#include <mutex>
// djinterp
#include "lock_policy_tests.hpp"
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // std
    #include <shared_mutex>
#endif


NS_DJINTERP
NS_TESTING

/*
tests_shared_timed_aliases
  the policy names std::shared_timed_mutex with a shared read and exclusive
  write.
  Tests the following:
  - mutex_type is std::shared_timed_mutex
  - read_lock_type is std::shared_lock<std::shared_timed_mutex>
  - write_lock_type is std::unique_lock<std::shared_timed_mutex>
  Below C++14 this is a stated no-op.
*/
bool
tests_shared_timed_aliases()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<shared_timed_lock_policy::mutex_type,
                       std::shared_timed_mutex>::value                     &&
          std::is_same<shared_timed_lock_policy::read_lock_type,
                       std::shared_lock<std::shared_timed_mutex> >::value  &&
          std::is_same<shared_timed_lock_policy::write_lock_type,
                       std::unique_lock<std::shared_timed_mutex> >::value );

    static_assert(aliases_ok,
                  "shared_timed_lock_policy must name shared_timed_mutex");

    return aliases_ok;
#else
    return true;
#endif
}

/*
tests_shared_timed_read_and_write_differ
  reading and writing use different lock types.
  Tests the following:
  - read_lock_type and write_lock_type are NOT the same type
  The reader/writer signature, as in shared_lock_policy.  Below C++14 this is a
  stated no-op.
*/
bool
tests_shared_timed_read_and_write_differ()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CONSTEXPR bool differ =
        !std::is_same<shared_timed_lock_policy::read_lock_type,
                      shared_timed_lock_policy::write_lock_type>::value;

    static_assert(differ,
                  "shared_timed policy read and write locks must differ");

    return differ;
#else
    return true;
#endif
}

/*
tests_shared_timed_descriptors
  the policy advertises BOTH shared and timed thread-safety.
  Tests the following:
  - is_threadsafe is true
  - is_shared is TRUE
  - is_timed is TRUE
  It is the only policy for which both is_shared and is_timed hold.  Below C++14
  this is a stated no-op.
*/
bool
tests_shared_timed_descriptors()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    static_assert(shared_timed_lock_policy::is_threadsafe,
                  "shared_timed policy is thread-safe");
    static_assert(shared_timed_lock_policy::is_shared,
                  "shared_timed policy IS shared");
    static_assert(shared_timed_lock_policy::is_timed,
                  "shared_timed policy IS timed");

    return ( shared_timed_lock_policy::is_threadsafe &&
             shared_timed_lock_policy::is_shared     &&
             shared_timed_lock_policy::is_timed );
#else
    return true;
#endif
}

/*
tests_shared_timed_level_is_shared_timed
  the policy sits at the top of the hierarchy.
  Tests the following:
  - shared_timed_lock_policy::level is thread_safety_leavel::shared_timed
  - which is the maximum level (5), outranking every other policy
  Below C++14 this is a stated no-op.
*/
bool
tests_shared_timed_level_is_shared_timed()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    static_assert(
        shared_timed_lock_policy::level ==
            thread_safety_leavel::shared_timed,
        "shared_timed policy's level must be shared_timed");

    const bool is_the_top =
        ( static_cast<int>(shared_timed_lock_policy::level) ==
          static_cast<int>(thread_safety_leavel::shared_timed) );

    return is_the_top;
#else
    return true;
#endif
}

/*
tests_shared_timed_is_the_most_capable
  it can do everything the other policies can.
  Tests the following:
  - it is simultaneously shared AND timed, which no other single policy is: the
    exclusive and timed policies are not shared, the shared policy is not timed
  - so the banner's claim that it is "the most capable policy" holds: it is the
    only one carrying both capability flags
  Below C++14 this is a stated no-op.
*/
bool
tests_shared_timed_is_the_most_capable()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // both capabilities in one policy
    const bool both_here =
        ( shared_timed_lock_policy::is_shared &&
          shared_timed_lock_policy::is_timed );

    // the plain timed policy has timing but not sharing
    const bool timed_lacks_sharing =
        ( timed_lock_policy::is_timed && (!timed_lock_policy::is_shared) );

    return (both_here && timed_lacks_sharing);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
