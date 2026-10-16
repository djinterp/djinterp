/*******************************************************************************
* djinterp [core]                                   lock_policy_tests_shared.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_shared.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     lock_policy_tests_shared.cpp
//   Section V: shared_lock_policy - std::shared_mutex, reader/writer locking
// (C++17+).
//
//   The first policy where reading and writing DIFFER: read_lock_type is a
// std::shared_lock (many readers may hold it at once) while write_lock_type is
// a
// std::unique_lock (writers are exclusive).  is_shared is true here for the
// first time.  The section pins the aliases, that read and write types differ,
// the descriptors, and the level.
//
//   The whole section is gated to C++17, because below that std::shared_mutex
// does not exist and NEITHER DOES shared_lock_policy - naming it would be a
// hard
// compile error, so every reference is inside the guard, and below C++17 each
// test degrades to a stated no-op.

// std
#include <mutex>
// djinterp
#include "lock_policy_tests.hpp"
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <shared_mutex>
#endif


NS_DJINTERP
NS_TESTING

/*
tests_shared_aliases
  the policy names std::shared_mutex with a shared read and exclusive write.
  Tests the following:
  - mutex_type is std::shared_mutex
  - read_lock_type is std::shared_lock<std::shared_mutex>
  - write_lock_type is std::unique_lock<std::shared_mutex>
  Below C++17 this is a stated no-op.
*/
bool
tests_shared_aliases()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<shared_lock_policy::mutex_type,
                       std::shared_mutex>::value                       &&
          std::is_same<shared_lock_policy::read_lock_type,
                       std::shared_lock<std::shared_mutex> >::value    &&
          std::is_same<shared_lock_policy::write_lock_type,
                       std::unique_lock<std::shared_mutex> >::value );

    static_assert(aliases_ok,
                  "shared_lock_policy must name shared_mutex, shared/unique");

    return aliases_ok;
#else
    return true;
#endif
}

/*
tests_shared_read_and_write_differ
  under a shared policy, reading and writing use DIFFERENT lock types.
  Tests the following:
  - read_lock_type and write_lock_type are NOT the same type
  - this is the structural signature of reader/writer locking, and the direct
    contrast with the exclusive and null policies where they are identical
  Below C++17 this is a stated no-op.
*/
bool
tests_shared_read_and_write_differ()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool differ =
        !std::is_same<shared_lock_policy::read_lock_type,
                      shared_lock_policy::write_lock_type>::value;

    static_assert(differ,
                  "shared policy read and write locks must differ");

    return differ;
#else
    return true;
#endif
}

/*
tests_shared_descriptors
  the policy advertises SHARED thread-safety.
  Tests the following:
  - is_threadsafe is true
  - is_shared is TRUE - the first policy for which it is
  - is_timed is false (this policy has no timeout support; shared_timed does)
  Below C++17 this is a stated no-op.
*/
bool
tests_shared_descriptors()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(shared_lock_policy::is_threadsafe,
                  "shared policy is thread-safe");
    static_assert(shared_lock_policy::is_shared,
                  "shared policy IS shared");
    static_assert(!shared_lock_policy::is_timed,
                  "shared policy is not timed");

    return ( shared_lock_policy::is_threadsafe &&
             shared_lock_policy::is_shared     &&
             (!shared_lock_policy::is_timed) );
#else
    return true;
#endif
}

/*
tests_shared_level_is_shared
  the policy sits at the `shared` level.
  Tests the following:
  - shared_lock_policy::level is thread_safety_leavel::shared
  - which outranks timed in the hierarchy
  Below C++17 this is a stated no-op.
*/
bool
tests_shared_level_is_shared()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(shared_lock_policy::level == thread_safety_leavel::shared,
                  "shared policy's level must be shared");

    const bool above_timed =
        ( static_cast<int>(shared_lock_policy::level) >
          static_cast<int>(timed_lock_policy::level) );

    return ( (shared_lock_policy::level == thread_safety_leavel::shared) &&
             above_timed );
#else
    return true;
#endif
}

/*
tests_shared_read_lock_is_a_shared_lock
  the read lock really is a shared_lock, so readers can coexist.
  Tests the following:
  - read_lock_type is specifically std::shared_lock<std::shared_mutex> - not a
    unique_lock, not some other wrapper
  This is the type-level guarantee behind reader concurrency; the lock_guard
  suite proves the concurrency itself at run time, but it rests on this alias
  being a shared_lock.  Below C++17 this is a stated no-op.
*/
bool
tests_shared_read_lock_is_a_shared_lock()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool is_shared_lock =
        std::is_same<shared_lock_policy::read_lock_type,
                     std::shared_lock<std::shared_mutex> >::value;

    static_assert(is_shared_lock,
                  "shared policy's read lock must be a std::shared_lock");

    return is_shared_lock;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
