/*******************************************************************************
* djinterp [test]                            threadsafe_container_tests_base.cpp
*
*   Layer 1 - threadsafe_container_base (the CRTP mutex base).  Types and tag,
* lock acquisition, try-lock, the constexpr policy and monograph queries, and
* the copy/move contract (including the pinned real-mutex move finding).
* Defines the "base" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_base.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_base_types_and_tag
//   Verifies the base forwards the policy's types and self-tags as locked.
//   Tests the following:
//   - lock_policy_type / mutex_type / read_guard / write_guard alias policy
//   - concurrency_strategy_tag is locked_strategy_tag
bool
tests_base_types_and_tag()
{
    using base_e = threadsafe_container_base<tc_vec<tc_excl>, tc_excl>;
    D_TC_CHECK((std::is_same<base_e::lock_policy_type, tc_excl>::value));
    D_TC_CHECK((std::is_same<base_e::mutex_type,
                             tc_excl::mutex_type>::value));
    D_TC_CHECK((std::is_same<base_e::read_guard,
                             tc_excl::read_lock_type>::value));
    D_TC_CHECK((std::is_same<base_e::write_guard,
                             tc_excl::write_lock_type>::value));
    D_TC_CHECK((std::is_same<base_e::concurrency_strategy_tag,
                             locked_strategy_tag>::value));
    return true;
}

// tests_base_lock_acquisition
//   Verifies the RAII lock handles acquire and, crucially, RELEASE - a second
//   acquisition on the same non-recursive mutex would deadlock if the first
//   had not released on scope exit.
//   Tests the following:
//   - read_lock / write_lock acquire and release (sequential re-acquire)
//   - mutex() returns a stable reference to the one underlying mutex
bool
tests_base_lock_acquisition()
{
    tc_vec<tc_excl> c;

    { c.read_lock(); }                  // acquire + release
    { c.write_lock(); }
    { c.write_lock(); }                 // would hang if the prior never freed

    D_TC_CHECK(&c.mutex() == &c.mutex());

    // null policy: the no-op guards are equally valid
    tc_vec<tc_null> cn;
    { cn.read_lock(); }
    { cn.write_lock(); }
    return true;
}

// tests_base_try_write_lock
//   Verifies the non-blocking exclusive lock succeeds when uncontended.
//   Tests the following:
//   - try_write_lock owns the lock under the null policy
//   - try_write_lock owns the lock under the exclusive policy (uncontended)
bool
tests_base_try_write_lock()
{
    tc_vec<tc_null> cn;
    { D_TC_CHECK(cn.try_write_lock().owns_lock()); }

    tc_vec<tc_excl> ce;
    { D_TC_CHECK(ce.try_write_lock().owns_lock()); }

    // released on scope exit, so a blocking write lock still acquires
    { ce.write_lock(); }
    return true;
}

// tests_base_policy_queries_null
//   Verifies the constexpr policy descriptors under the null policy.
//   Tests the following:
//   - not threadsafe, not shared, not timed
//   - safety level is none
bool
tests_base_policy_queries_null()
{
    D_TC_CHECK(tc_vec<tc_null>::safety_level() ==
               thread_safety_level::none);
    D_TC_CHECK(!tc_vec<tc_null>::is_threadsafe());
    D_TC_CHECK(!tc_vec<tc_null>::supports_shared());
    D_TC_CHECK(!tc_vec<tc_null>::supports_timed());
    return true;
}

// tests_base_policy_queries_locking
//   Verifies the constexpr descriptors under the real locking policies.
//   Tests the following:
//   - exclusive: threadsafe, not shared, not timed, level exclusive
//   - shared (C++17): threadsafe, shared, level shared
bool
tests_base_policy_queries_locking()
{
    D_TC_CHECK(tc_vec<tc_excl>::is_threadsafe());
    D_TC_CHECK(!tc_vec<tc_excl>::supports_shared());
    D_TC_CHECK(!tc_vec<tc_excl>::supports_timed());
    D_TC_CHECK(tc_vec<tc_excl>::safety_level() ==
               thread_safety_level::exclusive);

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_TC_CHECK(tc_vec<tc_shared>::is_threadsafe());
    D_TC_CHECK(tc_vec<tc_shared>::supports_shared());
    D_TC_CHECK(tc_vec<tc_shared>::safety_level() ==
               thread_safety_level::shared);
#endif
    return true;
}

// tests_base_monograph_queries
//   Verifies the monograph access-axis queries and their identities with the
//   policy flags.
//   Tests the following:
//   - is_blocking() == is_threadsafe(); false for null, true for exclusive
//   - permits_concurrent_readers() == supports_shared(); true only for shared
//   - offers_snapshot_iteration() and has_individual_reclamation() always true
bool
tests_base_monograph_queries()
{
    D_TC_CHECK(tc_vec<tc_null>::is_blocking() ==
               tc_vec<tc_null>::is_threadsafe());
    D_TC_CHECK(!tc_vec<tc_null>::is_blocking());
    D_TC_CHECK(tc_vec<tc_excl>::is_blocking());

    D_TC_CHECK(tc_vec<tc_null>::permits_concurrent_readers() ==
               tc_vec<tc_null>::supports_shared());
    D_TC_CHECK(!tc_vec<tc_excl>::permits_concurrent_readers());

    D_TC_CHECK(tc_vec<tc_null>::offers_snapshot_iteration());
    D_TC_CHECK(tc_vec<tc_excl>::offers_snapshot_iteration());
    D_TC_CHECK(tc_vec<tc_null>::has_individual_reclamation());
    D_TC_CHECK(tc_vec<tc_excl>::has_individual_reclamation());

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_TC_CHECK(tc_vec<tc_shared>::permits_concurrent_readers());
#endif
    return true;
}

// tests_base_copy_move
//   Verifies the copy/move contract.  A mutex-holding base is never copyable.
//   It is movable only when the mutex itself is movable (the null policy's
//   no_op_mutex); with a real std::mutex the `= default` move constructor is
//   deleted, so the container is NOT movable - pinned here as it stands.
//   Tests the following:
//   - never copy-constructible or copy-assignable
//   - move-constructible under the null policy
//   - NOT move-constructible under the exclusive policy (pinned finding)
bool
tests_base_copy_move()
{
    D_TC_CHECK(!std::is_copy_constructible<tc_vec<tc_null>>::value);
    D_TC_CHECK(!std::is_copy_constructible<tc_vec<tc_excl>>::value);
    D_TC_CHECK(!std::is_copy_assignable<tc_vec<tc_null>>::value);

    D_TC_CHECK(std::is_move_constructible<tc_vec<tc_null>>::value);

    // pinned: real mutex => non-movable (expected to invert if the base gains
    // a user-defined move ctor that fresh-constructs the mutex)
    D_TC_CHECK(!std::is_move_constructible<tc_vec<tc_excl>>::value);
    return true;
}

NS_END  // testing
NS_END  // djinterp
