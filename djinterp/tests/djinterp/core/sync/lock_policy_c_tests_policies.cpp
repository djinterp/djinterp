/*******************************************************************************
* djinterp [core]                               lock_policy_c_tests_policies.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_policies.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                  lock_policy_c_tests_policies.cpp
//   Section III: the C-backed policy structs - c_exclusive, c_shared,
// c_recursive (and, via the spinlock section, c_spinlock).
//
// Each is a policy in the same shape as the base header's: type aliases
// (mutex_type, read_lock_type, write_lock_type) plus boolean descriptors and a
// `level`.  The section pins those, and two structural facts worth calling out:
// c_shared is the only C policy whose read and write locks DIFFER (a shared
// reader guard vs an exclusive writer guard), and THREE policies - c_exclusive,
// c_recursive and c_spinlock - all report the SAME level, `exclusive`, since
// the level enum has no recursive or spin rung.
//
// The `level` members use thread_safety_level, the CORRECTLY spelled alias from
// the base header, and are gated to C++11+ there; the suite floors at C++11, so
// they are always present.  All static - no threads in this section.

// djinterp
#include "lock_policy_c_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_c_exclusive_aliases
  c_exclusive names the platform mutex and a c_lock_guard over it.
  Tests the following:
  - read_lock_type and write_lock_type are both
    c_lock_guard<c_exclusive::mutex_type>
  So both reading and writing take an exclusive c_lock_guard - there is no
  shared path.
*/
bool
tests_c_exclusive_aliases()
{
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<c_exclusive::read_lock_type,
                       c_lock_guard<c_exclusive::mutex_type> >::value &&
          std::is_same<c_exclusive::write_lock_type,
                       c_lock_guard<c_exclusive::mutex_type> >::value );

    static_assert(aliases_ok,
                  "c_exclusive must use c_lock_guard for read and write");

    return aliases_ok;
}

/*
tests_c_exclusive_read_and_write_are_identical
  reading and writing use the same exclusive guard.
  Tests the following:
  - read_lock_type and write_lock_type are the identical type
  The defining property of an exclusive policy: no reader concurrency.
*/
bool
tests_c_exclusive_read_and_write_are_identical()
{
    D_CONSTEXPR bool same =
        std::is_same<c_exclusive::read_lock_type,
                     c_exclusive::write_lock_type>::value;

    static_assert(same,
                  "c_exclusive read and write locks must be identical");

    return same;
}

/*
tests_c_exclusive_descriptors
  c_exclusive advertises exclusive thread-safety.
  Tests the following:
  - is_threadsafe is true, is_shared is false, is_timed is false
  - usable in a constant expression (the C policies spell these
    `static const bool`, the C++98-friendly form)
*/
bool
tests_c_exclusive_descriptors()
{
    static_assert(c_exclusive::is_threadsafe,
                  "c_exclusive is thread-safe");
    static_assert(!c_exclusive::is_shared,
                  "c_exclusive is not shared");
    static_assert(!c_exclusive::is_timed,
                  "c_exclusive is not timed");

    return ( c_exclusive::is_threadsafe    &&
             (!c_exclusive::is_shared)     &&
             (!c_exclusive::is_timed) );
}

/*
tests_c_exclusive_level_is_exclusive
  c_exclusive sits at the `exclusive` level.
  Tests the following:
  - c_exclusive::level is thread_safety_level::exclusive
*/
bool
tests_c_exclusive_level_is_exclusive()
{
    static_assert(c_exclusive::level == thread_safety_level::exclusive,
                  "c_exclusive's level must be exclusive");

    return (c_exclusive::level == thread_safety_level::exclusive);
}

/*
tests_c_shared_aliases
  c_shared names an rwlock, with a shared reader and an exclusive writer.
  Tests the following:
  - read_lock_type is c_shared_guard<c_shared::mutex_type> (the shared path)
  - write_lock_type is c_lock_guard<c_shared::mutex_type> (the exclusive path)
  So a reader takes the rwlock shared while a writer takes it exclusively -
  exactly the reader/writer split.
*/
bool
tests_c_shared_aliases()
{
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<c_shared::read_lock_type,
                       c_shared_guard<c_shared::mutex_type> >::value &&
          std::is_same<c_shared::write_lock_type,
                       c_lock_guard<c_shared::mutex_type> >::value );

    static_assert(aliases_ok,
                  "c_shared must use c_shared_guard to read, c_lock_guard to "
                  "write");

    return aliases_ok;
}

/*
tests_c_shared_read_and_write_differ
  c_shared is the one C policy whose read and write locks differ.
  Tests the following:
  - read_lock_type and write_lock_type are NOT the same type
  - which is the structural signature of reader/writer locking, and the contrast
    with c_exclusive, c_recursive and c_spinlock, where they are identical
*/
bool
tests_c_shared_read_and_write_differ()
{
    D_CONSTEXPR bool differ =
        !std::is_same<c_shared::read_lock_type,
                      c_shared::write_lock_type>::value;

    static_assert(differ,
                  "c_shared read and write locks must differ");

    return differ;
}

/*
tests_c_shared_descriptors
  c_shared advertises SHARED thread-safety.
  Tests the following:
  - is_threadsafe is true
  - is_shared is TRUE - the only C policy for which it is
  - is_timed is false (none of the C policies is timed)
*/
bool
tests_c_shared_descriptors()
{
    static_assert(c_shared::is_threadsafe, "c_shared is thread-safe");
    static_assert(c_shared::is_shared,     "c_shared IS shared");
    static_assert(!c_shared::is_timed,     "c_shared is not timed");

    return ( c_shared::is_threadsafe &&
             c_shared::is_shared     &&
             (!c_shared::is_timed) );
}

/*
tests_c_shared_level_is_shared
  c_shared sits at the `shared` level.
  Tests the following:
  - c_shared::level is thread_safety_level::shared
  - which outranks exclusive, matching that a shared lock offers everything an
    exclusive one does plus reader concurrency
*/
bool
tests_c_shared_level_is_shared()
{
    static_assert(c_shared::level == thread_safety_level::shared,
                  "c_shared's level must be shared");

    const bool above_exclusive =
        ( static_cast<int>(c_shared::level) >
          static_cast<int>(c_exclusive::level) );

    return ( (c_shared::level == thread_safety_level::shared) &&
             above_exclusive );
}

/*
tests_c_recursive_aliases_and_descriptors
  c_recursive is an exclusive policy over a recursive mutex.
  Tests the following:
  - read_lock_type and write_lock_type are both
    c_lock_guard<c_recursive::mutex_type>, and identical to each other
  - is_threadsafe is true, is_shared is false, is_timed is false
  Structurally it looks just like c_exclusive - same guard, same descriptors -
  and only its mutex_type (a recursive mutex) and its RUN-TIME reentrancy set it
  apart, which the mutex section covers.
*/
bool
tests_c_recursive_aliases_and_descriptors()
{
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<c_recursive::read_lock_type,
                       c_lock_guard<c_recursive::mutex_type> >::value &&
          std::is_same<c_recursive::write_lock_type,
                       c_lock_guard<c_recursive::mutex_type> >::value &&
          std::is_same<c_recursive::read_lock_type,
                       c_recursive::write_lock_type>::value );

    static_assert(aliases_ok,
                  "c_recursive must use one c_lock_guard for read and write");

    const bool descriptors_ok =
        ( c_recursive::is_threadsafe    &&
          (!c_recursive::is_shared)     &&
          (!c_recursive::is_timed) );

    return (aliases_ok && descriptors_ok);
}

/*
tests_c_recursive_level_is_exclusive
  c_recursive reports the `exclusive` level - there is no recursive level.
  Tests the following:
  - c_recursive::level is thread_safety_level::exclusive
  - so the level does NOT record recursiveness; a caller cannot tell c_recursive
    from c_exclusive by level alone
  The recursiveness lives in the mutex's behaviour, not in the policy's
  descriptors - a deliberate limitation of the level vocabulary, pinned.
*/
bool
tests_c_recursive_level_is_exclusive()
{
    static_assert(c_recursive::level == thread_safety_level::exclusive,
                  "c_recursive's level must be exclusive");

    return (c_recursive::level == thread_safety_level::exclusive);
}

/*
tests_three_policies_share_the_exclusive_level
  three distinct policies all report level `exclusive`.
  Tests the following:
  - c_exclusive, c_recursive and (at C++11+) c_spinlock all have level
    thread_safety_level::exclusive
  - so the level is many-to-one here: it groups these three under one rung, and
    their real differences (recursiveness, busy-waiting) are not expressible as
    a level
  Analogous to the base header's none/atomic_only both mapping to null -
  distinct concepts, one label.
*/
bool
tests_three_policies_share_the_exclusive_level()
{
    const bool exclusive_and_recursive =
        ( (c_exclusive::level == thread_safety_level::exclusive) &&
          (c_recursive::level == thread_safety_level::exclusive) );

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const bool spin_too =
        (c_spinlock::level == thread_safety_level::exclusive);
#else
    const bool spin_too = true;
#endif

    return (exclusive_and_recursive && spin_too);
}


NS_END  // testing
NS_END  // djinterp
